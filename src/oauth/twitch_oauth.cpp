// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "twitch_oauth.hpp"
#include "../obs/config_manager.hpp"

#include <obs-module.h>
#include <nlohmann/json.hpp>
#include <windows.h>
#include <wininet.h>

#pragma comment(lib, "Wininet.lib")

static const char *REDIRECT_URI = "http://localhost:38915/callback";
static const char *SCOPE = "channel:read:redemptions";

TwitchOAuth::TwitchOAuth()
{
	blog(LOG_DEBUG, "[obs-scene-switcher][OAuth] TwitchOAuth ctor");
}

TwitchOAuth::~TwitchOAuth()
{
	blog(LOG_DEBUG, "[obs-scene-switcher][OAuth] TwitchOAuth dtor");
}

void TwitchOAuth::startOAuthLogin()
{
	auto &cfg = ConfigManager::instance();
	clientId_ = cfg.getClientId();
	clientSecret_ = cfg.getClientSecret();

	if (clientId_.empty() || clientSecret_.empty()) {
		blog(LOG_ERROR, "[obs-scene-switcher] Client ID or Client Secret is not configured");
		return;
	}

	std::string url = buildAuthUrl();
	blog(LOG_INFO, "[obs-scene-switcher] Opening browser for Twitch authentication");

#ifdef _WIN32
	ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
#endif
}

void TwitchOAuth::handleAuthCode(const std::string &code)
{
	// TODO
}

