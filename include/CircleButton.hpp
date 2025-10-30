// CircleButton.hpp
#pragma once
#include <SFML/Graphics.hpp>
#include <functional>
#include <optional>

class CircleButton {
public:
    CircleButton() = default;
    CircleButton(float radius, const sf::Texture* iconTex = nullptr);

    void setPosition(const sf::Vector2f& p);
    void setOnClick(std::function<void()> cb);
    void setIcon(const sf::Texture* tex);
    void setDrawShadow(bool draw);

    // 👉 SFML 3-friendly : pas de sf::Event ici
    void handleInput(const sf::RenderWindow& win, bool mousePressedLeft, std::function<void()> onPressedSfx);

    void draw(sf::RenderTarget& rt) const;

private:
    sf::CircleShape circle_;
    std::optional<sf::Sprite> icon_;   // ✅ plus sûr en SFML 3
    bool hovered_{false};
    bool drawShadow_{true};
    std::function<void()> onClick_;
};
