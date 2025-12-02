// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "eventsub_client.hpp"
#include <obs-module.h>
#include <nlohmann/json.hpp>

EventSubClient &EventSubClient::instance()
{
	static EventSubClient s_instance;
	return s_instance;
}

EventSubClient::EventSubClient()
{
	ix::initNetSystem();
}

EventSubClient::~EventSubClient()
{
	stop();
}

void EventSubClient::start(const std::string &accessToken, const std::string &broadcasterUserId,
			   const std::string &clientId)
{
	if (running_) {
		blog(LOG_INFO, "[EventSub] Already running");
		return;
	}

	accessToken_ = accessToken;
	broadcasterUserId_ = broadcasterUserId;
	clientId_ = clientId;

	running_ = true;

	blog(LOG_INFO, "[EventSub] Starting WebSocket...");
	connectSocket();
}

void EventSubClient::stop()
{
	if (!running_)
		return;

	blog(LOG_INFO, "[EventSub] Stopping WebSocket...");
	running_ = false;

	try {
		websocket_.stop();
	} catch (...) {
		blog(LOG_ERROR, "[EventSub] Exception while stopping WebSocket");
	}
}

void EventSubClient::connectSocket()
{
	websocket_.setUrl("wss://eventsub.wss.twitch.tv/ws");

	setupHandlers();

	websocket_.start();
}

void EventSubClient::setupHandlers()
{
	websocket_.setOnMessageCallback([this](const ix::WebSocketMessagePtr &msg) {
		if (!running_)
			return;

		if (msg->type == ix::WebSocketMessageType::Message) {

			try {
				auto json = nlohmann::json::parse(msg->str);

				if (json["metadata"].contains("message_type")) {
					auto type = json["metadata"]["message_type"].get<std::string>();

					if (type == "session_welcome") {
						blog(LOG_INFO, "[EventSub] Received session_welcome");

						// ★ session_id を取り出す
						auto sessionId = json["payload"]["session"]["id"].get<std::string>();

						blog(LOG_INFO, "[EventSub] session_id = %s", sessionId.c_str());
						connected_ = true;

						// v0.4.3 で subscription POST をここで行う

					} else if (type == "notification") {
						blog(LOG_INFO, "[EventSub] Notification received");
						// v0.4.4 で redemption イベントを処理
					} else if (type == "session_keepalive") {
						blog(LOG_DEBUG, "[EventSub] KeepAlive received");
					} else if (type == "session_reconnect") {
						blog(LOG_WARNING, "[EventSub] Session reconnect requested");

						auto newUrl =
							json["payload"]["session"]["reconnect_url"].get<std::string>();

						websocket_.stop();
						websocket_.setUrl(newUrl);
						websocket_.start();
					}
				}

			} catch (...) {
				blog(LOG_ERROR, "[EventSub] JSON parse error");
			}

		} else if (msg->type == ix::WebSocketMessageType::Open) {
			blog(LOG_INFO, "[EventSub] WebSocket opened");

		} else if (msg->type == ix::WebSocketMessageType::Close) {
			blog(LOG_WARNING, "[EventSub] WebSocket closed");

			if (running_) {
				blog(LOG_WARNING, "[EventSub] Attempting reconnect...");
				websocket_.start();
			}

		} else if (msg->type == ix::WebSocketMessageType::Error) {
			blog(LOG_ERROR, "[EventSub] Error: %s", msg->errorInfo.reason.c_str());
		}
	});
}
