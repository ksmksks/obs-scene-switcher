// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "dock_main_widget.hpp"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

DockMainWidget::DockMainWidget(QWidget *parent) : QWidget(parent)
{
	auto *layout = new QVBoxLayout(this);

	labelScene_ = new QLabel("現在シーン: 未取得", this);
	labelCountdown_ = new QLabel("", this);

        labelAuthStatus_ = new QLabel("認証状態: 未ログイン", this);
	layout->addWidget(labelAuthStatus_);

	buttonTest_ = new QPushButton("テスト切替", this);
	buttonSettings_ = new QPushButton("設定", this);

	layout->addWidget(labelScene_);
	layout->addWidget(labelCountdown_);
	layout->addWidget(buttonTest_);
	layout->addWidget(buttonSettings_);

	connect(buttonTest_, &QPushButton::clicked, this, &DockMainWidget::testSwitchRequested);
	connect(buttonSettings_, &QPushButton::clicked, this, &DockMainWidget::settingsRequested);
}

void DockMainWidget::updateAuthStatus(const QString &status)
{
	if (labelAuthStatus_)
		labelAuthStatus_->setText(status);
}

void DockMainWidget::updateSceneInfo(const QString &sceneName, int remainingSeconds)
{
	labelScene_->setText("現在シーン: " + sceneName);

	if (remainingSeconds >= 0)
		labelCountdown_->setText(QString("元のシーンに戻るまで: %1 秒").arg(remainingSeconds));
	else
		labelCountdown_->clear();
}
