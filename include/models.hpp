#pragma once

#include "constants.hpp"
#include <string>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

// Struct: Profile
struct Profile {
    struct Skill {
        string name;
        int    level;      // 1=Beginner, 2=Intermediate, 3=Expert
    };

    struct Experience {
        string company;
        string role;
        int    duration;   // 1=<1thn, 2=1-2thn, 3=3-5thn, 4=5thn+
    };

    int    id;
    string username;      
    string name;         
    string location;
    double salaryExpectation;

    Skill      skills[MAX_SKILLS];
    int        jumlahSkill = 0;

    Experience experiences[MAX_EXPERIENCES];
    int        jumlahExperience = 0;
};

struct User {
    string username;
    string password;
    bool isAdmin;
    bool isBlocked = false;
};

// Enum: FieldPreference
enum FieldPreference {
    ALL,
    IT,
    ECONOMY,
    HEALTHCARE,
    EDUCATION,
    CREATIVE
};

// Helper: enum -> string
inline string fieldPreferenceToString(FieldPreference f) {
    switch (f) {
        case ALL:        return "ALL";
        case IT:         return "IT";
        case ECONOMY:    return "ECONOMY";
        case HEALTHCARE: return "HEALTHCARE";
        case EDUCATION:  return "EDUCATION";
        case CREATIVE:   return "CREATIVE";
        default:         return "ALL";
    }
}

// Helper: string -> enum
inline FieldPreference stringToFieldPreference(const string& s) {
    if (s == "IT")         return IT;
    if (s == "ECONOMY")    return ECONOMY;
    if (s == "HEALTHCARE") return HEALTHCARE;
    if (s == "EDUCATION")  return EDUCATION;
    if (s == "CREATIVE")   return CREATIVE;
    return ALL;
}

// Struct: Lowongan
struct Lowongan {
    int id;
    string title;
    string company;
    string desc;
    string skills;
    string location;
    double salaryExpectation;
    FieldPreference field;
    string postedBy;
};

// Struct: BookmarkedJob

struct BookmarkedJob {
    int bookmarkId;          // Auto-increment ID bookmark
    string username;         // User yang melakukan bookmark
    string source;           // "admin", "scraping", "auto", atau "manual"
    string sourceId;         // ID asli dari sumber (string karena scraping pakai string)
    string title;            // Judul posisi atau nama pekerjaan
    string company;          // Nama perusahaan yang membuka lowongan
    string location;         // Lokasi atau wilayah penempatan kerja
    string salary;           // String — unified (admin: "5000000", scraping: "Rp 6jt - 7jt")
    string desc;             // desc (admin) atau teaser (scraping)
    string skills;           // skills yang dibutuhkan (admin only, kosong untuk scraping)
    string field;            // bidang/field preference (admin only, kosong untuk scraping)
    string url;              // URL ke halaman job (scraping only, kosong untuk admin)
    string postedDate;       // tanggal posting (scraping only)
    string tanggalDitandai;  // timestamp kapan di-bookmark
    int score = 0;           // Skor kecocokan (0-100, 0 = manual/default)
};


// JSON Serialization (nlohmann/json)

// User
inline void to_json(json& j, const User& u) {
    j = json{
        {"username", u.username},
        {"password", u.password},
        {"isAdmin", u.isAdmin},
        {"isBlocked", u.isBlocked}
    };
}

inline void from_json(const json& j, User& u) {
    j.at("username").get_to(u.username);
    j.at("password").get_to(u.password);
    j.at("isAdmin").get_to(u.isAdmin);
    u.isBlocked = j.value("isBlocked", false);
}

// Profile Components (Nested)
inline void to_json(json& j, const Profile::Skill& s) {
    j = json{{"name", s.name}, {"level", s.level}};
}

inline void from_json(const json& j, Profile::Skill& s) {
    j.at("name").get_to(s.name);
    s.level = j.value("level", 1);
}

inline void to_json(json& j, const Profile::Experience& e) {
    j = json{
        {"company",  e.company},
        {"role",     e.role},
        {"duration", e.duration}
    };
}

