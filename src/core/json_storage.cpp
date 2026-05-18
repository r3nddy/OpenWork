#include "core/json_storage.hpp"
#include "constants.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

//  JSON Storage Implementation
void loadUsers(User* users, int* jumlahUser) {
    
    *jumlahUser = 0;
    if (!fs::exists(USERS_FILE)) {
        return;
    }

    try {
        ifstream file(USERS_FILE);
        if (!file.is_open()) {
            throw runtime_error("Tidak bisa membuka file: " + USERS_FILE);
        }

        json j;
        file >> j;
        file.close();

        vector<User> loadedUsers = j.get<vector<User>>();
        for (const auto& u : loadedUsers) {
            // Kita harus punya mekanisme batas maksimal array
            // MAX_USERS di main/common. Namun karena array diserahkan dari luar,
            // asumsikan cukup (MAX_USERS = 100).
            User* ptrUser = users + (*jumlahUser);
            *ptrUser = u;
            (*jumlahUser)++;
        }

    } catch (const json::exception& e) {
        cerr << "  [ERROR] Gagal parsing JSON users: " << e.what() << endl;
    } catch (const exception& e) {
        cerr << "  [ERROR] " << e.what() << endl;
    }
}

void saveUsers(User* users, int jumlahUser) {
    try {
        fs::create_directories("data");

        ofstream file(USERS_FILE);
        if (!file.is_open()) {
            throw runtime_error("Tidak bisa menulis ke file: " + USERS_FILE);
        }

        vector<User> userList;
        for (int i = 0; i < jumlahUser; i++) {
            userList.push_back(*(users + i));
        }

        json j = userList;
        file << j.dump(4, ' ', false, json::error_handler_t::replace);
        file.close();

    } catch (const exception& e) {
        cerr << "  [ERROR] Gagal menyimpan users: " << e.what() << endl;
    }
}

void loadProfiles(Profile* profiles, int* jumlahProfile) {
    *jumlahProfile = 0;
    if (!fs::exists(PROFILE_FILE)) {
        return;
    }

    try {
        ifstream file(PROFILE_FILE);
        if (!file.is_open()) {
            throw runtime_error("Tidak bisa membuka file: " + PROFILE_FILE);
        }

        json j;
        file >> j;
        file.close();

        if (j.is_array()) {
            vector<Profile> loadedProfiles = j.get<vector<Profile>>();
            for (const auto& p : loadedProfiles) {
                if (*jumlahProfile >= MAX_PROFILES) break;
                Profile* ptrProfile = profiles + (*jumlahProfile);
                *ptrProfile = p;
                (*jumlahProfile)++;
            }
        } else if (j.is_object()) {
            Profile p = j.get<Profile>();
            p.id = 1;
            profiles[0] = p;
            *jumlahProfile = 1;
        }

    } catch (const json::exception& e) {
        cerr << "  [ERROR] Gagal parsing JSON profiles: " << e.what() << endl;
    } catch (const exception& e) {
        cerr << "  [ERROR] " << e.what() << endl;
    }
}

void saveProfiles(Profile* profiles, int jumlahProfile) {
    try {
        fs::create_directories("data");

        ofstream file(PROFILE_FILE);
        if (!file.is_open()) {
            throw runtime_error("Tidak bisa menulis ke file: " + PROFILE_FILE);
        }

        vector<Profile> profileList;
        for (int i = 0; i < jumlahProfile; i++) {
            profileList.push_back(*(profiles + i));
        }

        json j = profileList;
        file << j.dump(4, ' ', false, json::error_handler_t::replace);
        file.close();

    } catch (const exception& e) {
        cerr << "  [ERROR] Gagal menyimpan profiles: " << e.what() << endl;
    }
}

int generateNewProfileId(Profile* profiles, int jumlahProfile, int n) {
    for (int i = 0; i < jumlahProfile; i++) {
        Profile* ptr = profiles + i;
        if (ptr->id == n) {
            return generateNewProfileId(profiles, jumlahProfile, n + 1);
        }
    }
    return n;
}

//  JSON Storage — Lowongan Kerja

