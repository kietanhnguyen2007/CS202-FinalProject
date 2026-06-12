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

void SkillBarView::Init() {
    m_loaded = true;
    // Default 4 skills
    m_skills = {
        { SkillType::Fireball, 2.0f, 0.0f },
        { SkillType::Heal,     5.0f, 0.0f },
        { SkillType::Dash,     1.5f, 0.0f },
        { SkillType::Shield,   4.0f, 0.0f },
    };
}

bool SkillBarView::LoadResources(const std::string& atlasJsonPath) {
    (void)atlasJsonPath;
    m_loaded = true;
    return true;
}

void SkillBarView::Shutdown() {
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
    if (!m_open || !m_loaded) return;
    // Update cooldown timers
    for (auto& s : m_skills) {
        if (s.currentTimer > 0.0f) {
            s.currentTimer -= dt;
            if (s.currentTimer < 0.0f) s.currentTimer = 0.0f;
        }
    }
    if (player) {
        // Show skill points if needed
    }
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

        // Slot background
        Color bg = selected ? (Color){80,80,100,220} : (Color){40,40,50,200};
        r.DrawRectangle({x, y}, {slotW, slotH}, bg, Layer::UI, 0.0f);

        // Skill label
        r.DrawText(SkillLabel(skill.type).c_str(), {x + 6, y + 6}, 12, WHITE);

        // Cooldown overlay
        if (!ready) {
            float frac = skill.currentTimer / skill.cooldown;
            float overlayH = slotH * frac;
            r.DrawRectangle({x, y + slotH - overlayH}, {slotW, overlayH},
                            {0,0,0,160}, Layer::UI, 0.0f);
            char buf[16];
            snprintf(buf, sizeof(buf), "%.1f", skill.currentTimer);
            r.DrawText(buf, {x + slotW*0.25f, y + slotH*0.35f}, 14, ORANGE);
        }

        // Selection highlight border
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

    // Sync initial state
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