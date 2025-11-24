#pragma once
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <nlohmann/json.hpp>
#include <sqlite3.h>

// Forward declarations
class IScoreValidator;

// Data structure for leaderboard entries
struct LeaderboardEntry {
    std::string username;      // Original casing
    std::string difficulty;    // "Easy", "Normal", "Hard", "Legendary"
    int best_score;
    std::string best_date;     // ISO format YYYY-MM-DD HH:MM:SS
    int waves_played;
    std::string username_norm; // Normalized for uniqueness
};

// Query result with pagination
struct QueryResult {
    std::vector<LeaderboardEntry> entries;
    int total_pages;
    int current_page;
    int page_size;
};

// Sorting options
enum class SortBy { ScoreDesc, DateDesc };

// Anti-cheat interface (stub)
class IScoreValidator {
public:
    virtual ~IScoreValidator() = default;
    virtual bool validateScore(int score, const std::string& username, const std::string& difficulty) = 0;
};

// Stub implementation
class StubScoreValidator : public IScoreValidator {
public:
    bool validateScore(int /*score*/, const std::string& /*username*/, const std::string& /*difficulty*/) override {
        return true; // Always valid
    }
};

class LeaderboardController {
public:
    LeaderboardController();
    ~LeaderboardController();

    // Username validation
    static bool isValidUsername(const std::string& s);
    static std::string normalizeUsername(const std::string& s);
    static bool hasProfanity(const std::string& s); // Stub

    // Persistence
    bool init(); // Try SQLite, fallback to JSON
    void upsertBest(const std::string& username, const std::string& difficulty, int score, int waves_played);
    // Insert or update a score while preserving an explicit date (used when loading from legacy files)
    void upsertBestWithDate(const std::string& username, const std::string& difficulty, int score, const std::string& date, int waves_played);
    QueryResult queryTopGlobal(int limit, int page = 1, SortBy sort = SortBy::ScoreDesc);
    QueryResult queryTopByDifficulty(const std::string& difficulty, int limit, int page = 1, SortBy sort = SortBy::ScoreDesc);
    QueryResult queryToday(int limit, int page = 1, SortBy sort = SortBy::ScoreDesc);
    QueryResult searchByUsername(const std::string& partial, int limit, int page = 1, SortBy sort = SortBy::ScoreDesc);

    // Export/Import
    nlohmann::json exportToJson();
    bool importFromJson(const nlohmann::json& j);

    // Clear data
    void clearData();

    // Anti-cheat
    void setScoreValidator(std::unique_ptr<IScoreValidator> validator);

private:
    sqlite3* db_ = nullptr;
    bool use_sqlite_ = true;
    std::unique_ptr<IScoreValidator> validator_;

    // SQLite helpers
    bool initSQLite();
    void closeSQLite();
    bool executeSQL(const std::string& sql);
    std::vector<LeaderboardEntry> querySQL(const std::string& sql, int limit, int offset);
    int countSQL(const std::string& sql);
    void upsertSQL(const std::string& username, const std::string& username_norm, const std::string& difficulty, int score, int waves_played);

    // JSON fallback
    std::string json_path_ = "saves/leaderboard.json";
    nlohmann::json json_data_;
    bool initJSON();
    void saveJSON();
    void loadJSON();
    void upsertJSON(const std::string& username, const std::string& username_norm, const std::string& difficulty, int score, int waves_played);
    std::vector<LeaderboardEntry> queryJSON(const std::string& filter, int limit, int offset, SortBy sort);
    int countJSON(const std::string& filter);

    // Helpers
    std::string getCurrentDateTime();
    void trimOldEntries(); // Keep max 1000 per difficulty, trim oldest
};