void loadLowongan(Lowongan* lowongan, int* jumlahLowongan) {
    *jumlahLowongan = 0;

    if (!fs::exists(LOWONGAN_FILE)) {
        return;
    }

    try {
        ifstream file(LOWONGAN_FILE);
        if (!file.is_open()) {
            throw runtime_error("Tidak bisa membuka file: " + LOWONGAN_FILE);
        }

        json j;
        file >> j;
        file.close();

        vector<Lowongan> loadedLowongan = j.get<vector<Lowongan>>();
        for (const auto& l : loadedLowongan) {
            if (*jumlahLowongan >= MAX_LOWONGAN) break;
            Lowongan* ptrLowongan = lowongan + (*jumlahLowongan);
            *ptrLowongan = l;
            (*jumlahLowongan)++;
        }

    } catch (const json::exception& e) {
        cerr << "  [ERROR] Gagal parsing JSON lowongan: " << e.what() << endl;
    } catch (const exception& e) {
        cerr << "  [ERROR] " << e.what() << endl;
    }
}

void saveLowongan(Lowongan* lowongan, int jumlahLowongan) {
    try {
        fs::create_directories("data");

        ofstream file(LOWONGAN_FILE);
        if (!file.is_open()) {
            throw runtime_error("Tidak bisa menulis ke file: " + LOWONGAN_FILE);
        }

        vector<Lowongan> lowonganList;
        for (int i = 0; i < jumlahLowongan; i++) {
            lowonganList.push_back(*(lowongan + i));
        }

        json j = lowonganList;
        file << j.dump(4, ' ', false, json::error_handler_t::replace);
        file.close();

    } catch (const exception& e) {
        cerr << "  [ERROR] Gagal menyimpan lowongan: " << e.what() << endl;
    }
}

int generateNewLowonganId(Lowongan* lowongan, int jumlahLowongan, int n) {
    for (int i = 0; i < jumlahLowongan; i++) {
        Lowongan* ptr = lowongan + i;
        if (ptr->id == n) {
            return generateNewLowonganId(lowongan, jumlahLowongan, n + 1);
        }
    }
    return n;
}

//  JSON Storage — Bookmarked Jobs

void loadBookmarkedJobs(BookmarkedJob* bookmarks, int* jumlahBookmarks) {
    *jumlahBookmarks = 0;

    if (!fs::exists(BOOKMARKED_JOBS_FILE)) {
        return;
    }

    try {
        ifstream file(BOOKMARKED_JOBS_FILE);
        if (!file.is_open()) {
            throw runtime_error("Tidak bisa membuka file: " + BOOKMARKED_JOBS_FILE);
        }

        json j;
        file >> j;
        file.close();

        vector<BookmarkedJob> loadedBookmarks = j.get<vector<BookmarkedJob>>();
        for (const auto& b : loadedBookmarks) {
            if (*jumlahBookmarks >= MAX_BOOKMARKS) break;
            BookmarkedJob* ptr = bookmarks + (*jumlahBookmarks);
            *ptr = b;
            (*jumlahBookmarks)++;
        }

    } catch (const json::exception& e) {
        cerr << "  [ERROR] Gagal parsing JSON bookmark: " << e.what() << endl;
    } catch (const exception& e) {
        cerr << "  [ERROR] " << e.what() << endl;
    }
}

void saveBookmarkedJobs(BookmarkedJob* bookmarks, int jumlahBookmarks) {
    try {
        fs::create_directories("data");

        ofstream file(BOOKMARKED_JOBS_FILE);
        if (!file.is_open()) {
            throw runtime_error("Tidak bisa menulis ke file: " + BOOKMARKED_JOBS_FILE);
        }

        vector<BookmarkedJob> list;
        for (int i = 0; i < jumlahBookmarks; i++) {
            list.push_back(*(bookmarks + i));
        }

        json j = list;
        file << j.dump(4, ' ', false, json::error_handler_t::replace);
        file.close();

    } catch (const exception& e) {
        cerr << "  [ERROR] Gagal menyimpan bookmark: " << e.what() << endl;
    }
}

int generateNewBookmarkId(BookmarkedJob* bookmarks, int jumlahBookmarks, int n) {
    for (int i = 0; i < jumlahBookmarks; i++) {
        BookmarkedJob* ptr = bookmarks + i;
        if (ptr->bookmarkId == n) {
            return generateNewBookmarkId(bookmarks, jumlahBookmarks, n + 1);
        }
    }
    return n;
}

bool clearBookmarkedJobs() {
    try {
        if (!fs::exists(BOOKMARKED_JOBS_FILE)) {
            return false;
        }
        return fs::remove(BOOKMARKED_JOBS_FILE);
    } catch (const exception& e) {
        cerr << "  [ERROR] Gagal menghapus bookmark: " << e.what() << endl;
        return false;
    }
}
