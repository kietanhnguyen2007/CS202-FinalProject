#pragma once

#include "raylib.h"
#include "Utils/Types.h"
#include "Systems/ObservableList.h"
#include <vector>
#include <string>
#include <functional>

class Player;

namespace View {

struct SkillSlotData {
    SkillType type = SkillType::Fireball;
    float cooldown = 0.0f;
    float currentTimer = 0.0f;
    bool IsReady() const { return currentTimer <= 0.0f; }
};

class SkillBarView {
public:
    static SkillBarView& GetInstance();

    void Init();
    bool LoadResources(const std::string& atlasJsonPath);
    void Shutdown();

    void Update(float dt, const Player* player);
    void Render();

    void Open();
    void Close();
    bool IsOpen() const;

    void SetSelection(int index);

    // Bind to an ObservableList<SkillSlotData> for auto-update
    void AttachObservable(ObservableList<SkillSlotData>* observable);
    void DetachObservable();

private:
    SkillBarView() = default;
    ~SkillBarView() = default;

    static std::string SkillLabel(SkillType t);

    bool m_open = true;
    int m_selection = -1;
    bool m_loaded = false;

    // Local cache of skill data (from observable or manual set)
    std::vector<SkillSlotData> m_skills;

    ObservableList<SkillSlotData>* m_attachedObservable = nullptr;
    std::function<void()> m_onDataChanged;
};

} // namespace View