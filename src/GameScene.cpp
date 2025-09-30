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
    const int W = 60, H = 24;
    tileSize_  = 64.f;
     worldW_ = W; worldH_ = H;

    (void)terrain_.loadFromFile("../assets/tiles/terrain_atlas_z.png");
    terrain_.setSmooth(false);
    terrain_.setRepeated(false);

    MapGenerator gen;
    map_ = gen.generate(W, H, /*entrées*/1, 2, /*sorties*/1, 2);

    tilemap_.setTexture(terrain_, {64, 64});
    tilemap_.build(map_, tileSize_);

    setPixelPerfectView(W, H, tileSize_);

    trees_.loadTextures("../assets/trees");
    const unsigned treeCount = static_cast<unsigned>((W * H) / 18);
    trees_.generate(map_, tileSize_, treeCount, rng_, /*roadPaddingTiles=*/1);

    {
        bool ok = resourceTex_.loadFromFile("../assets/buildings/resources_build.jpg");
        if (!ok) (void)resourceTex_.loadFromFile("../assets/tiles/resources_build-1.png");
        resourceTex_.setSmooth(false);

        resourceSprite_ = std::make_unique<sf::Sprite>(resourceTex_);
        resourceShadow_ = std::make_unique<sf::Sprite>(resourceTex_);

        const auto texSz = resourceTex_.getSize();
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
        WaypointPath p = buildMainPathPolyline(map_, tileSize_);
        if (!p.pts.empty()){
            creeps_.setPath(p);
            creeps_.spawnWave(1, 2, 1, 0.2f, 0.9f);
        }
    }

// ====== UI – bouton menu ======
       /*  (void)menuButtonTex_.loadFromFile("../assets/ui/menu_button.png");
        menuButton_ = std::make_unique<sf::Sprite>(menuButtonTex_);
        menuButton_->setScale(sf::Vector2f(0.4f, 0.4f));
        menuButton_->setPosition(sf::Vector2f(20.f, 5.f));

        // ====== UI – panneau ======
        (void)menuBgTex_.loadFromFile("../assets/ui/menu_bg.png");
        menuBg_ = std::make_unique<sf::Sprite>(menuBgTex_);
        menuBg_->setScale(sf::Vector2f(1.4f, 1.1f));

        // police / titre
        if (uiFont_.openFromFile("../assets/fonts/Roboto-Regular_2.ttf")) {  // SFML3
            uiTitle_ = std::make_unique<sf::Text>(uiFont_, "Armory & Supplies", 28);
            uiTitle_->setFillColor(sf::Color::White);
        }*/


    // slots matériaux (carrés radius 10%)
    for (int i=0;i<3;++i){
        matSlots_[i].setSize(sf::Vector2f(72.f, 72.f));
        matSlots_[i].setFillColor(sf::Color(220,220,220,240));
        matSlots_[i].setOutlineThickness(2.f);
        matSlots_[i].setOutlineColor(sf::Color(40,40,40,220));
        // Border radius visuel avec un petit arrondi = on peut simuler via texture / ou laisser carré propre
    }

    // boutons unités (ronds) + luminance selon affordability
   /*  for (int i=0;i<3;++i){
        unitBtns_[i] = sf::CircleShape(40.f);
        unitBtns_[i].setFillColor(sf::Color(255,255,255, unitAffordable_[i] ? 255 : 120)); // “luminosité”
        unitBtns_[i].setOutlineThickness(3.f);
        unitBtns_[i].setOutlineColor(sf::Color(30,30,30,220));
    }*/
    // GameScene.cpp (fin du constructeur)
    menu_ = std::make_unique<BuildMenu>(win_, tileSize_, map_);

    // load tower icon texture
    (void)cannonIconTex_.loadFromFile("../assets/ui/tower_build/cannon.png");
    cannonIconTex_.setSmooth(false);
    buildOcc_.assign(W*H, 0);


    // position initiale (fermé)
    updateMenuLayout();
}

bool cannonAvailable_ = true;

void GameScene::placeCannon(sf::Vector2f center){
    if (!cannonAvailable_) return;

    // Ensure we have a texture (lazy-load once)
    if (cannonTex_.getSize().x == 0) {
        (void)cannonTex_.loadFromFile("../assets/ui/tower-build/cannon.png");
        cannonTex_.setSmooth(false);
    }

    towers_.push_back(std::make_unique<CannonTower>(center, cannonTex_));
    cannonAvailable_ = false;

    if (menu_) menu_->setUnitEnabled(BuildMenu::Unit::Cannon, false);
}



