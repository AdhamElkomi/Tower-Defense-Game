#pragma once
#include "Map.hpp"
#include <SFML/Graphics.hpp>
#include <cmath>
#include <functional>

class TileMap : public sf::Drawable {
public:
    bool setTexture(const sf::Texture& tex, sf::Vector2i tileSize);
    void build(const Map& m, float tileSize);

    // ⬇️ IMPORTANT: en SFML 3 on passe RenderStates PAR VALEUR (pas const&)
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    // NOUVEAU : build d’une couche en filtrant les tuiles et en mappant l’atlas
    void buildFiltered(
        const Map& m,
        float tileSize,
        const std::function<bool(const Cell&)>& include,     // quels tiles dessiner ?
        const std::function<int(Tile)>& atlasIndexForTile    // colonne dans l’atlas courant
    );
  

private:
    const sf::Texture* texture_{nullptr};
    sf::Vector2i tilePx_{32,32};
    sf::VertexArray va_; // Triangles
    float tileSize_{32.f};

    static int atlasIndex(Tile t);
};
