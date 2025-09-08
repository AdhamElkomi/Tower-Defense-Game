#include "App.hpp"
#include "Menu.hpp"

// SFML 3: VideoMode prend un Vector2u
App::App(int w, int h, const std::string& title)
: window_(sf::VideoMode({static_cast<unsigned>(w), static_cast<unsigned>(h)}), title) {
    window_.setFramerateLimit(60);
    menu_ = std::make_unique<Menu>(window_);
}

// Définition du destructeur ici (Menu est un type complet)
App::~App() = default;

void App::run() {
    while (window_.isOpen()) {
        // UPDATE
        if (state_ == State::Menu) {
            if (auto choice = menu_->tick()) {
                if (choice->exit) window_.close();
                else if (choice->openDifficulty) { /* show overlay */ }
                else if (choice->start) { state_ = State::Playing; }
            }
        } else {
            // update game...
        }

        // RENDER
        window_.clear(sf::Color(10,12,18));
        if (state_ == State::Menu) {
            menu_->render(); // pas de display() ici
        } else {
            // draw game...
        }
        window_.display();
    }
}



void App::processEvents() {
    // SFML 3: pollEvent() -> std::optional<sf::Event>
    while (auto ev = window_.pollEvent()) {
        // Fermer la fenêtre si événement Closed
        if (ev->is<sf::Event::Closed>())
            window_.close();
        // (Laisse Menu gérer le reste dans son tick())
    }
}

void App::update(float) {
    if (state_ == State::Menu) {
        auto result = menu_->tick();
        if (result.has_value()) {
            if (result->exit) {
                window_.close();
            } else if (result->openDifficulty) {
                // TODO: open a simple overlay or cycle difficulty, then reflect on Menu
                // e.g., menu_->setDifficultySubtitle("Hard");
            } else if (result->start) {
                // TODO: transition to game state (generate map, etc.)
                state_ = State::Playing;
            }
        }
    } else if (state_ == State::Playing) {
        // TODO: your game loop (render world, handle input, etc.)
        // Keep Style::Default so window can be minimized/maximized by the OS.
    }
}

void App::render() {
    window_.clear(sf::Color(25,25,35));
    // Le draw est géré côté Menu::tick() pour l’instant
    window_.display();
}
