// CreatureSystem.hpp
#pragma once
#include "Creature.hpp"
#include "Defense.hpp"
#include <memory>
#include <unordered_map>
#include <SFML/Audio.hpp>   // ⬅️ IMPORTANT
#include <memory>
#include <unordered_map>
#include <vector>  


// Material types you already use in the menu
// Near the top, shared by GameScene
/*enum class Material { Wood=0, Stone=1, Crystal=2 };

struct MaterialDrop {
    Material type;
    sf::Vector2f pos; // world position where it appears (optional for your UX)
};*/


class CreatureSystem {
public:
    explicit CreatureSystem(float tileSize);

    // defs
    void loadTextures(); // charge les 3 spritesheets

     void loadSounds(); 
     


    // path global (entrée -> gate -> sortie) en px
    void setPath(const WaypointPath& p);

    // spawns
    void spawn(CreatureType t, float atTime);        // une créature plus tard
    void spawnWave(int nGrunt, int nRogue, int nGolem, float startTime, float period);

    // cycle
    void update(float dt, float timeNow);
    void draw(sf::RenderTarget& rt) const;
     // Returns true if something was hit.
    bool hitFirstAt(sf::Vector2f p, float r, int dmg);
    int applyDamagePoint(sf::Vector2f center, float radius, int dmg,
                                     std::vector<MaterialDrop>& outDrops);

    // NEW: move pending drops out (for GameScene)
    void extractPendingDrops(std::vector<MaterialDrop>& out) {
        out.insert(out.end(), pendingDrops_.begin(), pendingDrops_.end());
        pendingDrops_.clear();
    }



private:
    float tileSize_;
    WaypointPath path_;

    std::unordered_map<CreatureType, CreatureDef> defs_;
    std::unordered_map<CreatureType, std::unique_ptr<sf::Texture>> textures_;

    // ---------- NEW: audio ----------
    std::unordered_map<CreatureType, std::unique_ptr<sf::SoundBuffer>> soundBufs_;
    mutable std::vector<sf::Sound> activeSounds_;    // instances en cours
    void playSound(CreatureType t, float vol = 100.f);



    struct Scheduled {
        CreatureType type;
        float t;
    };
    std::vector<Scheduled> timeline_;

    std::vector<std::unique_ptr<Creature>> alive_;

    std::vector<MaterialDrop> pendingDrops_;
};
