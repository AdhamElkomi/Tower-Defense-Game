#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include <cmath>
#include "TreeSystem.hpp"
#include "BuildMenu.hpp"
#include "Button.hpp"


enum class Material { Wood=0, Stone=1, Crystal=2 };

struct MaterialDrop {
    Material type;
    sf::Vector2f pos; // world position where it appears (optional for your UX)
};


class CreatureSystem;


struct ImpactBurst {
    sf::Vector2f p;
    float t = 0.18f; // lifetime
};

struct Projectile {
    sf::Vector2f pos, vel, target;
    float life = 1.0f;
    bool  alive = true;

    struct Trail { sf::Vector2f p; float t; };
    std::vector<Trail> trail;
    float trailSpawn = 0.f;

    bool update(float dt);
    void draw(sf::RenderTarget& rt) const;
};

class CannonTower {
public:
    CannonTower(sf::Vector2f center, const sf::Texture& baseTex);

    void tryFireAt(sf::Vector2f mouseWorld, CreatureSystem& creeps);
    void update(float dt, CreatureSystem& creeps);
    void draw(sf::RenderTarget& rt, bool showRadius) const;

    int   footprintW() const { return 1; }
    int   footprintH() const { return 1; }
    sf::Vector2f pos() const { return pos_; }
    float        radius() const { return radius_; }
    bool         isDead() const { return shotsLeft_ <= 0 && projectiles_.empty() && muzzleT_<=0.f; }
    int          shotsLeft() const { return shotsLeft_; }
    void extractDrops(std::vector<MaterialDrop>& out) {
        out.insert(out.end(), dropBatch_.begin(), dropBatch_.end());
        dropBatch_.clear();
    }

private:
    sf::Vector2f  pos_;
    sf::Sprite    base_;          // cannon icon
    float         radius_   = 400.f;  // ⬅ bigger default range
    float         cooldown_ = 0.25f, cd_ = 0.f;
    int           shotsLeft_ = 8;

    // FX
    float         recoilT_ = 0.f;   // recoil timer
    float         muzzleT_ = 0.f;   // muzzle flash timer
    std::vector<Projectile>  projectiles_;
    std::vector<ImpactBurst> impacts_;
     std::vector<MaterialDrop> dropBatch_;
};
