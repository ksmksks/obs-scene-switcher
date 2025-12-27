// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <string>
#include <unordered_map>

#include "ui/rule_row.hpp"
#include "core/reward_rule.hpp"

class ConfigManager {
public:
	static ConfigManager &instance();

	void save();
	void load();

	const std::string &getClientId() const;
	const std::string &getClientSecret() const;
	void setClientId(const std::string &id);
	void setClientSecret(const std::string &secret);

	const std::string &getAccessToken() const;
	const std::string &getRefreshToken() const;
	long getTokenExpiresAt() const;
	void setAccessToken(const std::string &token);
	void setRefreshToken(const std::string &token);
	void setTokenExpiresAt(long expiresAt);

	bool isAuthValid() const;
	bool isTokenExpired() const;

	void setBroadcasterUserId(const std::string &id);
	void setBroadcasterLogin(const std::string &login);
	void setBroadcasterDisplayName(const std::string &name);
	const std::string &getBroadcasterUserId() const;
	const std::string &getBroadcasterLogin() const;
	const std::string &getBroadcasterDisplayName() const;

	const std::vector<RewardRule> &getRewardRules() const;
	void setRewardRules(const std::vector<RewardRule> &rules);
	void clearRewardRules();  // プラグイン有効状態（起動時は常にfalseを返す）
	bool getPluginEnabled() const { return false; }
	void setPluginEnabled(bool enabled);

private:
	ConfigManager();
	~ConfigManager() = default;

	ConfigManager(const ConfigManager &) = delete;
	ConfigManager &operator=(const ConfigManager &) = delete;

	std::string configPath_;
	std::string clientId_;
	std::string clientSecret_;

	std::string accessToken_;
	std::string refreshToken_;
	long expiresAt_ = 0;

	std::string broadcasterUserId_;
	std::string broadcasterLogin_;
	std::string streamerDisplayName_;

	std::vector<RewardRule> rewardRules_;  // プラグイン有効状態（内部保持のみ、getPluginEnabled()は常にfalseを返す）
	bool pluginEnabled_ = false;
};
