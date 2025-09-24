#include "BuildMenu.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>


namespace {
    const float PANEL_W = 520.f, PANEL_H = 430.f;
    const float PAD      = 28.f;      // marge intérieure
    const float GAP_X    = 32.f;      // espace horizontal entre cases
    const float MAT_SIZE = 96.f;      // carré des matériaux
    const float RING_R   = 52.f;      // rayon boutons tours
    const float TITLE_SIZE = 40.f;    // taille du titre
}




// ---------- MatBox ----------
BuildMenu::MatBox::MatBox(const sf::Texture& tex, const sf::Font& font, const std::string& name)
: icon(tex),
  label(font, name, 22),
  counter(font, "0", 20),
  plate(),
  badge_(),                          // ok: CircleShape a un ctor par défaut
  badgeText_(font, "0", 18) 
{
   /*  plate.setSize({110.f, 110.f});
    plate.setFillColor(sf::Color(30,30,32,200));
    plate.setOutlineThickness(2.f);
    plate.setOutlineColor(sf::Color(80,80,90,220));
    icon.setScale(sf::Vector2f(0.9f,0.9f));
    label.setFillColor(sf::Color(235,235,240));
    counter.setFillColor(sf::Color(180,220,255));*/



     // plaque arrière (gris foncé)
    plate.setSize(sf::Vector2f(MAT_SIZE, MAT_SIZE));
    plate.setFillColor(sf::Color(24, 46, 50, 255));
    plate.setOutlineThickness(8.f);                 // gros chanfrein
    plate.setOutlineColor(sf::Color(180, 190, 200));// “métal”
    plate.setScale(sf::Vector2f(1.f, 1.f));         // carré

    // icône
    icon.setScale(sf::Vector2f(0.85f, 0.85f));
    label.setFillColor(sf::Color(220,220,225));

    // badge compteur (petit rond orange)
    badge_.setRadius(14.f);
    badge_.setPointCount(40);
    badge_.setFillColor(sf::Color(218,144,74));
    badgeText_ = sf::Text(font, "0", 18);
    badgeText_.setFillColor(sf::Color(40,25,10));
}

void BuildMenu::MatBox::setCount(int v){
    count = std::max(0, v);
    counter.setString(std::to_string(count));   // (facultatif si tu n’affiches pas “counter”)
    badgeText_.setString(std::to_string(count));
}

void BuildMenu::MatBox::setPosition(sf::Vector2f p){
    plate.setPosition(p);

    const auto pb = plate.getGlobalBounds();
    const float cx = pb.position.x + pb.size.x*0.5f;
    // icône centrée
    const auto ib = icon.getGlobalBounds();
    icon.setPosition(sf::Vector2f(cx - ib.size.x*0.5f, pb.position.y + 10.f));

    // label en bas-gauche
    label.setPosition(sf::Vector2f(pb.position.x + 8.f, pb.position.y + pb.size.y + 6.f));

    // badge en haut-droit
    badge_.setPosition(sf::Vector2f(pb.position.x + pb.size.x - badge_.getRadius()*2.f + 6.f,
                                    pb.position.y - 6.f));
    // texte centré dans le badge
    const auto bb = badge_.getGlobalBounds();
    const auto tb = badgeText_.getGlobalBounds();
    badgeText_.setPosition(sf::Vector2f(
        bb.position.x + (bb.size.x - tb.size.x)*0.5f,
        bb.position.y + (bb.size.y - tb.size.y)*0.5f - 2.f
    ));
}

void BuildMenu::MatBox::draw(sf::RenderTarget& rt) const{
    rt.draw(plate);
    rt.draw(icon);
    rt.draw(label);
    rt.draw(counter);
}

