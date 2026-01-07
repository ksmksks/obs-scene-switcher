// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "rule_row.hpp"
#include "../i18n/locale_manager.hpp"
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
	dragHandle->setToolTip(Tr("SceneSwitcher.Rule.DragHandle"));
	dragHandle->setCursor(Qt::OpenHandCursor);

	// 有効/無効チェックボックス
	enabledCheckBox_ = new QCheckBox(this);
	enabledCheckBox_->setChecked(true);  // デフォルトは有効
	enabledCheckBox_->setToolTip(Tr("SceneSwitcher.Rule.EnabledCheckbox"));
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
	revertSpin_->setSuffix(QString(" %1").arg(Tr("SceneSwitcher.Rule.Duration")));
	revertSpin_->setFixedWidth(100);
	revertSpin_->setSizePolicy(fixedPolicy);

	// 削除ボタン
	removeButton_ = new QPushButton(Tr("SceneSwitcher.Rule.Remove"), this);
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
	// 現在の選択を保存
	QString currentSourceScene = currentScene();
	QString currentTargetScene = targetSceneBox_->currentText();
	
	// コンボボックスをクリアして再構築
	originalSceneBox_->clear();
	targetSceneBox_->clear();
	
	// 現在シーン用のコンボボックスに「任意(Any)」を先頭に追加
	originalSceneBox_->addItem(Tr("SceneSwitcher.Rule.Any"), "Any");
	originalSceneBox_->addItems(scenes);
	
	// 切替先シーンには通常のシーンのみ
	targetSceneBox_->addItems(scenes);
	
	// 以前の選択を復元
	if (currentSourceScene == "Any") {
		// 「任意(Any)」を選択
		originalSceneBox_->setCurrentIndex(0);
	} else if (!currentSourceScene.isEmpty()) {
		// 特定のシーンを選択（存在する場合）
		int index = originalSceneBox_->findText(currentSourceScene);
		if (index >= 0) {
			originalSceneBox_->setCurrentIndex(index);
		}
	}
	
	// 切替先シーンの選択を復元
	if (!currentTargetScene.isEmpty()) {
		int index = targetSceneBox_->findText(currentTargetScene);
		if (index >= 0) {
			targetSceneBox_->setCurrentIndex(index);
		}
	}
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
	// "任意(Any)" が選択されている場合は "Any" を返す
	QVariant data = originalSceneBox_->currentData();
	if (data.isValid() && data.toString() == "Any") {
		return "Any";
	}
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

	// sourceScene が空または "Any" の場合は「任意(Any)」を選択
	if (rule.sourceScene.empty() || rule.sourceScene == "Any") {
		// 「任意(Any)」のインデックスは0
		originalSceneBox_->setCurrentIndex(0);
	} else {
		originalSceneBox_->setCurrentText(QString::fromStdString(rule.sourceScene));
	}

	for (int i = 0; i < rewardBox_->count(); ++i) {
		if (rewardBox_->itemData(i).toString().toStdString() == rule.rewardId) {
			rewardBox_->setCurrentIndex(i);
			break;
		}
	}

	targetSceneBox_->setCurrentText(QString::fromStdString(rule.targetScene));
	revertSpin_->setValue(rule.revertSeconds);
	
	updateVisualState();
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
