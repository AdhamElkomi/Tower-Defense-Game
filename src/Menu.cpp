#include "Menu.hpp"
#include <cmath>

// Shader fragment (GLSL) sans RenderTexture : on calcule en coord écran via gl_FragCoord
// u_pos: position écran du coin supérieur gauche du cadre
// u_size: taille cadre (pix)
// Coins arrondis + gradient vertical + ombre douce + glow animé sur le bord
static const char* kPanelFragment = R"GLSL(
uniform vec2  u_pos;
uniform vec2  u_size;
uniform float u_radius;
uniform vec4  u_innerA;
uniform vec4  u_innerB;
uniform vec4  u_shadowCol;
uniform vec4  u_borderCol;
uniform float u_time;
uniform float u_fillAlpha; // 0 = pas de remplissage (laisser l'image), 1 = remplir

float sdRoundBox(vec2 p, vec2 b, float r){
    vec2 q = abs(p) - (b - vec2(r));
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

void main(){
    vec2 uv = gl_FragCoord.xy - u_pos;
    vec2 halfSize = u_size * 0.5;
    vec2 p = uv - halfSize;

    float d = sdRoundBox(p, halfSize, u_radius);
    float alpha = smoothstep(0.0, -1.5, d);

    float gy = clamp(uv.y / u_size.y, 0.0, 1.0);
    vec4 inner = mix(u_innerA, u_innerB, gy);

    float shadowEdge = 28.0;
    float shadowAmt = 1.0 - clamp(d / shadowEdge, 0.0, 1.0);
    vec4 shadowCol = u_shadowCol * shadowAmt * (1.0 - alpha);

    float pulse = 0.5 + 0.5 * sin(u_time * 2.6);
    float border = smoothstep(2.0, 0.0, abs(d));
    vec4 borderCol = u_borderCol * border * (0.25 + 0.75 * pulse);

    // applique u_fillAlpha au remplissage intérieur
    vec4 fill = inner * (alpha * u_fillAlpha);

    vec4 col = shadowCol + fill + borderCol;
    col.a = max(max(fill.a, borderCol.a), shadowCol.a);
    gl_FragColor = col;
}
)GLSL";

// ---- Constructor
Menu::Menu(sf::RenderWindow& win) : win_(win) {
    loadAssets();
    shaderOk_ = panelShader_.loadFromMemory(kPanelFragment, sf::Shader::Type::Fragment);
    buildLayout();
    positionElements();
}

// ---- tick: events + update + draw (sans clear/display)
std::optional<MenuChoice> Menu::tick() {
    MenuChoice choice;

    // Temps animé pour le glow
    animTime_ += animClock_.restart().asSeconds();

    while (auto ev = win_.pollEvent()) {
        if (ev->is<sf::Event::Closed>()) {
            choice.exit = true; return choice;
        }
        if (ev->is<sf::Event::Resized>()) {
            positionElements();
        }
        if (const auto* k = ev->getIf<sf::Event::KeyPressed>()) {
            using Scan = sf::Keyboard::Scan;
            if (k->scancode == Scan::Up) {
                focusIndex_ = (focusIndex_ + (int)buttons_.size() - 1) % (int)buttons_.size();
            } else if (k->scancode == Scan::Down) {
                focusIndex_ = (focusIndex_ + 1) % (int)buttons_.size();
            } else if (k->scancode == Scan::Enter) {
                const auto& id = buttons_[focusIndex_].id;
                if (id == "start")      { choice.start = true; return choice; }
                if (id == "difficulty") { choice.openDifficulty = true; return choice; }
                if (id == "exit")       { choice.exit = true; return choice; }
            }
        }
        if (const auto* m = ev->getIf<sf::Event::MouseButtonPressed>()) {
            if (m->button == sf::Mouse::Button::Left) {
                auto mp = win_.mapPixelToCoords(sf::Vector2i{m->position.x, m->position.y});
                for (auto& b : buttons_) {
                    sf::FloatRect bounds = b.hasSprite() ? b.sprite->getGlobalBounds()
                                                          : b.box.getGlobalBounds();
                    if (bounds.contains(mp)) {
                        if (b.id == "start")      { choice.start = true; return choice; }
                        if (b.id == "difficulty") { choice.openDifficulty = true; return choice; }
                        if (b.id == "exit")       { choice.exit = true; return choice; }
                    }
                }
            }
        }
    }

    // Hover/focus
    auto mouse = win_.mapPixelToCoords(sf::Mouse::getPosition(win_));
    updateHoverFocus(mouse);

    // Dessin (sans clear/display)
    draw();
    return std::nullopt;
}

// ---- render: alias (utile si tu préfères appeler depuis App::run())
void Menu::render() { draw(); }

