// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "update_checker.hpp"

#include <obs-module.h>
#include <obs-frontend-api.h>

#include "../plugin-support.h"
#include "../i18n/locale_manager.hpp"

#include <nlohmann/json.hpp>

#include <thread>
#include <sstream>
#include <algorithm>
#include <tuple>

#ifdef _WIN32
#define NOMINMAX  // Windows.h の min/max マクロを無効化
#include <Windows.h>
#include <wininet.h>
#include <shellapi.h>
#pragma comment(lib, "Wininet.lib")
#endif

static const char *GITHUB_API_URL = "https://api.github.com/repos/ksmksks/obs-scene-switcher/releases/latest";

void UpdateChecker::checkOnStartupAsync()
{
	blog(LOG_DEBUG, "[obs-scene-switcher] Starting update check...");
	
	// 別スレッドで実行（OBS 起動をブロックしない）
	std::thread checkThread(checkUpdateThread);
	checkThread.detach();
}

void UpdateChecker::checkUpdateThread()
{
	std::string latestVersion;
	std::string releaseUrl;
	
	// GitHub API からバージョン情報を取得
	if (!fetchLatestVersion(latestVersion, releaseUrl)) {
		blog(LOG_DEBUG, "[obs-scene-switcher] Failed to fetch latest version (network issue or API error)");
		return;
	}
	
	// 現在のバージョンと比較
	std::string currentVersion = PLUGIN_VERSION;
	
	if (isNewerVersion(currentVersion, latestVersion)) {
		blog(LOG_INFO, "[obs-scene-switcher] New version available: %s (current: %s)", 
		     latestVersion.c_str(), currentVersion.c_str());
		showUpdateNotification(latestVersion, releaseUrl);
	} else {
		blog(LOG_DEBUG, "[obs-scene-switcher] Already up to date (current: %s, latest: %s)", 
		     currentVersion.c_str(), latestVersion.c_str());
	}
}

bool UpdateChecker::fetchLatestVersion(std::string &outVersion, std::string &outUrl)
{
#ifdef _WIN32
	HINTERNET hInet = InternetOpenA("OBS-SceneSwitcher-UpdateChecker", 
	                                 INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (!hInet) {
		blog(LOG_DEBUG, "[obs-scene-switcher] InternetOpenA failed");
		return false;
	}
	
	HINTERNET hConnect = InternetConnectA(hInet, "api.github.com", 443, 
	                                       NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
	if (!hConnect) {
		blog(LOG_DEBUG, "[obs-scene-switcher] InternetConnectA failed");
		InternetCloseHandle(hInet);
		return false;
	}
	
	HINTERNET hRequest = HttpOpenRequestA(hConnect, "GET", 
	                                       "/repos/ksmksks/obs-scene-switcher/releases/latest",
	                                       NULL, NULL, NULL, 
	                                       INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_CACHE_WRITE, 0);
	if (!hRequest) {
		blog(LOG_DEBUG, "[obs-scene-switcher] HttpOpenRequestA failed");
		InternetCloseHandle(hConnect);
		InternetCloseHandle(hInet);
		return false;
	}
	
	// User-Agent ヘッダーを追加（GitHub API 推奨）
	std::string headers = "User-Agent: OBS-SceneSwitcher-UpdateChecker\r\n";
	
	if (!HttpSendRequestA(hRequest, headers.c_str(), (DWORD)headers.size(), NULL, 0)) {
		blog(LOG_DEBUG, "[obs-scene-switcher] HttpSendRequestA failed");
		InternetCloseHandle(hRequest);
		InternetCloseHandle(hConnect);
		InternetCloseHandle(hInet);
		return false;
	}
	
	// レスポンスを読み取る
	char buffer[8192];
	DWORD read = 0;
	std::string response;
	
	while (InternetReadFile(hRequest, buffer, sizeof(buffer), &read) && read > 0) {
		response.append(buffer, read);
	}
	
	InternetCloseHandle(hRequest);
	InternetCloseHandle(hConnect);
	InternetCloseHandle(hInet);
	
	// JSON パース
	auto json = nlohmann::json::parse(response, nullptr, false);
	if (json.is_discarded()) {
		blog(LOG_DEBUG, "[obs-scene-switcher] Failed to parse JSON response");
		return false;
	}
	
	// tag_name と html_url を取得
	if (!json.contains("tag_name") || !json.contains("html_url")) {
		blog(LOG_DEBUG, "[obs-scene-switcher] JSON missing required fields");
		return false;
	}
	
	std::string tagName = json["tag_name"].get<std::string>();
	outUrl = json["html_url"].get<std::string>();
	
	// "v" プレフィックスを削除（例: "v0.7.0" -> "0.7.0"）
	if (!tagName.empty() && tagName[0] == 'v') {
		outVersion = tagName.substr(1);
	} else {
		outVersion = tagName;
	}
	
	return true;
#else
	// Windows 以外では未実装
	return false;
#endif
}

bool UpdateChecker::isNewerVersion(const std::string &current, const std::string &latest)
{
	auto currentParts = parseVersion(current);
	auto latestParts = parseVersion(latest);
	
	// 各桁を比較
	size_t maxLen = std::max(currentParts.size(), latestParts.size());
	for (size_t i = 0; i < maxLen; ++i) {
		int currentPart = i < currentParts.size() ? currentParts[i] : 0;
		int latestPart = i < latestParts.size() ? latestParts[i] : 0;
		
		if (latestPart > currentPart) {
			return true;  // 新しいバージョン
		} else if (latestPart < currentPart) {
			return false; // 古いバージョン
		}
	}
	
	return false; // 同じバージョン
}

std::vector<int> UpdateChecker::parseVersion(const std::string &version)
{
	std::vector<int> parts;
	std::stringstream ss(version);
	std::string part;
	
	while (std::getline(ss, part, '.')) {
		try {
			parts.push_back(std::stoi(part));
		} catch (...) {
			parts.push_back(0);
		}
	}
	
	return parts;
}

void UpdateChecker::showUpdateNotification(const std::string &latestVersion, const std::string &releaseUrl)
{
	// メインスレッドで通知を表示する必要があるため、Qt のイベントループを使用
	// obs_queue_task を使用して OBS のメインスレッドで実行
	
	// メッセージボックスを表示（簡易実装）
	std::string currentVersion = PLUGIN_VERSION;
	
#ifdef _WIN32
	// メインスレッドで実行
	obs_queue_task(OBS_TASK_UI, [](void *param) {
		auto *data = static_cast<std::tuple<std::string, std::string, std::string>*>(param);
		
		// LocaleManagerを使用して翻訳
		QString message = Tr("SceneSwitcher.Message.UpdateAvailable")
			.arg(QString::fromStdString(std::get<0>(*data)))
			.arg(QString::fromStdString(std::get<1>(*data)));
		QString title = Tr("SceneSwitcher.Message.UpdateTitle");
		
		int result = MessageBoxW(NULL, 
		                         message.toStdWString().c_str(),
		                         title.toStdWString().c_str(),
		                         MB_YESNO | MB_ICONINFORMATION);
		
		if (result == IDYES) {
			// ブラウザでリリースページを開く
			ShellExecuteA(NULL, "open", std::get<2>(*data).c_str(), NULL, NULL, SW_SHOWNORMAL);
		}
		
		delete data;
	}, new std::tuple<std::string, std::string, std::string>(latestVersion, currentVersion, releaseUrl), false);
#endif
}
