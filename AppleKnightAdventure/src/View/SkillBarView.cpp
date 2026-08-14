#include "View/SkillBarView.h"
#include "View/Renderer.h"
#include "Model/Player.h"
#include "Model/KnightSkillSet.h"
#include "Model/FighterSkillSet.h"
#include "Model/MagicCasterSkillSet.h"
#include "Model/NinjaSkillSet.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace View {

namespace {
struct IconConfig { const char* path; const char* clip; };

std::array<IconConfig, 4> IconConfigs(CharacterClass cls) {
    switch (cls) {
        case CharacterClass::Fighter:
            return {{{"assets/textures/player/fighter/attack1.json", "attack"},
                     {"assets/textures/player/fighter/attack2.json", "attack_2"},
                     {"assets/textures/player/fighter/attack3.json", "attack_3"},
                     {"assets/textures/player/fighter/ultimate_projectile.json", "ultimate_projectile"}}};
        case CharacterClass::Knight:
            return {{{"assets/textures/player/knight/attack1.json", "attack"},
                     {"assets/textures/player/knight/attack2.json", "attack_2"},
                     {"assets/textures/player/knight/attack3.json", "attack_3"},
                     {"assets/textures/player/knight/ultimate_skill.json", "ultimate_skill"}}};
        case CharacterClass::Ninja:
            return {{{"assets/textures/player/ninja/attack1.json", "attack"},
                     {"assets/textures/player/ninja/projectile_attack2.json", "projectile_attack2"},
                     {"assets/textures/player/ninja/skill3_teleport_start.json", "skill3_teleport_start"},
                     {"assets/textures/player/ninja/projectile_ultimate_attack.json", "projectile_ultimate_attack"}}};
        case CharacterClass::MagicCaster:
        default:
            return {{{"assets/textures/player/magic_caster/projectile_attack1.json", "projectile_attack1"},
                     {"assets/textures/player/magic_caster/projectile_attack2.json", "projectile_attack2"},
                     {"assets/textures/player/magic_caster/projectile_attack3.json", "projectile_attack3"},
                     {"assets/textures/player/magic_caster/ultimate_skill_projectile.json", "ultimate_skill_projectile"}}};
    }
}

void DrawCentered(const char* text, Vector2 center, int size, Color color) {
    ::DrawText(text, static_cast<int>(center.x - ::MeasureText(text, size) * 0.5f),
               static_cast<int>(center.y - size * 0.5f), size, color);
}
} // namespace

SkillBarView& SkillBarView::GetInstance() {
    static SkillBarView instance;
    return instance;
}

bool SkillBarView::Init() {
    m_loaded = true;
    return true;
}

bool SkillBarView::LoadResources(const std::string&) {
    m_loaded = true;
    return true;
}

void SkillBarView::Shutdown() {
    for (auto& icon : m_icons) { icon.atlas.reset(); icon.source = {}; }
    for (auto& icon : m_secondIcons) { icon.atlas.reset(); icon.source = {}; }
    m_player = nullptr;
    m_secondPlayer = nullptr;
    m_loadedClass = static_cast<CharacterClass>(-1);
    m_secondLoadedClass = static_cast<CharacterClass>(-1);
    m_loaded = false;
}

void SkillBarView::LoadClassIcons(CharacterClass cls) {
    const auto configs = IconConfigs(cls);
    for (int i = 0; i < 4; ++i) {
        auto& icon = m_icons[i];
        icon = {};
        icon.atlas = Animations::TextureAtlas::LoadFromJSON(configs[i].path);
        if (!icon.atlas || !icon.atlas->LoadTexture() || !icon.atlas->HasClip(configs[i].clip)) continue;
        auto clip = icon.atlas->GetClip(configs[i].clip);
        if (!clip || clip->frames.empty()) continue;
        const size_t frameIndex = clip->frames.size() / 2;
        icon.source = clip->frames[frameIndex].src;
    }
    m_loadedClass = cls;
}

