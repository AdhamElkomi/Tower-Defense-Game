#pragma once
#include <SFML/Graphics.hpp>
#include <optional>
#include <string>
#include <vector>

#include "Button.hpp"
#include "CircleButton.hpp"
#include "Slider.hpp"
#include "AudioManager.hpp"
#include "LeaderboardController.hpp"


struct Theme {
    sf::Color text     {255,255,255};
    sf::Color btnHover {255,255,255,28};
};

class MenuScene {
public:
    explicit MenuScene(sf::RenderWindow& win);
    MenuScene(sf::RenderWindow& win, std::function<void()> onStartGame);
    void handleInput(bool mousePressedLeft, bool mouseReleasedLeft, bool mouseMoved);
    void handleTextInput(char32_t unicode);
    void handleUsernameInput(char32_t unicode);
    void handleUsernamePromptInput(bool mousePressedLeft, bool mouseReleasedLeft, bool mouseMoved);
    void draw();
    void drawUsernamePrompt();
    void update(float dt);
    bool started() const { return started_; }
    void setOnStart(std::function<void()> cb) { onStartGame_ = std::move(cb); }
    std::string getDifficulty() const { return difficulty_; }
    bool shouldGoToUsernamePrompt() const { return goToUsernamePrompt_; }
    bool shouldGoToLeaderboard() const { return goToLeaderboard_; }
    bool usernamePromptDone() const { return usernamePromptDone_; }
    bool usernamePromptCancelled() const { return usernamePromptCancelled_; }
    std::string getEnteredUsername() const { return enteredUsername_; }

    bool shouldReturnToMenu() const { return returnToMenu_; }
    void resetUsernamePrompt() {
        usernamePromptOpen_ = false;
        usernamePromptCancelled_ = false;
        usernamePromptDone_ = false;
        enteredUsername_.clear();
    }
    void clearUsernamePromptFlag() { goToUsernamePrompt_ = false; }
    void clearLeaderboardFlag() { goToLeaderboard_ = false; }
     AudioManager audio_;
private:
    sf::RenderWindow& win_;

    // Assets chargés
    sf::Font    font_;
    sf::Texture bgTex_;
    sf::Texture panelTex_;
    sf::Texture gearTex_;
    sf::Texture leaderboardIconTex_;

    // Éléments dépendants des assets (optionnels car non default-constructible)
    std::optional<sf::Sprite> bgSprite_;
    std::optional<sf::Sprite> panelSprite_;
    std::optional<sf::Text>   title_;
    std::optional<sf::Text>   musicLabel_;
    std::optional<sf::Text>   sfxLabel_;
    std::optional<sf::Text>   musicValueLabel_;
    std::optional<sf::Text>   sfxValueLabel_;
    sf::RectangleShape settingsPanelBG_;
    sf::FloatRect settingsBounds_{};
    std::function<void()> onStartGame_;
    bool editingMusic_{false};
    bool editingSfx_{false};
    std::string musicInput_;
    std::string sfxInput_;
   

    // UI
    std::optional<Button> btnStart_;
    std::optional<Button> btnDifficulty_;
    std::optional<Button> btnExit_;
    CircleButton settingsBtn_{40.f, nullptr};
    CircleButton leaderboardBtn_{50.f, nullptr};

    Slider musicSlider_{0.f, 1.f, 0.7f, 260.f};
    Slider sfxSlider_  {0.f, 1.f, 0.8f, 260.f};

    // Difficulty submenu
    bool difficultyMenuOpen_{false};
    sf::RectangleShape difficultyPanelBG_;
    std::optional<sf::Text> difficultyTitle_;
    std::vector<std::optional<Button>> difficultyButtons_;

    // Username prompt
    bool usernamePromptOpen_{false};
    sf::RectangleShape usernamePanelBG_;
    std::optional<sf::Text> usernameTitle_;
    std::optional<sf::Text> usernameInput_;
    std::optional<sf::RectangleShape> usernameInputBG_;
    std::optional<sf::Text> usernameError_;
    std::optional<Button> usernameOkBtn_;
    std::optional<Button> usernameBackBtn_;
    std::string enteredUsername_;
    bool usernameValid_{false};

    // État
   
    Theme theme_;

    bool started_{false};
    bool settingsOpen_{false};
    std::string difficulty_{"Normal"};
    bool goToUsernamePrompt_{false};
    bool goToLeaderboard_{false};
    bool usernamePromptDone_{false};
    bool usernamePromptCancelled_{false};
    bool returnToMenu_ {false};
};