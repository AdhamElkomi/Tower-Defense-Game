#include "BuildMenu.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>

// ================= MatBox =================
BuildMenu::MatBox::MatBox(const sf::Texture& tex, const sf::Font& font, unsigned charSize)
: plate(), icon(tex), badge(18.f), badgeTxt("0", font, 48)
{
    plate.setSize(sf::Vector2f(120.f, 120.f));              // ⬆️ plus grand
    plate.setFillColor(sf::Color(26,41,44,240));
    plate.setOutlineThickness(10.f);                         // ⬆️ plus épais
    plate.setOutlineColor(sf::Color(185,185,140,255));      // gris clair

    icon.setScale(sf::Vector2f(0.1f, 0.1f));   
    
    
    badge = sf::CircleShape(30.f);   // rayon 20 px au lieu de 14           // ⬆️ icône

    badge.setOrigin(sf::Vector2f(28.f,28.f));
    badge.setFillColor(sf::Color(223,155,74));

    badgeTxt.setFillColor(sf::Color::White);
}

void BuildMenu::MatBox::setCount(int v){
    count = std::max(0, v);
    badgeTxt.setString(std::to_string(count));
}


void BuildMenu::setUnitEnabled(Unit u, bool on){
    int i = static_cast<int>(u);
    if (i>=0 && i<3 && defs_[i]) defs_[i]->setEnabled(on);
}

void BuildMenu::MatBox::setPosition(sf::Vector2f p){
    plate.setPosition(p);

    // centrage précis de l’icône dans la plaque
    const auto pb = plate.getGlobalBounds();            // {position,size}
    const auto ib = icon.getGlobalBounds();             // taille visuelle de l’icône (après scale)
    icon.setPosition(sf::Vector2f(
        pb.left + (pb.width - ib.width) * 0.5f,
        pb.top + (pb.height - ib.height) * 0.5f - 4.f
    ));

    // pastille en bas-droite
    const sf::Vector2f c(
        pb.left + pb.width - 10.f,
        pb.top + pb.height - 10.f
    );
    badge.setPosition(c);

    // centrer le texte dans la pastille (offset empirique propre)
    const auto tb = badgeTxt.getGlobalBounds();
    badgeTxt.setPosition(c - sf::Vector2f(tb.width*0.5f, tb.height*0.85f));
}

void BuildMenu::MatBox::draw(sf::RenderTarget& rt) const{
    rt.draw(plate);
    rt.draw(icon);
    rt.draw(badge);
    rt.draw(badgeTxt);
}

// ================= DefButton =================
BuildMenu::DefButton::DefButton(const sf::Texture& tex, const sf::Font& font, const std::string& name)
: ring(54.f), icon(tex), label(name, font, 28)              // ⬆️ rayon + texte
{
    ring.setOrigin(sf::Vector2f(54.f,54.f));
    ring.setFillColor(sf::Color(38,52,56,230));
    ring.setOutlineThickness(6.f);                           // ⬆️ contour
    ring.setOutlineColor(sf::Color(185,185,190,255));        // gris clair

    icon.setScale(sf::Vector2f(0.90f,0.90f));                // ⬆️ icône
    label.setFillColor(sf::Color(235,235,240));
}

void BuildMenu::DefButton::setEnabled(bool on){
    enabled = on;
    float k = on ? 1.f : 0.45f;
    ring.setFillColor(dimColor(sf::Color(38,52,56,220), k));
    icon.setColor(dimColor(sf::Color::White, k));
}

void BuildMenu::DefButton::setPosition(sf::Vector2f c){
    ring.setPosition(c);
    const auto ib = icon.getGlobalBounds();
    icon.setPosition(sf::Vector2f(c.x - ib.width*0.5f, c.y - ib.height*0.5f));
    const auto lb = label.getGlobalBounds();
    label.setPosition(sf::Vector2f(c.x - lb.width*0.5f, c.y + 60.f)); // ⬇️ un peu plus bas
}

sf::FloatRect BuildMenu::DefButton::bounds() const{
    auto p = ring.getPosition();
    float d = ring.getRadius() * 2.f;
    return sf::FloatRect{ sf::Vector2f(p.x - ring.getRadius(), p.y - ring.getRadius()),
                          sf::Vector2f(d, d) };
}

void BuildMenu::DefButton::draw(sf::RenderTarget& rt) const{
    rt.draw(ring);
    rt.draw(icon);
    rt.draw(label);
}

// ================= Helpers =================
sf::Color BuildMenu::dimColor(sf::Color c, float f){
    f = std::clamp(f, 0.f, 1.f);
    return sf::Color(
        static_cast<std::uint8_t>(c.r * f),
        static_cast<std::uint8_t>(c.g * f),
        static_cast<std::uint8_t>(c.b * f),
        c.a
    );
}

