#include "services/job_service.hpp"
#include "bridge/python_bridge.hpp"
#include "api/api_server.hpp"
#include "models.hpp"
#include "core/json_storage.hpp"
#include "constants.hpp"
#include "utils.hpp"
#include "validation.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

using namespace std;

//  Helper: Input salary (non-negative)

static double inputSalaryLowongan(const string& prompt) {
    string errorMsg = "";
    while (true) {
        if (!errorMsg.empty()) cout << errorMsg << endl;
        string inputStr;
        cout << prompt;
        getline(cin, inputStr);
        inputStr = trim(inputStr);

        if (inputStr.empty()) {
            clearLines(1 + (!errorMsg.empty() ? 1 : 0));
            errorMsg = "  \033[31m[!]\033[0m Input tidak boleh kosong.";
            continue;
        }
        
        try {
            size_t pos;
            double value = stod(inputStr, &pos);
            if (pos != inputStr.length()) throw invalid_argument("trailing chars");
            if (value >= 0) {
                return value;
            } else {
                clearLines(1 + (!errorMsg.empty() ? 1 : 0));
                errorMsg = "  \033[31m[!]\033[0m Nilai tidak boleh negatif.";
                continue;
            }
        } catch (...) {
            clearLines(1 + (!errorMsg.empty() ? 1 : 0));
            errorMsg = "  \033[31m[!]\033[0m Input tidak valid. Masukkan angka.";
        }
    }
}

//  Helper: Input FieldPreference dari menu pilihan

FieldPreference inputFieldPreference() {
    int pilihan = inputMenu(1, 6, "\n  Pilih bidang:\n    [1] ALL\n    [2] IT\n    [3] ECONOMY\n    [4] HEALTHCARE\n    [5] EDUCATION\n    [6] CREATIVE\n\n  Pilih (1-6): ");
    switch (pilihan) {
        case 1: return ALL;
        case 2: return IT;
        case 3: return ECONOMY;
        case 4: return HEALTHCARE;
        case 5: return EDUCATION;
        case 6: return CREATIVE;
    }
    return ALL;
}


// [1] READ  Lihat Semua Lowongan

static const Lowongan* cariLowonganById(Lowongan* lowongan, int jumlahLowongan, int id);
static void tampilkanDetailLowongan(const Lowongan& l);
static void tampilkanTabelLowongan(Lowongan* lowongan, int jumlahLowongan);

void lihatSemuaLowongan(Lowongan* lowongan, int* jumlahLowongan) {
    while (true) {
        CLEAR;
        cout << ABU_REDUP << "\n=========================================================" << endl;
        cout << EMAS      << "|               DAFTAR DATA LOWONGAN KERJA              |" << endl;
        cout << ABU_REDUP << "=========================================================" << RESET << endl;

        if (*jumlahLowongan == 0) {
            cout << ABU_TERANG << "\n  [INFO] " << PUTIH << "Belum ada lowongan kerja terdaftar." << RESET << endl;
            tekanEnter();
            return;
        }

        tampilkanTabelLowongan(lowongan, *jumlahLowongan);

        // Sub-menu
        cout << endl;
        cout << ABU_TERANG << "  [" << BIRU << "1" << ABU_TERANG << "] " << PUTIH << "Lihat detail lowongan (by ID)" << RESET << endl;
        cout << ABU_TERANG << "  [" << BIRU << "0" << ABU_TERANG << "] " << PUTIH << "Kembali" << RESET << endl;
        cout << endl;

        int opsi = inputMenu(0, 1, "  Pilih: ");

        if (opsi == 0) {
            return;
        } else if (opsi == 1) {
            int targetId = inputInt("  Masukkan ID lowongan: ");

            const Lowongan* found = cariLowonganById(lowongan, *jumlahLowongan, targetId);
            if (!found) {
                cout << "  [!] Lowongan dengan ID " << targetId << " tidak ditemukan." << endl;
                tekanEnter();
            } else {
                CLEAR;
                cout << "\n=========================================================" << endl;
                cout << "|                  DETAIL LOWONGAN KERJA                |" << endl;
                cout << "=========================================================" << endl;
                tampilkanDetailLowongan(*found);
                tekanEnter();
            }
        } else {
            cout << "  [!] Opsi tidak valid." << endl;
            tekanEnter();
        }
    }
}

//  [2] CREATE Tambah Lowongan Baru
void tambahLowongan(Lowongan* lowongan, int* jumlahLowongan, const string& adminUsername) {
    
    if (*jumlahLowongan >= MAX_LOWONGAN) {
        cout << MERAH << "  [!] Kapasitas data lowongan penuh!" << RESET << endl;
        tekanEnter();
        return;
    }

    CLEAR;
    cout << ABU_REDUP << "\n=========================================================" << endl;
    cout << EMAS      << "|              TAMBAH LOWONGAN KERJA BARU               |" << endl;
    cout << ABU_REDUP << "=========================================================" << RESET << endl;
    cout << endl;

    Lowongan* ptrBaru = lowongan + (*jumlahLowongan);

    // Input data
    ptrBaru->title = inputStringWithValidation(PUTIH + "  Judul lowongan     : " + BIRU, isValidJobTitle, "Judul tidak valid.");
    cout << RESET;

    ptrBaru->company = inputStringWithValidation(PUTIH + "  Tempat Kerja    : " + BIRU, isValidCompany, "Tempat Kerja tidak valid.");
    cout << RESET;

    ptrBaru->desc = inputString(PUTIH + "  Deskripsi          : " + BIRU);
    cout << RESET;

    ptrBaru->skills = inputStringWithValidation(PUTIH + "  Skills dibutuhkan  : " + BIRU, isValidSkillsList, "Format skills tidak valid (Gunakan koma untuk memisahkan).");
    cout << RESET;

    ptrBaru->location = inputStringWithValidation(PUTIH + "  Lokasi             : " + BIRU, isValidLocation, "Lokasi tidak valid.");
    cout << RESET;

    ptrBaru->salaryExpectation = inputSalaryLowongan(PUTIH + "  Ekspektasi Gaji    : " + BIRU);
    cout << RESET;
    ptrBaru->field    = inputFieldPreference();
    ptrBaru->postedBy = adminUsername;

    // Auto-set ID
    ptrBaru->id = generateNewLowonganId(lowongan, *jumlahLowongan);

    // Preview / Konfirmasi
    CLEAR;
    cout << ABU_REDUP << "\n=========================================================" << endl;
    cout << EMAS      << "|                 PREVIEW DATA LOWONGAN                 |" << endl;
    cout << ABU_REDUP << "=========================================================" << RESET << endl;
    cout << PUTIH << "    ID           : " << BIRU << ptrBaru->id << endl;
    cout << PUTIH << "    Judul        : " << BIRU << ptrBaru->title << endl;
    cout << PUTIH << "    Tempat kerja : " << BIRU << ptrBaru->company << endl;
    cout << PUTIH << "    Deskripsi    : " << BIRU << ptrBaru->desc << endl;
    cout << PUTIH << "    Skills       : " << BIRU << ptrBaru->skills << endl;
    cout << PUTIH << "    Lokasi       : " << BIRU << ptrBaru->location << endl;
    cout << PUTIH << "    Gaji         : " << BIRU << fixed << setprecision(0) << ptrBaru->salaryExpectation << endl;
    cout << PUTIH << "    Bidang       : " << BIRU << fieldPreferenceToString(ptrBaru->field) << endl;
    cout << ABU_REDUP << "=========================================================" << RESET << endl;

    string konfirmasi = inputString(PUTIH + "\n  Simpan? (y/n): " + BIRU);
    cout << RESET;
    if (konfirmasi != "y" && konfirmasi != "Y") {
        cout << ABU_TERANG << "  [INFO] Dibatalkan." << RESET << endl;
        tekanEnter();
        return;
    }

    // Simpan
    (*jumlahLowongan)++;
    saveLowongan(lowongan, *jumlahLowongan);

    cout << endl;
    cout << HIJAU << "  [OK] Lowongan \"" << ptrBaru->title << "\" berhasil ditambahkan! (ID: " << ptrBaru->id << ")" << RESET << endl;
    tekanEnter();
}

