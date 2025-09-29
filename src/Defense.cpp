#include "Defense.hpp"
#include "CreatureSystem.hpp"
#include <algorithm>
#include <cmath>

CannonTower::CannonTower(sf::Vector2f center, const sf::Texture& baseTex)
: pos_(center)
, icon_(baseTex)   // init icon_ with the texture too
, base_(baseTex)   // init base_ with the texture
{
    base_.setOrigin({ baseTex.getSize().x * 0.5f, baseTex.getSize().y * 0.5f });
    base_.setPosition(pos_);
    base_.setScale({ 0.9f, 0.9f });

    icon_.setOrigin({ baseTex.getSize().x * 0.5f, baseTex.getSize().y * 0.5f });
    icon_.setPosition(pos_);
    icon_.setScale({ 0.5f, 0.5f }); // maybe smaller than base
}




void CannonTower::tryFireAt(sf::Vector2f mouseWorld, CreatureSystem& creeps){
    if (ammo_ <= 0 || cdTimer_ > 0.f) return;

    sf::Vector2f d = mouseWorld - pos_;
    float L2 = d.x*d.x + d.y*d.y;
    if (L2 > radius_*radius_) return; // out of range

    // projectile
    Projectile p;
    p.pos = pos_;
    p.target = mouseWorld;
    // initial kick
    float L = std::sqrt(L2); sf::Vector2f dir = (L>1e-4f) ? (d/L) : sf::Vector2f(1,0);
    p.vel = dir * 420.f;
    projectiles_.push_back(p);

    // muzzle flash (short-lived decal drawn via recoil)
    recoilT_ = 0.10f;

    ammo_--;
    cdTimer_ = cooldown_;
}

void CannonTower::update(float dt, CreatureSystem& creeps){
    if (cdTimer_ > 0.f) cdTimer_ -= dt;
    if (recoilT_ > 0.f) recoilT_ -= dt;

    for (auto& prj : projectiles_){
        const bool hit = prj.update(dt);
        if (hit){
            // impact FX: small shockwave (drawn below in draw())
            // Apply damage & collect dead → ask CreatureSystem for drops
            std::vector<MaterialDrop> drops;
            creeps.applyDamagePoint(prj.target, /*radius*/28.f, /*dmg*/14, drops);

            // TODO: send drops to your DropSystem / inventory
        }
    }
    projectiles_.erase(
        std::remove_if(projectiles_.begin(), projectiles_.end(),
                       [](const Projectile& p){ return !p.alive; }),
        projectiles_.end()
    );
}

void CannonTower::draw(sf::RenderTarget& rt, bool showRadius) const{
    if (showRadius){
        sf::CircleShape r(radius_);
        r.setOrigin(sf::Vector2f(radius_,radius_));
        r.setPosition(pos_);
        r.setFillColor(sf::Color(0,0,0,0));
        r.setOutlineThickness(3.f);
        r.setOutlineColor(sf::Color(90,210,120,140));
        rt.draw(r);
    }

    // direction line to cursor (optional; color green/red handled by caller)
    // (see §5 below)

    // cannon sprite + muzzle flash
    sf::Sprite spr = icon_;
    if (recoilT_ > 0.f){
        spr.move(sf::Vector2f(0.f, -8.f * (recoilT_ / 0.10f)));
        sf::CircleShape flash(14.f * (recoilT_/0.10f));
        flash.setOrigin(sf::Vector2f(flash.getRadius(), flash.getRadius()));
        flash.setPosition(pos_ + sf::Vector2f(0.f, -18.f));
        flash.setFillColor(sf::Color(255,240,180,180));
        rt.draw(flash);
    }
    rt.draw(spr);

    // projectiles
    for (const auto& p : projectiles_) p.draw(rt);
}
