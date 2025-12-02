// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "obs_scene_switcher.hpp"
#include "ui/plugin_properties.h"
#include "ui/plugin_dock.hpp"
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

	// 認証設定をロード
	reloadAuthConfig();
	loadConfig();

	auto &cfg = ConfigManager::instance();

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
	connectEventSub();
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

	// EventSubもついでに接続開始可能
	connectEventSub();
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

	accessToken_.clear();
	refreshToken_.clear();
	expiresAt_ = 0;
	authenticated_ = false;

	saveConfig();
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

void ObsSceneSwitcher::onRedemptionReceived(const std::string &rewardId, const std::string &userName,
					    const std::string &userInput)
{
	blog(LOG_INFO, "[SceneSwitcher] Redemption received: %s", rewardId.c_str());

	auto it = rewardSceneMap_.find(rewardId);
	if (it != rewardSceneMap_.end()) {
		switchScene(it->second);
	}
}

void ObsSceneSwitcher::switchScene(const std::string &sceneName)
{
	blog(LOG_INFO, "[SceneSwitcher] Switching scene to: %s", sceneName.c_str());

	// TODO: obs-frontend-api でシーン切り替え
}

void ObsSceneSwitcher::loadConfig()
{
	blog(LOG_INFO, "[SceneSwitcher] loadConfig()");

	auto &cfg = ConfigManager::instance();
	accessToken_ = cfg.getAccessToken();
	refreshToken_ = cfg.getRefreshToken();
	expiresAt_ = cfg.getTokenExpiresAt();
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
