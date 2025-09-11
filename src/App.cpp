#include "App.hpp"
#include "Menu.hpp"

#include <iostream>
#include <cmath> // (facultatif)

App::App(int /*w*/, int /*h*/, const std::string& title)
:  window_(sf::VideoMode::getDesktopMode(), title, sf::State::Fullscreen) {
    // VSync pour éviter le tearing
    window_.setVerticalSyncEnabled(true);

    // --- Audio (chemins relatifs depuis build/ grâce au symlink CMake)
    const char* menuPath = "assets/sounds/menu_theme.ogg";
    const char* gamePath = "assets/sounds/game_theme.ogg";

    if (!musicMenu_.openFromFile(menuPath)) {
        std::cerr << "[Audio] Failed to open " << menuPath << "\n";
    }
    if (!musicGame_.openFromFile(gamePath)) {
        std::cerr << "[Audio] Failed to open " << gamePath << "\n";
    }

    musicMenu_.setLooping(true);
    musicGame_.setLooping(true);

    // Menu UI
    menu_ = std::make_unique<Menu>(window_);

    // Musique du menu au démarrage
    startMenuMusic();
}

App::~App() = default;

// --- Musiques
void App::startMenuMusic() {
    musicGame_.stop();
    // setVolume attend 0..100 (float)
    musicMenu_.setVolume(menu_->musicVolume01() * 100.f);
    musicMenu_.play();
}

void App::startGameMusic() {
    musicMenu_.stop();
    musicGame_.setVolume(menu_->musicVolume01() * 100.f);
    musicGame_.play();
}

void App::run() {
    sf::Clock clk;
    while (window_.isOpen()) {
        clk.restart();

        // (Le Menu gère ses propres événements dans Menu::tick())
        // On garde tout de même la fermeture "hard" si jamais
        while (auto ev = window_.pollEvent()) {
            if (ev->is<sf::Event::Closed>()) {
                window_.close();
            }
            // Tu peux aussi capter Escape si tu veux forcer un retour menu/fermeture:
            // if (const auto* k = ev->getIf<sf::Event::KeyPressed>()) {
            //     if (k->scancode == sf::Keyboard::Scan::Escape) window_.close();
            // }
        }

        if (state_ == State::Menu) {
            auto choice = menu_->tick();

            // Suivre le slider "music" en temps réel
            musicMenu_.setVolume(menu_->musicVolume01() * 100.f);

            if (choice) {
                if (choice->exit) {
                    window_.close();
                } else if (choice->openDifficulty) {
                    // TODO: afficher l’overlay difficulté si besoin
                } else if (choice->start) {
                    state_ = State::Playing;
                    startGameMusic();
                }
            }
        } else if (state_ == State::Playing) {
            // TODO: boucle de jeu, rendu, etc.
            // Maintenir le volume sync avec le slider
            musicGame_.setVolume(menu_->musicVolume01() * 100.f);
        }

        // Le Menu dessine déjà; ici on ne fait que présenter la frame
        window_.display();
    }
}

void App::processEvents() {
    // L'essentiel est géré dans run(); cette fonction est là si tu veux séparer.
    while (auto ev = window_.pollEvent()) {
        if (ev->is<sf::Event::Closed>())
            window_.close();
    }
}

void App::update(float /*dt*/) {
    // Si tu veux une boucle "classique", tu peux appeler menu_->tick() ici
    // et déplacer la logique correspondante de run() vers update().
}

void App::render() {
    // Si tu décides de centraliser le rendu ici, clear/draw/display.
    // Actuellement, Menu::tick() fait le draw et run() fait display().
}
