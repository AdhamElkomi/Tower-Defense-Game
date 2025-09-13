#include "TreeSystem.hpp"
#include <filesystem>
#include <cmath>

bool TreeSystem::loadTextures(const std::string& folder) {
    tex_.clear();
    tex_.resize(3);
    bool ok = true;

    ok &= tex_[0].loadFromFile(std::filesystem::path(folder) / "tree_1.png");
    ok &= tex_[1].loadFromFile(std::filesystem::path(folder) / "tree_2.png");
    ok &= tex_[2].loadFromFile(std::filesystem::path(folder) / "tree_3.png");

    for (auto& t : tex_) t.setSmooth(false);
    return ok;
}

bool TreeSystem::isRoad(const Map& m, int x, int y) const {
    if (!m.inBounds(x,y)) return false;
    // ✅ on check si la cellule est un chemin
    return (m.at(x,y).ground == Tile::Path);
}

bool TreeSystem::isNearRoad(const Map& m, int x, int y, unsigned pad) const {
    for (int dy = -(int)pad; dy <= (int)pad; ++dy)
        for (int dx = -(int)pad; dx <= (int)pad; ++dx)
            if (m.inBounds(x+dx, y+dy) && m.at(x+dx,y+dy).ground == Tile::Path)
                return true;
    return false;
}

bool TreeSystem::overlapsOthers(sf::Vector2f p, float minDist) const {
    const float d2 = minDist * minDist;
    for (const auto& s : trees_) {
        sf::Vector2f q = s.getPosition();
        float dx = p.x - q.x, dy = p.y - q.y;
        if (dx*dx + dy*dy < d2) return true;
    }
    return false;
}

void TreeSystem::generate(const Map& map, float tileSize, unsigned count, std::mt19937& rng,
                          unsigned roadPaddingTiles) {
    trees_.clear();
    tileSize_ = tileSize;
    if (tex_.empty()) return;

    std::uniform_int_distribution<int> distX(0, map.w - 1);
    std::uniform_int_distribution<int> distY(0, map.h - 1);
    std::uniform_int_distribution<int> distTex(0, (int)tex_.size() - 1);
    std::uniform_real_distribution<float> jitter(-tileSize * 0.20f, tileSize * 0.20f);

    const float minDist = tileSize * 0.85f;
    const unsigned maxAttempts = count * 80;
    unsigned attempts = 0;

    while (trees_.size() < count && attempts < maxAttempts) {
        ++attempts;
        int tx = distX(rng), ty = distY(rng);

        if (!map.inBounds(tx,ty)) continue;
        if (map.at(tx,ty).ground != Tile::Grass) continue;        // ✅ seulement sur herbe
        if (isNearRoad(map, tx, ty, roadPaddingTiles)) continue;

        float px = (tx + 0.5f) * tileSize + jitter(rng);
        float py = (ty + 0.5f) * tileSize + jitter(rng);
        sf::Vector2f pos(px, py);

        if (overlapsOthers(pos, minDist)) continue;

        int k = distTex(rng);
        sf::Sprite sp(tex_[k]);                       // ✅ constructeur avec texture
        auto r = sp.getLocalBounds();                 // SFML3: {position,size}
        sp.setOrigin({ r.size.x * 0.5f, r.size.y });  // bas-centre
        sp.setPosition(pos);

        // ✅ réduire la taille (exemple : moitié)
        std::uniform_real_distribution<float> scaleDist(0.1f, 0.2f);
float s = scaleDist(rng);
sp.setScale(sf::Vector2f(s, s));


        trees_.push_back(sp);
    }
}

void TreeSystem::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    for (const auto& t : trees_) target.draw(t, states);
}
