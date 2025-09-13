#include "Menu.hpp"
#include <algorithm>

// Couvre l'écran en conservant le ratio
static void centerSpriteToCover(sf::Sprite& sp, const sf::RenderWindow& win) {
    const auto& tex = sp.getTexture(); // ✅ référence (pas de test null)
    const float TW = static_cast<float>(tex.getSize().x);
    const float TH = static_cast<float>(tex.getSize().y);
    const float W  = static_cast<float>(win.getSize().x);
    const float H  = static_cast<float>(win.getSize().y);
    const float s  = std::max(W / TW, H / TH);
    sp.setScale(sf::Vector2f(s, s));
    sp.setPosition(sf::Vector2f(0.f, 0.f));
}

void MenuScene::update(float /*dt*/) {
    // Laisse vide pour l’instant, ou ajoute des animations UI si nécessaire.
}

MenuScene::MenuScene(sf::RenderWindow& win) : win_(win) {
    // Chargement assets (tu peux vérifier les bool si tu veux logger)
    font_.openFromFile("../assets/fonts/Roboto-Regular.ttf");
    bgTex_.loadFromFile("../assets/images/background.png");
    panelTex_.loadFromFile("../assets/images/first-bg.png");
    gearTex_.loadFromFile("../assets/images/gear.png");

    // Construire éléments dépendants des assets
    bgSprite_.emplace(bgTex_);
    panelSprite_.emplace(panelTex_);
    title_.emplace(font_, "TOWER DEFENSE", 30);

    // Redimensionner le panel à une taille cible (par ex. 600x400)
    sf::Vector2f targetSize{700.f, 650.f};
    sf::Vector2u texSize = panelTex_.getSize();

    float scaleX = targetSize.x / static_cast<float>(texSize.x);
    float scaleY = targetSize.y / static_cast<float>(texSize.y);
    panelSprite_->setScale(sf::Vector2f(scaleX, scaleY));

    // Centrer le panel dans la fenêtre
    sf::Vector2f panelPos{
        (win_.getSize().x - targetSize.x) * 0.5f ,
        (win_.getSize().y - targetSize.y) * 0.5f 
    };
    panelSprite_->setPosition(panelPos);

    centerSpriteToCover(*bgSprite_, win_); // ✅ déréférence

    // Panel centré
    const auto plb = panelSprite_->getLocalBounds(); // ✅ local bounds pour taille
    panelSprite_->setPosition(sf::Vector2f(
        (win_.getSize().x - plb.size.x) * 0.5f + 160.f,
        (win_.getSize().y - plb.size.y) * 0.5f + 200.f
    ));

    // Titre
    title_->setFillColor(theme_.text);
    const auto tb = title_->getLocalBounds();
    title_->setPosition(sf::Vector2f(
        (float)win_.getSize().x * 0.5f - tb.size.x * 0.5f - tb.position.x,
        30.f - tb.position.y
    ));

    // Boutons
    sf::Vector2f btnSize{320.f, 72.f};
    btnStart_.emplace(font_, btnSize, "START",      sf::Color(30,144,255), sf::Color::White, 3.f);
    btnDifficulty_.emplace(font_, btnSize, "DIFFICULTY", sf::Color(76,175,80),  sf::Color::White, 3.f);
    btnExit_.emplace(font_, btnSize, "EXIT",       sf::Color(244,67,54),  sf::Color::White, 3.f);


    btnStart_->setHoverTint(theme_.btnHover);
    btnDifficulty_->setHoverTint(theme_.btnHover);
    btnExit_->setHoverTint(theme_.btnHover);

    // Position des boutons sur le panel
    const float x = panelSprite_->getPosition().x + 200.f;
    const float y = panelSprite_->getPosition().y + 180.f;
    float gap     = 100.f; // espace vertical entre boutons
    

    btnStart_->setPosition(sf::Vector2f(x, y));
    btnDifficulty_->setPosition(sf::Vector2f(x, y + gap));
    btnExit_->setPosition(sf::Vector2f(x, y + 2*gap));

    // Labels sliders (optionnels, car sf::Text non default-constructible)
    musicLabel_.emplace(font_, "Music Volume", 18);
    sfxLabel_.emplace  (font_, "SFX Volume",   18);
    musicLabel_->setFillColor(theme_.text);
    sfxLabel_->setFillColor(theme_.text);

    // Position sliders et labels (exemple)
    const float sx = x;
    const float sy = y + 3*gap + 30.f;
    musicLabel_->setPosition(sf::Vector2f(sx+480.f, sy+10.f));
    musicSlider_.setPosition  ({sx+ 480.f, sy + 50.f});
    sfxLabel_->setPosition   (sf::Vector2f(sx+480.f, sy + 88.f));
    sfxSlider_.setPosition    ({sx+480.f, sy + 128.f});

    // Audio
    audio_.load();
    audio_.playMenuLoop(0.7f);

    // Callbacks
    btnStart_->setOnClick([this]() {
        started_ = true;              // ✅ membre existant
        audio_.playClick();
        audio_.playGameLoop(0.7f);    // musique du jeu
           // 🚀 Basculer vers la GameScene
        if (onStartGame_) onStartGame_();
    });

    btnDifficulty_->setOnClick([this]() {
        static const char* levels[] = {"Easy","Normal","Hard"};
        idxDiff_ = (idxDiff_ + 1) % 3;
        difficulty_ = levels[idxDiff_];
        audio_.playClick();
    });

    btnExit_->setOnClick([this]() {
        audio_.playClick();
        win_.close();
    });

    // Settings bouton (icône)
    settingsBtn_.setPosition(sf::Vector2f(
        panelSprite_->getPosition().x + plb.size.x - 56.f,
        panelSprite_->getPosition().y + 600.f
    ));
    settingsBtn_.setOnClick([this]() {
        settingsOpen_ = !settingsOpen_;
        audio_.playClick();
    });
    // --- BG plein écran (si tu utilises bg.png)
   
}


void MenuScene::handleInput(bool mpLeft, bool mrLeft, bool mMoved) {
    auto clickSfx = [this](){ audio_.playClick(); };

    if (btnStart_)      btnStart_->handleInput(win_, mpLeft, clickSfx);
    if (btnDifficulty_) btnDifficulty_->handleInput(win_, mpLeft, clickSfx);
    if (btnExit_)       btnExit_->handleInput(win_, mpLeft, clickSfx);

    settingsBtn_.handleInput(win_, mpLeft, clickSfx);  // ✅ nouvelle API


    if (settingsOpen_) {
        musicSlider_.handleInput(win_, mpLeft, mrLeft, mMoved);
        sfxSlider_.handleInput(win_, mpLeft, mrLeft, mMoved);
        audio_.setMusicVolume(musicSlider_.value());
        audio_.setSfxVolume(sfxSlider_.value());
    }
}

void MenuScene::draw() {
    if (bgSprite_)    win_.draw(*bgSprite_);
    if (panelSprite_) win_.draw(*panelSprite_);
    if (title_)       win_.draw(*title_);

    if (btnStart_)      btnStart_->draw(win_);
    if (btnDifficulty_) btnDifficulty_->draw(win_);
    if (btnExit_)       btnExit_->draw(win_);

    settingsBtn_.draw(win_);

    if (settingsOpen_) {
        if (musicLabel_) win_.draw(*musicLabel_);
        if (sfxLabel_)   win_.draw(*sfxLabel_);
        musicSlider_.draw(win_);
        sfxSlider_.draw(win_);
    }
}
