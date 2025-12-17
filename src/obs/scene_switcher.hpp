// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <QStringList>
#include <QTimer>

class SceneSwitcher : public QObject {
	Q_OBJECT
public:
	explicit SceneSwitcher(QObject *parent = nullptr);
	~SceneSwitcher();

	QStringList getSceneList();

	void switchScene(const std::string &sceneName);
	void switchWithRevert(const std::string &targetScene, int revertSeconds);

	std::string getCurrentSceneName() const;

private slots:
	void revertScene();

private:
	std::string originalScene_;
	QTimer revertTimer_;
};
