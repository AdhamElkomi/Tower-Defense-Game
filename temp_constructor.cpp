GameScene::GameScene(sf::RenderWindow& win, const std::string& difficulty, const std::string& username) : win_(win), username_(username), difficulty_(difficulty) {
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
        unitBtns_[i].setFillColor(sf::Color(255,255,255, unitAffordable_[i] ? 255 : 120)); // “luminosité”
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
}
