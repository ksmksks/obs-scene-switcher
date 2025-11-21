// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "config_manager.h"
#include <obs-module.h>

ConfigManager::ConfigManager()
{
	blog(LOG_INFO, "[ConfigManager] ctor");
}

ConfigManager::~ConfigManager()
{
	blog(LOG_INFO, "[ConfigManager] dtor");
}

void ConfigManager::load()
{
	blog(LOG_INFO, "[ConfigManager] load()");
	// TODO: obs_data で読み込み
}

void ConfigManager::save()
{
	blog(LOG_INFO, "[ConfigManager] save()");
	// TODO: obs_data で保存
}
