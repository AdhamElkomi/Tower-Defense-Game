#include "Menu.hpp"
#include <cmath>
#include <cstdint>
#include <algorithm> // std::clamp

// ============================
//  Shader panel (cadre)
// ============================
static const char* kPanelFragment = R"GLSL(
uniform vec2  u_pos;
uniform vec2  u_size;
uniform float u_radius;
uniform vec4  u_innerA;
uniform vec4  u_innerB;
uniform vec4  u_shadowCol;
uniform vec4  u_borderCol;
uniform float u_time;
uniform float u_fillAlpha; // 0 = pas de remplissage, 1 = remplir

float sdRoundBox(vec2 p, vec2 b, float r){
    vec2 q = abs(p) - (b - vec2(r));
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

void main() {
    vec2 uv = gl_FragCoord.xy - u_pos;
    vec2 halfSize = u_size * 0.5;
    vec2 p = uv - halfSize;

    float d = sdRoundBox(p, halfSize, u_radius);

    // intérieur
    float alpha = smoothstep(0.0, -1.5, d);
    float gy = clamp(uv.y / u_size.y, 0.0, 1.0);
    vec4 inner = mix(u_innerA, u_innerB, gy) * u_fillAlpha;

    // ombre soft extérieure
    float shadowEdge = 28.0;
    float shadowAmt = 1.0 - clamp(d / shadowEdge, 0.0, 1.0);
    vec4 shadowCol = u_shadowCol * shadowAmt * (1.0 - alpha);

    // bord brillant
    float pulse = 0.5 + 0.5 * sin(u_time * 2.6);
    float borderMask = smoothstep(2.0, 0.0, abs(d));
    vec4 borderCol = u_borderCol * borderMask * (0.25 + 0.75 * pulse);

    vec4 fill = inner * alpha;
    vec4 col = shadowCol + fill + borderCol;
    col.a = max(max(fill.a, borderCol.a), shadowCol.a);
    gl_FragColor = col;
}
)GLSL";

// ============================
//  Shader button (boutons)
// ============================
static const char* kButtonFragment = R"GLSL(
uniform vec2  u_pos;
uniform vec2  u_size;
uniform float u_radius;
uniform vec4  u_fillA;
uniform vec4  u_fillB;
uniform vec4  u_borderCol;
uniform float u_time;
uniform float u_hover; // 0..1

float sdRoundBox(vec2 p, vec2 b, float r){
    vec2 q = abs(p) - (b - vec2(r));
    return length(max(q,0.0)) + min(max(q.x,q.y),0.0) - r;
}

void main() {
    vec2 uv = gl_FragCoord.xy - u_pos;
    vec2 halfSize = u_size * 0.5;
    vec2 p = uv - halfSize;
    float d = sdRoundBox(p, halfSize, u_radius);

    float alpha = smoothstep(0.0, -1.2, d);
    float gy = clamp(uv.y / u_size.y, 0.0, 1.0);

    // bord toujours visible, plus fort en hover + pulse
    float pulse = 0.5 + 0.5 * sin(u_time * 3.0);
    float borderMask  = smoothstep(2.0, 0.0, abs(d));
    float borderAlpha = mix(0.30, 1.0, u_hover) * (0.65 + 0.35 * pulse);
    vec4 border = u_borderCol * borderMask * borderAlpha;

    // remplissage apparaît avec le hover (gradient)
    vec4 inner = mix(vec4(0.0), mix(u_fillA, u_fillB, gy), u_hover);

    vec4 col = inner * alpha + border;
    col.a = max(inner.a * alpha, border.a);
    gl_FragColor = col;
}
)GLSL";


