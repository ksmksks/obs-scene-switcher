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

	// ドラッグハンドル（⋮⋮）
	QLabel *dragHandle = new QLabel("⋮⋮", this);
	dragHandle->setStyleSheet("font-size: 18px; color: #808080; padding: 0 4px;");
	dragHandle->setFixedWidth(20);
	dragHandle->setSizePolicy(fixedPolicy);
	dragHandle->setAlignment(Qt::AlignCenter);
	dragHandle->setToolTip("ドラッグして並び替え");
	dragHandle->setCursor(Qt::OpenHandCursor);

	// 有効/無効チェックボックス
	enabledCheckBox_ = new QCheckBox(this);
	enabledCheckBox_->setChecked(true);  // デフォルトは有効
	enabledCheckBox_->setToolTip("ルールを有効/無効にする");
	enabledCheckBox_->setFixedWidth(30);
	enabledCheckBox_->setSizePolicy(fixedPolicy);
	
	// チェックボックスの状態変化で見た目を更新
	connect(enabledCheckBox_, &QCheckBox::toggled, this, &RuleRow::updateVisualState);

	// 現在シーン
	originalSceneBox_ = new QComboBox(this);
	originalSceneBox_->setSizePolicy(expandPolicy);

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
	arrow2Label->setSizePolicy(fixedPolicy);
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

	layout->addWidget(dragHandle);
	layout->addWidget(enabledCheckBox_);
	layout->addWidget(originalSceneBox_);
	layout->addWidget(arrow1Label);
	layout->addWidget(rewardBox_);
	layout->addWidget(arrow2Label);
	layout->addWidget(targetSceneBox_);
	layout->addWidget(colonLabel);
	layout->addWidget(revertSpin_);
	layout->addWidget(removeButton_);

	// 伸縮はコンボ部分に寄せる
	layout->setStretch(2, 2); // currentScene (dragHandle=0, checkbox=1, scene=2)
	layout->setStretch(4, 2); // reward (arrow1=3, reward=4)
	layout->setStretch(6, 2); // targetScene (arrow2=5, target=6)

	connect(removeButton_, &QPushButton::clicked, [this]() { emit removeRequested(this); });
}

void RuleRow::setSceneList(const QList<QString> &scenes)
{
	originalSceneBox_->clear();
	targetSceneBox_->clear();
	originalSceneBox_->addItems(scenes);
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
	return originalSceneBox_->currentText();
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

bool RuleRow::enabled() const
{
	return enabledCheckBox_ ? enabledCheckBox_->isChecked() : true;
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
	r.rewardId = rewardId();  // rewardId を設定
	r.sourceScene = currentScene().toStdString();  // sourceScene を設定
	r.targetScene = targetSceneBox_->currentText().toStdString();
	r.revertSeconds = revertSpin_->value();
	r.enabled = enabled();
	return r;
}

void RuleRow::setRule(const RewardRule &rule)
{
	if (enabledCheckBox_)
		enabledCheckBox_->setChecked(rule.enabled);

	originalSceneBox_->setCurrentText(QString::fromStdString(rule.sourceScene));

	for (int i = 0; i < rewardBox_->count(); ++i) {
		if (rewardBox_->itemData(i).toString().toStdString() == rule.rewardId) {
			rewardBox_->setCurrentIndex(i);
			break;
		}
	}

	targetSceneBox_->setCurrentText(QString::fromStdString(rule.targetScene));
	revertSpin_->setValue(rule.revertSeconds);
	
	updateVisualState();  // 見た目を更新
}

void RuleRow::updateVisualState()
{
	bool isEnabled = enabledCheckBox_ ? enabledCheckBox_->isChecked() : true;
	
	// 無効時はグレーアウト
	qreal opacity = isEnabled ? 1.0 : 0.4;
	
	if (originalSceneBox_)
		originalSceneBox_->setEnabled(isEnabled);
	if (rewardBox_)
		rewardBox_->setEnabled(isEnabled);
	if (targetSceneBox_)
		targetSceneBox_->setEnabled(isEnabled);
	if (revertSpin_)
		revertSpin_->setEnabled(isEnabled);
	
	// 全体の透明度を調整してグレーアウト感を出す
	setStyleSheet(isEnabled ? "" : "QWidget { opacity: 0.5; }");
}
