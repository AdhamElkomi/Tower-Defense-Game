
#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <optional>
#include <string>
#include <SFML/Audio.hpp>
#include <memory>

enum class CreatureType { Grunt, Rogue, Golem };

struct CreatureDef {
    CreatureType type;
    std::string  texturePath;
    sf::Vector2i frameSize {64,64};
    int          framesPerRow = 8;  // nb de frames par direction
    float        animFps      = 10.f;

    // Stats
    int   hpMax;
    float speed;     // pixels/s (ou tuiles/s * tileSize)
    int   armor;     // réduction simple
    int   bounty;    // argent donné à la mort
    float scale = 1.f; // pour grossir le golem, etc.
    float footAnchorY = 0.88f;   // 0..1, 0.88 ≈ pieds vers le bas

    // ➜ NOUVEAU : pour corriger un découpage qui “mange” ou décale le perso
    sf::Vector2i sheetOrigin   = {0, 0};   // marge gauche/haut du premier frame
    sf::Vector2i sheetSpacing  = {0, 0};   // espacement horizontal/vertical entre deux frames
    int          rows          = 4;        // nb de lignes (directions)

};

struct WaypointPath {
    std::vector<sf::Vector2f> pts; // polyline en px (ou tuile*tileSize)
};

class Creature {
public:
    Creature(const CreatureDef& def, const sf::Texture& tex, const WaypointPath& path, const sf::SoundBuffer* loopBuffer = nullptr);

    bool  isDead() const { return hp_ <= 0; }
    void  hit(int dmg); // applique armure
    void  update(float dt); // avance le long du chemin + anime
    void  draw(sf::RenderTarget& rt) const;
    // Creature.hpp
    const CreatureDef def_;
    void setScale(float s);
    CreatureType type() const { return def_.type; }



    // position écran (centre du sprite)
    sf::Vector2f pos() const { return pos_; }

    void stopAudio() { if (loop_) loop_->stop(); }

    // Check if creature has reached the end of the path (exit)
    bool hasReachedExit() const { return pathIdx_ >= path_.pts.size(); }

    bool hasCollectedResource() const { return collectedResource_; }
    void setCollectedResource(bool collected) { collectedResource_ = collected; }

    bool hasReachedExitWithResource() const { return hasReachedExit() && collectedResource_; }

private:
    
    const sf::Texture* tex_;
    sf::Sprite sprite_;          // SFML3 : spr crée avec texture dans le ctor

    // Animation
    int   dirIndex_ = 2;         // 0:N 1:E 2:S 3:O
    int   frame_    = 0;
    float animTimer_= 0.f;

    // Mouvement
    WaypointPath path_;
    std::size_t  pathIdx_ = 0;   // point cible courant
    sf::Vector2f pos_;
    float        reachedEps_ = 4.f;

    // PV
    int   hp_;

    // Resource collection flag
    bool collectedResource_ = false;

    // Burn status
    float burnTime_ = 0.f;      // seconds remaining
    float burnDamage_ = 0.f;    // damage per second

public:
    // Public accessors for burn status
    void applyBurn(float time, float dps) {
        burnTime_ = std::max(burnTime_, time);
        burnDamage_ = std::max(burnDamage_, dps);
    }

    // audio loop (créé seulement si un buffer est fourni)
    std::unique_ptr<sf::Sound> loop_;
};

