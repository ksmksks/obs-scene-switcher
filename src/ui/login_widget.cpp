// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "login_widget.hpp"
#include "auth_settings_dialog.hpp"
#include "../obs_scene_switcher.hpp"

#include <QVBoxLayout>
#include <QPushButton>

LoginWidget::LoginWidget(QWidget *parent) : QWidget(parent)
{
	auto *layout = new QVBoxLayout(this);

	loginButton_ = new QPushButton("Twitch でログイン", this);
	authSettingsButton_ = new QPushButton("認証設定", this);

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
