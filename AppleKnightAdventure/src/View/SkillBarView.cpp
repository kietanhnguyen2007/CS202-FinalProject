#include "View/SkillBarView.h"
#include "View/Renderer.h"
#include "View/UIHelpers.h"
#include "Model/Player.h"
#include <cstdio>

namespace View {

SkillBarView& SkillBarView::GetInstance() {
    static SkillBarView inst;
    return inst;
}

bool SkillBarView::Init() {
    m_loaded = true;
    m_skills = {
        { SkillType::Fireball, 2.0f, 0.0f },
        { SkillType::Heal,     5.0f, 0.0f },
        { SkillType::Dash,     1.5f, 0.0f },
        { SkillType::Shield,   4.0f, 0.0f },
    };
    return true;
}

void SkillBarView::InitIcons() {
    m_skillIcons.clear();
    // Reserve exactly 4 slots matching m_skills order
    m_skillIcons.resize(4);

    auto loadStatic = [&](int idx, const std::string& jsonPath) {
        auto atlas = Animations::TextureAtlas::LoadFromJSON(jsonPath);
        if (!atlas) return;
        atlas->LoadTexture();
        m_skillIcons[idx] = {std::move(atlas), Animations::Animator{}, false, ""};
    };

    auto loadAnimated = [&](int idx, const std::string& jsonPath, const std::string& clipName) {
        auto atlas = Animations::TextureAtlas::LoadFromJSON(jsonPath);
        if (!atlas) return;
        atlas->LoadTexture();
        SkillIcon icon;
        icon.atlas = std::move(atlas);
        icon.animated = true;
        icon.clipName = clipName;
        icon.anim.SetTexture(atlas->GetTexture());
        if (atlas->HasClip(clipName)) {
            icon.anim.AddClip(atlas->GetClip(clipName));
            icon.anim.Play(clipName);
        }
        m_skillIcons[idx] = std::move(icon);
    };

    loadStatic(0, "assets/textures/projectiles/fire_bullet.json");     // Fireball
    loadStatic(1, "assets/textures/items/potion.json");                // Heal
    loadAnimated(2, "assets/textures/projectiles/slash.json", "slash"); // Dash
    loadAnimated(3, "assets/textures/projectiles/hit.json", "hit");     // Shield
}

bool SkillBarView::LoadResources(const std::string& atlasJsonPath) {
    (void)atlasJsonPath;
    m_skillIcons.clear();
    InitIcons();
    m_loaded = true;
    return true;
}

void SkillBarView::Shutdown() {
    for (auto& si : m_skillIcons) {
        si.anim.Stop();
        si.atlas.reset();
    }
    m_skillIcons.clear();
    m_loaded = false;
    DetachObservable();
}

std::string SkillBarView::SkillLabel(SkillType t) {
    switch (t) {
        case SkillType::Fireball: return "Fireball";
        case SkillType::Heal:     return "Heal";
        case SkillType::Dash:     return "Dash";
        case SkillType::Shield:   return "Shield";
    }
    return "?";
}

void SkillBarView::Update(float dt, const Player* player) {
    for (auto& s : m_skills) {
        if (s.currentTimer > 0.0f) {
            s.currentTimer -= dt;
            if (s.currentTimer < 0.0f) s.currentTimer = 0.0f;
        }
    }

    for (auto& si : m_skillIcons) {
        si.anim.Update(dt);
    }

    if (!m_open || !m_loaded) return;
    (void)player;
}

void SkillBarView::Render() {
    if (!m_open || !m_loaded) return;
    Renderer& r = Renderer::GetInstance();
    int w = r.GetWindowWidth();
    int h = r.GetWindowHeight();

    const float slotW = 56.0f;
    const float slotH = 56.0f;
    const float gap = 6.0f;
    const int count = (int)m_skills.size();
    const float totalW = count * slotW + (count - 1) * gap;
    const float startX = (w - totalW) * 0.5f;
    const float y = h - 80.0f;

    for (int i = 0; i < count; ++i) {
        float x = startX + i * (slotW + gap);
        const auto& skill = m_skills[i];
        bool selected = (i == m_selection);
        bool ready = skill.IsReady();

        Color bg = selected ? (Color){80, 80, 100, 220} : (Color){40, 40, 50, 200};
        r.DrawRectangle({x, y}, {slotW, slotH}, bg, Layer::UI, 0.0f);

        // Draw skill icon
        if (i < (int)m_skillIcons.size()) {
            const auto& icon = m_skillIcons[i];
            if (icon.atlas && icon.atlas->GetTexture() && icon.atlas->GetTexture()->id != 0) {
                Rectangle src{};
                if (icon.animated && icon.anim.IsPlaying()) {
                    src = icon.anim.GetCurrentSrcRect();
                } else if (icon.atlas->HasFrame("default")) {
                    src = icon.atlas->GetFrameRect("default");
                }
                if (src.width > 0 && src.height > 0) {
                    float iconSize = slotW - 12;
                    Color tint = ready ? WHITE : (Color){100, 100, 100, 200};
                    r.SubmitSprite(icon.atlas->GetTexture(), src,
                                   {x + 6, y + 6},
                                   {iconSize / src.width, iconSize / src.height},
                                   0.0f, {0, 0}, tint, Layer::UI, 0.0f, false, 0);
                }
            }
        }

        // Skill label
        r.DrawText(SkillLabel(skill.type).c_str(), {x + 4, y + slotH - 14}, 10, WHITE);

        // Cooldown overlay
        if (!ready) {
            float frac = skill.currentTimer / skill.cooldown;
            float overlayH = slotH * frac;
            r.DrawRectangle({x, y + slotH - overlayH}, {slotW, overlayH},
                            {0, 0, 0, 160}, Layer::UI, 0.0f);
            char buf[16];
            snprintf(buf, sizeof(buf), "%.1f", skill.currentTimer);
            r.DrawText(buf, {x + slotW * 0.25f, y + slotH * 0.35f}, 14, ORANGE);
        }

        if (selected) {
            r.DrawRectangle({x, y}, {slotW, 2}, YELLOW, Layer::UI, 0.0f);
        }
    }
}

void SkillBarView::Open() { m_open = true; }
void SkillBarView::Close() { m_open = false; }
bool SkillBarView::IsOpen() const { return m_open; }
void SkillBarView::SetSelection(int index) { m_selection = index; }

void SkillBarView::AttachObservable(ObservableList<SkillSlotData>* observable) {
    m_attachedObservable = observable;
    if (!observable) return;

    observable->OnItemAddedCallback = [this](const SkillSlotData& item) {
        m_skills.push_back(item);
    };
    observable->OnItemRemovedCallback = [this](const SkillSlotData& item) {
        for (auto it = m_skills.begin(); it != m_skills.end(); ++it) {
            if (it->type == item.type) { m_skills.erase(it); break; }
        }
    };
    observable->OnClearedCallback = [this]() {
        m_skills.clear();
    };

    m_skills.clear();
    for (size_t i = 0; i < observable->Size(); ++i) {
        m_skills.push_back((*observable)[i]);
    }
}

void SkillBarView::DetachObservable() {
    if (m_attachedObservable) {
        m_attachedObservable->OnItemAddedCallback = nullptr;
        m_attachedObservable->OnItemRemovedCallback = nullptr;
        m_attachedObservable->OnClearedCallback = nullptr;
        m_attachedObservable = nullptr;
    }
}

} // namespace View