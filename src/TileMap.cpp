#include "TileMap.hpp"

// ===================== Helpers =====================

static inline bool inB(const Map& m, int x, int y){ return m.inBounds(x,y); }

static inline bool isWater(const Map& m, int x, int y){
    return inB(m,x,y) && m.at(x,y).ground == Tile::Water;
}

static int waterCount8(const Map& m, int x, int y){
    static const int dx[8] = {-1,0,1, -1,1, -1,0,1};
    static const int dy[8] = {-1,-1,-1, 0,0, 1,1,1};
    int c = 0;
    for (int k=0;k<8;++k) if (isWater(m, x+dx[k], y+dy[k])) ++c;
    return c;
}

// UV avec petit inset pour éviter le texture bleeding
static inline void atlasUV(int col, int row, const sf::Vector2i tilePx, float inset,
                           sf::Vector2f& t0, sf::Vector2f& t1,
                           sf::Vector2f& t2, sf::Vector2f& t3)
{
    float u0 = col * tilePx.x + inset;
    float v0 = row * tilePx.y + inset;
    float u1 = (col+1) * tilePx.x - inset;
    float v1 = (row+1) * tilePx.y - inset;
    t0 = {u0, v0}; t1 = {u1, v0}; t2 = {u1, v1}; t3 = {u0, v1};
}

// Choix de la tuile d'eau selon les voisins (atlas 1x12)
static inline void pickWaterTile(const Map& m, int x, int y, int& col, int& row){
    bool N = isWater(m,x,  y-1);
    bool S = isWater(m,x,  y+1);
    bool W = isWater(m,x-1,y  );
    bool E = isWater(m,x+1,y  );

    // coins (prioritaires)
    if (!N && !W &&  E &&  S){ col = 7;  row = 0; return; } // NW
    if (!N && !E &&  W &&  S){ col = 8;  row = 0; return; } // NE
    if (!S && !E &&  W &&  N){ col = 9;  row = 0; return; } // SE
    if (!S && !W &&  E &&  N){ col = 10; row = 0; return; } // SW

    // bords
    if (!N && (S||E||W)){ col = 3; row = 0; return; } // edge N
    if (!S && (N||E||W)){ col = 4; row = 0; return; } // edge S
    if (!W && (N||S||E)){ col = 5; row = 0; return; } // edge W
    if (!E && (N||S||W)){ col = 6; row = 0; return; } // edge E

    // sinon centre
    col = 2; row = 0;
}

// ===================== TileMap API =====================

int TileMap::atlasIndex(Tile t){
    // mapping par défaut pour les types sans auto-tiling
    // 0=Grass, 1=Path, 2..10=Water variants, 11=FREE
    switch(t){
        case Tile::Grass:  return 0;
        case Tile::Path:   return 1;
        case Tile::Water:  return 2;   // (center — bords/corners gérés ailleurs)
        case Tile::Forest: return 0;   // fallback → Grass (pas présent dans l'atlas)
        case Tile::Rock:   return 11;  // FREE / zone libre (nouvelle tuile)
        case Tile::Bush:   return 0;   // fallback → Grass
    }
    return 0;
}

bool TileMap::setTexture(const sf::Texture& tex, sf::Vector2i tileSize){
    texture_ = &tex;
    tilePx_  = tileSize; // (64,64)
    return true;
}

void TileMap::build(const Map& m, float tileSize){
    tileSize_ = tileSize;

    // ✅ 2 triangles → 6 sommets
    const std::size_t vertsPerTile = 6;
    va_ = sf::VertexArray(sf::PrimitiveType::Triangles, m.w * m.h * vertsPerTile);

    const float renderT = tileSize_;
    const float uvInset = 0.5f; // anti-bleed

    std::size_t base = 0;
    for (int y=0; y<m.h; ++y){
        for (int x=0; x<m.w; ++x){
            Tile t = m.at(x,y).ground;

            // —— Règle qualité : l'eau doit appartenir à un amas ≥ 5 (8-voisinage)
            if (t == Tile::Water && waterCount8(m, x, y) < 4)
                t = Tile::Grass;

            // positions (quad plein écran)
            float px = x * renderT, py = y * renderT;
            sf::Vector2f p0{px,             py};
            sf::Vector2f p1{px+renderT,     py};
            sf::Vector2f p2{px+renderT, py+renderT};
            sf::Vector2f p3{px,         py+renderT};

            // UV
            int col = 0, row = 0;
            if (t == Tile::Water){
                pickWaterTile(m, x, y, col, row); // choisit center/edge/corner
            } else {
                col = atlasIndex(t); row = 0;
            }

            sf::Vector2f t0,t1,t2,t3;
            atlasUV(col, row, tilePx_, uvInset, t0,t1,t2,t3);

            // 2 triangles
            va_[base+0].position = p0; va_[base+0].texCoords = t0;
            va_[base+1].position = p1; va_[base+1].texCoords = t1;
            va_[base+2].position = p2; va_[base+2].texCoords = t2;

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
