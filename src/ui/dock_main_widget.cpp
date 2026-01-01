// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "dock_main_widget.hpp"
#include "../obs/config_manager.hpp"
#include "../i18n/locale_manager.hpp"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFrame>

DockMainWidget::DockMainWidget(QWidget *parent) : QWidget(parent)
{
	auto *mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(8, 8, 8, 8);
	mainLayout->setSpacing(4);

	// ========== 認証セクション ==========
	labelAuthStatus_ = new QLabel(this);
	updateAuthStatus(false);  // 初期は未認証
	
	mainLayout->addWidget(labelAuthStatus_);
	addSeparator(mainLayout);

	// ========== 状態セクション ==========
	labelState_ = new QLabel(Tr("SceneSwitcher.Status.Disabled"), this);
	labelCountdown_ = new QLabel("", this);
	
	// カウントダウンのスペースを常に確保
	labelCountdown_->setMinimumHeight(20);
	labelCountdown_->setText("");  // 初期は空
	
	mainLayout->addWidget(labelState_);
	mainLayout->addWidget(labelCountdown_);

	// ========== 制御セクション ==========
	buttonToggleEnabled_ = new QPushButton(Tr("SceneSwitcher.Button.Disable"), this);
	buttonToggleEnabled_->setCheckable(true);
	buttonToggleEnabled_->setChecked(false);
	buttonToggleEnabled_->setEnabled(false);
	buttonToggleEnabled_->setMinimumHeight(36);
	
	logoutButton_ = new QPushButton(Tr("SceneSwitcher.Button.Logout"), this);
	logoutButton_->setMinimumHeight(36);
	logoutButton_->setVisible(false);  // 認証後に表示
	
	buttonSettings_ = new QPushButton(Tr("SceneSwitcher.Button.Settings"), this);
	buttonSettings_->setMinimumHeight(36);
	
	applyToggleStyle();
	
	mainLayout->addWidget(buttonToggleEnabled_);
	mainLayout->addWidget(logoutButton_);
	mainLayout->addWidget(buttonSettings_);
	
	mainLayout->addStretch();

	// シグナル接続
	connect(buttonToggleEnabled_, &QPushButton::toggled, 
	        this, &DockMainWidget::enableToggleRequested);
	connect(buttonSettings_, &QPushButton::clicked, 
	        this, &DockMainWidget::settingsRequested);
	connect(logoutButton_, &QPushButton::clicked,
	        this, &DockMainWidget::logoutRequested);
}

void DockMainWidget::addSeparator(QVBoxLayout *layout)
{
	auto *line = new QFrame(this);
	line->setFrameShape(QFrame::HLine);
	line->setFrameShadow(QFrame::Plain);
	line->setLineWidth(1);
	line->setStyleSheet("QFrame { color: #3E3E42; border: none; background-color: #3E3E42; max-height: 1px; }");
	layout->addWidget(line);
}

void DockMainWidget::updateAuthStatus(bool authenticated)
{
	if (!labelAuthStatus_)
		return;
	
	if (!authenticated) {
		labelAuthStatus_->setText(Tr("SceneSwitcher.Message.NotAuthenticated"));
		if (logoutButton_)
			logoutButton_->setVisible(false);
		return;
	}
	
	auto &cfg = ConfigManager::instance();
	QString displayName = QString::fromStdString(cfg.getBroadcasterDisplayName());
	QString login = QString::fromStdString(cfg.getBroadcasterLogin());
	
	QString text = QString("🟢 %1 (%2)").arg(displayName).arg(login);
	
	labelAuthStatus_->setText(text);
	
	if (logoutButton_)
		logoutButton_->setVisible(true);
}

void DockMainWidget::updateUserInfo()
{
	updateAuthStatus(true);
}

void DockMainWidget::updateState(const QString &stateText)
{
	if (labelState_)
		labelState_->setText(stateText);
}

void DockMainWidget::updateCountdown(int seconds)
{
	if (!labelCountdown_)
		return;
		
	if (seconds >= 0) {
		QString countdownText = Tr("SceneSwitcher.Countdown.Seconds").arg(seconds);
		labelCountdown_->setText(QString("⏱ %1").arg(countdownText));
	} else {
		labelCountdown_->setText(" ");
	}
}

void DockMainWidget::applyToggleStyle()
{
	if (!buttonToggleEnabled_)
		return;
		
	buttonToggleEnabled_->setStyleSheet(
		"QPushButton {"
		"  border: 2px solid #555;"
		"  border-radius: 4px;"
		"  padding: 8px;"
		"  font-weight: bold;"
		"}"
		"QPushButton:checked {"
		"  background-color: #4CAF50;"
		"  color: white;"
		"  border-color: #4CAF50;"
		"}"
		"QPushButton:!checked {"
		"  background-color: #424242;"
		"  color: #AAA;"
		"}"
		"QPushButton:disabled {"
		"  background-color: #2A2A2A;"
		"  color: #666;"
		"  border-color: #444;"
		"}"
	);
}

void DockMainWidget::updateEnabledState(bool enabled)
{
	if (!buttonToggleEnabled_)
		return;
		
	buttonToggleEnabled_->blockSignals(true);
	buttonToggleEnabled_->setChecked(enabled);
	
	if (enabled) {
		buttonToggleEnabled_->setText(Tr("SceneSwitcher.Button.Enable"));
	} else {
		buttonToggleEnabled_->setText(Tr("SceneSwitcher.Button.Disable"));
	}
	
	buttonToggleEnabled_->blockSignals(false);
}

void DockMainWidget::setToggleEnabled(bool canToggle)
{
	if (buttonToggleEnabled_)
		buttonToggleEnabled_->setEnabled(canToggle);
}
