// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "rule_row.hpp"
#include <QHBoxLayout>
#include <QToolButton>
#include <QStyle>
#include <QLabel>

RuleRow::RuleRow(QWidget *parent) : QWidget(parent)
{
	auto *layout = new QHBoxLayout(this);
	layout->setContentsMargins(2, 2, 2, 2);
	layout->setSpacing(6);

	// 伸縮可能にするため SizePolicy を設定
	auto expandPolicy = QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	auto fixedPolicy = QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	// 現在シーン
	currentSceneBox_ = new QComboBox(this);
	currentSceneBox_->setSizePolicy(expandPolicy);

	// → 矢印（文字で対応）
	QLabel *arrow1Label = new QLabel("→", this);
	arrow1Label->setStyleSheet("font-weight: bold; font-size: 14px; color: #d0d0d0;");
	arrow1Label->setFixedWidth(20);
	arrow1Label->setSizePolicy(fixedPolicy);
	arrow1Label->setAlignment(Qt::AlignCenter);

	// リワード
	rewardBox_ = new QComboBox(this);
	rewardBox_->setSizePolicy(expandPolicy);

	// → 矢印（文字で対応）
	QLabel *arrow2Label = new QLabel("→", this);
	arrow2Label->setStyleSheet("font-weight: bold; font-size: 14px; color: #d0d0d0;");
	arrow2Label->setFixedWidth(20);
	arrow1Label->setSizePolicy(fixedPolicy);
	arrow2Label->setAlignment(Qt::AlignCenter);

	// 切替先シーン
	targetSceneBox_ = new QComboBox(this);
	targetSceneBox_->setSizePolicy(expandPolicy);

	// ： （文字で対応）
	QLabel *colonLabel = new QLabel("：", this);
	colonLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #d0d0d0;");
	colonLabel->setFixedWidth(20);
	colonLabel->setSizePolicy(fixedPolicy);
	colonLabel->setAlignment(Qt::AlignCenter);

	// 秒数 (単位付き)
	revertSpin_ = new QSpinBox(this);
	revertSpin_->setRange(0, 86400);
	revertSpin_->setValue(10);
	revertSpin_->setSuffix(" sec");
	revertSpin_->setFixedWidth(100);
	revertSpin_->setSizePolicy(fixedPolicy);

	// 削除ボタン
	removeButton_ = new QPushButton("削除", this);
	removeButton_->setFixedWidth(60);
	removeButton_->setSizePolicy(fixedPolicy);

	layout->addWidget(currentSceneBox_);
	layout->addWidget(arrow1Label);
	layout->addWidget(rewardBox_);
	layout->addWidget(arrow2Label);
	layout->addWidget(targetSceneBox_);
	layout->addWidget(colonLabel);
	layout->addWidget(revertSpin_);
	layout->addWidget(removeButton_);

	// 伸縮はコンボ部分に寄せる
	layout->setStretch(0, 2); // currentScene
	layout->setStretch(2, 2); // reward
	layout->setStretch(4, 2); // targetScene

	connect(removeButton_, &QPushButton::clicked, [this]() { emit removeRequested(this); });
}

void RuleRow::setSceneList(const QList<QString> &scenes)
{
	currentSceneBox_->clear();
	targetSceneBox_->clear();
	currentSceneBox_->addItems(scenes);
	targetSceneBox_->addItems(scenes);
}

void RuleRow::setRewardList(const std::vector<RewardInfo> &rewards)
{
	rewardList_ = rewards;
	rewardBox_->clear();

	for (const auto &reward : rewards) {
		// 表示：名前
		// 内部データ：ID
		rewardBox_->addItem(QString::fromStdString(reward.title), QString::fromStdString(reward.id));
	}
}

std::string RuleRow::getSelectedRewardId() const
{
	if (!rewardBox_)
		return "";

	QString id = rewardBox_->currentData().toString();
	return id.toStdString();
}

QString RuleRow::currentScene() const
{
	return currentSceneBox_->currentText();
}

QString RuleRow::reward() const
{
	return rewardBox_->currentText();
}

QString RuleRow::targetScene() const
{
	return targetSceneBox_->currentText();
}

int RuleRow::revertSeconds() const
{
	return revertSpin_->value();
}

std::string RuleRow::rewardId() const
{
	if (!rewardBox_)
		return "";

	return rewardBox_->currentData().toString().toStdString();
}

RewardRule RuleRow::rule() const
{
	RewardRule r;
	r.targetScene = targetSceneBox_->currentText().toStdString();
	r.revertSeconds = revertSpin_->value();
	return r;
}