//  [3] UPDATE Edit Lowongan

void editLowongan(Lowongan* lowongan, int* jumlahLowongan) {
    CLEAR;
    cout << ABU_REDUP << "\n=========================================================" << endl;
    cout << EMAS      << "|                  EDIT LOWONGAN KERJA                  |" << endl;
    cout << ABU_REDUP << "=========================================================" << RESET << endl;
    cout << endl;

    if (*jumlahLowongan == 0) {
        cout << endl;
        cout << ABU_TERANG << "  [INFO] " << PUTIH << "Belum ada lowongan untuk diedit." << RESET << endl;
        tekanEnter();
        return;
    }

    // Input ID
    int targetId = inputInt(PUTIH + "  Masukkan ID lowongan yang akan diedit: " + BIRU);
    cout << RESET;

    // Cari lowongan
    int index = -1;
    for (int i = 0; i < *jumlahLowongan; i++) {
        Lowongan* ptr = lowongan + i;
        if (ptr->id == targetId) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        cout << MERAH << "  [!] Lowongan dengan ID " << targetId << " tidak ditemukan." << RESET << endl;
        tekanEnter();
        return;
    }

    Lowongan* low = lowongan + index;

    // Tampilkan data lama
    CLEAR;
    cout << ABU_REDUP << "\n=========================================================" << endl;
    cout << EMAS      << "|                     DATA SAAT INI                     |" << endl;
    cout << ABU_REDUP << "=========================================================" << RESET << endl;
    cout << PUTIH << "    Judul        : " << BIRU << low->title << endl;
    cout << PUTIH << "    Tempat Kerja : " << BIRU << low->company << endl;
    cout << PUTIH << "    Deskripsi    : " << BIRU << low->desc << endl;
    cout << PUTIH << "    Skills       : " << BIRU << low->skills << endl;
    cout << PUTIH << "    Lokasi       : " << BIRU << low->location << endl;
    cout << PUTIH << "    Gaji         : " << BIRU << fixed << setprecision(0) << low->salaryExpectation << endl;
    cout << PUTIH << "    Bidang       : " << BIRU << fieldPreferenceToString(low->field) << endl;
    cout << ABU_REDUP << "=========================================================" << RESET << endl;

    // Pilih field
    cout << endl;
    cout << PUTIH << "  Pilih field yang akan diubah:" << RESET << endl;
    cout << ABU_TERANG << "    [" << BIRU << "1" << ABU_TERANG << "] " << PUTIH << "Judul" << RESET << endl;
    cout << ABU_TERANG << "    [" << BIRU << "2" << ABU_TERANG << "] " << PUTIH << "Tempat Kerja" << RESET << endl;
    cout << ABU_TERANG << "    [" << BIRU << "3" << ABU_TERANG << "] " << PUTIH << "Deskripsi" << RESET << endl;
    cout << ABU_TERANG << "    [" << BIRU << "4" << ABU_TERANG << "] " << PUTIH << "Skills" << RESET << endl;
    cout << ABU_TERANG << "    [" << BIRU << "5" << ABU_TERANG << "] " << PUTIH << "Lokasi" << RESET << endl;
    cout << ABU_TERANG << "    [" << BIRU << "6" << ABU_TERANG << "] " << PUTIH << "Ekspektasi Gaji" << RESET << endl;
    cout << ABU_TERANG << "    [" << BIRU << "7" << ABU_TERANG << "] " << PUTIH << "Bidang" << RESET << endl;
    cout << ABU_TERANG << "    [" << BIRU << "0" << ABU_TERANG << "] " << PUTIH << "Batal" << RESET << endl;
    cout << endl;

    int pilihan = inputMenu(0, 7, "  Pilih: ");

    switch (pilihan) {
        case 0:
            cout << ABU_TERANG << "  [INFO] Edit dibatalkan." << RESET << endl;
            tekanEnter();
            return;

        case 1:
            low->title = inputStringWithValidation(PUTIH + "  Judul baru: " + BIRU, isValidJobTitle, "Judul tidak valid.");
            cout << RESET;
            break;

        case 2:
            low->company = inputStringWithValidation(PUTIH + "  Tempat Kerja baru: " + BIRU, isValidCompany, "Tempat Kerja tidak valid.");
            cout << RESET;
            break;

        case 3:
            low->desc = inputString(PUTIH + "  Deskripsi baru: " + BIRU);
            cout << RESET;
            break;

        case 4:
            low->skills = inputStringWithValidation(PUTIH + "  Skills baru: " + BIRU, isValidSkillsList, "Format skills tidak valid.");
            cout << RESET;
            break;

        case 5:
            low->location = inputStringWithValidation(PUTIH + "  Lokasi baru: " + BIRU, isValidLocation, "Lokasi tidak valid.");
            cout << RESET;
            break;

        case 6:
            low->salaryExpectation = inputSalaryLowongan(PUTIH + "  Ekspektasi Gaji baru: " + BIRU);
            cout << RESET;
            break;

        case 7:
            low->field = inputFieldPreference();
            break;

        default:
            cout << MERAH << "  [!] Pilihan tidak valid." << RESET << endl;
            tekanEnter();
            return;
    }

    // Simpan
    saveLowongan(lowongan, *jumlahLowongan);

    CLEAR;
    cout << ABU_REDUP << "\n=========================================================" << endl;
    cout << EMAS      << "|              PREVIEW DATA SETELAH UPDATE              |" << endl;
    cout << ABU_REDUP << "=========================================================" << RESET << endl;
    cout << PUTIH << "    ID           : " << BIRU << low->id << endl;
    cout << PUTIH << "    Judul        : " << BIRU << low->title << endl;
    cout << PUTIH << "    Tempat Kerja : " << BIRU << low->company << endl;
    cout << PUTIH << "    Deskripsi    : " << BIRU << low->desc << endl;
    cout << PUTIH << "    Skills       : " << BIRU << low->skills << endl;
    cout << PUTIH << "    Lokasi       : " << BIRU << low->location << endl;
    cout << PUTIH << "    Gaji         : " << BIRU << fixed << setprecision(0) << low->salaryExpectation << endl;
    cout << PUTIH << "    Bidang       : " << BIRU << fieldPreferenceToString(low->field) << endl;
    cout << ABU_REDUP << "=========================================================" << RESET << endl;

    cout << HIJAU << "\n  [OK] Lowongan ID " << targetId << " berhasil diupdate!" << RESET << endl;
    tekanEnter();
}

