#include "services/auth_service.hpp"
#include "services/profile_service.hpp"
#include "core/json_storage.hpp"
#include "utils.hpp"
#include "validation.hpp"
#include <iostream>
#include <iomanip>
#include <string>
#include "services/job_service.hpp"

using namespace std;

void displayMenuAuth() {
    cout << ABU_REDUP << "\n\n=========================================================" << endl;
    cout << EMAS   <<    "|              OPENWORK - AI JOB HUNTER CLI             |" << endl;
    cout << ABU_REDUP << "=========================================================" << endl;
    cout << PUTIH  <<    "| No | Autentikasi                                      |" << endl;
    cout << ABU_TERANG <<"|----|--------------------------------------------------|" << endl;
    cout << PUTIH  <<    "| 1. | Login Akun                                       |" << endl;
    cout << PUTIH  <<    "| 2. | Register Akun Baru                               |" << endl;
    cout << PUTIH  <<    "| 3. | Keluar Program                                   |" << endl;
    cout << ABU_REDUP << "=========================================================" << RESET << endl;
}

int login(User *users, int *jumlahUser) {
    if (*jumlahUser == 0) {
        CLEAR;
        cout << ABU_REDUP << "============================================================" << endl;
        cout << EMAS      << "              OPENWORK - AI JOB HUNTER CLI             " << endl;
        cout << PUTIH     << "                          LOGIN" << endl;
        cout << ABU_REDUP << "============================================================" << RESET << endl;
        cout << MERAH << "[!] " << PUTIH << "Belum ada user terdaftar. Silakan register dulu." << RESET << endl;
        tekanEnter();
        return -2;
    }

    int idxLogin = -1;
    for (int percobaan = 1; percobaan <= 3; percobaan++) {
        string username, password;

        while (true) {
            CLEAR;
            cout << ABU_REDUP << "============================================" << endl;
            cout << EMAS      << "||            LOGIN SESSION  # " << percobaan << "          ||" << endl;
            cout << ABU_REDUP << "============================================" << RESET << endl;
            cout << PUTIH     << "\n  [>] Masukkan Username: " << BIRU;
            getline(cin, username);
            cout << RESET;
            if (username.empty()) {
                cout << MERAH << "[!] " << PUTIH << "Username tidak boleh kosong." << RESET << endl;
                tekanEnter();
                continue;
            }
            if (!isValidUsername(username)) {
                cout << MERAH << "[!] " << PUTIH << "Username tidak valid! Gunakan 3-20 karakter (huruf, _, .)." << RESET << endl;
                tekanEnter();
                continue;
            }
            break;
        }

        while (true) {
            CLEAR;
            cout << ABU_REDUP << "============================================" << endl;
            cout << EMAS      << "||            LOGIN SESSION  # " << percobaan << "          ||" << endl;
            cout << ABU_REDUP << "============================================" << RESET << endl;
            cout << PUTIH     << "\n  [>] Masukkan Username: " << BIRU << username << RESET << endl;
            password = inputPassword(PUTIH + "  [>] Masukkan Password: " + BIRU);
            cout << RESET;
            if (password.empty()) {
                cout << MERAH << "[!] " << PUTIH << "Password tidak boleh kosong." << RESET << endl;
                tekanEnter();
                continue;
            }

            if (password.length() < 3) {
                cout << MERAH << "[!] " << PUTIH << "Password minimal 8 karakter." << RESET << endl;
                tekanEnter();
                continue;
            }
            break;
        }

        for (int i = 0; i < *jumlahUser; i++) {
            User *ptrUser = users + i; 
            if (ptrUser->username == username && ptrUser->password == password) {
                idxLogin = i;
                break;
            }
        }

        if (idxLogin != -1) {
            User *ptrLogin = users + idxLogin; 
            if (ptrLogin->isBlocked) {
                cout << MERAH << "\n[!] Akses Ditolak: " << PUTIH << "Akun Anda telah diblokir oleh Admin." << RESET << endl;
                tekanEnter();
                return -2;
            }
            
            cout << endl;
            tampilkanProgressBar(50); // Animasi loading login

            cout << HIJAU << "\n[OK] " << PUTIH << "Login berhasil. Selamat datang, " << BIRU << ptrLogin->username << PUTIH << "." << RESET << endl;
            tekanEnter();
            return idxLogin;
        }

        cout << MERAH << "\n[!] " << PUTIH << "Username atau Password salah." << RESET << endl;
        if (percobaan < 3) {
            tekanEnter();
        }
    }

    cout << MERAH << "\n[!] BATAS PERCOBAAN HABIS " << PUTIH << "Program akan keluar." << RESET << endl;
    return -1;
}