void Menu::setDifficultySubtitle(const std::string& text) {
    for (auto& b : buttons_) {
        if (b.id == "difficulty" && b.label) {
            b.label->setString("Difficulty\n" + text);
            b.label->setLineSpacing(0.9f);
        }
    }
}

// ---- Assets
void Menu::loadAssets() {
    // Police + icônes
    loadFont("../assets/fonts/Roboto-Regular.ttf");
    loadTexture(icoStart_, "assets/images/start.png");
    loadTexture(icoGear_,  "assets/images/gear.png");
    loadTexture(icoExit_,  "assets/images/exit.png");

    // Fond d'écran (cover)
    if (loadTexture(texBg_, "../assets/images/background.png")) {
        bg_ = std::make_unique<sf::Sprite>(texBg_);
    }

    // *** Fond du petit cadre (ton image) ***
    if (loadTexture(texCardBg_, "../assets/images/first-bg.png")) {
        cardBg_ = std::make_unique<sf::Sprite>(texCardBg_);
    }

    // Texture de bouton arrondi (optionnelle)
    loadTexture(texBtn_, "assets/images/btn.png"); // <-- corrige le chemin ici
}

// ---- Layout
void Menu::buildLayout() {
    // Titre
    title_ = std::make_unique<sf::Text>(font_, sf::String("Tower Defense"), 42u);
    title_->setFillColor(sf::Color(240, 240, 248));

    // Ombre douce sous le panneau (grosse tache soft)
    dropShadow_.setSize(cardSize_);
    dropShadow_.setFillColor(sf::Color(0,0,0,110));

    // Boutons
    buttons_.clear();
    auto makeBtn = [&](const std::string& id, const std::string& text, const sf::Texture* icon) {
        MenuButton b;
        b.id = id;

        if (texBtn_.getSize().x > 0) {
            b.sprite = std::make_unique<sf::Sprite>(texBtn_);
            auto texSz = texBtn_.getSize();
            const sf::Vector2f target{400.f, 64.f};
            b.sprite->setScale(sf::Vector2f{ target.x / texSz.x, target.y / texSz.y });
        } else {
            b.box.setSize(sf::Vector2f{400.f, 64.f});
            b.box.setFillColor(sf::Color(46, 52, 68));
        }

        b.label = std::make_unique<sf::Text>(font_, sf::String(text), 22u);
        b.label->setFillColor(sf::Color(230, 233, 240));

        if (icon && icon->getSize().x > 0) {
            b.icon = std::make_unique<sf::Sprite>(*icon);
            b.icon->setScale(sf::Vector2f{0.6f, 0.6f});
        }
        return b;
    };

    buttons_.push_back(makeBtn("start",      "Start",      &icoStart_));
    buttons_.push_back(makeBtn("difficulty", "Difficulty", &icoGear_));
    buttons_.push_back(makeBtn("exit",       "Exit",       &icoExit_));
}

void Menu::positionElements() {
    const auto viewSize = win_.getView().getSize();
    const sf::Vector2f center{ viewSize.x * 0.5f, viewSize.y * 0.5f };

    // Fond cover
    if (bg_) {
        auto tex = bg_->getTexture().getSize();
        float sx = viewSize.x / (float)tex.x;
        float sy = viewSize.y / (float)tex.y;
        float s  = std::max(sx, sy);
        bg_->setScale(sf::Vector2f{s, s});
        bg_->setPosition(sf::Vector2f{0.f, 0.f});
    }

    // Panneau centré
    cardPos_ = sf::Vector2f{ center.x - cardSize_.x/2.f, center.y - cardSize_.y/2.f };

    if (cardBg_) {
    auto texSz = texCardBg_.getSize();

    // 👉 ton scale fixe
    sf::Vector2f scale{0.66f, 0.48f};
    cardBg_->setScale(scale);

    // Taille réelle de l’image après scale
    sf::Vector2f scaledSize{
        texSz.x * scale.x,
        texSz.y * scale.y
    };

    // On centre dans le cadre (cardPos_ + cardSize_/2)
    sf::Vector2f cardCenter = cardPos_ + cardSize_ * 0.5f;
    sf::Vector2f spriteCenter = scaledSize * 0.5f;

    cardBg_->setPosition(cardCenter - spriteCenter);
}


    // Ombre douce (un peu plus grande + offset)
    dropShadow_.setSize(cardSize_ + sf::Vector2f{18.f, 24.f});
    dropShadow_.setPosition(cardPos_ + sf::Vector2f{-9.f, 7.f});
    dropShadow_.setFillColor(sf::Color(0,0,0,105));

    // Titre au-dessus
    sf::FloatRect tb = title_->getLocalBounds();
    const sf::Vector2f tpos{tb.position.x, tb.position.y};
    const sf::Vector2f tsz {tb.size.x,     tb.size.y    };
    title_->setOrigin(sf::Vector2f{ tpos.x + tsz.x/2.f, tpos.y + tsz.y/2.f });
    title_->setPosition(sf::Vector2f{ center.x, cardPos_.y - 56.f });

    // Boutons
    float gap = 18.f;
    float top = cardPos_.y + 76.f;
    for (std::size_t i = 0; i < buttons_.size(); ++i) {
        auto& b = buttons_[i];
        float bx = cardPos_.x + (cardSize_.x - 400.f) / 2.f;
        float by = top + (float)i * (64.f + gap);

        if (b.hasSprite()) b.sprite->setPosition(sf::Vector2f{bx, by});
        else               b.box.setPosition(sf::Vector2f{bx, by});

        sf::FloatRect lb = b.label->getLocalBounds();
        const sf::Vector2f lbPos{lb.position.x, lb.position.y};
        const sf::Vector2f lbSz {lb.size.x,     lb.size.y    };

        float iconPad = (b.hasIcon() ? 46.f : 0.f);
        b.label->setOrigin(lbPos);
        b.label->setPosition(sf::Vector2f{ bx + 20.f + iconPad, by + (64.f - lbSz.y) / 2.f - 6.f });

        if (b.hasIcon()) {
            auto texSz = b.icon->getTexture().getSize();
            float scaledH = (float)texSz.y * b.icon->getScale().y;
            float iy = by + (64.f - scaledH)/2.f;
            b.icon->setPosition(sf::Vector2f{bx + 16.f, iy});
        }
    }
}