//  [4] DELETE â€” Hapus Lowongan
void hapusLowongan(Lowongan* lowongan, int* jumlahLowongan) {
    CLEAR;
    cout << ABU_REDUP << "\n=========================================================" << endl;
    cout << EMAS      << "|                 HAPUS LOWONGAN KERJA                  |" << endl;
    cout << ABU_REDUP << "=========================================================" << RESET << endl;
    cout << endl;

    if (*jumlahLowongan == 0) {
        cout << endl;
        cout << ABU_TERANG << "  [INFO] " << PUTIH << "Belum ada lowongan untuk dihapus." << RESET << endl;
        tekanEnter();
        return;
    }

    // Input ID
    int targetId = inputInt(PUTIH + "  Masukkan ID lowongan yang akan dihapus: " + BIRU);
    cout << RESET;

    // Cari lowongan
    int index = -1;
    for (int i = 0; i < *jumlahLowongan; i++) {
        Lowongan* ptr = lowongan + i;
        if (ptr->id == targetId) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        cout << MERAH << "  [!] Lowongan dengan ID " << targetId << " tidak ditemukan." << RESET << endl;
        tekanEnter();
        return;
    }

    Lowongan* low = lowongan + index;

    // Tampilkan data yang akan dihapus
    CLEAR;
    cout << ABU_REDUP << "\n=========================================================" << endl;
    cout << EMAS      << "|                DATA YANG AKAN DIHAPUS                 |" << endl;
    cout << ABU_REDUP << "=========================================================" << RESET << endl;
    cout << PUTIH << "    ID           : " << BIRU << low->id << endl;
    cout << PUTIH << "    Judul        : " << BIRU << low->title << endl;
    cout << PUTIH << "    Tempat Kerja : " << BIRU << low->company << endl;
    cout << PUTIH << "    Deskripsi    : " << BIRU << low->desc << endl;
    cout << PUTIH << "    Skills       : " << BIRU << low->skills << endl;
    cout << PUTIH << "    Lokasi       : " << BIRU << low->location << endl;
    cout << PUTIH << "    Gaji         : " << BIRU << fixed << setprecision(0) << low->salaryExpectation << endl;
    cout << PUTIH << "    Bidang       : " << BIRU << fieldPreferenceToString(low->field) << endl;
    cout << ABU_REDUP << "=========================================================" << RESET << endl;

    // Konfirmasi
    string konfirmasi = inputString(PUTIH + "  Yakin ingin menghapus? (y/n): " + BIRU);
    cout << RESET;
    if (konfirmasi != "y" && konfirmasi != "Y") {
        cout << ABU_TERANG << "  [INFO] Penghapusan dibatalkan." << RESET << endl;
        tekanEnter();
        return;
    }

    // Hapus
    string namaHapus = low->title;

    // Pointer arithmetic shifting for deletion
    for (int i = index; i < *jumlahLowongan - 1; i++) {
        Lowongan* ptrCurrent = lowongan + i;
        Lowongan* ptrNext    = lowongan + (i + 1);
        *ptrCurrent = *ptrNext;
    }

    (*jumlahLowongan)--;
    saveLowongan(lowongan, *jumlahLowongan);

    cout << endl;
    cout << HIJAU << "  [OK] Lowongan \"" << namaHapus << "\" (ID: " << targetId << ") berhasil dihapus!" << RESET << endl;
    tekanEnter();
}

//  Menu Handler â€” Loop Sub-menu Lowongan

// Forward declaration (sortingLowongan defined after helpers)
void sortingLowongan(Lowongan* lowongan, int* jumlahLowongan);

void showLowonganMenuDisplay() {
    CLEAR;
    cout << ABU_REDUP << "\n+==========================================================+" << endl;
    cout << EMAS      << "|                          LOWONGAN KERJA                  |" << endl;
    cout << ABU_REDUP << "+==========================================================+" << endl;
    cout << PUTIH     << "| No | Fitur                                               |" << endl;
    cout << ABU_TERANG << "|----|-----------------------------------------------------|" << endl;
    cout << PUTIH     << "|  1 | Lihat semua lowongan                                |" << endl;
    cout << PUTIH     << "|  2 | Tambah lowongan baru                                |" << endl;
    cout << PUTIH     << "|  3 | Edit lowongan                                       |" << endl;
    cout << PUTIH     << "|  4 | Hapus lowongan                                      |" << endl;
    cout << ABU_TERANG << "|----|-----------------------------------------------------|" << endl;
    cout << PUTIH     << "|  0 | Kembali                                             |" << endl;
    cout << ABU_REDUP << "+==========================================================+" << RESET << endl;
}

void handleLowonganMenu(const string& adminUsername) {
    Lowongan dataLowongan[MAX_LOWONGAN];
    int jumlahLowongan = 0;
    loadLowongan(dataLowongan, &jumlahLowongan);

    while (true) {
        CLEAR;
        showLowonganMenuDisplay();

        int opsi = inputMenu(0, 4, "  Pilih opsi: ");

        switch (opsi) {
            case 1: lihatSemuaLowongan(dataLowongan, &jumlahLowongan); break;
            case 2: tambahLowongan(dataLowongan, &jumlahLowongan, adminUsername); break;
            case 3: editLowongan(dataLowongan, &jumlahLowongan);       break;
            case 4: hapusLowongan(dataLowongan, &jumlahLowongan);       break;
            case 0: return;
            default:
                cout << "  [!] Opsi tidak valid (0-4)." << endl;
                tekanEnter();
                break;
        }
    }
}

//  Helper: Tampilkan detail 1 lowongan admin (reusable)

static void tampilkanDetailLowongan(const Lowongan& l) {
    cout << ABU_TERANG;
    garisTipis(50);
    cout << RESET;
    cout << PUTIH << "    ID           : " << BIRU << l.id << endl;
    cout << PUTIH << "    Judul        : " << BIRU << l.title << endl;
    cout << PUTIH << "    Tempat Kerja : " << BIRU << l.company << endl;
    cout << PUTIH << "    Deskripsi    : " << BIRU << l.desc << endl;
    cout << PUTIH << "    Skills       : " << BIRU << l.skills << endl;
    cout << PUTIH << "    Lokasi       : " << BIRU << l.location << endl;
    cout << PUTIH << "    Gaji         : " << BIRU << fixed << setprecision(0) << l.salaryExpectation << endl;
    cout << PUTIH << "    Bidang       : " << BIRU << fieldPreferenceToString(l.field) << endl;
    cout << PUTIH << "    Diposting Oleh: " << EMAS << (l.postedBy.empty() ? "admin" : l.postedBy) << endl;
    cout << ABU_TERANG;
    garisTipis(50);
    cout << RESET;
}

//  Helper: Tampilkan detail 1 bookmark (reusable)

static void tampilkanDetailBookmark(const BookmarkedJob& b) {
    cout << "  [INFO] Detail Bookmark (BID: " << b.bookmarkId << ")" << endl;
    garisTipis(60);
    cout << "    Judul         : " << b.title << endl;
    cout << "    Tempat Kerja  : " << b.company << endl;
    cout << "    Lokasi        : " << b.location << endl;
    cout << "    Gaji          : " << b.salary << endl;
    cout << "    Sumber        : " << b.source << " (ID: " << b.sourceId << ")" << endl;
    cout << "    Ditandai      : " << b.tanggalDitandai << endl;
    
    if (!b.url.empty() && b.url != "N/A") {
        cout << "    URL         : " << b.url << endl;
    }
    if (!b.postedDate.empty() && b.postedDate != "N/A") {
        cout << "    Diposting   : " << b.postedDate << endl;
    }
    if (!b.field.empty() && b.field != "N/A") {
        cout << "    Bidang      : " << b.field << endl;
    }

    cout << "\n  [DESKRIPSI]" << endl;
    cout << "    " << b.desc << endl;

    if (!b.skills.empty() && b.skills != "N/A") {
        cout << "\n  [SKILL YANG DIBUTUHKAN]" << endl;
        cout << "    " << b.skills << endl;
    }
    garisTipis(60);
}

