#include <httplib.h>
#include "api/api_server.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <thread>
#include <atomic>
#include <string>
#include <iostream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

using json = nlohmann::json;
namespace fs = std::filesystem;

// Globals
static httplib::Server*  g_svr = nullptr;
static std::thread       g_thread;
static std::atomic<bool> g_running{false};
static std::string       g_sessionUser;
static std::mutex        g_serverMutex;
static std::mutex        g_fileMutex; // Memastikan operasi file thread-safe

const std::string BOOKMARKS_FILE = "data/bookmarked_jobs.json";

const std::string SCRAPE_FILES[] = {
    "data/scrape/hasil_jobstreet.json",
    "data/scrape/hasil_kitalulus.json",
    "data/scrape/hasil_lokerid.json"
};

// Helpers
static void setCorsHeaders(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

static std::string getCurrentDate() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d");
    return ss.str();
}

static std::string formatSalary(long long sMin, long long sMax) {
    if (sMin > 0 && sMax > 0) return std::to_string(sMin) + " - " + std::to_string(sMax);
    if (sMax > 0) return "< " + std::to_string(sMax);
    if (sMin > 0) return "> " + std::to_string(sMin);
    return "N/A";
}

// Core Logic
static json readAllScrapeData(const std::string& filterSource = "") {
    json merged = json::array();
    
    for (const auto& filePath : SCRAPE_FILES) {
        if (!fs::exists(filePath)) continue;
        
        try {
            std::ifstream f(filePath);
            if (!f.is_open()) continue;
            
            json arr;
            f >> arr;
            
            if (!arr.is_array()) continue;
            
            for (auto& item : arr) {
                if (!filterSource.empty()) {
                    std::string src = item.value("source", "");
                    std::string srcLower = src;
                    std::string filterLower = filterSource;
                    
                    for (auto &c : srcLower) c = tolower(c);
                    for (auto &c : filterLower) c = tolower(c);
                    
                    if (srcLower != filterLower) continue;
                }
                merged.push_back(item);
            }
        } catch (const json::parse_error& e) {
            std::cerr << "[Error] JSON parse error in " << filePath << ": " << e.what() << "\n";
        } catch (const std::exception& e) {
            std::cerr << "[Error] Failed reading " << filePath << ": " << e.what() << "\n";
        }
    }
    return merged;
}

static json readBookmarks() {
    json bookmarks = json::array();
    if (fs::exists(BOOKMARKS_FILE)) {
        try {
            std::ifstream f(BOOKMARKS_FILE);
            if (f.is_open()) {
                f >> bookmarks;
            }
        } catch (const std::exception& e) {
            std::cerr << "[Error] Failed to read bookmarks: " << e.what() << "\n";
            bookmarks = json::array();
        }
    }
    return bookmarks;
}

static bool saveBookmarks(const json& bookmarks) {
    try {
        std::ofstream out(BOOKMARKS_FILE);
        if (!out.is_open()) return false;
        out << bookmarks.dump(4);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[Error] Failed to save bookmarks: " << e.what() << "\n";
        return false;
    }
}

static json findJobById(const std::string& jobId) {
    // Mencari job berdasarkan ID dan langsung me-return ketika ketemu (Optimasi Performa)
    for (const auto& filePath : SCRAPE_FILES) {
        if (!fs::exists(filePath)) continue;
        
        try {
            std::ifstream f(filePath);
            if (!f.is_open()) continue;
            
            json arr;
            f >> arr;
            
            if (!arr.is_array()) continue;
            
            for (const auto& job : arr) {
                std::string currentId = "";
                if (job.contains("id") && !job["id"].is_null()) {
                    if (job["id"].is_string()) currentId = job["id"].get<std::string>();
                    else currentId = std::to_string(job["id"].get<int>());
                }
                
                if (currentId == jobId) {
                    return job; 
                }
            }
        } catch (...) {
            continue;
        }
    }
    return json();
}

