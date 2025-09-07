#pragma once
#include <optional>
#include <string>
#include <SFML/Graphics.hpp>

struct MenuChoice {
    bool start{false};
    bool exit{false};
    std::string difficulty{"Normal"};
    std::string username;
};

class Menu {
public:
    explicit Menu(sf::RenderWindow& win);
    std::optional<MenuChoice> tick(); // gère input & draw
private:
    sf::RenderWindow& win_;
    std::string username_{"Player"};
    std::string difficulty_{"Normal"};
    int cursor_{0}; // 0 Start, 1 Difficulty, 2 Username, 3 Exit
};
