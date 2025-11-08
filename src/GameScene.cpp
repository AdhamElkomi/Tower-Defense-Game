#include <SFML/Graphics.hpp>
#include <memory>
#include <random>
#include <vector>
#include "GameScene.hpp"
#include <filesystem>
#include <algorithm>
#include <array>
#include <cstdint>
#include "MapGenerator.hpp"
#include "TileMap.hpp"
#include "CreatureSystem.hpp"
#include "TreeSystem.hpp"
#include "HouseSystem.hpp"
#include "BuildMenu.hpp"
#include "Defense.hpp"

// ---- helpers existants ----
void GameScene::setPixelPerfectView(int worldW, int worldH, float tileSize){
    sf::View v;
    v.setSize(sf::Vector2f(worldW * tileSize, worldH * tileSize));
    v.setCenter(sf::Vector2f((worldW * tileSize) * 0.5f, (worldH * tileSize) * 0.5f));
    win_.setView(v);
}

WaypointPath GameScene::buildMainPathPolyline(const Map& m, float tileSize) const {
    const int W = m.w, H = m.h;
    auto centerPx = [&](int x,int y){
        return sf::Vector2f( (x + 0.5f) * tileSize, (y + 0.5f) * tileSize );
    };
    auto inB = [&](int x,int y){ return x>=0 && y>=0 && x<W && y<H; };

    std::vector<sf::Vector2i> edge;
    for (int x=0;x<W;++x){
        if (m.at(x,0).ground==Tile::Path)     edge.push_back({x,0});
        if (m.at(x,H-1).ground==Tile::Path)   edge.push_back({x,H-1});
    }
    for (int y=0;y<H;++y){
        if (m.at(0,y).ground==Tile::Path)     edge.push_back({0,y});
        if (m.at(W-1,y).ground==Tile::Path)   edge.push_back({W-1,y});
    }
    if (edge.empty()){
        for (int y=0;y<H;++y){
            bool br=false;
            for(int x=0;x<W;++x){
                if (m.at(x,y).ground==Tile::Path){ edge.push_back({x,y}); br=true; break; }
            }
            if (br) break;
        }
    }
    if (edge.empty()) return {};

    auto start = *std::min_element(edge.begin(), edge.end(),
        [](auto a, auto b){ return (a.x+a.y) < (b.x+b.y); });
    auto goal  = *std::max_element(edge.begin(), edge.end(),
        [](auto a, auto b){ return (a.x+a.y) < (b.x+b.y); });

    std::vector<char> seen(W*H, 0);
    auto id = [&](int x,int y){ return y*W+x; };
    sf::Vector2i cur = start;
    sf::Vector2i prev = {-999,-999};

    WaypointPath path;
    path.pts.reserve(W*H/2);
    path.pts.push_back(centerPx(cur.x,cur.y));
    seen[id(cur.x,cur.y)] = 1;

    auto neigh4 = [&](sf::Vector2i p){
        std::array<sf::Vector2i,4> n{{{p.x+1,p.y},{p.x-1,p.y},{p.x,p.y+1},{p.x,p.y-1}}};
        return n;
    };

    int guard = W*H*4;
    while (guard-- > 0){
        if (cur == goal) break;
        sf::Vector2i next{-999,-999};
        for(auto n : neigh4(cur)){
            if (!inB(n.x,n.y)) continue;
            if (m.at(n.x,n.y).ground != Tile::Path) continue;
            if (n == prev) continue;
            if (!seen[id(n.x,n.y)]) { next = n; break; }
            if (next.x == -999){ next = n; }
        }
        if (next.x == -999) break;
        prev = cur;
        cur  = next;
        if (!seen[id(cur.x,cur.y)]){
            path.pts.push_back(centerPx(cur.x,cur.y));
            seen[id(cur.x,cur.y)] = 1;
        }
    }
    if (path.pts.size() < 2) path.pts.clear();
    return path;
}

// ---- ctor ----
GameScene::GameScene(sf::RenderWindow& win) : win_(win) {
    applyDifficulty("Normal");
    resourceCount_ = difficultyParams_.startResources;
    const int W = 60, H = 24;
    tileSize_  = 64.f;
     worldW_ = W; worldH_ = H;

    terrain_ = std::make_unique<sf::Texture>();
}

