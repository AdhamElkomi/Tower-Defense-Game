#pragma once
#include <SFML/Graphics.hpp>
#include <optional>
#include <string>
#include <vector>
#include <memory>

// Bouton (sprite arrondi optionnel + fallback rectangle)
struct MenuButton {
    sf::RectangleShape box;                  // fallback si pas de texture bouton
    std::unique_ptr<sf::Sprite> sprite;      // si texBtn_ chargé (coins arrondis)
    std::unique_ptr<sf::Text>   label;
    std::unique_ptr<sf::Sprite> icon;
    bool hovered{false};
    bool focused{false};
    std::string id;                          // "start", "difficulty", "exit"
    bool hasIcon()   const { return icon   != nullptr; }
    bool hasSprite() const { return sprite != nullptr; }
};

struct MenuChoice {
    bool start{false};
    bool exit{false};
    bool openDifficulty{false};
};



class Menu {
public:
    explicit Menu(sf::RenderWindow& win);
    std::optional<MenuChoice> tick();            // events + update + render (sans clear/display)
    void render();                                // alias de draw() pour App::run()
    void setDifficultySubtitle(const std::string& text);

private:
    sf::RenderWindow& win_;

    // Assets
    sf::Font   font_;
    sf::Texture icoStart_, icoGear_, icoExit_;
    sf::Texture texBg_;             // image de fond (cover)
    sf::Texture texPanel_;          // image cadre (optionnel, alpha arrondi)
    sf::Texture texBtn_;            // image bouton arrondi (optionnel)
    sf::Texture texCardBg_;
   



    // Sprites
    std::unique_ptr<sf::Sprite> bg_;
    std::unique_ptr<sf::Sprite> cardSprite_; // si texPanel_ chargé
     std::unique_ptr<sf::Sprite> cardBg_;
   // Titre
    std::unique_ptr<sf::Text> title_;

    // Cadre dessiné via shader fragment (arrondi + gradient + glow animé)
    sf::Shader panelShader_;
    bool shaderOk_{false};
    sf::Vector2f cardSize_{520.f, 360.f};
    sf::Vector2f cardPos_{};
    float cornerRadius_{18.f};

    // Animation
    sf::Clock animClock_;
    float animTime_{0.f};

    // Ombre douce sous le cadre
    sf::RectangleShape dropShadow_;

    // Boutons
    std::vector<MenuButton> buttons_;
    int focusIndex_{0};

    // Helpers
    void loadAssets();
    void buildLayout();
    void positionElements();
    void updateHoverFocus(const sf::Vector2f& mouse);
    void draw(); // NE FAIT PAS clear()/display()

    // Utils
    bool loadFont(const std::string& path);
    bool loadTexture(sf::Texture& t, const std::string& path);
};
