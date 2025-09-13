#pragma once
#include <SFML/Graphics.hpp>
#include "MapGenerator.hpp"
#include "TileMap.hpp"
#include "TreeSystem.hpp"
#include <random>

class GameScene {
public:
    explicit GameScene(sf::RenderWindow& win);

    void handleInput(bool mouseLeft, bool mouseLeftReleased, bool mouseMoved);
    void update(float dt);
    void draw();

private:
    sf::RenderWindow& win_;

    sf::Texture terrain_;  // atlas
    TileMap     tilemap_;
    Map         map_;

    float tileSize_ = 32.f;   // taille affichée de chaque tuile à l’écran

    TreeSystem trees_;
    std::mt19937 rng_{ std::random_device{}() };
};
