#include "LeaderboardController.hpp"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>

LeaderboardController::LeaderboardController() : validator_(std::make_unique<StubScoreValidator>()) {
    init();
}

LeaderboardController::~LeaderboardController() {
    closeSQLite();
}

bool LeaderboardController::init() {
    if (initSQLite()) {
        use_sqlite_ = true;
        return true;
    } else {
        std::cerr << "SQLite init failed, falling back to JSON\n";
        return initJSON();
    }
}

bool LeaderboardController::initSQLite() {
    if (sqlite3_open("saves/leaderboard.db", &db_) != SQLITE_OK) {
        return false;
    }

    const char* createTableSQL = R"(
        CREATE TABLE IF NOT EXISTS leaderboard (
            username TEXT NOT NULL,
            username_norm TEXT NOT NULL,
            difficulty TEXT NOT NULL,
            best_score INTEGER NOT NULL,
            best_date TEXT NOT NULL,
            waves_played INTEGER NOT NULL,
            UNIQUE(username_norm, difficulty)
        );
        CREATE INDEX IF NOT EXISTS idx_score ON leaderboard(best_score DESC);
        CREATE INDEX IF NOT EXISTS idx_date ON leaderboard(best_date DESC);
    )";

    return executeSQL(createTableSQL);
}

void LeaderboardController::closeSQLite() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool LeaderboardController::executeSQL(const std::string& sql) {
    char* errMsg = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << "\n";
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

std::vector<LeaderboardEntry> LeaderboardController::querySQL(const std::string& sql, int limit, int offset) {
    std::vector<LeaderboardEntry> results;
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return results;
    }

    sqlite3_bind_int(stmt, 1, limit);
    sqlite3_bind_int(stmt, 2, offset);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        LeaderboardEntry entry;
        entry.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        entry.difficulty = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        entry.best_score = sqlite3_column_int(stmt, 2);
        entry.best_date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        entry.waves_played = sqlite3_column_int(stmt, 4);
        entry.username_norm = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        results.push_back(entry);
    }

    sqlite3_finalize(stmt);
    return results;
}

int LeaderboardController::countSQL(const std::string& sql) {
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return count;
}

void LeaderboardController::upsertSQL(const std::string& username, const std::string& username_norm, const std::string& difficulty, int score, int waves_played) {
    const char* upsertSQL = R"(
        INSERT INTO leaderboard (username, username_norm, difficulty, best_score, best_date, waves_played)
        VALUES (?, ?, ?, ?, ?, ?)
        ON CONFLICT(username_norm, difficulty) DO UPDATE SET
            username = excluded.username,
            best_score = CASE WHEN excluded.best_score > best_score THEN excluded.best_score ELSE best_score END,
            best_date = CASE WHEN excluded.best_score > best_score THEN excluded.best_date ELSE best_date END,
            waves_played = CASE WHEN excluded.best_score > best_score THEN excluded.waves_played ELSE waves_played END;
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, upsertSQL, -1, &stmt, nullptr) != SQLITE_OK) {
        return;
    }

    std::string date = getCurrentDateTime();
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, username_norm.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, difficulty.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, score);
    sqlite3_bind_text(stmt, 5, date.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 6, waves_played);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    trimOldEntries();
}

bool LeaderboardController::initJSON() {
    std::filesystem::create_directories("saves");
    loadJSON();
    return true;
}

void LeaderboardController::saveJSON() {
    std::ofstream file(json_path_);
    if (file.is_open()) {
        file << json_data_.dump(4);
    }
}

void LeaderboardController::loadJSON() {
    std::ifstream file(json_path_);
    if (file.is_open()) {
        file >> json_data_;
    } else {
        json_data_ = nlohmann::json::array();
    }
}

