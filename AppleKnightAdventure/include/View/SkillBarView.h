#pragma once

#include "raylib.h"
#include "View/TextureAtlas.h"
#include "View/Animator.h"
#include "Utils/Types.h"
#include "Systems/ObservableList.h"
#include <vector>
#include <string>
#include <memory>
#include <functional>

class Player;

namespace View {

struct SkillSlotData {
    SkillType type = SkillType::Fireball;
    float cooldown = 0.0f;
    float currentTimer = 0.0f;
    bool IsReady() const { return currentTimer <= 0.0f; }
    bool operator==(const SkillSlotData& o) const {
        return type == o.type && cooldown == o.cooldown && currentTimer == o.currentTimer;
    }
};

struct SkillIcon {
    std::shared_ptr<Animations::TextureAtlas> atlas;
    Animations::Animator anim;
    bool animated = false;
    std::string clipName;
};

class SkillBarView {
public:
    static SkillBarView& GetInstance();

    bool Init();
    bool LoadResources(const std::string& atlasJsonPath);
    void Shutdown();

    void Update(float dt, const Player* player);
    void Render();

    void Open();
    void Close();
    bool IsOpen() const;

    void SetSelection(int index);

    void AttachObservable(ObservableList<SkillSlotData>* observable);
    void DetachObservable();

private:
    SkillBarView() = default;
    ~SkillBarView() = default;

    static std::string SkillLabel(SkillType t);
    void InitIcons();

    bool m_open = true;
    int m_selection = -1;
    bool m_loaded = false;

    std::vector<SkillSlotData> m_skills;
    std::vector<SkillIcon> m_skillIcons;

    ObservableList<SkillSlotData>* m_attachedObservable = nullptr;
    std::function<void()> m_onDataChanged;
};

} // namespace View