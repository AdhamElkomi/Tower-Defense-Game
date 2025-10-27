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
    std::unique_ptr<sf::Sprite> sprite;

    struct Trail { sf::Vector2f p; float t; };
    std::vector<Trail> trail;
    float trailSpawn = 0.f;

    Projectile() : sprite(nullptr) {}
    Projectile(const Projectile&) = delete;
    Projectile& operator=(const Projectile&) = delete;
    Projectile(Projectile&&) = default;
    Projectile& operator=(Projectile&&) = default;

    bool update(float dt, CreatureSystem& creeps);
    void draw(sf::RenderTarget& rt) const;
};

struct ArrowProjectile {
    sf::Vector2f pos, vel, target;
    float life = 1.0f;
    bool  alive = true;
    std::unique_ptr<sf::Sprite> sprite;
    float rotation = 0.f;

    ArrowProjectile() : sprite(nullptr) {}
    ArrowProjectile(const ArrowProjectile&) = delete;
    ArrowProjectile& operator=(const ArrowProjectile&) = delete;
    ArrowProjectile(ArrowProjectile&&) = default;
    ArrowProjectile& operator=(ArrowProjectile&&) = default;

    bool update(float dt, CreatureSystem& creeps);
    void draw(sf::RenderTarget& rt) const;
};

struct MageFireProjectile {
    sf::Vector2f pos, vel;
    float life = 3.0f; // longer life for dragon breath effect
    bool  alive = true;
    float angle = 0.f; // direction angle
    float powerMultiplier = 1.0f; // power based on hold time
    std::vector<sf::Vector2f> trail; // for particle trail
    float trailTimer = 0.f;

    MageFireProjectile() = default;
    MageFireProjectile(const MageFireProjectile&) = delete;
    MageFireProjectile& operator=(const MageFireProjectile&) = delete;
    MageFireProjectile(MageFireProjectile&&) = default;
    MageFireProjectile& operator=(MageFireProjectile&&) = default;

    bool update(float dt, CreatureSystem& creeps);
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
    virtual bool canHold() const { return false; }
    virtual void startHolding() {}
};

class CannonTower : public Tower {
public:
    static constexpr int DefaultShots = 8; // canon = le moins
    static constexpr float DefaultPower = 30.f; // Less powerful than archer and mage
    static constexpr float DefaultRadius = 300.f; // Reduced range
public:
    CannonTower(sf::Vector2f center, const sf::Texture& baseTex, const sf::Texture& cannonBallTex);
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
    const sf::Texture* cannonBallTexture_;
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
    static constexpr float DefaultPower = 60.f; // More powerful than cannon
    static constexpr float DefaultRadius = 400.f; // Reduced range
public:
    ArcherTower(sf::Vector2f center, const sf::Texture& baseTex, const sf::Texture& arrowTex);
    void update(float dt, CreatureSystem& creeps) override;
    void draw(sf::RenderTarget& rt, bool showRadius) const override;
    TowerType type() const override { return TowerType::Archer; }
    sf::Vector2f pos() const override;
    bool isDead() const override;
    int shotsLeft() const { return shotsLeft_; }
    void extractDrops(std::vector<MaterialDrop>& out) override;
    float radius() const override { return radius_; }
    void tryFireAt(sf::Vector2f mouseWorld, CreatureSystem& creeps) override;

private:
    sf::Vector2f pos_;
    sf::Sprite base_;
    const sf::Texture* arrowTexture_;
    int shotsLeft_ = DefaultShots;
    float power_ = DefaultPower;
    float radius_ = DefaultRadius;
    float cd_ = 0.f, cooldown_ = 0.5f; // Faster than cannon
    std::vector<ArrowProjectile> arrows_;
    std::vector<ImpactBurst> impacts_;
    std::vector<MaterialDrop> dropBatch_;
};

class MageTower : public Tower {
public:
    static constexpr int DefaultShots = 5; // mage = le plus puissant mais moins d'attaques
    static constexpr float DefaultPower = 120.f;
    static constexpr float DefaultRadius = 550.f; // Reduced range
public:
    MageTower(sf::Vector2f center, const sf::Texture& baseTex);
    void update(float dt, CreatureSystem& creeps) override;
    void draw(sf::RenderTarget& rt, bool showRadius) const override;
    TowerType type() const override { return TowerType::Mage; }
    sf::Vector2f pos() const override;
    bool isDead() const override;
    int shotsLeft() const { return shotsLeft_; }
    void extractDrops(std::vector<MaterialDrop>& out) override;
    float radius() const override { return radius_; }
    void tryFireAt(sf::Vector2f mouseWorld, CreatureSystem& creeps) override;
    bool canHold() const override { return true; }
    bool isGaugeFull() const { return holdTime_ >= maxHold_; }
    void startHolding() override;

public:
    bool isHolding_ = false;
    float holdTime_ = 0.f;

private:
    sf::Vector2f pos_;
    sf::Sprite base_;
    int shotsLeft_ = DefaultShots;
    float power_ = DefaultPower;
    float radius_ = DefaultRadius;
    float maxHold_ = 2.0f; // max hold time for full power
    float rechargeCooldown_ = 0.f; // cooldown after firing before recharging gauge
    std::vector<MageFireProjectile> projectiles_;
};
