#include "services/profile_service.hpp"
#include "models.hpp"
#include "core/json_storage.hpp"
#include "constants.hpp"
#include "utils.hpp"
#include "validation.hpp"
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>

using namespace std;

// Helper: level int → label
static string levelLabel(int lv) {
    if (lv == 1) return "Beginner";
    if (lv == 2) return "Intermediate";
    if (lv == 3) return "Expert";
    return "?";
}

// Helper: duration int → label
static string durationLabel(int d) {
    if (d == 1) return "< 1 tahun";
    if (d == 2) return "1 - 2 tahun";
    if (d == 3) return "3 - 5 tahun";
    if (d == 4) return "5 tahun ke atas";
    return "?";
}



// Helper: input gaji non-negatif
static double inputSalaryNonNegative(const string& prompt) {
    string errorMsg = "";
    while (true) {
        if (!errorMsg.empty()) cout << errorMsg << endl;
        string raw;
        cout << prompt;
        getline(cin, raw);
        raw = trim(raw);
        if (raw.empty()) {
            clearLines(1 + (!errorMsg.empty() ? 1 : 0));
            errorMsg = "  \033[31m[!]\033[0m Input tidak boleh kosong.";
            continue;
        }
        try {
            size_t idx = 0;
            double value = stod(raw, &idx);
            if (idx != raw.size()) throw invalid_argument("trailing chars");
            if (value < 0) {
                clearLines(1 + (!errorMsg.empty() ? 1 : 0));
                errorMsg = "  \033[31m[!]\033[0m Ekspektasi gaji tidak boleh negatif.";
                continue;
            }
            return value;
        } catch (...) {
            clearLines(1 + (!errorMsg.empty() ? 1 : 0));
            errorMsg = "  \033[31m[!]\033[0m Input angka tidak valid.";
        }
    }
}

//  Tampilkan header profil (info read-only + ringkasan)
static void tampilkanHeaderProfil(const Profile& p) {
    cout << ABU_TERANG << "+----------------------------------------------------------+" << endl;
    cout << "| " << EMAS << "[ DATA IDENTITAS ]                                       " << ABU_TERANG << "|" << endl;
    cout << "| " << PUTIH << "ID User    : " << BIRU << left << setw(44) << to_string(p.id) << ABU_TERANG << "|" << endl;
    cout << "| " << PUTIH << "Username   : " << BIRU << left << setw(44) << p.username << ABU_TERANG << "|" << endl;
    cout << "| " << PUTIH << "Lokasi     : " << BIRU << left << setw(44) << (p.location.empty() ? "-" : p.location) << ABU_TERANG << "|" << endl;
    cout << "| " << PUTIH << "Gaji (Exp) : " << BIRU << left << setw(44) << formatRupiah(p.salaryExpectation) << ABU_TERANG << "|" << endl;
    cout << "+----------------------------------------------------------+" << endl;
    cout << "| " << PUTIH << "Skills     : " << BIRU << left << setw(44) << (to_string(p.jumlahSkill) + " Keahlian terdaftar") << ABU_TERANG << "|" << endl;
    cout << "| " << PUTIH << "Pengalaman : " << BIRU << left << setw(44) << (to_string(p.jumlahExperience) + " Pengalaman terdaftar") << ABU_TERANG << "|" << endl;
    cout << "+----------------------------------------------------------+" << RESET << endl;
}

//  KELOLA SKILLS

