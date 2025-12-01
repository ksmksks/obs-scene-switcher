// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "config_manager.hpp"

#include <obs-module.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <windows.h>
#include <wincrypt.h>
#pragma comment(lib, "Crypt32.lib")

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

static std::string DPAPIEncrypt(const std::string &plain)
{
	DATA_BLOB input;
	input.pbData = (BYTE *)plain.data();
	input.cbData = (DWORD)plain.size();

	DATA_BLOB output;
	if (CryptProtectData(&input, L"", NULL, NULL, NULL, 0, &output)) {
		std::string encrypted(reinterpret_cast<char *>(output.pbData), output.cbData);
		LocalFree(output.pbData);
		return encrypted;
	}
	return {};
}

static std::string DPAPIDecrypt(const std::string &encrypted)
{
	DATA_BLOB input;
	input.pbData = (BYTE *)encrypted.data();
	input.cbData = (DWORD)encrypted.size();

	DATA_BLOB output;
	if (CryptUnprotectData(&input, NULL, NULL, NULL, NULL, 0, &output)) {
		std::string decrypted(reinterpret_cast<char *>(output.pbData), output.cbData);
		LocalFree(output.pbData);
		return decrypted;
	}

	DWORD err = GetLastError();
	blog(LOG_ERROR, "[ConfigManager] DPAPI decrypt failed (size=%lu, GetLastError=%lu)", input.cbData, err);

	return {};
}

static std::string base64_encode(const std::string &input)
{
	DWORD outLen = 0;
	CryptBinaryToStringA(reinterpret_cast<const BYTE *>(input.data()), (DWORD)input.size(),
			     CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &outLen);
	std::string output(outLen, '\0');
	CryptBinaryToStringA(reinterpret_cast<const BYTE *>(input.data()), (DWORD)input.size(),
			     CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, &output[0], &outLen);
	if (outLen > 0 && output[outLen - 1] == '\0')
		output.resize(outLen - 1);
	else
		output.resize(outLen);

	return output;
}

static std::string base64_decode(const std::string &input)
{
	if (input.empty()) {
		blog(LOG_ERROR, "[ConfigManager] Base64 decode failed: input is empty");
		return {};
	}

	DWORD outLen = 0;
	if (!CryptStringToBinaryA(input.c_str(), (DWORD)input.length(), CRYPT_STRING_BASE64, nullptr, &outLen, nullptr,
				  nullptr)) {
		blog(LOG_ERROR, "[ConfigManager] Base64 length query failed (input length=%zu)", input.size());
		return {};
	}

	std::string output(outLen, '\0');
	if (!CryptStringToBinaryA(input.c_str(), (DWORD)input.length(), CRYPT_STRING_BASE64,
				  reinterpret_cast<BYTE *>(&output[0]), &outLen, nullptr, nullptr)) {
		blog(LOG_ERROR, "[ConfigManager] Base64 decode failed (input length=%zu)", input.size());
		return {};
	}

	return output;
}

std::string secureEncode(const std::string &plain)
{
	auto enc = DPAPIEncrypt(plain);
	if (enc.empty()) {
		blog(LOG_ERROR, "[ConfigManager] DPAPI encrypt failed (input length=%zu)", plain.size());
		return {};
	}

	auto b64 = base64_encode(enc);
	if (b64.empty()) {
		blog(LOG_ERROR, "[ConfigManager] Base64 encode failed (encrypted length=%zu)", enc.size());
		return {};
	}

	return b64;
}

std::string secureDecode(const std::string &encoded)
{
	auto bin = base64_decode(encoded);
	if (bin.empty()) {
		blog(LOG_ERROR, "[ConfigManager] secureDecode failed: Base64 decode returned empty");
		return {};
	}

	auto dec = DPAPIDecrypt(bin);
	if (dec.empty()) {
		blog(LOG_ERROR, "[ConfigManager] secureDecode failed: DPAPI decrypt returned empty");
		return {};
	}

	return dec;
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
	ofs << "client_secret=" << secureEncode(clientSecret_) << "\n";
	ofs << "access_token=" << secureEncode(accessToken_) << "\n";
	ofs << "refresh_token=" << secureEncode(refreshToken_) << "\n";
	ofs << "expires_at=" << expiresAt_ << "\n";
	ofs << "broadcaster_user_id=" << broadcasterUserId_ << "\n";
	ofs << "broadcaster_login=" << broadcasterLogin_ << "\n";
	ofs << "broadcaster_display_name=" << streamerDisplayName_ << "\n";

	blog(LOG_DEBUG, "[ConfigManager] Settings saved successfully to %s", configPath_.c_str());
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
			clientSecret_ = secureDecode(line.substr(std::string("client_secret=").size()));
		} else if (line.rfind("access_token=", 0) == 0) {
			accessToken_ = secureDecode(line.substr(std::string("access_token=").size()));
		} else if (line.rfind("refresh_token=", 0) == 0) {
			refreshToken_ = secureDecode(line.substr(std::string("refresh_token=").size()));
		} else if (line.rfind("expires_at=", 0) == 0) {
			const auto rawVal = line.substr(std::string("expires_at=").size());
			try {
				expiresAt_ = std::stol(rawVal);
			} catch (...) {
				expiresAt_ = 0;
				blog(LOG_ERROR,
				     "[ConfigManager] Failed to parse expires_at (value=\"%s\"). Defaulting to 0.",
				     rawVal.c_str());
			}
		} else if (line.rfind("broadcaster_user_id=", 0) == 0) {
			broadcasterUserId_ = line.substr(std::string("broadcaster_user_id=").size());
		} else if (line.rfind("broadcaster_login=", 0) == 0) {
			broadcasterLogin_ = line.substr(std::string("broadcaster_login=").size());
		} else if (line.rfind("broadcaster_display_name=", 0) == 0) {
			streamerDisplayName_ = line.substr(std::string("broadcaster_display_name=").size());
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

const std::string &ConfigManager::getAccessToken() const
{
	return accessToken_;
}

const std::string &ConfigManager::getRefreshToken() const
{
	return refreshToken_;
}

long ConfigManager::getTokenExpiresAt() const
{
	return expiresAt_;
}

void ConfigManager::setAccessToken(const std::string &token)
{
	accessToken_ = token;
}

void ConfigManager::setRefreshToken(const std::string &token)
{
	refreshToken_ = token;
}

void ConfigManager::setTokenExpiresAt(long ts)
{
	expiresAt_ = ts;
}

bool ConfigManager::isAuthValid() const
{
	// 初回起動 or 設定が保存されていない
	if (clientId_.empty() || clientSecret_.empty())
		return false;

	// 一度でもログインした状態
	return !accessToken_.empty();
}

bool ConfigManager::isTokenExpired() const
{
	const long now = static_cast<long>(time(nullptr));
	if (expiresAt_ <= 0)
		return true;
	return expiresAt_ <= now;
}

void ConfigManager::setBroadcasterUserId(const std::string &id)
{
	broadcasterUserId_ = id;
}

void ConfigManager::setBroadcasterLogin(const std::string &login)
{
	broadcasterLogin_ = login;
}

void ConfigManager::setBroadcasterDisplayName(const std::string &name)
{
	streamerDisplayName_ = name;
}

const std::string &ConfigManager::getBroadcasterUserId() const
{
	return broadcasterUserId_;
}

const std::string &ConfigManager::getBroadcasterLogin() const
{
	return broadcasterLogin_;
}

const std::string &ConfigManager::getBroadcasterDisplayName() const
{
	return streamerDisplayName_;
}
