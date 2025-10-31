
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
    if (shotsLeft_ <= 0 || isHolding_) return;

    // Only fire if gauge is full
    if (holdTime_ < maxHold_) {
        // If not full, start holding
        isHolding_ = true;
        return;
    }

    // Calculate power based on hold time (should be max since full)
    float powerMultiplier = 1.0f; // always full power when firing
    int numProjectiles = 15 + static_cast<int>(15 * powerMultiplier); // 15 to 30 projectiles for denser arc

    // Spawn projectiles in a semicircle
    float startAngle = -M_PI / 2; // -90 degrees (left)
    float endAngle = M_PI / 2;    // 90 degrees (right)
    float angleStep = (endAngle - startAngle) / (numProjectiles - 1);

    for (int i = 0; i < numProjectiles; ++i) {
        float angle = startAngle + i * angleStep;
        MageFireProjectile proj;
        proj.pos = pos_;
        proj.vel = sf::Vector2f(std::cos(angle), std::sin(angle)) * 400.f; // faster speed
        proj.angle = angle;
        proj.life = radius_ / 400.f; // time to reach max range
        proj.powerMultiplier = powerMultiplier; // set power multiplier
        projectiles_.push_back(std::move(proj));
    }

    // Reset hold
    isHolding_ = false;
    holdTime_ = 0.f;
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
    isHolding_ = true; // Start holding automatically upon placement
}

void MageTower::update(float dt, CreatureSystem& creeps) {
    // Handle recharge cooldown after firing
    if (rechargeCooldown_ > 0.f) {
        rechargeCooldown_ -= dt;
        if (rechargeCooldown_ <= 0.f) {
            // Start recharging gauge after cooldown
            isHolding_ = true;
            holdTime_ = 0.f;
        }
    }

    // Handle hold mechanics (recharging gauge)
    if (isHolding_) {
        holdTime_ += dt;
        if (holdTime_ > maxHold_) holdTime_ = maxHold_;
    }

    // Automatic firing when gauge is full
    if (isGaugeFull() && shotsLeft_ > 0) {
        sf::Vector2f target = creeps.findClosestInRange(pos_, radius_);
        if (target != sf::Vector2f(0, 0)) { // valid target found
            // Fire sweeping arc of 7 projectiles in a fan towards the target direction
            sf::Vector2f dir = target - pos_;
            float baseAngle = std::atan2(dir.y, dir.x);
            float arcAngle = 120.f * 3.14159f / 180.f; // 120 degrees arc
            int numProjectiles = 7;
            float angleStep = arcAngle / (numProjectiles - 1);

            for (int i = 0; i < numProjectiles; ++i) {
                float angle = baseAngle - arcAngle/2 + i * angleStep;
                MageFireProjectile proj;
                proj.pos = pos_;
                proj.vel = sf::Vector2f(std::cos(angle), std::sin(angle)) * 250.f; // moderate speed
                proj.angle = angle;
                proj.life = radius_ / 250.f; // reach max range
                proj.powerMultiplier = 1.0f; // full power
                projectiles_.push_back(std::move(proj));
            }

            // Reset hold, start recharge cooldown, and consume shot
            isHolding_ = false;
            holdTime_ = 0.f;
            rechargeCooldown_ = 2.0f; // 2 second cooldown before recharging
            shotsLeft_--;
        }
    }

    // Update projectiles
    for (auto& proj : projectiles_) {
        if (proj.update(dt, creeps)) {
            // Hit something, apply damage + burn using stored powerMultiplier
            float damage = power_ * (1.0f + proj.powerMultiplier);
            std::vector<MaterialDrop> drops;
            creeps.applyDamagePoint(proj.pos, damage, 25.f, drops, true, 3.0f, 15.f); // burn for 3s at 15 DPS
            // Queue impacts or effects if needed
        }
    }
    projectiles_.erase(std::remove_if(projectiles_.begin(), projectiles_.end(),
                                      [](const MageFireProjectile& p){ return !p.alive; }),
                       projectiles_.end());
}