// ---- Constructor
Menu::Menu(sf::RenderWindow& win) : win_(win) {
    loadAssets();

    shaderOk_    = panelShader_.loadFromMemory(kPanelFragment,  sf::Shader::Type::Fragment);
    btnShaderOk_ = buttonShader_.loadFromMemory(kButtonFragment, sf::Shader::Type::Fragment);

    buildLayout();
    buildSettings();     // Doit exister avant le premier positionnement
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

        // --- Bouton "gear" (cercle) : hover + click
        if (const auto* m = ev->getIf<sf::Event::MouseMoved>()) {
            sf::Vector2f mp{(float)m->position.x, (float)m->position.y};
            sf::Vector2f center = gearButton_.getPosition()
                                + sf::Vector2f{gearButton_.getRadius(), gearButton_.getRadius()};
            gearHover_ = hitCircle(mp, center, gearButton_.getRadius());
        }
        if (const auto* m = ev->getIf<sf::Event::MouseButtonPressed>()) {
            if (m->button == sf::Mouse::Button::Left) {
                sf::Vector2f mp{(float)m->position.x, (float)m->position.y};
                sf::Vector2f center = gearButton_.getPosition()
                                    + sf::Vector2f{gearButton_.getRadius(), gearButton_.getRadius()};
                if (hitCircle(mp, center, gearButton_.getRadius())) {
                    optionsOpen_ = !optionsOpen_; // toggle
                }
                // sliders: commence drag si options ouvertes
                if (optionsOpen_) {
                    auto grab = [&](Slider& s){
                        sf::FloatRect bar(s.pos, s.size);
                        if (bar.contains(mp)) { s.dragging = true; }
                    };
                    grab(sliderMusic_);
                    grab(sliderSfx_);
                }
            }
        }
        if (const auto* m = ev->getIf<sf::Event::MouseButtonReleased>()) {
            if (m->button == sf::Mouse::Button::Left) {
                sliderMusic_.dragging = false;
                sliderSfx_.dragging   = false;
            }
        }
        if (const auto* m = ev->getIf<sf::Event::MouseMoved>()) {
            if (optionsOpen_) {
                // si on drag, on met à jour la valeur 0..1
                auto drag = [&](Slider& s, float& outVal){
                    if (!s.dragging) return;
                    float x = (float)m->position.x;
                    float t = (x - s.pos.x) / s.size.x;
                    outVal = clamp01(t);
                };
                drag(sliderMusic_, musicVol01_);
                drag(sliderSfx_,   sfxVol01_);
            }
        }

        // --- Click sur les boutons rectangulaires
        if (const auto* m = ev->getIf<sf::Event::MouseButtonPressed>()) {
            if (m->button == sf::Mouse::Button::Left) {
                auto mp = win_.mapPixelToCoords(sf::Vector2i{m->position.x, m->position.y});
                for (auto& b : buttons_) {
                    sf::Vector2f scaled{ b.size.x * b.scale, b.size.y * b.scale };
                    sf::Vector2f topLeft{
                        b.pos.x + (b.size.x - scaled.x)*0.5f,
                        b.pos.y + (b.size.y - scaled.y)*0.5f
                    };
                    sf::FloatRect bounds(topLeft, scaled);
                    if (bounds.contains(mp)) {
                        if (b.id == "start")      { choice.start = true;      return choice; }
                        if (b.id == "difficulty") { choice.openDifficulty = true; return choice; }
                        if (b.id == "exit")       { choice.exit = true;       return choice; }
                    }
                }
            }
        }
    }

    // Hover/focus des boutons
    auto mouse = win_.mapPixelToCoords(sf::Mouse::getPosition(win_));
    updateHoverFocus(mouse);

    // MAJ pourcentages sliders
    if (pctMusic_) pctMusic_->setString(std::to_string((int)std::round(musicVol01_*100)) + "%");
    if (pctSfx_)   pctSfx_->setString  (std::to_string((int)std::round(sfxVol01_*100))   + "%");

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

    // Utiliser des PNG (SFML ne charge pas SVG, et .ico pas partout)
   /*  loadTexture(icoStart_, "assets/images/start.png");
    loadTexture(icoGear_,  "assets/images/gear.png");   // icône gear pour les boutons ET le bouton rond
    loadTexture(icoExit_,  "assets/images/exit.png");*/

    // Fond d'écran (cover)
    if (loadTexture(texBg_, "../assets/images/background.png")) {
        bg_ = std::make_unique<sf::Sprite>(texBg_);
    }

    // Fond du petit cadre (image)
    if (loadTexture(texCardBg_, "../assets/images/first-bg.png")) {
        cardBg_ = std::make_unique<sf::Sprite>(texCardBg_);
    }

    // Texture de bouton arrondi (optionnelle)
    loadTexture(texBtn_, "assets/images/btn.png");
}

