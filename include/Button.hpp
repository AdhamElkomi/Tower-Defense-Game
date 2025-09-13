#pragma once
#include <SFML/Graphics.hpp>
#include <functional>


class Button {
public:
Button() = delete;  // ✅ empêche l'usage sans font/label
Button(const sf::Font& font, const sf::Vector2f& size, const std::string& label,
sf::Color fill, sf::Color border, float borderThickness = 3.f);


void setPosition(const sf::Vector2f& p);
void setHoverTint(const sf::Color& c);
void setOnClick(std::function<void()> cb);
 void handleInput(const sf::RenderWindow& win, bool mousePressedLeft, std::function<void()> onPressedSfx);



void handleEvent(const sf::Event& e, const sf::RenderWindow& win, std::function<void()> onPressedSfx);
void draw(sf::RenderTarget& rt) const;


sf::FloatRect bounds() const;


private:
sf::RectangleShape shape_;
sf::Text text_; // construit dans le ctor avec (font, string, size)
bool hovered_{false};
sf::Color hoverTint_{255,255,255,30};
std::function<void()> onClick_;
};