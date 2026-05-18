#pragma once
#include "models.hpp"

// CRUD Lowongan (Admin)
void lihatSemuaLowongan(Lowongan* lowongan, int* jumlahLowongan);

void tambahLowongan(Lowongan* lowongan, int* jumlahLowongan, const string& adminUsername);

void editLowongan(Lowongan* lowongan, int* jumlahLowongan);

void hapusLowongan(Lowongan* lowongan, int* jumlahLowongan);


void sortingLowongan(Lowongan* lowongan, int* jumlahLowongan);
void handleLowonganMenu(const string& adminUsername);

// Cari Kerja
void menuCariKerja(User* currentUser);

void tampilkanBookmark(User* currentUser);

void analisisKecocokanProfil(User* currentUser);

// Audit Bookmark
void tampilkanBookmarkUserAdmin(const string& targetUsername);

// Digunakan oleh bridge CLI dan UI manual bookmark
bool processBookmarkRequest(const string& username, const string& jobId, const string& source, int score);

// Bridge: CLI command handler
// Menangani bridge-get-profile dan bridge-add-bookmark dari CLI
// Returns true jika command adalah bridge command (dan sudah dihandle)
bool handleBridgeCommand(int argc, char* argv[]);