//  Helper: Tampilkan tabel ringkasan lowongan admin

static void tampilkanTabelLowongan(Lowongan* lowongan, int jumlahLowongan) {
    cout << left;
    const int WIDE = 106;

    cout << ABU_REDUP << string(WIDE, '=') << RESET << endl;
    cout << ABU_TERANG << "| " << PUTIH << setw(5)  << "ID"
         << ABU_TERANG << "| " << PUTIH << setw(25) << "Judul"
         << ABU_TERANG << "| " << PUTIH << setw(20) << "Tempat Kerja"
         << ABU_TERANG << "| " << PUTIH << setw(12) << "Bidang"
         << ABU_TERANG << "| " << PUTIH << setw(15) << "Lokasi"
         << ABU_TERANG << "| " << PUTIH << setw(16) << "Gaji"
         << ABU_TERANG << "|" << RESET << endl;
    cout << ABU_TERANG << string(WIDE, '-') << RESET << endl;

    for (int i = 0; i < jumlahLowongan; i++) {
        Lowongan* ptr = lowongan + i;
        ostringstream salaryStr;
        salaryStr << fixed << setprecision(0) << ptr->salaryExpectation;

        cout << ABU_TERANG << "| " << BIRU << setw(5)  << to_string(ptr->id)
             << ABU_TERANG << "| " << PUTIH << setw(25) << truncate(ptr->title, 23)
             << ABU_TERANG << "| " << PUTIH << setw(20) << truncate(ptr->company, 18)
             << ABU_TERANG << "| " << PUTIH << setw(12) << fieldPreferenceToString(ptr->field)
             << ABU_TERANG << "| " << PUTIH << setw(15) << truncate(ptr->location, 13)
             << ABU_TERANG << "| " << BIRU << setw(16) << salaryStr.str()
             << ABU_TERANG << "|" << RESET << endl;
    }

    cout << ABU_REDUP << string(WIDE, '=') << RESET << endl;
    cout << PUTIH << "  Total: " << BIRU << jumlahLowongan << PUTIH << " lowongan" << RESET << endl;
}

//  Helper: Tampilkan tabel bookmark gabungan

static void tampilkanTabelBookmark(BookmarkedJob* data, int jumlahBookmarks) {
    cout << PUTIH << "\n  Total: " << BIRU << jumlahBookmarks << PUTIH << " Lowongan Tersimpan\n" << RESET << endl;
    cout << left;
    const string SEP = "+======+===========+=================================+=====================+==============+==============+";

    cout << ABU_TERANG << SEP << RESET << endl;
    cout << ABU_TERANG << "| " << PUTIH << setw(5)  << "BID"
         << ABU_TERANG << "| " << PUTIH << setw(10) << "SUMBER"
         << ABU_TERANG << "| " << PUTIH << setw(32) << "JUDUL LOWONGAN"
         << ABU_TERANG << "| " << PUTIH << setw(20) << "TEMPAT KERJA"
         << ABU_TERANG << "| " << PUTIH << setw(13) << "LOKASI"
         << ABU_TERANG << "| " << PUTIH << setw(13) << "ESTIMASI GAJI"
         << ABU_TERANG << "|" << RESET << endl;
    cout << ABU_TERANG << SEP << RESET << endl;

    for (int i = 0; i < jumlahBookmarks; i++) {
        BookmarkedJob* b = data + i;
        cout << ABU_TERANG << "| " << BIRU << setw(5)  << to_string(b->bookmarkId)
             << ABU_TERANG << "| " << PUTIH << setw(10) << truncate(b->source, 10)
             << ABU_TERANG << "| " << PUTIH << setw(32) << truncate(b->title, 31)
             << ABU_TERANG << "| " << PUTIH << setw(20) << truncate(b->company, 19)
             << ABU_TERANG << "| " << PUTIH << setw(13) << truncate(b->location, 12)
             << ABU_TERANG << "| " << BIRU << setw(13) << truncate(b->salary, 12)
             << ABU_TERANG << "|" << RESET << endl;
    }

    cout << ABU_TERANG << SEP << RESET << endl;
}

//  Helper: Cari lowongan by ID dalam vector

static const Lowongan* cariLowonganById(Lowongan* lowongan, int jumlahLowongan, int id) {
    for (int i = 0; i < jumlahLowongan; i++) {
        Lowongan* ptr = lowongan + i;
        if (ptr->id == id) return ptr;
    }
    return nullptr;
}

//  Helper: Cek duplikat bookmark
static bool sudahDiBookmark(BookmarkedJob* bookmarks, int jumlahBookmarks, const string& source, const string& sourceId, const string& username) {
    for (int i = 0; i < jumlahBookmarks; i++) {
        BookmarkedJob* b = bookmarks + i;
        if (b->source == source && b->sourceId == sourceId && b->username == username) return true;
    }
    return false;
}