bool TwitchOAuth::refreshAccessToken()
{
	auto &cfg = ConfigManager::instance();
	refreshToken_ = cfg.getRefreshToken();
	clientId_ = cfg.getClientId();
	clientSecret_ = cfg.getClientSecret();

	if (refreshToken_.empty()) {
		blog(LOG_DEBUG, "[obs-scene-switcher][OAuth] No refresh token available, skipping refresh");
		return false;
	}

	blog(LOG_DEBUG, "[obs-scene-switcher][OAuth] Refreshing access token...");

	auto attemptRefresh = [&]() -> bool {
                HINTERNET hInet = InternetOpenA("TwitchOAuth", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	        if (!hInet) {
		        blog(LOG_ERROR, "[obs-scene-switcher] InternetOpenA failed");
		        return false;
	        }

	        HINTERNET hConnect = InternetConnectA(hInet, "id.twitch.tv", 443, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
	        if (!hConnect) {
		        blog(LOG_ERROR, "[obs-scene-switcher] InternetConnectA failed");
		        InternetCloseHandle(hInet);
		        return false;
	        }

	        HINTERNET hRequest =
		        HttpOpenRequestA(hConnect, "POST", "/oauth2/token", NULL, NULL, NULL, INTERNET_FLAG_SECURE, 0);
	        if (!hRequest) {
		        blog(LOG_ERROR, "[obs-scene-switcher] HttpOpenRequestA failed");
		        InternetCloseHandle(hConnect);
		        InternetCloseHandle(hInet);
		        return false;
	        }

	        std::string body = "client_id=" + clientId_ + "&client_secret=" + clientSecret_ +
		        	   "&refresh_token=" + refreshToken_ + "&grant_type=refresh_token";
	        std::string headers = "Content-Type: application/x-www-form-urlencoded";

	        BOOL requestOk = HttpSendRequestA(hRequest, headers.c_str(), (DWORD)headers.size(), (LPVOID)body.c_str(),
		        			  (DWORD)body.size());

	        if (!requestOk) {
		        blog(LOG_ERROR, "[obs-scene-switcher] HttpSendRequestA failed");
		        InternetCloseHandle(hRequest);
		        InternetCloseHandle(hConnect);
		        InternetCloseHandle(hInet);
		        return false;
	        }

	        char buffer[8192];
	        DWORD read = 0;
	        std::string response;
	        while (InternetReadFile(hRequest, buffer, sizeof(buffer), &read) && read > 0)
		        response.append(buffer, read);

	        InternetCloseHandle(hRequest);
	        InternetCloseHandle(hConnect);
	        InternetCloseHandle(hInet);

	        auto json = nlohmann::json::parse(response, nullptr, false);
	        if (json.is_discarded()) {
		        blog(LOG_ERROR, "[obs-scene-switcher] Failed to parse refresh response");
		        return false;
	        }

	        accessToken_ = json.value("access_token", "");
	        refreshToken_ = json.value("refresh_token", refreshToken_);
	        expiresAt_ = static_cast<long>(time(nullptr)) + static_cast<long>(json.value("expires_in", 0));

	        if (accessToken_.empty()) {
		        blog(LOG_ERROR, "[obs-scene-switcher] Refresh failed: access_token empty");
		        return false;
		}
		return true;
	};

	// 1回目試行
	if (attemptRefresh()) {
		blog(LOG_INFO, "[obs-scene-switcher] Token refresh successful");
		cfg.setAccessToken(accessToken_);
		cfg.setRefreshToken(refreshToken_);
		cfg.setTokenExpiresAt(expiresAt_);
		return true;
	}

        // リトライ試行
	blog(LOG_WARNING, "[obs-scene-switcher] Token refresh failed, retrying...");
	if (attemptRefresh()) {
		blog(LOG_INFO, "[obs-scene-switcher] Token refresh succeeded on retry");
		cfg.setAccessToken(accessToken_);
		cfg.setRefreshToken(refreshToken_);
		cfg.setTokenExpiresAt(expiresAt_);
		return true;
	}

	// 失敗時
	blog(LOG_ERROR, "[obs-scene-switcher] Token refresh failed after retry");
	return false;
}

std::string TwitchOAuth::buildAuthUrl()
{
	std::string url = "https://id.twitch.tv/oauth2/authorize?"
			  "client_id=" +  clientId_ + 
			  "&redirect_uri=" + std::string(REDIRECT_URI) +
			  "&response_type=code"
			  "&scope=" + std::string(SCOPE);

	return url;
}

bool TwitchOAuth::fetchUserInfo()
{
	HINTERNET hInet = InternetOpenA("TwitchOAuth", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (!hInet)
		return false;

	HINTERNET hConnect =
		InternetConnectA(hInet, "api.twitch.tv", 443, nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, 0);
	if (!hConnect)
		return false;

	HINTERNET hRequest =
		HttpOpenRequestA(hConnect, "GET", "/helix/users", NULL, NULL, NULL, INTERNET_FLAG_SECURE, 0);

	std::string headers = "Client-ID: " + clientId_ +
			      "\r\n"
			      "Authorization: Bearer " +
			      accessToken_ + "\r\n";

	if (!HttpSendRequestA(hRequest, headers.c_str(), (DWORD)headers.size(), nullptr, 0)) {
		blog(LOG_ERROR, "[obs-scene-switcher] Failed to send user info request");
		return false;
	}

	char buffer[8192];
	DWORD read = 0;
	std::string response;
	while (InternetReadFile(hRequest, buffer, sizeof(buffer), &read) && read > 0) {
		response.append(buffer, read);
	}

	InternetCloseHandle(hRequest);
	InternetCloseHandle(hConnect);
	InternetCloseHandle(hInet);

	auto json = nlohmann::json::parse(response, nullptr, false);
	if (json.is_discarded() || !json.contains("data") || json["data"].empty())
		return false;

	const auto &user = json["data"][0];
	broadcasterUserId_ = user.value("id", "");
	broadcasterLogin_ = user.value("login", "");
	displayName_ = user.value("display_name", "");

	blog(LOG_INFO, "[obs-scene-switcher] Authenticated as: %s (%s)", displayName_.c_str(), broadcasterLogin_.c_str());

	return true;
}

bool TwitchOAuth::exchangeCodeForToken(const std::string &code)
{
	blog(LOG_DEBUG, "[obs-scene-switcher][OAuth] Exchanging code for token...");

	HINTERNET hInet = InternetOpenA("TwitchOAuth", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (!hInet) return false;

	HINTERNET hConnect = InternetConnectA(hInet, "id.twitch.tv", 443, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
	HINTERNET hRequest =
		HttpOpenRequestA(hConnect, "POST", "/oauth2/token", NULL, NULL, NULL, INTERNET_FLAG_SECURE, 0);

	std::string body = "client_id=" + clientId_ + "&client_secret=" + clientSecret_ + "&code=" + code +
			   "&grant_type=authorization_code"
			   "&redirect_uri=" + std::string(REDIRECT_URI);
	std::string headers = "Content-Type: application/x-www-form-urlencoded";

	HttpSendRequestA(hRequest, headers.c_str(), (DWORD)headers.size(), (LPVOID)body.c_str(), (DWORD)body.size());

	char buffer[8192];
	DWORD read = 0;
	std::string response;

	while (InternetReadFile(hRequest, buffer, sizeof(buffer), &read) && read > 0) {
		response.append(buffer, read);
	}

	InternetCloseHandle(hRequest);
	InternetCloseHandle(hConnect);
	InternetCloseHandle(hInet);

	// JSON parse
	auto json = nlohmann::json::parse(response, nullptr, false);
	if (json.is_discarded())
		return false;

	accessToken_ = json.value("access_token", "");
	refreshToken_ = json.value("refresh_token", "");
	expiresAt_ = json.value("expires_in", 0);

	if (accessToken_.empty()) {
		blog(LOG_ERROR, "[obs-scene-switcher] Access token empty");
		return false;
	}

	blog(LOG_DEBUG, "[obs-scene-switcher][OAuth] Token exchange complete, fetching user info...");

	// ユーザー情報取得
	if (!fetchUserInfo()) {
		blog(LOG_ERROR, "[obs-scene-switcher] Failed to fetch user info");
		return false;
	}

	// Config 保存
	auto &cfg = ConfigManager::instance();
	cfg.setAccessToken(accessToken_);
	cfg.setRefreshToken(refreshToken_);
	cfg.setTokenExpiresAt(expiresAt_);
	cfg.setBroadcasterUserId(broadcasterUserId_);
	cfg.setBroadcasterLogin(broadcasterLogin_);
	cfg.setBroadcasterDisplayName(displayName_);
	cfg.save();

	return true;
}

std::vector<RewardInfo> TwitchOAuth::fetchChannelRewards()
{
	std::vector<RewardInfo> list;

	auto &cfg = ConfigManager::instance();
	clientId_ = cfg.getClientId();
	accessToken_ = cfg.getAccessToken();
	std::string broadcasterUserId = cfg.getBroadcasterUserId();

	if (clientId_.empty() || accessToken_.empty() || broadcasterUserId.empty()) {
		blog(LOG_ERROR, "[obs-scene-switcher] Missing credentials for fetchChannelRewards()");
		return list;
	}

	HINTERNET hInet = InternetOpenA("TwitchOAuth", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (!hInet)
		return list;

	HINTERNET hConnect =
		InternetConnectA(hInet, "api.twitch.tv", 443, nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, 0);
	if (!hConnect) {
		InternetCloseHandle(hInet);
		return list;
	}

	std::string path = "/helix/channel_points/custom_rewards?broadcaster_id=" + broadcasterUserId;

	HINTERNET hRequest = HttpOpenRequestA(hConnect, "GET", path.c_str(), NULL, NULL, NULL, INTERNET_FLAG_SECURE, 0);

	std::string headers = "Client-ID: " + clientId_ +
			      "\r\n"
			      "Authorization: Bearer " +
			      accessToken_ + "\r\n";

	if (!HttpSendRequestA(hRequest, headers.c_str(), (DWORD)headers.size(), nullptr, 0)) {
		blog(LOG_ERROR, "[obs-scene-switcher] Reward request failed");
		InternetCloseHandle(hRequest);
		InternetCloseHandle(hConnect);
		InternetCloseHandle(hInet);
		return list;
	}

	char buffer[8192];
	DWORD read = 0;
	std::string response;
	while (InternetReadFile(hRequest, buffer, sizeof(buffer), &read) && read > 0) {
		response.append(buffer, read);
	}

	InternetCloseHandle(hRequest);
	InternetCloseHandle(hConnect);
	InternetCloseHandle(hInet);

	auto json = nlohmann::json::parse(response, nullptr, false);
	if (json.is_discarded() || !json.contains("data"))
		return list;

	for (auto &r : json["data"]) {
		RewardInfo info;
		info.id = r.value("id", "");
		info.title = r.value("title", "");
		list.push_back(info);
	}

	blog(LOG_INFO, "[obs-scene-switcher] Fetched %zu channel rewards", list.size());

	return list;
}
