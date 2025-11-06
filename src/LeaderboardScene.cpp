#include "LeaderboardScene.hpp"
#include <algorithm>
#include <iostream>

LeaderboardScene::LeaderboardScene(sf::RenderWindow& win) : win_(win), leaderboard_() {
    loadAssets();
    setupUI();
    updateResults();
}

void LeaderboardScene::loadAssets() {
    font_.loadFromFile("../assets/fonts/PressStart2P-Regular.ttf");
    bgTex_.loadFromFile("../assets/images/background.png");
    panelTex_.loadFromFile("../assets/images/first-bg.png");

    bgSprite_.emplace(bgTex_);
    panelSprite_.emplace(panelTex_);

    // Scale background to cover
    const auto* tex = bgSprite_->getTexture();
    const float TW = static_cast<float>(tex->getSize().x);
    const float TH = static_cast<float>(tex->getSize().y);
    const float W = static_cast<float>(win_.getSize().x);
    const float H = static_cast<float>(win_.getSize().y);
    const float s = std::max(W / TW, H / TH);
    bgSprite_->setScale(sf::Vector2f(s, s));

    // Scale panel
    sf::Vector2f targetSize{800.f, 600.f};
    sf::Vector2u texSize = panelTex_.getSize();
    float scaleX = targetSize.x / static_cast<float>(texSize.x);
    float scaleY = targetSize.y / static_cast<float>(texSize.y);
    float scale = std::min(scaleX, scaleY);
    panelSprite_->setScale(sf::Vector2f(scale, scale));

    // Center panel
    sf::Vector2f panelPos{
        (W - targetSize.x) * 0.5f,
        (H - targetSize.y) * 0.5f
    };
    panelSprite_->setPosition(panelPos);
}

void LeaderboardScene::setupUI() {
    sf::Vector2f panelPos = panelSprite_->getPosition();
    sf::Vector2f panelSize = panelSprite_->getGlobalBounds().getSize();

    // Title
    title_.emplace("LEADERBOARD", font_, 24);
    title_->setFillColor(sf::Color::White);
    sf::FloatRect titleBounds = title_->getLocalBounds();
    title_->setOrigin(titleBounds.left + titleBounds.width / 2.0f, titleBounds.top);
    title_->setPosition(panelPos.x + panelSize.x / 2.0f, panelPos.y + 20.f);

    // Tabs
    std::vector<std::string> tabNames = {"Global", "By Difficulty", "Today"};
    tabTexts_.resize(tabNames.size());
    for (size_t i = 0; i < tabNames.size(); ++i) {
        tabTexts_[i].emplace(tabNames[i], font_, 16);
        tabTexts_[i]->setFillColor(sf::Color::White);
        tabTexts_[i]->setPosition(panelPos.x + 50.f + i * 200.f, panelPos.y + 70.f);
    }

    // Header
    headerText_.emplace("Rank  Username  Score  Difficulty  Date", font_, 14);
    headerText_->setFillColor(sf::Color::Yellow);
    headerText_->setPosition(panelPos.x + 50.f, panelPos.y + 110.f);

    // List (placeholder, will be updated)
    listTexts_.resize(pageSize_);
    for (int i = 0; i < pageSize_; ++i) {
        listTexts_[i].emplace("", font_, 12);
        listTexts_[i]->setFillColor(sf::Color::White);
        listTexts_[i]->setPosition(panelPos.x + 50.f, panelPos.y + 140.f + i * 20.f);
    }

    // Footer
    footerText_.emplace("Press ESC to return", font_, 12);
    footerText_->setFillColor(sf::Color::White);
    footerText_->setPosition(panelPos.x + 50.f, panelPos.y + panelSize.y - 30.f);

    // Search
    searchLabel_.emplace("Search:", font_, 14);
    searchLabel_->setFillColor(sf::Color::White);
    searchLabel_->setPosition(panelPos.x + 50.f, panelPos.y + panelSize.y - 80.f);

    searchInput_.emplace("", font_, 14);
    searchInput_->setFillColor(sf::Color::Cyan);
    searchInput_->setPosition(panelPos.x + 150.f, panelPos.y + panelSize.y - 80.f);

    // Page info
    pageInfo_.emplace("Page 1 of 1", font_, 12);
    pageInfo_->setFillColor(sf::Color::White);
    pageInfo_->setPosition(panelPos.x + panelSize.x - 200.f, panelPos.y + panelSize.y - 80.f);

    // Nav buttons
    navButtons_.resize(2);
    navButtons_[0].emplace("Prev", font_, 14);
    navButtons_[0]->setFillColor(sf::Color::White);
    navButtons_[0]->setPosition(panelPos.x + panelSize.x - 150.f, panelPos.y + panelSize.y - 50.f);

    navButtons_[1].emplace("Next", font_, 14);
    navButtons_[1]->setFillColor(sf::Color::White);
    navButtons_[1]->setPosition(panelPos.x + panelSize.x - 80.f, panelPos.y + panelSize.y - 50.f);
}