// ---------- DefButton ----------
BuildMenu::DefButton::DefButton(const sf::Texture& tex, const sf::Font& font, const std::string& name)
: icon(tex), ring(42.f), label(font, name, 18)
{
    ring.setPointCount(48);
    ring.setFillColor(sf::Color(28,28,30,210));
    ring.setOutlineThickness(3.f);
    ring.setOutlineColor(sf::Color(90,90,120,220));
    icon.setScale(sf::Vector2f(0.85f,0.85f));
    label.setFillColor(sf::Color(230,230,235));
}

void BuildMenu::DefButton::setEnabled(bool on){
    enabled = on;
    float f = enabled ? 1.f : 0.42f;
    icon.setColor(dimColor(sf::Color::White, f));
    ring.setOutlineColor(enabled ? sf::Color(120,200,120,240)
                                 : sf::Color(90,90,90,160));
}

void BuildMenu::DefButton::setPosition(sf::Vector2f p){
    ring.setPosition(sf::Vector2f(p.x - 42.f, p.y - 42.f)); // ring est centré visuellement

    const sf::FloatRect ib = icon.getGlobalBounds();
    icon.setPosition(sf::Vector2f(
        p.x - ib.size.x*0.5f,
        p.y - ib.size.y*0.5f
    ));

    const sf::FloatRect lb = label.getGlobalBounds();
    label.setPosition(sf::Vector2f(
        p.x - lb.size.x*0.5f,
        p.y + 52.f
    ));
}

sf::FloatRect BuildMenu::DefButton::bounds() const{
    const sf::Vector2f rp = ring.getPosition();
    const float d = ring.getRadius()*2.f;
    return sf::FloatRect{ rp, sf::Vector2f{d, d} };
}


void BuildMenu::DefButton::draw(sf::RenderTarget& rt) const{
    rt.draw(ring);
    rt.draw(icon);
    rt.draw(label);
}

// ---------- utils ----------
sf::Color BuildMenu::dimColor(sf::Color c, float f){
    f = std::clamp(f, 0.f, 1.f);
    return sf::Color(
        static_cast<std::uint8_t>(c.r * f),
        static_cast<std::uint8_t>(c.g * f),
        static_cast<std::uint8_t>(c.b * f),
        c.a
    );
}

bool BuildMenu::canPlaceAtPixel(sf::Vector2f px) const{
    int tx = int(px.x / tileSize_);
    int ty = int(px.y / tileSize_);
    if (!map_.inBounds(tx,ty)) return false;
    const auto& c = map_.at(tx,ty);
    // règle: zones libres = Tile::Rock buildable=true
    return (c.ground == Tile::Rock) && c.buildable;
}

// ---------- BuildMenu ----------
BuildMenu::BuildMenu(sf::RenderWindow& win, float tileSize, const Map& map,
                     const std::string& fontPath,
                     const std::string& bgPath)
: win_(win), map_(map), tileSize_(tileSize)
{
    // font
    // panel geometry (type SFML3)
    panelBounds_ = sf::FloatRect({30.f, 30.f}, {PANEL_W, PANEL_H});

    // background (image bois)
    bgTex_.loadFromFile(bgPath);
    bg_ = std::make_unique<sf::Sprite>(bgTex_);
    {
        const auto gb = bg_->getGlobalBounds();
        const float sx = panelBounds_.size.x / std::max(1.f, gb.size.x);
        const float sy = panelBounds_.size.y / std::max(1.f, gb.size.y);
        bg_->setScale(sf::Vector2f(sx, sy));
        bg_->setPosition(panelBounds_.position);
    }

    // titre centré — “STOCK & TOOLS”
    font_.openFromFile(fontPath);
    title_ = std::make_unique<sf::Text>(font_, "STOCK & TOOLS", (unsigned)TITLE_SIZE);
    title_->setFillColor(sf::Color(250,240,220));
    {
        const float cx = panelBounds_.position.x + panelBounds_.size.x*0.5f;
        // centrage horizontal
        const auto tb = title_->getGlobalBounds();
        title_->setPosition(sf::Vector2f(cx - tb.size.x*0.5f, panelBounds_.position.y + 12.f));
    }


    // header
   /*  header_.setSize(sf::Vector2f(panelBounds_.size.x, 50.f));
    header_.setFillColor(sf::Color(18,18,24,220));
    header_.setPosition(panelBounds_.position);

    // title (Text SFML3 : ctor = (font, string, charSize))
    title_ = std::make_unique<sf::Text>(font_, "Armory & Supplies", 26);
    title_->setFillColor(sf::Color(240,240,245));
    title_->setPosition(sf::Vector2f(
        panelBounds_.position.x + 16.f,
        panelBounds_.position.y + 10.f
));*/

// menu button
btnTex_.loadFromFile("../assets/ui/menu_btn.png");
menuBtn_ = std::make_unique<sf::Sprite>(btnTex_);
menuBtn_->setPosition(sf::Vector2f(16.f,16.f));
menuBtnHit_ = menuBtn_->getGlobalBounds();

}