GameScene::GameScene(sf::RenderWindow& win, const std::string& difficulty) : win_(win) {
    applyDifficulty(difficulty);
    resourceCount_ = difficultyParams_.startResources;
    const int W = 60, H = 24;
    tileSize_  = 64.f;
     worldW_ = W; worldH_ = H;

    terrain_ = std::make_unique<sf::Texture>();
    (void)terrain_->loadFromFile("../assets/tiles/terrain_atlas_z.png");
    terrain_->setSmooth(false);
    terrain_->setRepeated(false);

    MapGenerator gen;
    map_ = gen.generate(W, H, /*entrées*/1, 2, /*sorties*/1, 2);

    tilemap_.setTexture(*terrain_, {64, 64});
    tilemap_.build(map_, tileSize_);

    setPixelPerfectView(W, H, tileSize_);

    trees_.loadTextures("../assets/trees");
    const unsigned treeCount = static_cast<unsigned>((W * H) / 18);
    trees_.generate(map_, tileSize_, treeCount, rng_, /*roadPaddingTiles=*/1);

    houses_.loadTextures("../assets/design_image");
    const unsigned maxHouses = 10; // Increased max houses
    houses_.generate(map_, tileSize_, maxHouses, rng_, /*roadPaddingTiles=*/1);

    // Initialize material counts based on difficulty
    // For now, keep default values; can be adjusted per difficulty later
    materialCount_[0] = 10; // bois
    materialCount_[1] = 10; // pierre
    materialCount_[2] = 5;  // cristal

    {
        resourceTex_ = std::make_unique<sf::Texture>();
        bool ok = resourceTex_->loadFromFile("../assets/buildings/resources_build.jpg");
        if (!ok) (void)resourceTex_->loadFromFile("../assets/tiles/resources_build-1.png");
        resourceTex_->setSmooth(false);

        resourceSprite_ = std::make_unique<sf::Sprite>(*resourceTex_);
        resourceShadow_ = std::make_unique<sf::Sprite>(*resourceTex_);

        const auto texSz = resourceTex_->getSize();
        const float targetW = 6.f * tileSize_;
        const float targetH = 6.f * tileSize_;
        const float sx = (targetW / static_cast<float>(texSz.x)) * 0.92f;
        const float sy = (targetH / static_cast<float>(texSz.y)) * 0.92f;
        const float s  = std::min(sx, sy);

        resourceSprite_->setScale(sf::Vector2f(s, s));
        resourceShadow_->setScale(sf::Vector2f(s * 1.02f, s * 0.88f));
        resourceShadow_->setColor(sf::Color(0,0,0,80));

        resourceSprite_->setOrigin(sf::Vector2f(texSz.x * 0.5f, texSz.y * 0.5f));
        resourceShadow_->setOrigin(sf::Vector2f(texSz.x * 0.5f, texSz.y * 0.5f));

        const float cx = (W * tileSize_) * 0.5f;
        const float cy = (H * tileSize_) * 0.5f;
        resourceSprite_->setPosition(sf::Vector2f(cx, cy));
        resourceShadow_->setPosition(sf::Vector2f(cx, cy + 6.f));
    }

    // ---- Créatures ----
    {
        creeps_.loadTextures();
        creeps_.loadSounds();

        // Find base tiles (Rock tiles that are walkable)
        std::vector<sf::Vector2i> baseTiles;
        for (int y = 0; y < map_.h; ++y) {
            for (int x = 0; x < map_.w; ++x) {
                const auto& cell = map_.at(x, y);
                if (cell.ground == Tile::Rock && cell.walkable) {
                    baseTiles.emplace_back(x, y);
                }
            }
        }

        // Use stored exits
        std::vector<sf::Vector2i> exits;
        for (auto& p : map_.exits) {
            exits.emplace_back(p.x, p.y);
        }

        // Use stored entries as start
        sf::Vector2i start(-1, -1);
        if (!map_.entries.empty()) {
            start = sf::Vector2i(map_.entries[0].x, map_.entries[0].y);
        }

        // Walkable function: Path tiles and Rock tiles that are not buildable (base only)
        auto isWalkable = [](const Cell& cell, int x, int y) -> bool {
            return cell.ground == Tile::Path || (cell.ground == Tile::Rock && !cell.buildable);
        };

        if (!baseTiles.empty() && !exits.empty() && start.x != -1) {
            creeps_.setPathViaBase(map_, start, baseTiles, exits, isWalkable, nullptr);
        } else {
            // Fallback to old path
            WaypointPath p = buildMainPathPolyline(map_, tileSize_);
            if (!p.pts.empty()){
                creeps_.setPath(p);
            }
        }

        // Set resource position (center of the map)
        sf::Vector2f resourcePos((worldW_ * tileSize_) * 0.5f, (worldH_ * tileSize_) * 0.5f);
        creeps_.setResourcePos(resourcePos);

    // Initialize waves
    generateWaves();
    spawnNextWave();

    // Apply speed multiplier to creatures
    creeps_.setSpeedMultiplier(difficultyParams_.speedMul);
    }

    // ===== UI =====
    menuButtonTex_ = std::make_unique<sf::Texture>();
    (void)menuButtonTex_->loadFromFile("../assets/ui/menu_button.png");
    menuButtonTex_->setSmooth(false);
    menuButton_ = std::make_unique<sf::Sprite>(*menuButtonTex_);
    menuButton_->setScale({0.5f, 0.5f});

    menuBgTex_ = std::make_unique<sf::Texture>();
    (void)menuBgTex_->loadFromFile("../assets/ui/menu_bg.png");
    menuBgTex_->setSmooth(false);
    menuBg_ = std::make_unique<sf::Sprite>(*menuBgTex_);
    menuBg_->setScale({0.5f, 0.5f});

    uiFont_ = std::make_unique<sf::Font>();
    if (uiFont_->loadFromFile("../assets/fonts/Roboto-Regular_2.ttf")) {  // SFML2
        uiTitle_ = std::make_unique<sf::Text>("Armory & Supplies", *uiFont_, 28);
        uiTitle_->setFillColor(sf::Color::White);
    }

    // Initialiser le stock de matériaux pour permettre au moins 2 tours de chaque type
    // Coûts max : Mage (0,3,2) x2 = 0 bois, 6 pierre, 4 cristal
    // Archer (3,2,0) x2 = 6 bois, 4 pierre, 0 cristal
    // Cannon (2,1,0) x2 = 4 bois, 2 pierre, 0 cristal
    // Total safe : bois=10, pierre=10, cristal=5
    materialCount_[0] = 10; // bois
    materialCount_[1] = 10; // pierre
    materialCount_[2] = 5;  // cristal

    for (int i=0;i<3;++i){
        matSlots_[i].setSize(sf::Vector2f(72.f, 72.f));
        matSlots_[i].setFillColor(sf::Color(220,220,220,240));
        matSlots_[i].setOutlineThickness(2.f);
        matSlots_[i].setOutlineColor(sf::Color(40,40,40,220));
        // Border radius visuel avec un petit arrondi = on peut simuler via texture / ou laisser carré propre
    }

    // boutons unités (ronds) + luminance selon affordability
    for (int i=0;i<3;++i){
        unitBtns_[i] = sf::CircleShape(40.f);
        unitBtns_[i].setFillColor(sf::Color(255,255,255, unitAffordable_[i] ? 255 : 120)); // "luminosité"
        unitBtns_[i].setOutlineThickness(3.f);
        unitBtns_[i].setOutlineColor(sf::Color(30,30,30,220));
    }
    // GameScene.cpp (fin du constructeur)
    menu_ = std::make_unique<BuildMenu>(win_, tileSize_, map_);

    // load tower icon texture
    (void)cannonIconTex_.loadFromFile("../assets/ui/tower_build/cannon.png");
    cannonIconTex_.setSmooth(false);
    (void)arrowTex_.loadFromFile("../assets/ui/attack_tower/arrow.png");
    arrowTex_.setSmooth(false);
    (void)cannonBallTex_.loadFromFile("../assets/ui/attack_tower/cannon_ball.png");
    cannonBallTex_.setSmooth(false);
    buildOcc_.assign(W*H, 0);


    // position initiale (fermé)
    updateMenuLayout();

    // Load Game Over assets
    if (!gameOverTexture_.loadFromFile("../assets/ui/gameover.png")) {
        // Fallback: create a simple game over texture
        sf::RenderTexture tempRenderTexture;
        tempRenderTexture.create(900, 450);
        tempRenderTexture.clear(sf::Color::Black);
        // Draw "GAME OVER" text on the texture
        sf::Font tempFont;
        if (tempFont.loadFromFile("../assets/ui/FreckleFace-Regular.ttf")) {
            sf::Text tempText("GAME OVER", tempFont, 48);
            tempText.setFillColor(sf::Color::Red);
            tempText.setStyle(sf::Text::Bold);
            sf::FloatRect textRect = tempText.getLocalBounds();
            tempText.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f);
            tempText.setPosition(200.f, 100.f);
            tempRenderTexture.draw(tempText);
        }
        tempRenderTexture.display();
        gameOverTexture_ = tempRenderTexture.getTexture();
    }
    gameOverSprite_ = std::make_unique<sf::Sprite>(gameOverTexture_);
    gameOverSprite_->setOrigin({gameOverTexture_.getSize().x / 2.0f, gameOverTexture_.getSize().y / 2.0f});
    gameOverSprite_->setPosition({win_.getSize().x / 2.0f, win_.getSize().y / 2.0f});

    // Load button font and create return button
    if (!buttonFont_.loadFromFile("../assets/ui/FreckleFace-Regular.ttf")) {
        buttonFont_.loadFromFile("../assets/fonts/Roboto-Regular.ttf");
    }
    returnButton_ = std::make_unique<Button>(buttonFont_, sf::Vector2f(200.f, 50.f), "Return to Menu", sf::Color(140, 200, 110), sf::Color::White, 3.f);
    returnButton_->setPosition({win_.getSize().x / 2.0f - 100.f, win_.getSize().y / 2.0f + 190.f});



    // Load sounds
    if (collapseBuffer_.loadFromFile("../assets/ui/game_over_sound/collapse.mp3")) {
        collapseSound_.emplace(collapseBuffer_);
    }
    if (gameOverBuffer_.loadFromFile("../assets/ui/game_over_sound/game_over.mp3")) {
        gameOverSound_.emplace(gameOverBuffer_);
        // Note: sf::Sound doesn't have setLoop, only sf::Music does
        // We'll handle looping in update if needed
    } else if (gameOverBuffer_.loadFromFile("../assets/sfx/game_over.ogg")) {
        gameOverSound_.emplace(gameOverBuffer_);
        // Note: sf::Sound doesn't have setLoop, only sf::Music does
        // We'll handle looping in update if needed
    }

    // Stop any existing music and start game music
    // Note: We don't have direct access to AudioManager here, so we'll assume it's handled elsewhere

    // Load score UI
    scoreTexture_ = std::make_unique<sf::Texture>();
    if (!scoreTexture_->loadFromFile("../assets/ui/score.png")) {
        // Fallback: create a simple rectangle
        sf::RenderTexture tempRT;
        tempRT.create(200, 50);
        tempRT.clear(sf::Color(100, 100, 100, 200));
        tempRT.display();
        scoreTexture_ = std::make_unique<sf::Texture>(tempRT.getTexture());
    }
    scoreTexture_->setSmooth(false);

    scoreSprite_ = std::make_unique<sf::Sprite>(*scoreTexture_);
    // Position top-right, small size
    sf::Vector2u texSize = scoreTexture_->getSize();
    float scale = 0.5f; // small dimension
    scoreSprite_->setScale(scale, scale);
    scoreSprite_->setPosition(win_.getSize().x - texSize.x * scale - 10.f, 10.f);

    scoreText_ = std::make_unique<sf::Text>();
    if (uiFont_) {
        scoreText_->setFont(*uiFont_);
    } else {
        sf::Font tempFont;
        if (tempFont.loadFromFile("../assets/fonts/Roboto-Regular.ttf")) {
            scoreText_->setFont(tempFont);
        }
    }
    scoreText_->setCharacterSize(20);
    scoreText_->setFillColor(sf::Color::White);
    // Center on sprite
    sf::FloatRect spriteBounds = scoreSprite_->getGlobalBounds();
    scoreText_->setPosition(spriteBounds.left + spriteBounds.width / 2.f, spriteBounds.top + spriteBounds.height / 2.f - 10.f);
    scoreText_->setOrigin(scoreText_->getLocalBounds().width / 2.f, scoreText_->getLocalBounds().height / 2.f);
}

