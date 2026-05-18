#pragma once

#include <iostream>
#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <conio.h>
#include <vector>
#include <thread>
#include <chrono>
#include "models.hpp"

using namespace std;

// Deklarasi awal untuk fungsi trim
inline string trim(const string& s);

// Menghapus n baris ke atas di terminal
inline void clearLines(int n) {
    for (int i = 0; i < n; i++) {
        cout << "\033[A"    // Naik 1 baris
             << "\033[2K"   // Bersihkan baris
             << "\r";       // Ke awal baris
    }
}

// Input string dengan validasi (tidak boleh kosong)
inline string inputString(const string& prompt) {
    string result;
    bool hasError = false;
    while (true) {
        if (hasError) {
            cout << "  \033[31m[!]\033[0m Input tidak boleh kosong." << endl;
        }
        cout << prompt;
        getline(cin, result);
        
        // Kita loop hanya mengecek apakah string kosong
        if (!result.empty()) {
            bool allSpaces = true;
            for (char c : result) {
                if (c != ' ' && c != '\t') { allSpaces = false; break; }
            }
            if (!allSpaces) return result;
        }
        
        int promptNewlines = 0;
        for (char c : prompt) if (c == '\n') promptNewlines++;
        int barisDihapus = 1 + promptNewlines + (hasError ? 1 : 0);
        clearLines(barisDihapus);
        hasError = true;
    }
}

// Input string dengan custom validator & auto clear line error
inline string inputStringWithValidation(const string& prompt, bool (*validator)(const string&), const string& customErrorMsg) {
    string currentError = "";
    while (true) {
        if (!currentError.empty()) {
            cout << currentError << endl;
        }
        cout << prompt;
        string inputStr;
        getline(cin, inputStr);
        inputStr = trim(inputStr);

        int promptNewlines = 0;
        for (char c : prompt) if (c == '\n') promptNewlines++;
        int barisDihapus = 1 + promptNewlines + (!currentError.empty() ? 1 : 0);
        
        if (inputStr.empty()) {
            clearLines(barisDihapus);
            currentError = "  \033[31m[!]\033[0m Input tidak boleh kosong.";
            continue;
        }
        
        if (validator && !validator(inputStr)) {
            clearLines(barisDihapus);
            currentError = "  \033[31m[!]\033[0m " + customErrorMsg;
            continue;
        }
        
        return inputStr;
    }
}

// Input integer dengan validasi
inline int inputInt(const string& prompt) {
    string errorMsg = "";
    while (true) {
        if (!errorMsg.empty()) {
            cout << errorMsg << endl;
        }
        cout << prompt;
        string inputStr;
        getline(cin, inputStr);
        
        string trimmedInput = trim(inputStr);
        try {
            size_t pos;
            int val = stoi(trimmedInput, &pos);
            if (pos == trimmedInput.length() && trimmedInput == to_string(val)) {
                return val;
            }
        } catch (...) {}
        
        int promptNewlines = 0;
        for (char c : prompt) if (c == '\n') promptNewlines++;
        int barisDihapus = 1 + promptNewlines + (!errorMsg.empty() ? 1 : 0);
        clearLines(barisDihapus);
        errorMsg = "  \033[31m[!]\033[0m Input tidak valid. Masukkan angka.";
    }
}

