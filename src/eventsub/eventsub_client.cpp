// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "eventsub_client.hpp"
#include "obs_scene_switcher.hpp"
#include <obs-module.h>
#include <wininet.h>

#pragma comment(lib, "Wininet.lib")

using json = nlohmann::json;

static constexpr const char *kDefaultWsUrl = "wss://eventsub.wss.twitch.tv/ws";

EventSubClient &EventSubClient::instance()
{
	static EventSubClient s_instance;
	return s_instance;
}

EventSubClient::EventSubClient()
{
	ix::initNetSystem();
}

std::string EventSubClient::defaultWebSocketUrl() const
{
	return kDefaultWsUrl;
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
	connected_ = false;

	blog(LOG_INFO, "[EventSub] Starting WebSocket...");
	connectSocket(); // default URL
	setupHandlers();
	websocket_.start();
}

void EventSubClient::stop()
{
	if (!running_)
		return;

	blog(LOG_INFO, "[EventSub] Stopping WebSocket...");
	running_ = false;
	connected_ = false;

        {
		std::lock_guard<std::mutex> lk(reconnectMutex_);
		pendingReconnectUrl_.clear();
		reconnectRequested_ = false;
	}

	try {
		websocket_.stop();
	} catch (...) {
		blog(LOG_ERROR, "[EventSub] Exception while stopping WebSocket");
	}
}

void EventSubClient::connectSocket(const std::string &url)
{
	const std::string wsUrl = url.empty() ? defaultWebSocketUrl() : url;

	{
		std::lock_guard<std::mutex> lk(urlMutex_);
		currentWsUrl_ = wsUrl;
	}

	websocket_.setUrl(wsUrl);

	// ログ量/keepalive調整
	websocket_.disablePerMessageDeflate();
	websocket_.setPingInterval(25); // twitch keepalive より少し短めに ping
}

json EventSubClient::httpGet(const std::string &url)
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

	auto json = json::parse(response, nullptr, false);
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

	auto json = json::parse(response, nullptr, false);
	if (json.is_discarded())
		return false;

	blog(LOG_INFO, "[EventSub] POST /subscriptions response: %s", response.c_str());
	return true;
}

void EventSubClient::ensureSubscription(const std::string &sessionId)
{
	blog(LOG_INFO, "[EventSub] Checking subscriptions...");

	json body = {{"type", "channel.channel_points_custom_reward_redemption.add"},
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
		if (!msg)
			return;

		switch (msg->type) {
		case ix::WebSocketMessageType::Open:
			connected_ = true;
			blog(LOG_INFO, "[EventSub] WebSocket opened");
			break;

		case ix::WebSocketMessageType::Close:
			connected_ = false;
			blog(LOG_INFO, "[EventSub] WebSocket closed");

			if (!running_)
				return;

	                {
				std::string nextUrl;

				// Twitch指定 reconnect_url があれば優先
				{
					std::lock_guard<std::mutex> lk(reconnectMutex_);
					if (reconnectRequested_) {
						nextUrl = pendingReconnectUrl_;
						pendingReconnectUrl_.clear();
						reconnectRequested_ = false;
					}
				}

				if (nextUrl.empty()) {
					blog(LOG_INFO, "[EventSub] Attempting reconnect...");
					std::lock_guard<std::mutex> lk(urlMutex_);
					nextUrl = currentWsUrl_;
				} else {
					blog(LOG_INFO, "[EventSub] Reconnecting using Twitch-provided reconnect_url");
				}

				connectSocket(nextUrl);

				// コールバック内で start() しない（再入・二重start回避）
				std::thread([this]() {
					std::this_thread::sleep_for(std::chrono::milliseconds(200));
					if (running_) {
						websocket_.start();
					}
				}).detach();
			}
			break;

		case ix::WebSocketMessageType::Message:
			// Twitch EventSub はテキスト JSON
			if (!msg->str.empty()) {
				handleMessage(msg->str);
			}
			break;

		case ix::WebSocketMessageType::Error:
			blog(LOG_ERROR, "[EventSub] WebSocket error: %s", msg->errorInfo.reason.c_str());
			break;

		default:
			break;
		}
	});
}

void EventSubClient::handleMessage(const std::string &msg)
{
	try {
		auto json = json::parse(msg);

		const std::string type = json["metadata"].value("message_type", "");
		if (type.empty())
			return;

		if (type == "session_welcome") {
			blog(LOG_INFO, "[EventSub] Received session_welcome");
			handleSessionWelcome(json);
		} else if (type == "session_reconnect") {
			// ここで start/stop しない（フラグ立てて close 側で繋ぎ直す）
			blog(LOG_WARNING, "[EventSub] Session reconnect requested");
			handleSessionReconnect(json);
		} else if (type == "notification") {
			handleNotification(json);
		} else if (type == "session_keepalive") {
			// 必要ならログ
		} else if (type == "revocation") {
			blog(LOG_WARNING, "[EventSub] Subscription revoked");
		} else {
			// 他の message_type は無視
		}

	} catch (const json::parse_error &e) {
		// "JSON parse error"の理由が分かるようにログ改善
		blog(LOG_ERROR, "[EventSub] JSON parse error: %s", e.what());
	} catch (const std::exception &e) {
		blog(LOG_ERROR, "[EventSub] Exception while handling message: %s", e.what());
	}
}

void EventSubClient::handleSessionWelcome(const json &json)
{
	const std::string sessionId = json["payload"]["session"].value("id", "");
	blog(LOG_INFO, "[EventSub] session_id = %s", sessionId.c_str());

	ensureSubscription(sessionId);
}

void EventSubClient::handleSessionReconnect(const json &json)
{
	const std::string reconnectUrl = json["payload"]["session"].value("reconnect_url", "");
	if (reconnectUrl.empty()) {
		blog(LOG_ERROR, "[EventSub] session_reconnect missing reconnect_url");
		return;
	}

	{
		std::lock_guard<std::mutex> lk(reconnectMutex_);
		pendingReconnectUrl_ = reconnectUrl;
		reconnectRequested_ = true;
	}

	// stop() は呼ばない。close で切断だけ要求し、Closeイベント側で繋ぎ直す。
	blog(LOG_INFO, "[EventSub] Closing current WebSocket to switch to reconnect_url...");
	websocket_.close();
}

void EventSubClient::handleNotification(const json &json)
{
	// redemption 通知の取り出し
	try {
		const auto &event = json["payload"]["event"];

		const std::string rewardId = event["reward"].value("id", "");
		const std::string userName = event.value("user_name", "");
		const std::string userInput = event.value("user_input", "");

		blog(LOG_INFO, "[EventSub] Notification received");
		blog(LOG_INFO, "[EventSub] Redemption: reward_id=%s user=%s input=%s", rewardId.c_str(),
		     userName.c_str(), userInput.c_str());

		emit redemptionReceived(rewardId, userName, userInput);

	} catch (const std::exception &e) {
		blog(LOG_ERROR, "[EventSub] Failed to parse notification payload: %s", e.what());
	}
}
