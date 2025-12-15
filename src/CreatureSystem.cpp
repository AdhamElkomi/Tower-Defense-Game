


#include "CreatureSystem.hpp"
#include <algorithm>
#include <SFML/Audio.hpp>
#include <cmath>
#include <cfloat>

CreatureSystem::CreatureSystem(float tileSize) : tileSize_(tileSize) {}


void CreatureSystem::loadTextures(){
    auto load = [&](CreatureType t, const std::string& path){
        auto tex = std::make_unique<sf::Texture>();
        (void)tex->loadFromFile(path);
        tex->setSmooth(false);
        textures_[t] = std::move(tex);
    };

    // -------- Défs complètes (tous les champs, ordre = celui de CreatureDef) --------
    defs_[CreatureType::Grunt] = CreatureDef{
        /*type*/         CreatureType::Grunt,
        /*texturePath*/  "../assets/creeps/grunt.png",
        /*frameSize*/    {128,128},    // chaque case ≈ 128x128 px (à ajuster si besoin)
        /*framesPerRow*/ 4,            // 4 colonnes
        /*animFps*/      8.f,
        /*hpMax*/        60,
        /*speed*/        80.f,
        /*armor*/        0,
        /*bounty*/       5,
        /*scale*/        1.5f,         // réduit un peu pour l’écran
        /*footAnchorY*/  0.90f,        // pieds proches du bas
        /*sheetOrigin*/  {0,0},        // pas de marge au début
        /*sheetSpacing*/ {10,50},        // ~5 px d’espacement horizontal
        /*rows*/         3             // 3 lignes (directions ou animations)
    };


    defs_[CreatureType::Rogue] = CreatureDef{
        /*type*/         CreatureType::Rogue,
        /*texturePath*/  "../assets/creeps/rogue.png",
        /*frameSize*/    {64,64},    // largeur × hauteur d’un frame
        /*framesPerRow*/ 4,          // 8 images d’animation par ligne
        /*animFps*/      12.f,       // anim plus rapide que Grunt
        /*hpMax*/        40,
        /*speed*/        120.f,      // rapide
        /*armor*/        0,
        /*bounty*/       7,
        /*scale*/        1.5f,      // un peu plus petit
        /*footAnchorY*/  0.9f,      // pieds un peu plus haut
        /*sheetOrigin*/  {0,0},      // pas de marge (ajuste si besoin, ex {10,0})
        /*sheetSpacing*/ {10,50},      // pas d’espacement
        /*rows*/         3           // N, E, S, O
    };


    defs_[CreatureType::Golem] = CreatureDef{
        CreatureType::Golem,
        "../assets/creeps/golem2.png", // <-- corrigé
        {128,128},
        4,
        7.f,
        220,
        50.f,
        4,
        15,
        2.f,
        0.9f,     // golem lourd, pieds plus bas
        {0,0},
        {0,0},
        3
    };

    // -------- Chargement textures --------
    for (auto& [t,def] : defs_) load(t, def.texturePath);

    // -------- Auto-détection frameSize tenant compte des marges/espaces --------
    for (auto& [t,def] : defs_){
        const auto& tex = *textures_.at(t);
        const auto ts   = tex.getSize(); // Vector2u

        // largeur/hauteur utile après origine et espacements
        const int usableW = int(ts.x) - def.sheetOrigin.x - def.sheetSpacing.x * (def.framesPerRow - 1);
        const int usableH = int(ts.y) - def.sheetOrigin.y - def.sheetSpacing.y * (def.rows        - 1);

        const int fw = (def.framesPerRow > 0) ? (usableW / def.framesPerRow) : 0;
        const int fh = (def.rows        > 0) ? (usableH / def.rows)         : 0;

        if (fw > 0 && fh > 0) def.frameSize = { fw, fh };
    }
}