// Input menu dengan batasan min-max (dengan fitur pembersihan error)
inline int inputMenu(int minOpsi, int maxOpsi, const string& prompt = "\nPilih Menu: ") {
    string errorMsg = "";

    while (true) {
        if (!errorMsg.empty()) {
            cout << errorMsg << endl;
        }
        
        cout << prompt;
        
        string inputStr;
        getline(cin, inputStr);

        string trimmedInput = trim(inputStr);
        int opsi = 0;
        bool valid = false;
        
        try {
            size_t pos;
            opsi = stoi(trimmedInput, &pos);
            // Pastikan seluruh string adalah angka dan dalam jangkauan serta identik dengan string konversinya
            if (pos == trimmedInput.length() && opsi >= minOpsi && opsi <= maxOpsi && trimmedInput == to_string(opsi)) {
                valid = true;
            }
        } catch (...) {
            valid = false;
        }

        if (valid) {
            return opsi;
        } else {
            // Hitung jumlah baris yang harus dihapus
            int promptNewlines = 0;
            for (char c : prompt) {
                if (c == '\n') promptNewlines++;
            }
            
            // 1 baris untuk input user (Enter) + baris prompt + baris error (jika ada)
            int barisDihapus = 1 + promptNewlines + (!errorMsg.empty() ? 1 : 0);
            
            clearLines(barisDihapus);
            errorMsg = "  \033[31m[!]\033[0m Input tidak valid. Masukkan angka " + to_string(minOpsi) + " - " + to_string(maxOpsi) + ".";
        }
    }
}

// Input string opsional (boleh kosong, return defaultValue jika kosong)
inline string inputStringOptional(const string& prompt, const string& defaultValue) {
    string result;
    cout << prompt;
    getline(cin, result);
    return result.empty() ? defaultValue : result;
}

// Input password terselubung (menampilkan bintang *)
inline string inputPassword(const string& prompt) {
    string password = "";
    char ch;
    cout << prompt;
    while ((ch = _getch()) != 13) { // 13 adalah Enter
        if (ch == 8) { // 8 adalah Backspace
            if (password.length() > 0) {
                password.pop_back();
                cout << "\b \b";
            }
        } else if (ch == 3 || ch == 26) { 
            // 3: Ctrl+C, 26: Ctrl+Z
            exit(0);
        } else {
            password += ch;
            cout << "*";
        }
    }
    cout << endl;
    return password;
}

// Dapatkan tanggal hari ini (format: YYYY-MM-DD)
inline string getCurrentDate() {
    time_t now = time(nullptr);
    tm* ltm = localtime(&now);
    ostringstream oss;
    oss << (1900 + ltm->tm_year) << "-"
        << setw(2) << setfill('0') << (1 + ltm->tm_mon) << "-"
        << setw(2) << setfill('0') << ltm->tm_mday;
    return oss.str();
}

// Tekan Enter untuk lanjut
inline void tekanEnter() {
    cout << "\n  Tekan Enter untuk lanjut...";
    while (true) {
        char ch = _getch();
        if (ch == 13) { // 13 adalah kode ASCII untuk Enter
            break;
        }
    }
}

// Garis pemisah
inline void garisPemisah(int panjang = 70) {
    cout << string(panjang, '=') << endl;
}

// Garis tipis
inline void garisTipis(int panjang = 70) {
    cout << string(panjang, '-') << endl;
}

// Truncate string jika terlalu panjang
inline string truncate(const string& s, size_t maxLen) {
    if (s.length() <= maxLen) return s;
    return s.substr(0, maxLen - 2) + "..";
}

// Trim spasi di awal/akhir string
inline string trim(const string& s) {
    int start = 0;
    int end = (int)s.size();
    while (start < end && (s[start] == ' ' || s[start] == '\t' || s[start] == '\n' || s[start] == '\r')) start++;
    while (end > start && (s[end-1] == ' ' || s[end-1] == '\t' || s[end-1] == '\n' || s[end-1] == '\r')) end--;
    return s.substr(start, end - start);
}

// Format number with dots (Rupiah)
inline string formatRupiah(double value) {
    ostringstream oss;
    oss << fixed << setprecision(0) << value;
    string s = oss.str();
    for (int i = (int)s.length() - 3; i > 0; i -= 3) {
        s.insert(i, ".");
    }
    return "Rp " + s;
}

// Cek apakah folder platform ada di root project
inline bool platformFolderExists(const string& platform) {
    return std::filesystem::exists(platform) && std::filesystem::is_directory(platform);
}

