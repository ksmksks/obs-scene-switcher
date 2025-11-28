// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "plugin_dock.hpp"

#include "login_widget.hpp"
#include "dock_main_widget.hpp"
#include "settings_window.hpp"
#include "../obs_scene_switcher.hpp"

#include <obs-frontend-api.h>
#include <QMessageBox>

PluginDock *PluginDock::s_instance_ = nullptr;

PluginDock *PluginDock::instance()
{
	if (!s_instance_)
		s_instance_ = new PluginDock();
	return s_instance_;
}

PluginDock::PluginDock()
{
	// メイン QWidget
	mainWidget_ = new QWidget();
	layout_ = new QStackedLayout(mainWidget_);

	// Login / Settings を作る
	loginWidget_ = new LoginWidget(mainWidget_);
	mainDockWidget_ = new DockMainWidget(mainWidget_);

	// Stack に追加（index 0: login, 1: main）
	layout_->addWidget(loginWidget_);
	layout_->addWidget(mainDockWidget_);

	// 初期状態はログイン画面
	layout_->setCurrentWidget(loginWidget_);

	// SettingsWindow のボタンシグナル
	connect(mainDockWidget_, &DockMainWidget::settingsRequested, this, &PluginDock::onSettingsRequested);

	QObject::connect(mainDockWidget_, &DockMainWidget::testSwitchRequested, []() {
		// TODO: テスト用にシーン切り替えを呼ぶ処理
	});
}

void PluginDock::registerDock()
{
	PluginDock *dock = PluginDock::instance();

	bool ok = obs_frontend_add_dock_by_id("scene_switcher_dock", "Scene Switcher", (void *)dock->getWidget());

	if (!ok) {
		blog(LOG_ERROR, "[SceneSwitcher] Failed to add dock");
	} else {
		blog(LOG_INFO, "[SceneSwitcher] Dock added");
	}

	// SceneSwitcher に Dock を渡しておく
	ObsSceneSwitcher::instance()->setPluginDock(dock);
}

void PluginDock::onAuthenticationSucceeded()
{
	this->showMain();
	blog(LOG_INFO, "[Dock] Switched to MainDockWidget");
}

void PluginDock::onAuthenticationFailed()
{
	this->showLogin();
	blog(LOG_INFO, "[Dock] Switched to LoginWidget");

        // ログイン失敗メッセージ表示
	if (mainWidget_) {
		QMessageBox::warning(mainWidget_, tr("OBS Scene Switcher プラグイン - 認証エラー"),
				     tr("Twitch へのログインに失敗しました。\n"
					"クライアントID / クライアントシークレットや\n"
					"ネットワーク設定を確認して、再度お試しください。"));
	}
}

void PluginDock::onSettingsRequested()
{
	if (!settingsWindow_) {
		settingsWindow_ = new SettingsWindow(nullptr);
	}

	settingsWindow_->show();
	settingsWindow_->raise();
	settingsWindow_->activateWindow();
}

void PluginDock::showLogin()
{
	if (layout_ && loginWidget_)
		layout_->setCurrentWidget(loginWidget_);
}

void PluginDock::showMain()
{
	if (layout_ && mainDockWidget_)
		layout_->setCurrentWidget(mainDockWidget_);
}
