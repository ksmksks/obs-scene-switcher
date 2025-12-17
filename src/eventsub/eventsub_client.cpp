// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "eventsub_client.hpp"
#include "obs_scene_switcher.hpp"
#include <obs-module.h>
#include <nlohmann/json.hpp>
#include <wininet.h>

#pragma comment(lib, "Wininet.lib")

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

nlohmann::json EventSubClient::httpGet(const std::string &url)
{
	HINTERNET hInet = InternetOpenA("EventSubClient", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	HINTERNET hConn = InternetConnectA(hInet, "api.twitch.tv", 443, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);

	HINTERNET hReq = HttpOpenRequestA(hConn, "GET", url.c_str(), NULL, NULL, NULL, INTERNET_FLAG_SECURE, 0);

	std::string headers = "Client-ID: " + clientId_ + "\r\nAuthorization: Bearer " + accessToken_ + "\r\n";

	HttpSendRequestA(hReq, headers.c_str(), (DWORD)headers.size(), NULL, 0);

	char buf[8192];
	DWORD read = 0;
	std::string response;

	while (InternetReadFile(hReq, buf, sizeof(buf), &read) && read > 0)
		response.append(buf, read);

	InternetCloseHandle(hReq);
	InternetCloseHandle(hConn);
	InternetCloseHandle(hInet);

	auto json = nlohmann::json::parse(response, nullptr, false);
	if (json.is_discarded())
		return {};

	return json;
}

bool EventSubClient::httpPost(const std::string &body)
{
	HINTERNET hInet = InternetOpenA("EventSubClient", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	HINTERNET hConn = InternetConnectA(hInet, "api.twitch.tv", 443, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);

	HINTERNET hReq = HttpOpenRequestA(hConn, "POST", "/helix/eventsub/subscriptions", NULL, NULL, NULL,
					  INTERNET_FLAG_SECURE, 0);

	std::string headers = "Client-ID: " + clientId_ + "\r\nAuthorization: Bearer " + accessToken_ +
			      "\r\nContent-Type: application/json\r\n";

	HttpSendRequestA(hReq, headers.c_str(), (DWORD)headers.size(), (LPVOID)body.c_str(), (DWORD)body.size());

	char buf[8192];
	DWORD read = 0;
	std::string response;

	while (InternetReadFile(hReq, buf, sizeof(buf), &read) && read > 0)
		response.append(buf, read);

	InternetCloseHandle(hReq);
	InternetCloseHandle(hConn);
	InternetCloseHandle(hInet);

	auto json = nlohmann::json::parse(response, nullptr, false);
	if (json.is_discarded())
		return false;

	blog(LOG_INFO, "[EventSub] POST /subscriptions response: %s", response.c_str());
	return true;
}

void EventSubClient::ensureSubscription(const std::string &sessionId)
{
	blog(LOG_INFO, "[EventSub] Checking subscriptions...");

	nlohmann::json body = {{"type", "channel.channel_points_custom_reward_redemption.add"},
			       {"version", "1"},
			       {"condition",
				{
					{"broadcaster_user_id", broadcasterUserId_},
				}},
			       {"transport",
				{
					{"method", "websocket"},
					{"session_id", sessionId},
				}}};

	if (!httpPost(body.dump())) {
		blog(LOG_ERROR, "[EventSub] Failed to create subscription");
	}
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
						auto sessionId = json["payload"]["session"]["id"].get<std::string>();
						blog(LOG_INFO, "[EventSub] Received session_welcome");
						blog(LOG_INFO, "[EventSub] session_id = %s", sessionId.c_str());

						ensureSubscription(sessionId);

					} else if (type == "notification") {
						blog(LOG_INFO, "[EventSub] Notification received");

						try {
							const auto &event = json["payload"]["event"];

							std::string rewardId = event["reward"]["id"].get<std::string>();
							std::string userName = event["user_name"].get<std::string>();
							std::string userInput = event.value("user_input", "");

							blog(LOG_INFO,
							     "[EventSub] Redemption: reward_id=%s user=%s input=%s",
							     rewardId.c_str(), userName.c_str(), userInput.c_str());

							// SceneSwitcher に通知
							emit redemptionReceived(rewardId, userName, userInput);

						} catch (...) {
							blog(LOG_ERROR, "[EventSub] Failed to parse redemption event");
						}
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