void SkillBarView::LoadSecondClassIcons(CharacterClass cls) {
    const auto configs = IconConfigs(cls);
    for (int i = 0; i < 4; ++i) {
        auto& icon = m_secondIcons[i];
        icon = {};
        icon.atlas = Animations::TextureAtlas::LoadFromJSON(configs[i].path);
        if (!icon.atlas || !icon.atlas->LoadTexture() || !icon.atlas->HasClip(configs[i].clip)) continue;
        auto clip = icon.atlas->GetClip(configs[i].clip);
        if (!clip || clip->frames.empty()) continue;
        icon.source = clip->frames[clip->frames.size() / 2].src;
    }
    m_secondLoadedClass = cls;
}

void SkillBarView::CopySkill(int index, const char* key, float cooldown, float timer,
                             bool charging, bool active) {
    m_skills[index] = {key, cooldown, std::max(0.0f, timer), charging, active};
}

void SkillBarView::CopySecondSkill(int index, const char* key, float cooldown, float timer,
                                   bool charging, bool active) {
    m_secondSkills[index] = {key, cooldown, std::max(0.0f, timer), charging, active};
}

void SkillBarView::Update(float dt, const Player* player, const Player* secondPlayer) {
    m_player = player;
    m_secondPlayer = secondPlayer;
    m_time += dt;
    if (!player) return;
    if (m_loadedClass != player->GetCharacterClass()) LoadClassIcons(player->GetCharacterClass());

    if (auto* skills = player->GetKnightSkills()) {
        CopySkill(0, "J", skills->attack1.cooldownMax, skills->attack1.cooldownTimer, skills->attack1.isCharging, skills->attack1.isActive);
        CopySkill(1, "K", skills->attack2.cooldownMax, skills->attack2.cooldownTimer, skills->attack2.isCharging, skills->attack2.isActive);
        CopySkill(2, "U", skills->attack3.cooldownMax, skills->attack3.cooldownTimer, skills->attack3.isCharging, skills->attack3.isActive);
        CopySkill(3, "H", skills->ultimate.cooldownMax, skills->ultimate.cooldownTimer, skills->ultimate.isCharging, skills->ultimate.isActive);
    } else if (auto* skills = player->GetFighterSkills()) {
        CopySkill(0, "J", skills->attack1.cooldownMax, skills->attack1.cooldownTimer, skills->attack1.isCharging, skills->attack1.isActive);
        CopySkill(1, "K", skills->attack2.cooldownMax, skills->attack2.cooldownTimer, skills->attack2.isCharging, skills->attack2.isActive);
        CopySkill(2, "U", skills->attack3.cooldownMax, skills->attack3.cooldownTimer, skills->attack3.isCharging, skills->attack3.isActive);
        CopySkill(3, "H", skills->ultimate.cooldownMax, skills->ultimate.cooldownTimer, skills->ultimate.isCharging, skills->IsUltimateCharging());
    } else if (auto* skills = player->GetMagicSkills()) {
        CopySkill(0, "J", skills->attack1.cooldownMax, skills->attack1.cooldownTimer, skills->attack1.isCharging, skills->attack1.isActive);
        CopySkill(1, "K", skills->attack2.cooldownMax, skills->attack2.cooldownTimer, skills->attack2.isCharging, skills->attack2.isActive);
        CopySkill(2, "U", skills->attack3.cooldownMax, skills->attack3.cooldownTimer, skills->attack3.isCharging, skills->attack3.isActive);
        CopySkill(3, "H", skills->ultimate.cooldownMax, skills->ultimate.cooldownTimer, skills->ultimate.isCharging, skills->ultimate.isActive);
    } else if (auto* skills = player->GetNinjaSkills()) {
        CopySkill(0, "J", skills->attack1.cooldownMax, skills->attack1.cooldownTimer, skills->attack1.isCharging, skills->attack1.isActive);
        CopySkill(1, "K", skills->attack2.cooldownMax, skills->attack2.cooldownTimer, skills->attack2.isCharging, skills->attack2.isActive);
        CopySkill(2, "U", skills->attack3.cooldownMax, skills->attack3.cooldownTimer, false, skills->IsTeleporting());
        CopySkill(3, "H", skills->ultimate.cooldownMax, skills->ultimate.cooldownTimer, skills->ultimate.isCharging, skills->WantsShadowClone());
    }

    if (!secondPlayer) return;
    if (m_secondLoadedClass != secondPlayer->GetCharacterClass()) {
        LoadSecondClassIcons(secondPlayer->GetCharacterClass());
    }
    if (auto* skills = secondPlayer->GetKnightSkills()) {
        CopySecondSkill(0, "1", skills->attack1.cooldownMax, skills->attack1.cooldownTimer, skills->attack1.isCharging, skills->attack1.isActive);
        CopySecondSkill(1, "2", skills->attack2.cooldownMax, skills->attack2.cooldownTimer, skills->attack2.isCharging, skills->attack2.isActive);
        CopySecondSkill(2, "3", skills->attack3.cooldownMax, skills->attack3.cooldownTimer, skills->attack3.isCharging, skills->attack3.isActive);
        CopySecondSkill(3, "0", skills->ultimate.cooldownMax, skills->ultimate.cooldownTimer, skills->ultimate.isCharging, skills->ultimate.isActive);
    } else if (auto* skills = secondPlayer->GetFighterSkills()) {
        CopySecondSkill(0, "1", skills->attack1.cooldownMax, skills->attack1.cooldownTimer, skills->attack1.isCharging, skills->attack1.isActive);
        CopySecondSkill(1, "2", skills->attack2.cooldownMax, skills->attack2.cooldownTimer, skills->attack2.isCharging, skills->attack2.isActive);
        CopySecondSkill(2, "3", skills->attack3.cooldownMax, skills->attack3.cooldownTimer, skills->attack3.isCharging, skills->attack3.isActive);
        CopySecondSkill(3, "0", skills->ultimate.cooldownMax, skills->ultimate.cooldownTimer, skills->ultimate.isCharging, skills->IsUltimateCharging());
    } else if (auto* skills = secondPlayer->GetMagicSkills()) {
        CopySecondSkill(0, "1", skills->attack1.cooldownMax, skills->attack1.cooldownTimer, skills->attack1.isCharging, skills->attack1.isActive);
        CopySecondSkill(1, "2", skills->attack2.cooldownMax, skills->attack2.cooldownTimer, skills->attack2.isCharging, skills->attack2.isActive);
        CopySecondSkill(2, "3", skills->attack3.cooldownMax, skills->attack3.cooldownTimer, skills->attack3.isCharging, skills->attack3.isActive);
        CopySecondSkill(3, "0", skills->ultimate.cooldownMax, skills->ultimate.cooldownTimer, skills->ultimate.isCharging, skills->ultimate.isActive);
    } else if (auto* skills = secondPlayer->GetNinjaSkills()) {
        CopySecondSkill(0, "1", skills->attack1.cooldownMax, skills->attack1.cooldownTimer, skills->attack1.isCharging, skills->attack1.isActive);
        CopySecondSkill(1, "2", skills->attack2.cooldownMax, skills->attack2.cooldownTimer, skills->attack2.isCharging, skills->attack2.isActive);
        CopySecondSkill(2, "3", skills->attack3.cooldownMax, skills->attack3.cooldownTimer, false, skills->IsTeleporting());
        CopySecondSkill(3, "0", skills->ultimate.cooldownMax, skills->ultimate.cooldownTimer, skills->ultimate.isCharging, skills->WantsShadowClone());
    }
}

