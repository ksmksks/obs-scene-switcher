// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <atomic>
#include <string>
#include <thread>

#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXNetSystem.h>

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
	EventSubClient();

	EventSubClient(const EventSubClient &) = delete;
	EventSubClient &operator=(const EventSubClient &) = delete;

	// WebSocket
	ix::WebSocket websocket_;

	// 認証情報
	std::string accessToken_;
	std::string broadcasterUserId_;
	std::string clientId_;

	// 状態
	std::atomic<bool> running_{false};
	std::atomic<bool> connected_{false};

	// 内部処理
	void setupHandlers();
	void connectSocket();
};
