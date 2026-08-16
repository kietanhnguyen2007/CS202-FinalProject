#include "View/HUDView.h"
#include "View/Renderer.h"
#include "Model/Boss.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace View {

namespace {
float Clamp01(float value) { return std::clamp(value, 0.0f, 1.0f); }

const char* BossName(int type) {
    switch (type) {
        case 1: return "BOSS I";
        case 2: return "BOSS II";
        case 3: return "BOSS III";
        default: return "BOSS";
    }
}

void DrawPanel(Texture2D texture, Rectangle rect, float border = 12.0f, Color tint = WHITE) {
    if (!texture.id) return;
    NPatchInfo patch{{0.0f, 0.0f, (float)texture.width, (float)texture.height},
                     (int)border, (int)border, (int)border, (int)border, NPATCH_NINE_PATCH};
    ::DrawTextureNPatch(texture, patch, rect, {0.0f, 0.0f}, 0.0f, tint);
}

void DrawThreeSlice(Texture2D left, Texture2D mid, Texture2D right,
                    Rectangle rect, Color tint = WHITE) {
    if (!left.id || !mid.id || !right.id || rect.width <= 0.0f) return;
    const float capWidth = rect.height * 0.5f;
    const float middleWidth = std::max(0.0f, rect.width - capWidth * 2.0f);
    ::DrawTexturePro(left, {0,0,(float)left.width,(float)left.height},
                     {rect.x, rect.y, capWidth, rect.height}, {0,0}, 0, tint);
    if (middleWidth > 0.0f) {
        ::DrawTexturePro(mid, {0,0,(float)mid.width,(float)mid.height},
                         {rect.x + capWidth, rect.y, middleWidth, rect.height}, {0,0}, 0, tint);
    }
    ::DrawTexturePro(right, {0,0,(float)right.width,(float)right.height},
                     {rect.x + rect.width - capWidth, rect.y, capWidth, rect.height}, {0,0}, 0, tint);
}

void DrawCenteredText(const char* text, float centerX, float y, int size, Color color) {
    ::DrawText(text, static_cast<int>(centerX - ::MeasureText(text, size) * 0.5f), static_cast<int>(y), size, color);
}
} // namespace

HUDView& HUDView::GetInstance() {
    static HUDView instance;
    return instance;
}

bool HUDView::Init() {
    m_loaded = true;
    return true;
}

bool HUDView::LoadResources(const std::string&) {
    m_panelBrown = ::LoadTexture("assets/ui/kenney_rpg/panel_brown.png");
    m_panelInsetBrown = ::LoadTexture("assets/ui/kenney_rpg/panelInset_brown.png");
    m_buttonRoundBrown = ::LoadTexture("assets/ui/kenney_rpg/buttonRound_brown.png");
    m_barBackLeft = ::LoadTexture("assets/ui/kenney_rpg/barBack_horizontalLeft.png");
    m_barBackMid = ::LoadTexture("assets/ui/kenney_rpg/barBack_horizontalMid.png");
    m_barBackRight = ::LoadTexture("assets/ui/kenney_rpg/barBack_horizontalRight.png");
    m_barRedLeft = ::LoadTexture("assets/ui/kenney_rpg/barRed_horizontalLeft.png");
    m_barRedMid = ::LoadTexture("assets/ui/kenney_rpg/barRed_horizontalMid.png");
    m_barRedRight = ::LoadTexture("assets/ui/kenney_rpg/barRed_horizontalRight.png");
    Texture2D* uiTextures[] = {&m_panelBrown, &m_panelInsetBrown, &m_buttonRoundBrown,
        &m_barBackLeft, &m_barBackMid, &m_barBackRight,
        &m_barRedLeft, &m_barRedMid, &m_barRedRight};
    for (Texture2D* texture : uiTextures) {
        if (texture->id) ::SetTextureFilter(*texture, TEXTURE_FILTER_POINT);
    }
    m_coinAtlas = Animations::TextureAtlas::LoadFromJSON("assets/textures/items/coin.json");
    if (m_coinAtlas && m_coinAtlas->LoadTexture() && m_coinAtlas->HasClip("spin")) {
        m_coinAnim.SetTexture(m_coinAtlas->GetTexture());
        m_coinAnim.AddClip(m_coinAtlas->GetClip("spin"));
        m_coinAnim.Play("spin");
    }
    m_loaded = true;
    return true;
}