// ---- Layout
void Menu::buildLayout() {
    // Titre principal
    title_ = std::make_unique<sf::Text>(font_, sf::String("Tower Defense"), 50u);
    title_->setFillColor(sf::Color(235, 245, 255));
    title_->setOutlineThickness(2.f);
    title_->setOutlineColor(sf::Color(20, 30, 50, 200));

    // Ombre du titre
    titleShadow_ = std::make_unique<sf::Text>(*title_);
    titleShadow_->setFillColor(sf::Color(0,0,0,1));
    titleShadow_->setOutlineThickness(0.f);

    // Boutons
    buttons_.clear();
    auto makeBtn = [&](const std::string& id, const std::string& text, const sf::Texture* icon) {
        MenuButton b;
        b.id   = id;
        b.size = {360.f, 56.f};

        b.label = std::make_unique<sf::Text>(font_, sf::String(text), 26u);
        const auto lb = b.label->getLocalBounds();
        b.label->setOrigin({lb.position.x + lb.size.x * 0.5f, lb.position.y + lb.size.y * 0.5f});
        b.label->setFillColor(sf::Color(235, 240, 250));
        b.label->setOutlineThickness(1.f);
        b.label->setOutlineColor(sf::Color(20, 30, 50, 160));

        if (icon && icon->getSize().x > 0) {
            b.icon = std::make_unique<sf::Sprite>(*icon);
            b.icon->setScale({0.6f, 0.6f});
        }
        return b;
    };

    buttons_.push_back(makeBtn("start",      "Start",      &icoStart_));
    buttons_.push_back(makeBtn("difficulty", "Difficulty", &icoGear_));
    buttons_.push_back(makeBtn("exit",       "Exit",       &icoExit_));

    // Bouton circulaire "Settings"
    gearButton_.setRadius(26.f);
    gearButton_.setFillColor(sf::Color(35,40,52));
    gearButton_.setOutlineThickness(0.f);

    if (icoGear_.getSize().x > 0) {
        gearIcon_ = std::make_unique<sf::Sprite>(icoGear_);
        gearIcon_->setScale(sf::Vector2f{0.7f, 0.7f});
    }

    // Thèmes couleurs par bouton
    for (auto& b : buttons_) {
        if (b.id == "start") {
            b.fillA       = sf::Color(255, 170,  90, 180);
            b.fillB       = sf::Color( 85, 200, 120, 180);
            b.border      = sf::Color(255, 185, 110, 180);

            b.hoverFillA  = sf::Color(255, 155,  70, 255);
            b.hoverFillB  = sf::Color( 70,  210, 140, 255);
            b.hoverBorder = sf::Color(255, 205, 130, 255);
        }
        else if (b.id == "difficulty") {
            b.fillA       = sf::Color(120, 170, 255, 180);
            b.fillB       = sf::Color(255, 170,  90, 180);
            b.border      = sf::Color(180, 210, 255, 190);

            b.hoverFillA  = sf::Color(140, 190, 255, 255);
            b.hoverFillB  = sf::Color(255, 150,  70, 255);
            b.hoverBorder = sf::Color(210, 230, 255, 255);
        }
        else if (b.id == "exit") {
            b.fillA       = sf::Color(220,  80,  80, 180);
            b.fillB       = sf::Color(255, 150,  70, 180);
            b.border      = sf::Color(255, 170, 120, 190);

            b.hoverFillA  = sf::Color(240,  70,  70, 255);
            b.hoverFillB  = sf::Color(255, 140,  60, 255);
            b.hoverBorder = sf::Color(255, 190, 140, 255);
        }
    }
}

