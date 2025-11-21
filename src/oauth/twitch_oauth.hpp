// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <string>

class TwitchOAuth {
public:
	TwitchOAuth();
	~TwitchOAuth();

	void startOAuthLogin();                       // ブラウザで認証開始
	void handleAuthCode(const std::string &code); // /callback で受信

	bool refreshAccessToken();

	std::string buildAuthUrl();
	bool exchangeCodeForToken(const std::string &code);

	std::string getAccessToken() const { return accessToken_; }
	std::string getRefreshToken() const { return refreshToken_; }
	long getExpiresAt() const { return expiresAt_; }

private:
	std::string clientId_;
	std::string clientSecret_;

	std::string accessToken_;
	std::string refreshToken_;
	long expiresAt_ = 0;
};
