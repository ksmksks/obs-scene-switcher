// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "obs_scene_switcher.hpp"
#include "ui/plugin_properties.h"
#include "ui/plugin_dock.hpp"
#include "ui/dock_main_widget.hpp"
#include "obs/config_manager.hpp"
#include "oauth/http_server.hpp"
#include "eventsub/eventsub_client.hpp"

#ifdef _WIN32
#include <Windows.h>
#include <shellapi.h>
#endif

// OBS logging
extern "C" {
#include <obs-module.h>
}

ObsSceneSwitcher *ObsSceneSwitcher::s_instance_ = nullptr;

ObsSceneSwitcher *ObsSceneSwitcher::instance()
{
	if (!s_instance_) {
		s_instance_ = new ObsSceneSwitcher();
	}
	return s_instance_;
}

void ObsSceneSwitcher::destroy()
{
	if (s_instance_) {
		delete s_instance_;
		s_instance_ = nullptr;
	}
}

ObsSceneSwitcher::ObsSceneSwitcher()
{
	blog(LOG_INFO, "[SceneSwitcher] Initialized");

	sceneSwitcher_ = std::make_unique<SceneSwitcher>(this);  // SceneSwitcher の状態変更を UI に転送
	connect(sceneSwitcher_.get(), &SceneSwitcher::stateChanged, 
	        this, &ObsSceneSwitcher::onSceneSwitcherStateChanged);
}

ObsSceneSwitcher::~ObsSceneSwitcher()
{
	blog(LOG_INFO, "[SceneSwitcher] Destroyed");
}

void ObsSceneSwitcher::start()
{
	blog(LOG_INFO, "[SceneSwitcher] start() called");

	// Dock 生成・登録
	PluginDock *dock = PluginDock::instance();
	PluginDock::registerDock();

	// UI 更新のための signal-slot 接続
	QObject::connect(this, // ObsSceneSwitcher (signal発信元)
			 &ObsSceneSwitcher::authenticationSucceeded,
			 dock, // PluginDock (UI側)
			 &PluginDock::onAuthenticationSucceeded,
			 Qt::QueuedConnection // UIスレッド保証
	);
	QObject::connect(this, // ObsSceneSwitcher (signal発信元)
			 &ObsSceneSwitcher::authenticationFailed,
			 dock, // PluginDock (UI側)
			 &PluginDock::onAuthenticationFailed,
			 Qt::QueuedConnection // UIスレッド保証
	);
	QObject::connect(this, // ObsSceneSwitcher (signal発信元)
			 &ObsSceneSwitcher::loggedOut,
			 dock, // PluginDock (UI側)
			 &PluginDock::onLoggedOut,
			 Qt::QueuedConnection // UIスレッド保証
	);
	QObject::connect(&EventSubClient::instance(),
                         &EventSubClient::redemptionReceived, this,
			 &ObsSceneSwitcher::onRedemptionReceived,
			 Qt::QueuedConnection // UIスレッド保証
	);

	// 認証設定をロード
	reloadAuthConfig();
	loadConfig();

	auto &cfg = ConfigManager::instance();
	
	// ルールをロード
	setRewardRules(cfg.getRewardRules());
	blog(LOG_INFO, "[SceneSwitcher] Loaded %zu reward rules from config", rewardRules_.size());


        // 初回 or 未設定
	if (!cfg.isAuthValid()) {
		blog(LOG_INFO, "[SceneSwitcher] Initial startup or no auth config");
		authenticated_ = false;
		emit authenticationFailed();
		return;
	}

	// 期限切れならトークン更新
	if (cfg.isTokenExpired()) {
		blog(LOG_INFO, "[SceneSwitcher] Token expired. Trying refresh.");
		if (!TwitchOAuth::instance().refreshAccessToken()) {
			blog(LOG_ERROR, "[SceneSwitcher] Token refresh failed");
			authenticated_ = false;
			emit authenticationFailed();
			return;
		}
		cfg.save();
	}

	// 認証成功
	authenticated_ = true;
	emit authenticationSucceeded();
	
	// チャンネルポイント一覧を取得
	fetchRewardList();
}

void ObsSceneSwitcher::stop()
{
	blog(LOG_INFO, "[SceneSwitcher] stop() called");
	disconnectEventSub();
}

