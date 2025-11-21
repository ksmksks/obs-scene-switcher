// obs-scene-switcher plugin
// Copyright (C) 2025 ksmksks
// SPDX-License-Identifier: GPL-2.0-or-later

#include "http_server.hpp"
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

HttpServer *HttpServer::instance()
{
	static HttpServer instance;
	return &instance;
}

void HttpServer::start(int port, std::function<void(const std::string &)> cb)
{
	if (running_)
		return;

	running_ = true;
	callback_ = cb;

	serverThread_ = std::thread([=]() {
		WSAData wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);

		SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = INADDR_ANY;

		bind(serverSocket, (sockaddr *)&addr, sizeof(addr));
		listen(serverSocket, 1);

		while (running_) {
			SOCKET client = accept(serverSocket, NULL, NULL);
			if (client == INVALID_SOCKET)
				continue;

			char buffer[4096] = {};
			int len = recv(client, buffer, sizeof(buffer), 0);
			if (len > 0) {
				std::string req(buffer, len);

				// GET /callback?code=XXXX
				auto pos = req.find("GET /callback?code=");
				if (pos != std::string::npos) {
					auto codeStart = pos + strlen("GET /callback?code=");
					auto codeEnd = req.find(" ", codeStart);
					std::string code = req.substr(codeStart, codeEnd - codeStart);

					if (callback_)
						callback_(code);

					std::string res = "HTTP/1.1 200 OK\r\n"
							  "Content-Type: text/html\r\n\r\n"
							  "<h1>Authentication successful!</h1>"
							  "You can close this window.";
					send(client, res.c_str(), (int)res.size(), 0);
				}
			}

			closesocket(client);
		}

		WSACleanup();
	});
}

void HttpServer::stop()
{
	running_ = false;
}
