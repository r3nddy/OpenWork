#pragma once
#include "models.hpp"

// Dipanggil dari handleUserMenu — tampilkan & kelola profil user yang sedang login
void handleProfileMenu(const string& username);

// Dipanggil dari registerUser() di auth_service — buat profil otomatis
void createProfileForUser(const string& username);

// Dipanggil dari auth_service saat akun dihapus — BUKAN dari UI
void deleteProfileForUser(const string& username);
