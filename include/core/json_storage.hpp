#pragma once

//  OpenWork — JSON Storage Layer

#include <vector>
#include <string>
#include <optional>
#include "models.hpp"

using namespace std;

// ── User ──
void loadUsers(User* users, int* jumlahUser);
void saveUsers(User* users, int jumlahUser);


// ── Profile ──
void loadProfiles(Profile* profiles, int* jumlahProfile);
void saveProfiles(Profile* profiles, int jumlahProfile);
int generateNewProfileId(Profile* profiles, int jumlahProfile, int n = 1);

// ── Lowongan ──
void loadLowongan(Lowongan* lowongan, int* jumlahLowongan);
void saveLowongan(Lowongan* lowongan, int jumlahLowongan);
int generateNewLowonganId(Lowongan* lowongan, int jumlahLowongan, int n = 1);

// ── Bookmarked Jobs ──
void loadBookmarkedJobs(BookmarkedJob* bookmarks, int* jumlahBookmarks);
void saveBookmarkedJobs(BookmarkedJob* bookmarks, int jumlahBookmarks);
int generateNewBookmarkId(BookmarkedJob* bookmarks, int jumlahBookmarks, int n = 1);
bool clearBookmarkedJobs();
