#include "GameScene.hpp"
#include <filesystem>

// (optionnel) vue pixel-perfect pour éviter le flou
static void setPixelPerfectView(sf::RenderWindow& win, int worldW, int worldH, float tileSize){
    sf::View v;
    v.setSize(sf::Vector2f(worldW * tileSize, worldH * tileSize));
    v.setCenter(sf::Vector2f((worldW * tileSize) * 0.5f, (worldH * tileSize) * 0.5f));
    win.setView(v);
}


GameScene::GameScene(sf::RenderWindow& win) : win_(win) {
    const int W = 60, H = 24;
    tileSize_  = 64.f; // correspond à l’atlas 64×64

    // 1) Charger l’atlas (12 colonnes, 1 ligne)
    //    0:Grass, 1:Path, 2..10:Water variants, 11:FREE (zone libre/pads)
    terrain_.loadFromFile("../assets/tiles/terrain_atlas_z.png");
    terrain_.setSmooth(false);
    terrain_.setRepeated(false);

    // 2) Générer la map
    MapGenerator gen;
    map_ = gen.generate(W, H, /*entrées*/1, 2, /*sorties*/1, 2);

    // 3) Construire la géométrie
    tilemap_.setTexture(terrain_, {64, 64});
    tilemap_.build(map_, tileSize_);

    // 3.5) Vue pixel-perfect (évite le flou si la fenêtre est plus grande)
    setPixelPerfectView(win_, W, H, tileSize_);

    // 4) Arbres (assure-toi que Trees::generate évite Path/Water/Tile::Rock(pads))
    trees_.loadTextures("../assets/trees");  // tree_1.png .. tree_3.png
    const unsigned treeCount = static_cast<unsigned>((W * H) / 18);
    trees_.generate(map_, tileSize_, treeCount, rng_, /*roadPaddingTiles=*/1);

    // 5) Bâtiment de ressources — centré (3×3 cases)
    {
        bool ok = resourceTex_.loadFromFile("../assets/buildings/resources_build.jpg");
        if (!ok) (void)resourceTex_.loadFromFile("../assets/tiles/resources_build-1.png");
        resourceTex_.setSmooth(false);

        // Création des sprites maintenant que la texture existe (SFML3)
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
}

void GameScene::handleInput(bool, bool, bool) {}
void GameScene::update(float) {}

void GameScene::draw() {
    win_.draw(tilemap_);
    if (resourceShadow_) win_.draw(*resourceShadow_);
    if (resourceSprite_) win_.draw(*resourceSprite_);
    win_.draw(trees_);
}

