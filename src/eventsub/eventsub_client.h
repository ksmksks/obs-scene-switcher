// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <string>
#include <functional>

class EventSubClient {
public:
	using RedemptionCallback = std::function<void(const std::string &rewardId, const std::string &userName,
						      const std::string &userInput)>;

	EventSubClient();
	~EventSubClient();

	void connect(const std::string &accessToken, RedemptionCallback cb);
	void disconnect();

private:
	// TODO: IXWebSocket インスタンス
	bool connected_ = false;
};
