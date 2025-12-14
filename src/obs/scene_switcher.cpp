// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "scene_switcher.hpp"
#include <obs-frontend-api.h>
#include <obs-module.h>

SceneSwitcher::SceneSwitcher()
{
	blog(LOG_INFO, "[SceneSwitcher] ctor");
}

SceneSwitcher::~SceneSwitcher()
{
	blog(LOG_INFO, "[SceneSwitcher] dtor");
}

QStringList SceneSwitcher::getSceneList()
{
	QStringList list;

	obs_frontend_source_list scenes = {};
	obs_frontend_get_scenes(&scenes);

	for (size_t i = 0; i < scenes.sources.num; i++) {
		obs_source_t *src = scenes.sources.array[i];
		const char *name = obs_source_get_name(src);
		list << QString::fromUtf8(name);
	}

	obs_frontend_source_list_free(&scenes);

	return list;
}

void SceneSwitcher::switchScene(const std::string &sceneName)
{
	blog(LOG_INFO, "[SceneSwitcher] switchScene(%s)", sceneName.c_str());

	obs_frontend_source_list scenes = {};
	obs_frontend_get_scenes(&scenes);

	for (size_t i = 0; i < scenes.sources.num; i++) {
		obs_source_t *src = scenes.sources.array[i];
		const char *name = obs_source_get_name(src);

		if (sceneName == name) {
			obs_frontend_set_current_scene(src);
			blog(LOG_INFO, "[SceneSwitcher] Scene switched to %s", name);
			break;
		}
	}

	obs_frontend_source_list_free(&scenes);
}
