// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "plugin_properties.h"

extern "C" {
#include <obs-module.h>
}

// UI を返すだけの最小実装
obs_properties_t *PluginProperties::getProperties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_properties_add_text(props, "token", "Access Token", OBS_TEXT_DEFAULT);

	obs_properties_add_button(props, "login_button", "Login to Twitch",
				  [](obs_properties_t *props, obs_property_t *prop, void *data) {
					  blog(LOG_INFO, "[SceneSwitcher] Login button clicked");
					  return true;
				  });

	obs_properties_add_button(props, "logout_button", "Logout",
				  [](obs_properties_t *props, obs_property_t *prop, void *data) {
					  blog(LOG_INFO, "[SceneSwitcher] Logout button clicked");
					  return true;
				  });

	obs_properties_add_text(props, s_clientIdKey_, "Twitch Client ID", OBS_TEXT_DEFAULT);		
        obs_properties_add_text(props, s_clientSecretKey_, "Twitch Client Secret", OBS_TEXT_PASSWORD);

	return props;
}
