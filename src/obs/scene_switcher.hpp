// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include "core/reward_rule.hpp"
#include <QStringList>
#include <QTimer>

class SceneSwitcher : public QObject {
	Q_OBJECT
public:
	enum class State {
		Idle,      // 何もしていない
		Switched,  // 一時的に切り替え中（戻し待ち）
		Reverting, // 元シーンに戻している最中
		Suppressed // 競合により無視中
	};

	explicit SceneSwitcher(QObject *parent = nullptr);
	~SceneSwitcher();

	QStringList getSceneList();

	void switchScene(const std::string &sceneName);
	void switchWithRevert(const RewardRule &rule);

	QString getCurrentSceneName() const;

private slots:
	void onRevertTimeout();

private:
	State state_ = State::Idle;
	QString originalScene_;
	QString currentTargetScene_;
	QTimer revertTimer_;
};