void Menu::buildSettings() {
    optShadow_.setSize(optSize_ + sf::Vector2f{18.f, 22.f});
    optShadow_.setFillColor(sf::Color(0,0,0,100));

    optTitle_ = std::make_unique<sf::Text>(font_, sf::String("Options"), 28u);
    optTitle_->setFillColor(sf::Color(235,245,255));

    lblMusic_ = std::make_unique<sf::Text>(font_, sf::String("Music"), 20u);
    lblSfx_   = std::make_unique<sf::Text>(font_, sf::String("SFX"),   20u);
    for (auto* t : {lblMusic_.get(), lblSfx_.get()}) {
        t->setFillColor(sf::Color(215,220,230));
    }

    pctMusic_ = std::make_unique<sf::Text>(font_, sf::String("80%"), 18u);
    pctSfx_   = std::make_unique<sf::Text>(font_, sf::String("80%"), 18u);
    for (auto* t : {pctMusic_.get(), pctSfx_.get()}) {
        t->setFillColor(sf::Color(235,245,255));
    }

    // sliders – zone cliquable (barre)
    sliderMusic_.size = {280.f, 10.f};
    sliderSfx_.size   = {280.f, 10.f};
    sliderMusic_.value01 = musicVol01_;
    sliderSfx_.value01   = sfxVol01_;
}

void Menu::positionElements() {
    // Taille de la vue et centre logique de la fenêtre
    const sf::Vector2f viewSize = win_.getView().getSize();
    const sf::Vector2f center{ viewSize.x * 0.5f, viewSize.y * 0.5f };

    // --- Cadre central (card)
    cardPos_ = sf::Vector2f{ center.x - cardSize_.x * 0.5f,
                             center.y - cardSize_.y * 0.5f };

    // --- Fond "cover"
    if (bg_) {
        const auto tex = bg_->getTexture().getSize();
        const float sx = viewSize.x / static_cast<float>(tex.x);
        const float sy = viewSize.y / static_cast<float>(tex.y);
        const float s  = std::max(sx, sy);
        bg_->setScale(sf::Vector2f{ s, s });
        bg_->setPosition(sf::Vector2f{ 0.f, 0.f });
    }

    // --- Image du cadre
    if (cardBg_) {
        const sf::Vector2u texSz = texCardBg_.getSize();
        const sf::Vector2f scale{ 0.66f, 0.48f };
        cardBg_->setScale(scale);

        const sf::Vector2f scaledSize{
            static_cast<float>(texSz.x) * scale.x,
            static_cast<float>(texSz.y) * scale.y
        };
        const sf::Vector2f cardCenter = cardPos_ + cardSize_ * 0.5f;
        cardBg_->setPosition(cardCenter - scaledSize * 0.5f);
    }

    // --- Ombre sous le cadre
    dropShadow_.setSize(cardSize_ + sf::Vector2f{ 18.f, 24.f });
    dropShadow_.setPosition(cardPos_ + sf::Vector2f{ -9.f, 7.f });
    dropShadow_.setFillColor(sf::Color(0, 0, 0, 105));

    // --- Titre
    const sf::FloatRect tb = title_->getLocalBounds();
    const sf::Vector2f tpos{ tb.position.x, tb.position.y };
    const sf::Vector2f tsz { tb.size.x,     tb.size.y     };
    title_->setOrigin(sf::Vector2f{ tpos.x + tsz.x * 0.5f, tpos.y + tsz.y * 0.5f });
    title_->setPosition(sf::Vector2f{ center.x, cardPos_.y - 56.f });

    // --- Boutons rectangulaires
    const float gap = 20.f;
    const float top = cardPos_.y + 86.f;
    for (std::size_t i = 0; i < buttons_.size(); ++i) {
        auto& b = buttons_[i];

        b.size = sf::Vector2f{ 360.f, 56.f };
        const float bx = cardPos_.x + (cardSize_.x - b.size.x) * 0.5f;
        const float by = top + static_cast<float>(i) * (b.size.y + gap);
        b.pos = sf::Vector2f{ bx, by };

        // Label centré
        b.label->setPosition(sf::Vector2f{ bx + b.size.x * 0.5f, by + b.size.y * 0.5f });

        // Icône à gauche si présente
        if (b.hasIcon()) {
            const sf::Vector2u texSz = b.icon->getTexture().getSize();
            b.icon->setScale(sf::Vector2f{ 0.6f, 0.6f });
            const float scaledH = static_cast<float>(texSz.y) * b.icon->getScale().y;
            const float iy = by + (b.size.y - scaledH) * 0.5f;
            b.icon->setPosition(sf::Vector2f{ bx + 16.f, iy });

            // Décale légèrement le texte si icône
            b.label->setPosition(sf::Vector2f{ bx + b.size.x * 0.5f + 10.f, by + b.size.y * 0.5f });
        }
    }

    // --- Bouton "settings" (gear)
    const sf::Vector2f gearCenter{
        cardPos_.x + cardSize_.x - 26.f - 14.f,
        cardPos_.y - 34.f
    };
    gearButton_.setPosition(gearCenter - sf::Vector2f{ gearButton_.getRadius(), gearButton_.getRadius() });
    if (gearIcon_) {
        const sf::Vector2f gpos = gearButton_.getPosition();
        gearIcon_->setPosition(gpos + sf::Vector2f{ 6.f, 6.f });
    }

    // --- Panneau "Options"
    positionSettings();
}

