// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QVBoxLayout;
class QFrame;

class DockMainWidget : public QWidget {
	Q_OBJECT

public:
	explicit DockMainWidget(QWidget *parent = nullptr);

	// 認証セクション
	void updateAuthStatus(bool authenticated);
	void updateUserInfo();

	// 制御セクション
	void updateEnabledState(bool enabled);
	void setToggleEnabled(bool canToggle);

	// 状態セクション
	void updateState(const QString &stateText);
	void updateCountdown(int seconds);

signals:
	/// 「設定を開く」ボタン
	void settingsRequested();

	/// 有効/無効トグル
	void enableToggleRequested(bool enabled);

	/// ログアウトボタン
	void logoutRequested();

private:
	void addSeparator(QVBoxLayout *layout);
	void applyToggleStyle();

	// 認証セクション
	QLabel *labelAuthStatus_ = nullptr;

	// 状態セクション
	QLabel *labelState_ = nullptr;
	QLabel *labelCountdown_ = nullptr;

	// 制御セクション
	QPushButton *buttonToggleEnabled_ = nullptr;
	QPushButton *logoutButton_ = nullptr;
	QPushButton *buttonSettings_ = nullptr;
};
