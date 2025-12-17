// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "scene_switcher.hpp"
#include <obs-frontend-api.h>
#include <obs-module.h>

SceneSwitcher::SceneSwitcher(QObject *parent) : QObject(parent)
{
	blog(LOG_INFO, "[SceneSwitcher] ctor");
	revertTimer_.setSingleShot(true);
	connect(&revertTimer_, &QTimer::timeout, this, &SceneSwitcher::revertScene);
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

void SceneSwitcher::switchWithRevert(const std::string &targetScene, int revertSeconds)
{
	originalScene_ = getCurrentSceneName();

	blog(LOG_INFO, "[SceneSwitcher] Switching to '%s'; will revert to '%s' in %d seconds", targetScene.c_str(),
	     originalScene_.c_str(), revertSeconds);

	switchScene(targetScene);

	if (revertSeconds > 0) {
		revertTimer_.start(revertSeconds * 1000);
	}
}

std::string SceneSwitcher::getCurrentSceneName() const
{
	obs_source_t *current = obs_frontend_get_current_scene();
	if (!current)
		return {};

	const char *name = obs_source_get_name(current);
	std::string sceneName = name ? name : "";

	obs_source_release(current);
	return sceneName;
}

void SceneSwitcher::revertScene()
{
	blog(LOG_INFO, "[SceneSwitcher] Reverting to previous scene: %s", originalScene_.c_str());

	switchScene(originalScene_);
}