static void handleSkillMenu(Profile* p) {
    while (true) {
        CLEAR;
        cout << ABU_REDUP << "\n+==========================================================+" << endl;
        cout << EMAS      << "|                  KELOLA SKILL PENGGUNA                   |" << endl;
        cout << ABU_REDUP << "+==========================================================+" << RESET << endl;
        cout << PUTIH     << "  Profil : " << BIRU << p->name << RESET << endl;
        cout << ABU_TERANG << "  ----------------------------------------------------------" << RESET << endl;

        if (p->jumlahSkill == 0) {
            cout << ABU_TERANG << "\n  [INFO] " << PUTIH << "Belum ada skill terdaftar." << RESET << endl;
        } else {
            cout << left;
            cout << PUTIH     << "  " << setw(5) << "NO" << setw(35) << "NAMA SKILL" << "LEVEL" << RESET << endl;
            cout << ABU_TERANG << "  " << string(52, '-') << RESET << endl;
            for (int i = 0; i < p->jumlahSkill; i++) {
                const Profile::Skill& s = p->skills[i];
                string lvStr = levelLabel(s.level);
                cout << ABU_TERANG << "  " << setw(5) << (to_string(i + 1) + ".")
                             << PUTIH << setw(35) << truncate(s.name, 33)
                             << BIRU << "[" << s.level << "] " << PUTIH << lvStr << RESET << endl;
            }
        }

        cout << ABU_REDUP << "\n+==========================================================+" << endl;
        cout << PUTIH     << "| No | Pilihan                                             |" << endl;
        cout << ABU_TERANG << "|----|-----------------------------------------------------|" << endl;
        cout << PUTIH     << "|  1 | Tambah Skill Baru                                   |" << endl;
        if (p->jumlahSkill > 0)
        cout << PUTIH     << "|  2 | Hapus Skill Terdaftar                               |" << endl;
        cout << ABU_TERANG << "|----|-----------------------------------------------------|" << endl;
        cout << PUTIH     << "|  0 | Kembali ke Menu Profil                              |" << endl;
        cout << ABU_REDUP << "+==========================================================+" << RESET << endl;

        int opsi = inputMenu(0, 2, "\n  Pilih (0-2): ");

        if (opsi == 0) return;

        if (opsi == 1) {
            if (p->jumlahSkill >= MAX_SKILLS) {
                cout << MERAH << "\n  [!] " << PUTIH << "Kapasitas skill penuh (" << MAX_SKILLS << ")." << RESET << endl;
                tekanEnter();
                continue;
            }
            CLEAR;
            cout << ABU_REDUP << "\n+==========================================================+" << endl;
            cout << EMAS      << "|                   TAMBAH SKILL BARU                      |" << endl;
            cout << ABU_REDUP << "+==========================================================+" << RESET << endl;
            Profile::Skill& s = p->skills[p->jumlahSkill];
            
            s.name = inputStringWithValidation(PUTIH + "  Nama Skill : " + BIRU, isValidSkillsList, "Nama Skill tidak valid. Gunakan huruf, angka, spasi, +, -, #.");
            cout << RESET;

            s.level = inputMenu(1, 3, "\n  Pilih Level Skill:\n    [1] Beginner\n    [2] Intermediate\n    [3] Expert\n  Pilihan: ");
            p->jumlahSkill++;
            cout << HIJAU << "\n  [OK] " << PUTIH << "Skill berhasil ditambahkan ke profil." << RESET << endl;
            tekanEnter();

        } else if (opsi == 2 && p->jumlahSkill > 0) {
            int idx = inputMenu(1, p->jumlahSkill, "\n  Masukkan nomor skill (1-" + to_string(p->jumlahSkill) + "): ");

            if (idx < 1 || idx > p->jumlahSkill) {
                cout << MERAH << "  [!] " << PUTIH << "Nomor tidak valid." << RESET << endl;
                tekanEnter();
                continue;
            }
            cout << PUTIH << "  Yakin hapus skill [" << BIRU << p->skills[idx - 1].name << PUTIH << "]? (y/n): " << BIRU;
            string konfirm;
            getline(cin, konfirm);
            cout << RESET;
            if (konfirm == "y" || konfirm == "Y") {
                for (int i = idx - 1; i < p->jumlahSkill - 1; i++)
                    p->skills[i] = p->skills[i + 1];
                p->jumlahSkill--;
                cout << HIJAU << "  [OK] " << PUTIH << "Skill berhasil dihapus." << RESET << endl;
                tekanEnter();
            } else {
                cout << ABU_TERANG << "  [INFO] Penghapusan dibatalkan." << RESET << endl;
                tekanEnter();
            }
        }
    }
}

//  KELOLA PENGALAMAN KERJA