GameScene::GameScene(sf::RenderWindow& win, const std::string& difficulty, const std::string& username) : GameScene(win, difficulty) {
    username_ = username;
    difficulty_ = difficulty;
}






// ---- input ----
void GameScene::handleInput(bool leftDown, bool leftUp, bool moved){
    // Handle return to menu button always
    if (leftUp && returnButton_) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(win_);
        if (returnButton_->bounds().contains(static_cast<sf::Vector2f>(mousePos))) {
            returnToMenu_ = true;
        }
    }

    if (gameOver_ && gameOverState_ == GameOverState::ShowingImage) {
        return; // Don't process other input when game over
    }

    if (gameOver_) return; // Don't process input during collapse animation
    sf::Vector2f world = win_.mapPixelToCoords(sf::Mouse::getPosition(win_));
    if (leftDown)  menu_->onMousePressed(world);
    if (moved)     menu_->onMouseMoved(world);
    if (leftUp)    menu_->onMouseReleased(world);

     sf::Vector2i mp = sf::Mouse::getPosition(win_);

    // --- placement after drag: place the dragged unit ---
    // If your BuildMenu exposes: isDragging(), draggingUnit(), dragPosition()
    if (leftUp){
        if (isBuildableAtPixel(world)){
            int tx = int(world.x / tileSize_);
            int ty = int(world.y / tileSize_);
            sf::Vector2f center( (tx + 0.5f) * tileSize_, (ty + 0.5f) * tileSize_ );
            placeTower(menu_->draggingUnit(), center);
        }
    }

    // --- manual fire: click inside any tower radius to shoot at mouse point ---
    if (leftUp){
        for (auto& t : towers_){
            sf::Vector2f d = world - t->pos();
            float R = t->radius();
            if (d.x*d.x + d.y*d.y <= R*R){
                t->tryFireAt(world, creeps_);
                break;
            }
        }
    }


    if (menu_ && menu_->isDragging()){
        draggingUnit_ = true;
        dragWorld_ = menu_->dragPosition();

        // map unit type -> footprint
        switch (menu_->draggingUnit()){
            case BuildMenu::Unit::Cannon: dragW_=1; dragH_=1; break;
            case BuildMenu::Unit::Archer: dragW_=2; dragH_=2; break;
            case BuildMenu::Unit::Mage:   dragW_=3; dragH_=3; break;
            case BuildMenu::Unit::Count:  default:  dragW_=1; dragH_=1; break;
        }

        // snap top-left to tile under mouse so the footprint is aligned
        int tx = int(dragWorld_.x / tileSize_);
        int ty = int(dragWorld_.y / tileSize_);
        dragValid_ = canPlaceRect(tx,ty,dragW_,dragH_);

        // On release → place if valid
        if (leftUp){
            if (dragValid_){
                // occupy cells
                occupyRect(tx,ty,dragW_,dragH_, true);

                // place tower center = footprint center
                float cx = (tx + dragW_*0.5f) * tileSize_;
                float cy = (ty + dragH_*0.5f) * tileSize_;
                placeTower(menu_->draggingUnit(), sf::Vector2f(cx,cy));
                menu_->endDrag();                  // tell menu to stop dragging
            }
        }
        return;
    }else{
        draggingUnit_ = false;
    }

    // Map mouse + keyboard
    bool esc = sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Escape);
    bool rmb = sf::Mouse::isButtonPressed(sf::Mouse::Button::Right);

    // --- exit aiming if ESC or RMB ---
    if (esc || rmb) {
        // Stop hold for Mage if holding
        if (isAiming()) {
            auto* t = getActiveAimingTower();
            if (t && t->canHold() && t->type() == TowerType::Mage) {
                MageTower* mage = dynamic_cast<MageTower*>(t);
                if (mage) {
                    mage->isHolding_ = false;
                    mage->holdTime_ = 0.f;
                }
            }
        }
        deselectTower();
    }

    // --- exit aiming if mouse leaves tower radius while aiming ---
    if (moved && isAiming()) {
        auto* t = getActiveAimingTower();
        if (t) {
            sf::Vector2f d = world - t->pos();
            float R = t->radius();
            if (d.x*d.x + d.y*d.y > R*R) {
                deselectTower();
            }
        }
    }

        // --- select tower when you click inside its radius (même si déjà désélectionné)
        if (leftDown) {
            for (int i = (int)towers_.size()-1; i >= 0; --i) { // top-most first
                auto& t = towers_[i];
                sf::Vector2f d = world - t->pos();
                if (d.x*d.x + d.y*d.y <= t->radius()*t->radius()) {
                    activeTowerIndex_ = i; // on sélectionne toujours la tour cliquée
                    // For Mage tower, handle click: start holding
                    if (t->type() == TowerType::Mage) {
                        MageTower* mage = dynamic_cast<MageTower*>(t.get());
                        if (mage) {
                            mage->startHolding();
                        }
                    }
                    break;
                }
            }
        }

        // --- fire only if we are aiming and release LMB inside range (for non-Mage towers)
        if (leftUp && isAiming()) {
            auto* t = getActiveAimingTower();
            if (t && t->type() != TowerType::Mage) {
                t->tryFireAt(world, creeps_);
            } else if (t && t->type() == TowerType::Mage) {
                // For Mage, release to fire if gauge full
                MageTower* mage = dynamic_cast<MageTower*>(t);
                if (mage && mage->isGaugeFull()) {
                    t->tryFireAt(world, creeps_);
                }
            }
        }

        // Handle drop clicks
        if (leftUp) {
            handleDropClick(world);
        }




}