void LeaderboardScene::updateResults() {
    switch (currentTab_) {
        case Tab::Global:
            currentResults_ = leaderboard_.queryTopGlobal(pageSize_, currentPage_, currentSort_);
            break;
        case Tab::ByDifficulty:
            currentResults_ = leaderboard_.queryTopByDifficulty(currentDifficulty_, pageSize_, currentPage_, currentSort_);
            break;
        case Tab::Today:
            currentResults_ = leaderboard_.queryToday(pageSize_, currentPage_, currentSort_);
            break;
    }

    // Update page info
    if (pageInfo_) {
        pageInfo_->setString("Page " + std::to_string(currentResults_.current_page) + " of " + std::to_string(currentResults_.total_pages));
    }

    // Update list
    for (size_t i = 0; i < listTexts_.size(); ++i) {
        if (i < currentResults_.entries.size()) {
            const auto& entry = currentResults_.entries[i];
            int rank = (currentPage_ - 1) * pageSize_ + i + 1;
            std::string text = std::to_string(rank) + ". " + entry.username + " " +
                               std::to_string(entry.best_score) + " " + entry.difficulty + " " + entry.best_date;
            listTexts_[i]->setString(text);
        } else {
            listTexts_[i]->setString("");
        }
    }
}

void LeaderboardScene::handleInput(bool /*mousePressedLeft*/, bool mouseReleasedLeft, bool /*mouseMoved*/) {
    if (!mouseReleasedLeft) return;

    sf::Vector2i mousePos = sf::Mouse::getPosition(win_);
    sf::Vector2f worldPos = win_.mapPixelToCoords(mousePos);

    // ESC to return
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Escape)) {
        returnToMenu_ = true;
        return;
    }

    // Tab clicks
    for (size_t i = 0; i < tabTexts_.size(); ++i) {
        if (tabTexts_[i] && tabTexts_[i]->getGlobalBounds().contains(worldPos)) {
            currentTab_ = static_cast<Tab>(i);
            currentPage_ = 1;
            updateResults();
            return;
        }
    }

    // Nav clicks
    for (size_t i = 0; i < navButtons_.size(); ++i) {
        if (navButtons_[i] && navButtons_[i]->getGlobalBounds().contains(worldPos)) {
            if (i == 0 && currentPage_ > 1) { // Prev
                currentPage_--;
                updateResults();
            } else if (i == 1 && currentPage_ < currentResults_.total_pages) { // Next
                currentPage_++;
                updateResults();
            }
            return;
        }
    }

    // Search click (toggle search mode)
    if (searchInput_ && searchInput_->getGlobalBounds().contains(worldPos)) {
        isSearching_ = !isSearching_;
        if (isSearching_) {
            searchInput_->setFillColor(sf::Color::Yellow);
        } else {
            searchInput_->setFillColor(sf::Color::Cyan);
            searchQuery_.clear();
            searchInput_->setString("");
            currentPage_ = 1;
            updateResults();
        }
    }
}

void LeaderboardScene::update(float /*dt*/) {
    // Handle text input for search
    if (isSearching_) {
        // Note: Text input handling would need to be added, but for simplicity, assume search is set externally or via other means
        // In a full implementation, you'd handle sf::Event::TextEntered here
    }
}

void LeaderboardScene::draw() {
    if (bgSprite_) win_.draw(*bgSprite_);
    if (panelSprite_) win_.draw(*panelSprite_);
    if (title_) win_.draw(*title_);

    // Tabs
    for (auto& tab : tabTexts_) {
        if (tab) win_.draw(*tab);
    }

    if (headerText_) win_.draw(*headerText_);

    // List
    for (auto& item : listTexts_) {
        if (item) win_.draw(*item);
    }

    if (footerText_) win_.draw(*footerText_);
    if (searchLabel_) win_.draw(*searchLabel_);
    if (searchInput_) win_.draw(*searchInput_);
    if (pageInfo_) win_.draw(*pageInfo_);

    for (auto& btn : navButtons_) {
        if (btn) win_.draw(*btn);
    }
}