void LeaderboardController::upsertJSON(const std::string& username, const std::string& username_norm, const std::string& difficulty, int score, int waves_played) {
    std::string date = getCurrentDateTime();
    bool updated = false;

    for (auto& entry : json_data_) {
        if (entry["username_norm"] == username_norm && entry["difficulty"] == difficulty) {
            if (score > entry["best_score"]) {
                entry["username"] = username;
                entry["best_score"] = score;
                entry["best_date"] = date;
                entry["waves_played"] = waves_played;
            }
            updated = true;
            break;
        }
    }

    if (!updated) {
        nlohmann::json newEntry = {
            {"username", username},
            {"username_norm", username_norm},
            {"difficulty", difficulty},
            {"best_score", score},
            {"best_date", date},
            {"waves_played", waves_played}
        };
        json_data_.push_back(newEntry);
    }

    trimOldEntries();
    saveJSON();
}

std::vector<LeaderboardEntry> LeaderboardController::queryJSON(const std::string& filter, int limit, int offset, SortBy sort) {
    std::vector<LeaderboardEntry> results;

    for (const auto& entry : json_data_) {
        if (filter.empty() || entry["username_norm"].get<std::string>().find(filter) != std::string::npos) {
            LeaderboardEntry e;
            e.username = entry["username"];
            e.difficulty = entry["difficulty"];
            e.best_score = entry["best_score"];
            e.best_date = entry["best_date"];
            e.waves_played = entry["waves_played"];
            e.username_norm = entry["username_norm"];
            results.push_back(e);
        }
    }

    // Sort
    if (sort == SortBy::ScoreDesc) {
        std::sort(results.begin(), results.end(), [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
            if (a.best_score != b.best_score) return a.best_score > b.best_score;
            return a.best_date > b.best_date;
        });
    } else {
        std::sort(results.begin(), results.end(), [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
            return a.best_date > b.best_date;
        });
    }

    // Paginate
    if (offset >= (int)results.size()) return {};
    int end = std::min((int)results.size(), offset + limit);
    return std::vector<LeaderboardEntry>(results.begin() + offset, results.begin() + end);
}

int LeaderboardController::countJSON(const std::string& filter) {
    int count = 0;
    for (const auto& entry : json_data_) {
        if (filter.empty() || entry["username_norm"].get<std::string>().find(filter) != std::string::npos) {
            count++;
        }
    }
    return count;
}

void LeaderboardController::trimOldEntries() {
    if (use_sqlite_) {
        // SQLite: delete oldest entries per difficulty if > 1000
        const char* trimSQL = R"(
            DELETE FROM leaderboard
            WHERE rowid IN (
                SELECT rowid FROM leaderboard
                WHERE difficulty = ?
                ORDER BY best_date ASC
                LIMIT (SELECT MAX(0, COUNT(*) - 1000) FROM leaderboard WHERE difficulty = ?)
            );
        )";
        for (const std::string& diff : {"Easy", "Normal", "Hard", "Legendary"}) {
            sqlite3_stmt* stmt;
            if (sqlite3_prepare_v2(db_, trimSQL, -1, &stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, diff.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, diff.c_str(), -1, SQLITE_STATIC);
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);
            }
        }
    } else {
        // JSON: group by difficulty, sort by date, keep top 1000
        std::map<std::string, std::vector<nlohmann::json>> byDiff;
        for (const auto& entry : json_data_) {
            byDiff[entry["difficulty"]].push_back(entry);
        }

        json_data_.clear();
        for (auto& [diff, entries] : byDiff) {
            std::sort(entries.begin(), entries.end(), [](const nlohmann::json& a, const nlohmann::json& b) {
                return a["best_date"] > b["best_date"];
            });
            if (entries.size() > 1000) {
                entries.resize(1000);
            }
            for (const auto& entry : entries) {
                json_data_.push_back(entry);
            }
        }
        saveJSON();
    }
}

std::string LeaderboardController::getCurrentDateTime() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&time_t);
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// Username validation
bool LeaderboardController::isValidUsername(const std::string& s) {
    if (s.length() < 3 || s.length() > 16) return false;
    std::regex pattern("^[A-Za-z0-9_-]+$");
    if (!std::regex_match(s, pattern)) return false;
    return !hasProfanity(s);
}