bool GameScene::canPlaceRect(int tx, int ty, int w, int h) const {
    for (int j=0;j<h;++j)
        for (int i=0;i<w;++i){
            int x = tx+i, y = ty+j;
            if (!inB(x,y)) return false;
            const auto& t = map_.at(x,y);
            if (!t.buildable || t.ground != Tile::Rock) return false; // your rule
            if (buildOcc_[cellId(x,y)]) return false;
        }
    return true;
}
void GameScene::occupyRect(int tx, int ty, int w, int h, bool on){
    for (int j=0;j<h;++j)
        for (int i=0;i<w;++i)
            buildOcc_[cellId(tx+i,ty+j)] = on ? 1 : 0;
}

// Somewhere in GameScene.cpp (after includes and within the namespace if any)

// Place a 1x1 cannon centered on a tile center in world coordinates
/*void GameScene::placeCannon(sf::Vector2f center){
    // lazy-load cannon texture once if needed
    if (cannonTex_.getSize().x == 0) {
        (void)cannonTex_.loadFromFile("../assets/ui/units/cannon.png");
        cannonTex_.setSmooth(false);
    }
    towers_.push_back(std::make_unique<CannonTower>(center, cannonTex_));
}*/


// Pour plusieurs types de tours
Tower* GameScene::getActiveAimingTower(){
        if (!isAiming()) return nullptr;
        return towers_[activeTowerIndex_].get();
    }

void GameScene::placeTower(BuildMenu::Unit unit, sf::Vector2f center) {
    // Interdire la construction dans la zone de tir d'une autre tour
    float newRadius = 120.f;
    for (const auto& t : towers_) {
        float dist2 = (t->pos().x - center.x)*(t->pos().x - center.x) + (t->pos().y - center.y)*(t->pos().y - center.y);
        float minDist = t->radius() + newRadius;
        if (dist2 < minDist*minDist) {
            // Trop proche d'une autre tour
            return;
        }
    }
        switch (unit) {
            case BuildMenu::Unit::Cannon:
                newRadius = CannonTower::DefaultRadius; 
                if (cannonTex_.getSize().x == 0) {
                    (void)cannonTex_.loadFromFile("../assets/ui/tower_build/cannon.png");
                    cannonTex_.setSmooth(false);
                }
                if (materialCount_[0] >= costCannon_.wood && materialCount_[1] >= costCannon_.stone && materialCount_[2] >= costCannon_.crystal) {
                    materialCount_[0] -= costCannon_.wood;
                    materialCount_[1] -= costCannon_.stone;
                    materialCount_[2] -= costCannon_.crystal;
                    towers_.push_back(std::make_unique<CannonTower>(center, cannonTex_, cannonBallTex_));
                }
                break;
            case BuildMenu::Unit::Archer:
                newRadius = ArcherTower::DefaultRadius; 
                if (archerTex_.getSize().x == 0) {
                    (void)archerTex_.loadFromFile("../assets/ui/tower_build/archer.png");
                    archerTex_.setSmooth(false);
                }
                if (materialCount_[0] >= costArcher_.wood && materialCount_[1] >= costArcher_.stone && materialCount_[2] >= costArcher_.crystal) {
                    materialCount_[0] -= costArcher_.wood;
                    materialCount_[1] -= costArcher_.stone;
                    materialCount_[2] -= costArcher_.crystal;
                    towers_.push_back(std::make_unique<ArcherTower>(center, archerTex_, arrowTex_));
                }
                break;
            case BuildMenu::Unit::Mage:
                newRadius = MageTower::DefaultRadius;
                if (mageTex_.getSize().x == 0) {
                    (void)mageTex_.loadFromFile("../assets/ui/tower_build/mage.png");
                    mageTex_.setSmooth(false);
                }
                if (materialCount_[0] >= costMage_.wood && materialCount_[1] >= costMage_.stone && materialCount_[2] >= costMage_.crystal) {
                    materialCount_[0] -= costMage_.wood;
                    materialCount_[1] -= costMage_.stone;
                    materialCount_[2] -= costMage_.crystal;
                    auto mage = std::make_unique<MageTower>(center, mageTex_);
                    mage->startHolding(); // Start charging immediately
                    towers_.push_back(std::move(mage));
                }
                break;
            default:
                break;
        }
}




