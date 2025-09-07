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
    sf::Clock clock;
    while (window_.isOpen()) {
        processEvents();
        float dt = clock.restart().asSeconds();
        update(dt);
        render();
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
    // Pour l’instant: tout se passe dans Menu::tick() côté rendu
}

void App::render() {
    window_.clear(sf::Color(25,25,35));
    // Le draw est géré côté Menu::tick() pour l’instant
    window_.display();
}
