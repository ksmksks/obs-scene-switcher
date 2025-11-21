// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <string>
#include <unordered_map>

class ConfigManager {
public:
	ConfigManager();
	~ConfigManager();

	void load();
	void save();

	// 公開フィールド（簡易版）
	std::string accessToken_;
	std::string refreshToken_;
	long expiresAt_ = 0;
	std::unordered_map<std::string, std::string> rewardMapping_;
};
