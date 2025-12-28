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
#include "ui/rule_row.hpp"

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

	// プラグイン有効/無効制御
	void setEnabled(bool enabled);
	bool isEnabled() const { return pluginEnabled_; }

	// UI
	void setPluginDock(PluginDock *dock) { pluginDock_ = dock; }
	PluginDock *getPluginDock() const { return pluginDock_; }

        // リワード一覧取得
	const std::vector<RewardInfo> &getRewardList() const { return rewardList_; }
	
	// チャンネルポイント一覧を取得（WebSocket接続不要）
	void fetchRewardList();

	// OBS シーン切り替え
	void switchScene(const std::string &sceneName);

	void setRewardRules(const std::vector<RewardRule> &rules);

	// 設定
	void loadConfig();
	void saveConfig();
	void reloadAuthConfig();

signals:
	void authenticationSucceeded();
	void authenticationFailed();
	
	// 有効状態変更通知
	void enabledStateChanged(bool enabled);
	void loggedOut();  // ログアウト専用シグナル

public slots:
	// EventSub 通知コールバック
	void onRedemptionReceived(const std::string &rewardId, const std::string &userName,
				  const std::string &userInput);
	
	// SceneSwitcher 状態変更
	void onSceneSwitcherStateChanged(SceneSwitcher::State state, int remainingSeconds = -1,
	                                  const QString &targetScene = QString(),
	                                  const QString &originalScene = QString());

	private:
	ObsSceneSwitcher();
	~ObsSceneSwitcher();

	static ObsSceneSwitcher *s_instance_;

	PluginDock *pluginDock_ = nullptr;

	// Twitch Auth 状態
	bool authenticated_ = false;
	bool eventsubConnected_ = false;
	
	// プラグイン有効状態（起動時は常にfalse）
	bool pluginEnabled_ = false;

	std::string clientId_;
	std::string clientSecret_;
	std::string accessToken_;
	std::string refreshToken_;
	long expiresAt_ = 0;
	std::string broadcasterUserId_;

        std::vector<RewardInfo> rewardList_;

	// Reward → Scene のマッピング（順序を保持）
	std::vector<RewardRule> rewardRules_;

	std::unique_ptr<SceneSwitcher> sceneSwitcher_;
};
