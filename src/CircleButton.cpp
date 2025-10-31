#include "CircleButton.hpp"
#include <cmath> // std::sqrt

// Ombre portée rectangulaire simplifiée pour le bouton circulaire
static void drawDropShadowCircle(sf::RenderTarget& rt, const sf::FloatRect& rect, const sf::Color& color, sf::Vector2f offset = {5.f, 7.f}) {
    sf::RectangleShape shadow(sf::Vector2f(rect.width, rect.height));             // SFML 2
    shadow.setPosition(sf::Vector2f(rect.left, rect.top) + offset);       // SFML 2
    shadow.setFillColor(color);
    rt.draw(shadow);
}

CircleButton::CircleButton(float radius, const sf::Texture* iconTex) {
    circle_.setRadius(radius);
    circle_.setFillColor(sf::Color(40, 43, 55));
    circle_.setOutlineThickness(3.f);
    circle_.setOutlineColor(sf::Color::White);

    if (iconTex) {
        // ✅ SFML 3 : construire le sprite avec la texture (pas de ctor par défaut)
        icon_.emplace(*iconTex);
        float s = (radius * 1.6f) / static_cast<float>(iconTex->getSize().x);
        icon_->setScale(sf::Vector2f(s, s));          // ✅ Vector2f
    }
}

void CircleButton::setPosition(const sf::Vector2f& p) {
    circle_.setPosition(p);

    if (icon_.has_value()) {
        const auto c  = circle_.getGlobalBounds();    // FloatRect {left, top, width, height}
        const auto ib = icon_->getLocalBounds();      // FloatRect {left, top, width, height}
        const auto sc = icon_->getScale();            // Vector2f

        icon_->setPosition(sf::Vector2f(
            c.left + (c.width - ib.width * sc.x) / 2.f,
            c.top + (c.height - ib.height * sc.y) / 2.f
        ));
    }
}

void CircleButton::setOnClick(std::function<void()> cb) { onClick_ = std::move(cb); }

void CircleButton::setIcon(const sf::Texture* tex) {
    if (tex) {
        icon_.emplace(*tex);
        float s = (circle_.getRadius() * 1.6f) / static_cast<float>(tex->getSize().x);
        icon_->setScale(sf::Vector2f(s, s));
        // Repositionner l'icône
        const auto c  = circle_.getGlobalBounds();
        const auto ib = icon_->getLocalBounds();
        const auto sc = icon_->getScale();
        icon_->setPosition(sf::Vector2f(
            c.left + (c.width - ib.width * sc.x) / 2.f,
            c.top + (c.height - ib.height * sc.y) / 2.f
        ));
    } else {
        icon_.reset();
    }
}

void CircleButton::setDrawShadow(bool draw) { drawShadow_ = draw; }

void CircleButton::handleInput(const sf::RenderWindow& win, bool mousePressedLeft, std::function<void()> onPressedSfx) {
    // Survol : test distance au centre
    const auto mouse = sf::Mouse::getPosition(win);
    const auto c     = circle_.getPosition();
    const float r    = circle_.getRadius();

    const sf::Vector2f center{ c.x + r, c.y + r };
    const float dx = static_cast<float>(mouse.x) - center.x;
    const float dy = static_cast<float>(mouse.y) - center.y;

    hovered_ = std::sqrt(dx * dx + dy * dy) <= r;

    if (mousePressedLeft && hovered_) {
        if (onPressedSfx) onPressedSfx();
        if (onClick_)     onClick_();
    }
}

void CircleButton::draw(sf::RenderTarget& rt) const {
    rt.draw(circle_);

    if (hovered_) {
        sf::CircleShape tint(circle_.getRadius());
        tint.setPosition(circle_.getPosition());
        tint.setFillColor(sf::Color(255, 255, 255, 28));
        rt.draw(tint);
    }

    if (icon_.has_value())   // ✅ SFML 3 : tester la présence
        rt.draw(*icon_);
}
