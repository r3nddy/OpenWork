#include "constants.hpp"
#include "services/job_service.hpp"
#include "services/profile_service.hpp"
#include "services/auth_service.hpp"
#include "core/json_storage.hpp"
#include "bridge/python_bridge.hpp"
#include "utils.hpp"

#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;
using namespace std;


// Auto-detect project root (cari CMakeLists.txt ke atas dari lokasi exe)
void setProjectRoot(bool silent = false) {
    #ifdef _WIN32
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        fs::path dir = fs::path(exePath).parent_path();
    #else
        fs::path dir = fs::current_path();
    #endif

    // Naik ke atas sampai ketemu CMakeLists.txt (max 5 level)
    for (int i = 0; i < 5; i++) {
        if (fs::exists(dir / "CMakeLists.txt")) {
            fs::current_path(dir);
            if (!silent) cout << "[OK] Project root: " << dir.string() << endl;
            return;
        }
        dir = dir.parent_path();
    }
    // Fallback: tetap di current directory
    if (!silent) {
        cout << "[WARN] CMakeLists.txt tidak ditemukan, menggunakan: "
             << fs::current_path().string() << endl;
    }
}

//  Menu User
void handleUserMenu(User *currentUser) {
    while (true) {
        CLEAR;
        cout << ABU_REDUP << "\n+==========================================================+" << endl;
        cout << EMAS      << "|                   OPENWORK - MENU USER                   |" << endl;
        cout << ABU_REDUP << "+==========================================================+" << endl;
        cout << PUTIH     << "| No | Fitur                                               |" << endl;
        cout << ABU_TERANG << "|----|-----------------------------------------------------|" << endl;
        cout << PUTIH     << "|  1 | Profile                                             |" << endl;
        cout << PUTIH     << "|  2 | Cari Kerja (Scrape + Lihat Hasil)                   |" << endl;
        cout << PUTIH     << "|  3 | Chat AI Agent (OpenWork)                            |" << endl;
        cout << ABU_TERANG << "|----|-----------------------------------------------------|" << endl;
        cout << PUTIH     << "|  0 | Kembali ke Menu Utama                               |" << endl;
        cout << ABU_REDUP << "+==========================================================+" << RESET << endl;

        int menu = inputMenu(0, 3);

        switch (menu) {
            case 1: handleProfileMenu(currentUser->username); break;
            case 2: menuCariKerja(currentUser); break;
            case 3:jalankanAgent(); break;
            case 0: return;
        }
    }
}

//  Menu Admin
void handleAdminMenu(User *users, int *jumlahUser, User *currentUser) {
    while (true) {
        CLEAR;
        cout << ABU_REDUP << "\n+==========================================================+" << endl;
        cout << EMAS      << "|                  OPENWORK - MENU ADMIN                   |" << endl;
        cout << ABU_REDUP << "+==========================================================+" << endl;
        cout << PUTIH     << "| No | Fitur                                               |" << endl;
        cout << ABU_TERANG << "|----|-----------------------------------------------------|" << endl;
        cout << PUTIH     << "|  1 | Cari & Kelola Lowongan (CRUD Job)                   |" << endl;
        cout << PUTIH     << "|  2 | Audit User (Lihat & Blokir User)                    |" << endl;
        cout << PUTIH     << "|  3 | Tambah Akun Admin Baru                              |" << endl;
        cout << ABU_TERANG << "|----|-----------------------------------------------------|" << endl;
        cout << PUTIH     << "|  0 | Kembali ke Menu Login                               |" << endl;
        cout << ABU_REDUP << "+==========================================================+" << RESET << endl;

        int menu = inputMenu(0, 3);

        switch (menu) {
            case 1: handleLowonganMenu(currentUser->username); break;
            case 2: handleAuditUserMenu(users, jumlahUser); break;
            case 3: tambahAdmin(users, jumlahUser); break;
            case 0: return;
        }
    }
}

//  Menu Utama — Pilih Role

void asciiArt() {

    const string KUNING_INVINCIBLE = "\033[38;2;255;229;86m";
    const string BIRU_INVINCIBLE = "\033[38;2;0;188;240m";
    const string PUTIH_INVINCIBLE = "\033[38;2;255;255;255m";
    const string RESET = "\033[0m";

    cout << PUTIH_INVINCIBLE   << "  ───────────────────────────────────────────────────" << endl;
    cout << KUNING_INVINCIBLE << "        █▀█ █▀█ ██▀ █▄ █ "<< BIRU_INVINCIBLE << " █   █ █▀█ █▀█ █▄▀ " << endl;
    cout << KUNING_INVINCIBLE << "        █▄█ █▀▀ █▄▄ █ ▀█ "<< BIRU_INVINCIBLE << " ▀▄▀▄▀ █▄█ █▀▄ █▀▄ " << endl;
    cout << PUTIH_INVINCIBLE   << "  ───────────────────────────────────────────────────" << RESET <<endl;
}

int main(int argc, char* argv[]) {
    
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    #endif
    
    bool isBridgeCommand = (argc > 1 && string(argv[1]).find("bridge-") == 0);
    setProjectRoot(isBridgeCommand); // Set working directory ke project root, silent jika bridge

    // Bridge : menangani perintah bridge dari Python
    if (isBridgeCommand && handleBridgeCommand(argc, argv)) {
        return 0;
    }

    // init users
    User users[MAX_USERS];
    int jumlahUser = 0;

    // load data user dari json
    loadUsers(users, &jumlahUser);

    while (true) {
        CLEAR;
        asciiArt();
        displayMenuAuth();

        int auth = inputMenu(1, 3);

        if (auth == 1) {
            int idx = login(users, &jumlahUser);
            if (idx == -1) {
                exit(0);
            } else if (idx == -2) {
                continue;
            }
            
            // Masuk ke menu spesifik sesuai role (Admin/User)
            User *ptrCurrentUser = users + idx;
            if (ptrCurrentUser->isAdmin) {
                handleAdminMenu(users, &jumlahUser, ptrCurrentUser);
            } else {
                handleUserMenu(ptrCurrentUser);
            }
            
        } else if (auth == 2) {
            registerUser(users, &jumlahUser);
        } else if (auth == 3) {
            CLEAR;
            cout << "\n  Shutting down... Program akan keluar." << endl;
            exit(0);
        }
    }

    return 0;
}