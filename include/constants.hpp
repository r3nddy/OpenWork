#pragma once

#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    #define CLEAR system("cls")
#else
    #define CLEAR system("clear")
#endif

#include <string>

// Palette warna
inline const std::string RESET       = "\033[0m";
inline const std::string HIJAU       = "\033[38;5;82m";
inline const std::string COKELAT     = "\033[38;5;94m";
inline const std::string ABU_TERANG  = "\033[38;5;246m"; 
inline const std::string UNGU_GELAP  = "\033[38;5;55m";  
inline const std::string BIRU        = "\033[38;5;87m";  
inline const std::string EMAS        = "\033[38;5;220m"; 
inline const std::string MERAH       = "\033[38;5;196m"; 
inline const std::string TOSKA       = "\033[38;5;48m"; 
inline const std::string PUTIH       = "\033[97m";      
inline const std::string KUNING      = "\033[93m";      
inline const std::string UNGU        = "\033[38;5;129m";
inline const std::string ABU_REDUP   = "\033[90m";

// File path constants (data/)
inline const std::string PROFILE_FILE         = "data/profile.json";
inline const std::string LOWONGAN_FILE        = "data/lowongan.json";
inline const std::string BOOKMARKED_JOBS_FILE = "data/bookmarked_jobs.json";
inline const std::string USERS_FILE           = "data/users.json";

// Array Limits
inline const int MAX_LOWONGAN    = 100;
inline const int MAX_PROFILES    = 50;
inline const int MAX_USERS       = 50;
inline const int MAX_SKILLS      = 20;
inline const int MAX_EXPERIENCES = 20;
inline const int MAX_BOOKMARKS   = 500;