//  Menu: Lihat Lowongan dari Admin + Cari & Urutkan
static void tampilkanLowonganAdmin(User* currentUser) {
    Lowongan lowongan[MAX_LOWONGAN];
    int jumlahLowongan = 0;
    loadLowongan(lowongan, &jumlahLowongan);

    while (true) {
        CLEAR;
        cout << ABU_REDUP << "\n=========================================================" << endl;
        cout << EMAS      << "|               DAFTAR DATA LOWONGAN KERJA              |" << endl;
        cout << ABU_REDUP << "=========================================================" << RESET << endl;

        if (jumlahLowongan == 0) {
            cout << ABU_TERANG << "\n  [INFO] " << PUTIH << "Belum ada lowongan kerja dari admin." << RESET << endl;
            tekanEnter();
            return;
        }

        tampilkanTabelLowongan(lowongan, jumlahLowongan);

        // Sub-menu
        cout << endl;
        cout << ABU_TERANG << "  [" << BIRU << "1" << ABU_TERANG << "] " << PUTIH << "Binary Search - Lihat Detail (by ID)" << RESET << endl;
        cout << ABU_TERANG << "  [" << BIRU << "2" << ABU_TERANG << "] " << PUTIH << "Linear Search - Cari Berdasarkan Nama" << RESET << endl;
        cout << ABU_TERANG << "  [" << BIRU << "3" << ABU_TERANG << "] " << PUTIH << "Bubble Sort - Urutkan Gaji (Tertinggi)" << RESET << endl;
        cout << ABU_TERANG << "  [" << BIRU << "4" << ABU_TERANG << "] " << PUTIH << "Tandai Lowongan (Bookmark)" << RESET << endl;
        cout << ABU_TERANG << "  [" << BIRU << "0" << ABU_TERANG << "] " << PUTIH << "Kembali" << RESET << endl;
        cout << endl;

        int opsi = inputMenu(0, 4, "  Pilih: ");

        if (opsi == 0) return;

        if (opsi == 1) {
            int targetId = inputInt("  Masukkan ID lowongan: ");

            const Lowongan* found = binarySearchLowonganById(lowongan, jumlahLowongan, targetId);
            if (!found) {
                cout << "  [!] Lowongan dengan ID " << targetId << " tidak ditemukan." << endl;
                tekanEnter();
            } else {
                CLEAR;
                cout << "\n=========================================================" << endl;
                cout << "|                  DETAIL LOWONGAN KERJA                |" << endl;
                cout << "=========================================================" << endl;
                tampilkanDetailLowongan(*found);
                tekanEnter();
            }

        } else if (opsi == 2) {
            if (jumlahLowongan == 0) {
                cout << "  [!] Belum ada data lowongan." << endl;
                tekanEnter();
                continue;
            }

            string keyword = inputString("  Masukkan judul lowongan yang dicari: ");

            int langkah = 0;
            Lowongan* result = linearSearchLowonganByTitle(lowongan, jumlahLowongan, keyword, langkah);

            if (result == nullptr) {
                cout << "  [!] Lowongan dengan judul '" << keyword << "' tidak ditemukan." << endl;
                cout << "  Total langkah pencarian: " << langkah << endl;
            } else {
                CLEAR;
                cout << "\n=========================================================" << endl;
                cout << "|               HASIL PENCARIAN LOWONGAN                |" << endl;
                cout << "=========================================================" << endl;
                tampilkanDetailLowongan(*result);
                cout << "\n   Total langkah pencarian: " << langkah << endl;
            }

            tekanEnter();

        } else if (opsi == 3) {
            sortingLowongan(lowongan, &jumlahLowongan);

        } else if (opsi == 4) {
            int targetId = inputInt("  Masukkan ID lowongan yang ingin ditandai: ");

            const Lowongan* found = cariLowonganById(lowongan, jumlahLowongan, targetId);
            if (!found) {
                cout << "  [!] Lowongan dengan ID " << targetId << " tidak ditemukan." << endl;
                tekanEnter();
                continue;
            }

            // Cek duplikat
            BookmarkedJob bookmarks[MAX_BOOKMARKS];
            int jumlahBookmarks = 0;
            loadBookmarkedJobs(bookmarks, &jumlahBookmarks);

            string srcId = to_string(found->id);

            if (sudahDiBookmark(bookmarks, jumlahBookmarks, "admin", srcId, currentUser->username)) {
                cout << "  [INFO] Lowongan \"" << found->title << "\" sudah ditandai sebelumnya." << endl;
                tekanEnter();
            } else {
                if (jumlahBookmarks >= MAX_BOOKMARKS) {
                    cout << "  [!] Kapasitas bookmark penuh." << endl;
                    tekanEnter();
                    continue;
                }
                BookmarkedJob bm;
                bm.bookmarkId = generateNewBookmarkId(bookmarks, jumlahBookmarks);
                bm.username = currentUser->username;
                bm.source = "admin";
                bm.sourceId = srcId;
                bm.title = found->title;
                bm.company = found->company;
                bm.location = found->location;
                ostringstream salStr;
                salStr << fixed << setprecision(0) << found->salaryExpectation;
                bm.salary = salStr.str();
                bm.desc = found->desc;
                bm.skills = found->skills;
                bm.field = fieldPreferenceToString(found->field);
                bm.url = "";
                bm.postedDate = "";
                bm.tanggalDitandai = getCurrentDate();

                BookmarkedJob* ptrBaru = bookmarks + jumlahBookmarks;
                *ptrBaru = bm;
                jumlahBookmarks++;

                saveBookmarkedJobs(bookmarks, jumlahBookmarks);
                cout << "  [OK] Lowongan \"" << found->title << "\" berhasil ditandai! ("
                     << jumlahBookmarks << " total bookmark)" << endl;
                tekanEnter();
            }
        } else if (opsi != 0) {
            cout << "  [!] Opsi tidak valid." << endl;
            tekanEnter();
        }
    }
}


//  Menu: Lihat Hasil Scraping & Bookmark [GUI React]
static void tampilkanHasilScrapingBookmark(User* currentUser) {
    CLEAR;
    cout << "\n+==========================================================+" << endl;
    cout << "|         LIHAT HASIL SCRAPING (MODE GUI - BROWSER)        |" << endl;
    cout << "+==========================================================+" << endl;

    // Cek apakah ada data scraping
    if (!fs::exists("data/scrape/hasil_jobstreet.json") &&
        !fs::exists("data/scrape/hasil_kitalulus.json") &&
        !fs::exists("data/scrape/hasil_lokerid.json")) {
        cout << "\n  [INFO] Belum ada data scraping." << endl;
        cout << "         Jalankan scraping dulu lewat menu [4]." << endl;
        tekanEnter();
        return;
    }

    // Start API server jika belum berjalan
    if (!isApiServerRunning()) {
        cout << "\n  [i] Memulai API server (port 8080)..." << endl;
        startApiServerAsync(currentUser->username);
        #ifdef _WIN32
        Sleep(800);
        #else
        usleep(800000);
        #endif
        cout << "  [OK] API server aktif di http://localhost:8080" << endl;
    } else {
        cout << "\n  [OK] API server sudah aktif." << endl;
    }

    // Buka browser ke React frontend
    cout << "\n  [i] Membuka browser ke http://localhost:5173 ..." << endl;
    #ifdef _WIN32
    system("start http://localhost:5173");
    #elif __APPLE__
    system("open http://localhost:5173");
    #else
    system("xdg-open http://localhost:5173");
    #endif

    cout << "\n+==========================================================+" << endl;
    cout << "|  [INFO] Tabel hasil scraping dibuka di browser.           |" << endl;
    cout << "|  Pastikan React dev server sudah berjalan:                |" << endl;
    cout << "|    cd frontend && bun run dev                             |" << endl;
    cout << "+==========================================================+" << endl;
    tekanEnter();
}


