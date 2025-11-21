// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <QWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QList>

class RuleRow : public QWidget {
	Q_OBJECT

public:
	explicit RuleRow(QWidget *parent = nullptr);

	void setSceneList(const QStringList &scenes);
	void setRewardList(const QStringList &rewards);

	QString currentScene() const;
	QString reward() const;
	QString targetScene() const;
	int revertSeconds() const;

signals:
	void removeRequested(RuleRow *self);

private:
	QComboBox *currentSceneBox_;
	QComboBox *rewardBox_;
	QComboBox *targetSceneBox_;
	QSpinBox *revertSpin_;
	QPushButton *removeButton_;
};
