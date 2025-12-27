// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <functional>
#include <string>
#include <thread>
#include <atomic>

class HttpServer {
public:
	static HttpServer *instance();

	void start(int port, std::function<void(const std::string &)> callback);
	void stop();

private:
	HttpServer() = default;
	std::thread serverThread_;
	std::atomic<bool> running_ = false;

	std::function<void(const std::string &)> callback_;
};