void SkillBarView::Render() {
    if (!m_open || !m_loaded || !m_player) return;
    Renderer::GetInstance().EndFrameAndFlush();

    const int w = ::GetScreenWidth();
    const int h = ::GetScreenHeight();
    const float scale = std::clamp(std::min(w / 1280.0f, h / 720.0f), 0.65f, 1.5f);

    if (m_secondPlayer) {
        auto drawCoopBar = [&](float panelCenterX,
                               const std::array<GameplaySkillSlot, 4>& skills,
                               const std::array<GameplaySkillIcon, 4>& icons,
                               const char* controls, Color accent) {
            const float radius = 22.0f * scale;
            const float gap = 10.0f * scale;
            const float totalWidth = radius * 8.0f + gap * 3.0f;
            const float centerY = 151.0f * scale;
            Rectangle background{panelCenterX - totalWidth * 0.5f - 13*scale,
                                 centerY - radius - 10*scale,
                                 totalWidth + 26*scale, radius * 2.0f + 38*scale};
            ::DrawRectangleRounded({background.x + 3*scale, background.y + 4*scale,
                                    background.width, background.height},
                                   0.18f, 8, Color{5,4,12,145});
            ::DrawRectangleRounded(background, 0.18f, 8, Color{20,15,34,232});
            ::DrawRectangleRoundedLinesEx(background, 0.18f, 8, 1.5f*scale, Fade(accent, 0.82f));

            float cursorX = panelCenterX - totalWidth * 0.5f;
            for (int i = 0; i < 4; ++i) {
                const Vector2 center{cursorX + radius, centerY};
                cursorX += radius * 2.0f + gap;
                const auto& skill = skills[i];
                const bool ready = skill.timer <= 0.001f;
                const float pulse = ready ? 1.0f + 0.025f * std::sin(m_time * 5.0f) : 1.0f;
                const float drawRadius = radius * pulse;
                ::DrawCircleV(center, drawRadius + 4*scale, Color{7,5,15,230});
                ::DrawCircleV(center, drawRadius, i == 3 ? Color{53,28,65,255} : Color{26,22,43,255});

                const auto& icon = icons[i];
                if (icon.atlas && icon.atlas->GetTexture() && icon.source.width > 0 && icon.source.height > 0) {
                    const float diameter = drawRadius * 1.46f;
                    const float fit = std::min(diameter / icon.source.width, diameter / icon.source.height);
                    const Rectangle dest{center.x, center.y, icon.source.width * fit, icon.source.height * fit};
                    ::DrawTexturePro(*icon.atlas->GetTexture(), icon.source, dest,
                                     {dest.width * 0.5f, dest.height * 0.5f}, 0.0f,
                                     ready ? WHITE : Color{145,145,160,220});
                }
                if (!ready) {
                    const float remaining = skill.cooldown > 0.0f
                        ? std::clamp(skill.timer / skill.cooldown, 0.0f, 1.0f) : 0.0f;
                    ::DrawCircleSector(center, drawRadius, -90.0f,
                                       -90.0f + remaining * 360.0f, 36, Color{4,3,10,190});
                    char cooldown[16];
                    std::snprintf(cooldown, sizeof(cooldown), skill.timer >= 10.0f ? "%.0f" : "%.1f", skill.timer);
                    DrawCentered(cooldown, center, std::max(10, static_cast<int>(13*scale)), WHITE);
                }
                Color ring = ready ? accent : Color{100,88,125,255};
                if (skill.charging) ring = Color{90,210,255,255};
                if (skill.active) ring = Color{255,120,62,255};
                ::DrawRing(center, drawRadius + scale, drawRadius + 3.5f*scale, 0, 360, 36, ring);

                const Vector2 keyCenter{center.x, center.y + drawRadius * 0.76f};
                ::DrawCircleV(keyCenter, 8.5f*scale, Color{14,10,25,245});
                ::DrawCircleLinesV(keyCenter, 8.5f*scale, accent);
                DrawCentered(skill.key, keyCenter, std::max(8, static_cast<int>(10*scale)), WHITE);
            }
            DrawCentered(controls, {panelCenterX, background.y + background.height - 11*scale},
                         std::max(8, static_cast<int>(10*scale)), Color{204,194,220,235});
        };

        const float panelCenterOffset = (18.0f + 382.0f * 0.5f) * scale;
        drawCoopBar(panelCenterOffset, m_skills, m_icons,
                    "L DASH   P GUARD   F USE", Color{104,210,255,255});
        drawCoopBar(w - panelCenterOffset, m_secondSkills, m_secondIcons,
                    "4 DASH   5 GUARD   6 USE   7 RUN", Color{220,168,255,255});
        return;
    }

    const float normalRadius = 31.0f * scale;
    const float ultimateRadius = 37.0f * scale;
    const float gap = 17.0f * scale;
    const float totalWidth = normalRadius * 6.0f + ultimateRadius * 2.0f + gap * 3.0f;
    float cursorX = (w - totalWidth) * 0.5f;
    const float centerY = h - 58.0f * scale;

    for (int i = 0; i < 4; ++i) {
        const float radius = (i == 3) ? ultimateRadius : normalRadius;
        Vector2 center{cursorX + radius, centerY};
        cursorX += radius * 2.0f + gap;

        const auto& skill = m_skills[i];
        const bool ready = skill.timer <= 0.001f;
        const float pulse = ready ? (1.0f + 0.035f * std::sin(m_time * 5.0f)) : 1.0f;
        const float drawRadius = radius * pulse;

        ::DrawCircleV(center, drawRadius + 5*scale, Color{7,5,15,220});
        ::DrawCircleV(center, drawRadius, i == 3 ? Color{53,28,65,255} : Color{26,22,43,255});

        const auto& icon = m_icons[i];
        if (icon.atlas && icon.atlas->GetTexture() && icon.source.width > 0 && icon.source.height > 0) {
            const float diameter = drawRadius * 1.48f;
            const float fit = std::min(diameter / icon.source.width, diameter / icon.source.height);
            Rectangle dest{center.x, center.y, icon.source.width * fit, icon.source.height * fit};
            ::DrawTexturePro(*icon.atlas->GetTexture(), icon.source, dest,
                             {dest.width * 0.5f, dest.height * 0.5f}, 0.0f,
                             ready ? WHITE : Color{150,150,165,220});
        }

        if (!ready) {
            const float remaining = skill.cooldown > 0.0f
                ? std::clamp(skill.timer / skill.cooldown, 0.0f, 1.0f) : 0.0f;
            ::DrawCircleSector(center, drawRadius, -90.0f, -90.0f + remaining * 360.0f,
                               48, Color{4,3,10,190});
            char cooldown[16];
            std::snprintf(cooldown, sizeof(cooldown), skill.timer >= 10.0f ? "%.0f" : "%.1f", skill.timer);
            DrawCentered(cooldown, center, std::max(12, static_cast<int>(17*scale)), WHITE);
        }

        Color ring = ready ? Color{255,205,87,255} : Color{100,88,125,255};
        if (skill.charging) ring = Color{90,210,255,255};
        if (skill.active) ring = Color{255,120,62,255};
        ::DrawRing(center, drawRadius + 1*scale, drawRadius + 4*scale, 0, 360, 48, ring);
        if (skill.charging) {
            ::DrawRing(center, drawRadius + 5*scale, drawRadius + 8*scale,
                       -90, -90 + std::fmod(m_time * 240.0f, 360.0f), 32, Color{120,226,255,230});
        }

        Vector2 keyCenter{center.x, center.y + drawRadius * 0.76f};
        ::DrawCircleV(keyCenter, 11*scale, Color{14,10,25,245});
        ::DrawCircleLinesV(keyCenter, 11*scale, Color{226,188,92,255});
        DrawCentered(skill.key, keyCenter, std::max(10, static_cast<int>(14*scale)), WHITE);
    }

    // Secondary combat controls kept small so J/K/U/H remain the visual focus.
    const float dashReady = m_player->GetDashCooldownRatio();
    char dashText[32];
    std::snprintf(dashText, sizeof(dashText), dashReady >= 0.999f ? "L  DASH" : "L  %.1fs", m_player->GetDashCooldownRemaining());
    const int badgeFont = std::max(10, static_cast<int>(13*scale));
    const float badgeY = h - 112*scale;
    ::DrawRectangleRounded({w*0.5f - 102*scale, badgeY, 92*scale, 27*scale}, 0.5f, 8,
                           Color{12,9,24,220});
    DrawCentered(dashText, {w*0.5f - 56*scale, badgeY + 13*scale}, badgeFont,
                 dashReady >= 0.999f ? Color{175,230,255,255} : GRAY);
    ::DrawRectangleRounded({w*0.5f + 10*scale, badgeY, 92*scale, 27*scale}, 0.5f, 8,
                           Color{12,9,24,220});
    DrawCentered(m_player->IsParrying() ? "P  GUARDING" : "P  GUARD",
                 {w*0.5f + 56*scale, badgeY + 13*scale}, badgeFont,
                 m_player->IsParrying() ? Color{255,210,95,255} : LIGHTGRAY);
}

} // namespace View