void ObsSceneSwitcher::handleOAuthCallback(const std::string &code)
{
	blog(LOG_INFO, "[OAuth] Received code: %s", code.c_str());

	if (!TwitchOAuth::instance().exchangeCodeForToken(code)) {
		blog(LOG_ERROR, "[OAuth] Failed to exchange token");
		return;
	}

	// 結果を読み取る
	accessToken_ = TwitchOAuth::instance().getAccessToken();
	refreshToken_ = TwitchOAuth::instance().getRefreshToken();
	expiresAt_ = TwitchOAuth::instance().getExpiresAt();

	authenticated_ = true;

	saveConfig();

	blog(LOG_INFO, "[OAuth] Authentication success!");

	emit authenticationSucceeded();
	
	// チャンネルポイント一覧を取得
	fetchRewardList();
}

void ObsSceneSwitcher::startOAuthLogin()
{
	blog(LOG_INFO, "[SceneSwitcher] startOAuthLogin()");

	// ローカルHTTPサーバー起動して code を受け取れるようにする
	HttpServer::instance()->start(38915, [this](const std::string &code) { this->handleOAuthCallback(code); });

	TwitchOAuth::instance().startOAuthLogin();
}

void ObsSceneSwitcher::logout()
{
	blog(LOG_INFO, "[SceneSwitcher] logout()");

	// プラグインを無効化（WebSocketを切断）
	if (pluginEnabled_) {
		setEnabled(false);
	}

	// 認証情報をクリア
	accessToken_.clear();
	refreshToken_.clear();
	expiresAt_ = 0;
	authenticated_ = false;

	// リワードリストをクリア
	rewardList_.clear();

	saveConfig();
	
	// ログアウト専用シグナルを送信（エラーダイアログは表示しない）
	emit loggedOut();
}

void ObsSceneSwitcher::connectEventSub()
{
	if (!authenticated_) {
		blog(LOG_WARNING, "[SceneSwitcher] connectEventSub() called while not authenticated");
		return;
	}

	auto &cfg = ConfigManager::instance();

	const std::string &accessToken = cfg.getAccessToken();
	const std::string &broadcasterId = cfg.getBroadcasterUserId();
	const std::string &clientId = cfg.getClientId();

	if (accessToken.empty() || broadcasterId.empty() || clientId.empty()) {
		blog(LOG_WARNING,
		     "[SceneSwitcher] connectEventSub() missing auth info "
		     "(access=%s, user_id=%s, client_id=%s)",
		     accessToken.empty() ? "empty" : "ok", broadcasterId.empty() ? "empty" : "ok",
		     clientId.empty() ? "empty" : "ok");
		return;
	}

	blog(LOG_INFO, "[SceneSwitcher] Starting EventSub client");
	EventSubClient::instance().start(accessToken, broadcasterId, clientId);

	eventsubConnected_ = true;
}

void ObsSceneSwitcher::disconnectEventSub()
{
	blog(LOG_INFO, "[SceneSwitcher] disconnectEventSub()");

	if (!eventsubConnected_) return;
        eventsubConnected_ = false;

	// EventSub クライアントの停止
	EventSubClient::instance().stop();
}

void ObsSceneSwitcher::setEnabled(bool enabled)
{
	if (pluginEnabled_ == enabled)
		return; // 状態変化なし

	blog(LOG_INFO, "[ObsSceneSwitcher] Plugin %s", enabled ? "ENABLED" : "DISABLED");

	if (enabled) {
		// 認証済みの場合のみ接続
		if (isAuthenticated()) {
			pluginEnabled_ = true;
			connectEventSub();  // UI 状態更新（待機中）
			
			if (pluginDock_) {
				auto *mainWidget = pluginDock_->getWidget()->findChild<DockMainWidget*>();
				if (mainWidget) {
					mainWidget->updateState("🟢 待機中");
				}
			}
		} else {
			blog(LOG_WARNING, "[ObsSceneSwitcher] Cannot enable: not authenticated");
			pluginEnabled_ = false; // 無効に戻す
			return;
		}
	} else {
		// 完全停止
		pluginEnabled_ = false;
		disconnectEventSub();  // UI 状態更新（無効）
		
		if (pluginDock_) {
			auto *mainWidget = pluginDock_->getWidget()->findChild<DockMainWidget*>();
			if (mainWidget) {
				mainWidget->updateState("⏸ 待機中（無効）");
				mainWidget->updateCountdown(-1);  // カウントダウンをクリア
			}
		}
	}

	// 設定に保存
	auto &cfg = ConfigManager::instance();
	cfg.setPluginEnabled(pluginEnabled_);
	cfg.save();

	emit enabledStateChanged(pluginEnabled_);
}

