// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <QObject>
#include <atomic>
#include <string>
#include <mutex>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>

#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXNetSystem.h>

class EventSubClient : public QObject {
	Q_OBJECT
public:
	static EventSubClient &instance();

	// EventSub開始
	void start(const std::string &accessToken, const std::string &broadcasterUserId, const std::string &clientId);

	// EventSub停止
	void stop();

	bool isRunning() const { return running_; }

signals:
	void redemptionReceived(const std::string &rewardId, const std::string &userName, const std::string &userInput);

private:
	EventSubClient();
	~EventSubClient() override = default;

	EventSubClient(const EventSubClient &) = delete;
	EventSubClient &operator=(const EventSubClient &) = delete;

	nlohmann::json httpGet(const std::string &url);
	bool httpPost(const std::string &body);

	// 接続関連
	void setupHandlers();
	void connectSocket(const std::string &url = {});
	std::string defaultWebSocketUrl() const;

	// 受信処理
	void handleMessage(const std::string &msg);
	void handleSessionWelcome(const nlohmann::json &j);
	void handleSessionReconnect(const nlohmann::json &j);
	void handleNotification(const nlohmann::json &j);

	// Subscription
	void ensureSubscription(const std::string &sessionId);

	// WebSocket
	ix::WebSocket websocket_;

	// 認証情報
	std::string accessToken_;
	std::string broadcasterUserId_;
	std::string clientId_;

	// 状態
	std::atomic<bool> running_{false};
	std::atomic<bool> connected_{false};

	// 現在の WS URL
	std::mutex urlMutex_;
	std::string currentWsUrl_;

	// session_reconnect 制御
	std::mutex reconnectMutex_;
	std::string pendingReconnectUrl_;
	std::atomic<bool> reconnectRequested_{false};
};
