#pragma once
#include <memory>
#include <SFML/Graphics.hpp>
#include <string>

class Menu;

class App {
public:
    App(int w=1280, int h=720, const std::string& title="Tower Defense");
    ~App(); // <- Ajout: destructeur non-inline, défini dans App.cpp
    void run();
private:
    sf::RenderWindow window_;
    enum class State { Menu, Playing, Exiting } state_{State::Menu};
    std::unique_ptr<Menu> menu_;
    void processEvents();
    void update(float dt);
    void render();
};
