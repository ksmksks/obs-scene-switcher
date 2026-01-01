// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <vector>

class UpdateChecker {
public:
	// OBS 起動時に非同期でバージョンチェックを開始
	static void checkOnStartupAsync();

private:
	// GitHub Releases API からバージョン情報を取得
	static bool fetchLatestVersion(std::string &outVersion, std::string &outUrl);
	
	// バージョン文字列を比較（例: "0.7.0" < "0.8.0"）
	static bool isNewerVersion(const std::string &current, const std::string &latest);
	
	// バージョン文字列を数値配列に変換（例: "0.7.0" -> [0, 7, 0]）
	static std::vector<int> parseVersion(const std::string &version);
	
	// OBS に更新通知を表示
	static void showUpdateNotification(const std::string &latestVersion, const std::string &releaseUrl);
	
	// バックグラウンドスレッドでの処理
	static void checkUpdateThread();
};
