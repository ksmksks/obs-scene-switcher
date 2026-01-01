/*
OBS Scene Switcher Plugin
Copyright (C) 2025 ksmksks

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <plugin-support.h>

#include "obs_scene_switcher.hpp"
#include "update/update_checker.hpp"

OBS_DECLARE_MODULE()

bool obs_module_load(void)
{
	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);

	ObsSceneSwitcher::instance()->start();
	
	// 非同期でアップデートチェック
	UpdateChecker::checkOnStartupAsync();
	
	return true;
}

void obs_module_unload(void)
{
	ObsSceneSwitcher::instance()->stop();
	ObsSceneSwitcher::destroy();
}

obs_properties_t *obs_module_properties(void)
{
	return NULL;
}
