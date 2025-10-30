#pragma once
#include <SFML/Graphics.hpp>

class Slider {
public:
    // ✅ Constructeur attendu dans ton Menu.hpp
    Slider(float min = 0.f, float max = 1.f, float value = 0.5f, float width = 200.f);

    // Position du rail (coin haut-gauche)
    void setPosition(sf::Vector2f p) { pos_ = p; }
    void setWidth(float w) { width_ = w; }

    // Gestion input à partir des drapeaux de App::run()
    void handleInput(const sf::RenderWindow& win,
                     bool mousePressedLeft,
                     bool mouseReleasedLeft,
                     bool mouseMoved);

    // Rendu
    void draw(sf::RenderTarget& rt) const;

    // Valeur courante
    float value() const { return value_; }
    void setValue(float v) { value_ = std::clamp(v, min_, max_); }

    

private:
    // Helpers dessin
    void drawDropShadowMini(sf::RenderTarget& rt, const sf::FloatRect& rect,
                            const sf::Color& color, sf::Vector2f offset = {4.f, 6.f}) const;

    float min_{0.f}, max_{1.f}, value_{0.5f}, width_{200.f};
    sf::Vector2f pos_{0.f, 0.f};
    bool dragging_{false};
};
