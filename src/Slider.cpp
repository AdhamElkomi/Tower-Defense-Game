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
    // Rail (plus épais)
    const float trackH = 10.f; // ← était 6.f
    sf::FloatRect rail{{pos_.x, pos_.y}, {width_, trackH}};
    drawDropShadowMini(rt, rail, sf::Color(0,0,0,120), {3.f,5.f});

    sf::RectangleShape track(rail.size);
    track.setPosition(rail.position);
    track.setFillColor(sf::Color(220, 222, 230));
    rt.draw(track);

    // Curseur (plus large/haut)
    float t = (value_ - min_) / (max_ - min_);
    float cx = pos_.x + t * width_;
    const sf::Vector2f knobSize{22.f, 28.f}; // ← plus grand
    sf::FloatRect knob{{cx - knobSize.x * 0.5f, pos_.y - (knobSize.y - trackH) * 0.5f}, knobSize};
    drawDropShadowMini(rt, knob, sf::Color(0,0,0,140), {2.f,4.f});

    sf::RectangleShape handle(knob.size);
    handle.setPosition(knob.position);
    handle.setFillColor(sf::Color(255, 255, 255));
    handle.setOutlineThickness(2.f);
    handle.setOutlineColor(sf::Color(30, 144, 255)); // bleu “pro”
    rt.draw(handle);
}