void HUDView::Shutdown() {
    Texture2D* uiTextures[] = {&m_panelBrown, &m_panelInsetBrown, &m_buttonRoundBrown,
        &m_barBackLeft, &m_barBackMid, &m_barBackRight,
        &m_barRedLeft, &m_barRedMid, &m_barRedRight};
    for (Texture2D* texture : uiTextures) {
        if (texture->id) ::UnloadTexture(*texture);
        *texture = {};
    }
    m_avatarAtlas.reset();
    m_avatarSource = {};
    m_avatarClass = -1;
    m_secondAvatarAtlas.reset();
    m_secondAvatarSource = {};
    m_secondAvatarClass = -1;
    m_coinAnim.Stop();
    m_coinAtlas.reset();
    m_player = nullptr; m_secondPlayer = nullptr; m_boss = nullptr; m_lastBoss = nullptr;
    m_loaded = false;
}

void HUDView::LoadAvatar(CharacterClass characterClass) {
    const char* folder = "knight";
    switch (characterClass) {
        case CharacterClass::Fighter: folder = "fighter"; break;
        case CharacterClass::Knight: folder = "knight"; break;
        case CharacterClass::Ninja: folder = "ninja"; break;
        case CharacterClass::MagicCaster: folder = "magic_caster"; break;
    }
    const std::string path = std::string("assets/textures/player/") + folder + "/idle.json";
    auto atlas = Animations::TextureAtlas::LoadFromJSON(path);
    if (!atlas || !atlas->LoadTexture() || !atlas->HasClip("idle")) return;
    auto clip = atlas->GetClip("idle");
    if (!clip || clip->frames.empty()) return;
    const size_t bestFrame = clip->frames.size() / 2;
    m_avatarSource = clip->frames[bestFrame].src;
    m_avatarAtlas = std::move(atlas);
    m_avatarClass = static_cast<int>(characterClass);
}

void HUDView::LoadSecondAvatar(CharacterClass characterClass) {
    const char* folder = "knight";
    switch (characterClass) {
        case CharacterClass::Fighter: folder = "fighter"; break;
        case CharacterClass::Knight: folder = "knight"; break;
        case CharacterClass::Ninja: folder = "ninja"; break;
        case CharacterClass::MagicCaster: folder = "magic_caster"; break;
    }
    const std::string path = std::string("assets/textures/player/") + folder + "/idle.json";
    auto atlas = Animations::TextureAtlas::LoadFromJSON(path);
    if (!atlas || !atlas->LoadTexture() || !atlas->HasClip("idle")) return;
    auto clip = atlas->GetClip("idle");
    if (!clip || clip->frames.empty()) return;
    m_secondAvatarSource = clip->frames[clip->frames.size() / 2].src;
    m_secondAvatarAtlas = std::move(atlas);
    m_secondAvatarClass = static_cast<int>(characterClass);
}

void HUDView::Update(float dt, const Player* player, const Boss* boss, const Player* secondPlayer) {
    if (!m_loaded) return;
    m_player = player;
    m_secondPlayer = secondPlayer;
    m_boss = boss;
    m_time += dt;
    m_coinAnim.Update(dt);

    if (player) {
        if (m_avatarClass != static_cast<int>(player->GetCharacterClass())) {
            LoadAvatar(player->GetCharacterClass());
        }
        const float hp = player->GetMaxHealth() > 0
            ? Clamp01(static_cast<float>(player->GetHealth()) / player->GetMaxHealth()) : 0.0f;
        m_displayHp += (hp - m_displayHp) * std::min(1.0f, dt * 14.0f);
        if (hp >= m_damageTrailHp) m_damageTrailHp = hp;
        else m_damageTrailHp += (hp - m_damageTrailHp) * std::min(1.0f, dt * 2.2f);
    }

    if (secondPlayer) {
        if (m_secondAvatarClass != static_cast<int>(secondPlayer->GetCharacterClass())) {
            LoadSecondAvatar(secondPlayer->GetCharacterClass());
        }
        const float hp = secondPlayer->GetMaxHealth() > 0
            ? Clamp01(static_cast<float>(secondPlayer->GetHealth()) / secondPlayer->GetMaxHealth()) : 0.0f;
        m_secondDisplayHp += (hp - m_secondDisplayHp) * std::min(1.0f, dt * 14.0f);
        if (hp >= m_secondDamageTrailHp) m_secondDamageTrailHp = hp;
        else m_secondDamageTrailHp += (hp - m_secondDamageTrailHp) * std::min(1.0f, dt * 2.2f);
    } else {
        m_secondDisplayHp = m_secondDamageTrailHp = 1.0f;
    }

    if (boss != m_lastBoss) {
        m_bossDisplayHp = m_bossDamageTrailHp = 1.0f;
        m_lastBoss = boss;
    }
    if (boss) {
        const float hp = boss->GetMaxHealth() > 0
            ? Clamp01(static_cast<float>(boss->GetHealth()) / boss->GetMaxHealth()) : 0.0f;
        m_bossDisplayHp += (hp - m_bossDisplayHp) * std::min(1.0f, dt * 12.0f);
        if (hp >= m_bossDamageTrailHp) m_bossDamageTrailHp = hp;
        else m_bossDamageTrailHp += (hp - m_bossDamageTrailHp) * std::min(1.0f, dt * 1.8f);
    }
}

