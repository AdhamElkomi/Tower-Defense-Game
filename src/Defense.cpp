
#include "Defense.hpp"
#include "CreatureSystem.hpp"
#include <algorithm>
#include <cmath>
#include <SFML/Graphics.hpp>
#include <memory>
#include <random>

CannonTower::CannonTower(sf::Vector2f center, const sf::Texture& baseTex, const sf::Texture& cannonBallTex)
: pos_(center), base_(baseTex)
{
    base_.setOrigin(sf::Vector2f(baseTex.getSize().x * 0.5f, baseTex.getSize().y * 0.5f));
    base_.setPosition(pos_);
    base_.setScale(sf::Vector2f(0.5f, 0.5f)); // smaller icon if needed

    // Preload cannon ball texture for projectiles
    cannonBallTexture_ = &cannonBallTex;
}


// ===================== ArcherTower =====================
ArcherTower::ArcherTower(sf::Vector2f center, const sf::Texture& baseTex, const sf::Texture& arrowTex)
    : pos_(center), base_(baseTex)
{
    base_.setOrigin(sf::Vector2f(baseTex.getSize().x * 0.5f, baseTex.getSize().y * 0.5f));
    base_.setPosition(pos_);
    base_.setScale(sf::Vector2f(0.5f, 0.5f));

    // Preload arrow texture for projectiles
    arrowTexture_ = &arrowTex;
}

void ArcherTower::update(float dt, CreatureSystem& creeps) {
    if (cd_ > 0.f) cd_ -= dt;

    // update arrows + generate impacts
    for (auto& arrow : arrows_) {
        bool hit = arrow.update(dt, creeps);
        if (hit) {
            // damage + drops
            std::vector<MaterialDrop> drops;
            creeps.applyDamagePoint(arrow.target, power_, 40, drops);
            // queue a short impact flash
            impacts_.push_back({arrow.target, 0.18f});
            // keep a copy for the tower (so GameScene can pick them up this frame)
            dropBatch_.insert(dropBatch_.end(), drops.begin(), drops.end());
        }
    }
    arrows_.erase(std::remove_if(arrows_.begin(), arrows_.end(),
                       [](const ArrowProjectile& p){ return !p.alive; }),
                       arrows_.end());

    // update impact flashes
    for (auto& im : impacts_) im.t -= dt;
    impacts_.erase(std::remove_if(impacts_.begin(), impacts_.end(),
                       [](const ImpactBurst& i){ return i.t<=0.f; }),
                       impacts_.end());
}

void ArcherTower::tryFireAt(sf::Vector2f mouseWorld, CreatureSystem& creeps) {
    if (shotsLeft_ <= 0 || cd_ > 0.f) return;

    // must be in range
    sf::Vector2f d = mouseWorld - pos_;
    if (d.x*d.x + d.y*d.y > radius_*radius_) return;

    // compute direction
    float len = std::sqrt(d.x*d.x + d.y*d.y);
    sf::Vector2f dir = (len > 0.f) ? (d / len) : sf::Vector2f(0,0);

    // perpendicular for spread
    sf::Vector2f perp(-dir.y, dir.x);
    float spreadDist = 15.f; // smaller spread distance

    // spawn single arrow that flies to the end of range if no hit
    ArrowProjectile arrow;
    arrow.pos = pos_;
    arrow.vel = sf::Vector2f(0,0);
    arrow.target = mouseWorld;
    arrow.life = 2.0f; // longer life to reach end of range
    arrow.sprite = std::make_unique<sf::Sprite>(*arrowTexture_);
    arrow.sprite->setOrigin(sf::Vector2f(arrowTexture_->getSize().x * 0.5f, arrowTexture_->getSize().y * 0.5f));
    arrow.sprite->setScale(sf::Vector2f(0.1f, 0.1f)); // Much smaller arrow
    arrows_.push_back(std::move(arrow));

    // ammo / cooldown
    shotsLeft_--;
    cd_ = cooldown_;
}

void MageTower::tryFireAt(sf::Vector2f mouseWorld, CreatureSystem& creeps) {
    if (shotsLeft_ <= 0) return;
    // TODO: Ajoutez la logique de projectile réel ici
    // Appliquer les dégâts personnalisés
    std::vector<MaterialDrop> drops;
    creeps.applyDamagePoint(mouseWorld, power_, 40, drops);
    shotsLeft_--;
}