static void handleExperienceMenu(Profile* p) {
    while (true) {
        CLEAR;
        cout << ABU_REDUP << "\n+================================================================================+" << endl;
        cout << EMAS      << "|                           KELOLA PENGALAMAN PENGGUNA                           |" << endl;
        cout << ABU_REDUP << "+================================================================================+" << RESET << endl;
        cout << PUTIH     << "  Profil : " << BIRU << p->name << RESET << endl;
        cout << ABU_TERANG << "  --------------------------------------------------------------------------------" << RESET << endl;

        if (p->jumlahExperience == 0) {
            cout << ABU_TERANG << "\n  [INFO] " << PUTIH << "Belum ada pengalaman terdaftar." << RESET << endl;
        } else {
            cout << left;
            cout << PUTIH     << "  " << setw(5) << "NO"
                         << setw(28) << "POSISI"
                         << setw(28) << "PERUSAHAAN"
                         << "DURASI" << RESET << endl;
            cout << ABU_TERANG << "  " << string(80, '-') << RESET << endl;
            for (int i = 0; i < p->jumlahExperience; i++) {
                const Profile::Experience& e = p->experiences[i];
                cout << ABU_TERANG << "  " << setw(5)  << (to_string(i + 1) + ".")
                             << PUTIH << setw(28) << truncate(e.role,    26)
                             << BIRU << setw(28) << truncate(e.company, 26)
                             << PUTIH << durationLabel(e.duration) << RESET << endl;
            }
        }

        cout << ABU_REDUP << "\n+================================================================================+" << endl;
        cout << PUTIH     << "|  NO | PILIHAN MENU                                                             |" << endl;
        cout << ABU_TERANG << "|-----|--------------------------------------------------------------------------|" << endl;
        cout << PUTIH     << "|  1  | Tambah Pengalaman Baru                                                   |" << endl;
        if (p->jumlahExperience > 0)
        cout << PUTIH     << "|  2  | Hapus Pengalaman Terdaftar                                               |" << endl;
        cout << ABU_TERANG << "|-----|--------------------------------------------------------------------------|" << endl;
        cout << PUTIH     << "|  0  | Kembali ke Menu Profil                                                   |" << endl;
        cout << ABU_REDUP << "+================================================================================+" << RESET << endl;

        int opsi = inputMenu(0, 2, "\n  Pilih (0-2): ");

        if (opsi == 0) return;

        if (opsi == 1) {
            if (p->jumlahExperience >= MAX_EXPERIENCES) {
                cout << MERAH << "\n  [!] " << PUTIH << "Kapasitas pengalaman penuh (" << MAX_EXPERIENCES << ")." << RESET << endl;
                tekanEnter();
                continue;
            }
            CLEAR;
            cout << ABU_REDUP << "\n+==========================================================+" << endl;
            cout << EMAS      << "|                TAMBAH PENGALAMAN KERJA                   |" << endl;
            cout << ABU_REDUP << "+==========================================================+" << RESET << endl;
            Profile::Experience& e = p->experiences[p->jumlahExperience];
            
            e.company = inputStringWithValidation(PUTIH + "  Nama Perusahaan  : " + BIRU, isValidCompany, "Nama perusahaan tidak valid.");
            cout << RESET;

            e.role = inputStringWithValidation(PUTIH + "  Posisi / Jabatan : " + BIRU, isValidName, "Jabatan tidak valid.");
            cout << RESET;

            e.duration = inputMenu(1, 4, "\n  Pilih Durasi:\n    [1] Kurang dari 1 tahun\n    [2] 1 - 2 tahun\n    [3] 3 - 5 tahun\n    [4] 5 tahun ke atas\n  Pilihan: ");
            p->jumlahExperience++;
            cout << HIJAU << "\n  [OK] " << PUTIH << "Pengalaman berhasil ditambahkan ke profil." << RESET << endl;
            tekanEnter();

        } else if (opsi == 2 && p->jumlahExperience > 0) {
            int idx = inputMenu(1, p->jumlahExperience, "\n  Masukkan nomor pengalaman (1-" + to_string(p->jumlahExperience) + "): ");

            if (idx < 1 || idx > p->jumlahExperience) {
                cout << MERAH << "  [!] " << PUTIH << "Nomor tidak valid." << RESET << endl;
                tekanEnter();
                continue;
            }
            cout << PUTIH << "  Yakin hapus pengalaman [" << BIRU << p->experiences[idx - 1].role << PUTIH << "]? (y/n): " << BIRU;
            string konfirm;
            getline(cin, konfirm);
            cout << RESET;
            if (konfirm == "y" || konfirm == "Y") {
                for (int i = idx - 1; i < p->jumlahExperience - 1; i++)
                    p->experiences[i] = p->experiences[i + 1];
                p->jumlahExperience--;
                cout << HIJAU << "  [OK] " << PUTIH << "Pengalaman berhasil dihapus." << RESET << endl;
                tekanEnter();
            } else {
                cout << ABU_TERANG << "  [INFO] Penghapusan dibatalkan." << RESET << endl;
                tekanEnter();
            }
        }
    }
}

