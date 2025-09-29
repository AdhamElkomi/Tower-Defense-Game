#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include <cmath>

class CreatureSystem;

struct Projectile {
    sf::Vector2f pos, vel;
    sf::Vector2f target;
    float life = 1.2f;     // seconds
    bool  alive = true;

    // visuals
    struct Trail { sf::Vector2f p; float t; };
    std::vector<Trail> trail;
    float trailSpawn = 0.f;

    bool update(float dt){
        if (!alive) return false;
        sf::Vector2f d = target - pos;
        float L2 = d.x*d.x + d.y*d.y;
        float L = std::sqrt(L2);

        // ease-in accel to target
        sf::Vector2f dir = (L>1e-4f) ? (d / L) : sf::Vector2f(0,0);
        vel += dir * 2200.f * dt;          // acceleration
        vel *= 0.986f;                     // tiny drag
        pos += vel * dt;

        // trail
        trailSpawn -= dt;
        if (trailSpawn <= 0.f){
            trailSpawn = 0.015f;
            trail.push_back({pos, 0.25f});
        }
        for (auto& t : trail) t.t -= dt;
        trail.erase(std::remove_if(trail.begin(), trail.end(),
                   [](const Trail& tr){ return tr.t<=0.f; }), trail.end());

        life -= dt;
        if (L < 18.f || life <= 0.f){ alive = false; return true; }
        return false;
    }

    void draw(sf::RenderTarget& rt) const{
        // trail puffs
        for (auto & tr : trail){
            float a = std::clamp(tr.t/0.25f, 0.f, 1.f);
            sf::CircleShape puff(6.f*(1.f-a));
            puff.setOrigin(sf::Vector2f(puff.getRadius(), puff.getRadius()));
            puff.setPosition(tr.p);
            puff.setFillColor(sf::Color(200,200,220, (std::uint8_t)(110*a)));
            rt.draw(puff);
        }

        // projectile core
        sf::CircleShape core(4.f);
        core.setOrigin(sf::Vector2f(4.f,4.f));
        core.setPosition(pos);
        core.setFillColor(sf::Color(255,230,120));
        rt.draw(core);

        // glow
        sf::CircleShape glow(10.f);
        glow.setOrigin(sf::Vector2f(10.f,10.f));
        glow.setPosition(pos);
        glow.setFillColor(sf::Color(255,180,80,120));
        rt.draw(glow);
    }
};

class CannonTower {
public:
   CannonTower(sf::Vector2f center, const sf::Texture& baseTex);

    void tryFireAt(sf::Vector2f mouseWorld, CreatureSystem& creeps);
    void update(float dt, CreatureSystem& creeps);
    void draw(sf::RenderTarget& rt, bool showRadius) const;

    // footprint (1x1 for cannon; archer 2x2, mage 3x3 in their own classes later)
    int footprintW() const { return 1; }
    int footprintH() const { return 1; }

    // === NEW public accessors ===
    sf::Vector2f pos() const { return pos_; }
    float        radius() const { return radius_; }
    bool         isDead() const { return shotsLeft_ <= 0; }
    int          shotsLeft() const { return shotsLeft_; }


private:
    sf::Vector2f pos_;
    sf::CircleShape baseRing_;
    sf::Sprite icon_;

    float cooldown_ = 0.35f, cdTimer_ = 0.f;
    int   ammo_ = 8;
    float recoilT_ = 0.f;
     float        radius_     = 220.f;  // or your value
    int          shotsLeft_  = 6;      // example
    // sprite(s)
    sf::Sprite   base_;

    std::vector<Projectile> projectiles_;
};