std::string LeaderboardController::normalizeUsername(const std::string& s) {
    std::string normalized;
    for (char c : s) {
        if (std::isalnum(c) || c == '_' || c == '-') {
            normalized += std::tolower(c);
        }
    }
    // Collapse multiple _ or -
    std::string result;
    for (size_t i = 0; i < normalized.size(); ++i) {
        if (i == 0 || (!(normalized[i] == '_' && normalized[i-1] == '_') &&
            !(normalized[i] == '-' && normalized[i-1] == '-'))) {
            result += normalized[i];
        }
    }
    return result;
}

bool LeaderboardController::hasProfanity(const std::string& s) {
    // Stub: no profanity check
    return false;
}

// Upsert
void LeaderboardController::upsertBest(const std::string& username, const std::string& difficulty, int score, int waves_played) {
    if (!validator_->validateScore(score, username, difficulty)) return;

    std::string username_norm = normalizeUsername(username);
    if (use_sqlite_) {
        upsertSQL(username, username_norm, difficulty, score, waves_played);
    } else {
        upsertJSON(username, username_norm, difficulty, score, waves_played);
    }
}

// Queries
QueryResult LeaderboardController::queryTopGlobal(int limit, int page, SortBy sort) {
    int offset = (page - 1) * limit;
    std::vector<LeaderboardEntry> entries;
    int total = 0;

    if (use_sqlite_) {
        std::string order = (sort == SortBy::ScoreDesc) ? "best_score DESC, best_date DESC" : "best_date DESC";
        std::string sql = "SELECT username, difficulty, best_score, best_date, waves_played, username_norm FROM leaderboard ORDER BY " + order + " LIMIT ? OFFSET ?";
        entries = querySQL(sql, limit, offset);
        sql = "SELECT COUNT(*) FROM leaderboard";
        total = countSQL(sql);
    } else {
        entries = queryJSON("", limit, offset, sort);
        total = countJSON("");
    }

    int totalPages = (total + limit - 1) / limit;
    return {entries, totalPages, page, limit};
}

QueryResult LeaderboardController::queryTopByDifficulty(const std::string& difficulty, int limit, int page, SortBy sort) {
    int offset = (page - 1) * limit;
    std::vector<LeaderboardEntry> entries;
    int total = 0;

    if (use_sqlite_) {
        std::string order = (sort == SortBy::ScoreDesc) ? "best_score DESC, best_date DESC" : "best_date DESC";
        std::string sql = "SELECT username, difficulty, best_score, best_date, waves_played, username_norm FROM leaderboard WHERE difficulty = ? ORDER BY " + order + " LIMIT ? OFFSET ?";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, difficulty.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 2, limit);
            sqlite3_bind_int(stmt, 3, offset);

            while (sqlite3_step(stmt) == SQLITE_ROW) {
                LeaderboardEntry entry;
                entry.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                entry.difficulty = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                entry.best_score = sqlite3_column_int(stmt, 2);
                entry.best_date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
                entry.waves_played = sqlite3_column_int(stmt, 4);
                entry.username_norm = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
                entries.push_back(entry);
            }
            sqlite3_finalize(stmt);
        }
        sql = "SELECT COUNT(*) FROM leaderboard WHERE difficulty = ?";
        if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, difficulty.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                total = sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }
    } else {
        std::string filter = difficulty;
        entries = queryJSON(filter, limit, offset, sort);
        total = countJSON(filter);
    }

    int totalPages = (total + limit - 1) / limit;
    return {entries, totalPages, page, limit};
}

