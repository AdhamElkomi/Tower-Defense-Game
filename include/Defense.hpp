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

enum class TowerType { Cannon, Archer, Mage };

struct TowerCost {
    int wood = 0, stone = 0, crystal = 0;
};

inline TowerCost getTowerCost(TowerType type) {
    switch(type) {
        case TowerType::Cannon: return {2, 1, 0};
        case TowerType::Archer: return {3, 2, 0};
        case TowerType::Mage:   return {0, 3, 2};
        default: return {};
    }
}

class Tower {
public:
    virtual ~Tower() = default;
    virtual void update(float dt, CreatureSystem& creeps) = 0;
    virtual void draw(sf::RenderTarget& rt, bool showRadius) const = 0;
    virtual TowerType type() const = 0;
    virtual sf::Vector2f pos() const = 0;
    virtual bool isDead() const = 0;
    virtual void extractDrops(std::vector<MaterialDrop>& out) = 0;
    virtual float radius() const = 0;
    virtual void tryFireAt(sf::Vector2f mouseWorld, CreatureSystem& creeps) = 0;
};

class CannonTower : public Tower {
public:
    static constexpr int DefaultShots = 8; // canon = le moins
    static constexpr float DefaultPower = 30.f;
    static constexpr float DefaultRadius = 180.f;
public:
    CannonTower(sf::Vector2f center, const sf::Texture& baseTex);
    void update(float dt, CreatureSystem& creeps) override;
    void draw(sf::RenderTarget& rt, bool showRadius) const override;
    TowerType type() const override { return TowerType::Cannon; }
    sf::Vector2f pos() const override { return pos_; }
    bool isDead() const override;
    int shotsLeft() const { return shotsLeft_; }
    void extractDrops(std::vector<MaterialDrop>& out) override;
    float radius() const override { return radius_; }
    void tryFireAt(sf::Vector2f mouseWorld, CreatureSystem& creeps) override;

private:
    sf::Vector2f pos_;
    sf::Sprite base_;
    float radius_ = DefaultRadius;
    float cd_ = 0.f, cooldown_ = 0.7f;
    int shotsLeft_ = DefaultShots;
    float power_ = DefaultPower;
    float recoilT_ = 0.f, muzzleT_ = 0.f;
    std::vector<Projectile> projectiles_;
    std::vector<ImpactBurst> impacts_;
    std::vector<MaterialDrop> dropBatch_;
};

class ArcherTower : public Tower {
public:
    static constexpr int DefaultShots = 14; // archer = intermédiaire
    static constexpr float DefaultPower = 50.f;
    static constexpr float DefaultRadius = 220.f;
public:
    ArcherTower(sf::Vector2f center, const sf::Texture& baseTex);
    void update(float dt, CreatureSystem& creeps) override;
    void draw(sf::RenderTarget& rt, bool showRadius) const override;
    TowerType type() const override { return TowerType::Archer; }
    sf::Vector2f pos() const override;
    bool isDead() const override;
    int shotsLeft() const { return shotsLeft_; }
    void extractDrops(std::vector<MaterialDrop>& out) override;
    float radius() const override { return 120.f; }
    void tryFireAt(sf::Vector2f mouseWorld, CreatureSystem& creeps) override;

private:
    sf::Vector2f pos_;
    sf::Sprite base_;
    int shotsLeft_ = DefaultShots;
    float power_ = DefaultPower;
    float radius_ = DefaultRadius;
    // Ajoutez ici les membres spécifiques à l'archer (ex: projectiles, cooldown, etc.)
};

class MageTower : public Tower {
public:
    static constexpr int DefaultShots = 10; // mage = le plus puissant mais moins d'attaques
    static constexpr float DefaultPower = 80.f;
    static constexpr float DefaultRadius = 260.f;
public:
    MageTower(sf::Vector2f center, const sf::Texture& baseTex);
    void update(float dt, CreatureSystem& creeps) override;
    void draw(sf::RenderTarget& rt, bool showRadius) const override;
    TowerType type() const override { return TowerType::Mage; }
    sf::Vector2f pos() const override;
    bool isDead() const override;
    int shotsLeft() const { return shotsLeft_; }
    void extractDrops(std::vector<MaterialDrop>& out) override;
    float radius() const override { return 120.f; }
    void tryFireAt(sf::Vector2f mouseWorld, CreatureSystem& creeps) override;

private:
    sf::Vector2f pos_;
    sf::Sprite base_;
    int shotsLeft_ = DefaultShots;
    float power_ = DefaultPower;
    float radius_ = DefaultRadius;
    // Ajoutez ici les membres spécifiques au mage (ex: projectiles, cooldown, etc.)
};