// ---- update ----
void GameScene::update(float dt) {
    if (!gameOver_) {
        // créatures/time
        gameTime_ += dt;
        creeps_.update(dt, gameTime_);

        // Update kill counters from CreatureSystem
        golemKills_ = creeps_.getGolemKills();
        gruntKills_ = creeps_.getGruntKills();
        rogueKills_ = creeps_.getRogueKills();

        // ====== ★ NEW : animation du menu ======
        float target = menuOpen_ ? 1.f : 0.f;
        if (menuAnim_ < target) menuAnim_ = std::min(target, menuAnim_ + dt * menuAnimSpeed_);
        if (menuAnim_ > target) menuAnim_ = std::max(target, menuAnim_ - dt * menuAnimSpeed_);
        menuAlpha_ = 255.f * menuAnim_;

        menu_->update(dt);
        updateMenuLayout();

        // towers (générique)
        for (auto& t : towers_) t->update(dt, creeps_);
        towers_.erase(
            std::remove_if(towers_.begin(), towers_.end(),
                           [](const std::unique_ptr<Tower>& t){ return t->isDead(); }),
            towers_.end()
        );
        for (auto& t : towers_) t->extractDrops(pendingDrops_);

        // Also collect any drops the CreatureSystem queued internally (safety)
        creeps_.extractPendingDrops(pendingDrops_);

        // Update score
        score_ = golemKills_ * 10 + gruntKills_ * 4 + rogueKills_ * 1;
        if (scoreText_) {
            scoreText_->setString(std::to_string(score_));
            // Re-center text
            sf::FloatRect spriteBounds = scoreSprite_->getGlobalBounds();
            scoreText_->setPosition(spriteBounds.left + spriteBounds.width / 2.f, spriteBounds.top + spriteBounds.height / 2.f - 10.f);
            scoreText_->setOrigin(scoreText_->getLocalBounds().width / 2.f, scoreText_->getLocalBounds().height / 2.f);
        }

        // Collect collected resources from creatures (near resource)
        std::vector<CreatureSystem::CollectedResource> collected;
        creeps_.extractCollected(collected);
        for (const auto& c : collected) {
            // No resource deduction here, just animation
            // Add animation for collected resources
            stolenAnimations_.push_back({c.pos, "Stealing!", 2.0f, 1.0f, sf::Vector2f(0.f, 0.f), 255.f}); // 2 seconds lifetime, initial scale 1.0, no offset, full alpha
        }

        // Collect stolen resources from creatures (reached exit with resource)
        std::vector<CreatureSystem::StolenResource> stolen;
        creeps_.extractStolen(stolen);
        for (const auto& c : stolen) {
            int points = 0;
            switch (c.type) {
                case CreatureType::Grunt: points = 2; break;
                case CreatureType::Rogue: points = 1; break;
                case CreatureType::Golem: points = 3; break;
            }
            resourceCount_ -= points;
            if (resourceCount_ < 0) resourceCount_ = 0; // Prevent negative
            // Add animation for stolen resources
            stolenAnimations_.push_back({c.pos, "-" + std::to_string(points), 2.0f, 1.0f, sf::Vector2f(0.f, 0.f), 255.f}); // 2 seconds lifetime, initial scale 1.0, no offset, full alpha
        }

        // Check for game over
        if (resourceCount_ <= 0) {
            gameOver_ = true;
            gameOverState_ = GameOverState::Pausing;
            collapseTimer_ = 0.f;
            creeps_.stopAllCreatureSounds(); // Stop all creature sounds on game over
            if (collapseSound_) {
                collapseSound_->play();
            }
        }

        // Check if it's time to spawn the next wave
        if (currentWaveIndex_ < waves_.size() && gameTime_ >= waves_[currentWaveIndex_].spawnTime) {
            spawnNextWave();
        }
    } else {
        // Update game over animation
        if (gameOverState_ == GameOverState::Pausing) {
            collapseTimer_ += dt;
            if (collapseTimer_ >= 1.0f) { // Pause for 1 second
                gameOverState_ = GameOverState::Collapsing;
                collapseTimer_ = 0.f;
            }
        } else if (gameOverState_ == GameOverState::Collapsing) {
            collapseTimer_ += dt;
            if (collapseTimer_ >= collapseDuration_) {
                gameOverState_ = GameOverState::ShowingImage;
                playGameOverSounds();
                // Scale game over image to cover entire window
            }
        }
    }

    // Update stolen animations
    for (auto it = stolenAnimations_.begin(); it != stolenAnimations_.end(); ) {
        it->lifetime -= dt;
        if (it->lifetime > 0) {
            // Animate scale and offset
            float progress = 1.0f - (it->lifetime / 2.0f); // 0 to 1 over 2 seconds
            it->scale = 1.0f + progress * 4.0f; // Scale from 1.0 to 3.0
            it->offset.y = -progress * 50.f; // Move up by 50 pixels
            it->alpha = 255.f * (it->lifetime / 2.0f); // Fade from 255 to 0 over 2 seconds
        }
        if (it->lifetime <= 0) {
            it = stolenAnimations_.erase(it);
        } else {
            ++it;
        }
    }

    // Update drops
    updateDrops(dt);

    // Credit inventory
    for (const auto& d : pendingDrops_) {
        switch (d.type) {
            case Material::Wood:    materialCount_[0]++; totalKills_++; break;
            case Material::Stone:   materialCount_[1]++; totalKills_++; break;
            case Material::Crystal: materialCount_[2]++; totalKills_++; break;
        }
    }
    pendingDrops_.clear();

    // Update counts in the menu (if you expose setters)
    if (menu_) {
        menu_->setMaterialCount(BuildMenu::Material::Wood,    materialCount_[0]);
        menu_->setMaterialCount(BuildMenu::Material::Stone,   materialCount_[1]);
        menu_->setMaterialCount(BuildMenu::Material::Crystal, materialCount_[2]);

        // Enable/disable depending on affordability
    menu_->setUnitEnabled(BuildMenu::Unit::Cannon, canAfford(costCannon_));
        menu_->setUnitEnabled(BuildMenu::Unit::Archer, canAfford(costArcher_));
        menu_->setUnitEnabled(BuildMenu::Unit::Mage,   canAfford(costMage_));
    }

    
}

    // ---- draw ----