void CreatureSystem::playSound(CreatureType t, float pitchRand) {
    auto it = soundBufs_.find(t);
    if (it == soundBufs_.end() || !it->second) return;

    // purge des sons terminés
    activeSounds_.erase(
        std::remove_if(activeSounds_.begin(), activeSounds_.end(),
            [](const sf::Sound& s){ return s.getStatus() == sf::SoundSource::Status::Stopped; }),
        activeSounds_.end()
    );

    // créer une nouvelle instance et jouer
    activeSounds_.emplace_back(*it->second);
    sf::Sound& s = activeSounds_.back();

    if (pitchRand != 0.f) {
        float base = 1.f;
        float var  = (static_cast<float>(std::rand())/RAND_MAX * 2.f - 1.f) * pitchRand;
        s.setPitch(std::max(0.5f, base + var));
    }
    s.setVolume(100.f);
    s.play();
}


void CreatureSystem::loadSounds(){
    auto load = [&](CreatureType t, const std::string& path){
        auto buf = std::make_unique<sf::SoundBuffer>();
        if (buf->loadFromFile(path)) {
            soundBufs_[t] = std::move(buf);
        }
    };

    load(CreatureType::Grunt, "../assets/sfx/grunt_spawn.ogg");
    load(CreatureType::Rogue, "../assets/sfx/grunt_spawn.ogg");
    load(CreatureType::Golem, "../assets/sfx/rogue_spawn.ogg");
}

bool CreatureSystem::hitFirstAt(sf::Vector2f p, float r, int dmg){
    float r2 = r*r;
    for (auto& uptr : alive_){
        auto* c = uptr.get();
        sf::Vector2f cp = c->pos();             // center (we set sprite origin properly earlier)
        sf::Vector2f d = cp - p;
        if (d.x*d.x + d.y*d.y <= r2){
            c->hit(dmg);
            return true;
        }
    }
    return false;
}




void CreatureSystem::setPath(const WaypointPath& p){ path_ = p; }

void CreatureSystem::setExits(const std::vector<sf::Vector2i>& exits) {
    exits_ = exits;
}

void CreatureSystem::setResourcePos(sf::Vector2f pos) {
    resourcePos_ = pos;
}

bool CreatureSystem::isOnExitTile(sf::Vector2f pos) const {
    int tileX = static_cast<int>(pos.x / tileSize_);
    int tileY = static_cast<int>(pos.y / tileSize_);
    for (const auto& exit : exits_) {
        if (exit.x == tileX && exit.y == tileY) {
            return true;
        }
    }
    return false;
}

bool CreatureSystem::isNearResource(sf::Vector2f pos) const {
    sf::Vector2f d = pos - resourcePos_;
    float distSq = d.x * d.x + d.y * d.y;
    return distSq <= resourceRadius_ * resourceRadius_;
}

void CreatureSystem::extractCollected(std::vector<CollectedResource>& out) {
    out.insert(out.end(), collectedResources_.begin(), collectedResources_.end());
    collectedResources_.clear();
}

// New function to set path via base to exit
void CreatureSystem::setPathViaBase(const Map& map, sf::Vector2i start, const std::vector<sf::Vector2i>& baseTiles, const std::vector<sf::Vector2i>& exits, const WalkableFn& isWalkable, const OccupancyGrid* occ) {
    TilePath tilePath = routeViaBaseToBestExit(map, start, baseTiles, exits, isWalkable, occ);
    if (!tilePath.empty()) {
        path_.pts.clear();
        for (auto& tile : tilePath) {
            path_.pts.emplace_back((tile.x + 0.5f) * tileSize_, (tile.y + 0.5f) * tileSize_);
        }
    }
    setExits(exits);
}

// NEW: Set paths for multiple entries
void CreatureSystem::setPathsForMultipleEntries(const Map& map, const std::vector<sf::Vector2i>& entries, const std::vector<sf::Vector2i>& baseTiles, const std::vector<sf::Vector2i>& exits, const WalkableFn& isWalkable, const OccupancyGrid* occ) {
    paths_.clear();
    entries_ = entries;
    currentEntryIndex_ = 0;

    // Build a path for each entry
    for (const auto& entry : entries) {
        TilePath tilePath = routeViaBaseToBestExit(map, entry, baseTiles, exits, isWalkable, occ);
        WaypointPath waypointPath;
        if (!tilePath.empty()) {
            for (auto& tile : tilePath) {
                waypointPath.pts.emplace_back((tile.x + 0.5f) * tileSize_, (tile.y + 0.5f) * tileSize_);
            }
        }
        paths_.push_back(waypointPath);
    }

    // Also set the main path to the first entry for backwards compatibility
    if (!paths_.empty()) {
        path_ = paths_[0];
    }

    setExits(exits);
}