static int calculateNextId(const json& bookmarks) {
    int nextId = 1;
    for (const auto& b : bookmarks) {
        int bid = b.value("bookmarkId", 0);
        if (bid >= nextId) nextId = bid + 1;
    }
    return nextId;
}

static bool addBookmark(const std::string& username, const std::string& jobId) {
    std::lock_guard<std::mutex> lock(g_fileMutex); // Thread-safe lock untuk file
    
    json bookmarks = readBookmarks();
    
    // Cek duplikat
    for (const auto& b : bookmarks) {
        if (b.value("sourceId", "") == jobId && b.value("username", "") == username) {
            return true; 
        }
    }
    
    // Ambil data job
    json job = findJobById(jobId);
    if (job.empty()) {
        std::cerr << "[Warning] Job ID " << jobId << " not found for bookmarking.\n";
        return false;
    }
    
    int newId = calculateNextId(bookmarks);
    
    auto safeString = [&](const std::string& key, const std::string& def) {
        return (job.contains(key) && job[key].is_string()) ? job[key].get<std::string>() : def;
    };
    
    auto safeNumber = [&](const std::string& key, long long def) {
        return (job.contains(key) && job[key].is_number()) ? job[key].get<long long>() : def;
    };
    
    long long sMin = safeNumber("salary_min", 0LL);
    long long sMax = safeNumber("salary_max", 0LL);
    
    json bm;
    bm["bookmarkId"]      = newId;
    bm["username"]        = username;
    bm["source"]          = safeString("source", "scraping");
    bm["sourceId"]        = jobId;
    bm["title"]           = safeString("title", "N/A");
    bm["company"]         = safeString("company", "N/A");
    bm["location"]        = safeString("location", "N/A");
    bm["salary"]          = formatSalary(sMin, sMax);
    bm["desc"]            = safeString("description", "N/A");
    bm["skills"]          = "";
    bm["field"]           = safeString("field", "N/A");
    bm["url"]             = safeString("url", "");
    bm["postedDate"]      = safeString("scraped_at", "");
    bm["tanggalDitandai"] = getCurrentDate();
    bm["score"]           = 0;
    
    bookmarks.push_back(bm);
    
    return saveBookmarks(bookmarks);
}

static bool removeBookmark(const std::string& username, const std::string& jobId) {
    std::lock_guard<std::mutex> lock(g_fileMutex);
    
    json bookmarks = readBookmarks();
    json newBookmarks = json::array();
    bool found = false;
    
    for (const auto& b : bookmarks) {
        if (b.value("sourceId", "") == jobId && b.value("username", "") == username) {
            found = true;
            continue; // Skip this one to remove it
        }
        newBookmarks.push_back(b);
    }
    
    if (!found) return true; // Already gone or never existed
    return saveBookmarks(newBookmarks);
}

// Public API

