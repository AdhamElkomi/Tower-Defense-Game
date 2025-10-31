#include "App.hpp"
#include "Menu.hpp"  
#include "GameScene.hpp"  // ⬅️ OBLIGATOIRE pour make_unique<GameScene>
#include <variant>  // ✅ important: type complet ici
#include <type_traits>
App::~App() = default;

App::App() {
      sf::VideoMode mode({1920u, 1080u});
    window_.create(mode, "Tower Defense");

    menu_ = std::make_unique<MenuScene>(window_);
}

// ✅ le type MenuScene est connu ici


void App::goToGame(){
    std::string difficulty = menu_->getDifficulty();
    game_ = std::make_unique<GameScene>(window_, difficulty);
    state_ = State::Game;
    // Stop menu music and start game music
    // Note: AudioManager is in MenuScene, so we need to access it differently
    // For now, we'll handle music in GameScene itself
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

            // Si ton bouton Start met started_ à true :
            if (menu_->started()) {
                goToGame();
            }
        } else { // State::Game
            game_->handleInput(mouseLeft_, mouseLeftReleased_, mouseMoved_);
            game_->update(dt);
            game_->draw();

            // Check if game over and return to menu requested
            if (game_->shouldReturnToMenu()) {
                state_ = State::Menu;
                game_.reset(); // Reset game scene
                menu_ = std::make_unique<MenuScene>(window_);
                // Note: AudioManager is in MenuScene, so music will be handled there
            }
        }

        window_.display();
    }
}
