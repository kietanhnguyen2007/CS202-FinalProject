#include "View/SkillBarView.h"
#include "View/Renderer.h"
#include "View/UIHelpers.h"
#include "View/UIResourceManager.h"
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
        { SkillType::UltimateFighter,      10.0f, 0.0f },
        { SkillType::UltimateKnight,       12.0f, 6.0f },
        { SkillType::UltimateNinja,         8.0f, 0.0f },
        { SkillType::UltimateMagicCaster,  15.0f, 0.0f },
    };
    return true;
}

void SkillBarView::InitIcons() {
    m_skillIcons.clear();
    m_skillIcons.resize(4);

    auto loadAnimated = [&](int idx, const std::string& jsonPath, const std::string& clipName) {
        auto atlas = Animations::TextureAtlas::LoadFromJSON(jsonPath);
        if (!atlas) return;
        atlas->LoadTexture();
        SkillIcon icon;
        icon.atlas = std::move(atlas);
        icon.animated = true;
        icon.clipName = clipName;
        icon.anim.SetTexture(icon.atlas->GetTexture());
        if (icon.atlas->HasClip(clipName)) {
            icon.anim.AddClip(icon.atlas->GetClip(clipName));
            icon.anim.Play(clipName);
        }
        m_skillIcons[idx] = std::move(icon);
    };

    loadAnimated(0, "assets/textures/player/fighter/ultimate_skill.json", "ultimate_skill");
    loadAnimated(1, "assets/textures/player/knight/ultimate_skill.json", "ultimate_skill");
    loadAnimated(2, "assets/textures/player/ninja/ultimate_skill.json", "ultimate_skill");
    loadAnimated(3, "assets/textures/player/magic_caster/ultimate_skill.json", "ultimate_skill");
}

bool SkillBarView::LoadResources(const std::string& atlasJsonPath) {
    (void)atlasJsonPath;
    m_skillIcons.clear();
    InitIcons();
    
    m_texCursor = ::LoadTexture("assets/ui/darkDwellers/20251118darkDwellersCursourA1-Sheet.png");
    m_texBar    = ::LoadTexture("assets/ui/darkDwellers/20251118darkDwellersBarE.png");

    m_loaded = true;
    return true;
}

void SkillBarView::Shutdown() {
    for (auto& si : m_skillIcons) {
        si.anim.Stop();
        si.atlas.reset();
    }
    m_skillIcons.clear();

    if (m_texCursor.id != 0) ::UnloadTexture(m_texCursor);
    if (m_texBar.id != 0) ::UnloadTexture(m_texBar);
    m_texCursor = {};
    m_texBar = {};

    m_loaded = false;
    DetachObservable();
}

std::string SkillBarView::SkillLabel(SkillType t) {
    switch (t) {
        case SkillType::UltimateFighter:      return "Fighter";
        case SkillType::UltimateKnight:       return "Knight";
        case SkillType::UltimateNinja:        return "Ninja";
        case SkillType::UltimateMagicCaster:  return "Caster";
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

        auto& res = UIResourceManager::GetInstance();
        Texture2D* texSlot = res.GetSlot();
        float slotFrameW = res.GetSlotFrameWidth();

        if (texSlot && texSlot->id != 0 && slotFrameW > 0) {
            int frame = selected ? 1 : 0;
            Rectangle src = { (float)(frame * slotFrameW), 0.0f, slotFrameW, (float)texSlot->height };
            float scaleX = slotW / slotFrameW;
            float scaleY = slotH / (float)texSlot->height;
            r.SubmitSprite(texSlot, src, {x, y}, {scaleX, scaleY}, 0.0f, {0,0}, WHITE, Layer::UI, 0.0f, false, 0);
        } else {
            Color bg = selected ? (Color){80, 80, 100, 220} : (Color){40, 40, 50, 200};
            r.DrawRectangle({x, y}, {slotW, slotH}, bg, Layer::UI, 0.0f);
        }

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
            if (m_texBar.id != 0) {
                Rectangle src = { 0.0f, 0.0f, (float)m_texBar.width, (float)m_texBar.height * frac };
                float scaleX = slotW / (float)m_texBar.width;
                float scaleY = slotH / (float)m_texBar.height;
                r.SubmitSprite(&m_texBar, src, {x, y + slotH - overlayH}, {scaleX, scaleY}, 0.0f, {0,0}, (Color){50,50,50,220}, Layer::UI, 0.5f, false, 0);
            } else {
                r.DrawRectangle({x, y + slotH - overlayH}, {slotW, overlayH}, {0, 0, 0, 160}, Layer::UI, 0.5f);
            }
            char buf[16];
            snprintf(buf, sizeof(buf), "%.1f", skill.currentTimer);
            r.DrawText(buf, {x + slotW * 0.25f, y + slotH * 0.35f}, 14, ORANGE);
        }

        if (selected) {
            if (m_texCursor.id != 0) {
                int cFrameW = m_texCursor.width / 4; 
                Rectangle cSrc = { 0.0f, 0.0f, (float)cFrameW, (float)m_texCursor.height };
                float cScaleX = slotW / (float)cFrameW;
                float cScaleY = slotH / (float)m_texCursor.height;
                r.SubmitSprite(&m_texCursor, cSrc, {x, y}, {cScaleX, cScaleY}, 0.0f, {0,0}, WHITE, Layer::UI, 0.6f, false, 0);
            } else {
                r.DrawRectangle({x, y}, {slotW, 2}, YELLOW, Layer::UI, 0.6f);
            }
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