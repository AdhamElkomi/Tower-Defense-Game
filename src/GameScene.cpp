#include "GameScene.hpp"
#include <filesystem>
#include <algorithm>  // min_element, max_element
#include <array>  


void GameScene::setPixelPerfectView(int worldW, int worldH, float tileSize){
    sf::View v;
    v.setSize(sf::Vector2f(worldW * tileSize, worldH * tileSize));
    v.setCenter(sf::Vector2f((worldW * tileSize) * 0.5f, (worldH * tileSize) * 0.5f));
    win_.setView(v);
}

// Suit les tuiles Tile::Path depuis un bord → un autre (polyline en pixels)
WaypointPath GameScene::buildMainPathPolyline(const Map& m, float tileSize) const {
    const int W = m.w, H = m.h;
    auto centerPx = [&](int x,int y){
        return sf::Vector2f( (x + 0.5f) * tileSize, (y + 0.5f) * tileSize );
    };
    auto inB = [&](int x,int y){ return x>=0 && y>=0 && x<W && y<H; };

    // 1) collecter les cases Path situées sur les bords
    std::vector<sf::Vector2i> edge;
    for (int x=0;x<W;++x){
        if (m.at(x,0).ground==Tile::Path)     edge.push_back({x,0});
        if (m.at(x,H-1).ground==Tile::Path)   edge.push_back({x,H-1});
    }
    for (int y=0;y<H;++y){
        if (m.at(0,y).ground==Tile::Path)     edge.push_back({0,y});
        if (m.at(W-1,y).ground==Tile::Path)   edge.push_back({W-1,y});
    }

    // fallback : si pas de case path sur bord, prendre une case path quelconque
    if (edge.empty()){
        for (int y=0;y<H;++y){
            bool br=false;
            for(int x=0;x<W;++x){
                if (m.at(x,y).ground==Tile::Path){ edge.push_back({x,y}); br=true; break; }
            }
            if (br) break;
        }
    }
    if (edge.empty()) return {}; // pas de chemin

    // choisir start = edge au plus petit (x+y), end = edge au plus grand (x+y)
    auto start = *std::min_element(edge.begin(), edge.end(),
        [](auto a, auto b){ return (a.x+a.y) < (b.x+b.y); });
    auto goal  = *std::max_element(edge.begin(), edge.end(),
        [](auto a, auto b){ return (a.x+a.y) < (b.x+b.y); });

    // 2) marcher de case en case en suivant la route
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

        // trouver un voisin Path (priorité non-visité)
        sf::Vector2i next{-999,-999};
        int choicesVisited = 0;

        for(auto n : neigh4(cur)){
            if (!inB(n.x,n.y)) continue;
            if (m.at(n.x,n.y).ground != Tile::Path) continue;
            if (n == prev) continue;

            if (!seen[id(n.x,n.y)]) { next = n; break; }
            // sinon, on garde une option visitée comme fallback (évite blocage)
            if (next.x == -999){ next = n; ++choicesVisited; }
        }

        if (next.x == -999) break; // pas de suite

        prev = cur;
        cur  = next;
        if (!seen[id(cur.x,cur.y)]){
            path.pts.push_back(centerPx(cur.x,cur.y));
            seen[id(cur.x,cur.y)] = 1;
        }
    }

    // Nettoyage : si trop court, renvoyer vide
    if (path.pts.size() < 2) path.pts.clear();
    return path;
}







GameScene::GameScene(sf::RenderWindow& win) : win_(win) {
    const int W = 60, H = 24;
    tileSize_  = 64.f; // atlas en 64×64

    // 1) Charger l’atlas
    (void)terrain_.loadFromFile("../assets/tiles/terrain_atlas_z.png");
    terrain_.setSmooth(false);
    terrain_.setRepeated(false);

    // 2) Générer la map
    MapGenerator gen;
    map_ = gen.generate(W, H, /*entrées*/1, 2, /*sorties*/1, 2);

    // 3) Construire la géométrie
    tilemap_.setTexture(terrain_, {64, 64});
    tilemap_.build(map_, tileSize_);

    // 3.5) Vue pixel-perfect
    setPixelPerfectView(W, H, tileSize_);

    // 4) Arbres
    trees_.loadTextures("../assets/trees");  // tree_1.png..tree_3.png
    const unsigned treeCount = static_cast<unsigned>((W * H) / 18);
    trees_.generate(map_, tileSize_, treeCount, rng_, /*roadPaddingTiles=*/1);

    // 5) Bâtiment ressources au centre (6×6 visuel)
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

    // 6) Créatures : textures, chemin et 1 vague
    {
        creeps_.loadTextures();

        // construit une polyline basée sur les cases Path de la carte
        WaypointPath p = buildMainPathPolyline(map_, tileSize_);
        if (!p.pts.empty()){
            creeps_.setPath(p);
            // vague de test
            creeps_.spawnWave(/*grunt*/1, /*rogue*/2, /*golem*/1,
                              /*startTime*/ 0.2f,
                              /*period*/    0.9f);
        }
    }
}
void GameScene::handleInput(bool, bool, bool) {}
void GameScene::update(float dt) {
     gameTime_ += dt;
    creeps_.update(dt, gameTime_);
}

void GameScene::draw() {
      // ordre : sol → arbres → creeps → bâtiment (pour le mettre en valeur)
    win_.draw(tilemap_);
    win_.draw(trees_);
    creeps_.draw(win_);
    if (resourceShadow_) win_.draw(*resourceShadow_);
    if (resourceSprite_) win_.draw(*resourceSprite_);
}