bool registerUser(User *users, int *jumlahUser) {
    CLEAR;
    cout << ABU_REDUP << "============================================================" << endl;
    cout << EMAS      << "              OPENWORK - AI JOB HUNTER CLI             " << endl;
    cout << PUTIH     << "                        REGISTER" << endl;
    cout << ABU_REDUP << "============================================================" << RESET << endl;

    if (*jumlahUser >= MAX_USERS) {
        cout << MERAH << "[!] " << PUTIH << "Kapasitas user penuh. Tidak bisa mendaftar user baru." << RESET << endl;
        tekanEnter();
        return false;
    }

    // Pointer ke slot user baru menggunakan pointer arithmetic
    User *ptrUserBaru = users + (*jumlahUser);

    string username, password;

    while (true) {
        CLEAR;
        cout << ABU_REDUP << "============================================================" << endl;
        cout << EMAS      << "              OPENWORK - AI JOB HUNTER CLI             " << endl;
        cout << PUTIH     << "                        REGISTER" << endl;
        cout << ABU_REDUP << "============================================================" << RESET << endl;
        cout << PUTIH     << "Username: " << BIRU;
        getline(cin, username);
        cout << RESET;
        
        if (username.empty()) {
            cout << MERAH << "[!] " << PUTIH << "Username tidak boleh kosong." << RESET << endl;
            tekanEnter();
            continue;
        }
        
        if (!isValidUsername(username)) {
            cout << MERAH << "[!] " << PUTIH << "Username tidak valid! Gunakan 3-20 karakter (huruf, _, .)." << RESET << endl;
            tekanEnter();
            continue;
        }

        // Cek apakah username sudah ada
        bool usernameAda = false;
        for (int i = 0; i < *jumlahUser; i++) {
            User *ptrCek = users + i;
            if (ptrCek->username == username) {
                usernameAda = true;
                break;
            }
        }
        
        if (usernameAda) {
            cout << MERAH << "[!] " << PUTIH << "Username sudah digunakan. Silakan gunakan Username lain." << RESET << endl;
            tekanEnter();
        } else {
            break;
        }
    }

    while (true) {
        CLEAR;
        cout << ABU_REDUP << "============================================================" << endl;
        cout << EMAS      << "              OPENWORK - AI JOB HUNTER CLI             " << endl;
        cout << PUTIH     << "                        REGISTER" << endl;
        cout << ABU_REDUP << "============================================================" << RESET << endl;
        cout << PUTIH     << "Username: " << BIRU << username << RESET << endl;
        password = inputPassword(PUTIH + "Password: " + BIRU);
        cout << RESET;
        
        if (password.empty()) {
            cout << MERAH << "[!] " << PUTIH << "Password tidak boleh kosong." << RESET << endl;
            tekanEnter();
            continue;
        }
        
        if (!isValidPassword(password)) {
            cout << MERAH << "[!] " << PUTIH << "Password lemah! Minimal 8 karakter dan mengandung minimal 1 angka." << RESET << endl;
            tekanEnter();
            continue;
        }
        
        break;
    }

    // Mengisi data melalui pointer ke struct
    ptrUserBaru->username = username;
    ptrUserBaru->password = password;
    
    // Registrasi publik selalu menjadi role User
    ptrUserBaru->isAdmin = false;

    // dereference pointer lalu increment
    (*jumlahUser)++;

    // Simpan data setelah registrasi
    saveUsers(users, *jumlahUser);

    // Buat profil otomatis untuk user baru
    createProfileForUser(username);

    cout << HIJAU << "\n[OK] " << PUTIH << "Registrasi berhasil. Silakan login dengan akun baru." << RESET << endl;
    tekanEnter();
    return true;
}

