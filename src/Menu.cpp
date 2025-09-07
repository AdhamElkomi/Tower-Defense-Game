#include "Menu.hpp"
#include <SFML/Graphics.hpp>
#include <optional>

Menu::Menu(sf::RenderWindow& win) : win_(win) {}

std::optional<MenuChoice> Menu::tick() {
    MenuChoice choice;

    // Gestion d'événements (SFML 3)
    while (auto ev = win_.pollEvent()) {
        if (ev->is<sf::Event::Closed>()) {
            choice.exit = true;
            return choice;
        }
        if (auto* kp = ev->getIf<sf::Event::KeyPressed>()) {
            using Key = sf::Keyboard::Key;
            if (kp->code == Key::Enter) {
                choice.start = true;
                choice.difficulty = difficulty_;
                choice.username = username_;
                return choice;
            }
            // TODO: gérer Up/Down pour naviguer, et la saisie du username
        }
    }

    // Rendu minimal (placeholder)
    win_.clear(sf::Color(15,15,22));
    // TODO: dessiner le menu (Start / Difficulty / Username / Exit)
    win_.display();

    return std::nullopt;
}