QueryResult LeaderboardController::queryToday(int limit, int page, SortBy sort) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&time_t);
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d");
    std::string today = ss.str();

    int offset = (page - 1) * limit;
    std::vector<LeaderboardEntry> entries;
    int total = 0;

    if (use_sqlite_) {
        std::string order = (sort == SortBy::ScoreDesc) ? "best_score DESC, best_date DESC" : "best_date DESC";
        std::string sql = "SELECT username, difficulty, best_score, best_date, waves_played, username_norm FROM leaderboard WHERE best_date LIKE ? ORDER BY " + order + " LIMIT ? OFFSET ?";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            std::string like = today + "%";
            sqlite3_bind_text(stmt, 1, like.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 2, limit);
            sqlite3_bind_int(stmt, 3, offset);

            while (sqlite3_step(stmt) == SQLITE_ROW) {
                LeaderboardEntry entry;
                entry.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                entry.difficulty = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                entry.best_score = sqlite3_column_int(stmt, 2);
                entry.best_date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
                entry.waves_played = sqlite3_column_int(stmt, 4);
                entry.username_norm = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
                entries.push_back(entry);
            }
            sqlite3_finalize(stmt);
        }
        sql = "SELECT COUNT(*) FROM leaderboard WHERE best_date LIKE ?";
        if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            std::string like = today + "%";
            sqlite3_bind_text(stmt, 1, like.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                total = sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }
    } else {
        // JSON: filter by date starting with today
        entries = queryJSON(today, limit, offset, sort);
        total = countJSON(today);
    }

    int totalPages = (total + limit - 1) / limit;
    return {entries, totalPages, page, limit};
}

QueryResult LeaderboardController::searchByUsername(const std::string& partial, int limit, int page, SortBy sort) {
    std::string partial_norm = normalizeUsername(partial);
    int offset = (page - 1) * limit;
    std::vector<LeaderboardEntry> entries;
    int total = 0;

    if (use_sqlite_) {
        std::string order = (sort == SortBy::ScoreDesc) ? "best_score DESC, best_date DESC" : "best_date DESC";
        std::string sql = "SELECT username, difficulty, best_score, best_date, waves_played, username_norm FROM leaderboard WHERE username_norm LIKE ? ORDER BY " + order + " LIMIT ? OFFSET ?";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            std::string like = "%" + partial_norm + "%";
            sqlite3_bind_text(stmt, 1, like.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 2, limit);
            sqlite3_bind_int(stmt, 3, offset);

            while (sqlite3_step(stmt) == SQLITE_ROW) {
                LeaderboardEntry entry;
                entry.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                entry.difficulty = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                entry.best_score = sqlite3_column_int(stmt, 2);
                entry.best_date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
                entry.waves_played = sqlite3_column_int(stmt, 4);
                entry.username_norm = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
                entries.push_back(entry);
            }
            sqlite3_finalize(stmt);
        }
        sql = "SELECT COUNT(*) FROM leaderboard WHERE username_norm LIKE ?";
        if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            std::string like = "%" + partial_norm + "%";
            sqlite3_bind_text(stmt, 1, like.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                total = sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }
    } else {
        entries = queryJSON(partial_norm, limit, offset, sort);
        total = countJSON(partial_norm);
    }

    int totalPages = (total + limit - 1) / limit;
    return {entries, totalPages, page, limit};
}

// Export/Import
nlohmann::json LeaderboardController::exportToJson() {
    if (use_sqlite_) {
        // Export from SQLite to JSON
        nlohmann::json j = nlohmann::json::array();
        std::string sql = "SELECT username, username_norm, difficulty, best_score, best_date, waves_played FROM leaderboard";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                nlohmann::json entry;
                entry["username"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                entry["username_norm"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                entry["difficulty"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
                entry["best_score"] = sqlite3_column_int(stmt, 3);
                entry["best_date"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
                entry["waves_played"] = sqlite3_column_int(stmt, 5);
                j.push_back(entry);
            }
            sqlite3_finalize(stmt);
        }
        return j;
    } else {
        return json_data_;
    }
}

bool LeaderboardController::importFromJson(const nlohmann::json& j) {
    if (!j.is_array()) return false;

    if (use_sqlite_) {
        // Clear table
        executeSQL("DELETE FROM leaderboard");
        // Insert from JSON
        for (const auto& entry : j) {
            upsertSQL(entry["username"], entry["username_norm"], entry["difficulty"], entry["best_score"], entry["waves_played"]);
        }
    } else {
        json_data_ = j;
        saveJSON();
    }
    return true;
}

void LeaderboardController::clearData() {
    if (use_sqlite_) {
        executeSQL("DELETE FROM leaderboard");
    } else {
        json_data_ = nlohmann::json::array();
        saveJSON();
    }
}

void LeaderboardController::setScoreValidator(std::unique_ptr<IScoreValidator> validator) {
    validator_ = std::move(validator);
}