void CreatureSystem::spawn(CreatureType t, float atTime){
    timeline_.push_back({t, atTime});
}

void CreatureSystem::spawnWave(int nGrunt, int nRogue, int nGolem, float startTime, float period){
    float t = startTime;
    
    // If we have multiple entries, distribute creatures among them
    if (!paths_.empty() && paths_.size() > 1) {
        for (int i=0;i<nGrunt;++i){ 
            spawn(CreatureType::Grunt, t);  
            t += period; 
        }
        for (int i=0;i<nRogue;++i){ 
            spawn(CreatureType::Rogue, t);  
            t += period; 
        }
        for (int i=0;i<nGolem;++i){ 
            spawn(CreatureType::Golem, t);  
            t += period; 
        }
    } else {
        // Fallback to original behavior if only one path
        for (int i=0;i<nGrunt;++i){ spawn(CreatureType::Grunt, t);  t += period; }
        for (int i=0;i<nRogue;++i){ spawn(CreatureType::Rogue, t);  t += period; }
        for (int i=0;i<nGolem;++i){ spawn(CreatureType::Golem, t);  t += period; }
    }
    std::sort(timeline_.begin(), timeline_.end(), [](auto&a,auto&b){return a.t<b.t;});
}

void CreatureSystem::update(float dt, float timeNow){
    while(!timeline_.empty() && timeline_.front().t <= timeNow){
        auto sc = timeline_.front(); timeline_.erase(timeline_.begin());
        const auto& def = defs_.at(sc.type);
        const auto& tex = *textures_.at(sc.type);

        
        // -- scale propre : on vise 80% de la tuile
        const float fit = 0.80f;                 // marge pour éviter tout recouvrement
        const float pxTarget = tileSize_ * fit;  // taille cible en pixels à l’écran
        const float base = (def.frameSize.x > 0) ? (pxTarget / float(def.frameSize.x)) : 1.f;

        // CreatureSystem.cpp – au spawn
        const sf::SoundBuffer* loopBuf = nullptr;
        if (auto it = soundBufs_.find(sc.type); it != soundBufs_.end() && it->second)
            loopBuf = it->second.get();

        // Apply speed multiplier to creature definition
        CreatureDef modifiedDef = def;
        modifiedDef.speed *= speedMultiplier_;

        // Select the path based on round-robin distribution among available entries
        WaypointPath selectedPath = path_;  // Default to main path
        if (!paths_.empty()) {
            selectedPath = paths_[currentEntryIndex_];
            currentEntryIndex_ = (currentEntryIndex_ + 1) % paths_.size();
        }

        auto c = std::make_unique<Creature>(modifiedDef, tex, selectedPath, loopBuf);

        //alive_.push_back(std::make_unique<Creature>(def, tex, path_, loopBuf));

        // def.scale reste un multiplicateur artistique (grunt petit, golem massif, etc.)
        c->setScale(base * def.scale);

        alive_.push_back(std::move(c));

        playSound(sc.type, 105.f); // ← son de spawn
    }

    for (auto it = alive_.begin(); it != alive_.end(); ){
        (*it)->update(dt);
        bool shouldRemove = (*it)->isDead() || isOnExitTile((*it)->pos());
        if (shouldRemove){
            // Check if creature is on exit tile with resource
            if (isOnExitTile((*it)->pos()) && (*it)->hasCollectedResource()) {
                stolenResources_.push_back({(*it)->type(), (*it)->pos()});
                // Increase HP after passage by resource tower (stealing)
                int hpBoost = 0;
                switch ((*it)->type()) {
                    case CreatureType::Grunt: hpBoost = 10; break;
                    case CreatureType::Rogue: hpBoost = 7; break;
                    case CreatureType::Golem: hpBoost = 15; break;
                }
                (*it)->hit(-hpBoost); // Negative damage to heal
            }
            (*it)->stopAudio();          // ⬅️ coupe le loop
            it = alive_.erase(it);
        }else{
            // Check if creature is near resource and hasn't collected yet
            if (isNearResource((*it)->pos()) && !(*it)->hasCollectedResource()) {
                collectedResources_.push_back({(*it)->type(), (*it)->pos()});
                (*it)->setCollectedResource(true);
            }
            ++it;
        }
    }




     activeSounds_.erase(
        std::remove_if(activeSounds_.begin(), activeSounds_.end(),
                       [](const sf::Sound& s){ return s.getStatus() == sf::SoundSource::Status::Stopped; }),
        activeSounds_.end()
    );
}



