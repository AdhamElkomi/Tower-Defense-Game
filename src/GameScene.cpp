#include "GameScene.hpp"

#include <filesystem>

GameScene::GameScene(sf::RenderWindow& win) : win_(win) {
    const int W = 60, H = 34;

    // 1) Charger l’atlas (ex: 6 colonnes x 1 ligne, tiles 32x32)
    terrain_.loadFromFile("../assets/tiles/terrain_atlas.png");

    // 2) Générer la map (assure-toi que MapGenerator.hpp/cpp sont dans le projet)
    MapGenerator gen;
    map_ = gen.generate(W, H, /*entrées*/1, 2, /*sorties*/1, 2);

    // 3) Construire la géométrie
    tilemap_.setTexture(terrain_, {32, 32});
    tilemap_.build(map_, tileSize_);

    // 4) Charger les 3 arbres et en générer un certain nombre
    trees_.loadTextures("../assets/trees");  // tree_1.png .. tree_3.png
    const unsigned treeCount = (unsigned)((W * H) / 18); // densité: 1 arbre / 18 tuiles (à ajuster)
    trees_.generate(map_, tileSize_, treeCount, rng_, /*roadPaddingTiles=*/1);
}

void GameScene::handleInput(bool, bool, bool) {}
void GameScene::update(float) {}
void GameScene::draw() { win_.draw(tilemap_);   win_.draw(trees_); }
