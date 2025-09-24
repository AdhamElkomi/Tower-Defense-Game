#include "GameScene.hpp"
#include <filesystem>
#include <algorithm>
#include <array>
#include <cstdint>
#include "BuildMenu.hpp"
#include "Button.hpp"

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
        (void)menuButtonTex_.loadFromFile("../assets/ui/menu_button.png");
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
        }


    // slots matériaux (carrés radius 10%)
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
        unitBtns_[i].setFillColor(sf::Color(255,255,255, unitAffordable_[i] ? 255 : 120)); // “luminosité”
        unitBtns_[i].setOutlineThickness(3.f);
        unitBtns_[i].setOutlineColor(sf::Color(30,30,30,220));
    }

    // position initiale (fermé)
    updateMenuLayout();
}

// ---- input ----
void GameScene::handleInput(bool mouseLeft, bool mouseLeftReleased, bool /*mouseMoved*/){
    sf::Vector2i mp = sf::Mouse::getPosition(win_);
    sf::Vector2f world = win_.mapPixelToCoords(mp);

    // toggle bouton menu
    // handleInput: test du bouton
        if (mouseLeftReleased && menuButton_ && menuButton_->getGlobalBounds().contains(world)){
            menuOpen_ = !menuOpen_;
        }

    // si menu visible presque totalement, on pourrait traiter drag/drop ici (à ajouter plus tard)
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

    updateMenuLayout();
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

        // draw: panneau
    if (menuAnim_ > 0.01f) drawMenu();
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

