// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <QWidget>
#include <QStackedLayout>

class LoginWidget;
class DockMainWidget;
class SettingsWindow;

class PluginDock : public QWidget {
	Q_OBJECT

public:
	static PluginDock* instance();
	static void registerDock();

	QWidget *getWidget() const { return mainWidget_; }

	void showLogin();
	void showMain();

public slots:
	void onAuthenticationSucceeded();
	void onSettingsRequested();

private:
	PluginDock();
	static PluginDock *s_instance_;

	QWidget *mainWidget_ = nullptr;
	QStackedLayout *layout_ = nullptr;

	LoginWidget *loginWidget_ = nullptr;
	DockMainWidget *mainDockWidget_ = nullptr;
	SettingsWindow *settingsWindow_ = nullptr;
};
