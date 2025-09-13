#include "Button.hpp"
#include <SFML/Graphics.hpp>
#include <utility> // pour std::move

// Fonction utilitaire pour l’ombre portée
static void drawDropShadowRect(sf::RenderTarget& rt, const sf::FloatRect& rect, const sf::Color& color, sf::Vector2f offset = {4.f, 6.f}) {
    sf::RectangleShape shadow(rect.size);
    shadow.setPosition(rect.position + offset);          // ✅ SFML 3 : position au lieu de left/top
    shadow.setFillColor(color);
    rt.draw(shadow);
}

// Constructeur du bouton

Button::Button(const sf::Font& font,
               const sf::Vector2f& size,
               const std::string& label,
               sf::Color fill, sf::Color border, float borderThickness)
: text_(font, label, 22) // ✅ construit ici
{
    shape_.setSize(size);
    shape_.setFillColor(fill);
    shape_.setOutlineColor(border);
    shape_.setOutlineThickness(borderThickness);
    text_.setFillColor(sf::Color::White);
}

// Positionne le bouton et centre le texte
void Button::setPosition(const sf::Vector2f& p) {
     shape_.setPosition(p);
     const auto b  = shape_.getGlobalBounds();
    const auto tb = text_.getLocalBounds();
    text_.setPosition(sf::Vector2f(
        b.position.x + (b.size.x - tb.size.x)/2.f - tb.position.x,
        b.position.y + (b.size.y - tb.size.y)/2.f - tb.position.y - 4.f
    ));
}

void Button::setHoverTint(const sf::Color& c) { hoverTint_ = c; }
void Button::setOnClick(std::function<void()> cb) { onClick_ = std::move(cb); }

void Button::handleInput(const sf::RenderWindow& win, bool mousePressedLeft, std::function<void()> onPressedSfx) {
    const auto mouse = sf::Mouse::getPosition(win);
    hovered_ = shape_.getGlobalBounds().contains(sf::Vector2f((float)mouse.x, (float)mouse.y));
    if (mousePressedLeft && hovered_) {
        if (onPressedSfx) onPressedSfx();
        if (onClick_)     onClick_();

    }
}

// Dessine le bouton avec son effet hover + ombre
void Button::draw(sf::RenderTarget& rt) const {
    drawDropShadowRect(rt, shape_.getGlobalBounds(), sf::Color(0, 0, 0, 110));
    rt.draw(shape_);

    if (hovered_) {
        sf::RectangleShape hover(shape_.getSize());
        hover.setPosition(shape_.getPosition());
        hover.setFillColor(hoverTint_);
        rt.draw(hover);
    }

    rt.draw(text_);
}

// Retourne les bounds (utile pour du layout)
sf::FloatRect Button::bounds() const { 
    return shape_.getGlobalBounds(); 
}
