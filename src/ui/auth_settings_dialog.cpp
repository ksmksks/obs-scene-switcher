// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "auth_settings_dialog.hpp"
#include "../obs/config_manager.hpp"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

AuthSettingsDialog::AuthSettingsDialog(QWidget *parent) : QDialog(parent)
{
	setWindowTitle(tr("認証設定"));
	resize(400, 180);
	setModal(true);

	auto *layout = new QVBoxLayout(this);
	auto *formLayout = new QFormLayout();

	// ConfigManager から既存値を取得
	auto &cfg = ConfigManager::instance();

	// Client ID
	clientIdEdit_ = new QLineEdit(this);
	clientIdEdit_->setText(QString::fromStdString(cfg.getClientId()));
	formLayout->addRow(new QLabel(tr("Client ID"), this), clientIdEdit_);

	// Client Secret
	clientSecretEdit_ = new QLineEdit(this);
	clientSecretEdit_->setEchoMode(QLineEdit::Password);
	clientSecretEdit_->setText(QString::fromStdString(cfg.getClientSecret()));
	formLayout->addRow(new QLabel(tr("Client Secret"), this), clientSecretEdit_);

	layout->addLayout(formLayout);

	// ボタン行
	auto *buttonLayout = new QHBoxLayout();
	saveButton_ = new QPushButton(tr("保存"), this);
	cancelButton_ = new QPushButton(tr("キャンセル"), this);

	buttonLayout->addStretch();
	buttonLayout->addWidget(saveButton_);
	buttonLayout->addWidget(cancelButton_);

	layout->addLayout(buttonLayout);

	// シグナル
	connect(saveButton_, &QPushButton::clicked, this, &AuthSettingsDialog::onSaveClicked);
	connect(cancelButton_, &QPushButton::clicked, this, &AuthSettingsDialog::onCancelClicked);
}

void AuthSettingsDialog::onSaveClicked()
{
	auto &cfg = ConfigManager::instance();
	cfg.setClientId(clientIdEdit_->text().toStdString());
	cfg.setClientSecret(clientSecretEdit_->text().toStdString());
	cfg.save();

	accept();
}

void AuthSettingsDialog::onCancelClicked()
{
	reject();
}
