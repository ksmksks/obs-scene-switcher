// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

extern "C" {
#include <obs-module.h>
}

class PluginProperties {
public:
	static obs_properties_t *getProperties(void *data);
	static bool onLoginClicked(obs_properties_t *props, obs_property_t *property, void *data);
	static bool onLogoutClicked(obs_properties_t *props, obs_property_t *property, void *data);
};
