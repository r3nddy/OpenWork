#pragma once
#include <string>

//  OpenWork — HTTP API Server (port 8080)
// Mulai API server di background thread.

// username: user yang sedang login, dikirim ke React via /api/session
void startApiServerAsync(const std::string& username = "");

void stopApiServer();

bool isApiServerRunning();
