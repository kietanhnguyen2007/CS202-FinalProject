#pragma once

#include "raylib.h"
#include "View/TextureAtlas.h"
#include "Utils/Types.h"
#include <array>
#include <memory>
#include <string>

class Player;

namespace View {

struct GameplaySkillSlot {
    const char* key = "?";
    float cooldown = 0.0f;
    float timer = 0.0f;
    bool charging = false;
    bool active = false;
};

struct GameplaySkillIcon {
    std::shared_ptr<Animations::TextureAtlas> atlas;
    Rectangle source{};
};

class SkillBarView {
public:
    static SkillBarView& GetInstance();

    bool Init();
    bool LoadResources(const std::string& atlasJsonPath = "");
    void Shutdown();
    void Update(float dt, const Player* player);
    void Render();

    void Open() { m_open = true; }
    void Close() { m_open = false; }
    bool IsOpen() const { return m_open; }
    void SetSelection(int) {}

private:
    SkillBarView() = default;
    void LoadClassIcons(CharacterClass cls);
    void CopySkill(int index, const char* key, float cooldown, float timer, bool charging, bool active);

    bool m_open = true;
    bool m_loaded = false;
    const Player* m_player = nullptr;
    CharacterClass m_loadedClass = static_cast<CharacterClass>(-1);
    float m_time = 0.0f;
    std::array<GameplaySkillSlot, 4> m_skills{};
    std::array<GameplaySkillIcon, 4> m_icons{};
};

} // namespace View
