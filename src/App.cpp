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
    game_ = std::make_unique<GameScene>(window_);
    state_ = State::Game;
}


void App::run() {
    sf::Clock clk;

    while (window_.isOpen()) {
        mouseMoved_ = false;
        mouseLeftReleased_ = false;

        while (auto ev = window_.pollEvent()) {
            // --- CLOSED
            if (const auto* e = ev->getIf<sf::Event::Closed>()) {
                (void)e;
                window_.close();
            }
            // --- MOUSE MOVED
            else if (const auto* e = ev->getIf<sf::Event::MouseMoved>()) {
                (void)e;
                mouseMoved_ = true;
            }
            // --- MOUSE BUTTON PRESSED
            else if (const auto* e = ev->getIf<sf::Event::MouseButtonPressed>()) {
                if (e->button == sf::Mouse::Button::Left) mouseLeft_ = true;
            }
            // --- MOUSE BUTTON RELEASED
            else if (const auto* e = ev->getIf<sf::Event::MouseButtonReleased>()) {
                if (e->button == sf::Mouse::Button::Left) {
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
        }

        window_.display();
    }
}