void GameScene::draw() {
    win_.draw(tilemap_);
    win_.draw(trees_);
    win_.draw(houses_);
    creeps_.draw(win_);
    if (resourceShadow_) win_.draw(*resourceShadow_);
    if (resourceSprite_) win_.draw(*resourceSprite_);

    // Draw entry tiles in blue
    for (auto& e : map_.entries) {
        sf::RectangleShape rect(sf::Vector2f(static_cast<float>(tileSize_), static_cast<float>(tileSize_)));
        rect.setPosition(sf::Vector2f(static_cast<float>(e.x * tileSize_), static_cast<float>(e.y * tileSize_)));
        rect.setFillColor(sf::Color(0, 0, 255, 100)); // Semi-transparent blue
        win_.draw(rect);
    }

    // Draw exit tiles in red
    for (auto& ex : map_.exits) {
        sf::RectangleShape rect(sf::Vector2f(static_cast<float>(tileSize_), static_cast<float>(tileSize_)));
        rect.setPosition(sf::Vector2f(static_cast<float>(ex.x * tileSize_), static_cast<float>(ex.y * tileSize_)));
        rect.setFillColor(sf::Color(255, 0, 0, 100)); // Semi-transparent red
        win_.draw(rect);
    }

    // bouton menu (toujours visible)
   // draw: bouton menu
    if (menuButton_) win_.draw(*menuButton_);

    // Draw score
    if (scoreSprite_) win_.draw(*scoreSprite_);
    if (scoreText_) win_.draw(*scoreText_);

    // towers
    for (const auto& t : towers_) t->draw(win_, /*showRadius=*/true);
    if (auto* tower = getActiveAimingTower()) {
        sf::Vector2f m = win_.mapPixelToCoords(sf::Mouse::getPosition(win_));
        sf::Vector2f d = m - tower->pos();
        float L2 = d.x*d.x + d.y*d.y;
        float R  = tower->radius();
        bool inRange = (L2 <= R*R);

        // Range circle
        sf::CircleShape r(R);
        r.setOrigin(sf::Vector2f(R, R));
        r.setPosition(tower->pos());
        r.setFillColor(sf::Color(0,0,0,0));
        r.setOutlineThickness(3.f);
        r.setOutlineColor(inRange ? sf::Color(80,210,100,200) : sf::Color(210,90,90,200));
        win_.draw(r);

        // Aiming ray
        sf::VertexArray line(sf::PrimitiveType::Lines, 2);
        line[0].position = tower->pos();
        line[0].color = inRange ? sf::Color(40,140,60,220) : sf::Color(180,60,60,220);
        line[1].position = m;
        line[1].color = inRange ? sf::Color(20,100,40,220) : sf::Color(160,40,40,220);
        win_.draw(line);
    }



        // draw: panneau
    if (menuAnim_ > 0.01f) drawMenu();

    menu_->draw(win_);

    // Update stolen animations (moved to update function)

    // Affichage du nombre de ressources restantes
    sf::Font font;
    font.loadFromFile("../assets/ui/FreckleFace-Regular.ttf");  // SFML 2

    sf::Text resText("Ressources: " + std::to_string(resourceCount_), font, 32);
    resText.setFillColor(sf::Color::Yellow);
    resText.setPosition(sf::Vector2f(30.f, 30.f));              // or {30.f, 30.f}
    win_.draw(resText);

    // Draw stolen animations
    for (const auto& anim : stolenAnimations_) {
        sf::Text animText(anim.text, font, 24);
        sf::Color color = sf::Color::Red;
        color.a = static_cast<std::uint8_t>(anim.alpha);
        animText.setFillColor(color);
        animText.setPosition(anim.pos + anim.offset);
        animText.setScale(sf::Vector2f(anim.scale, anim.scale));
        win_.draw(animText);
    }

    // Draw drops
    drawDrops();

    // Draw game over animation
    if (gameOver_) {
        if (gameOverState_ == GameOverState::Pausing) {
            // Game is paused, no drawing changes yet
        } else if (gameOverState_ == GameOverState::Collapsing) {
            // Apply pixelation shader to the entire screen
            sf::RenderTexture renderTexture;
            renderTexture.create(win_.getSize().x, win_.getSize().y);
            renderTexture.clear(sf::Color::Transparent);

            // Note: In SFML 3, capturing the screen requires different approach
            // For now, we'll create a pixelation effect on a full-screen quad

            sf::RectangleShape screenQuad({win_.getSize().x, win_.getSize().y});
            screenQuad.setPosition({0.f, 0.f});
            screenQuad.setFillColor(sf::Color::White);

            // Load pixelation shader if not loaded
            if (!pixelShader_.isAvailable()) {
                const std::string vertexShader = R"(
                    void main() {
                        gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
                        gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
                    }
                )";
                const std::string fragmentShader = R"(
                    uniform sampler2D texture;
                    uniform float pixelSize;
                    uniform float time;
                    uniform float screenWidth;
                    uniform float screenHeight;

                    void main() {
                        vec2 texCoord = gl_TexCoord[0].xy;
                        // Create a wave-like distortion effect
                        float wave = sin(texCoord.y * 10.0 + time * 5.0) * 0.01;
                        texCoord.x += wave;

                        // Pixelation effect
                        float pixel = pixelSize + (time * 30.0);
                        vec2 pixelated = floor(texCoord * pixel) / pixel;

                        // Add some color distortion
                        vec4 color = texture2D(texture, pixelated);
                        color.r += sin(time * 2.0) * 0.1;
                        color.g += cos(time * 3.0) * 0.1;
                        color.b += sin(time * 4.0) * 0.1;

                        // Fade to black as time progresses
                        float fade = 1.0 - (time / 3.0);
                        color *= fade;

                        gl_FragColor = color;
                    }
                )";
                pixelShader_.loadFromMemory(vertexShader, fragmentShader);
            }

            float pixelSize = 1.f + (collapseTimer_ / collapseDuration_) * 50.f;
            pixelShader_.setUniform("pixelSize", pixelSize);
            pixelShader_.setUniform("time", collapseTimer_);
            pixelShader_.setUniform("screenWidth", static_cast<float>(win_.getSize().x));
            pixelShader_.setUniform("screenHeight", static_cast<float>(win_.getSize().y));

            win_.draw(screenQuad, &pixelShader_);
        } else if (gameOverState_ == GameOverState::ShowingImage) {
            // Set view to default to cover entire window
            win_.setView(win_.getDefaultView());

            // Draw full black background
            sf::RectangleShape blackBg({win_.getSize().x, win_.getSize().y});
            blackBg.setPosition({0.f, 0.f});
            blackBg.setFillColor(sf::Color::Black);
            win_.draw(blackBg);

            // Scale game over image to cover entire window
            float scaleX = static_cast<float>(win_.getSize().x) / gameOverTexture_.getSize().x;
            float scaleY = static_cast<float>(win_.getSize().y) / gameOverTexture_.getSize().y;
            float scale = std::max(scaleX, scaleY);
            gameOverSprite_->setScale(sf::Vector2f(scale+0.4f, scale+0.4f));

            // Draw game over image
            win_.draw(*gameOverSprite_);

            // Draw return button
            if (returnButton_) {
                returnButton_->draw(win_);
            }
        }
    }

}

