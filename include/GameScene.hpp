#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <random>
#include <cstdint>                // ← pour std::uint8_t

#include "MapGenerator.hpp"
#include "TileMap.hpp"
#include "CreatureSystem.hpp"
#include "TreeSystem.hpp"
#include "BuildMenu.hpp"
#include "Button.hpp"
#include "Defense.hpp"

class GameScene {
public:
    explicit GameScene(sf::RenderWindow& win);

    void handleInput(bool mouseLeft, bool mouseLeftReleased, bool mouseMoved);
    void update(float dt);
    void draw();
    // ===== NEW (declared so GameScene.cpp can define/use them) =====
    void placeCannon(sf::Vector2f center);
    CannonTower* getActiveAimingTower();  // simple helper for the aim overlay

   // std::unique_ptr<BuildMenu> menu_;
   

private:
    // utils
    void setPixelPerfectView(int worldW, int worldH, float tileSize);
    WaypointPath buildMainPathPolyline(const Map& m, float tileSize) const;

    // ★ Déclarations manquantes (pour corriger les “no declaration matches”)
    void updateMenuLayout();
    void drawMenu();

    sf::RenderWindow& win_;

    // terrain
    sf::Texture terrain_;
    TileMap     tilemap_;
    Map         map_;
    float       tileSize_ = 64.f;

    // décor
    TreeSystem  trees_;
    std::mt19937 rng_{ std::random_device{}() };

    // bâtiment ressource
    sf::Texture resourceTex_;
    std::unique_ptr<sf::Sprite> resourceSprite_;  // SFML3 : pas de ctor par défaut
    std::unique_ptr<sf::Sprite> resourceShadow_;  // idem

    // créatures
    CreatureSystem creeps_{tileSize_};
    float gameTime_ = 0.f;

    // ===== UI (SFML3: Sprite/Text via pointeurs) =====
    sf::Texture menuButtonTex_, menuBgTex_;
    std::unique_ptr<sf::Sprite> menuButton_;   // ← était sf::Sprite
    std::unique_ptr<sf::Sprite> menuBg_;       // ← était sf::Sprite
    // GameScene.hpp
    std::unique_ptr<BuildMenu> menu_;


    sf::Font    uiFont_;
    std::unique_ptr<sf::Text> uiTitle_;        // ← était sf::Text

    sf::RectangleShape matSlots_[3];
    sf::CircleShape    unitBtns_[3];
    bool unitAffordable_[3] = {true,true,true};
    int  materialCount_[3]  = {0,0,0};

    bool  menuOpen_ = false;
    float menuAnim_ = 0.f;
    float menuAnimSpeed_ = 3.f;
    float menuAlpha_ = 0.f;


     // --- defenses ---
     // placement API
     std::vector<uint8_t> buildOcc_; // 0 free, 1 occupied
    int worldW_ = 0, worldH_ = 0;
    // --- in private (GameScene.hpp)
    bool draggingUnit_ = false;
    int  dragW_ = 1, dragH_ = 1;      // footprint in tiles
    sf::Vector2f dragWorld_{};
    bool dragValid_ = false;


    inline int cellId(int tx,int ty) const { return ty*worldW_ + tx; }
    bool inB(int tx,int ty) const { return tx>=0 && ty>=0 && tx<worldW_ && ty<worldH_; }
    bool canPlaceRect(int tx, int ty, int w, int h) const;
    void occupyRect(int tx, int ty, int w, int h, bool on);
    std::vector<std::unique_ptr<CannonTower>> towers_;
    bool cannonAvailable_ = true;   // only one at start
    sf::Texture cannonIconTex_;     // def_cannon.png for the tower icon

    // helper
    bool isBuildableAtPixel(sf::Vector2f px) const {
        int tx = int(px.x / tileSize_);
        int ty = int(px.y / tileSize_);
        if (!map_.inBounds(tx,ty)) return false;
        const auto& c = map_.at(tx,ty);
        return (c.ground == Tile::Rock) && c.buildable;
    }

     // Towers (you already referenced a vector in .cpp; declare it here)
   // std::vector<std::unique_ptr<CannonTower>> towers_;

    // (optional) texture you use for the cannon
    sf::Texture cannonTex_;

        int  activeTowerIndex_ = -1;   // -1 => none selected
        bool isAiming() const { return activeTowerIndex_ >= 0 && activeTowerIndex_ < (int)towers_.size(); }
        void deselectTower() { activeTowerIndex_ = -1; }

       // Inventory (shared with BuildMenu display)
    //int materialCount_[3] = {0,0,0}; // 0:Wood, 1:Stone, 2:Crystal

    // Costs
    struct Cost { int wood, stone, crystal; };
    Cost costCannon_{ /*wood*/2, /*stone*/1, /*crystal*/0 };
    Cost costArcher_{ /*wood*/3, /*stone*/2, /*crystal*/0 };
    Cost costMage_  { /*wood*/0, /*stone*/5, /*crystal*/2 };

    bool canAfford(const Cost& c) const {
        return materialCount_[0] >= c.wood
            && materialCount_[1] >= c.stone
            && materialCount_[2] >= c.crystal;
    }
    void spend(const Cost& c) {
        materialCount_[0] -= c.wood;
        materialCount_[1] -= c.stone;
        materialCount_[2] -= c.crystal;
    }

    // Drops buffer fetched from systems/towers this frame
    std::vector<MaterialDrop> pendingDrops_;

};