//  Menu: Lihat & Kelola Bookmark
void tampilkanBookmark(User* currentUser) {
    while (true) {
        BookmarkedJob allBookmarks[MAX_BOOKMARKS];
        int jumlahAllBookmarks = 0;
        loadBookmarkedJobs(allBookmarks, &jumlahAllBookmarks);
        
        BookmarkedJob bookmarks[MAX_BOOKMARKS];
        int jumlahBookmarks = 0;
        for (int i = 0; i < jumlahAllBookmarks; i++) {
            if (allBookmarks[i].username == currentUser->username) {
                bookmarks[jumlahBookmarks] = allBookmarks[i];
                jumlahBookmarks++;
            }
        }

        CLEAR;
        cout << ABU_REDUP << "\n+========================================================================================================+" << endl;
        cout << EMAS      << "|                                    LOWONGAN YANG DITANDAI (BOOKMARK)                                   |" << endl;
        cout << ABU_REDUP << "+========================================================================================================+" << RESET << endl;

        if (jumlahBookmarks == 0) {
            cout << ABU_TERANG << "\n  [INFO] " << PUTIH << "Belum ada lowongan yang ditandai." << RESET << endl;
            cout << ABU_TERANG << "         " << PUTIH << "Tandai lewat menu Cari Kerja." << RESET << endl;
            tekanEnter();
            return;
        }

        tampilkanTabelBookmark(bookmarks, jumlahBookmarks);

        // Sub-menu gaya Admin
        const string SUB_SEP = "+======+=================================================================================================+";
        cout << "\n" << ABU_REDUP << SUB_SEP << endl;
        cout << "|  NO  | " << EMAS << "PILIHAN MENU KELOLA BOOKMARK                                                                    " << ABU_REDUP << "|" << endl;
        cout << "+======+=================================================================================================+" << endl;
        cout << PUTIH     << "|  1   | Lihat Detail Lowongan (Masukkan BID)                                                            |" << endl;
        cout << PUTIH     << "|  2   | Hapus Lowongan dari Bookmark                                                                    |" << endl;
        cout << PUTIH     << "|  3   | Kosongkan Semua Daftar Bookmark                                                                 |" << endl;
        cout << PUTIH     << "|  4   | Tanya AI: Analisis Kecocokan dengan Profil                                                      |" << endl;
        cout << ABU_REDUP << "+======+=================================================================================================+" << endl;
        cout << PUTIH     << "|  0   | Kembali ke Menu Utama                                                                           |" << endl;
        cout << ABU_REDUP << "+======+=================================================================================================+" << RESET << endl;

        int opsi = inputMenu(0, 4, "\n  Pilih (0-4): ");

        if (opsi == 0) return;

        if (opsi == 1) {
            int targetId = inputInt("  Masukkan Bookmark ID (BID): ");

            bool found = false;
            for (int i = 0; i < jumlahBookmarks; i++) {
                BookmarkedJob* b = bookmarks + i;
                if (b->bookmarkId == targetId) {
                    CLEAR;
                    cout << "\n+==========================================================+" << endl;
                    cout << "|                  DETAIL BOOKMARK LOWONGAN                |" << endl;
                    cout << "+==========================================================+" << endl;
                    tampilkanDetailBookmark(*b);
                    tekanEnter();
                    found = true;
                    break;
                }
            }
            if (!found) {
                cout << "  [!] Bookmark ID " << targetId << " tidak ditemukan." << endl;
                tekanEnter();
            }

        } else if (opsi == 2) {
            int targetId = inputInt("  Masukkan Bookmark ID (BID) yang ingin dihapus: ");

            bool ditemukan = false;
            for (int i = 0; i < jumlahAllBookmarks; i++) {
                if (allBookmarks[i].bookmarkId == targetId && allBookmarks[i].username == currentUser->username) {
                    string nama = allBookmarks[i].title;
                    
                    for (int j = i; j < jumlahAllBookmarks - 1; j++) {
                        allBookmarks[j] = allBookmarks[j + 1];
                    }
                    jumlahAllBookmarks--;

                    saveBookmarkedJobs(allBookmarks, jumlahAllBookmarks);
                    cout << "  [OK] Bookmark \"" << nama << "\" berhasil dihapus." << endl;
                    tekanEnter();
                    ditemukan = true;
                    break;
                }
            }
            if (!ditemukan) {
                cout << "  [!] Bookmark ID " << targetId << " tidak ditemukan atau bukan milik Anda." << endl;
                tekanEnter();
            }

        } else if (opsi == 3) {
            string konfirmasi = inputString("  Yakin hapus semua bookmark Anda? (y/n): ");
            if (konfirmasi == "y" || konfirmasi == "Y") {
                BookmarkedJob newBookmarks[MAX_BOOKMARKS];
                int jumlahNewBookmarks = 0;
                for (int i = 0; i < jumlahAllBookmarks; i++) {
                    if (allBookmarks[i].username != currentUser->username) {
                        newBookmarks[jumlahNewBookmarks] = allBookmarks[i];
                        jumlahNewBookmarks++;
                    }
                }
                saveBookmarkedJobs(newBookmarks, jumlahNewBookmarks);
                cout << "  [OK] Semua bookmark Anda berhasil dihapus." << endl;
                tekanEnter();
                return;
            } else {
                cout << "  [INFO] Dibatalkan." << endl;
                tekanEnter();
            }

        } else if (opsi == 4) {
            analisisKecocokanProfil(currentUser);

        } else {
            cout << "  [!] Opsi tidak valid." << endl;
            tekanEnter();
        }
    }
}

//  Menu: Analisis Kecocokan Profil (AI Agent)

void analisisKecocokanProfil(User* currentUser) {
    CLEAR;
    cout << "\n+==========================================================+" << endl;
    cout << "|           ANALISIS KECOCOKAN PROFIL (AI AGENT)           |" << endl;
    cout << "+==========================================================+" << endl;

    // 1. Cek bookmark
    BookmarkedJob allBookmarks[MAX_BOOKMARKS];
    int jumlahAllBookmarks = 0;
    loadBookmarkedJobs(allBookmarks, &jumlahAllBookmarks);
    
    BookmarkedJob bookmarks[MAX_BOOKMARKS];
    int jumlahBookmarks = 0;
    for (int i = 0; i < jumlahAllBookmarks; i++) {
        if (allBookmarks[i].username == currentUser->username) {
            bookmarks[jumlahBookmarks] = allBookmarks[i];
            jumlahBookmarks++;
        }
    }
    if (jumlahBookmarks == 0) {
        cout << "\n  [INFO] Belum ada lowongan yang ditandai." << endl;
        cout << "         Tandai lowongan dulu lewat menu Cari Kerja." << endl;
        tekanEnter();
        return;
    }

    // 2. Load profil user dan cari profil milik currentUser
    Profile dataProfiles[MAX_PROFILES];
    int jumlahProfile = 0;
    loadProfiles(dataProfiles, &jumlahProfile);

    int selectedIdx = -1;
    for (int i = 0; i < jumlahProfile; i++) {
        if (dataProfiles[i].username == currentUser->username) {
            selectedIdx = i;
            break;
        }
    }

    if (selectedIdx == -1) {
        cout << "\n  [INFO] Profil Anda belum dibuat." << endl;
        cout << "         Harap lengkapi data diri Anda di menu \"Manajemen Profil\" terlebih dahulu." << endl;
        tekanEnter();
        return;
    }

    const Profile& profile = dataProfiles[selectedIdx];

    // 4. Konfirmasi & Ringkasan
    CLEAR;
    cout << "\n+==========================================================+" << endl;
    cout << "|                RINGKASAN DATA ANALISIS                   |" << endl;
    cout << "+==========================================================+" << endl;
    cout << "  [PROFIL]" << endl;
    cout << "    Nama    : " << profile.name << endl;
    cout << "    Lokasi  : " << profile.location << endl;
    cout << "    Gaji    : " << fixed << setprecision(0) << profile.salaryExpectation << endl;
    cout << "    Skills  : ";
    for (int i = 0; i < profile.jumlahSkill; i++) {
        if (i > 0) cout << ", ";
        cout << profile.skills[i].name << " (" << profile.skills[i].level << ")";
    }
    cout << "\n" << endl;
    cout << "  [LOWONGAN]" << endl;
    cout << "    Total   : " << jumlahBookmarks << " bookmark" << endl;
    for (int i = 0; i < jumlahBookmarks; i++) {
        BookmarkedJob* b = bookmarks + i;
        cout << "    - [" << b->bookmarkId << "] " << b->title << " (" << b->company << ")" << endl;
    }
    cout << "+==========================================================+" << endl;

    string konfirmasi = inputString("\n  Lanjut analisis kecocokan? (y/n): ");
    if (konfirmasi != "y" && konfirmasi != "Y") {
        cout << "  [INFO] Analisis dibatalkan." << endl;
        tekanEnter();
        return;
    }

    // 5. Eksekusi AI
    fs::path absBookmark = fs::absolute("data/temp_bookmarks.json");
    fs::path absProfil = fs::absolute("data/temp_profile.json");
    
    try {
        // Simpan bookmark terpilih ke file sementara
        ofstream fTempBM("data/temp_bookmarks.json");
        json jBM = json::array();
        for (int i = 0; i < jumlahBookmarks; i++) {
            jBM.push_back(bookmarks[i]);
        }
        fTempBM << jBM.dump(4);
        fTempBM.close();

        // Simpan PROFIL terpilih ke file sementara (AI butuh 1 object, bukan array)
        ofstream fTempProf("data/temp_profile.json");
        json jProf = profile;
        fTempProf << jProf.dump(4);
        fTempProf.close();
    } catch (...) {}

    string args = "match \"" + absBookmark.string() + "\" \"" + absProfil.string() + "\"";
    string cmd = buildServicesCommand(args);

    CLEAR;
    cout << "\n+==========================================================+" << endl;
    cout << "|         MENJALANKAN AI AGENT - ANALISIS KECOCOKAN        |" << endl;
    cout << "+==========================================================+" << endl;
    cout << "  Mode     : Kecocokan Profil vs Lowongan" << endl;
    cout << "  Bookmark : " << absBookmark.filename().string() << endl;
    cout << "  Profil   : " << absProfil.filename().string() << endl;
    garisTipis(60);
    cout << endl;

    int result = system(cmd.c_str());

    // Bersihkan file sementara setelah AI selesai
    try {
        if (fs::exists("data/temp_bookmarks.json")) fs::remove("data/temp_bookmarks.json");
        if (fs::exists("data/temp_profile.json"))   fs::remove("data/temp_profile.json");
    } catch (...) {}

    if (result != 0) {
        cout << endl;
        garisTipis(60);
        cout << "  [ERROR] Analisis gagal (exit code: " << result << ")" << endl;
        tekanEnter();
    }
}

