// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "eventsub_client.hpp"
#include <obs-module.h>

#include <chrono>

EventSubClient &EventSubClient::instance()
{
	static EventSubClient s_instance;
	return s_instance;
}

EventSubClient::~EventSubClient()
{
	stop();
}

void EventSubClient::start(const std::string &accessToken, const std::string &broadcasterUserId,
			   const std::string &clientId)
{
	// すでに動作中なら何もしない
	if (running_) {
		blog(LOG_INFO, "[EventSub] EventSubClient already running");
		return;
	}

	accessToken_ = accessToken;
	broadcasterUserId_ = broadcasterUserId;
	clientId_ = clientId;

	running_ = true;

	blog(LOG_INFO,
	       "[EventSub] Starting EventSub client thread "
	       "(user_id=%s, client_id=%s)",
	       broadcasterUserId_.c_str(), clientId_.c_str());

	worker_ = std::thread(&EventSubClient::runLoop, this);
}

void EventSubClient::stop()
{
	if (!running_)
		return;

	blog(LOG_INFO, "[EventSub] Stopping EventSub client thread");

	running_ = false;

	if (worker_.joinable())
		worker_.join();
}

void EventSubClient::runLoop()
{
	blog(LOG_INFO, "[EventSub] EventSub client loop started");

	// v0.4.0 時点では「基盤」としてスレッドだけ動かす。
	// 実際の WebSocket 接続処理は v0.4.1 以降で実装する想定。
	while (running_) {
		// TODO: ここで EventSub WebSocket への接続／再接続処理を実装する
		// 例）接続状態の確認、Ping/Pong、メッセージ受信など

		std::this_thread::sleep_for(std::chrono::seconds(10));
	}

	blog(LOG_INFO, "[EventSub] EventSub client loop stopped");
}