// -- Hover/clavier : met à jour l'état des boutons
void Menu::updateHoverFocus(const sf::Vector2f& mouse) {
    for (std::size_t i = 0; i < buttons_.size(); ++i) {
        auto& b = buttons_[i];

        const sf::FloatRect rect(b.pos, b.size);
        b.hovered = rect.contains(mouse);
        b.focused = (static_cast<int>(i) == focusIndex_);

        const bool hot = b.hovered || b.focused;

        b.targetScale = hot ? 1.06f : 1.0f;
        b.scale += (b.targetScale - b.scale) * 0.25f;

        const float targetHover = hot ? 1.f : 0.f;
        b.hover += (targetHover - b.hover) * 0.30f;
        if (b.hover < 0.f) b.hover = 0.f;
        if (b.hover > 1.f) b.hover = 1.f;

        b.label->setFillColor(hot ? sf::Color(245, 248, 255)
                                  : sf::Color(235, 240, 250));
    }
}

void Menu::positionSettings() {
    const sf::Vector2f center = cardPos_ + cardSize_ * 0.5f;
    optPos_  = center - optSize_ * 0.5f - sf::Vector2f{0.f, cardSize_.y*0.10f};
    optShadow_.setPosition(optPos_ + sf::Vector2f{-9.f, 7.f});

    // Titre
    if (optTitle_) {
        const auto tb = optTitle_->getLocalBounds();
        optTitle_->setOrigin(sf::Vector2f{tb.position.x + tb.size.x * 0.5f,
                                          tb.position.y + tb.size.y * 0.5f});
        optTitle_->setPosition(sf::Vector2f{optPos_.x + optSize_.x * 0.5f,
                                            optPos_.y + 28.f});
    }

    // Références horizontales
    const float left   = optPos_.x + 36.f;
    const float right  = optPos_.x + optSize_.x - 36.f;
    const float y1     = optPos_.y + 70.f;
    const float y2     = y1 + 56.f;

    if (lblMusic_) lblMusic_->setPosition(sf::Vector2f{left,  y1 - 14.f});
    if (lblSfx_)   lblSfx_->setPosition  (sf::Vector2f{left,  y2 - 14.f});

    const int pctMusic = static_cast<int>(std::round(musicVol01_ * 100.f));
    const int pctSfx   = static_cast<int>(std::round(sfxVol01_   * 100.f));

    if (pctMusic_) {
        pctMusic_->setString(std::to_string(pctMusic) + "%");
        const auto pb = pctMusic_->getLocalBounds();
        pctMusic_->setOrigin(sf::Vector2f{pb.position.x + pb.size.x, pb.position.y});
        pctMusic_->setPosition(sf::Vector2f{right, y1 - 20.f});
    }
    if (pctSfx_) {
        pctSfx_->setString(std::to_string(pctSfx) + "%");
        const auto pb = pctSfx_->getLocalBounds();
        pctSfx_->setOrigin(sf::Vector2f{pb.position.x + pb.size.x, pb.position.y});
        pctSfx_->setPosition(sf::Vector2f{right, y2 - 20.f});
    }

    sliderMusic_.pos     = sf::Vector2f{left, y1};
    sliderSfx_.pos       = sf::Vector2f{left, y2};
    sliderMusic_.size.x  = std::max(140.f, optSize_.x - 36.f - 36.f - 70.f);
    sliderSfx_.size.x    = sliderMusic_.size.x;

    sliderMusic_.size.y  = (sliderMusic_.size.y > 0.f) ? sliderMusic_.size.y : 10.f;
    sliderSfx_.size.y    = (sliderSfx_.size.y   > 0.f) ? sliderSfx_.size.y   : 10.f;

    musicVol01_ = clamp01(musicVol01_);
    sfxVol01_   = clamp01(sfxVol01_);
}