// ====== ★ NEW : layout + draw menu ======
void GameScene::updateMenuLayout(){
    const sf::Vector2f posOpen (40.f, 150.f);
    const sf::Vector2f posClosed(-520.f, 40.f);
    sf::Vector2f pos = posClosed + (posOpen - posClosed) * menuAnim_;

    // bg
    if (menuBg_){
        menuBg_->setPosition(pos);
        
        menuBg_->setColor(sf::Color(255,255,255,(std::uint8_t)menuAlpha_));
    }

    // titre
    if (uiTitle_){
        uiTitle_->setPosition(pos + sf::Vector2f(24.f, 18.f));
        uiTitle_->setFillColor(sf::Color(255,255,255,(std::uint8_t)menuAlpha_));
    }

    // slots matériaux
    for (int i=0;i<3;++i){
        sf::Vector2f p = pos + sf::Vector2f(28.f + i*90.f, 80.f);
        matSlots_[i].setPosition(p);
        auto c  = matSlots_[i].getFillColor();    c.a  = (std::uint8_t)menuAlpha_; matSlots_[i].setFillColor(c);
        auto oc = matSlots_[i].getOutlineColor(); oc.a = (std::uint8_t)menuAlpha_; matSlots_[i].setOutlineColor(oc);
    }

    // boutons unités
    for (int i=0;i<3;++i){
        sf::Vector2f p = pos + sf::Vector2f(60.f + i*120.f, 220.f);
        unitBtns_[i].setPosition(p);
        auto c = unitBtns_[i].getFillColor();
        c.a = (std::uint8_t)( (unitAffordable_[i] ? 1.f : 0.6f) * menuAlpha_ );
        unitBtns_[i].setFillColor(c);
        auto oc = unitBtns_[i].getOutlineColor(); oc.a = (std::uint8_t)menuAlpha_; unitBtns_[i].setOutlineColor(oc);
    }
}


void GameScene::drawMenu(){
    // fond
    if (menuBg_) {
        win_.draw(*menuBg_);
    } else {
        sf::RectangleShape panel(sf::Vector2f(520.f, 320.f));
        const sf::Vector2f posOpen (40.f, 40.f);
        const sf::Vector2f posClosed(-520.f, 40.f);
        sf::Vector2f pos = posClosed + (posOpen - posClosed) * menuAnim_;
        panel.setPosition(pos);
        panel.setFillColor(sf::Color(30,30,38,(std::uint8_t)(220 * menuAnim_)));
        panel.setOutlineThickness(2.f);
        panel.setOutlineColor(sf::Color(200,200,220,(std::uint8_t)(220 * menuAnim_)));
        win_.draw(panel);
    }

    if (uiTitle_) win_.draw(*uiTitle_);

    // matériaux + compteurs
    for (int i=0;i<3;++i){
        win_.draw(matSlots_[i]);

        sf::Text count("", *uiFont_, 18);
        if (!uiFont_->getInfo().family.empty()) count.setFont(*uiFont_);
        count.setString(std::to_string(materialCount_[i]));
        count.setCharacterSize(18);
        count.setFillColor(sf::Color::Black);

        auto rect = matSlots_[i].getGlobalBounds(); // SFML2
        count.setPosition(sf::Vector2f(rect.left + rect.width + 8.f,
                                       rect.top + 24.f));
        win_.draw(count);
    }

    // unités
    for (int i=0;i<3;++i) win_.draw(unitBtns_[i]);
}

void GameScene::generateWaves() {
    waves_.clear();
    float time = 0.f;
    int grunt = 1;
    int rogue = 1;
    int golem = 1;
    for (int i = 0; i < 10; ++i) { // Generate 10 waves for example
        waves_.push_back({grunt, rogue, golem, time});
        grunt *= 2; // Double grunts each wave (+1 times more, i.e., double)
        rogue += 2; // Add 2 rogues each wave
        if ((i + 1) % 2 == 0) golem += 1; // Add 1 golem every 2 waves
        time += 15.f; // 15 seconds delay between waves
    }
}

void GameScene::spawnNextWave() {
    if (currentWaveIndex_ < waves_.size()) {
        const auto& wave = waves_[currentWaveIndex_];
        float speedMultiplier = 1.0f + static_cast<float>(currentWaveIndex_) * 0.1f; // Increase speed by 10% per wave
        creeps_.spawnWave(wave.grunt, wave.rogue, wave.golem, wave.spawnTime, 0.5f / speedMultiplier); // Adjust period for speed
        currentWaveIndex_++;
    }
}

void GameScene::stopGameSceneSounds() {
    // Stop any game-related sounds
    // Note: We don't have direct access to AudioManager, so we can't stop music here
    // This would need to be handled at the App level
}

void GameScene::playGameOverSounds() {
    // Stop game scene sounds and play game over sounds
    stopGameSceneSounds();
    if (gameOverSound_) {
        gameOverSound_->play();
    }
}

