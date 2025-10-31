// Creature.cpp
#include "Creature.hpp"
#include <cmath>
#include <memory>
// Creature.cpp
#include <SFML/Audio.hpp>

static int directionIndexFromVel(const sf::Vector2f& v){
    if (std::abs(v.x) > std::abs(v.y)) {
        // >>> Lignes de ton sheet : 0 = droite(E), 1 = gauche(W)
        return (v.x >= 0.f) ? 0 : 1;
    } else {
        // Pas de rangée "face/Sud" : on réutilise la rangée 0 pour S
        // Rangée 2 = dos (Nord)
        return (v.y >= 0.f) ? 0 : 2;
    }
}
// SFML 3 : IntRect = { position, size }
static sf::IntRect frameRect(const CreatureDef& def, int dirIndex, int frame){
    const int w = def.frameSize.x;
    const int h = def.frameSize.y;

    const int x = def.sheetOrigin.x + frame   * (w + def.sheetSpacing.x);
    const int y = def.sheetOrigin.y + dirIndex* (h + def.sheetSpacing.y);

    return sf::IntRect{ sf::Vector2i{x, y}, sf::Vector2i{w, h} };
}


void Creature::setScale(float s){
    sprite_.setScale(sf::Vector2f(s, s));
}


// --------------------------------- Creature --------------------------------

Creature::Creature(const CreatureDef& def,
                   const sf::Texture& tex,
                   const WaypointPath& path,
                   const sf::SoundBuffer* loopBuffer)
: def_(def), tex_(&tex), sprite_(tex), path_(path), hp_(def.hpMax)
{
    // Échelle : utilise def.scale (tu peux l’ajuster dans CreatureSystem)
    sprite_.setScale(sf::Vector2f(def.scale, def.scale));

    // départ au premier point
    pos_ = path_.pts.empty() ? sf::Vector2f{} : path_.pts.front();

    // Origine centrée (SFML 3 : Vector2f)
    

    const float foot = def.footAnchorY > 0.f ? def.footAnchorY : 0.80f; // 88% de la hauteur
    sprite_.setOrigin(sf::Vector2f(def.frameSize.x * 0.5f, def.frameSize.y * foot));

    // Premier frame/direction
    sprite_.setTextureRect(frameRect(def_, dirIndex_, frame_));
    sprite_.setPosition(pos_);


     // 🔊 son continu tant que la créature est vivante
    if (loopBuffer){
        loop_ = std::make_unique<sf::Sound>(*loopBuffer); // SFML2: ctor avec buffer
         loop_->setLoop(true);
        loop_->setVolume(40.f);      // ajuste par type si tu veux
        loop_->play();
    }



    
}

void Creature::hit(int dmg){
    int real = std::max(1, dmg - def_.armor);
    hp_ -= real;
}

void Creature::update(float dt){
    // Burn damage over time
    if (burnTime_ > 0.f) {
        burnTime_ -= dt;
        if (burnTime_ <= 0.f) {
            burnTime_ = 0.f;
            burnDamage_ = 0.f;
        } else {
            int burnDmg = static_cast<int>(burnDamage_ * dt);
            if (burnDmg > 0) {
                hit(burnDmg);
            }
        }
    }

    // Mouvement
    if (pathIdx_+1 < path_.pts.size()){
        sf::Vector2f target = path_.pts[pathIdx_+1];
        sf::Vector2f v      = target - pos_;
        float d = std::sqrt(v.x*v.x + v.y*v.y);
        if (d > 0.0001f){
            sf::Vector2f dir = (1.f/d) * v;
            pos_ += dir * (def_.speed * dt);
            sprite_.setPosition(pos_);
            // Direction anim
            dirIndex_ = directionIndexFromVel(dir);
            // Arrivé au prochain waypoint ?
            if (std::abs(target.x - pos_.x) <= reachedEps_ &&
                std::abs(target.y - pos_.y) <= reachedEps_){
                pathIdx_++;
            }
        }else{
            pathIdx_++;
        }
    }

    // Animation
    animTimer_ += dt;
    const float frameDur = 1.f / std::max(1.f, def_.animFps);
    if (animTimer_ >= frameDur){
        animTimer_ -= frameDur;
        frame_ = (frame_ + 1) % def_.framesPerRow;
        sprite_.setTextureRect(frameRect(def_, dirIndex_, frame_));
    }
}

void Creature::draw(sf::RenderTarget& rt) const{
    // // Ombre légère
    // sf::Sprite shadow = sprite_;
    // shadow.setColor(sf::Color(0,0,0,90));
    // shadow.setScale(sf::Vector2f(def_.scale*1.05f, def_.scale*0.7f));
    // shadow.move(sf::Vector2f(0.f, 10.f));
    // rt.draw(shadow);

    rt.draw(sprite_);

    // Burn effect: fire particles if burning
    if (burnTime_ > 0.f) {
        // Draw 3-5 small fire particles around the creature
        for (int i = 0; i < 5; ++i) {
            float angle = (i * 72.f) * 3.14159f / 180.f; // 72 degrees apart
            sf::Vector2f offset(std::cos(angle) * 10.f, std::sin(angle) * 10.f);
            sf::CircleShape flame(3.f + (i % 2) * 2.f);
            flame.setOrigin(sf::Vector2f(flame.getRadius(), flame.getRadius()));
            flame.setPosition(pos_ + offset);
            flame.setFillColor(sf::Color(255, 100 + i * 20, 0, 200));
            rt.draw(flame);
        }
    }

    // Barre de vie (au-dessus)
    float w = def_.frameSize.x * def_.scale * 0.5;
    float h = 6.f;
    float ratio = std::max(0.f, (float)hp_ / (float)def_.hpMax);
    sf::Vector2f base = pos_ + sf::Vector2f(-w*0.5f, -def_.frameSize.y*0.25f*def_.scale);

    sf::RectangleShape bg(sf::Vector2f(w, h));
    bg.setPosition(base);
    bg.setFillColor(sf::Color(0,0,0,120));

    sf::RectangleShape fg(sf::Vector2f(w * ratio, h));
    fg.setPosition(base);
    // couleur en fonction des PV
    sf::Color good(60, 220, 90), mid(250, 200, 60), low(230, 60, 60);
    fg.setFillColor(ratio > 0.6f ? good : (ratio > 0.3f ? mid : low));

    rt.draw(bg);
    rt.draw(fg);
}
