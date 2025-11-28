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
	blog(LOG_INFO, "[OAuth] TwitchOAuth ctor");
}

TwitchOAuth::~TwitchOAuth()
{
	blog(LOG_INFO, "[OAuth] TwitchOAuth dtor");
}

void TwitchOAuth::startOAuthLogin()
{
	auto &cfg = ConfigManager::instance();
	clientId_ = cfg.getClientId();
	clientSecret_ = cfg.getClientSecret();

	if (clientId_.empty() || clientSecret_.empty()) {
		blog(LOG_ERROR, "[OAuth] Client ID or Client Secret is not configured");
		return;
	}

	std::string url = buildAuthUrl();
	blog(LOG_INFO, "[OAuth] Opening browser: %s", url.c_str());

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
	blog(LOG_INFO, "[OAuth] Refreshing access token...");

	auto &cfg = ConfigManager::instance();
	refreshToken_ = cfg.getRefreshToken();
	clientId_ = cfg.getClientId();
	clientSecret_ = cfg.getClientSecret();

	if (refreshToken_.empty()) {
		blog(LOG_ERROR, "[OAuth] No refresh_token available");
		return false;
	}

	HINTERNET hInet = InternetOpenA("TwitchOAuth", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (!hInet) {
		blog(LOG_ERROR, "[OAuth] InternetOpenA failed");
		return false;
	}

	HINTERNET hConnect = InternetConnectA(hInet, "id.twitch.tv", 443, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
	if (!hConnect) {
		blog(LOG_ERROR, "[OAuth] InternetConnectA failed");
		InternetCloseHandle(hInet);
		return false;
	}

	HINTERNET hRequest =
		HttpOpenRequestA(hConnect, "POST", "/oauth2/token", NULL, NULL, NULL, INTERNET_FLAG_SECURE, 0);
	if (!hRequest) {
		blog(LOG_ERROR, "[OAuth] HttpOpenRequestA failed");
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
		blog(LOG_ERROR, "[OAuth] HttpSendRequestA failed");
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
		blog(LOG_ERROR, "[OAuth] Failed to parse refresh response");
		return false;
	}

	accessToken_ = json.value("access_token", "");
	refreshToken_ = json.value("refresh_token", refreshToken_);
	expiresAt_ = static_cast<long>(time(nullptr)) + static_cast<long>(json.value("expires_in", 0));

	if (accessToken_.empty()) {
		blog(LOG_ERROR, "[OAuth] refresh failed: access_token empty");
		return false;
	}

	cfg.setAccessToken(accessToken_);
	cfg.setRefreshToken(refreshToken_);
	cfg.setTokenExpiresAt(expiresAt_);

	blog(LOG_INFO, "[OAuth] Token refresh successful");
	return true;
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

bool TwitchOAuth::exchangeCodeForToken(const std::string &code)
{
	blog(LOG_INFO, "[OAuth] Exchanging code for token...");

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
		blog(LOG_ERROR, "[OAuth] Access token empty");
		return false;
	}

	blog(LOG_INFO, "[OAuth] Token received (length=%d)", (int)accessToken_.size());
	return true;
}
