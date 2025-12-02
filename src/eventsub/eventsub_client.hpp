// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <atomic>
#include <string>
#include <thread>

class EventSubClient {
public:
	static EventSubClient &instance();
	~EventSubClient();

	// EventSub開始
	void start(const std::string &accessToken, const std::string &broadcasterUserId, const std::string &clientId);

	// スレッド停止
	void stop();

	bool isRunning() const { return running_; }

private:
	EventSubClient() = default;

	EventSubClient(const EventSubClient &) = delete;
	EventSubClient &operator=(const EventSubClient &) = delete;
	EventSubClient(EventSubClient &&) = delete;
	EventSubClient &operator=(EventSubClient &&) = delete;

	void runLoop();

	std::atomic<bool> running_{false};
	std::thread worker_;

	std::string accessToken_;
	std::string broadcasterUserId_;
	std::string clientId_;
};