//  Helper Scraper: Jalankan scraping Jobstreet langsung

static void jalankanScrapingJobstreet(const string& username) {
    CLEAR;
    cout << "\n+==========================================================+" << endl;
    cout << "|                JALANKAN SCRAPING JOBSTREET               |" << endl;
    cout << "+==========================================================+" << endl;

    // Cek folder & script
    if (!platformFolderExists("scripts")) {
        cout << "\n  [ERROR] Folder \"scripts/\" tidak ditemukan!" << endl;
        tekanEnter();
        return;
    }
    if (!scriptFileExists("scripts/main.py")) {
        cout << "\n  [ERROR] Script \"scripts/main.py\" tidak ditemukan!" << endl;
        tekanEnter();
        return;
    }

    // Kirim username ke Python agar bisa auto-match setelah scraping
    string cmd = buildServicesCommand("scrape " + username);

    cout << "\n  [i] Konfigurasi Scraper:" << endl;
    cout << "  - Platform : Multi-source (Jobstreet, KitaLulus, LokerID)" << endl;
    cout << "  - User     : " << username << " (auto-match aktif)" << endl;
    cout << "  - Script   : scripts/main.py scrape " << username << endl;
    cout << "  ----------------------------------------------------------" << endl;
    cout << "  Proses scraping + auto-match sedang berjalan...\n" << endl;

    int result = system(cmd.c_str());

    cout << "\n  ----------------------------------------------------------" << endl;
    if (result == 0) {
        cout << "  [OK] Scraping + auto-match selesai!" << endl;
    } else {
        cout << "  [ERROR] Proses gagal (exit code: " << result << ")" << endl;
    }
    tekanEnter();
}


//  Helper Scraper: Analisis hasil scraping Jobstreet (AI)

static void analisisHasilGabungan() {
    string hasilFile = "data/scrape/merged_data.json";

    CLEAR;
    cout << "\n+==========================================================+" << endl;
    cout << "|              ANALISIS HASIL SCRAPING (AI)                |" << endl;
    cout << "+==========================================================+" << endl;

    if (!fs::exists(hasilFile)) {
        cout << "\n  [INFO] File \"" << hasilFile << "\" belum ada." << endl;
        cout << "         Jalankan scraping dulu lewat menu [4]." << endl;
        tekanEnter();
        return;
    }

    jalankanAgentAnalisis(hasilFile);
}

//  Menu: Cari Kerja (User Flow)
void menuCariKerja(User* currentUser) {
    while (true) {
        CLEAR;
        cout << ABU_REDUP << "\n+==========================================================+" << endl;
        cout << EMAS      << "|                   OPENWORK - CARI KERJA                  |" << endl;
        cout << ABU_REDUP << "+==========================================================+" << endl;
        cout << PUTIH     << "| No | Fitur Lowongan                                      |" << endl;
        cout << ABU_TERANG << "|----|-----------------------------------------------------|" << endl;
        cout << PUTIH     << "|  1 | Lihat Lowongan dari Admin                           |" << endl;
        cout << PUTIH     << "|  2 | Lihat Hasil Scraping & Bookmark [GUI React]         |" << endl;
        cout << PUTIH     << "|  3 | Lihat Lowongan yang Ditandai                        |" << endl;
        cout << ABU_TERANG << "|----|-----------------------------------------------------|" << endl;
        cout << PUTIH     << "| No | Fitur Scraper                                       |" << endl;
        cout << ABU_TERANG << "|----|-----------------------------------------------------|" << endl;
        cout << PUTIH     << "|  4 | Jalankan Scraping Baru                              |" << endl;
        cout << PUTIH     << "|  5 | Analisis Hasil Scraping (AI)                        |" << endl;
        cout << ABU_TERANG << "|----|-----------------------------------------------------|" << endl;
        cout << PUTIH     << "|  0 | Kembali                                             |" << endl;
        cout << ABU_REDUP << "+==========================================================+" << RESET << endl;

        int opsi = inputMenu(0, 5, "  Pilih: ");

        switch (opsi) {
            case 1: tampilkanLowonganAdmin(currentUser); break;
            case 2: tampilkanHasilScrapingBookmark(currentUser); break;
            case 3: tampilkanBookmark(currentUser); break;
            case 4: jalankanScrapingJobstreet(currentUser->username); break;
            case 5: analisisHasilGabungan(); break;
            case 0: return;
                default:
                cout << "  [!] Opsi tidak valid (0-5)." << endl;
                tekanEnter();
                break;
        }
    }
}

//  [5] SORTING Urutkan Lowongan (A-Z)
void sortingLowongan(Lowongan* lowongan, int* jumlahLowongan) {
    CLEAR;
    cout << "\n=========================================================" << endl;
    cout << "|             SORTING LOWONGAN KERJA (GAJI)             |" << endl;
    cout << "|    Metode: Bubble Sort - Urutkan Gaji (Tertinggi)     |" << endl;
    cout << "=========================================================" << endl;

    if (*jumlahLowongan <= 1) {
        cout << "[i] Data terlalu sedikit untuk diurutkan." << endl;
        tekanEnter();
        return;
    }

    int n = *jumlahLowongan;
    for (int i = 0; i < n - 1; i++) {
        bool swapped = false;
        for (int j = 0; j < n - 1 - i; j++) {
            Lowongan* ptrA = lowongan + j;
            Lowongan* ptrB = lowongan + (j + 1);

            // Melakukan perbandingan secara gaji (Descending)
            if (ptrA->salaryExpectation < ptrB->salaryExpectation) {
                Lowongan temp = *ptrA;
                *ptrA = *ptrB;
                *ptrB = temp;
                swapped = true;
            }
        }
        if (!swapped) break;
    }

    saveLowongan(lowongan, *jumlahLowongan);

    cout << "\n[OK] Data berhasil diurutkan berdasarkan Gaji (Tertinggi ke Terendah).\n" << endl;

    tampilkanTabelLowongan(lowongan, *jumlahLowongan);
    tekanEnter();
}

