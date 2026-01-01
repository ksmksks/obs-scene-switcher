// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "login_widget.hpp"
#include "auth_settings_dialog.hpp"
#include "../obs_scene_switcher.hpp"
#include "../i18n/locale_manager.hpp"

#include <QVBoxLayout>
#include <QPushButton>

LoginWidget::LoginWidget(QWidget *parent) : QWidget(parent)
{
	auto *layout = new QVBoxLayout(this);

	loginButton_ = new QPushButton(Tr("SceneSwitcher.Button.Login"), this);
	authSettingsButton_ = new QPushButton(Tr("SceneSwitcher.Button.AuthSettings"), this);

	layout->addWidget(loginButton_);
	layout->addWidget(authSettingsButton_);

	connect(loginButton_, &QPushButton::clicked, this, &LoginWidget::onLoginClicked);
	connect(authSettingsButton_, &QPushButton::clicked, this, &LoginWidget::onAuthSettingsClicked);
}

void LoginWidget::onLoginClicked()
{
	// OAuth 開始。UI遷移は ObsSceneSwitcher 側のシグナルで実行
	ObsSceneSwitcher::instance()->startOAuthLogin();
}

void LoginWidget::onAuthSettingsClicked()
{
	AuthSettingsDialog dlg(this);
	dlg.exec();
}
