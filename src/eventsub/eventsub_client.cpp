// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "eventsub_client.h"
#include <obs-module.h>

EventSubClient::EventSubClient()
{
	blog(LOG_INFO, "[EventSub] ctor");
}

EventSubClient::~EventSubClient()
{
	blog(LOG_INFO, "[EventSub] dtor");
}

void EventSubClient::connect(const std::string &accessToken, RedemptionCallback cb)
{
	blog(LOG_INFO, "[EventSub] connect()");
	// TODO
}

void EventSubClient::disconnect()
{
	blog(LOG_INFO, "[EventSub] disconnect()");
	// TODO
}