// ---- input ----
void GameScene::handleInput(bool leftDown, bool leftUp, bool moved){
    sf::Vector2f world = win_.mapPixelToCoords(sf::Mouse::getPosition(win_));
    if (leftDown)  menu_->onMousePressed(world);
    if (moved)     menu_->onMouseMoved(world);
    if (leftUp)    menu_->onMouseReleased(world);

     sf::Vector2i mp = sf::Mouse::getPosition(win_);

    // --- placement after drag: one cannon only ---
    // If your BuildMenu exposes: isDragging(), draggingUnit(), dragPosition()
    if (leftUp && cannonAvailable_){
        // Accept placement if cursor currently valid & we are (conceptually) dragging a cannon
        // If your BuildMenu is a separate object with state, you can also check that.
        // Here we just check buildability at the release point.
        if (isBuildableAtPixel(world)){
            // Snap to tile center
            int tx = int(world.x / tileSize_);
            int ty = int(world.y / tileSize_);
            sf::Vector2f center( (tx + 0.5f) * tileSize_, (ty + 0.5f) * tileSize_ );

            towers_.push_back(std::make_unique<CannonTower>(center, cannonIconTex_));
            cannonAvailable_ = false; // disable further placement for now
        }
    }

    // --- manual fire: click inside any tower radius to shoot at mouse point ---
    if (leftUp){
        for (auto& t : towers_){
            // only fire if the click is inside this tower's radius
            sf::Vector2f d = world - t->pos();
            if (d.x*d.x + d.y*d.y <= t->radius()*t->radius()){
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
                placeCannon(sf::Vector2f(cx,cy)); // (see §2 below)
                menu_->endDrag();                  // tell menu to stop dragging
            }
        }
        return;
    }else{
        draggingUnit_ = false;
    }

    // Map mouse + keyboard
        //sf::Vector2f world = win_.mapPixelToCoords(sf::Mouse::getPosition(win_));
        bool esc = sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Escape);
        bool rmb = sf::Mouse::isButtonPressed(sf::Mouse::Button::Right);

        // --- exit aiming if ESC or RMB ---
        if (esc || rmb) {
            deselectTower();
        }

        // --- select tower when you click inside its radius, but only if not already aiming
        if (leftDown && !isAiming()) {
            for (int i = (int)towers_.size()-1; i >= 0; --i) { // top-most first
                auto& t = towers_[i];
                sf::Vector2f d = world - t->pos();
                if (d.x*d.x + d.y*d.y <= t->radius()*t->radius()) {
                    activeTowerIndex_ = i; // now we’re aiming this tower
                    break;
                }
            }
        }

        // --- fire only if we are aiming and release LMB inside range
        if (leftUp && isAiming()) {
            auto* t = getActiveAimingTower();
            if (t) t->tryFireAt(world, creeps_);
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

// For now, return the last placed tower as the “active” one
CannonTower* GameScene::getActiveAimingTower(){
    if (towers_.empty()) return nullptr;
    return towers_.back().get();
}




// ---- update ----
void GameScene::update(float dt) {
    // créatures/time
    gameTime_ += dt;
    creeps_.update(dt, gameTime_);

    // ====== ★ NEW : animation du menu ======
    float target = menuOpen_ ? 1.f : 0.f;
    if (menuAnim_ < target) menuAnim_ = std::min(target, menuAnim_ + dt * menuAnimSpeed_);
    if (menuAnim_ > target) menuAnim_ = std::max(target, menuAnim_ - dt * menuAnimSpeed_);
    menuAlpha_ = 255.f * menuAnim_;

    menu_->update(dt);
    updateMenuLayout();

     // towers
    for (auto& t : towers_) t->update(dt, creeps_);
    towers_.erase(
        std::remove_if(towers_.begin(), towers_.end(),
                       [](const std::unique_ptr<CannonTower>& t){ return t->isDead(); }),
        towers_.end()
    );
    // Collect drops from towers (their local batches)
    for (auto& t : towers_) t->extractDrops(pendingDrops_);

    // Also collect any drops the CreatureSystem queued internally (safety)
    creeps_.extractPendingDrops(pendingDrops_);

    // Credit inventory
    for (const auto& d : pendingDrops_) {
        switch (d.type) {
            case Material::Wood:    materialCount_[0]++; break;
            case Material::Stone:   materialCount_[1]++; break;
            case Material::Crystal: materialCount_[2]++; break;
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
    creeps_.draw(win_);
    if (resourceShadow_) win_.draw(*resourceShadow_);
    if (resourceSprite_) win_.draw(*resourceSprite_);

    // bouton menu (toujours visible)
   // draw: bouton menu
    if (menuButton_) win_.draw(*menuButton_);

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
                sf::Vertex line[2];
                line[0] = sf::Vertex(tower->pos(), inRange ? sf::Color(40,140,60,220)
                                                        : sf::Color(180,60,60,220));
                line[1] = sf::Vertex(m,            inRange ? sf::Color(20,100,40,220)
                                                        : sf::Color(160,40,40,220));
                win_.draw(line, 2, sf::PrimitiveType::Lines);
        }



        // draw: panneau
    if (menuAnim_ > 0.01f) drawMenu();

    menu_->draw(win_);

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

        sf::Text count(uiFont_, "", 18);  
        if (!uiFont_.getInfo().family.empty()) count.setFont(uiFont_);
        count.setString(std::to_string(materialCount_[i]));
        count.setCharacterSize(18);
        count.setFillColor(sf::Color::Black);

        auto rect = matSlots_[i].getGlobalBounds(); // SFML3
        count.setPosition(sf::Vector2f(rect.position.x + rect.size.x + 8.f,
                                       rect.position.y + 24.f));
        win_.draw(count);
    }

    // unités
    for (int i=0;i<3;++i) win_.draw(unitBtns_[i]);
}

