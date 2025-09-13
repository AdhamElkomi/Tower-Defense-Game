#pragma once
#include <SFML/Graphics.hpp>
#include <optional>

class IScene {
public:
    virtual ~IScene() = default;

    // 1) Entrée utilisateur : on passe l'event si dispo (pollEvent() de SFML 3 est optional)
    virtual void handleEvent(const sf::Event& e, const sf::RenderWindow& win) = 0;

    // 2) Tick logique
    virtual void update(float dt) = 0;

    // 3) Rendu
    virtual void draw(sf::RenderTarget& rt) = 0;
};
