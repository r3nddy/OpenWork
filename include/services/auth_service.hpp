#pragma once
#include "models.hpp"

void displayMenuAuth();

int login(User *users, int *jumlahUser);

bool registerUser(User *users, int *jumlahUser);

void handleAuditUserMenu(User *users, int *jumlahUser);

// Hanya dipanggil dari menu Admin — membuat akun Admin baru
void tambahAdmin(User *users, int *jumlahUser);
