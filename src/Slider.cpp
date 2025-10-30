#include "Slider.hpp"
#include <algorithm>

// ✅ Constructeur qui correspond à {min, max, value, width}
Slider::Slider(float min, float max, float value, float width)
: min_(min), max_(max), value_(std::clamp(value, min, max)), width_(width) {}

void Slider::drawDropShadowMini(sf::RenderTarget& rt, const sf::FloatRect& rect,
                                const sf::Color& color, sf::Vector2f offset) const {
    // SFML 3 : FloatRect -> position / size
    sf::RectangleShape shadow(rect.size);
    shadow.setPosition(rect.position + offset);
    shadow.setFillColor(color);
    rt.draw(shadow);
}

void Slider::handleInput(const sf::RenderWindow& win,
                         bool mousePressedLeft,
                         bool mouseReleasedLeft,
                         bool mouseMoved)
{
    auto mouse = sf::Mouse::getPosition(win);

    // Zone interactive (rail + marge)
    sf::FloatRect area{{pos_.x - 10.f, pos_.y - 10.f}, {width_ + 20.f, 26.f}};

    if (mousePressedLeft) {
        dragging_ = area.contains(sf::Vector2f(static_cast<float>(mouse.x),
                                               static_cast<float>(mouse.y)));
    } else if (mouseReleasedLeft) {
        dragging_ = false;
    } else if (mouseMoved && dragging_) {
        // Convertit la position souris -> valeur [min_, max_]
        float x = std::clamp(static_cast<float>(mouse.x), pos_.x, pos_.x + width_);
        float t = (x - pos_.x) / width_;     // [0,1]
        value_ = min_ + t * (max_ - min_);
    }
}

void Slider::draw(sf::RenderTarget& rt) const {
    // Rail (plus épais et moderne)
    const float trackH = 12.f;
    sf::FloatRect rail{{pos_.x, pos_.y}, {width_, trackH}};

    sf::RectangleShape track(rail.size);
    track.setPosition(rail.position);
    track.setFillColor(sf::Color(60, 63, 75)); // gris foncé professionnel
    track.setOutlineThickness(1.f);
    track.setOutlineColor(sf::Color(100, 100, 100));
    rt.draw(track);

    // Curseur (plus large/haut, design moderne)
    float t = (value_ - min_) / (max_ - min_);
    float cx = pos_.x + t * width_;
    const sf::Vector2f knobSize{24.f, 32.f};
    sf::FloatRect knob{{cx - knobSize.x * 0.5f, pos_.y - (knobSize.y - trackH) * 0.5f}, knobSize};

    sf::RectangleShape handle(knob.size);
    handle.setPosition(knob.position);
    handle.setFillColor(sf::Color(255, 255, 255));
    handle.setOutlineThickness(3.f);
    handle.setOutlineColor(sf::Color(30, 144, 255)); // bleu professionnel
    rt.draw(handle);

    // Ajouter un petit cercle intérieur pour plus de style
    sf::CircleShape inner(4.f);
    inner.setPosition(sf::Vector2f(cx - 4.f, pos_.y + trackH * 0.5f - 4.f));
    inner.setFillColor(sf::Color(30, 144, 255));
    rt.draw(inner);
}
