#pragma once
#include <memory>
#include <string>

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

class Menu;

class App {
public:
    // w/h sont ignorés en plein écran mais gardés pour compat.
    App(int w = 1920, int h = 1080, const std::string& title = "Tower Defense");
    ~App();

    void run();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

private:
    // Fenêtre plein écran, non redimensionnable
    sf::RenderWindow window_;

    enum class State { Menu, Playing, Exiting };
    State state_{State::Menu};

    std::unique_ptr<Menu> menu_;

    // Boucles de jeu
    void processEvents();
    void update(float dt);
    void render();

    // --- AUDIO (boucle continue tant que le jeu est ouvert)
    sf::Music musicMenu_;
    sf::Music musicGame_;
    void startMenuMusic();
    void startGameMusic();
};
