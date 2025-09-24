#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <random>
#include <cstdint>                // ← pour std::uint8_t

#include "MapGenerator.hpp"
#include "TileMap.hpp"
#include "CreatureSystem.hpp"
#include "TreeSystem.hpp"
#include "BuildMenu.hpp"
#include "Button.hpp"

class GameScene {
public:
    explicit GameScene(sf::RenderWindow& win);

    void handleInput(bool mouseLeft, bool mouseLeftReleased, bool mouseMoved);
    void update(float dt);
    void draw();

    std::unique_ptr<BuildMenu> menu_;

private:
    // utils
    void setPixelPerfectView(int worldW, int worldH, float tileSize);
    WaypointPath buildMainPathPolyline(const Map& m, float tileSize) const;

    // ★ Déclarations manquantes (pour corriger les “no declaration matches”)
    void updateMenuLayout();
    void drawMenu();

    sf::RenderWindow& win_;

    // terrain
    sf::Texture terrain_;
    TileMap     tilemap_;
    Map         map_;
    float       tileSize_ = 64.f;

    // décor
    TreeSystem  trees_;
    std::mt19937 rng_{ std::random_device{}() };

    // bâtiment ressource
    sf::Texture resourceTex_;
    std::unique_ptr<sf::Sprite> resourceSprite_;  // SFML3 : pas de ctor par défaut
    std::unique_ptr<sf::Sprite> resourceShadow_;  // idem

    // créatures
    CreatureSystem creeps_{tileSize_};
    float gameTime_ = 0.f;

    // ===== UI (SFML3: Sprite/Text via pointeurs) =====
    sf::Texture menuButtonTex_, menuBgTex_;
    std::unique_ptr<sf::Sprite> menuButton_;   // ← était sf::Sprite
    std::unique_ptr<sf::Sprite> menuBg_;       // ← était sf::Sprite

    sf::Font    uiFont_;
    std::unique_ptr<sf::Text> uiTitle_;        // ← était sf::Text

    sf::RectangleShape matSlots_[3];
    sf::CircleShape    unitBtns_[3];
    bool unitAffordable_[3] = {true,true,true};
    int  materialCount_[3]  = {0,0,0};

    bool  menuOpen_ = false;
    float menuAnim_ = 0.f;
    float menuAnimSpeed_ = 3.f;
    float menuAlpha_ = 0.f;
};