inline void from_json(const json& j, Profile::Experience& e) {
    j.at("company").get_to(e.company);
    j.at("role").get_to(e.role);
    e.duration = j.value("duration", 1);
}

// Profile
inline void to_json(json& j, const Profile& p) {
    json sArr = json::array();
    for (int i = 0; i < p.jumlahSkill; i++) sArr.push_back(p.skills[i]);

    json eArr = json::array();
    for (int i = 0; i < p.jumlahExperience; i++) eArr.push_back(p.experiences[i]);

    j = json{
        {"id",               p.id},
        {"username",         p.username},
        {"name",             p.name},
        {"location",         p.location},
        {"salaryExpectation",p.salaryExpectation},
        {"skills",           sArr},
        {"experiences",      eArr}
    };
}

inline void from_json(const json& j, Profile& p) {
    j.at("id").get_to(p.id);
    p.username         = j.value("username", string(""));
    p.name             = j.value("name", string(""));
    p.location         = j.value("location", string(""));
    p.salaryExpectation= j.value("salaryExpectation", 0.0);

    p.jumlahSkill = 0;
    if (j.contains("skills") && j.at("skills").is_array()) {
        const json& sArr = j.at("skills");
        for (int i = 0; i < (int)sArr.size() && p.jumlahSkill < MAX_SKILLS; i++) {
            from_json(sArr[i], p.skills[p.jumlahSkill++]);
        }
    }

    p.jumlahExperience = 0;
    if (j.contains("experiences") && j.at("experiences").is_array()) {
        const json& eArr = j.at("experiences");
        for (int i = 0; i < (int)eArr.size() && p.jumlahExperience < MAX_EXPERIENCES; i++) {
            from_json(eArr[i], p.experiences[p.jumlahExperience++]);
        }
    }
}

// Lowongan
inline void to_json(json& j, const Lowongan& l) {
    j = json{
        {"id", l.id},
        {"title", l.title},
        {"company", l.company},
        {"desc", l.desc},
        {"skills", l.skills},
        {"location", l.location},
        {"salaryExpectation", l.salaryExpectation},
        {"field", fieldPreferenceToString(l.field)},
        {"postedBy", l.postedBy}
    };
}

inline void from_json(const json& j, Lowongan& l) {
    j.at("id").get_to(l.id);
    j.at("title").get_to(l.title);
    j.at("company").get_to(l.company);
    j.at("desc").get_to(l.desc);
    j.at("skills").get_to(l.skills);
    j.at("location").get_to(l.location);
    j.at("salaryExpectation").get_to(l.salaryExpectation);
    string fieldStr;
    j.at("field").get_to(fieldStr);
    l.field = stringToFieldPreference(fieldStr);
    l.postedBy = j.value("postedBy", "admin");
}

// BookmarkedJob
inline void to_json(json& j, const BookmarkedJob& b) {
    j = json{
        {"bookmarkId", b.bookmarkId},
        {"username", b.username},
        {"source", b.source},
        {"sourceId", b.sourceId},
        {"title", b.title},
        {"company", b.company},
        {"location", b.location},
        {"salary", b.salary},
        {"desc", b.desc},
        {"skills", b.skills},
        {"field", b.field},
        {"url", b.url},
        {"postedDate", b.postedDate},
        {"tanggalDitandai", b.tanggalDitandai},
        {"score", b.score}
    };
}

inline void from_json(const json& j, BookmarkedJob& b) {
    j.at("bookmarkId").get_to(b.bookmarkId);
    b.username = j.value("username", string("unknown"));
    j.at("source").get_to(b.source);
    j.at("sourceId").get_to(b.sourceId);
    j.at("title").get_to(b.title);
    j.at("company").get_to(b.company);
    j.at("location").get_to(b.location);
    j.at("salary").get_to(b.salary);
    j.at("desc").get_to(b.desc);
    j.at("skills").get_to(b.skills);
    j.at("field").get_to(b.field);
    j.at("url").get_to(b.url);
    j.at("postedDate").get_to(b.postedDate);
    j.at("tanggalDitandai").get_to(b.tanggalDitandai);
    b.score = j.value("score", 0);
}
