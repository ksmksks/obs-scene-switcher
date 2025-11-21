// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "obs_scene_switcher.hpp"
#include "ui/plugin_properties.h"
#include "ui/plugin_dock.hpp"
#include "obs/config_manager.hpp"

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

	// 認証設定をロード
	reloadAuthConfig();

	loadConfig();

	if (authenticated_) {
		dock->showMain();
		connectEventSub();
	}
}

void ObsSceneSwitcher::stop()
{
	blog(LOG_INFO, "[SceneSwitcher] stop() called");
	disconnectEventSub();
}

void ObsSceneSwitcher::handleOAuthCallback(const std::string &code)
{
	blog(LOG_INFO, "[OAuth] Received code: %s", code.c_str());

	// 結果を読み取る
	accessToken_ = oauth_->getAccessToken();
	refreshToken_ = oauth_->getRefreshToken();
	expiresAt_ = oauth_->getExpiresAt();

	authenticated_ = true;

	saveConfig();

	blog(LOG_INFO, "[OAuth] Authentication success!");
}

void ObsSceneSwitcher::startOAuthLogin()
{
	blog(LOG_INFO, "[SceneSwitcher] startOAuthLogin()");

	// 強制的に認証成功扱い
	emit authenticationSucceeded();
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
	blog(LOG_INFO, "[SceneSwitcher] connectEventSub()");

	// TODO: EventSub 接続
	eventsubConnected_ = true;
}

void ObsSceneSwitcher::disconnectEventSub()
{
	blog(LOG_INFO, "[SceneSwitcher] disconnectEventSub()");
	eventsubConnected_ = false;

	// TODO: WebSocket 切断
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

	// TODO: OBS 設定読み込み
}

void ObsSceneSwitcher::saveConfig()
{
	blog(LOG_INFO, "[SceneSwitcher] saveConfig()");

	// TODO: OBS 設定保存
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