// case buildable ? (ex: Rock buildable=true)
bool BuildMenu::canPlaceAtPixel(sf::Vector2f px) const{
    int tx = int(px.x / tileSize_);
    int ty = int(px.y / tileSize_);
    if (!map_.inBounds(tx,ty)) return false;
    const auto& c = map_.at(tx,ty);
    return (c.ground == Tile::Rock) && c.buildable;
}

// ================= BuildMenu =================
BuildMenu::BuildMenu(sf::RenderWindow& win, float tileSize, const Map& map,
                     const std::string& fontPath,
                     const std::string& bgPath,
                     const std::string& menuBtnPath,
                     const std::string& woodPath,
                     const std::string& stonePath,
                     const std::string& crystalPath,
                     const std::string& cannonPath,
                     const std::string& archerPath,
                     const std::string& magePath)
: win_(win), map_(map), tileSize_(tileSize)
{
    // police
    (void)font_.loadFromFile(fontPath);

    // textures UI
    bgTex_      = std::make_unique<sf::Texture>();  (void)bgTex_->loadFromFile(bgPath);
    menuBtnTex_ = std::make_unique<sf::Texture>();  (void)menuBtnTex_->loadFromFile(menuBtnPath);

    // sprites UI
    bg_      = std::make_unique<sf::Sprite>(*bgTex_);
    menuBtn_ = std::make_unique<sf::Sprite>(*menuBtnTex_);
    menuBtn_->setScale(sf::Vector2f(0.4f, 0.4f));
    bg_->setScale(sf::Vector2f(1.8f, 1.25f));
    menuBtn_->setPosition(sf::Vector2f(30.f, 1.f));
    menuBtn_->setPosition(sf::Vector2f(20.f, 5.f));
    menuBtnHit_ = menuBtn_->getGlobalBounds();

    // scale bg pour s’adapter au panelBounds_
    {
        auto gb = bg_->getGlobalBounds();
        float sx = panelBounds_.width / std::max(1.f, gb.width);
        float sy = panelBounds_.height / std::max(1.f, gb.height);
        bg_->setScale(sf::Vector2f(sx, sy));
        bg_->setPosition(sf::Vector2f(panelBounds_.left, panelBounds_.top));
    }

    // textures items
    auto load = [](const std::string& p){
        auto t = std::make_unique<sf::Texture>();
        (void)t->loadFromFile(p);
        t->setSmooth(false);
        return t;
    };
    matTex_[0] = load(woodPath);
    matTex_[1] = load(stonePath);
    matTex_[2] = load(crystalPath);
    unitTex_[0]= load(cannonPath);
    unitTex_[1]= load(archerPath);
    unitTex_[2]= load(magePath);

    // éléments
    mats_[0] = std::make_unique<MatBox>(*matTex_[0], font_, 16);
    mats_[1] = std::make_unique<MatBox>(*matTex_[1], font_, 16);
    mats_[2] = std::make_unique<MatBox>(*matTex_[2], font_, 16);

    defs_[0] = std::make_unique<DefButton>(*unitTex_[0], font_, "Cannon");
    defs_[1] = std::make_unique<DefButton>(*unitTex_[1], font_, "Archer");
    defs_[2] = std::make_unique<DefButton>(*unitTex_[2], font_, "Mage");
   defs_[0]->icon.setScale(sf::Vector2f(0.2f, 0.2f));
    defs_[1]->icon.setScale(sf::Vector2f(0.09f, 0.09f));
    defs_[2]->icon.setScale(sf::Vector2f(0.08f, 0.08f));


    // titre
    title_ = std::make_unique<sf::Text>("STOCK & TOOLS", font_, 50);
    title_->setFillColor(sf::Color(250,240,220));
    // position fixée dans layout()

    layout();
}

