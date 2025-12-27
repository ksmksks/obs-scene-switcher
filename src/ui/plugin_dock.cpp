// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "plugin_dock.hpp"

#include "login_widget.hpp"
#include "dock_main_widget.hpp"
#include "settings_window.hpp"
#include "../obs/config_manager.hpp"
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
	connect(mainDockWidget_, &DockMainWidget::settingsRequested, this, &PluginDock::onSettingsRequested);  // 有効/無効トグル
	connect(mainDockWidget_, &DockMainWidget::enableToggleRequested, [](bool enabled) {
		ObsSceneSwitcher::instance()->setEnabled(enabled);
	});  // 有効状態変更の通知を受け取る
	connect(ObsSceneSwitcher::instance(), &ObsSceneSwitcher::enabledStateChanged,
	        mainDockWidget_, &DockMainWidget::updateEnabledState);

	// ログアウトボタン
	connect(mainDockWidget_, &DockMainWidget::logoutRequested, []() {
		ObsSceneSwitcher::instance()->logout();
		PluginDock::instance()->showLogin();
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

	if (mainDockWidget_) {
		mainDockWidget_->updateAuthStatus(true);
		mainDockWidget_->setToggleEnabled(true);
		
		// 初期状態は無効（起動時は常に無効）
		mainDockWidget_->updateEnabledState(false);
		mainDockWidget_->updateState("⏸ 待機中（無効）");
	}
}

void PluginDock::onAuthenticationFailed()
{
	this->showLogin();
	blog(LOG_INFO, "[Dock] Switched to LoginWidget");

	auto &cfg = ConfigManager::instance();

        // 初期設定 or アクセストークンが保存されていない → 警告不要
	if (cfg.getClientId().empty() || cfg.getClientSecret().empty()) {
		blog(LOG_INFO, "[Dock] First startup – no auth warning shown.");
		return;
	}

	// 過去に認証履歴ある → 警告表示
	QMessageBox::warning(mainWidget_, tr("OBS Scene Switcher プラグイン - 認証エラー"),
			     tr("Twitch へのログインに失敗しました。\n"
				"クライアントID / クライアントシークレットや\n"
				"ネットワーク設定を確認して、再度お試しください。"));
}

void PluginDock::onLoggedOut()
{
	// ログアウト時はエラーダイアログを表示せずにログイン画面に戻る
	this->showLogin();
	blog(LOG_INFO, "[Dock] Logged out - switched to LoginWidget");
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
	if (layout_ && mainDockWidget_) {
		layout_->setCurrentWidget(mainDockWidget_);

		mainDockWidget_->updateAuthStatus(true);
		mainDockWidget_->setToggleEnabled(true);
		
		// 初期状態は無効
		mainDockWidget_->updateEnabledState(false);
		mainDockWidget_->updateState("⏸ 待機中（無効）");
	}
}