void BuildMenu::setMaterialCount(Material m, int v){
    int i = static_cast<int>(m);
    if (i>=0 && i<3 && mat_[i]) mat_[i]->setCount(v);
}
int BuildMenu::materialCount(Material m) const {
    int i = static_cast<int>(m);
    return (i>=0 && i<3 && mat_[i]) ? mat_[i]->count : 0;
}

void BuildMenu::onMousePressed(sf::Vector2f mouse){
    // bouton menu (toujours actif)
    if (menuBtnHit_.contains(mouse)) { toggle(); return; }
    if (!visible_) return;

    // clic sur un bouton d’unité → start drag si enabled
    for (int i=0;i<3;++i){
        if (def_[i] && def_[i]->bounds().contains(mouse) && def_[i]->enabled){
            dragging_ = true;
            dragUnit_ = static_cast<Unit>(i);
            dragGhost_ = std::make_unique<sf::Sprite>(*unitTex_[0]);
            dragGhost_->setScale(sf::Vector2f(0.9f,0.9f));
            dragGhost_->setColor(sf::Color(255,255,255,220));

            dragValid_ = canPlaceAtPixel(mouse);
    

            break;
        }
    }
}

void BuildMenu::onMouseReleased(sf::Vector2f mouse){
    if (!dragging_) return;
    bool ok = canPlaceAtPixel(mouse);
    // ici : si ok → signaler à la scène de construire (via un callback si tu veux)
    // (pour l’instant on s’arrête au visuel)
    dragging_ = false;
    (void)ok;
}

void BuildMenu::onMouseMoved(sf::Vector2f mouse){
    if (dragging_ && dragGhost_){
        const int idx = static_cast<int>(dragUnit_);
        dragGhost_->setTexture(*unitTex_[idx]);  // si tu veux refléter le type
        dragGhost_->setScale(sf::Vector2f(0.9f,0.9f));
        dragGhost_->setPosition(mouse);
        dragValid_ = canPlaceAtPixel(mouse);
    }
}

void BuildMenu::update(float){
    // (place pour cooldowns/boutons selon ressources)
}

void BuildMenu::draw(sf::RenderTarget& rt) const {
    if (menuBtn_) rt.draw(*menuBtn_);
    if (!visible_) {
        if (dragging_ && dragGhost_) {
            sf::Sprite ghost = *dragGhost_;
            ghost.setColor(dragValid_ ? sf::Color(120,255,140,230)
                                      : sf::Color(255,120,120,230));
            rt.draw(ghost);
        }
        return;
    }

    if (bg_)    rt.draw(*bg_);
    rt.draw(header_);
    if (title_) rt.draw(*title_);

    for (int i=0;i<3;++i) if (mat_[i])  mat_[i]->draw(rt);
    for (int i=0;i<3;++i) if (def_[i])  def_[i]->draw(rt);

    if (dragging_ && dragGhost_) {
        sf::Sprite ghost = *dragGhost_;
        ghost.setColor(dragValid_ ? sf::Color(120,255,140,230)
                                  : sf::Color(255,120,120,230));
        rt.draw(ghost);
    }
}