void GameScene::loadDropTextures() {
    dropTextures_[DropType::Wood] = std::make_unique<sf::Texture>();
    if (!dropTextures_[DropType::Wood]->loadFromFile("../assets/ui/ico_drop/ico_wood.png")) {
        // Handle error if needed
    }
    dropTextures_[DropType::Wood]->setSmooth(false);

    dropTextures_[DropType::Stone] = std::make_unique<sf::Texture>();
    if (!dropTextures_[DropType::Stone]->loadFromFile("../assets/ui/ico_drop/ico_stone.png")) {
        // Handle error if needed
    }
    dropTextures_[DropType::Stone]->setSmooth(false);

    dropTextures_[DropType::Crystal] = std::make_unique<sf::Texture>();
    if (!dropTextures_[DropType::Crystal]->loadFromFile("../assets/ui/ico_drop/ico_crystal.png")) {
        // Handle error if needed
    }
    dropTextures_[DropType::Crystal]->setSmooth(false);

    dropTextures_[DropType::Resource] = std::make_unique<sf::Texture>();
    if (!dropTextures_[DropType::Resource]->loadFromFile("../assets/ui/ico_drop/ico_ressource.png")) {
        // Handle error if needed
    }
    dropTextures_[DropType::Resource]->setSmooth(false);
}

void GameScene::spawnDrop() {
    if (dropTextures_.empty()) loadDropTextures();

    // Random drop type based on rarity (more kills = higher chance for better drops)
    float rarity = static_cast<float>(totalKills_) / 100.f; // Example: every 100 kills increase rarity
    rarity = std::min(rarity, 1.f); // Cap at 1.0

    DropType type;
    float rand = static_cast<float>(rng_()) / static_cast<float>(rng_.max());
    if (rand < 0.4f - rarity * 0.2f) { // Wood: 40% - 20% = 20% at max rarity
        type = DropType::Wood;
    } else if (rand < 0.7f - rarity * 0.3f) { // Stone: 30% - 30% = 0% at max rarity
        type = DropType::Stone;
    } else if (rand < 0.9f - rarity * 0.1f) { // Crystal: 20% - 10% = 10% at max rarity
        type = DropType::Crystal;
    } else { // Resource: 10% + rarity adjustments
        type = DropType::Resource;
    }

    // Random valid position on the map (Water or Forest tiles that are unoccupied)
    sf::Vector2f pos;
    int attempts = 0;
    do {
        int tx = rng_() % worldW_;
        int ty = rng_() % worldH_;
        const auto& tile = map_.at(tx, ty);
        if ((tile.ground == Tile::Water || tile.ground == Tile::Forest) && !buildOcc_[ty * worldW_ + tx]) {
            pos.x = static_cast<float>(tx) * tileSize_;
            pos.y = static_cast<float>(ty) * tileSize_;
            break;
        }
        attempts++;
    } while (attempts < 100); // Prevent infinite loop
    if (attempts >= 100) {
        // Fallback to random position if no valid tile found
        pos.x = static_cast<float>(rng_() % worldW_) * tileSize_;
        pos.y = static_cast<float>(rng_() % worldH_) * tileSize_;
    }

    sf::Sprite sprite(*dropTextures_[type]);
    sprite.setPosition(pos);
    sprite.setScale({0.1f, 0.1f}); // Smaller size
    sprite.setOrigin(sprite.getLocalBounds().width / 2.f, sprite.getLocalBounds().height / 2.f);

    Drop drop(type, pos, 15.f, std::move(sprite)); // 15 seconds lifetime

    drops_.push_back(drop);
}

void GameScene::updateDrops(float dt) {
    // Spawn drops periodically (3 per minute = every 20 seconds)
    lastDropSpawnTime_ += dt;
    if (lastDropSpawnTime_ >= 20.f) {
        spawnDrop();
        lastDropSpawnTime_ = 0.f;
    }

    // Update existing drops
    for (auto it = drops_.begin(); it != drops_.end(); ) {
        it->lifetime -= dt;
        if (it->lifetime <= 0) {
            it = drops_.erase(it);
        } else {
            ++it;
        }
    }
}

void GameScene::handleDropClick(sf::Vector2f clickPos) {
    for (auto it = drops_.begin(); it != drops_.end(); ++it) {
        sf::FloatRect bounds = it->sprite.getGlobalBounds();
        if (bounds.contains(clickPos)) {
            // Collect the drop
            switch (it->type) {
                case DropType::Wood:
                    materialCount_[0]++;
                    break;
                case DropType::Stone:
                    materialCount_[1]++;
                    break;
                case DropType::Crystal:
                    materialCount_[2]++;
                    break;
                case DropType::Resource:
                    resourceCount_++;
                    break;
            }
            // Remove the drop
            drops_.erase(it);
            break; // Only collect one drop per click
        }
    }
}

void GameScene::drawDrops() {
    for (const auto& drop : drops_) {
        win_.draw(drop.sprite);
    }
}

void GameScene::applyDifficulty(const std::string& difficulty) {
    if (difficulty == "Easy") {
        difficultyParams_.hpMul = 0.8f;
        difficultyParams_.speedMul = 0.7f;
        difficultyParams_.rewardMultiplier = 1.2f;
        difficultyParams_.lives = 25;
        difficultyParams_.startResources = 25;
    } else if (difficulty == "Normal") {
        difficultyParams_.hpMul = 1.0f;
        difficultyParams_.speedMul = 1.f;
        difficultyParams_.rewardMultiplier = 1.0f;
        difficultyParams_.lives = 20;
        difficultyParams_.startResources = 20;
    } else if (difficulty == "Hard") {
        difficultyParams_.hpMul = 1.3f;
        difficultyParams_.speedMul = 1.5f;
        difficultyParams_.rewardMultiplier = 0.9f;
        difficultyParams_.lives = 15;
        difficultyParams_.startResources = 15;
    } else if (difficulty == "Legendary") {
        difficultyParams_.hpMul = 1.5f;
        difficultyParams_.speedMul = 2.f;
        difficultyParams_.rewardMultiplier = 0.8f;
        difficultyParams_.lives = 10;
        difficultyParams_.startResources = 10;
    } else {
        // Default to Normal
        difficultyParams_.hpMul = 1.0f;
        difficultyParams_.speedMul = 1.0f;
        difficultyParams_.rewardMultiplier = 1.0f;
        difficultyParams_.lives = 20;
        difficultyParams_.startResources = 20;
    }
}

