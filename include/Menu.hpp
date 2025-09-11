#pragma once
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <cstdint>

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

struct MenuChoice {
    bool start          = false;
    bool openDifficulty = false;
    bool exit           = false;
};

struct Slider {
    sf::Vector2f pos{0.f, 0.f};
    sf::Vector2f size{280.f, 10.f};
    float value01 = 0.8f;   // 0..1
    bool  dragging = false;
};

struct MenuButton {
    std::string id;

    sf::Vector2f pos{0.f, 0.f};   // position logique (coin haut-gauche)
    sf::Vector2f size{360.f, 56.f};

    // animation / interaction
    float scale       = 1.f;
    float targetScale = 1.f;
    float hover       = 0.f; // 0..1
    bool  hovered     = false;
    bool  focused     = false;

    // couleurs base / hover
    sf::Color fillA{}, fillB{}, border{};
    sf::Color hoverFillA{}, hoverFillB{}, hoverBorder{};

    std::unique_ptr<sf::Text>   label;
    std::unique_ptr<sf::Sprite> icon;

    bool hasIcon() const { return icon != nullptr; }
};

class Menu {
public:
    explicit Menu(sf::RenderWindow& win);

    // Tick = events + update + draw (ne fait PAS display/clear)
    std::optional<MenuChoice> tick();
    void render(); // alias draw()

    // Accès aux sliders pour l’audio global
    float musicVolume01() const { return musicVol01_; }
    float sfxVolume01()   const { return sfxVol01_;   }

    // Optionnel : sous-titre de la difficulté
    void setDifficultySubtitle(const std::string& text);

private:
    // --- Référence fenêtre
    sf::RenderWindow& win_;

    // --- Ressources
    sf::Font   font_;
    sf::Texture icoStart_, icoGear_, icoExit_;
    sf::Texture texBg_, texCardBg_, texBtn_;

    std::unique_ptr<sf::Sprite> bg_;
    std::unique_ptr<sf::Sprite> cardBg_;
    std::unique_ptr<sf::Sprite> gearIcon_;

    // --- Shaders
    sf::Shader panelShader_;
    sf::Shader buttonShader_;
    bool shaderOk_    = false;
    bool btnShaderOk_ = false;
    float animTime_   = 0.f;
    sf::Clock animClock_;

    // --- Mise en page principale (card)
    sf::Vector2f cardPos_{0.f, 0.f};
    sf::Vector2f cardSize_{720.f, 360.f};
    float        cornerRadius_ = 18.f;

    // Ombre sous le card
    sf::RectangleShape dropShadow_;

    // Titre
    std::unique_ptr<sf::Text> title_;
    std::unique_ptr<sf::Text> titleShadow_;
    float titlePulseT_ = 0.f;

    // --- Boutons
    std::vector<MenuButton> buttons_;
    float btnRadius_ = 14.f;
    int   focusIndex_ = 0;

    // --- Bouton "Settings" (cercle en bas à droite)
    sf::CircleShape gearButton_;
    bool optionsOpen_ = false;
    bool gearHover_   = false;

    // --- Panneau Options
    sf::Vector2f optPos_{0.f, 0.f};
    sf::Vector2f optSize_{420.f, 180.f};
    sf::RectangleShape optShadow_;
    std::unique_ptr<sf::Text> optTitle_;
    std::unique_ptr<sf::Text> lblMusic_, lblSfx_;
    std::unique_ptr<sf::Text> pctMusic_, pctSfx_;
    Slider sliderMusic_, sliderSfx_;
    float  musicVol01_ = 0.8f;
    float  sfxVol01_   = 0.8f;

    // --- Construction
    void loadAssets();
    void buildLayout();
    void buildSettings();
    void positionElements();
    void positionSettings();

    // --- Interaction
    void updateHoverFocus(const sf::Vector2f& mouse);

    // --- Dessin
    void draw();
    void drawSettings();

    // --- Utils
    static float clamp01(float v) {
        if (v < 0.f) return 0.f;
        if (v > 1.f) return 1.f;
        return v;
    }
    bool loadFont(const std::string& path);
    bool loadTexture(sf::Texture& t, const std::string& path);

    bool hitCircle(const sf::Vector2f& p, const sf::Vector2f& c, float r) const;
};
