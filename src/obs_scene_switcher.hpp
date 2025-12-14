// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <QObject>

#include "obs/scene_switcher.hpp"
#include "eventsub/eventsub_client.hpp"
#include "oauth/twitch_oauth.hpp"

class TwitchOAuth;
class EventSubClient;
class SceneSwitcher;
class PluginDock;

class ObsSceneSwitcher : public QObject {
	Q_OBJECT

public:
	static ObsSceneSwitcher *instance();
	static void destroy();

	// プラグイン起動・終了時
	void start();
	void stop();

	// Twitch OAuth
	void startOAuthLogin();
	void handleOAuthCallback(const std::string &code);
	bool isAuthenticated() const { return authenticated_; }
	void logout();

	// EventSub
	void connectEventSub();
	void disconnectEventSub();

	// UI
	void setPluginDock(PluginDock *dock) { pluginDock_ = dock; }
	PluginDock *getPluginDock() const { return pluginDock_; }

	// EventSub 通知コールバック
	void onRedemptionReceived(const std::string &rewardId, const std::string &userName,
				  const std::string &userInput);

        // リワード一覧取得
	const std::vector<RewardInfo> &getRewardList() const { return rewardList_; }

	// OBS シーン切り替え
	void switchScene(const std::string &sceneName);

	void setRewardSceneMap(const std::unordered_map<std::string, std::string> &map);

	// 設定
	void loadConfig();
	void saveConfig();
	void reloadAuthConfig();

signals:
	void authenticationSucceeded();
	void authenticationFailed();

private:
	ObsSceneSwitcher();
	~ObsSceneSwitcher();

	static ObsSceneSwitcher *s_instance_;

	PluginDock *pluginDock_ = nullptr;

	// Twitch Auth 状態
	bool authenticated_ = false;
	bool eventsubConnected_ = false;

	std::string clientId_;
	std::string clientSecret_;
	std::string accessToken_;
	std::string refreshToken_;
	long expiresAt_ = 0;
	std::string broadcasterUserId_;

        std::vector<RewardInfo> rewardList_;

	// Reward → Scene のマッピング
	std::unordered_map<std::string, std::string> rewardSceneMap_;

	std::unique_ptr<SceneSwitcher> sceneSwitcher_;
};
