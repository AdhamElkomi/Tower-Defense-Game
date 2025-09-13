#pragma once
#include <SFML/Graphics.hpp>
#include <random>
#include <vector>
#include <string>
  #include "Map.hpp"// ✅ utilise la définition existante de Map/Tile

class TreeSystem : public sf::Drawable {
public:
    TreeSystem() = default;

    // Charge tree_1.png .. tree_3.png
    bool loadTextures(const std::string& folder);

    // Place 'count' arbres aléatoirement (évite route + chevauchements)
    void generate(const Map& map, float tileSize, unsigned count, std::mt19937& rng,
                  unsigned roadPaddingTiles = 1);

    const std::vector<sf::Sprite>& trees() const { return trees_; }

private:
    // SFML 3: RenderStates par valeur
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    bool isRoad(const Map& m, int x, int y) const;
    bool isNearRoad(const Map& m, int x, int y, unsigned pad) const;
    bool overlapsOthers(sf::Vector2f p, float minDist) const;

    std::vector<sf::Texture> tex_;   // textures des arbres
    std::vector<sf::Sprite>  trees_; // sprites placés
    float tileSize_{32.f};
};
