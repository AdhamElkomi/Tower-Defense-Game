#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <array>
#include <vector>
#include <string>
#include <cstdint>
#include "Map.hpp"

class BuildMenu {
public:
    enum class Material { Wood=0, Stone=1, Crystal=2, Count=3 };
    enum class Unit     { Cannon=0, Archer=1, Mage=2, Count=3 };

    BuildMenu(sf::RenderWindow& win, float tileSize, const Map& map,
              const std::string& fontPath = "../assets/ui/Inter-SemiBold.ttf",
              const std::string& bgPath   = "../assets/ui/menu_bg.png");

    // état / intégration
    void toggle() { visible_ = !visible_; }
    bool visible() const { return visible_; }

    // events (à appeler depuis GameScene)
    void onMousePressed(sf::Vector2f mouse);
    void onMouseReleased(sf::Vector2f mouse);
    void onMouseMoved(sf::Vector2f mouse);

    // maj affichage/cooldowns si besoin
    void update(float /*dt*/);

    // rendu
    void draw(sf::RenderTarget& rt) const;

    // API simple pour MAJ compteurs matériaux
    void setMaterialCount(Material m, int v);
    int  materialCount(Material m) const;

    // Pour GameScene : savoir si on est en drag et où
    bool isDragging() const { return dragging_; }
    sf::Vector2f dragPosition() const {
    return dragGhost_ ? dragGhost_->getPosition() : sf::Vector2f{};
}
    Unit draggingUnit() const { return dragUnit_; }

private:
    struct MatBox {
        sf::Sprite icon;
        sf::Text   label;       // nom
        sf::Text   counter;     // quantité
        sf::RectangleShape plate; // fond carré
        int count = 0;

        MatBox(const sf::Texture& tex, const sf::Font& font, const std::string& name);
        void setCount(int v);
        void setPosition(sf::Vector2f p);
        void draw(sf::RenderTarget& rt) const;

        sf::CircleShape badge_;
        sf::Text        badgeText_;

    };

    struct DefButton {
        sf::Sprite icon;
        sf::CircleShape ring;
        sf::Text   label;
        bool enabled = false;

        DefButton(const sf::Texture& tex, const sf::Font& font, const std::string& name);
        void setEnabled(bool on);
        void setPosition(sf::Vector2f p);
        sf::FloatRect bounds() const;
        void draw(sf::RenderTarget& rt) const;
    };

    // helpers
    static sf::Color dimColor(sf::Color c, float f);
    bool pointIn(const sf::FloatRect& r, sf::Vector2f p) const { return r.contains(p); }
    bool canPlaceAtPixel(sf::Vector2f px) const; // valide zone libre (Tile::Rock / buildable)

private:
    sf::RenderWindow& win_;
    const Map& map_;
    float tileSize_;

    // visuel panneau
    bool visible_ = false;
    sf::Texture bgTex_;
    std::unique_ptr<sf::Sprite> bg_;
    std::unique_ptr<sf::Text>   title_;
    std::unique_ptr<sf::Sprite> menuBtn_;
    std::unique_ptr<sf::Sprite> dragGhost_;
    sf::RectangleShape header_;


    // bouton de menu (always visible)
    sf::Texture btnTex_;
    sf::FloatRect menuBtnHit_;

    // police
    sf::Font font_;

    // textures items
    std::unique_ptr<sf::Texture> matTex_[3];
    std::unique_ptr<sf::Texture> unitTex_[3];

    // éléments (pointeurs pour éviter les ctors par défaut SFML3)
    std::unique_ptr<MatBox>   mat_[3];
    std::unique_ptr<DefButton> def_[3];

    // drag & drop
    bool dragging_ = false;
    Unit dragUnit_ = Unit::Cannon;
    bool dragValid_ = false;

    // layout
   sf::FloatRect panelBounds_{ sf::Vector2f{30.f, 30.f}, sf::Vector2f{420.f, 360.f} };
};
