// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include "core/reward_rule.hpp"
#include <QObject>
#include <QTimer>
#include <QString>
#include <QStringList>

class SceneSwitcher : public QObject {
	Q_OBJECT

public:
	explicit SceneSwitcher(QObject *parent = nullptr);
	~SceneSwitcher() override;

	enum class State {
		Idle,
		Switched,
		Reverting,
		Suppressed
	};

	QStringList getSceneList();
	void switchScene(const std::string &sceneName);
	void switchWithRevert(const RewardRule &rule);
	QString getCurrentSceneName() const;

signals:
	// シーン名を含む詳細な状態通知
	void stateChanged(State newState, int remainingSeconds = -1, 
	                  const QString &targetScene = QString(), 
	                  const QString &originalScene = QString());

private:
	void onRevertTimeout();
	void onCountdownTick();

	State state_ = State::Idle;
	QString originalScene_;
	QString currentTargetScene_;
	QTimer revertTimer_;
	QTimer countdownTimer_;
	int totalRevertSeconds_ = 0;
};
