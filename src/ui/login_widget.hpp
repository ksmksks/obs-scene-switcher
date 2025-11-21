// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>

class LoginWidget : public QWidget {
	Q_OBJECT
public:
	explicit LoginWidget(QWidget *parent = nullptr);

private slots:
	void onLoginClicked();
};