bool HUDView::WantsQuitTest() {
    const bool result = m_wantsQuitTest;
    m_wantsQuitTest = false;
    return result;
}

void HUDView::Render() {
    if (!m_visible || !m_loaded || !m_player) return;
    Renderer::GetInstance().EndFrameAndFlush();

    const int w = ::GetScreenWidth();
    const int h = ::GetScreenHeight();
    const float scale = std::clamp(std::min(w / 1280.0f, h / 720.0f), 0.65f, 1.5f);

    auto drawHealthBar = [&](Rectangle rect, float fill, float trail) {
        DrawThreeSlice(m_barBackLeft, m_barBackMid, m_barBackRight, rect);
        Rectangle inner{rect.x + 4.0f*scale, rect.y + 5.0f*scale,
                        rect.width - 8.0f*scale, rect.height - 10.0f*scale};
        if (trail > fill && inner.width > 0.0f) {
            Rectangle trailRect = inner;
            trailRect.width *= Clamp01(trail);
            ::DrawRectangleRounded(trailRect, 0.45f, 8, Color{255, 203, 83, 230});
        }
        const int fillWidth = std::max(0, static_cast<int>(std::ceil(rect.width * Clamp01(fill))));
        if (fillWidth > 0) {
            ::BeginScissorMode(static_cast<int>(rect.x), static_cast<int>(rect.y), fillWidth,
                               std::max(1, static_cast<int>(std::ceil(rect.height))));
            const Color tint = fill < 0.25f && std::sin(m_time * 8.0f) > 0.0f
                ? Color{255, 160, 160, 255} : WHITE;
            DrawThreeSlice(m_barRedLeft, m_barRedMid, m_barRedRight, rect, tint);
            ::EndScissorMode();
        }
    };

    // Kenney RPG panel + a real idle frame from the currently selected character class.
    const Rectangle panel{18.0f * scale, 18.0f * scale, 229.0f * scale, 48.0f * scale};
    ::DrawRectangleRounded({panel.x + 5*scale, panel.y + 7*scale, panel.width, panel.height},
                           0.12f, 8, Color{12, 7, 5, 145});
    DrawPanel(m_panelBrown, panel, 13.0f);
    const float portraitSize = 38.0f * scale;
    Rectangle portraitRect{panel.x + 5 * scale, panel.y + 5 * scale, portraitSize, portraitSize};
    if (m_buttonRoundBrown.id) {
        ::DrawTexturePro(m_buttonRoundBrown,
                         {0,0,(float)m_buttonRoundBrown.width,(float)m_buttonRoundBrown.height},
                         portraitRect, {0,0}, 0.0f, WHITE);
    }
    if (m_avatarAtlas && m_avatarAtlas->GetTexture() && m_avatarAtlas->GetTexture()->id &&
        m_avatarSource.width > 0.0f && m_avatarSource.height > 0.0f) {
        const float avatarHeight = portraitSize * 0.91f;
        const float avatarWidth = avatarHeight * m_avatarSource.width / m_avatarSource.height;
        Rectangle avatarDest{portraitRect.x + (portraitSize - avatarWidth) * 0.5f,
                             portraitRect.y + portraitSize - avatarHeight - 2.0f*scale,
                             avatarWidth, avatarHeight};
        ::DrawTexturePro(*m_avatarAtlas->GetTexture(), m_avatarSource, avatarDest, {0,0}, 0.0f, WHITE);
    }
    ::DrawCircleLinesV({portraitRect.x + portraitSize*0.5f, portraitRect.y + portraitSize*0.5f},
                       portraitSize*0.46f, Color{255, 224, 151, 220});

    const float barX = portraitRect.x + portraitSize + 6 * scale;
    const float barY = panel.y + 15 * scale;
    const float barW = panel.x + panel.width - barX - 6 * scale;
    const float barH = 17 * scale;
    DrawPanel(m_panelInsetBrown,
              {barX - 5*scale, barY - 5*scale, barW + 10*scale, barH + 10*scale},
              11.0f, Color{222, 197, 154, 255});
    drawHealthBar({barX, barY, barW, barH}, m_displayHp, m_damageTrailHp);
    char hpText[64];
    std::snprintf(hpText, sizeof(hpText), "%d / %d", m_player->GetHealth(), m_player->GetMaxHealth());
    DrawCenteredText(hpText, barX + barW * 0.5f, barY + 3 * scale,
                     std::max(9, static_cast<int>(11 * scale)), WHITE);

    // Potions heal immediately on pickup, so only persistent coins need a counter.
    const Inventory& inv = m_player->GetInventory();
    const float resourceY = (m_secondPlayer ? 218.0f : 20.0f) * scale;
    const float resourceW = 84 * scale;
    const float resourceH = 48 * scale;
    auto drawResource = [&](float x, Texture2D* texture, Rectangle source, int amount, Color tint) {
        Rectangle box{x, resourceY, resourceW, resourceH};
        ::DrawRectangleRounded({box.x + 3*scale, box.y + 4*scale, box.width, box.height},
                               0.18f, 8, Color{12,7,5,130});
        DrawPanel(m_panelBrown, box, 12.0f);
        const float iconSize = 32 * scale;
        if (texture && texture->id) {
            ::DrawTexturePro(*texture, source, {x + 8*scale, resourceY + 8*scale, iconSize, iconSize}, {0,0}, 0, tint);
        }
        char amountText[16]; std::snprintf(amountText, sizeof(amountText), "%d", amount);
        ::DrawText(amountText, static_cast<int>(x + 47*scale), static_cast<int>(resourceY + 13*scale),
                   std::max(12, static_cast<int>(18*scale)), WHITE);
    };
    float right = w - 18 * scale;
    Texture2D* coinTexture = m_coinAnim.GetCurrentTexture();
    drawResource(right - resourceW, coinTexture, m_coinAnim.GetCurrentSrcRect(), inv.GetCoins(), WHITE);

    if (m_secondPlayer) {
        const Rectangle secondPanel{w - 18.0f * scale - panel.width, panel.y,
                                    panel.width, panel.height};
        ::DrawRectangleRounded({secondPanel.x + 5*scale, secondPanel.y + 7*scale,
                                secondPanel.width, secondPanel.height},
                               0.12f, 8, Color{10, 7, 20, 150});
        DrawPanel(m_panelBrown, secondPanel, 13.0f, Color{220, 202, 242, 255});

        Rectangle secondPortrait{secondPanel.x + secondPanel.width - portraitSize - 5*scale,
                                  secondPanel.y + 5*scale, portraitSize, portraitSize};
        if (m_buttonRoundBrown.id) {
            ::DrawTexturePro(m_buttonRoundBrown,
                {0,0,(float)m_buttonRoundBrown.width,(float)m_buttonRoundBrown.height},
                secondPortrait, {0,0}, 0.0f, Color{220, 196, 255, 255});
        }
        if (m_secondAvatarAtlas && m_secondAvatarAtlas->GetTexture() &&
            m_secondAvatarAtlas->GetTexture()->id && m_secondAvatarSource.width > 0.0f &&
            m_secondAvatarSource.height > 0.0f) {
            const float avatarHeight = portraitSize * 0.91f;
            const float avatarWidth = avatarHeight * m_secondAvatarSource.width / m_secondAvatarSource.height;
            const Rectangle avatarDest{
                secondPortrait.x + (portraitSize - avatarWidth) * 0.5f,
                secondPortrait.y + portraitSize - avatarHeight - 2.0f*scale,
                avatarWidth, avatarHeight};
            ::DrawTexturePro(*m_secondAvatarAtlas->GetTexture(), m_secondAvatarSource,
                             avatarDest, {0,0}, 0.0f, WHITE);
        }
        ::DrawCircleLinesV({secondPortrait.x + portraitSize*0.5f,
                            secondPortrait.y + portraitSize*0.5f},
                           portraitSize*0.46f, Color{222, 184, 255, 235});

        const float secondBarX = secondPanel.x + 6*scale;
        const float secondBarY = secondPanel.y + 15*scale;
        const float secondBarW = secondPortrait.x - secondBarX - 6*scale;
        DrawPanel(m_panelInsetBrown,
                  {secondBarX - 5*scale, secondBarY - 5*scale,
                   secondBarW + 10*scale, barH + 10*scale},
                  11.0f, Color{218, 198, 235, 255});
        drawHealthBar({secondBarX, secondBarY, secondBarW, barH},
                      m_secondDisplayHp, m_secondDamageTrailHp);
        char secondHp[48];
        std::snprintf(secondHp, sizeof(secondHp), "%d / %d",
                      m_secondPlayer->GetHealth(), m_secondPlayer->GetMaxHealth());
        DrawCenteredText(secondHp, secondBarX + secondBarW * 0.5f,
                         secondBarY + 3*scale,
                         std::max(9, static_cast<int>(11*scale)), WHITE);
    }

    // Boss bar: exact HP plus explicit phase count and phase segments.
    if (m_boss && m_boss->IsActive()) {
        const float bossW = std::min(540.0f * scale, w * 0.54f);
        const float bossH = 34.0f * scale;
        const float bossX = (w - bossW) * 0.5f;
        const float bossY = (w < 1000)
            ? panel.y + panel.height + 12.0f * scale
            : 20.0f * scale;
        const int currentPhase = std::clamp(m_boss->GetCurrentPhaseNumber(), 1, m_boss->GetTotalPhases());
        const int totalPhases = m_boss->GetTotalPhases();
        char bossTitle[80];
        std::snprintf(bossTitle, sizeof(bossTitle), "%s    PHASE %d / %d",
                      BossName(m_boss->GetBossType()), currentPhase, totalPhases);
        DrawCenteredText(bossTitle, w * 0.5f, bossY - 1 * scale,
                         std::max(12, static_cast<int>(17 * scale)), Color{255,226,153,255});
        drawHealthBar({bossX, bossY + 23*scale, bossW, bossH},
                      m_bossDisplayHp, m_bossDamageTrailHp);
        char bossHp[64];
        std::snprintf(bossHp, sizeof(bossHp), "%d / %d HP", m_boss->GetHealth(), m_boss->GetMaxHealth());
        DrawCenteredText(bossHp, w * 0.5f, bossY + 30*scale,
                         std::max(11, static_cast<int>(15 * scale)), WHITE);

        const float gap = 6 * scale;
        const float segW = (bossW - gap * (totalPhases - 1)) / totalPhases;
        for (int i = 0; i < totalPhases; ++i) {
            Rectangle seg{bossX + i * (segW + gap), bossY + 62*scale, segW, 5*scale};
            const Color color = i < currentPhase ? Color{246,184,69,255} : Color{56,45,71,230};
            ::DrawRectangleRounded(seg, 0.8f, 4, color);
        }
    }

    if (m_isPlaytest) {
        Rectangle button{w * 0.5f - 60*scale, 92*scale, 120*scale, 34*scale};
        bool hover = ::CheckCollisionPointRec(::GetMousePosition(), button);
        ::DrawRectangleRounded(button, 0.35f, 8, hover ? RED : MAROON);
        DrawCenteredText("QUIT TEST", w * 0.5f, button.y + 8*scale,
                         std::max(10, static_cast<int>(14*scale)), WHITE);
        if (hover && ::IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) m_wantsQuitTest = true;
    }
}

} // namespace View
