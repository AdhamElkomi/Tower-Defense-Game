#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <array>
#include <string>
#include <cstdint>
#include "Map.hpp"

class BuildMenu {
public:
    enum class Material { Wood=0, Stone=1, Crystal=2, Count=3 };
    enum class Unit     { Cannon=0, Archer=1, Mage=2, Count=3 };

    // bgPath/menuBtnPath peuvent pointer vers tes assets (png fournis)
    BuildMenu(sf::RenderWindow& win, float tileSize, const Map& map,
              const std::string& fontPath     = "../assets/ui/FreckleFace-Regular.ttf",
              const std::string& bgPath       = "../assets/ui/menu_bg.png",
              const std::string& menuBtnPath  = "../assets/ui/menu_button.png",
              const std::string& woodPath     = "../assets/ui/material_wood.png",
              const std::string& stonePath    = "../assets/ui/material_stone.png",
              const std::string& crystalPath  = "../assets/ui/material_crystal.png",
              const std::string& cannonPath   = "../assets/ui/def_cannon.png",
              const std::string& archerPath   = "../assets/ui/def_archer.png",
              const std::string& magePath     = "../assets/ui/def_mage.png");

    // état
    void toggle()         { visible_ = !visible_; }
    bool visible() const  { return visible_; }

    // events (appelle-les depuis GameScene)
    void onMousePressed(sf::Vector2f mouse);
    void onMouseReleased(sf::Vector2f mouse);
    void onMouseMoved(sf::Vector2f mouse);

    // update/draw
    void update(float dt);
    void draw(sf::RenderTarget& rt) const;

    // API compteurs
    void setMaterialCount(Material m, int v);
    int  materialCount(Material m) const;

    // drag info pour la scène (si tu veux l’utiliser)
    bool         isDragging()   const { return dragging_; }
    sf::Vector2f dragPosition() const { return dragGhost_ ? dragGhost_->getPosition() : sf::Vector2f{}; }
    Unit         draggingUnit() const { return dragUnit_; }

    // bouton menu (pour tests/clic externes éventuels)
    sf::FloatRect menuButtonBounds() const { return menuBtn_ ? menuBtn_->getGlobalBounds() : sf::FloatRect{}; }

private:
    // === widgets internes ================================================
    struct MatBox {
        sf::RectangleShape plate;     // carré
        sf::Sprite         icon;      // icône (bois/pierre/cristal)
        sf::CircleShape    badge;     // pastille ronde
        sf::Text           badgeTxt;  // texte compteur
        int                count = 0;

        MatBox(const sf::Texture& tex, const sf::Font& font, unsigned charSize);
        void setCount(int v);
        void setPosition(sf::Vector2f p);
        void draw(sf::RenderTarget& rt) const;
    };

    struct DefButton {
        sf::CircleShape ring;   // bouton rond
        sf::Sprite      icon;   // icône d’unité
        sf::Text        label;  // nom (facultatif)
        bool            enabled = true;

        DefButton(const sf::Texture& tex, const sf::Font& font, const std::string& name);
        void setEnabled(bool on);
        void setPosition(sf::Vector2f center);
        sf::FloatRect bounds() const; // pour hit-test
        void draw(sf::RenderTarget& rt) const;
    };

    // === helpers ==========================================================
    static sf::Color dimColor(sf::Color c, float factor);
    bool  canPlaceAtPixel(sf::Vector2f px) const;
    
    void  layout(); // place tous les éléments dans le panneau

private:
    // contexte
    sf::RenderWindow& win_;
    const Map&        map_;
    float             tileSize_;

    // ressources
    sf::Font  font_;

    std::unique_ptr<sf::Texture> bgTex_;
    std::unique_ptr<sf::Sprite>  bg_;

    std::unique_ptr<sf::Texture> menuBtnTex_;
    std::unique_ptr<sf::Sprite>  menuBtn_;
    sf::FloatRect                menuBtnHit_; // pour clic

    std::unique_ptr<sf::Texture> matTex_[3];
    std::unique_ptr<sf::Texture> unitTex_[3];

    // éléments UI (pointeurs pour éviter ctors par défaut SFML3)
    std::unique_ptr<MatBox>    mats_[3];
    std::unique_ptr<DefButton> defs_[3];

    // titre
    std::unique_ptr<sf::Text> title_;

    // état panneau
    bool visible_ = false;
    // BuildMenu.hpp
    sf::FloatRect panelBounds_{ {60.f, 140.f}, {720.f, 480.f} };


    // drag&drop unité
    bool                       dragging_ = false;
    Unit                       dragUnit_ = Unit::Cannon;
    std::unique_ptr<sf::Sprite> dragGhost_;
    bool                       dragValid_ = false;

    // compteurs matériaux
    int materialCount_[3] {0,0,0};
};