void CreatureSystem::draw(sf::RenderTarget& rt) const{
    for (auto& c : alive_) c->draw(rt);
}


int CreatureSystem::applyDamagePoint(sf::Vector2f center, float radius, int dmg,
                                     std::vector<MaterialDrop>& outDrops, bool applyBurn, float burnTime, float burnDps)
{
    int killed = 0;

    for (auto& c : alive_) {
        if (!c) continue;
        sf::Vector2f d = c->pos() - center;
        if (d.x*d.x + d.y*d.y <= radius*radius) {
            c->hit(dmg);
            // Apply burn if specified
            if (applyBurn && burnTime > 0.f && burnDps > 0.f) {
                c->applyBurn(burnTime, burnDps);
            }
            if (c->isDead()) {
                // ----- loot table -----
                switch (c->def_.type) {
                    case CreatureType::Rogue: {
                        outDrops.push_back({Material::Wood,   c->pos()});
                        outDrops.push_back({Material::Wood,  c->pos()});
                        rogueKills_++;
                    } break;
                    case CreatureType::Grunt: {
                        outDrops.push_back({Material::Wood,   c->pos()});
                        outDrops.push_back({Material::Stone,  c->pos()});
                        outDrops.push_back({Material::Crystal,c->pos()});
                        gruntKills_++;
                    } break;
                    case CreatureType::Golem: {
                        outDrops.push_back({Material::Wood,   c->pos()});
                        outDrops.push_back({Material::Stone,   c->pos()});
                        outDrops.push_back({Material::Stone,  c->pos()});
                        outDrops.push_back({Material::Crystal,c->pos()});
                        outDrops.push_back({Material::Crystal,c->pos()});
                        outDrops.push_back({Material::Crystal,c->pos()});

                        golemKills_++;
                    } break;
                }
                killed++;
            }
        }
    }

    // purge dead
    alive_.erase(std::remove_if(alive_.begin(), alive_.end(),
                    [](const std::unique_ptr<Creature>& c){ return !c || c->isDead(); }),
                 alive_.end());

    // ALSO store them internally so GameScene can fetch later
    pendingDrops_.insert(pendingDrops_.end(), outDrops.begin(), outDrops.end());

    return killed;
}

void CreatureSystem::extractStolen(std::vector<StolenResource>& out) {
    out.insert(out.end(), stolenResources_.begin(), stolenResources_.end());
    stolenResources_.clear();
}

void CreatureSystem::stopAllCreatureSounds() {
    // Stop sounds for all alive creatures
    for (auto& c : alive_) {
        c->stopAudio();
    }
    // Stop any active spawn sounds
    for (auto& s : activeSounds_) {
        s.stop();
    }
    activeSounds_.clear();
}

sf::Vector2f CreatureSystem::findClosestInRange(sf::Vector2f center, float radius) const {
    sf::Vector2f closest = {FLT_MAX, FLT_MAX};
    float minDistSq = FLT_MAX;
    float r2 = radius * radius;

    for (const auto& c : alive_) {
        if (!c) continue;
        sf::Vector2f pos = c->pos();
        sf::Vector2f d = pos - center;
        float distSq = d.x * d.x + d.y * d.y;
        if (distSq <= r2 && distSq < minDistSq) {
            minDistSq = distSq;
            closest = pos;
        }
    }

    return (closest.x == FLT_MAX) ? sf::Vector2f(0, 0) : closest; // Return (0,0) if no creature found
}