void ArcherTower::draw(sf::RenderTarget& rt, bool showRadius) const {
    // range (optional when not aiming)
    if (showRadius){
        sf::CircleShape r(radius_);
        r.setOrigin(sf::Vector2f(radius_, radius_));
        r.setPosition(pos_);
        r.setFillColor(sf::Color(0,0,0,0));
        r.setOutlineThickness(1.f);
        r.setOutlineColor(sf::Color(80,180,100,120));
        rt.draw(r);
    }

    // draw base
    rt.draw(base_);

    // draw arrows
    for (const auto& arrow : arrows_) arrow.draw(rt);

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

sf::Vector2f ArcherTower::pos() const { return pos_; }
bool ArcherTower::isDead() const { return shotsLeft_ <= 0; }
void ArcherTower::extractDrops(std::vector<MaterialDrop>& out) { /* TODO: drops spécifiques */ }


// ===================== MageTower =====================
MageTower::MageTower(sf::Vector2f center, const sf::Texture& baseTex)
    : pos_(center), base_(baseTex)
{
    base_.setOrigin(sf::Vector2f(baseTex.getSize().x * 0.5f, baseTex.getSize().y * 0.5f));
    base_.setPosition(pos_);
    base_.setScale(sf::Vector2f(0.5f, 0.5f));
}

void MageTower::update(float dt, CreatureSystem& creeps) {
    // TODO: Implémenter la logique de tir du mage (projectile lent, dégâts de zone, etc.)
}

void MageTower::draw(sf::RenderTarget& rt, bool showRadius) const {
    // Affiche le rayon si demandé
    if (showRadius) {
        float r = 120.f;
        sf::CircleShape range(r);
        range.setOrigin(sf::Vector2f(r, r));
        range.setPosition(pos_);
        range.setFillColor(sf::Color(0,0,0,0));
        range.setOutlineThickness(1.f);
        range.setOutlineColor(sf::Color(80,100,180,120));
        rt.draw(range);
    }
    // Dessine la base de la tour
    rt.draw(base_);
    // TODO: dessiner les projectiles du mage
}

sf::Vector2f MageTower::pos() const { return pos_; }
bool MageTower::isDead() const { return shotsLeft_ <= 0; }
void MageTower::extractDrops(std::vector<MaterialDrop>& out) { /* TODO: drops spécifiques */ }





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
    p.sprite = std::make_unique<sf::Sprite>(*cannonBallTexture_);
    p.sprite->setOrigin(sf::Vector2f(cannonBallTexture_->getSize().x * 0.5f, cannonBallTexture_->getSize().y * 0.5f));
    p.sprite->setScale(sf::Vector2f(0.04f, 0.04f)); // Small cannon ball
    projectiles_.push_back(std::move(p));

    // FX
    recoilT_ = 0.10f;
    muzzleT_ = 0.06f;


    // ammo / cooldown
    shotsLeft_--;
    cd_ = cooldown_;
}

bool CannonTower::isDead() const {
    return shotsLeft_ <= 0;
    return shotsLeft_ <= 0;
    return shotsLeft_ <= 0;
}

void CannonTower::extractDrops(std::vector<MaterialDrop>& out) {
    // Déplacer les drops collectés dans dropBatch_ vers le vecteur out
    out.insert(out.end(), dropBatch_.begin(), dropBatch_.end());
    dropBatch_.clear();
}


bool Projectile::update(float dt, CreatureSystem& creeps){
    if (!alive) return false;
    sf::Vector2f d = target - pos;
    float L2 = d.x*d.x + d.y*d.y;
    float L  = std::sqrt(L2);
    sf::Vector2f dir = (L>1e-4f) ? (d / L) : sf::Vector2f(0,0);

    // snappy ballistic feel
    vel += dir * 2600.f * dt;
    vel *= 0.985f;
    pos += vel * dt;

    // update sprite position
    if (sprite) {
        sprite->setPosition(pos);
    }

    // trail puffs
    trailSpawn -= dt;
    if (trailSpawn <= 0.f){
        trailSpawn = 0.012f;
        trail.push_back({pos, 0.22f});
    }
    for (auto& t : trail) t.t -= dt;
    trail.erase(std::remove_if(trail.begin(), trail.end(),
               [](const Trail& tr){ return tr.t<=0.f; }), trail.end());

    // check for hits on creatures
    std::vector<MaterialDrop> dummyDrops;
    if (creeps.applyDamagePoint(pos, 0.f, 15.f, dummyDrops)) { // small radius hit check
        alive = false;
        return true; // hit
    }

    life -= dt;
    if (L < 14.f || life <= 0.f){ alive = false; return true; }
    return false;
}

bool ArrowProjectile::update(float dt, CreatureSystem& creeps){
    if (!alive) return false;
    sf::Vector2f d = target - pos;
    float L2 = d.x*d.x + d.y*d.y;
    float L  = std::sqrt(L2);
    sf::Vector2f dir = (L>1e-4f) ? (d / L) : sf::Vector2f(0,0);

    // faster and straighter than cannon
    vel += dir * 3200.f * dt;
    vel *= 0.99f;
    pos += vel * dt;

    // calculate rotation for arrow pointing towards target
    rotation = std::atan2(dir.y, dir.x) * 180.f / 3.14159f;
    sprite->setPosition(pos);
    sprite->setRotation(sf::degrees(rotation));

    // check for hits on creatures
    std::vector<MaterialDrop> dummyDrops;
    if (creeps.applyDamagePoint(pos, 0.f, 10.f, dummyDrops)) { // small radius hit check
        alive = false;
        return true; // hit
    }

    life -= dt;
    if (L < 10.f || life <= 0.f){ alive = false; return true; }
    return false;
}

void Projectile::draw(sf::RenderTarget& rt) const{
    // draw sprite if available
    if (sprite) {
        rt.draw(*sprite);
    } else {
        // fallback to old drawing
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
}

void ArrowProjectile::draw(sf::RenderTarget& rt) const{
    if (sprite) rt.draw(*sprite);
}


void CannonTower::update(float dt, CreatureSystem& creeps){
    if (cd_ > 0.f) cd_ -= dt;
    if (recoilT_ > 0.f) recoilT_ -= dt;
    if (muzzleT_ > 0.f) muzzleT_ -= dt;

    // update projectiles + generate impacts
    for (auto& prj : projectiles_){
        bool hit = prj.update(dt, creeps);
        if (hit){
            // damage + drops
            std::vector<MaterialDrop> drops;
            creeps.applyDamagePoint(prj.target, power_, 40, drops);
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
        base.setScale(sf::Vector2f(0.5f + 0.05f*k, 0.5f + 0.05f*k));
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

