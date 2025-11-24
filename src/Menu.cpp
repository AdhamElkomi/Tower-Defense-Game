#include "Menu.hpp"
#include <algorithm>

// Couvre l'écran en conservant le ratio
static void centerSpriteToCover(sf::Sprite& sp, const sf::RenderWindow& win) {
    const auto* tex = sp.getTexture(); // ✅ pointer
    const float TW = static_cast<float>(tex->getSize().x);
    const float TH = static_cast<float>(tex->getSize().y);
    const float W  = static_cast<float>(win.getSize().x);
    const float H  = static_cast<float>(win.getSize().y);
    const float s  = std::max(W / TW, H / TH);
    sp.setScale(sf::Vector2f(s, s));
    sp.setPosition(sf::Vector2f(0.f, 0.f));
}

void MenuScene::update(float /*dt*/) {
    // Update username validation
    if (!enteredUsername_.empty()) {
        usernameValid_ = LeaderboardController::isValidUsername(enteredUsername_);
    } else {
        usernameValid_ = false;
    }
    if (usernameError_) {
        usernameError_->setString(usernameValid_ ? "" : "Invalid username \n \n(3-16 chars, A-Z a-z 0-9 _ -)");
    }
}

MenuScene::MenuScene(sf::RenderWindow& win) : win_(win) {
    // Chargement assets (tu peux vérifier les bool si tu veux logger)
    font_.loadFromFile("../assets/fonts/PressStart2P-Regular.ttf");
    bgTex_.loadFromFile("../assets/images/background.png");
    panelTex_.loadFromFile("../assets/images/first-bg.png");
    gearTex_.loadFromFile("../assets/images/gear.png");
    leaderboardIconTex_.loadFromFile("../assets/ui/leaderboard_icon.png");

    // Construire éléments dépendants des assets
    bgSprite_.emplace(bgTex_);
    panelSprite_.emplace(panelTex_);
    title_.emplace("TOWER DEFENSE", font_, 100);

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
        (win_.getSize().x - plb.width) * 0.5f + 160.f,
        (win_.getSize().y - plb.height) * 0.5f + 200.f
    ));

    // Titre
    title_->setFillColor(theme_.text);
    const auto tb = title_->getLocalBounds();
    title_->setPosition(sf::Vector2f(
        (float)win_.getSize().x * 0.5f - tb.width * 0.5f - tb.left,
        120.f - tb.top
    ));

    // Boutons
    sf::Vector2f btnSize{320.f, 72.f};
    btnStart_.emplace(font_, btnSize, "START",      sf::Color(30,144,255), sf::Color::White, 3.f);
    btnDifficulty_.emplace(font_, btnSize, "DIFFICULTY", sf::Color(76,175,80),  sf::Color::White, 3.f);
    btnExit_.emplace(font_, btnSize, "EXIT",       sf::Color(244,67,54),  sf::Color::White, 3.f);

    btnStart_->setHoverTint(theme_.btnHover);
    btnDifficulty_->setHoverTint(theme_.btnHover);
    btnExit_->setHoverTint(theme_.btnHover);

    // Obtenir les bounds globaux du panel après scaling
    sf::FloatRect panelGlobal = panelSprite_->getGlobalBounds();

    // Position des boutons sur le panel
    const float x = panelGlobal.left + 200.f;
    const float y = panelGlobal.top + 180.f;
    float gap     = 100.f; // espace vertical entre boutons

    btnStart_->setPosition(sf::Vector2f(x, y));
    btnDifficulty_->setPosition(sf::Vector2f(x, y + gap));
    btnExit_->setPosition(sf::Vector2f(x, y + 2*gap));

    // Labels sliders (optionnels, car sf::Text non default-constructible)
    musicLabel_.emplace("Music Volume", font_, 18);
    sfxLabel_.emplace  ("SFX Volume",   font_, 18);
    musicLabel_->setFillColor(theme_.text);
    sfxLabel_->setFillColor(theme_.text);

    // Valeurs en pourcentage (éditables)
    musicValueLabel_.emplace("70%", font_, 16);
    sfxValueLabel_.emplace  ("80%", font_, 16);
    musicValueLabel_->setFillColor(theme_.text);
    sfxValueLabel_->setFillColor(theme_.text);

    // Cadre pour les sliders
    settingsPanelBG_.setSize(sf::Vector2f(350.f, 220.f));
    settingsPanelBG_.setFillColor(sf::Color(40, 43, 55, 220)); // semi-transparent
    settingsPanelBG_.setOutlineThickness(3.f);
    settingsPanelBG_.setOutlineColor(sf::Color(100, 100, 100));

    // Position sliders et labels dans le cadre, relative au panel global
    const float sx = panelGlobal.left + panelGlobal.width + 50.f;
    const float sy = panelGlobal.top + 50.f;
    settingsPanelBG_.setPosition(sf::Vector2f(sx - 25.f, sy - 25.f));
    musicLabel_->setPosition(sf::Vector2f(sx, sy));
    musicSlider_.setPosition  ({sx, sy + 30.f});
    musicValueLabel_->setPosition(sf::Vector2f(sx + 280.f, sy + 30.f));
    sfxLabel_->setPosition   (sf::Vector2f(sx, sy + 78.f));
    sfxSlider_.setPosition    ({sx, sy + 108.f});
    sfxValueLabel_->setPosition(sf::Vector2f(sx + 280.f, sy + 108.f));

    // Audio
    audio_.load();
    audio_.playMenuLoop(0.7f);

    // Callbacks
    btnStart_->setOnClick([this]() {
        goToUsernamePrompt_ = true;
        audio_.playClick();
    });

    btnDifficulty_->setOnClick([this]() {
        difficultyMenuOpen_ = !difficultyMenuOpen_;
        audio_.playClick();
    });

    btnExit_->setOnClick([this]() {
        audio_.playClick();
        win_.close();
    });

    // Settings bouton (à droite du panel)
    settingsBtn_.setPosition(sf::Vector2f(
        panelGlobal.left + panelGlobal.width + 20.f,
        panelGlobal.top + panelGlobal.height * 0.5f  // centré verticalement
    ));
    settingsBtn_.setIcon(&gearTex_);
    settingsBtn_.setOnClick([this]() {
        settingsOpen_ = !settingsOpen_;
        audio_.playClick();
    });
    settingsBtn_.setDrawShadow(false);

    // Leaderboard button
    leaderboardBtn_.setPosition(sf::Vector2f(
        panelGlobal.left + panelGlobal.width + 20.f,
        panelGlobal.top + panelGlobal.height * 0.64f
    ));
    leaderboardBtn_.setIcon(&leaderboardIconTex_);
    // leaderboardBtn_.setFillColor(sf::Color(110, 0, 26)); // Bordeaux - removed as CircleButton doesn't have setFillColor
    leaderboardBtn_.setOnClick([this]() {
        goToLeaderboard_ = true;
        audio_.playClick();
    });
    leaderboardBtn_.setDrawShadow(false);

    // Difficulty submenu
    difficultyPanelBG_.setSize(sf::Vector2f(300.f, 400.f));
    difficultyPanelBG_.setFillColor(sf::Color(40, 43, 55, 220));
    difficultyPanelBG_.setOutlineThickness(3.f);
    difficultyPanelBG_.setOutlineColor(sf::Color(100, 100, 100));
    difficultyPanelBG_.setPosition(sf::Vector2f(panelGlobal.left - 320.f, panelGlobal.top));

    difficultyTitle_.emplace("Select", font_, 30);
    difficultyTitle_->setFillColor(theme_.text);
    difficultyTitle_->setPosition(sf::Vector2f(panelGlobal.left - 260.f, panelGlobal.top + 20.f));

    std::vector<std::string> difficulties = {"Easy", "Normal", "Hard", "Legendary"};
    difficultyButtons_.resize(difficulties.size());
    for (size_t i = 0; i < difficulties.size(); ++i) {
        difficultyButtons_[i].emplace(font_, sf::Vector2f(250.f, 60.f), difficulties[i], sf::Color(76,175,80), sf::Color::White, 3.f);
        difficultyButtons_[i]->setHoverTint(theme_.btnHover);
        difficultyButtons_[i]->setPosition(sf::Vector2f(panelGlobal.left - 300.f, panelGlobal.top + 80.f + i * 80.f));
        difficultyButtons_[i]->setOnClick([this, diff = difficulties[i]]() {
            difficulty_ = diff;
            difficultyMenuOpen_ = false;
            audio_.playClick();
            goToUsernamePrompt_ = true;
        });
    }

    // Username prompt
    usernamePanelBG_.setSize(sf::Vector2f(400.f, 250.f));
    usernamePanelBG_.setFillColor(sf::Color(40, 43, 55, 240));
    usernamePanelBG_.setOutlineThickness(3.f);
    usernamePanelBG_.setOutlineColor(sf::Color(100, 100, 100));
    usernamePanelBG_.setPosition(sf::Vector2f(
        (win_.getSize().x - 400.f) * 0.5f,
        (win_.getSize().y - 250.f) * 0.5f
    ));

    usernameTitle_.emplace("Enter Username", font_, 24);
    usernameTitle_->setFillColor(theme_.text);
    usernameTitle_->setPosition(sf::Vector2f(
        usernamePanelBG_.getPosition().x + 200.f - usernameTitle_->getLocalBounds().width / 2.f,
        usernamePanelBG_.getPosition().y + 20.f
    ));

    usernameInput_.emplace("", font_, 20);
    usernameInput_->setFillColor(sf::Color::Cyan);
    usernameInput_->setPosition(sf::Vector2f(
        usernamePanelBG_.getPosition().x + 50.f,
        usernamePanelBG_.getPosition().y + 80.f
    ));

    usernameError_.emplace("", font_, 10);
    usernameError_->setFillColor(sf::Color::Red);
    usernameError_->setPosition(sf::Vector2f(
        usernamePanelBG_.getPosition().x + 50.f,
        usernamePanelBG_.getPosition().y + 120.f
    ));

    usernameOkBtn_.emplace(font_, sf::Vector2f(80.f, 40.f), "OK", sf::Color(76,175,80), sf::Color::White, 2.f);
    usernameOkBtn_->setPosition(sf::Vector2f(
        usernamePanelBG_.getPosition().x + 150.f,
        usernamePanelBG_.getPosition().y + 180.f
    ));

    usernameBackBtn_.emplace(font_, sf::Vector2f(80.f, 40.f), "Back", sf::Color(244,67,54), sf::Color::White, 2.f);
    usernameBackBtn_->setPosition(sf::Vector2f(
        usernamePanelBG_.getPosition().x + 50.f,
        usernamePanelBG_.getPosition().y + 180.f
    ));

    usernameOkBtn_->setOnClick([this]() {
        if (usernameValid_) {
            usernamePromptDone_ = true;
            audio_.playClick();
        }
    });

    usernameBackBtn_->setOnClick([this]() {
        usernamePromptCancelled_ = true;
        audio_.playClick();
    });
}


