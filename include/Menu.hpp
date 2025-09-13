#pragma once
#include <SFML/Graphics.hpp>
#include <optional>
#include <string>

#include "Button.hpp"
#include "CircleButton.hpp"
#include "Slider.hpp"
#include "AudioManager.hpp"

struct Theme {
    sf::Color text     {255,255,255};
    sf::Color btnHover {255,255,255,28};
};

class MenuScene {
public:
    explicit MenuScene(sf::RenderWindow& win);
      MenuScene(sf::RenderWindow& win, std::function<void()> onStartGame);
    void handleInput(bool mousePressedLeft, bool mouseReleasedLeft, bool mouseMoved);
    void draw(); 
    void update(float dt);
     bool started() const { return started_; }
      void setOnStart(std::function<void()> cb) { onStartGame_ = std::move(cb); }
     

private:
    sf::RenderWindow& win_;

    // Assets chargés
    sf::Font    font_;
    sf::Texture bgTex_;
    sf::Texture panelTex_;
    sf::Texture gearTex_;

    // Éléments dépendants des assets (optionnels car non default-constructible)
    std::optional<sf::Sprite> bgSprite_;
    std::optional<sf::Sprite> panelSprite_;
    std::optional<sf::Text>   title_;
    std::optional<sf::Text>   musicLabel_;
    std::optional<sf::Text>   sfxLabel_;
    sf::RectangleShape settingsPanelBG_;
    sf::FloatRect settingsBounds_{};
    std::function<void()> onStartGame_;

    // UI
    std::optional<Button> btnStart_;
    std::optional<Button> btnDifficulty_;
    std::optional<Button> btnExit_;
    CircleButton settingsBtn_{40.f, nullptr};

    Slider musicSlider_{0.f, 1.f, 0.7f, 260.f}; // valeurs exemple
    Slider sfxSlider_  {0.f, 1.f, 0.8f, 260.f};

    // État
    AudioManager audio_;
    Theme theme_;

    bool started_{false};
    bool settingsOpen_{false};
    int  idxDiff_{0};
    std::string difficulty_{"Normal"};
};
