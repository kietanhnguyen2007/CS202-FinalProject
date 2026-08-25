#include "View/ResultView.h"
#include "View/AssetManager.h"
#include "View/Renderer.h"
#include "View/UIResourceManager.h"
#include "Systems/SoundManager.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace View {

namespace {
float Clamp01(float value) { return std::clamp(value, 0.0f, 1.0f); }

float EaseOutCubic(float t) {
    t = Clamp01(t);
    const float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
}

float EaseOutBack(float t) {
    t = Clamp01(t);
    constexpr float c1 = 1.70158f;
    constexpr float c3 = c1 + 1.0f;
    const float x = t - 1.0f;
    return 1.0f + c3*x*x*x + c1*x*x;
}

void DrawPanel(Texture2D texture, Rectangle rect, float border = 12.0f, Color tint = WHITE) {
    if (!texture.id) return;
    NPatchInfo patch{{0, 0, (float)texture.width, (float)texture.height},
                     (int)border, (int)border, (int)border, (int)border,
                     NPATCH_NINE_PATCH};
    ::DrawTextureNPatch(texture, patch, rect, {0,0}, 0.0f, tint);
}

void DrawCentered(Font font, const char* text, float centerX, float y,
                  float fontSize, float spacing, Color color) {
    Vector2 measured = ::MeasureTextEx(font, text, fontSize, spacing);
    ::DrawTextEx(font, text, {centerX - measured.x * 0.5f, y}, fontSize, spacing, color);
}

float FitFontSize(Font font, const char* text, float preferredSize,
                  float minSize, float maxWidth, float spacing = 1.0f) {
    float size = preferredSize;
    while (size > minSize && ::MeasureTextEx(font, text, size, spacing).x > maxWidth)
        size -= 1.0f;
    return std::max(size, minSize);
}

Rectangle VictoryPanelRect() {
    const float w = (float)::GetScreenWidth();
    const float h = (float)::GetScreenHeight();
    const float scale = std::clamp(std::min(w / 1280.0f, h / 720.0f), 0.70f, 1.35f);
    const float panelW = std::min(w * 0.78f, 930.0f * scale);
    const float panelH = std::min(h * 0.88f, 630.0f * scale);
    return {(w-panelW)*0.5f, (h-panelH)*0.5f, panelW, panelH};
}

float VictoryContentScale(Rectangle panel) {
    return std::min(panel.width / 930.0f, panel.height / 630.0f);
}

Color WithAlpha(Color color, float alpha) {
    color.a = (unsigned char)std::clamp(alpha, 0.0f, 255.0f);
    return color;
}
} // namespace

ResultView& ResultView::GetInstance() {
    static ResultView instance;
    return instance;
}

bool ResultView::Init() {
    if (m_loaded) return true;
    m_panel = ::LoadTexture("assets/ui/kenney_rpg/panel_brown.png");
    m_panelInset = ::LoadTexture("assets/ui/kenney_rpg/panelInset_brown.png");
    m_roundButton = ::LoadTexture("assets/ui/kenney_rpg/buttonRound_brown.png");
    m_medal = ::LoadTexture("assets/ui/victory/new_record_medal.png");
    m_lightRing = ::LoadTexture("assets/ui/victory/light_ring.png");
    m_sparkle = ::LoadTexture("assets/ui/victory/sparkle.png");
    m_starBurst = ::LoadTexture("assets/ui/victory/star_burst.png");
    m_iconTrophy = ::LoadTexture("assets/ui/victory/icon_trophy.png");
    m_iconTarget = ::LoadTexture("assets/ui/victory/icon_target.png");
    m_iconStar = ::LoadTexture("assets/ui/victory/icon_star.png");
    m_trophy = ::LoadTexture("assets/textures/tutorial/golden_trophy.png");
    // Keep the result screen visually consistent with the menu/loading UI.
    m_font = ::LoadFontEx("assets/fonts/game_font.ttf", 48, nullptr, 0);

    Texture2D* textures[] = {&m_panel, &m_panelInset, &m_roundButton, &m_medal,
        &m_lightRing, &m_sparkle, &m_starBurst, &m_iconTrophy, &m_iconTarget,
        &m_iconStar, &m_trophy};
    for (Texture2D* texture : textures) {
        if (texture->id) ::SetTextureFilter(*texture, TEXTURE_FILTER_BILINEAR);
    }
    if (m_font.texture.id) ::SetTextureFilter(m_font.texture, TEXTURE_FILTER_BILINEAR);
    m_particles.reserve(180);
    m_loaded = m_panel.id != 0 && m_font.texture.id != 0;
    return m_loaded;
}