void Menu::updateHoverFocus(const sf::Vector2f& mouse) {
    for (std::size_t i = 0; i < buttons_.size(); ++i) {
        auto& b = buttons_[i];
        sf::FloatRect bounds = b.hasSprite() ? b.sprite->getGlobalBounds()
                                             : b.box.getGlobalBounds();
        b.hovered = bounds.contains(mouse);
        b.focused = ((int)i == focusIndex_);

        if (b.hasSprite()) {
            if (b.focused)      b.sprite->setColor(sf::Color(255,255,255,255));
            else if (b.hovered) b.sprite->setColor(sf::Color(230,235,255,255));
            else                b.sprite->setColor(sf::Color(255,255,255,225));
        } else {
            const sf::Color base (46, 52, 68);
            const sf::Color hover(62, 70, 90);
            const sf::Color focus(84, 96, 122);
            if (b.focused)      b.box.setFillColor(focus);
            else if (b.hovered) b.box.setFillColor(hover);
            else                b.box.setFillColor(base);
        }
    }
}

void Menu::draw() {
    // Fond
    if (bg_) win_.draw(*bg_);

    // Ombre douce large
    win_.draw(dropShadow_);

// 1) Image de fond du cadre si disponible
if (cardBg_) {
    win_.draw(*cardBg_);
}

// 2) Overlay shader : ombre/glow + éventuellement remplissage
if (shaderOk_) {
    sf::RectangleShape panel(cardSize_);
    panel.setPosition(cardPos_);

    panelShader_.setUniform("u_pos",       sf::Glsl::Vec2{cardPos_.x, cardPos_.y});
    panelShader_.setUniform("u_size",      sf::Glsl::Vec2{cardSize_.x, cardSize_.y});
    panelShader_.setUniform("u_radius",    cornerRadius_);
    // teintes internes (peu importe si fillAlpha=0)
    panelShader_.setUniform("u_innerA",    sf::Glsl::Vec4{0.13f, 0.15f, 0.22f, 0.98f});
    panelShader_.setUniform("u_innerB",    sf::Glsl::Vec4{0.09f, 0.11f, 0.17f, 0.98f});
    panelShader_.setUniform("u_shadowCol", sf::Glsl::Vec4{0.0f, 0.0f, 0.0f, 0.55f});
    panelShader_.setUniform("u_borderCol", sf::Glsl::Vec4{0.40f, 0.60f, 1.0f, 0.70f});
    panelShader_.setUniform("u_time",      animTime_);

    // *** clé : si image présente -> pas de remplissage ***
    float fillAlpha = cardBg_ ? 0.f : 1.f;
    panelShader_.setUniform("u_fillAlpha", fillAlpha);

    sf::RenderStates rs;
    rs.shader = &panelShader_;
    win_.draw(panel, rs);
}
    // Titre
    win_.draw(*title_);

    // Boutons
    for (auto& b : buttons_) {
        if (b.hasSprite()) win_.draw(*b.sprite);
        else               win_.draw(b.box);
        if (b.hasIcon())   win_.draw(*b.icon);
        win_.draw(*b.label);
    }
}

// ---- Utils
bool Menu::loadFont(const std::string& path)  { return font_.openFromFile(path); }
bool Menu::loadTexture(sf::Texture& t, const std::string& path) {
    t.setSmooth(true);
    return t.loadFromFile(path);
}
