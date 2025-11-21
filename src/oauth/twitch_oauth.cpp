// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "twitch_oauth.hpp"
#include "twitch_oauth_secrets.h"
#include <obs-module.h>
#include <windows.h>
#include <wininet.h>

#pragma comment(lib, "Wininet.lib")

static const char *CLIENT_ID = TWITCH_CLIENT_ID;
static const char *CLIENT_SECRET = TWITCH_CLIENT_SECRET;
static const char *REDIRECT_URI = TWITCH_REDIRECT_URI;
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
	// TODO
}

void TwitchOAuth::handleAuthCode(const std::string &code)
{
	// TODO
}

bool TwitchOAuth::refreshAccessToken()
{
	// TODO
	return false;
}