bool ResultView::LoadResources(const std::string&) { return Init(); }

void ResultView::Shutdown() {
    if (!m_loaded) return;
    Texture2D* textures[] = {&m_panel, &m_panelInset, &m_roundButton, &m_medal,
        &m_lightRing, &m_sparkle, &m_starBurst, &m_iconTrophy, &m_iconTarget,
        &m_iconStar, &m_trophy};
    for (Texture2D* texture : textures) {
        if (texture->id) ::UnloadTexture(*texture);
        *texture = {};
    }
    if (m_font.texture.id) ::UnloadFont(m_font);
    m_font = {};
    m_avatarAnimator.ClearClips();
    m_avatarAtlas.reset();
    m_particles.clear();
    m_loaded = false;
    Dismiss();
}

void ResultView::LoadAvatar(CharacterClass cls) {
    const char* folder = "knight";
    switch (cls) {
        case CharacterClass::Fighter: folder = "fighter"; break;
        case CharacterClass::Knight: folder = "knight"; break;
        case CharacterClass::Ninja: folder = "ninja"; break;
        case CharacterClass::MagicCaster: folder = "magic_caster"; break;
    }
    const std::string path = std::string("assets/textures/player/") + folder + "_v2/idle_v2.json";
    auto atlas = AssetManager::GetInstance().GetAtlas(path);
    if (!atlas || !atlas->IsTextureLoaded() || !atlas->HasClip("idle")) return;
    m_avatarAnimator.ClearClips();
    m_avatarAnimator.LoadClipsFromAtlas(*atlas);
    m_avatarAnimator.Play("idle");
    m_avatarAtlas = std::move(atlas);
}

void ResultView::ResetCelebration() {
    m_elapsed = 0.0f;
    m_confettiTimer = 0.0f;
    m_action = ResultAction::None;
    m_starSoundPlayed = {false, false, false};
    m_particles.clear();
    SpawnConfetti(72);
}

void ResultView::Show(const LevelResultSnapshot& snap) {
    if (!m_loaded) Init();
    m_snap = snap;
    m_visible = true;
    m_gameOver = false;
    ResetCelebration();
    LoadAvatar(snap.characterClass);
    SoundManager::GetInstance().PlaySound("victory_reveal");
}

void ResultView::ShowGameOver(const LevelResultSnapshot& snap) {
    if (!m_loaded) Init();
    m_snap = snap;
    m_visible = true;
    m_gameOver = true;
    ResetCelebration();
    LoadAvatar(snap.characterClass);
}

void ResultView::Dismiss() {
    m_visible = false;
    m_gameOver = false;
    m_elapsed = 0.0f;
    m_action = ResultAction::None;
    m_particles.clear();
}

void ResultView::SpawnConfetti(int count) {
    const float w = (float)::GetScreenWidth();
    const float h = (float)::GetScreenHeight();
    static const Color palette[] = {
        Color{255,205,72,255}, Color{255,108,91,255}, Color{100,210,255,255},
        Color{154,233,126,255}, Color{203,139,255,255}, Color{255,244,185,255}
    };
    for (int i = 0; i < count; ++i) {
        CelebrationParticle particle;
        particle.position = {
            (float)(std::rand() % std::max(1, (int)w)),
            (float)(-(std::rand() % std::max(1, (int)(h * 0.75f))))
        };
        particle.velocity = {(float)(std::rand()%121 - 60), 85.0f + (float)(std::rand()%120)};
        particle.lifetime = 3.5f + (float)(std::rand()%160) / 100.0f;
        particle.size = 5.0f + (float)(std::rand()%8);
        particle.rotation = (float)(std::rand()%360);
        particle.rotationSpeed = (float)(std::rand()%241 - 120);
        particle.color = palette[std::rand() % (sizeof(palette)/sizeof(palette[0]))];
        particle.shape = std::rand() % 3;
        m_particles.push_back(particle);
    }
}

