// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "../oauth/twitch_oauth.hpp"
#include <QDialog>
#include <QStringList>
#include <QVector>
#include <QVBoxLayout>

class RuleRow;

/**
 * 設定ウィンドウ
 *
 * - ルール一覧 [＋ルール]
 * - 1行ごとに RuleRow (現在シーン / リワード / 切替先 / 秒数 / 削除)
 * - [保存] [閉じる]
 */
class SettingsWindow : public QDialog {
	Q_OBJECT
public:
	explicit SettingsWindow(QWidget *parent = nullptr);

	// シーン一覧の更新（ルール行に反映）
	void setSceneList(const QStringList &scenes);

	// リワード一覧の更新（ルール行に反映）
	void setRewardList(const std::vector<RewardInfo> &rewards);

signals:
	// 「保存」が押されたあとに通知（ConfigManager 側で拾う想定）
	void rulesSaved();

private slots:
	void onAddRuleClicked();
	void onSaveClicked();
	void onCloseClicked();

private:
	void addRuleRow();
	void removeRuleRow(RuleRow *row);

	void loadRules(); // FIXME: ConfigManager 実装後に中身を書く
	void saveRules(); // FIXME: ConfigManager 実装後に中身を書く

	QVBoxLayout *rulesLayout_ = nullptr;
	QPushButton *addRuleButton_ = nullptr;
	QPushButton *saveButton_ = nullptr;
	QPushButton *closeButton_ = nullptr;

	// Scene / Reward の候補一覧
	QStringList sceneList_;
	std::vector<RewardInfo> rewardList_;

	// 画面上に並んでいるルール行
	QVector<RuleRow *> ruleRows_;
};