//  PUBLIC API

// Buat profil otomatis saat register — dipanggil dari auth_service
void createProfileForUser(const string& username) {
    Profile profiles[MAX_PROFILES];
    int jumlahProfile = 0;
    loadProfiles(profiles, &jumlahProfile);

    // Idempoten: jika profil sudah ada, tidak dibuat ulang
    for (int i = 0; i < jumlahProfile; i++) {
        if (profiles[i].username == username) return;
    }

    if (jumlahProfile >= MAX_PROFILES) return;  // penuh, abaikan

    Profile* p = profiles + jumlahProfile;
    p->id                = generateNewProfileId(profiles, jumlahProfile);
    p->username          = username;
    p->name              = username;
    p->location          = "";
    p->salaryExpectation = 0.0;
    p->jumlahSkill       = 0;
    p->jumlahExperience  = 0;
    jumlahProfile++;

    saveProfiles(profiles, jumlahProfile);
}

// Hapus profil saat akun dihapus — HANYA dipanggil dari auth_service
void deleteProfileForUser(const string& username) {
    Profile profiles[MAX_PROFILES];
    int jumlahProfile = 0;
    loadProfiles(profiles, &jumlahProfile);

    int index = -1;
    for (int i = 0; i < jumlahProfile; i++) {
        if (profiles[i].username == username) { index = i; break; }
    }
    if (index == -1) return;  // tidak ditemukan, tidak apa-apa

    for (int i = index; i < jumlahProfile - 1; i++)
        profiles[i] = profiles[i + 1];
    jumlahProfile--;

    saveProfiles(profiles, jumlahProfile);
}