void ResultView::UpdateParticles(float dt) {
    for (CelebrationParticle& particle : m_particles) {
        particle.age += dt;
        particle.position.x += particle.velocity.x * dt;
        particle.position.y += particle.velocity.y * dt;
        particle.velocity.y += 28.0f * dt;
        particle.rotation += particle.rotationSpeed * dt;
    }
    m_particles.erase(std::remove_if(m_particles.begin(), m_particles.end(),
        [](const CelebrationParticle& particle) { return particle.age >= particle.lifetime; }),
        m_particles.end());
}

Rectangle ResultView::ContinueButtonRect() const {
    Rectangle panel = VictoryPanelRect();
    const float scale = VictoryContentScale(panel);
    return {panel.x + panel.width*0.5f + 10.0f*scale,
            panel.y + panel.height - 66.0f*scale,
            190.0f*scale, 46.0f*scale};
}

Rectangle ResultView::RetryButtonRect() const {
    Rectangle panel = VictoryPanelRect();
    const float scale = VictoryContentScale(panel);
    return {panel.x + panel.width*0.5f - 200.0f*scale,
            panel.y + panel.height - 66.0f*scale,
            180.0f*scale, 46.0f*scale};
}

void ResultView::Update(float dt) {
    if (!m_visible) return;
    m_elapsed += dt;
    m_avatarAnimator.Update(dt);
    UpdateParticles(dt);

    if (!m_gameOver && m_elapsed > 0.55f) {
        m_confettiTimer += dt;
        if (m_confettiTimer >= 0.10f) {
            m_confettiTimer = 0.0f;
            SpawnConfetti(3);
        }
    }

    for (int i = 0; i < m_snap.stars && i < 3; ++i) {
        const float revealAt = 0.75f + i * 0.34f;
        if (!m_starSoundPlayed[i] && m_elapsed >= revealAt) {
            m_starSoundPlayed[i] = true;
            SoundManager::GetInstance().PlaySound("victory_star");
        }
    }

    const float readyAt = m_gameOver ? 0.8f : 2.15f;
    if (m_elapsed < readyAt || m_action != ResultAction::None) return;
    const Vector2 mouse = ::GetMousePosition();
    const bool click = ::IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    if (::IsKeyPressed(KEY_R) || (click && ::CheckCollisionPointRec(mouse, RetryButtonRect()))) {
        m_action = ResultAction::Retry;
    } else if (::IsKeyPressed(KEY_ESCAPE)) {
        m_action = ResultAction::LevelSelect;
    } else if (::IsKeyPressed(KEY_ENTER) || ::IsKeyPressed(KEY_SPACE) ||
               (click && ::CheckCollisionPointRec(mouse, ContinueButtonRect()))) {
        m_action = ResultAction::Continue;
    }
    if (m_action != ResultAction::None) {
        SoundManager::GetInstance().PlaySound("victory_confirm");
    }
}

ResultAction ResultView::ConsumeAction() {
    const ResultAction action = m_action;
    m_action = ResultAction::None;
    return action;
}

