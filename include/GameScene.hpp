#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <random>

#include "MapGenerator.hpp"
#include "TileMap.hpp"
#include "CreatureSystem.hpp"
#include "TreeSystem.hpp"

class GameScene {
public:
    explicit GameScene(sf::RenderWindow& win);

    void handleInput(bool mouseLeft, bool mouseLeftReleased, bool mouseMoved);
    void update(float dt);
    void draw();

private:
    // utils
    void setPixelPerfectView(int worldW, int worldH, float tileSize);
    WaypointPath buildMainPathPolyline(const Map& m, float tileSize) const;

private:
    sf::RenderWindow& win_;

    // terrain
    sf::Texture terrain_;  // atlas
    TileMap     tilemap_;
    Map         map_;
    float       tileSize_ = 64.f;

    // décor
    TreeSystem  trees_;
    std::mt19937 rng_{ std::random_device{}() };

    // bâtiment ressource (centré)
    sf::Texture resourceTex_;
    std::unique_ptr<sf::Sprite> resourceSprite_; // sprite bâtiment
    std::unique_ptr<sf::Sprite> resourceShadow_; // ombre

    // créatures
    CreatureSystem creeps_{tileSize_};
    float gameTime_ = 0.f;
};
