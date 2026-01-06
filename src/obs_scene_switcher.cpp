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
#include "i18n/locale_manager.hpp"

#include <obs-frontend-api.h>

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
	blog(LOG_DEBUG, "[obs-scene-switcher] Initialized");

	sceneSwitcher_ = std::make_unique<SceneSwitcher>(this);  // SceneSwitcher の状態変更を UI に転送
	connect(sceneSwitcher_.get(), &SceneSwitcher::stateChanged, 
	        this, &ObsSceneSwitcher::onSceneSwitcherStateChanged);
}

ObsSceneSwitcher::~ObsSceneSwitcher()
{
	blog(LOG_DEBUG, "[obs-scene-switcher] Destroyed");
}

void ObsSceneSwitcher::start()
{
	blog(LOG_DEBUG, "[obs-scene-switcher] Initializing plugin");

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
	blog(LOG_DEBUG, "[obs-scene-switcher] Loaded %zu reward rules from config", rewardRules_.size());


        // 初回 or 未設定
	if (!cfg.isAuthValid()) {
		blog(LOG_DEBUG, "[obs-scene-switcher] No valid authentication ");
		authenticated_ = false;
		emit authenticationFailed();
		return;
	}

	// 期限切れならトークン更新
	if (cfg.isTokenExpired()) {
		blog(LOG_DEBUG, "[obs-scene-switcher] Token expired, attempting refresh");
		if (!TwitchOAuth::instance().refreshAccessToken()) {
			blog(LOG_ERROR, "[obs-scene-switcher] Token refresh failed");
			authenticated_ = false;
			emit authenticationFailed();
			return;
		}
		cfg.save();
		blog(LOG_DEBUG, "[obs-scene-switcher] Token refreshed successfully");
	}

	// 認証成功
	authenticated_ = true;
	blog(LOG_DEBUG, "[obs-scene-switcher] Authentication successful");
	emit authenticationSucceeded();
	
	// チャンネルポイント一覧を取得
	fetchRewardList();

	// OBS イベントコールバック登録
	setupObsCallbacks();
}

void ObsSceneSwitcher::stop()
{
	blog(LOG_DEBUG, "[obs-scene-switcher] Shutting down plugin");
	
	// OBS イベントコールバック解除
	removeObsCallbacks();
	
	disconnectEventSub();
}

void ObsSceneSwitcher::handleOAuthCallback(const std::string &code)
{
	blog(LOG_DEBUG, "[obs-scene-switcher] Received code: %s", code.c_str());

	if (!TwitchOAuth::instance().exchangeCodeForToken(code)) {
		blog(LOG_ERROR, "[obs-scene-switcher] OAuth token exchange failed");
		return;
	}

	// 結果を読み取る
	accessToken_ = TwitchOAuth::instance().getAccessToken();
	refreshToken_ = TwitchOAuth::instance().getRefreshToken();
	expiresAt_ = TwitchOAuth::instance().getExpiresAt();

	authenticated_ = true;

	saveConfig();

	blog(LOG_DEBUG, "[obs-scene-switcher] OAuth authentication successful");

	emit authenticationSucceeded();
	
	// チャンネルポイント一覧を取得
	fetchRewardList();
}

void ObsSceneSwitcher::startOAuthLogin()
{
	// ローカルHTTPサーバー起動して code を受け取れるようにする
	HttpServer::instance()->start(38915, [this](const std::string &code) { this->handleOAuthCallback(code); });

	TwitchOAuth::instance().startOAuthLogin();
}

void ObsSceneSwitcher::logout()
{
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
	
	emit loggedOut();
}

void ObsSceneSwitcher::connectEventSub()
{
	if (!authenticated_) {
		blog(LOG_WARNING, "[obs-scene-switcher] Cannot connect EventSub: not authenticated");
		return;
	}

	auto &cfg = ConfigManager::instance();

	const std::string &accessToken = cfg.getAccessToken();
	const std::string &broadcasterId = cfg.getBroadcasterUserId();
	const std::string &clientId = cfg.getClientId();

	if (accessToken.empty() || broadcasterId.empty() || clientId.empty()) {
		blog(LOG_WARNING, "[obs-scene-switcher] Cannot connect EventSub: missing credentials");
		return;
	}

	blog(LOG_DEBUG, "[obs-scene-switcher] Connecting to Twitch EventSub");
	EventSubClient::instance().start(accessToken, broadcasterId, clientId);

	eventsubConnected_ = true;
}