// Menu profil user yang sedang login
void handleProfileMenu(const string& username) {
    Profile profiles[MAX_PROFILES];
    int jumlahProfile = 0;
    loadProfiles(profiles, &jumlahProfile);

    // Cari profil milik user ini
    int profileIdx = -1;
    for (int i = 0; i < jumlahProfile; i++) {
        if (profiles[i].username == username) { profileIdx = i; break; }
    }

    // Safety net: jika profil belum ada (mis. user lama sebelum fitur ini), buat otomatis
    if (profileIdx == -1) {
        createProfileForUser(username);
        loadProfiles(profiles, &jumlahProfile);
        for (int i = 0; i < jumlahProfile; i++) {
            if (profiles[i].username == username) { profileIdx = i; break; }
        }
    }

    Profile* p = profiles + profileIdx;

    while (true) {
        CLEAR;
        cout << ABU_REDUP << "\n+==========================================================+" << endl;
        cout << EMAS      << "|                  MANAJEMEN PROFIL SAYA                   |" << endl;
        cout << ABU_REDUP << "+==========================================================+" << RESET << endl;
        tampilkanHeaderProfil(*p);
        cout << ABU_REDUP << "+==========================================================+" << endl;
        cout << PUTIH     << "| No | Kelola Bagian                                       |" << endl;
        cout << ABU_TERANG << "|----|-----------------------------------------------------|" << endl;
        cout << PUTIH     << "|  1 | Edit Lokasi                                         |" << endl;
        cout << PUTIH     << "|  2 | Edit Ekspektasi Gaji Bulanan                        |" << endl;
        cout << PUTIH     << "|  3 | Kelola Daftar Skills                                |" << endl;
        cout << PUTIH     << "|  4 | Kelola Pengalaman Kerja                             |" << endl;
        cout << ABU_TERANG << "|----|-----------------------------------------------------|" << endl;
        cout << PUTIH     << "|  0 | Kembali ke Menu Utama                               |" << endl;
        cout << ABU_REDUP << "+==========================================================+" << RESET << endl;

        int opsi = inputMenu(0, 4, "\n  Pilih (0-4): ");

        switch (opsi) {
            case 1: {
                CLEAR;
                cout << ABU_REDUP << "\n+==========================================================+" << endl;
                cout << EMAS      << "|                      EDIT LOKASI                         |" << endl;
                cout << ABU_REDUP << "+==========================================================+" << RESET << endl;
                cout << PUTIH     << "  Lokasi saat ini : " << BIRU << (p->location.empty() ? "-" : p->location) << RESET << endl;
                p->location = inputStringWithValidation(PUTIH + "  Lokasi baru     : " + BIRU, isValidLocation, "Lokasi tidak valid! Gunakan huruf, spasi, koma, titik.");
                cout << RESET;
                saveProfiles(profiles, jumlahProfile);
                cout << HIJAU << "\n  [OK] " << PUTIH << "Lokasi berhasil diperbarui." << RESET << endl;
                tekanEnter();
                break;
            }
            case 2: {
                while (true) {
                    CLEAR;
                    cout << ABU_REDUP << "\n+================================================================================+" << endl;
                    cout << EMAS      << "|                              EDIT EKSPEKTASI GAJI                              |" << endl;
                    cout << ABU_REDUP << "+================================================================================+" << RESET << endl;
                    cout << PUTIH     << "  Ekspektasi Saat Ini : " << BIRU << formatRupiah(p->salaryExpectation) << RESET << "\n" << endl;
                    
                    cout << ABU_REDUP << "+-----+--------------------------------------------------------------------------+" << endl;
                    cout << PUTIH     << "|  NO | PILIHAN NOMINAL GAJI                                                     |" << endl;
                    cout << ABU_REDUP << "+-----+--------------------------------------------------------------------------+" << endl;
                    cout << PUTIH     << "|  1  | Rp 2.500.000                                                             |" << endl;
                    cout << PUTIH     << "|  2  | Rp 4.000.000                                                             |" << endl;
                    cout << PUTIH     << "|  3  | Rp 6.000.000                                                             |" << endl;
                    cout << PUTIH     << "|  4  | Rp 8.500.000                                                             |" << endl;
                    cout << PUTIH     << "|  5  | Rp 12.000.000                                                            |" << endl;
                    cout << PUTIH     << "|  6  | Input Manual (Tentukan Nominal Sendiri)                                  |" << endl;
                    cout << ABU_REDUP << "+-----+--------------------------------------------------------------------------+" << endl;
                    cout << PUTIH     << "|  0  | Batal (Kembali ke Menu Profil)                                           |" << endl;
                    cout << ABU_REDUP << "+-----+--------------------------------------------------------------------------+\n" << RESET << endl;
                    
                    int opsiGaji = inputMenu(0, 6, "  Pilih Nomor Rentang Gaji (0-6): ");
                    
                    if (opsiGaji == 0) break;
                    
                    if (opsiGaji >= 1 && opsiGaji <= 5) {
                        double newSalary = 0;
                        if (opsiGaji == 1) newSalary = 2500000;
                        else if (opsiGaji == 2) newSalary = 4000000;
                        else if (opsiGaji == 3) newSalary = 6000000;
                        else if (opsiGaji == 4) newSalary = 8500000;
                        else if (opsiGaji == 5) newSalary = 12000000;
                        p->salaryExpectation = newSalary;
                    } else if (opsiGaji == 6) {
                        p->salaryExpectation = inputSalaryNonNegative(PUTIH + "  Ekspektasi baru   : Rp " + BIRU);
                        cout << RESET;
                    } else {
                        cout << MERAH << "  [!] " << PUTIH << "Pilihan tidak valid." << RESET << endl;
                        tekanEnter();
                        continue;
                    }
                    
                    saveProfiles(profiles, jumlahProfile);
                    cout << HIJAU << "\n  [OK] " << PUTIH << "Ekspektasi gaji berhasil diperbarui." << RESET << endl;
                    tekanEnter();
                    break;
                }
                break;
            }
            case 3:
                handleSkillMenu(p);
                saveProfiles(profiles, jumlahProfile);
                break;
            case 4:
                handleExperienceMenu(p);
                saveProfiles(profiles, jumlahProfile);
                break;
            case 0:
                return;
            default:
                cout << "  [!] Opsi tidak valid (0-4)." << endl;
                tekanEnter();
                break;
        }
    }
}
