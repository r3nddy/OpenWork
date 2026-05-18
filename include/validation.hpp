#pragma once

#include <string>
#include <regex>

using namespace std;

// Username: hanya huruf, _, ., spasi, panjang 3-20 (tidak boleh ada angka)
inline bool isValidUsername(const string& username) {
    regex pattern("^[a-zA-Z_. ]{3,20}$");
    return regex_match(username, pattern);
}

// Password: min 8 karakter, 1 angka (huruf besar/kecil opsional)
inline bool isValidPassword(const string& password) {
    regex pattern("^(?=.*\\d)[a-zA-Z\\d\\w\\W]{8,}$");
    return regex_match(password, pattern);
}

// Nama: Huruf, spasi, ', -, panjang 2-50
inline bool isValidName(const string& name) {
    regex pattern("^[a-zA-Z\\s'\\-]{2,50}$");
    return regex_match(name, pattern);
}

// Lokasi: Huruf, spasi, ,, ., panjang 3-100 (TIDAK boleh ada angka)
inline bool isValidLocation(const string& location) {
    regex pattern("^[a-zA-Z\\s,\\.]{3,100}$");
    return regex_match(location, pattern);
}

// Perusahaan: Huruf, angka, spasi, ,, ., panjang 3-100 (Boleh ada angka)
inline bool isValidCompany(const string& company) {
    regex pattern("^[a-zA-Z0-9\\s,\\.]{3,100}$");
    return regex_match(company, pattern);
}

// URL: HTTP/HTTPS
inline bool isValidURL(const string& url) {
    regex pattern("^https?:\\/\\/(?:www\\.)?[-a-zA-Z0-9@:%._\\+~#=]{1,256}\\.[a-zA-Z0-9()]{1,6}\\b(?:[-a-zA-Z0-9()@:%_\\+.~#?&\\/=]*)$");
    return regex_match(url, pattern);
}

// Tanggal: YYYY-MM-DD
inline bool isValidDate(const string& date) {
    regex pattern("^\\d{4}-\\d{2}-\\d{2}$");
    return regex_match(date, pattern);
}

// Judul Pekerjaan: Huruf, angka, spasi, &, -, (, )
inline bool isValidJobTitle(const string& title) {
    regex pattern("^[a-zA-Z0-9\\s&\\-\\(\\)\\.]{3,100}$");
    return regex_match(title, pattern);
}

// Skills List: comma separated strings (e.g. "C++, Java, Python")
inline bool isValidSkillsList(const string& skills) {
    if (skills.empty()) return true;
    regex pattern("^([a-zA-Z0-9\\s+#\\-\\.]+)(,\\s*[a-zA-Z0-9\\s+#\\-\\.]+)*$");
    return regex_match(skills, pattern);
}
