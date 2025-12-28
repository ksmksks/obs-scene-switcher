// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "scene_switcher.hpp"
#include <obs-frontend-api.h>
#include <obs-module.h>

SceneSwitcher::SceneSwitcher(QObject *parent) : QObject(parent)
{
	revertTimer_.setSingleShot(true);
	connect(&revertTimer_, &QTimer::timeout, this, &SceneSwitcher::onRevertTimeout);
	countdownTimer_.setInterval(1000);
	connect(&countdownTimer_, &QTimer::timeout, this, &SceneSwitcher::onCountdownTick);
}

SceneSwitcher::~SceneSwitcher()
{
}

QStringList SceneSwitcher::getSceneList()
{
	QStringList list;

	obs_frontend_source_list scenes = {};
	obs_frontend_get_scenes(&scenes);

	for (size_t i = 0; i < scenes.sources.num; i++) {
		obs_source_t *src = scenes.sources.array[i];
		const char *name = obs_source_get_name(src);
		list << QString::fromUtf8(name);
	}

	obs_frontend_source_list_free(&scenes);

	return list;
}

void SceneSwitcher::switchScene(const std::string &sceneName)
{
	blog(LOG_DEBUG, "[obs-scene-switcher] switchScene(%s)", sceneName.c_str());

	obs_frontend_source_list scenes = {};
	obs_frontend_get_scenes(&scenes);

	for (size_t i = 0; i < scenes.sources.num; i++) {
		obs_source_t *src = scenes.sources.array[i];
		const char *name = obs_source_get_name(src);

		if (sceneName == name) {
			obs_frontend_set_current_scene(src);
			blog(LOG_DEBUG, "[obs-scene-switcher] Scene switched to %s", name);
			break;
		}
	}

	obs_frontend_source_list_free(&scenes);
}

void SceneSwitcher::switchWithRevert(const RewardRule &rule)
{
	const bool hasRevert = rule.revertSeconds > 0;

	switch (state_) {
	case State::Idle:
		break;

	case State::Switched:
	case State::Reverting:
		// 抑制状態を一時的に表示するが、タイマーは継続
		blog(LOG_DEBUG, "[obs-scene-switcher] Request suppressed - timer continues");
		emit stateChanged(State::Suppressed, revertTimer_.remainingTime() / 1000, currentTargetScene_, originalScene_);
		
		// 一定時間後に Switched 状態に戻す（UI表示のため）
		QTimer::singleShot(1000, this, [this]() {
			if (state_ == State::Switched) {
				int remaining = revertTimer_.remainingTime() / 1000;
				emit stateChanged(State::Switched, remaining, currentTargetScene_, originalScene_);
			}
		});
		return;

	case State::Suppressed:
		// 既に抑制中なら何もしない
		return;
	}

	originalScene_ = getCurrentSceneName();
	currentTargetScene_ = QString::fromStdString(rule.targetScene);

	blog(LOG_DEBUG, "[obs-scene-switcher] Switching to '%s'%s", rule.targetScene.c_str(),
	     hasRevert ? "" : " (no revert)");

	switchScene(rule.targetScene);

	if (!hasRevert) {
		state_ = State::Idle;
		emit stateChanged(State::Idle);
		return;
	}

	state_ = State::Switched;
	totalRevertSeconds_ = rule.revertSeconds;
	emit stateChanged(State::Switched, totalRevertSeconds_, currentTargetScene_, originalScene_);

	revertTimer_.start(rule.revertSeconds * 1000);
	countdownTimer_.start();
}

QString SceneSwitcher::getCurrentSceneName() const
{
	obs_source_t *current = obs_frontend_get_current_scene();
	if (!current)
		return {};

	const char *name = obs_source_get_name(current);
	QString sceneName = name ? name : "";

	obs_source_release(current);
	return sceneName;
}

void SceneSwitcher::onCountdownTick()
{
	// Switched 状態の時のみカウントダウンを更新
	if (state_ != State::Switched)
		return;
	
	int remaining = revertTimer_.remainingTime() / 1000;
	if (remaining >= 0) {
		emit stateChanged(State::Switched, remaining, currentTargetScene_, originalScene_);
	}
}

void SceneSwitcher::onRevertTimeout()
{
	countdownTimer_.stop();
	
	// タイマーが終了した時に Switched 状態でない場合は Idle に戻す
	if (state_ != State::Switched && state_ != State::Suppressed) {
		state_ = State::Idle;
		emit stateChanged(State::Idle);
		return;
	}

	state_ = State::Reverting;
	emit stateChanged(State::Reverting, -1, originalScene_, QString());

	blog(LOG_DEBUG, "[obs-scene-switcher] Reverting to previous scene: %s", originalScene_.toStdString().c_str());

	switchScene(originalScene_.toStdString());

	state_ = State::Idle;
	emit stateChanged(State::Idle);
}
