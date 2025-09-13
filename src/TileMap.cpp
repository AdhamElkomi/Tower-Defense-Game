#include "TileMap.hpp"

int TileMap::atlasIndex(Tile t){
    switch(t){
        case Tile::Grass:  return 0;
        case Tile::Path:   return 1;
        case Tile::Water:  return 2;
        case Tile::Forest: return 3;
        case Tile::Rock:   return 4;
        case Tile::Bush:   return 5;
    }
    return 0;
}

bool TileMap::setTexture(const sf::Texture& tex, sf::Vector2i tileSize){
    texture_ = &tex;
    tilePx_  = tileSize;
    return true;
}

void TileMap::build(const Map& m, float tileSize){
    tileSize_ = tileSize;

    // ⬇️ SFML 3: plus de Quads → Triangles
    const std::size_t vertsPerTile = 6; // 2 triangles
    va_ = sf::VertexArray(sf::PrimitiveType::Triangles, m.w * m.h * vertsPerTile);

    auto vAt = [&](std::size_t i){ return va_[i]; };

    const float renderT = tileSize_;
    const int   pxT     = tilePx_.x;

    std::size_t base = 0;
    for (int y=0; y<m.h; ++y){
        for (int x=0; x<m.w; ++x){
            const auto& c = m.at(x,y);

            float px = x*renderT, py = y*renderT;
            // positions
            sf::Vector2f p0{px,             py};
            sf::Vector2f p1{px+renderT,     py};
            sf::Vector2f p2{px+renderT, py+renderT};
            sf::Vector2f p3{px,         py+renderT};

            // UV
            int ai = atlasIndex(c.ground);
            float u0 = static_cast<float>(ai * pxT);
            float v0 = 0.f;
            float u1 = static_cast<float>((ai+1) * pxT);
            float v1 = static_cast<float>(tilePx_.y);

            sf::Vector2f t0{u0, v0};
            sf::Vector2f t1{u1, v0};
            sf::Vector2f t2{u1, v1};
            sf::Vector2f t3{u0, v1};

            // triangle 1 : p0-p1-p2
            va_[base+0].position = p0; va_[base+0].texCoords = t0;
            va_[base+1].position = p1; va_[base+1].texCoords = t1;
            va_[base+2].position = p2; va_[base+2].texCoords = t2;
            // triangle 2 : p0-p2-p3
            va_[base+3].position = p0; va_[base+3].texCoords = t0;
            va_[base+4].position = p2; va_[base+4].texCoords = t2;
            va_[base+5].position = p3; va_[base+5].texCoords = t3;

            base += vertsPerTile;
        }
    }
}

void TileMap::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (texture_) states.texture = texture_;
    target.draw(va_, states);
}
