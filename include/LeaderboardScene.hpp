#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <vector>
#include "LeaderboardController.hpp"

class LeaderboardScene {
public:
    explicit LeaderboardScene(sf::RenderWindow& win);
    void handleInput(bool mousePressedLeft, bool mouseReleasedLeft, bool mouseMoved);
    void update(float dt);
    void draw();
    bool shouldReturnToMenu() const { return returnToMenu_; }

private:
    sf::RenderWindow& win_;

    // Assets
    sf::Font font_;
    sf::Texture bgTex_;
    std::optional<sf::Sprite> bgSprite_;
    std::optional<sf::RectangleShape> panelShape_;

    // UI Elements
    std::optional<sf::Text> title_;
    std::optional<sf::Text> trophyText_;
    std::vector<std::optional<sf::Text>> tabTexts_;
    std::optional<sf::Text> headerText_;
    std::vector<std::optional<sf::Text>> listTexts_;
    std::optional<sf::Text> footerText_;
    std::optional<sf::Text> searchLabel_;
    std::optional<sf::Text> searchInput_;
    std::optional<sf::Text> pageInfo_;
    std::vector<std::optional<sf::Text>> navButtons_; // Prev, Next

    // State
    enum class Tab { Global, ByDifficulty, Today };
    Tab currentTab_ = Tab::Global;
    std::string currentDifficulty_ = "Normal";
    SortBy currentSort_ = SortBy::ScoreDesc;
    std::string searchQuery_;
    int currentPage_ = 1;
    int pageSize_ = 10;
    bool returnToMenu_ = false;
    bool isSearching_ = false;

    // Data
    LeaderboardController leaderboard_;
    QueryResult currentResults_;

    // Helpers
    void loadAssets();
    void setupUI();
    void updateResults();
    void loadLeaderboardFromFile();
    void drawTabs();
    void drawList();
    void drawPagination();
    void handleTabClick(sf::Vector2f pos);
    void handleNavClick(sf::Vector2f pos);
    void handleSearchClick(sf::Vector2f pos);
    sf::FloatRect getTabBounds(int index);
    sf::FloatRect getNavBounds(int index);
    sf::FloatRect getSearchBounds();
};