void handleAuditUserMenu(User *users, int *jumlahUser) {
    while (true) {
        CLEAR;
        cout << ABU_REDUP << "\n+==========================================================+" << endl;
        cout << EMAS      << "|                  OPENWORK - AUDIT USER                   |" << endl;
        cout << ABU_REDUP << "+==========================================================+" << endl;
        cout << PUTIH     << "| Username             | Role       | Status               |" << endl;
        cout << ABU_TERANG << "|----------------------|------------|----------------------|" << endl;
        for (int i = 0; i < *jumlahUser; i++) {
            User *ptrUser = users + i;
            
            // Hanya tampilkan role User, skip jika Admin
            if (ptrUser->isAdmin) continue;
            
            string role = ptrUser->isAdmin ? "Admin" : "User";
            string status = ptrUser->isBlocked ? "Terblokir" : "Aktif";
            string coloredStatus = ptrUser->isBlocked ? (MERAH + status + RESET) : (HIJAU + status + RESET);
            
            int padding = 20 - (ptrUser->isBlocked ? 9 : 5);
            
            cout << ABU_TERANG << "| " << PUTIH << left << setw(20) << ptrUser->username
                 << ABU_TERANG << " | " << PUTIH << left << setw(10) << role
                 << ABU_TERANG << " | " << coloredStatus << string(padding, ' ') << ABU_TERANG << " |" << RESET << endl;
        }
        cout << ABU_REDUP << "+==========================================================+" << endl;
        cout << PUTIH     << "|  1 | Blokir User                                         |" << endl;
        cout << PUTIH     << "|  2 | Buka Blokir User                                    |" << endl;
        cout << PUTIH     << "|  3 | Lihat Bookmark User                                 |" << endl;
        cout << ABU_TERANG << "|----|-----------------------------------------------------|" << endl;
        cout << PUTIH     << "|  0 | Kembali ke Menu Admin                               |" << endl;
        cout << ABU_REDUP << "+==========================================================+" << RESET << endl;

        int menu = inputMenu(0, 3);

        if (menu == 0) return;

        if (menu == 1 || menu == 2 || menu == 3) {
            string targetUsername;
            cout << PUTIH << "\nMasukkan Username target: " << BIRU;
            getline(cin, targetUsername);
            cout << RESET;

            int targetIdx = -1;
            for (int i = 0; i < *jumlahUser; i++) {
                User *ptrUser = users + i;
                if (ptrUser->username == targetUsername) {
                    targetIdx = i;
                    break;
                }
            }

            if (targetIdx == -1) {
                cout << MERAH << "  [!] User tidak ditemukan." << RESET << endl;
                tekanEnter();
                continue;
            }

            User *ptrTarget = users + targetIdx;

            if (menu == 1) {
                if (ptrTarget->isAdmin) {
                    cout << MERAH << "  [!] Operasi Ditolak: Anda tidak dapat memblokir sesama Admin." << RESET << endl;
                } else if (ptrTarget->isBlocked) {
                    cout << ABU_TERANG << "  [INFO] " << PUTIH << "User " << targetUsername << " sudah dalam status terblokir." << RESET << endl;
                } else {
                    ptrTarget->isBlocked = true;
                    saveUsers(users, *jumlahUser);
                    cout << HIJAU << "  [OK] User " << targetUsername << " berhasil diblokir." << RESET << endl;
                }
            } else if (menu == 2) {
                if (!ptrTarget->isBlocked) {
                    cout << ABU_TERANG << "  [INFO] " << PUTIH << "User " << targetUsername << " tidak sedang terblokir." << RESET << endl;
                } else {
                    ptrTarget->isBlocked = false;
                    saveUsers(users, *jumlahUser);
                    cout << HIJAU << "  [OK] Blokir untuk User " << targetUsername << " berhasil dibuka." << RESET << endl;
                }
            } else if (menu == 3) {
                tampilkanBookmarkUserAdmin(targetUsername);
            }
            if (menu != 3) {
                tekanEnter();
            }
        }
    }
}

//  Tambah Akun Admin Baru (hanya bisa dipanggil dari menu Admin)

void tambahAdmin(User* users, int* jumlahUser) {
    if (*jumlahUser >= MAX_USERS) {
        CLEAR;
        cout << "\n+==========================================================+" << endl;
        cout << "|               TAMBAH AKUN ADMIN BARU                    |" << endl;
        cout << "+==========================================================+" << endl;
        cout << "\n  [!] Kapasitas user penuh. Tidak bisa menambah admin baru." << endl;
        tekanEnter();
        return;
    }

    User* ptrBaru = users + (*jumlahUser);
    string username, password;

    // ── Input username ──
    while (true) {
        CLEAR;
        cout << "\n+==========================================================+" << endl;
        cout << "|               TAMBAH AKUN ADMIN BARU                    |" << endl;
        cout << "+==========================================================+" << endl;
        cout << endl;

        username = inputString("  Username Admin baru : ");

        if (!isValidUsername(username)) {
            cout << "  [!] Username tidak valid! Gunakan 3-20 karakter (huruf, _, .)." << endl;
            tekanEnter();
            continue;
        }

        bool ada = false;
        for (int i = 0; i < *jumlahUser; i++) {
            if ((users + i)->username == username) { ada = true; break; }
        }
        if (ada) {
            cout << "  [!] Username sudah dipakai. Gunakan username lain." << endl;
            tekanEnter();
            continue;
        }
        break;
    }

    // Input password 
    while (true) {
        CLEAR;
        cout << "\n+==========================================================+" << endl;
        cout << "|               TAMBAH AKUN ADMIN BARU                    |" << endl;
        cout << "+==========================================================+" << endl;
        cout << endl;
        cout << "  Username : " << username << endl;

        password = inputPassword("  Password          : ");

        if (!isValidPassword(password)) {
            cout << "  [!] Password lemah! Minimal 8 karakter dan mengandung minimal 1 angka." << endl;
            tekanEnter();
            continue;
        }
        break;
    }

    ptrBaru->username  = username;
    ptrBaru->password  = password;
    ptrBaru->isAdmin   = true;    // hardcode Admin
    ptrBaru->isBlocked = false;

    (*jumlahUser)++;

    saveUsers(users, *jumlahUser);
    createProfileForUser(username);

    CLEAR;
    cout << "\n+==========================================================+" << endl;
    cout << "|               TAMBAH AKUN ADMIN BARU                    |" << endl;
    cout << "+==========================================================+" << endl;
    cout << "\n  [OK] Akun Admin '" << username << "' berhasil dibuat." << endl;
    tekanEnter();
}