void MenuScene::handleTextInput(char32_t unicode) {
    if (!settingsOpen_) return;

    if (editingMusic_) {
        if (unicode == '\b' && !musicInput_.empty()) {
            musicInput_.pop_back();
        } else if (unicode >= '0' && unicode <= '9' && musicInput_.size() < 3) {
            musicInput_ += static_cast<char>(unicode);
        }
        musicValueLabel_->setString(musicInput_ + (musicInput_.empty() ? "" : "%"));
    } else if (editingSfx_) {
        if (unicode == '\b' && !sfxInput_.empty()) {
            sfxInput_.pop_back();
        } else if (unicode >= '0' && unicode <= '9' && sfxInput_.size() < 3) {
            sfxInput_ += static_cast<char>(unicode);
        }
        sfxValueLabel_->setString(sfxInput_ + (sfxInput_.empty() ? "" : "%"));
    }
}

void MenuScene::handleUsernameInput(char32_t unicode) {
    if (unicode == '\b' && !enteredUsername_.empty()) {
        enteredUsername_.pop_back();
    } else if (unicode >= 32 && unicode < 127 && enteredUsername_.size() < 16) {
        enteredUsername_ += static_cast<char>(unicode);
    }
    if (usernameInput_) {
        usernameInput_->setString(enteredUsername_);
    }
}

