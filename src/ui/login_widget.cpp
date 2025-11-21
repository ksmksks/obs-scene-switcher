// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "login_widget.hpp"
#include "../obs_scene_switcher.hpp"

LoginWidget::LoginWidget(QWidget *parent) : QWidget(parent)
{
	auto *layout = new QVBoxLayout(this);

	auto *btn = new QPushButton("Twitch でログイン", this);
	layout->addWidget(btn);

	connect(btn, &QPushButton::clicked, this, &LoginWidget::onLoginClicked);
}

void LoginWidget::onLoginClicked()
{
	// OAuth 開始。UI遷移は ObsSceneSwitcher 側のシグナルで実行
	ObsSceneSwitcher::instance()->startOAuthLogin();
}