void ObsSceneSwitcher::disconnectEventSub()
{
	if (!eventsubConnected_) return;
        eventsubConnected_ = false;

	blog(LOG_DEBUG, "[obs-scene-switcher] Disconnecting from EventSub");
	EventSubClient::instance().stop();
}

void ObsSceneSwitcher::setEnabled(bool enabled)
{
	if (pluginEnabled_ == enabled)
		return;

	blog(LOG_DEBUG, "[obs-scene-switcher] Plugin %s", enabled ? "enabled" : "disabled");

	if (enabled) {
		// 認証済みの場合のみ接続
		if (isAuthenticated()) {
			pluginEnabled_ = true;
			connectEventSub();  // UI 状態更新（待機中）
			
			if (pluginDock_) {
				auto *mainWidget = pluginDock_->getWidget()->findChild<DockMainWidget*>();
				if (mainWidget) {
					mainWidget->updateState(Tr("SceneSwitcher.Status.Idle"));
				}
			}
		} else {
			blog(LOG_WARNING, "[obs-scene-switcher] Cannot enable: not authenticated");
			pluginEnabled_ = false; // 無効に戻す
			return;
		}
	} else {
		// 完全停止
		pluginEnabled_ = false;
		disconnectEventSub();
		
		if (pluginDock_) {
			auto *mainWidget = pluginDock_->getWidget()->findChild<DockMainWidget*>();
			if (mainWidget) {
				mainWidget->updateState(Tr("SceneSwitcher.Status.Disabled"));
				mainWidget->updateCountdown(-1);
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
	blog(LOG_DEBUG, "[obs-scene-switcher] Redemption received: %s", rewardId.c_str());
	
	// プラグインが無効の場合は無視
	if (!pluginEnabled_) {
		blog(LOG_DEBUG, "[obs-scene-switcher] Plugin disabled, ignoring redemption");
		return;
	}

	// 現在のシーン名を取得
	QString currentScene = sceneSwitcher_->getCurrentSceneName();
	std::string currentSceneStr = currentScene.toStdString();
	
	blog(LOG_DEBUG, "[obs-scene-switcher] Current scene: %s", currentSceneStr.c_str());

	// 上から順にルールを検索（最初の有効なマッチを優先）
	for (const auto &rule : rewardRules_) {
		if (rule.rewardId != rewardId)
			continue;

		// ルールが無効の場合は次のルールへ
		if (!rule.enabled) {
			blog(LOG_DEBUG, "[obs-scene-switcher] Rule for reward_id=%s is disabled, checking to next rule", rewardId.c_str());
			continue;
		}

		// ソースシーンのチェック
		// 空文字列または "Any" の場合は任意のシーンにマッチ
		bool sourceMatches = rule.sourceScene.empty() || 
		                     rule.sourceScene == "Any" || 
		                     rule.sourceScene == currentSceneStr;
		
		if (!sourceMatches) {
			blog(LOG_DEBUG, "[obs-scene-switcher] Source scene mismatch: rule requires '%s', current is '%s'", 
			     rule.sourceScene.c_str(), currentSceneStr.c_str());
			continue;
		}

		blog(LOG_DEBUG, "[obs-scene-switcher] Matched rule: %s -> %s (revert: %d sec)", 
		     rule.sourceScene.empty() || rule.sourceScene == "Any" ? "Any" : rule.sourceScene.c_str(),
		     rule.targetScene.c_str(), rule.revertSeconds);

		sceneSwitcher_->switchWithRevert(rule);
		return;  // 最初の有効なルールを実行したら終了
	}

	blog(LOG_WARNING, "[obs-scene-switcher] No enabled rule found for reward_id=%s (total rules: %zu)", 
	     rewardId.c_str(), rewardRules_.size());
}

void ObsSceneSwitcher::switchScene(const std::string &sceneName)
{
	blog(LOG_DEBUG, "[obs-scene-switcher] Switching scene to: %s", sceneName.c_str());
        
	sceneSwitcher_->switchScene(sceneName);
}

void ObsSceneSwitcher::setRewardRules(const std::vector<RewardRule> &rules)
{
	rewardRules_ = rules;

	blog(LOG_DEBUG, "[obs-scene-switcher] Loaded %zu rules", rewardRules_.size());
}

void ObsSceneSwitcher::onSceneSwitcherStateChanged(SceneSwitcher::State state, int remainingSeconds,
                                                    const QString &targetScene, const QString &originalScene)
{
	if (!pluginDock_)
		return;
	
	auto *mainWidget = pluginDock_->getWidget()->findChild<DockMainWidget*>();
	if (!mainWidget)
		return;
	
	QString stateText;
	switch (state) {
	case SceneSwitcher::State::Idle:
		stateText = pluginEnabled_ ? Tr("SceneSwitcher.Status.Idle") : Tr("SceneSwitcher.Status.Disabled");
		mainWidget->updateCountdown(-1);
		break;
	case SceneSwitcher::State::Switched:
		if (!targetScene.isEmpty()) {
			stateText = Tr("SceneSwitcher.Status.Switching").arg(targetScene);
		} else {
			stateText = Tr("SceneSwitcher.Status.Switching").arg("");
		}
		mainWidget->updateCountdown(remainingSeconds);
		break;
	case SceneSwitcher::State::Reverting:
		if (!targetScene.isEmpty()) {
			stateText = Tr("SceneSwitcher.Status.Reverting").arg(targetScene);
		} else {
			stateText = Tr("SceneSwitcher.Status.Reverting").arg("");
		}
		break;
	case SceneSwitcher::State::Suppressed:
		stateText = Tr("SceneSwitcher.Status.Suppressed");
		break;
	}
	
	mainWidget->updateState(stateText);
}

void ObsSceneSwitcher::loadConfig()
{
	auto &cfg = ConfigManager::instance();
	accessToken_ = cfg.getAccessToken();
	refreshToken_ = cfg.getRefreshToken();
	expiresAt_ = cfg.getTokenExpiresAt();
}

void ObsSceneSwitcher::fetchRewardList()
{
	if (!authenticated_) {
		blog(LOG_WARNING, "[obs-scene-switcher] Cannot fetch rewards: not authenticated");
		return;
	}
	
	blog(LOG_DEBUG, "[obs-scene-switcher] Fetching channel rewards list...");
	rewardList_ = TwitchOAuth::instance().fetchChannelRewards();
	blog(LOG_DEBUG, "[obs-scene-switcher] Fetched %zu rewards", rewardList_.size());
}

void ObsSceneSwitcher::saveConfig()
{
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

	blog(LOG_DEBUG, "[obs-scene-switcher] Auth config loaded (client_id=%s)", clientId_.empty() ? "(empty)" : "*****");
}

void ObsSceneSwitcher::setupObsCallbacks()
{
	obs_frontend_add_event_callback(
		[](enum obs_frontend_event event, void *private_data) {
			auto *self = static_cast<ObsSceneSwitcher*>(private_data);
			
			switch (event) {
			case OBS_FRONTEND_EVENT_STREAMING_STARTED:
				// 配信開始時：認証済みなら自動的に有効化
				if (self->isAuthenticated() && !self->isEnabled()) {
					blog(LOG_INFO, "[obs-scene-switcher] Streaming started - auto-enabling plugin");
					self->setEnabled(true);
				}
				break;
				
			case OBS_FRONTEND_EVENT_STREAMING_STOPPED:
				// 配信停止時：自動的に無効化
				if (self->isEnabled()) {
					blog(LOG_INFO, "[obs-scene-switcher] Streaming stopped - auto-disabling plugin");
					self->setEnabled(false);
				}
				break;
				
			default:
				break;
			}
		},
		this
	);
}

void ObsSceneSwitcher::removeObsCallbacks()
{
	obs_frontend_remove_event_callback(
		[](enum obs_frontend_event event, void *private_data) {
			auto *self = static_cast<ObsSceneSwitcher*>(private_data);
			
			switch (event) {
			case OBS_FRONTEND_EVENT_STREAMING_STARTED:
			case OBS_FRONTEND_EVENT_STREAMING_STOPPED:
				break;
			default:
				break;
			}
		},
		this
	);
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
