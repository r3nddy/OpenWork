#pragma once

#include "models.hpp"
#include <string>

using namespace std;

// Build command untuk memanggil scripts/main.py
string buildServicesCommand(const string& args = "");

// Jalankan AI agent Python (interactive REPL mode)
int jalankanAgent();

// Jalankan AI agent Python dengan analisis file JSON hasil scraping
int jalankanAgentAnalisis(const string& jsonFilePath);