void MenuScene::handleUsernamePromptInput(bool mpLeft, bool /*mrLeft*/, bool /*mMoved*/) {
    auto clickSfx = [this](){ audio_.playClick(); };

    if (usernameOkBtn_) usernameOkBtn_->handleInput(win_, mpLeft, clickSfx);
    if (usernameBackBtn_) usernameBackBtn_->handleInput(win_, mpLeft, clickSfx);
}

void MenuScene::drawUsernamePrompt() {
    win_.draw(usernamePanelBG_);
    if (usernameTitle_) win_.draw(*usernameTitle_);
    if (usernameInput_) win_.draw(*usernameInput_);
    if (usernameError_) win_.draw(*usernameError_);
    if (usernameOkBtn_) usernameOkBtn_->draw(win_);
    if (usernameBackBtn_) usernameBackBtn_->draw(win_);
}

void MenuScene::handleInput(bool mpLeft, bool mrLeft, bool mMoved) {
    auto clickSfx = [this](){ audio_.playClick(); };

    if (btnStart_)      btnStart_->handleInput(win_, mpLeft, clickSfx);
    if (btnDifficulty_) btnDifficulty_->handleInput(win_, mpLeft, clickSfx);
    if (btnExit_)       btnExit_->handleInput(win_, mpLeft, clickSfx);
    settingsBtn_.handleInput(win_, mpLeft, clickSfx);
    leaderboardBtn_.handleInput(win_, mpLeft, clickSfx);

    // Handle difficulty submenu input
    if (difficultyMenuOpen_) {
        for (auto& btn : difficultyButtons_) {
            if (btn) btn->handleInput(win_, mpLeft, clickSfx);
        }
    }

    if (settingsOpen_) {
        // Gestion des clics sur les labels de valeur pour édition (avec zone élargie)
        if (mpLeft && musicValueLabel_) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(win_);
            sf::FloatRect musicBounds = musicValueLabel_->getGlobalBounds();
            musicBounds.left -= 30.f;
            musicBounds.top -= 30.f;
            musicBounds.width += 30.f;
            musicBounds.height += 30.f;
            if (musicBounds.contains(sf::Vector2f(mousePos))) {
                editingMusic_ = true;
                editingSfx_ = false;
                musicInput_ = "";
                musicValueLabel_->setFillColor(sf::Color::Yellow);
                return;
            }
        }
        if (mpLeft && sfxValueLabel_) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(win_);
            sf::FloatRect sfxBounds = sfxValueLabel_->getGlobalBounds();
            sfxBounds.left -= 10.f;
            sfxBounds.top -= 10.f;
            sfxBounds.width += 20.f;
            sfxBounds.height += 20.f;
            if (sfxBounds.contains(sf::Vector2f(mousePos))) {
                editingSfx_ = true;
                editingMusic_ = false;
                sfxInput_ = "";
                sfxValueLabel_->setFillColor(sf::Color::Yellow);
                return;
            }
        }

        // Si on clique ailleurs, arrêter l'édition
        if (mpLeft && (editingMusic_ || editingSfx_)) {
            if (editingMusic_ && !musicInput_.empty()) {
                try {
                    int percent = std::stoi(musicInput_);
                    percent = std::clamp(percent, 0, 100);
                    float value = percent / 100.f;
                    musicSlider_.setValue(value);
                    audio_.setMusicVolume(value);
                    musicValueLabel_->setString(std::to_string(percent) + "%");
                } catch (...) {
                    // Valeur invalide, garder l'ancienne
                }
            }
            if (editingSfx_ && !sfxInput_.empty()) {
                try {
                    int percent = std::stoi(sfxInput_);
                    percent = std::clamp(percent, 0, 100);
                    float value = percent / 100.f;
                    sfxSlider_.setValue(value);
                    audio_.setSfxVolume(value);
                    sfxValueLabel_->setString(std::to_string(percent) + "%");
                } catch (...) {
                    // Valeur invalide, garder l'ancienne
                }
            }
            editingMusic_ = false;
            editingSfx_ = false;
            musicValueLabel_->setFillColor(theme_.text);
            sfxValueLabel_->setFillColor(theme_.text);
            return;
        }

        musicSlider_.handleInput(win_, mpLeft, mrLeft, mMoved);
        sfxSlider_.handleInput(win_, mpLeft, mrLeft, mMoved);
        audio_.setMusicVolume(musicSlider_.value());
        audio_.setSfxVolume(sfxSlider_.value());

        // Mettre à jour les labels de valeur
        int musicPercent = static_cast<int>(musicSlider_.value() * 100.f);
        int sfxPercent = static_cast<int>(sfxSlider_.value() * 100.f);
        if (musicValueLabel_ && !editingMusic_) musicValueLabel_->setString(std::to_string(musicPercent) + "%");
        if (sfxValueLabel_ && !editingSfx_)   sfxValueLabel_->setString(std::to_string(sfxPercent) + "%");
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
    leaderboardBtn_.draw(win_);

    if (settingsOpen_) {
        win_.draw(settingsPanelBG_);
        if (musicLabel_) win_.draw(*musicLabel_);
        if (sfxLabel_)   win_.draw(*sfxLabel_);
        if (musicValueLabel_) win_.draw(*musicValueLabel_);
        if (sfxValueLabel_)   win_.draw(*sfxValueLabel_);
        musicSlider_.draw(win_);
        sfxSlider_.draw(win_);
    }

    if (difficultyMenuOpen_) {
        win_.draw(difficultyPanelBG_);
        if (difficultyTitle_) win_.draw(*difficultyTitle_);
        for (auto& btn : difficultyButtons_) {
            if (btn) btn->draw(win_);
        }
    }
}
