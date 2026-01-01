// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "settings_window.hpp"
#include "rule_row.hpp"
#include "../oauth/twitch_oauth.hpp"
#include "../obs/scene_switcher.hpp"
#include "../obs/config_manager.hpp"
#include "../obs_scene_switcher.hpp"
#include "../i18n/locale_manager.hpp"

#include <obs-module.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QListWidget>

SettingsWindow::SettingsWindow(QWidget *parent) : QDialog(parent)
{
	setWindowTitle(Tr("SceneSwitcher.Settings.Title"));
	setModal(true);
	resize(600, 400);

	auto *mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(8, 8, 8, 8);
	mainLayout->setSpacing(6);

	// ヘッダ「ルール一覧」＋「＋ルール」ボタン
	auto *headerLayout = new QHBoxLayout();
	auto *titleLabel = new QLabel(Tr("SceneSwitcher.Settings.Title"), this);
	QFont titleFont = titleLabel->font();
	titleFont.setBold(true);
	titleLabel->setFont(titleFont);

	addRuleButton_ = new QPushButton(Tr("SceneSwitcher.Settings.AddRule"), this);

	headerLayout->addWidget(titleLabel);
	headerLayout->addStretch();
	headerLayout->addWidget(addRuleButton_);

	mainLayout->addLayout(headerLayout);

	// ルール一覧をドラッグ&ドロップ対応リストに
	rulesListWidget_ = new QListWidget(this);
	rulesListWidget_->setDragDropMode(QAbstractItemView::InternalMove);
	rulesListWidget_->setSelectionMode(QAbstractItemView::SingleSelection);
	rulesListWidget_->setSpacing(2);
	
	// 選択時に青くならないようにスタイルを設定
	rulesListWidget_->setStyleSheet(
		"QListWidget::item:selected { background: transparent; }"
		"QListWidget::item:hover { background: rgba(255, 255, 255, 0.1); }"
		"QListWidget { background: transparent; border: none; }"
	);
	
	// フォーカス時の点線枠も非表示
	rulesListWidget_->setFocusPolicy(Qt::NoFocus);
	
	mainLayout->addWidget(rulesListWidget_, 1);

	// 下部ボタン行 [ 保存 ][ 閉じる ]
	auto *buttonLayout = new QHBoxLayout();
	saveButton_ = new QPushButton(Tr("SceneSwitcher.Settings.Save"), this);
	closeButton_ = new QPushButton(Tr("SceneSwitcher.Settings.Cancel"), this);

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

	loadRules();
}

void SettingsWindow::setSceneList(const QStringList &scenes)
{
	sceneList_ = scenes;

	// 既存の行にも新しい候補を反映
	for (int i = 0; i < rulesListWidget_->count(); ++i) {
		QListWidgetItem *item = rulesListWidget_->item(i);
		RuleRow *row = qobject_cast<RuleRow*>(rulesListWidget_->itemWidget(item));
		if (row)
			row->setSceneList(sceneList_);
	}
}

void SettingsWindow::setRewardList(const std::vector<RewardInfo> &rewards)
{
	rewardList_ = rewards;

	for (int i = 0; i < rulesListWidget_->count(); ++i) {
		QListWidgetItem *item = rulesListWidget_->item(i);
		RuleRow *row = qobject_cast<RuleRow*>(rulesListWidget_->itemWidget(item));
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
	auto *row = new RuleRow(this);

	// 事前に取得済みのシーン／リワード一覧を反映
	if (!sceneList_.isEmpty())
		row->setSceneList(sceneList_);
	if (!rewardList_.empty())
		row->setRewardList(rewardList_);

	// QListWidgetItem を作成
	auto *item = new QListWidgetItem(rulesListWidget_);
	item->setSizeHint(row->sizeHint());
	rulesListWidget_->setItemWidget(item, row);

	// 行側の「削除」ボタンが押されたら、このウィンドウから行を消す
	connect(row, &RuleRow::removeRequested, this, &SettingsWindow::removeRuleRow);
}

void SettingsWindow::removeRuleRow(RuleRow *row)
{
	if (!row)
		return;

	// QListWidget から対応するアイテムを探して削除
	for (int i = 0; i < rulesListWidget_->count(); ++i) {
		QListWidgetItem *item = rulesListWidget_->item(i);
		if (rulesListWidget_->itemWidget(item) == row) {
			delete rulesListWidget_->takeItem(i);
			break;
		}
	}
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
	rulesListWidget_->clear();

	auto &cfg = ConfigManager::instance();
	for (const auto &rule : cfg.getRewardRules()) {
		auto *row = new RuleRow(this);

		row->setSceneList(sceneList_);
		row->setRewardList(rewardList_);
		row->setRule(rule);

		auto *item = new QListWidgetItem(rulesListWidget_);
		item->setSizeHint(row->sizeHint());
		rulesListWidget_->setItemWidget(item, row);
		
		connect(row, &RuleRow::removeRequested, this, &SettingsWindow::removeRuleRow);
	}
}

void SettingsWindow::saveRules()
{
	std::vector<RewardRule> rules;

	// QListWidget の順序でルールを保存（ドラッグ&ドロップでの並び替えを反映）
	for (int i = 0; i < rulesListWidget_->count(); ++i) {
		QListWidgetItem *item = rulesListWidget_->item(i);
		RuleRow *row = qobject_cast<RuleRow*>(rulesListWidget_->itemWidget(item));
		
		if (!row)
			continue;

		RewardRule rule;
		rule.rewardId = row->rewardId();
		rule.sourceScene = row->currentScene().toStdString();
		rule.targetScene = row->targetScene().toStdString();
		rule.revertSeconds = row->revertSeconds();
		rule.enabled = row->enabled();

		if (rule.rewardId.empty() || rule.targetScene.empty())
			continue;

		rules.push_back(std::move(rule));
	}

	auto &cfg = ConfigManager::instance();
	cfg.setRewardRules(rules);
	cfg.save();

	blog(LOG_DEBUG, "[obs-scene-switcher] Saved %zu rules", rules.size());

	// ObsSceneSwitcher に即時反映
	ObsSceneSwitcher::instance()->setRewardRules(rules);
}
