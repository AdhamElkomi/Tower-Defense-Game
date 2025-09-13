#pragma once
#include <SFML/Graphics.hpp>
#include <memory>

class MenuScene;
class GameScene;

class App {
public:
    App();
    ~App();
    void run();
    void goToGame();

private:
    enum class State { Menu, Game };

    sf::RenderWindow window_;

    std::unique_ptr<MenuScene> menu_;
    std::unique_ptr<GameScene> game_;   // ← manquait

    State state_{State::Menu};          // ← manquait

    bool mouseLeft_{false};
    bool mouseLeftReleased_{false};
    bool mouseMoved_{false};
};
