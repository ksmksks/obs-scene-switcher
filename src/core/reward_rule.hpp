// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <string>

struct RewardRule {
	std::string sourceScene;
	std::string rewardId;
	std::string rewardTitle;
	std::string targetScene;
	int revertSeconds = 0;
};
