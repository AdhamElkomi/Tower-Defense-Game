#pragma once
#include <SFML/Graphics.hpp>
#include <memory>

class MenuScene;
class GameScene;
class LeaderboardScene;

class App {
public:
    App();
    ~App();
    void run();
    void goToGame();
    void goToLeaderboard();
    void goToUsernamePrompt();

private:
    enum class State { Menu, UsernamePrompt, Game, Leaderboard };

    sf::RenderWindow window_;

    std::unique_ptr<MenuScene> menu_;
    std::unique_ptr<GameScene> game_;
    std::unique_ptr<LeaderboardScene> leaderboard_;

    State state_{State::Menu};

    bool mouseLeft_{false};
    bool mouseLeftReleased_{false};
    bool mouseMoved_{false};

    // Username prompt state
    std::string currentUsername_;
    std::string currentDifficulty_;
};