void ResultView::Render() {
    if (!m_visible || !m_loaded) return;
    Renderer::GetInstance().EndFrameAndFlush();
    const float w = (float)::GetScreenWidth();
    const float h = (float)::GetScreenHeight();
    const Rectangle panelTarget = VictoryPanelRect();
    // Scale all content against both axes so short/wide windows cannot make
    // vertical rows collide even when there is ample horizontal space.
    const float scale = VictoryContentScale(panelTarget);
    const float panelProgress = EaseOutBack(m_elapsed / 0.48f);
    const float panelW = panelTarget.width * panelProgress;
    const float panelH = panelTarget.height * panelProgress;
    Rectangle panel{panelTarget.x + (panelTarget.width-panelW)*0.5f,
                    panelTarget.y + (panelTarget.height-panelH)*0.5f,
                    panelW, panelH};

    ::DrawRectangleGradientV(0, 0, (int)w, (int)h,
        Color{18,9,30,215}, Color{4,3,10,238});

    // Rotating golden rays behind the panel.
    const Vector2 rayCenter{w*0.5f, h*0.42f};
    const float rayRotation = m_elapsed * 8.0f;
    for (int i = 0; i < 18; ++i) {
        const float a = (rayRotation + i*20.0f) * DEG2RAD;
        const float b = a + 6.0f*DEG2RAD;
        Vector2 p1{rayCenter.x + std::cos(a)*h*0.62f, rayCenter.y + std::sin(a)*h*0.62f};
        Vector2 p2{rayCenter.x + std::cos(b)*h*0.62f, rayCenter.y + std::sin(b)*h*0.62f};
        ::DrawTriangle(rayCenter, p1, p2, Color{255,190,55,18});
    }
    if (m_lightRing.id) {
        const float ring = std::min(w,h) * 0.72f;
        ::BeginBlendMode(BLEND_ADDITIVE);
        ::DrawTexturePro(m_lightRing, {0,0,(float)m_lightRing.width,(float)m_lightRing.height},
                         {rayCenter.x, rayCenter.y, ring, ring}, {ring*0.5f,ring*0.5f},
                         -m_elapsed*9.0f, Color{255,195,75,65});
        ::EndBlendMode();
    }

    // Confetti continues across the whole result screen.
    for (const CelebrationParticle& particle : m_particles) {
        const float alpha = 255.0f * Clamp01(1.0f - particle.age / particle.lifetime);
        Color color = WithAlpha(particle.color, alpha);
        if (particle.shape == 0) {
            Rectangle piece{particle.position.x, particle.position.y,
                            particle.size*0.55f, particle.size*1.35f};
            ::DrawRectanglePro(piece, {piece.width*0.5f,piece.height*0.5f}, particle.rotation, color);
        } else if (particle.shape == 1) {
            ::DrawCircleV(particle.position, particle.size*0.42f, color);
        } else {
            ::DrawPoly(particle.position, 3, particle.size*0.65f, particle.rotation, color);
        }
    }

    if (panelProgress <= 0.01f) return;
    ::DrawRectangleRounded({panel.x+10*scale,panel.y+14*scale,panel.width,panel.height},
                           0.06f, 12, Color{5,2,1,160});
    DrawPanel(m_panel, panel, 14.0f);

    Rectangle header{panel.x + 105*scale, panel.y + 20*scale,
                     panel.width - 210*scale, 72*scale};
    DrawPanel(m_panel, header, 13.0f, m_gameOver ? Color{190,105,105,255} : WHITE);
    const float titleProgress = EaseOutBack((m_elapsed - 0.25f) / 0.40f);
    const char* title = m_gameOver ? "DEFEAT" : "VICTORY";
    const float fittedTitleSize = FitFontSize(m_font, title, 50.0f*scale,
                                               22.0f*scale,
                                               header.width-28.0f*scale, 2.0f);
    const float titleSize = fittedTitleSize*std::max(0.0f,titleProgress);
    if (titleSize > 1.0f) {
        DrawCentered(m_font, title, panel.x+panel.width*0.5f,
                     header.y + 7*scale, titleSize, 2.0f,
                     m_gameOver ? Color{255,172,163,255} : Color{255,236,159,255});
    }

    // Animated hero and trophy flank the star ceremony.
    if (!m_gameOver && m_avatarAnimator.HasTexture()) {
        Texture2D* texture = m_avatarAnimator.GetCurrentTexture();
        Rectangle src = m_avatarAnimator.GetCurrentSrcRect();
        const float heroH = 125.0f*scale;
        const float heroW = heroH * std::abs(src.width) / std::max(1.0f,std::abs(src.height));
        const float bob = std::sin(m_elapsed*3.0f)*4.0f*scale;
        ::DrawTexturePro(*texture, src,
                         {panel.x+42*scale, panel.y+103*scale+bob, heroW, heroH},
                         {0,0}, 0.0f, WHITE);
    }
    if (!m_gameOver && m_trophy.id) {
        const float trophySize = 112.0f*scale;
        const float bob = std::sin(m_elapsed*2.5f+1.0f)*6.0f*scale;
        ::DrawTexturePro(m_trophy, {0,0,(float)m_trophy.width,(float)m_trophy.height},
                         {panel.x+panel.width-145*scale,panel.y+105*scale+bob,
                          trophySize,trophySize}, {0,0}, 0.0f, WHITE);
    }

    Texture2D* starTexture = UIResourceManager::GetInstance().GetStarIcon();
    const float starSize = 86.0f*scale;
    const float starGap = 22.0f*scale;
    const float starRowW = starSize*3.0f + starGap*2.0f;
    const float starStartX = panel.x + (panel.width-starRowW)*0.5f;
    const float starY = panel.y + 105.0f*scale;
    if (!m_gameOver && starTexture && starTexture->id) {
        for (int i = 0; i < 3; ++i) {
            const float centerX = starStartX + i*(starSize+starGap) + starSize*0.5f;
            const float centerY = starY + starSize*0.5f;
            Rectangle src{0,0,(float)starTexture->width,(float)starTexture->height};
            ::DrawTexturePro(*starTexture, src,
                             {centerX,centerY,starSize,starSize},
                             {starSize*0.5f,starSize*0.5f}, 0.0f,
                             Color{55,44,55,210});
            if (i >= m_snap.stars) continue;
            const float revealAt = 0.75f + i*0.34f;
            const float reveal = EaseOutBack((m_elapsed-revealAt)/0.32f);
            if (reveal <= 0.0f) continue;
            const float size = starSize*reveal;
            if (m_starBurst.id) {
                const float burst = starSize*1.9f*reveal;
                ::BeginBlendMode(BLEND_ADDITIVE);
                ::DrawTexturePro(m_starBurst,{0,0,(float)m_starBurst.width,(float)m_starBurst.height},
                                 {centerX,centerY,burst,burst},{burst*0.5f,burst*0.5f},
                                 m_elapsed*35.0f,Color{255,205,78,150});
                ::EndBlendMode();
            }
            ::DrawTexturePro(*starTexture, src, {centerX,centerY,size,size},
                             {size*0.5f,size*0.5f}, (1.0f-Clamp01(reveal))*-25.0f, WHITE);
        }
    }

    const float statsReveal = EaseOutCubic((m_elapsed-1.55f)/0.55f);
    Rectangle stats{panel.x+105*scale,panel.y+205*scale,
                    panel.width-210*scale,270*scale};
    if (m_gameOver) stats.y = panel.y+135*scale;
    if (statsReveal > 0.0f || m_gameOver) {
        DrawPanel(m_panelInset, stats, 14.0f, Color{250,226,176,255});
        const float count = m_gameOver ? 1.0f : statsReveal;
        const float labelSize = 18.0f*scale;
        const float valueSize = 21.0f*scale;
        const float leftX = stats.x+62*scale;
        const float rightX = stats.x+stats.width*0.55f;
        const float row1 = stats.y+29*scale;
        const float row2 = stats.y+94*scale;
        const float valueOffset = 26.0f*scale;
        auto drawIcon = [&](Texture2D tex, float x, float y, Color tint) {
            if (!tex.id) return;
            ::DrawTexturePro(tex,{0,0,(float)tex.width,(float)tex.height},
                             {x,y,30*scale,30*scale},{0,0},0,tint);
        };
        drawIcon(m_iconTrophy, stats.x+22*scale,row1-2*scale,Color{255,210,91,255});
        drawIcon(m_iconTarget, stats.x+stats.width*0.50f,row1-2*scale,Color{255,184,105,255});
        drawIcon(m_iconStar, stats.x+22*scale,row2-2*scale,Color{119,215,255,255});
        drawIcon(m_iconStar, stats.x+stats.width*0.50f,row2-2*scale,Color{255,232,122,255});

        char value[96];
        ::DrawTextEx(m_font,"TIME",{leftX,row1},labelSize,1,Color{84,48,25,255});
        std::snprintf(value,sizeof(value),"%.1fs / %.0fs",m_snap.clearTime*count,m_snap.parTime);
        ::DrawTextEx(m_font,value,{leftX,row1+valueOffset},valueSize,1,Color{54,31,20,255});
        ::DrawTextEx(m_font,"ENEMIES",{rightX,row1},labelSize,1,Color{84,48,25,255});
        std::snprintf(value,sizeof(value),"%d / %d",(int)std::round(m_snap.enemiesKilled*count),m_snap.totalEnemies);
        ::DrawTextEx(m_font,value,{rightX,row1+valueOffset},valueSize,1,Color{54,31,20,255});
        ::DrawTextEx(m_font,"PICKUPS",{leftX,row2},labelSize,1,Color{84,48,25,255});
        std::snprintf(value,sizeof(value),"%d / %d",(int)std::round(m_snap.itemsCollected*count),m_snap.totalItems);
        ::DrawTextEx(m_font,value,{leftX,row2+valueOffset},valueSize,1,Color{54,31,20,255});
        ::DrawTextEx(m_font,"SCORE",{rightX,row2},labelSize,1,Color{84,48,25,255});
        std::snprintf(value,sizeof(value),"%d",(int)std::round(m_snap.score*count));
        ::DrawTextEx(m_font,value,{rightX,row2+valueOffset},valueSize,1,Color{54,31,20,255});

        const float barX = stats.x+36*scale;
        const float barY = stats.y+174*scale;
        const float barW = stats.width-72*scale;
        const float barH = 30*scale;
        ::DrawRectangleRounded({barX,barY,barW,barH},0.45f,10,Color{83,55,44,220});
        ::DrawRectangleRounded({barX+3*scale,barY+3*scale,
                               (barW-6*scale)*m_snap.performance*count,barH-6*scale},
                               0.45f,10,Color{235,169,54,255});
        std::snprintf(value,sizeof(value),"PERFORMANCE  %d%%",
                      (int)std::round(m_snap.performance*100.0f*count));
        const float performanceSize = FitFontSize(m_font,value,19*scale,11*scale,
                                                   barW-20*scale);
        DrawCentered(m_font,value,stats.x+stats.width*0.5f,barY+3*scale,
                     performanceSize,1,WHITE);
        const char* rule = "1 STAR: CLEAR   |   2 STARS: 60%   |   3 STARS: 85%";
        const float ruleSize = FitFontSize(m_font,rule,16*scale,9*scale,
                                           stats.width-44*scale);
        DrawCentered(m_font,rule,stats.x+stats.width*0.5f,stats.y+218*scale,
                     ruleSize,1,Color{80,48,29,255});
    }

    const bool anyRecord = m_snap.newHighScore || m_snap.newBestStars || m_snap.newBestTime;
    if (!m_gameOver && anyRecord && m_medal.id) {
        const float medalReveal = EaseOutBack((m_elapsed-1.90f)/0.42f);
        if (medalReveal > 0.0f) {
            const float medalH = 92*scale*medalReveal;
            const float medalW = medalH*(float)m_medal.width/std::max(1,m_medal.height);
            ::DrawTexturePro(m_medal,{0,0,(float)m_medal.width,(float)m_medal.height},
                             {panel.x+panel.width-78*scale,panel.y+24*scale,medalW,medalH},
                             {medalW*0.5f,0},0,WHITE);
            const float recordSize = FitFontSize(m_font,"NEW RECORD",14*scale,9*scale,
                                                  128*scale);
            DrawCentered(m_font,"NEW RECORD",panel.x+panel.width-78*scale,
                         panel.y+92*scale,recordSize,1,Color{255,239,177,255});
        }
    }

    const float buttonsReveal = EaseOutCubic((m_elapsed-(m_gameOver?0.55f:2.05f))/0.35f);
    if (buttonsReveal > 0.0f) {
        const Vector2 mouse = ::GetMousePosition();
        auto drawButton = [&](Rectangle rect, const char* label) {
            const bool hover = ::CheckCollisionPointRec(mouse,rect);
            ::DrawRectangleRounded({rect.x+3*scale,rect.y+4*scale,rect.width,rect.height},
                                   0.22f,8,Color{20,9,4,135});
            DrawPanel(m_panel,rect,12.0f,hover?Color{255,229,169,255}:WHITE);
            const float buttonSize = FitFontSize(m_font,label,20*scale,10*scale,
                                                  rect.width-18*scale);
            const Vector2 buttonText = ::MeasureTextEx(m_font,label,buttonSize,1.0f);
            ::DrawTextEx(m_font,label,
                         {rect.x+(rect.width-buttonText.x)*0.5f,
                          rect.y+(rect.height-buttonText.y)*0.5f},
                         buttonSize,1.0f,Color{255,242,205,255});
        };
        drawButton(RetryButtonRect(),"[R] RETRY");
        drawButton(ContinueButtonRect(),"[ENTER] CONTINUE");
        const float footerSize = FitFontSize(m_font,"ESC - LEVEL SELECT",13*scale,
                                              9*scale,panel.width-40*scale);
        DrawCentered(m_font,"ESC - LEVEL SELECT",panel.x+panel.width*0.5f,
                     panel.y+panel.height-17*scale,footerSize,1,Color{238,211,159,220});
    }
}

} // namespace View