void ObsSceneSwitcher::onRedemptionReceived(const std::string &rewardId, const std::string &userName,
					    const std::string &userInput)
{
	blog(LOG_INFO, "[SceneSwitcher] Redemption received: %s", rewardId.c_str());  // プラグインが無効の場合は無視
	if (!pluginEnabled_) {
		blog(LOG_INFO, "[SceneSwitcher] Plugin disabled, ignoring redemption");
		return;
	}

	auto it = rewardRules_.find(rewardId);
	if (it == rewardRules_.end()) {
		blog(LOG_WARNING, "[SceneSwitcher] No rule found for reward_id=%s (total rules: %zu)", 
		     rewardId.c_str(), rewardRules_.size());
		return;
	}

	blog(LOG_INFO, "[SceneSwitcher] Matched rule: %s -> %s (revert: %d sec)", 
	     it->second.sourceScene.c_str(), it->second.targetScene.c_str(), it->second.revertSeconds);

	sceneSwitcher_->switchWithRevert(it->second);
}

void ObsSceneSwitcher::switchScene(const std::string &sceneName)
{
	blog(LOG_INFO, "[SceneSwitcher] Switching scene to: %s", sceneName.c_str());
        
	sceneSwitcher_->switchScene(sceneName);
}

void ObsSceneSwitcher::setRewardRules(const std::vector<RewardRule> &rules)
{
	rewardRules_.clear();

	for (const auto &rule : rules) {
		rewardRules_[rule.rewardId] = rule;
	}

	blog(LOG_INFO, "[SceneSwitcher] Loaded %zu rules", rewardRules_.size());
}

void ObsSceneSwitcher::onSceneSwitcherStateChanged(SceneSwitcher::State state, int remainingSeconds)
{
	if (!pluginDock_)
		return;
	
	auto *mainWidget = pluginDock_->getWidget()->findChild<DockMainWidget*>();
	if (!mainWidget)
		return;
	
	QString stateText;
	switch (state) {
	case SceneSwitcher::State::Idle:
		stateText = pluginEnabled_ ? "🟢 待機中" : "⏸ 待機中（無効）";
		mainWidget->updateCountdown(-1);
		break;
	case SceneSwitcher::State::Switched:
		stateText = "🔄 切替中";
		mainWidget->updateCountdown(remainingSeconds);
		break;
	case SceneSwitcher::State::Reverting:
		stateText = "⏱ 復帰中";
		break;
	case SceneSwitcher::State::Suppressed:
		stateText = "⚠ 抑制中";
		break;
	}
	
	mainWidget->updateState(stateText);
}

void ObsSceneSwitcher::loadConfig()
{
	blog(LOG_INFO, "[SceneSwitcher] loadConfig()");

	auto &cfg = ConfigManager::instance();
	accessToken_ = cfg.getAccessToken();
	refreshToken_ = cfg.getRefreshToken();
	expiresAt_ = cfg.getTokenExpiresAt();
}

void ObsSceneSwitcher::fetchRewardList()
{
	// 認証済みの場合のみチャンネルポイント一覧を取得
	// WebSocket接続は不要（Helix API使用）
	if (!authenticated_) {
		blog(LOG_WARNING, "[SceneSwitcher] Cannot fetch rewards: not authenticated");
		return;
	}
	
	blog(LOG_INFO, "[SceneSwitcher] Fetching channel rewards list...");
	rewardList_ = TwitchOAuth::instance().fetchChannelRewards();
	blog(LOG_INFO, "[SceneSwitcher] Fetched %zu rewards", rewardList_.size());
}

void ObsSceneSwitcher::saveConfig()
{
	blog(LOG_INFO, "[SceneSwitcher] saveConfig()");

	auto &cfg = ConfigManager::instance();
	cfg.setAccessToken(accessToken_);
	cfg.setRefreshToken(refreshToken_);
	cfg.setTokenExpiresAt(expiresAt_);
	cfg.save();
}

void ObsSceneSwitcher::reloadAuthConfig()
{
	auto &cfg = ConfigManager::instance();
	clientId_ = cfg.getClientId();
	clientSecret_ = cfg.getClientSecret();

	blog(LOG_INFO, "[SceneSwitcher] Auth config loaded (client_id=%s)", clientId_.empty() ? "(empty)" : "*****");
}

extern "C" {

void obs_scene_switcher_start(void)
{
	ObsSceneSwitcher::instance()->start();
}

void obs_scene_switcher_stop(void)
{
	ObsSceneSwitcher::instance()->stop();
}

} // extern "C"

extern "C" obs_properties_t *obs_scene_switcher_properties(void *)
{
	return PluginProperties::getProperties(nullptr);
}
