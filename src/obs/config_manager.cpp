// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "config_manager.hpp"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <obs-module.h>

ConfigManager &ConfigManager::instance()
{
	static ConfigManager inst;
	return inst;
}

ConfigManager::ConfigManager()
{
        // OBS のモジュール設定ディレクトリ配下に自前の設定ファイルを作成
	char *path = obs_module_config_path("obs-scene-switcher.conf");
	if (path) {
		configPath_ = path;
		bfree(path);
	} else {
		configPath_.clear();
	}

	load();
}

void ConfigManager::save()
{
	if (configPath_.empty()) {
		blog(LOG_ERROR, "[ConfigManager] configPath_ is empty! Save aborted.");
		return;
	}

	std::filesystem::path path(configPath_);
	std::filesystem::create_directories(path.parent_path());

	std::ofstream ofs(configPath_, std::ios::trunc);
	if (!ofs.is_open()) {
		blog(LOG_ERROR, "[ConfigManager] Failed to open config file for writing: %s", configPath_.c_str());
		return;
	}

	ofs << "client_id=" << clientId_ << "\n";
	ofs << "client_secret=" << clientSecret_ << "\n";

	blog(LOG_INFO, "[ConfigManager] Settings saved successfully to %s", configPath_.c_str());
}

void ConfigManager::load()
{
	if (configPath_.empty())
		return;

	std::ifstream ifs(configPath_);
	if (!ifs.is_open())
		return;

	std::string line;
	while (std::getline(ifs, line)) {
		if (line.rfind("client_id=", 0) == 0) {
			clientId_ = line.substr(std::string("client_id=").size());
		} else if (line.rfind("client_secret=", 0) == 0) {
			clientSecret_ = line.substr(std::string("client_secret=").size());
		}
	}
}

const std::string &ConfigManager::getClientId() const
{
	return clientId_;
}

const std::string &ConfigManager::getClientSecret() const
{
	return clientSecret_;
}

void ConfigManager::setClientId(const std::string &id)
{
	clientId_ = id;
}

void ConfigManager::setClientSecret(const std::string &secret)
{
	clientSecret_ = secret;
}
