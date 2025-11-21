// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <string>
#include <unordered_map>

class ConfigManager {
public:
	static ConfigManager &instance();

	void save();
	void load();

	const std::string &getClientId() const;
	const std::string &getClientSecret() const;
	void setClientId(const std::string &id);
	void setClientSecret(const std::string &secret);


private:
	ConfigManager();
	~ConfigManager() = default;

	ConfigManager(const ConfigManager &) = delete;
	ConfigManager &operator=(const ConfigManager &) = delete;

	std::string configPath_;
	std::string clientId_;
	std::string clientSecret_;

	// 公開フィールド（簡易版）
	std::string accessToken_;
	std::string refreshToken_;
	long expiresAt_ = 0;
	std::unordered_map<std::string, std::string> rewardMapping_;
};
