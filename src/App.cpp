#include "App.hpp"
#include "Menu.hpp"
#include "GameScene.hpp"
#include "LeaderboardScene.hpp"
#include <variant>
#include <type_traits>

App::~App() = default;

App::App() {
    sf::VideoMode mode({1920u, 1080u});
    window_.create(mode, "Tower Defense");

    menu_ = std::make_unique<MenuScene>(window_);
}

void App::goToGame() {
    game_ = std::make_unique<GameScene>(window_, currentDifficulty_, currentUsername_);
    state_ = State::Game;
}

void App::goToLeaderboard() {
    leaderboard_ = std::make_unique<LeaderboardScene>(window_);
    state_ = State::Leaderboard;
}

void App::goToUsernamePrompt() {
    state_ = State::UsernamePrompt;
}

void App::run() {
    sf::Clock clk;

    while (window_.isOpen()) {
        mouseMoved_ = false;
        mouseLeftReleased_ = false;

        sf::Event event;
        while (window_.pollEvent(event)) {
            // --- CLOSED
            if (event.type == sf::Event::Closed) {
                window_.close();
            }
            // --- TEXT ENTERED
            else if (event.type == sf::Event::TextEntered) {
                if (state_ == State::Menu && menu_) {
                    menu_->handleTextInput(event.text.unicode);
                } else if (state_ == State::UsernamePrompt && menu_) {
                    menu_->handleUsernameInput(event.text.unicode);
                }
            }
            // --- MOUSE MOVED
            else if (event.type == sf::Event::MouseMoved) {
                mouseMoved_ = true;
            }
            // --- MOUSE BUTTON PRESSED
            else if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left) mouseLeft_ = true;
            }
            // --- MOUSE BUTTON RELEASED
            else if (event.type == sf::Event::MouseButtonReleased) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    mouseLeft_ = false;
                    mouseLeftReleased_ = true;
                }
            }
        }

        const float dt = clk.restart().asSeconds();
        window_.clear(sf::Color(20,22,27));

        if (state_ == State::Menu) {
            menu_->handleInput(mouseLeft_, mouseLeftReleased_, mouseMoved_);
            menu_->update(dt);
            menu_->draw();

            if (menu_->shouldGoToUsernamePrompt()) {
                currentDifficulty_ = menu_->getDifficulty();
                goToUsernamePrompt();
            } else if (menu_->shouldGoToLeaderboard()) {
                goToLeaderboard();
            }
        } else if (state_ == State::UsernamePrompt) {
            menu_->handleUsernamePromptInput(mouseLeft_, mouseLeftReleased_, mouseMoved_);
            menu_->update(dt);
            menu_->drawUsernamePrompt();

            if (menu_->usernamePromptDone()) {
                currentUsername_ = menu_->getEnteredUsername();
                goToGame();
            } else if (menu_->usernamePromptCancelled()) {
                state_ = State::Menu;
            }
        } else if (state_ == State::Game) {
            game_->handleInput(mouseLeft_, mouseLeftReleased_, mouseMoved_);
            game_->update(dt);
            game_->draw();

            if (game_->shouldReturnToMenu()) {
                state_ = State::Menu;
                game_.reset();
                menu_ = std::make_unique<MenuScene>(window_);
            }
        } else if (state_ == State::Leaderboard) {
            leaderboard_->handleInput(mouseLeft_, mouseLeftReleased_, mouseMoved_);
            leaderboard_->update(dt);
            leaderboard_->draw();

            if (leaderboard_->shouldReturnToMenu()) {
                state_ = State::Menu;
                leaderboard_.reset();
                menu_ = std::make_unique<MenuScene>(window_);
            }
        }

        window_.display();
    }
}