// Cek apakah script Python ada
inline bool scriptFileExists(const string& scriptPath) {
    return std::filesystem::exists(scriptPath);
}

// ── Bubble Sort: Lowongan berdasarkan Gaji (Tertinggi ke Terendah)
inline void bubbleSortLowonganBySalary(Lowongan *lowongan, int &jumlah) {
    if (jumlah <= 1) return;
    int n = jumlah;
    for (int i = 0; i < n - 1; i++) {
        bool swapped = false;
        for (int j = 0; j < n - 1 - i; j++) {
            Lowongan *ptrA = lowongan + j;
            Lowongan *ptrB = lowongan + (j + 1);

            // Urutkan berdasarkan Gaji (Descending)
            if (ptrA->salaryExpectation < ptrB->salaryExpectation) {
                Lowongan temp = *ptrA;
                *ptrA = *ptrB;
                *ptrB = temp;
                swapped = true;
            }
        }
        if (!swapped) break;
    }
}

// Linear Search: Lowongan berdasarkan Judul (Exact Match)
inline Lowongan* linearSearchLowonganByTitle(Lowongan* lowongan, int jumlahLowongan, const string& keyword, int& langkahKeluar) {
    Lowongan* result = nullptr;
    int langkah = 0;

    for (int i = 0; i < jumlahLowongan; i++) {
        Lowongan* ptr = lowongan + i;
        langkah++;

        if (ptr->title == keyword) {
            if (i > 0) {
                Lowongan* ptrDepan = lowongan + 0;

                Lowongan temp = *ptr;
                *ptr = *ptrDepan;
                *ptrDepan = temp;
            }

            result = lowongan + 0;
            break;
        }
    }
    
    langkahKeluar = langkah;
    return result;
}

// Sort array (bubble) indeks berdasarkan ID Lowongan (Ascending)
inline void sortIndeksByIdAsc(const Lowongan* lowongan, int* indeks, int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (lowongan[indeks[j]].id > lowongan[indeks[j + 1]].id) {
                int temp = indeks[j];
                indeks[j] = indeks[j + 1];
                indeks[j + 1] = temp;
            }
        }
    }
}

// Binary Search: Lowongan berdasarkan ID
inline const Lowongan* binarySearchLowonganById(const Lowongan* lowongan, int jumlahLowongan, int targetId) {
    if (jumlahLowongan <= 0) return nullptr;

    int indeks[MAX_LOWONGAN];
    for (int i = 0; i < jumlahLowongan; i++) {
        indeks[i] = i;
    }
    
    sortIndeksByIdAsc(lowongan, indeks, jumlahLowongan);

    int low = 0;
    int high = jumlahLowongan - 1;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        int idMid = lowongan[indeks[mid]].id;

        if (idMid == targetId) {
            return &lowongan[indeks[mid]];
        } else if (idMid < targetId) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    return nullptr;
}

inline void tampilkanProgressBar(int lebar_total) {
    std::cout << "\033[?25l"; // Sembunyikan kursor
    
    string slate_grey   = "\033[38;2;71;85;105m";  
    string corporate_blue = "\033[38;2;37;99;235m";
    string slate_shadow = "\033[38;2;30;41;59m";  
    string sky_blue     = "\033[38;2;96;165;250m";
    string reset        = "\033[0m";

    for (int i = 0; i <= 100; ++i) {
        int pos = (lebar_total * i) / 100;

        std::cout << "\r " << slate_grey << "[" << reset; 
        
        for (int j = 0; j < lebar_total; ++j) {
            if (j <= pos) {
                std::cout << corporate_blue << "█" << reset; 
            } else {
                std::cout << slate_shadow << "░" << reset; 
            }
        }
        
        std::cout << slate_grey << "] " << "\033[1m" << sky_blue << i << "%" << reset << std::flush;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    
    std::cout << std::endl;
    std::cout << "\033[?25h"; // Tampilkan kursor kembali
}
