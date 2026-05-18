#include "bridge/python_bridge.hpp"
#include "utils.hpp"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;
using namespace std;

//  Helper: Bangun command subprocess yang reliable di Windows & Linux

string buildServicesCommand(const string& args) {
    fs::path akarProyek = fs::current_path();
    fs::path dirScripts = akarProyek / "scripts";

    fs::path pythonVenv;
    #ifdef _WIN32
        pythonVenv = akarProyek / "scripts" / ".venv" / "Scripts" / "python.exe";
    #else
        pythonVenv = akarProyek / "scripts" / ".venv" / "bin" / "python";
    #endif

    string perintahScript = "\"" + pythonVenv.string() + "\" main.py";
    if (!args.empty()) {
        perintahScript += " " + args;
    }

    #ifdef _WIN32
        return "cmd /c \"cd /d \"" + dirScripts.string() + "\" && " + perintahScript + "\"";
    #else
        return "cd \"" + dirScripts.string() + "\" && " + perintahScript;
    #endif
}

//  Jalankan AI Agent — Mode Chat Bebas

int jalankanAgent() {
    if (!fs::exists("scripts/main.py")) {
        CLEAR;
        cout << "\n+==========================================================+" << endl;
        cout << "|              CHAT AI AGENT — OPENWORK                    |" << endl;
        cout << "+==========================================================+" << endl;
        cout << "|  [ERROR] Script \"scripts/main.py\" tidak ditemukan!      |" << endl;
        cout << "|  Pastikan folder scripts/ ada di root project.           |" << endl;
        cout << "+==========================================================+" << endl;
        return -1;
    }

    string cmd = buildServicesCommand("chat");

    CLEAR;
    cout << "\n+==========================================================+" << endl;
    cout << "|              CHAT AI AGENT - OPENWORK                    |" << endl;
    cout << "+==========================================================+" << endl;
    cout << "|  Mode    : Chat Bebas dengan AI                          |" << endl;
    cout << "|  Tips    : Tanya lowongan, skill, atau saran karier      |" << endl;
    cout << "|  Keluar  : Ketik 'exit' atau 'quit' di dalam chat        |" << endl;
    cout << "|  Reset   : Ketik '/reset' untuk hapus konteks chat       |" << endl;
    cout << "+==========================================================+" << endl;
    cout << endl;
    cout << "  Memuat AI Agent..." << endl;
    cout << endl;

    int hasil = system(cmd.c_str());

    cout << endl;
    cout << "+==========================================================+" << endl;
    if (hasil == 0) {
        cout << "|  [OK] Sesi chat selesai. Sampai jumpa!                   |" << endl;
    } else {
        cout << "|  [ERROR] Agent gagal (exit code: " << hasil << ")";
        // Padding agar lebar konsisten
        int padLen = 26 - (int)to_string(hasil).length();
        if (padLen > 0) cout << string(padLen, ' ');
        cout << "|" << endl;
    }
    cout << "+==========================================================+" << endl;

    return hasil;
}

// ═══════════════════════════════════════════════════════
//  Jalankan AI Agent — Mode Analisis File JSON
// ═══════════════════════════════════════════════════════

int jalankanAgentAnalisis(const string& pathFileScraping) {
    if (!fs::exists("scripts/main.py")) {
        CLEAR;
        cout << "\n+==========================================================+" << endl;
        cout << "|           AI AGENT — ANALISIS LOWONGAN                  |" << endl;
        cout << "+==========================================================+" << endl;
        cout << "|  [ERROR] Script \"scripts/main.py\" tidak ditemukan!      |" << endl;
        cout << "+==========================================================+" << endl;
        return -1;
    }

    if (!fs::exists(pathFileScraping)) {
        CLEAR;
        cout << "\n+==========================================================+" << endl;
        cout << "|           AI AGENT — ANALISIS LOWONGAN                  |" << endl;
        cout << "+==========================================================+" << endl;
        cout << "|  [ERROR] File hasil scraping tidak ditemukan!            |" << endl;
        cout << "+==========================================================+" << endl;
        return -1;
    }

    fs::path pathAbsolut = fs::absolute(pathFileScraping);
    string cmd = buildServicesCommand("analyze \"" + pathAbsolut.string() + "\"");

    CLEAR;
    cout << "\n+==========================================================+" << endl;
    cout << "|           AI AGENT — ANALISIS LOWONGAN                  |" << endl;
    cout << "+==========================================================+" << endl;
    cout << "|  Mode    : Analisis pasar kerja dari hasil scraping      |" << endl;
    cout << "|  Output  : Skill terpanas, gaji, perusahaan, insight     |" << endl;
    cout << "+==========================================================+" << endl;
    cout << endl;
    cout << "  Memuat AI Agent..." << endl;
    cout << endl;

    int hasil = system(cmd.c_str());

    cout << endl;
    cout << "+==========================================================+" << endl;
    if (hasil == 0) {
        cout << "|  [OK] Analisis selesai.                                  |" << endl;
    } else {
        cout << "|  [ERROR] Analisis gagal (exit code: " << hasil << ")";
        int padLen = 24 - (int)to_string(hasil).length();
        if (padLen > 0) cout << string(padLen, ' ');
        cout << "|" << endl;
    }
    cout << "+==========================================================+" << endl;

    return hasil;
}