void Menu::draw() {
    // Fond
    if (bg_) win_.draw(*bg_);

    // Ombre douce large
    win_.draw(dropShadow_);

    // 1) Image de fond du cadre si disponible
    if (cardBg_) win_.draw(*cardBg_);

    // 2) Overlay shader (ombre/glow + remplissage optionnel)
    if (shaderOk_) {
        sf::RectangleShape panel(cardSize_);
        panel.setPosition(cardPos_);

        panelShader_.setUniform("u_pos",       sf::Glsl::Vec2{cardPos_.x, cardPos_.y});
        panelShader_.setUniform("u_size",      sf::Glsl::Vec2{cardSize_.x, cardSize_.y});
        panelShader_.setUniform("u_radius",    cornerRadius_);
        panelShader_.setUniform("u_innerA",    sf::Glsl::Vec4{0.13f, 0.15f, 0.22f, 0.98f});
        panelShader_.setUniform("u_innerB",    sf::Glsl::Vec4{0.09f, 0.11f, 0.17f, 0.98f});
        panelShader_.setUniform("u_shadowCol", sf::Glsl::Vec4{0.0f, 0.0f, 0.0f, 0.55f});
        panelShader_.setUniform("u_borderCol", sf::Glsl::Vec4{0.40f, 0.60f, 1.0f, 0.70f});
        panelShader_.setUniform("u_time",      animTime_);

        float fillAlpha = cardBg_ ? 0.f : 1.f;
        panelShader_.setUniform("u_fillAlpha", fillAlpha);

        sf::RenderStates rs; rs.shader = &panelShader_;
        win_.draw(panel, rs);
    }

    // Titre (avec légère pulsation/ombre)
    titlePulseT_ += 0.016f;
    float tPulse = 1.f + 0.02f * std::sin(titlePulseT_ * 2.2f);

    sf::RectangleShape soft(cardSize_ + sf::Vector2f{36.f, 42.f});
    soft.setPosition(cardPos_ + sf::Vector2f{-18.f, 12.f});
    soft.setFillColor(sf::Color(0,0,0,48));
    soft.setScale({1.f, 0.95f});
    win_.draw(soft);

    titleShadow_->setPosition(title_->getPosition() + sf::Vector2f{0.f, 2.f});
    titleShadow_->setScale({tPulse, tPulse});
    win_.draw(*titleShadow_);

    title_->setScale({tPulse, tPulse});
    win_.draw(*title_);

    // Boutons (shader + texte + icône)
    for (auto& b : buttons_) {
        sf::Vector2f scaledSize{ b.size.x * b.scale, b.size.y * b.scale };
        sf::Vector2f topLeft{
            b.pos.x + (b.size.x - scaledSize.x) * 0.5f,
            b.pos.y + (b.size.y - scaledSize.y) * 0.5f
        };

        sf::RectangleShape quad(scaledSize);
        quad.setPosition(topLeft);

        if (btnShaderOk_) {
            auto lerp = [](float a, float b, float t){ return a + (b - a) * t; };
            auto lerpColor = [&](sf::Color A, sf::Color B, float t){
                auto L = [&](std::uint8_t x, std::uint8_t y){
                    float xf = static_cast<float>(x);
                    float yf = static_cast<float>(y);
                    return static_cast<std::uint8_t>(std::lround(lerp(xf, yf, t)));
                };
                return sf::Color(L(A.r, B.r), L(A.g, B.g), L(A.b, B.b), L(A.a, B.a));
            };
            auto toVec4 = [](sf::Color c){
                return sf::Glsl::Vec4{ c.r/255.f, c.g/255.f, c.b/255.f, c.a/255.f };
            };

            sf::Color curA      = lerpColor(b.fillA,      b.hoverFillA,   b.hover);
            sf::Color curB      = lerpColor(b.fillB,      b.hoverFillB,   b.hover);
            sf::Color curBorder = lerpColor(b.border,     b.hoverBorder,  b.hover);

            float h = std::clamp(b.hover, 0.f, 1.f);
            float smoothHover = std::clamp((h - 0.06f) / 0.94f, 0.f, 1.f);

            buttonShader_.setUniform("u_hover",     smoothHover);
            buttonShader_.setUniform("u_pos",       sf::Glsl::Vec2{topLeft.x, topLeft.y});
            buttonShader_.setUniform("u_size",      sf::Glsl::Vec2{scaledSize.x, scaledSize.y});
            buttonShader_.setUniform("u_radius",    btnRadius_);
            buttonShader_.setUniform("u_fillA",     toVec4(curA));
            buttonShader_.setUniform("u_fillB",     toVec4(curB));
            buttonShader_.setUniform("u_borderCol", toVec4(curBorder));
            buttonShader_.setUniform("u_time",      animTime_);

            sf::RenderStates rs; rs.shader = &buttonShader_;
            win_.draw(quad, rs);
        } else {
            quad.setFillColor(b.hover > 0.5f ? b.hoverFillA : b.fillA);
            win_.draw(quad);
        }

        if (b.hasIcon()) {
            auto texSz   = b.icon->getTexture().getSize();
            float sY     = b.icon->getScale().y;
            float scaledH= static_cast<float>(texSz.y) * sY;
            float iy     = topLeft.y + (scaledSize.y - scaledH) * 0.5f;
            b.icon->setPosition({ topLeft.x + 16.f, iy });
            win_.draw(*b.icon);
        }

        float centerX = topLeft.x + scaledSize.x * 0.5f;
        float centerY = topLeft.y + scaledSize.y * 0.5f;
        float iconTextOffset = b.hasIcon() ? 10.f : 0.f;
        b.label->setPosition({ centerX + iconTextOffset, centerY });
        win_.draw(*b.label);
    }

    // Bouton gear (hover = léger zoom)
    gearButton_.setScale(gearHover_ ? sf::Vector2f{1.06f, 1.06f} : sf::Vector2f{1.f, 1.f});
    win_.draw(gearButton_);
    if (gearIcon_) win_.draw(*gearIcon_);

    // Panneau Options
    if (optionsOpen_) drawSettings();
}