void MageTower::draw(sf::RenderTarget& rt, bool showRadius) const {
    // Affiche le rayon si demandé
    if (showRadius) {
        sf::CircleShape range(radius_);
        range.setOrigin(sf::Vector2f(radius_, radius_));
        range.setPosition(pos_);
        range.setFillColor(sf::Color(0,0,0,0));
        range.setOutlineThickness(1.f);
        range.setOutlineColor(sf::Color(80,100,180,120));
        rt.draw(range);
    }
    // Dessine la base de la tour
    rt.draw(base_);

    // Draw power gauge if holding
    if (isHolding_) {
        float gaugeHeight = 20.f;
        float gaugeWidth = 100.f;
        sf::Vector2f gaugePos = pos_ + sf::Vector2f(-gaugeWidth/2, -60.f);
        sf::RectangleShape bg(sf::Vector2f(gaugeWidth, gaugeHeight));
        bg.setPosition(gaugePos);
        bg.setFillColor(sf::Color(50,50,50,200));
        bg.setOutlineThickness(2.f);
        bg.setOutlineColor(sf::Color::White);
        rt.draw(bg);

        float fillRatio = holdTime_ / maxHold_;
        sf::RectangleShape fill(sf::Vector2f(gaugeWidth * fillRatio, gaugeHeight));
        fill.setPosition(gaugePos);
        fill.setFillColor(sf::Color(255,100,0,200)); // orange
        rt.draw(fill);
    }

    // Draw projectiles
    for (const auto& proj : projectiles_) {
        proj.draw(rt);
    }
}

sf::Vector2f MageTower::pos() const { return pos_; }
bool MageTower::isDead() const { return shotsLeft_ <= 0; }
void MageTower::extractDrops(std::vector<MaterialDrop>& out) { /* TODO: drops spécifiques */ }

void MageTower::startHolding() {
    isHolding_ = true;
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
    sprite->setRotation(rotation);

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

bool MageFireProjectile::update(float dt, CreatureSystem& creeps){
    if (!alive) return false;
    pos += vel * dt;
    life -= dt;

    // Add trail particles
    trailTimer += dt;
    if (trailTimer >= 0.05f) { // every 0.05s
        trail.push_back(pos);
        if (trail.size() > 20) trail.erase(trail.begin()); // limit trail length
        trailTimer = 0.f;
    }

    if (life <= 0.f){ alive = false; return true; } // reached range

    // Check for hits during movement (continuous damage)
    std::vector<MaterialDrop> dummyDrops;
    if (creeps.applyDamagePoint(pos, 25.f, 0.f, dummyDrops, true, 0.1f, 5.f)) { // small damage radius, short burn
        // Don't kill projectile on hit, let it continue sweeping
        // alive = false; // commented out to allow continuous damage
        // return true; // commented out
    }
    return false; // no hit to report for projectile lifetime
}

void MageFireProjectile::draw(sf::RenderTarget& rt) const{
    // Draw thicker trail particles for sweeping wave effect
    for (size_t i = 0; i < trail.size(); ++i) {
        float alpha = (float)i / trail.size() * 255.f;
        sf::CircleShape particle(8.f); // larger particles for thickness
        particle.setOrigin(sf::Vector2f(8.f, 8.f));
        particle.setPosition(trail[i]);
        particle.setFillColor(sf::Color(255, 100, 0, static_cast<std::uint8_t>(alpha))); // fading orange
        rt.draw(particle);
    }

    float baseSize = 35.f; // even larger for imposing wave
    float size = baseSize * (1.0f + powerMultiplier * 0.5f); // size varies from 35 to 52.5
    sf::CircleShape fire(size); // massive fire balls
    fire.setOrigin(sf::Vector2f(size, size));
    fire.setPosition(pos);
    fire.setFillColor(sf::Color(255, 69, 0, 255)); // bright orange-red
    rt.draw(fire);

    // Add a stronger glow for wave effect
    float glowSize = 70.f * (1.0f + powerMultiplier * 0.5f); // glow size varies from 70 to 105
    sf::CircleShape glow(glowSize);
    glow.setOrigin(sf::Vector2f(glowSize, glowSize));
    glow.setPosition(pos);
    glow.setFillColor(sf::Color(255, 140, 0, 120)); // brighter, more yellow glow
    rt.draw(glow);

    // Add inner core for intensity
    sf::CircleShape core(size * 0.6f);
    core.setOrigin(sf::Vector2f(size * 0.6f, size * 0.6f));
    core.setPosition(pos);
    core.setFillColor(sf::Color(255, 255, 0, 220)); // yellow core
    rt.draw(core);
}

