// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QVBoxLayout;

class DockMainWidget : public QWidget {
	Q_OBJECT

public:
	explicit DockMainWidget(QWidget *parent = nullptr);

	/// 現在シーン名と残り秒数の表示更新
	void updateSceneInfo(const QString &sceneName, int remainingSeconds = -1);

signals:
	/// 「設定を開く」ボタン
	void settingsRequested();

	/// 「テスト切替」ボタン
	void testSwitchRequested();

private:
	QLabel *labelScene_;
	QLabel *labelCountdown_;
	QPushButton *buttonSettings_;
	QPushButton *buttonTest_;
};