//  Admin Flow: Audit Bookmark
void tampilkanBookmarkUserAdmin(const string& targetUsername) {
    BookmarkedJob allBookmarks[MAX_BOOKMARKS];
    int jumlahAllBookmarks = 0;
    loadBookmarkedJobs(allBookmarks, &jumlahAllBookmarks);

    BookmarkedJob userBookmarks[MAX_BOOKMARKS];
    int jumlahUserBookmarks = 0;

    for (int i = 0; i < jumlahAllBookmarks; i++) {
        if (allBookmarks[i].username == targetUsername) {
            userBookmarks[jumlahUserBookmarks] = allBookmarks[i];
            jumlahUserBookmarks++;
        }
    }

    CLEAR;
    cout << "\n+==========================================================+" << endl;
    cout << "|              DAFTAR BOOKMARK USER (AUDIT)                |" << endl;
    cout << "+==========================================================+" << endl;
    cout << "  Username Target : " << targetUsername << endl;
    cout << "  Total Bookmark  : " << jumlahUserBookmarks << endl;
    cout << "------------------------------------------------------------" << endl;

    if (jumlahUserBookmarks == 0) {
        cout << "\n  [INFO] User ini belum memiliki bookmark." << endl;
        tekanEnter();
        return;
    }

    tampilkanTabelBookmark(userBookmarks, jumlahUserBookmarks);
    tekanEnter();
}

//  Bridge: processBookmarkRequest Reusable bookmark logic
//  Digunakan oleh bridge CLI dan UI manual bookmark

bool processBookmarkRequest(const string& username, const string& jobId, 
                            const string& source, int score) {
    // 1. Load existing bookmarks
    BookmarkedJob bookmarks[MAX_BOOKMARKS];
    int jumlahBookmarks = 0;
    loadBookmarkedJobs(bookmarks, &jumlahBookmarks);

    // 2. Cek duplikat  jika sudah ada, update source & score
    for (int i = 0; i < jumlahBookmarks; i++) {
        BookmarkedJob* b = bookmarks + i;
        if (b->sourceId == jobId && b->username == username) {
            b->source = source;
            b->score = score;
            b->tanggalDitandai = getCurrentDate();
            saveBookmarkedJobs(bookmarks, jumlahBookmarks);
            return true;
        }
    }

    // 3. Cari data lowongan dari merged_data.json
    string mergedFile = "data/scrape/merged_data.json";
    if (!fs::exists(mergedFile)) {
        return false;
    }

    try {
        ifstream file(mergedFile);
        if (!file.is_open()) return false;

        json jobsJson;
        file >> jobsJson;
        file.close();

        if (!jobsJson.is_array()) return false;

        // Cari job berdasarkan ID
        for (const auto& job : jobsJson) {
            string currentId = "";
            if (job.contains("id") && !job["id"].is_null()) {
                if (job["id"].is_string()) currentId = job["id"].get<string>();
                else currentId = to_string(job["id"].get<int>());
            }

            if (currentId == jobId) {
                if (jumlahBookmarks >= MAX_BOOKMARKS) return false;

                BookmarkedJob bm;
                bm.bookmarkId = generateNewBookmarkId(bookmarks, jumlahBookmarks);
                bm.username = username;
                bm.source = source;
                bm.sourceId = jobId;
                bm.score = score;

                bm.title = (job.contains("title") && !job["title"].is_null()) 
                    ? job.value("title", "N/A") : "N/A";
                bm.company = (job.contains("company") && !job["company"].is_null()) 
                    ? job.value("company", "N/A") : "N/A";
                bm.location = (job.contains("location") && !job["location"].is_null()) 
                    ? job.value("location", "N/A") : "N/A";
                bm.desc = (job.contains("description") && !job["description"].is_null()) 
                    ? job.value("description", "N/A") : "N/A";
                bm.url = (job.contains("url") && !job["url"].is_null()) 
                    ? job.value("url", "N/A") : "N/A";
                bm.field = (job.contains("field") && !job["field"].is_null()) 
                    ? job.value("field", "N/A") : "N/A";
                bm.postedDate = (job.contains("scraped_at") && !job["scraped_at"].is_null()) 
                    ? job.value("scraped_at", "N/A") : "N/A";
                bm.skills = "";

                // Handle salary
                long long salMin = (job.contains("salary_min") && !job["salary_min"].is_null()) 
                    ? job.value("salary_min", 0LL) : 0LL;
                long long salMax = (job.contains("salary_max") && !job["salary_max"].is_null()) 
                    ? job.value("salary_max", 0LL) : 0LL;
                if (salMin > 0 && salMax > 0) bm.salary = to_string(salMin) + " - " + to_string(salMax);
                else if (salMin > 0) bm.salary = "> " + to_string(salMin);
                else if (salMax > 0) bm.salary = "< " + to_string(salMax);
                else bm.salary = "N/A";

                bm.tanggalDitandai = getCurrentDate();

                BookmarkedJob* ptrBaru = bookmarks + jumlahBookmarks;
                *ptrBaru = bm;
                jumlahBookmarks++;

                saveBookmarkedJobs(bookmarks, jumlahBookmarks);
                return true;
            }
        }
    } catch (...) {
        return false;
    }

    return false;
}

//  Bridge: handleBridgeCommand  CLI endpoint handler
//  Dipanggil dari main() jika argc > 1

bool handleBridgeCommand(int argc, char* argv[]) {
    if (argc < 2) return false;

    string command = argv[1];

    if (command == "bridge-get-profile") {
        if (argc < 3) {
            cerr << "{\"error\": \"Missing username argument\"}" << endl;
            return true;
        }
        string targetUsername = argv[2];

        Profile profiles[MAX_PROFILES];
        int jumlahProfile = 0;
        loadProfiles(profiles, &jumlahProfile);

        // Cari profil berdasarkan username
        for (int i = 0; i < jumlahProfile; i++) {
            Profile* p = profiles + i;
            if (p->username == targetUsername) {
                json j = *p;
                cout << j.dump(4) << endl;
                return true;
            }
        }

        // Jika tidak ditemukan, output JSON kosong
        cerr << "{\"error\": \"Profile not found for user: " << targetUsername << "\"}" << endl;
        cout << "{}" << endl;
        return true;
    }

    if (command == "bridge-add-bookmark") {
        if (argc < 6) {
            cerr << "{\"error\": \"Missing arguments. Usage: bridge-add-bookmark <user> <job_id> <source> <score>\"}" << endl;
            cout << "{\"status\": \"error\"}" << endl;
            return true;
        }

        string username = argv[2];
        string jobId = argv[3];
        string source = argv[4];
        int score = 0;
        try { score = stoi(argv[5]); } catch (...) { score = 0; }

        bool success = processBookmarkRequest(username, jobId, source, score);
        if (success) {
            cout << "{\"status\": \"success\"}" << endl;
        } else {
            cout << "{\"status\": \"error\", \"message\": \"Bookmark gagal\"}" << endl;
        }
        return true;
    }

    return false;
}
