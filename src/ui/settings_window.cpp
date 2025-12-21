// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "settings_window.hpp"
#include "rule_row.hpp"
#include "../oauth/twitch_oauth.hpp"
#include "../obs/scene_switcher.hpp"
#include "../obs/config_manager.hpp"
#include "../obs_scene_switcher.hpp"

#include <obs-module.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>

SettingsWindow::SettingsWindow(QWidget *parent) : QDialog(parent)
{
	setWindowTitle(tr("Scene Switcher Settings"));
	setModal(true);
	resize(600, 400);

	auto *mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(8, 8, 8, 8);
	mainLayout->setSpacing(6);

	// ヘッダ「ルール一覧」＋「＋ルール」ボタン
	auto *headerLayout = new QHBoxLayout();
	auto *titleLabel = new QLabel(tr("ルール一覧"), this);
	QFont titleFont = titleLabel->font();
	titleFont.setBold(true);
	titleLabel->setFont(titleFont);

	addRuleButton_ = new QPushButton(tr("＋ ルール"), this);

	headerLayout->addWidget(titleLabel);
	headerLayout->addStretch();
	headerLayout->addWidget(addRuleButton_);

	mainLayout->addLayout(headerLayout);

	// ルール一覧をスクロール領域に
	auto *scrollArea = new QScrollArea(this);
	scrollArea->setWidgetResizable(true);

	QWidget *rulesContainer = new QWidget(scrollArea);
	rulesLayout_ = new QVBoxLayout(rulesContainer);
	rulesLayout_->setContentsMargins(0, 0, 0, 0);
	rulesLayout_->setSpacing(4);
	rulesLayout_->addStretch();

	scrollArea->setWidget(rulesContainer);
	mainLayout->addWidget(scrollArea, 1);

	// 下部ボタン行 [ 保存 ][ 閉じる ]
	auto *buttonLayout = new QHBoxLayout();
	saveButton_ = new QPushButton(tr("保存"), this);
	closeButton_ = new QPushButton(tr("閉じる"), this);

	buttonLayout->addStretch();
	buttonLayout->addWidget(saveButton_);
	buttonLayout->addWidget(closeButton_);

	mainLayout->addLayout(buttonLayout);

	// シグナル接続
	connect(addRuleButton_, &QPushButton::clicked, this, &SettingsWindow::onAddRuleClicked);
	connect(saveButton_, &QPushButton::clicked, this, &SettingsWindow::onSaveClicked);
	connect(closeButton_, &QPushButton::clicked, this, &SettingsWindow::onCloseClicked);

	// ---- ここで SceneSwitcher からシーン一覧を取得する ----
	{
		SceneSwitcher sceneSwitcherTool;
		sceneList_ = sceneSwitcherTool.getSceneList();
	}

	// ---- Twitch Reward list を取得 ----
	{
		// 初回ロード時にリワード一覧を取得
		rewardList_ = ObsSceneSwitcher::instance()->getRewardList();
	}

	// TODO: 設定ファイルからルールを読み込んで行を追加する
	loadRules();
}

void SettingsWindow::setSceneList(const QStringList &scenes)
{
	sceneList_ = scenes;

	// 既存の行にも新しい候補を反映
	for (RuleRow *row : ruleRows_) {
		if (row)
			row->setSceneList(sceneList_);
	}
}

void SettingsWindow::setRewardList(const std::vector<RewardInfo> &rewards)
{
	rewardList_ = rewards;

	for (RuleRow *row : ruleRows_) {
		if (row)
			row->setRewardList(rewardList_);
	}
}

void SettingsWindow::onAddRuleClicked()
{
	addRuleRow();
}

void SettingsWindow::addRuleRow()
{
	// ストレッチを一旦取り除く
	if (rulesLayout_->itemAt(rulesLayout_->count() - 1)->spacerItem()) {
		QLayoutItem *spacer = rulesLayout_->takeAt(rulesLayout_->count() - 1);
		delete spacer;
	}

	auto *row = new RuleRow(this);

	// 事前に取得済みのシーン／リワード一覧を反映
	if (!sceneList_.isEmpty())
		row->setSceneList(sceneList_);
	if (!rewardList_.empty())
		row->setRewardList(rewardList_);

	rulesLayout_->addWidget(row);
	ruleRows_.append(row);

	// 末尾に再度ストレッチ
	rulesLayout_->addStretch();

	// 行側の「削除」ボタンが押されたら、このウィンドウから行を消す
	connect(row, &RuleRow::removeRequested, this, &SettingsWindow::removeRuleRow);
}

void SettingsWindow::removeRuleRow(RuleRow *row)
{
	if (!row)
		return;

	ruleRows_.removeOne(row);
	rulesLayout_->removeWidget(row);
	row->deleteLater();
}

void SettingsWindow::onSaveClicked()
{
	// ここで ruleRows からルールをかき集めて ConfigManager に保存する想定
	saveRules();

	emit rulesSaved();
        
	accept();
}

void SettingsWindow::onCloseClicked()
{
	close();
}

void SettingsWindow::loadRules()
{
	qDeleteAll(ruleRows_);
	ruleRows_.clear();

	if (rulesLayout_->count() > 0) {
		QLayoutItem *last = rulesLayout_->itemAt(rulesLayout_->count() - 1);
		if (last && last->spacerItem()) {
			delete rulesLayout_->takeAt(rulesLayout_->count() - 1);
		}
	}

	auto &cfg = ConfigManager::instance();
	for (const auto &rule : cfg.getRewardRules()) {
		auto *row = new RuleRow(this);

		row->setSceneList(sceneList_);
		row->setRewardList(rewardList_);
		row->setRule(rule);

		rulesLayout_->addWidget(row);
		ruleRows_.append(row);
	}

	rulesLayout_->addStretch();
}

void SettingsWindow::saveRules()
{
	std::vector<RewardRule> rules;

	for (RuleRow *row : ruleRows_) {
		if (!row)
			continue;

		RewardRule rule;
		rule.rewardId = row->rewardId();
		rule.sourceScene = row->currentScene().toStdString();
		rule.targetScene = row->targetScene().toStdString();
		rule.revertSeconds = row->revertSeconds();

		if (rule.rewardId.empty() || rule.targetScene.empty())
			continue;

		rules.push_back(std::move(rule));
	}

	auto &cfg = ConfigManager::instance();
	cfg.setRewardRules(rules);
	cfg.save();

	blog(LOG_INFO, "[Settings] Saved %zu rules", rules.size());

	// ObsSceneSwitcher に即時反映
	ObsSceneSwitcher::instance()->setRewardRules(rules);
}