void BuildMenu::layout(){
    // position + scale bg
    bg_->setPosition(sf::Vector2f(panelBounds_.left, panelBounds_.top));
    {
        const auto gb = bg_->getGlobalBounds();
        const float sx = panelBounds_.width / std::max(1.f, gb.width);
        const float sy = panelBounds_.height / std::max(1.f, gb.height);
        bg_->setScale(sf::Vector2f(sx, sy));
    }

    // titre centré
    auto tb = title_->getGlobalBounds();
    title_->setPosition(sf::Vector2f(
        panelBounds_.left + (panelBounds_.width - tb.width)*0.71f,
        panelBounds_.top + 110.f
    ));

    // zone de contenu (marges)
    const float marginX = 40.f;
    const float marginTop = 84.f;
    const float marginBottom = 46.f;

    const float contentW = panelBounds_.width - marginX*2.f;
    const float contentH = panelBounds_.height - marginTop - marginBottom;

    // GRID 3 colonnes centrées, 2 lignes (mats / defs)
    const int cols = 3;
    const float colW = contentW / cols;
    const float rowGap = 36.f;

    // ---- Matériaux (ligne 1) ----
    // hauteur d’un MatBox (plaque 120 + outline) => on se base sur 124
    const float matH = 124.f;
    const float matY = panelBounds_.top + marginTop;

    for (int i=0; i<3; ++i){
        if (!mats_[i]) continue;
        // centre chaque plaque dans sa colonne
        const float colX = panelBounds_.left + marginX + i*colW;
        const float x = colX + (colW - 50.f)*0.85f;
        mats_[i]->setPosition(sf::Vector2f(x, matY+100.f));
    }

    // ---- Défenses (ligne 2) ----
    const float defsY = matY + matH + rowGap;
    for (int i=0; i<3; ++i){
        if (!defs_[i]) continue;
        const float colX = panelBounds_.left + marginX + i*colW;
        const float cx = colX + colW*0.5f;   // centre de la colonne
        defs_[i]->setPosition(sf::Vector2f(cx+95.f, defsY + 150.f)); // “70” ≈ centre vertical visuel
    }
}


void BuildMenu::setMaterialCount(Material m, int v){
    int i = static_cast<int>(m);
    materialCount_[i] = std::max(0, v);
    if (mats_[i]) mats_[i]->setCount(materialCount_[i]);
}

int BuildMenu::materialCount(Material m) const {
    return materialCount_[static_cast<int>(m)];
}

// ============ input ============
void BuildMenu::onMousePressed(sf::Vector2f mouse){
    // bouton toggle (toujours)
    if (menuBtnHit_.contains(mouse)){
        toggle();
        return;
    }
    if (!visible_) return;

    // start drag si clic sur un bouton actif
    for (int i=0;i<3;++i){
        if (defs_[i] && defs_[i]->bounds().contains(mouse) && defs_[i]->enabled){
            dragging_ = true;
            dragUnit_ = static_cast<Unit>(i);
            dragGhost_ = std::make_unique<sf::Sprite>(*unitTex_[i]);
            dragGhost_->setScale(sf::Vector2f(0.9f,0.9f));
            dragGhost_->setColor(sf::Color(255,255,255,230));
            dragGhost_->setPosition(mouse);
            dragValid_ = canPlaceAtPixel(mouse);
            break;
        }
    }
}

/*void BuildMenu::setUnitEnabled(Unit u, bool on){
    int i = static_cast<int>(u);
    if (i>=0 && i<3 && defs_[i]) defs_[i]->setEnabled(on);
}*/

void BuildMenu::endDrag(){
    dragging_ = false;
    dragGhost_.reset();
}



void BuildMenu::onMouseReleased(sf::Vector2f mouse){
    if (!dragging_) return;
    bool ok = canPlaceAtPixel(mouse);
    // TODO: signaler à la scène la construction si ok
    (void)ok;
    dragging_ = false;
    dragGhost_.reset();
}

void BuildMenu::onMouseMoved(sf::Vector2f mouse){
    if (dragging_ && dragGhost_){
        dragGhost_->setPosition(mouse);
        dragValid_ = canPlaceAtPixel(mouse);
    }
}

void BuildMenu::update(float){
    // activer/désactiver les unités selon ressources (exemple simple)
    // ici : Cannon toujours on, Archer si wood>=3, Mage si crystal>=2
    if (defs_[0]) defs_[0]->setEnabled(true);
    if (defs_[1]) defs_[1]->setEnabled(materialCount_[static_cast<int>(Material::Wood)]   >= 3);
    if (defs_[2]) defs_[2]->setEnabled(materialCount_[static_cast<int>(Material::Crystal)]>= 2);
}

// ============ draw ============
void BuildMenu::draw(sf::RenderTarget& rt) const{
    // bouton menu
    if (menuBtn_) rt.draw(*menuBtn_);

    if (!visible_){
        // si on drag avec le menu fermé, montre seulement le ghost coloré
        if (dragging_ && dragGhost_){
            sf::Sprite ghost = *dragGhost_;
            ghost.setColor(dragValid_ ? sf::Color(120,255,140,230)
                                      : sf::Color(255,120,120,230));
            rt.draw(ghost);
        }
        return;
    }

    // panneau
    if (bg_) rt.draw(*bg_);
    if (title_) rt.draw(*title_);

    for (int i=0;i<3;++i) if (mats_[i]) mats_[i]->draw(rt);
    for (int i=0;i<3;++i) if (defs_[i]) defs_[i]->draw(rt);

    if (dragging_ && dragGhost_){
        sf::Sprite ghost = *dragGhost_;
        ghost.setColor(dragValid_ ? sf::Color(120,255,140,230)
                                  : sf::Color(255,120,120,230));
        rt.draw(ghost);
    }
}