void Menu::drawSettings() {
    // Ombre
    win_.draw(optShadow_);

    // Cadre simple (tu peux le remplacer par le shader panel si tu veux)
    sf::RectangleShape panel(optSize_);
    panel.setPosition(optPos_);
    panel.setFillColor(sf::Color(32,36,48,240));
    panel.setOutlineThickness(0.f);
    win_.draw(panel);

    // Titre + libellés
    if (optTitle_)  win_.draw(*optTitle_);
    if (lblMusic_)  win_.draw(*lblMusic_);
    if (lblSfx_)    win_.draw(*lblSfx_);
    if (pctMusic_)  win_.draw(*pctMusic_);
    if (pctSfx_)    win_.draw(*pctSfx_);

    // Sliders
    auto drawSlider = [&](Slider& s, float val01){
        sf::RectangleShape bar(s.size);
        bar.setPosition(s.pos);
        bar.setFillColor(sf::Color(60,66,82));
        win_.draw(bar);

        sf::RectangleShape fill({ s.size.x * clamp01(val01), s.size.y });
        fill.setPosition(s.pos);
        fill.setFillColor(sf::Color(90,160,255));
        win_.draw(fill);

        float x = s.pos.x + s.size.x * clamp01(val01);
        float y = s.pos.y + s.size.y * 0.5f;
        sf::CircleShape handle(8.f);
        handle.setOrigin({8.f,8.f});
        handle.setPosition({x,y});
        handle.setFillColor(sf::Color(240,245,255));
        win_.draw(handle);
    };

    drawSlider(sliderMusic_, musicVol01_);
    drawSlider(sliderSfx_,   sfxVol01_);
}

// ---- Utils
bool Menu::loadFont(const std::string& path)  { return font_.openFromFile(path); }
bool Menu::loadTexture(sf::Texture& t, const std::string& path) {
    t.setSmooth(true);
    return t.loadFromFile(path);
}

bool Menu::hitCircle(const sf::Vector2f& p, const sf::Vector2f& c, float r) const {
    const float dx = p.x - c.x;
    const float dy = p.y - c.y;
    return dx * dx + dy * dy <= r * r;
}
