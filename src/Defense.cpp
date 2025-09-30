#include "Defense.hpp"
#include "CreatureSystem.hpp"
#include <algorithm>
#include <cmath>

CannonTower::CannonTower(sf::Vector2f center, const sf::Texture& baseTex)
: pos_(center), base_(baseTex)
{
    base_.setOrigin(sf::Vector2f(baseTex.getSize().x * 0.5f, baseTex.getSize().y * 0.5f));
    base_.setPosition(pos_);
    base_.setScale(sf::Vector2f(0.65f, 0.65f)); // smaller icon if needed
}





void CannonTower::tryFireAt(sf::Vector2f mouseWorld, CreatureSystem& creeps){
    if (shotsLeft_ <= 0) return;
    if (cd_ > 0.f) return;

    // must be in range
    sf::Vector2f d = mouseWorld - pos_;
    if (d.x*d.x + d.y*d.y > radius_*radius_) return;

    // spawn projectile
    Projectile p;
    p.pos = pos_;
    p.vel = sf::Vector2f(0,0);
    p.target = mouseWorld;
    p.life = 1.1f;
    projectiles_.push_back(p);

    // FX
    recoilT_ = 0.10f;
    muzzleT_ = 0.06f;

    // ammo / cooldown
    shotsLeft_--;
    cd_ = cooldown_;
}


bool Projectile::update(float dt){
    if (!alive) return false;
    sf::Vector2f d = target - pos;
    float L2 = d.x*d.x + d.y*d.y;
    float L  = std::sqrt(L2);
    sf::Vector2f dir = (L>1e-4f) ? (d / L) : sf::Vector2f(0,0);

    // snappy ballistic feel
    vel += dir * 2600.f * dt;
    vel *= 0.985f;
    pos += vel * dt;

    // trail puffs
    trailSpawn -= dt;
    if (trailSpawn <= 0.f){
        trailSpawn = 0.012f;
        trail.push_back({pos, 0.22f});
    }
    for (auto& t : trail) t.t -= dt;
    trail.erase(std::remove_if(trail.begin(), trail.end(),
               [](const Trail& tr){ return tr.t<=0.f; }), trail.end());

    life -= dt;
    if (L < 14.f || life <= 0.f){ alive = false; return true; }
    return false;
}

void Projectile::draw(sf::RenderTarget& rt) const{
    for (auto & tr : trail){
        float a = std::clamp(tr.t/0.22f, 0.f, 1.f);
        sf::CircleShape puff(7.f*(1.f-a));
        puff.setOrigin(sf::Vector2f(puff.getRadius(), puff.getRadius()));
        puff.setPosition(tr.p);
        puff.setFillColor(sf::Color(240,210,160, (std::uint8_t)(120*a)));
        rt.draw(puff);
    }
    sf::CircleShape core(4.5f);
    core.setOrigin(sf::Vector2f(4.5f,4.5f));
    core.setPosition(pos);
    core.setFillColor(sf::Color(255,230,120));
    rt.draw(core);

    sf::CircleShape glow(12.f);
    glow.setOrigin(sf::Vector2f(12.f,12.f));
    glow.setPosition(pos);
    glow.setFillColor(sf::Color(255,180,80,130));
    rt.draw(glow);
}


void CannonTower::update(float dt, CreatureSystem& creeps){
    if (cd_ > 0.f) cd_ -= dt;
    if (recoilT_ > 0.f) recoilT_ -= dt;
    if (muzzleT_ > 0.f) muzzleT_ -= dt;

    // update projectiles + generate impacts
    for (auto& prj : projectiles_){
        bool hit = prj.update(dt);
        if (hit){
            // damage + drops
            std::vector<MaterialDrop> drops;
            creeps.applyDamagePoint(prj.target, 50.f, 40, drops);
            // queue a short impact flash
            impacts_.push_back({prj.target, 0.18f});
            // keep a copy for the tower (so GameScene can pick them up this frame)
            dropBatch_.insert(dropBatch_.end(), drops.begin(), drops.end());
        }
    }
    projectiles_.erase(std::remove_if(projectiles_.begin(), projectiles_.end(),
                       [](const Projectile& p){ return !p.alive; }),
                       projectiles_.end());

    // update impact flashes
    for (auto& im : impacts_) im.t -= dt;
    impacts_.erase(std::remove_if(impacts_.begin(), impacts_.end(),
                       [](const ImpactBurst& i){ return i.t<=0.f; }),
                       impacts_.end());
}


void CannonTower::draw(sf::RenderTarget& rt, bool showRadius) const{
    // range (optional when not aiming)
    if (showRadius){
        sf::CircleShape r(radius_);
        r.setOrigin(sf::Vector2f(radius_, radius_));
        r.setPosition(pos_);
        r.setFillColor(sf::Color(0,0,0,0));
        r.setOutlineThickness(1.f);
        r.setOutlineColor(sf::Color(80,80,100,120));
        rt.draw(r);
    }

    // recoil visual: slight push back + subtle scale
    sf::Sprite base = base_;
    if (recoilT_ > 0.f){
        float k = recoilT_ / 0.10f;
        base.setScale(sf::Vector2f(0.65f + 0.05f*k, 0.65f + 0.05f*k));
    }
    rt.draw(base);

    // muzzle flash
    if (muzzleT_ > 0.f){
        float a = std::clamp(muzzleT_/0.06f, 0.f, 1.f);
        sf::CircleShape flash(14.f + 12.f*a);
        flash.setOrigin(sf::Vector2f(flash.getRadius(), flash.getRadius()));
        flash.setPosition(pos_);
        flash.setFillColor(sf::Color(255,220,120, (std::uint8_t)(180*a)));
        rt.draw(flash);
    }

    // projectiles
    for (const auto& p : projectiles_) p.draw(rt);

    // impact bursts
    for (const auto& im : impacts_){
        float a = std::clamp(im.t/0.18f, 0.f, 1.f);
        sf::CircleShape ring(22.f*(1.f-a));
        ring.setOrigin(sf::Vector2f(ring.getRadius(), ring.getRadius()));
        ring.setPosition(im.p);
        ring.setFillColor(sf::Color(0,0,0,0));
        ring.setOutlineThickness(3.f*(a));
        ring.setOutlineColor(sf::Color(255,200,120, (std::uint8_t)(180*a)));
        rt.draw(ring);
    }
}

