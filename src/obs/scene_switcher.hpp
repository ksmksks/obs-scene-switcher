// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <QStringList>

class SceneSwitcher {
public:
	SceneSwitcher();
	~SceneSwitcher();

	QStringList getSceneList();

	void switchScene(const std::string &sceneName);
};