void startApiServerAsync(const std::string& username) {
    std::lock_guard<std::mutex> lock(g_serverMutex);
    
    if (g_running.load()) return;
    g_running.store(true);
    g_sessionUser = username;

    g_thread = std::thread([]() {
        g_svr = new httplib::Server();

        g_svr->Options(R"(/api/.*)", [](const httplib::Request&, httplib::Response& res) {
            setCorsHeaders(res);
            res.status = 200;
        });

        g_svr->Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
            setCorsHeaders(res);
            res.set_content(R"({"status":"ok"})", "application/json");
        });

        g_svr->Get("/api/session", [](const httplib::Request&, httplib::Response& res) {
            setCorsHeaders(res);
            
            std::string currentUser;
            {
                std::lock_guard<std::mutex> lock(g_serverMutex);
                currentUser = g_sessionUser;
            }
            
            json j = {{"username", currentUser}};
            res.set_content(j.dump(), "application/json");
        });

        g_svr->Get("/api/scrape", [](const httplib::Request& req, httplib::Response& res) {
            setCorsHeaders(res);
            std::string src = req.get_param_value("source");
            json data = readAllScrapeData(src);
            res.set_content(data.dump(), "application/json");
        });

        g_svr->Get("/api/bookmarks", [](const httplib::Request&, httplib::Response& res) {
            setCorsHeaders(res);
            
            std::string currentUser;
            {
                std::lock_guard<std::mutex> lock(g_serverMutex);
                currentUser = g_sessionUser;
            }
            
            json allBookmarks = readBookmarks();
            json userBookmarks = json::array();
            
            for (const auto& b : allBookmarks) {
                if (b.value("username", "") == currentUser) {
                    userBookmarks.push_back(b);
                }
            }
            
            res.set_content(userBookmarks.dump(), "application/json");
        });

        g_svr->Post("/api/bookmark", [](const httplib::Request& req, httplib::Response& res) {
            setCorsHeaders(res);
            try {
                json body = json::parse(req.body);
                
                std::string currentUser;
                {
                    std::lock_guard<std::mutex> lock(g_serverMutex);
                    currentUser = g_sessionUser;
                }
                
                std::string user  = body.value("username", currentUser);
                
                std::string jobId = "";
                if (body.contains("jobId")) {
                    if (body["jobId"].is_string()) {
                        jobId = body["jobId"].get<std::string>();
                    } else if (body["jobId"].is_number()) {
                        jobId = std::to_string(body["jobId"].get<int>());
                    }
                }

                if (jobId.empty()) {
                    res.status = 400;
                    res.set_content(R"({"status":"error","message":"jobId diperlukan"})", "application/json");
                    return;
                }

                bool ok = addBookmark(user, jobId);
                json result = {{"status", ok ? "success" : "error"}};
                res.status = ok ? 200 : 500;
                res.set_content(result.dump(), "application/json");
                
            } catch (const json::parse_error& e) {
                std::cerr << "[Error] Invalid JSON in /api/bookmark: " << e.what() << "\n";
                res.status = 400;
                res.set_content(R"({"status":"error","message":"Invalid JSON format"})", "application/json");
            } catch (const std::exception& e) {
                std::cerr << "[Error] Server Exception in /api/bookmark: " << e.what() << "\n";
                res.status = 500;
                res.set_content(R"({"status":"error"})", "application/json");
            }
        });

        g_svr->Delete("/api/bookmark", [](const httplib::Request& req, httplib::Response& res) {
            setCorsHeaders(res);
            try {
                json body = json::parse(req.body);
                
                std::string currentUser;
                {
                    std::lock_guard<std::mutex> lock(g_serverMutex);
                    currentUser = g_sessionUser;
                }
                
                std::string user  = body.value("username", currentUser);
                
                std::string jobId = "";
                if (body.contains("jobId")) {
                    if (body["jobId"].is_string()) {
                        jobId = body["jobId"].get<std::string>();
                    } else if (body["jobId"].is_number()) {
                        jobId = std::to_string(body["jobId"].get<int>());
                    }
                }

                if (jobId.empty()) {
                    res.status = 400;
                    res.set_content(R"({"status":"error","message":"jobId diperlukan"})", "application/json");
                    return;
                }

                bool ok = removeBookmark(user, jobId);
                json result = {{"status", ok ? "success" : "error"}};
                res.status = ok ? 200 : 500;
                res.set_content(result.dump(), "application/json");
                
            } catch (const std::exception& e) {
                std::cerr << "[Error] Server Exception in DELETE /api/bookmark: " << e.what() << "\n";
                res.status = 500;
                res.set_content(R"({"status":"error"})", "application/json");
            }
        });

        if (!g_svr->listen("0.0.0.0", 8080)) {
            std::cerr << "[Error] Failed to start API server on port 8080\n";
            g_running.store(false);
        }
    });
    
    g_thread.detach();
}

void stopApiServer() {
    std::lock_guard<std::mutex> lock(g_serverMutex);
    if (g_svr && g_running.load()) {
        g_svr->stop();
        delete g_svr;
        g_svr = nullptr;
        g_running.store(false);
    }
}

bool isApiServerRunning() {
    return g_running.load();
}
