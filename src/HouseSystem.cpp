#include "HouseSystem.hpp"
#include <filesystem>
#include <cmath>

bool HouseSystem::loadTextures(const std::string& folder) {
    tex_.clear();
    tex_.resize(4);
    bool ok = true;

    ok &= tex_[0].loadFromFile(std::filesystem::path(folder) / "house1.png");
    ok &= tex_[1].loadFromFile(std::filesystem::path(folder) / "house2.png");
    ok &= tex_[2].loadFromFile(std::filesystem::path(folder) / "house3.png");
    ok &= tex_[3].loadFromFile(std::filesystem::path(folder) / "house4.png");

    for (auto& t : tex_) t.setSmooth(false);
    return ok;
}

bool HouseSystem::isRoad(const Map& m, int x, int y) const {
    if (!m.inBounds(x,y)) return false;
    return (m.at(x,y).ground == Tile::Path);
}

bool HouseSystem::isNearRoad(const Map& m, int x, int y, unsigned pad) const {
    for (int dy = -(int)pad; dy <= (int)pad; ++dy)
        for (int dx = -(int)pad; dx <= (int)pad; ++dx)
            if (m.inBounds(x+dx, y+dy) && m.at(x+dx,y+dy).ground == Tile::Path)
                return true;
    return false;
}

bool HouseSystem::isNearRock(const Map& m, int x, int y, unsigned pad) const {
    for (int dy = -(int)pad; dy <= (int)pad; ++dy)
        for (int dx = -(int)pad; dx <= (int)pad; ++dx)
            if (m.inBounds(x+dx, y+dy) && m.at(x+dx,y+dy).ground == Tile::Rock)
                return true;
    return false;
}

bool HouseSystem::overlapsOthers(sf::Vector2f p, float minDist) const {
    const float d2 = minDist * minDist;
    for (const auto& s : houses_) {
        sf::Vector2f q = s.getPosition();
        float dx = p.x - q.x, dy = p.y - q.y;
        if (dx*dx + dy*dy < d2) return true;
    }
    return false;
}

void HouseSystem::generate(const Map& map, float tileSize, unsigned maxCount, std::mt19937& rng,
                          unsigned roadPaddingTiles) {
    houses_.clear();
    tileSize_ = tileSize;
    if (tex_.empty()) return;

    std::uniform_int_distribution<int> distX(0, map.w - 1);
    std::uniform_int_distribution<int> distY(0, map.h - 1);
    std::uniform_int_distribution<int> distTex(0, (int)tex_.size() - 1);
    std::uniform_real_distribution<float> jitter(-tileSize * 0.20f, tileSize * 0.20f);

    const float minDist = tileSize * 3.0f; // More spaced apart
    const unsigned maxAttempts = maxCount * 200; // More attempts
    unsigned attempts = 0;

    while (houses_.size() < maxCount && attempts < maxAttempts) {
        ++attempts;
        int tx = distX(rng), ty = distY(rng);

        if (!map.inBounds(tx,ty)) continue;
        if (map.at(tx,ty).ground != Tile::Grass) continue;        // Only on grass
        if (isNearRoad(map, tx, ty, roadPaddingTiles)) continue;
        if (isNearRock(map, tx, ty, 2)) continue; // Avoid near rock (buildable areas)

        float px = (tx + 0.5f) * tileSize + jitter(rng);
        float py = (ty + 0.5f) * tileSize + jitter(rng);
        sf::Vector2f pos(px, py);

        if (overlapsOthers(pos, minDist)) continue;

        int k = distTex(rng);
        sf::Sprite sp(tex_[k]);
        auto r = sp.getLocalBounds();
        sp.setOrigin(r.width * 0.5f, r.height);  // Bottom-center
        sp.setPosition(pos);

        // Larger scale
        std::uniform_real_distribution<float> scaleDist(0.2f, 0.3f);
        float s = scaleDist(rng);
        sp.setScale(sf::Vector2f(s, s));

        houses_.push_back(sp);
    }
}

void HouseSystem::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    for (const auto& h : houses_) target.draw(h, states);
}
