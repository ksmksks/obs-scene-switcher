// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>

class AuthSettingsDialog : public QDialog {
	Q_OBJECT

public:
	explicit AuthSettingsDialog(QWidget *parent = nullptr);

private slots:
	void onSaveClicked();
	void onCancelClicked();

private:
	QLineEdit *clientIdEdit_;
	QLineEdit *clientSecretEdit_;
	QPushButton *saveButton_;
	QPushButton *cancelButton_;
};
