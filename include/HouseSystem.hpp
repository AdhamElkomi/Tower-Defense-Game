#pragma once
#include <SFML/Graphics.hpp>
#include <random>
#include <vector>
#include <string>
#include "Map.hpp"

class HouseSystem : public sf::Drawable {
public:
    HouseSystem() = default;

    // Load house1.png to house4.png
    bool loadTextures(const std::string& folder);

    // Place up to 'maxCount' houses randomly on grass tiles, spaced apart
    void generate(const Map& map, float tileSize, unsigned maxCount, std::mt19937& rng,
                  unsigned roadPaddingTiles = 1);

    const std::vector<sf::Sprite>& houses() const { return houses_; }

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    bool isRoad(const Map& m, int x, int y) const;
    bool isNearRoad(const Map& m, int x, int y, unsigned pad) const;
    bool isNearRock(const Map& m, int x, int y, unsigned pad) const;
    bool overlapsOthers(sf::Vector2f p, float minDist) const;

    std::vector<sf::Texture> tex_;   // textures des maisons
    std::vector<sf::Sprite>  houses_; // sprites placés
    float tileSize_{64.f};
};
