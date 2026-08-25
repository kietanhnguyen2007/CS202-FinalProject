#pragma once

#include "raylib.h"
#include "View/TextureAtlas.h"
#include "View/Animator.h"
#include "Model/Player.h"
#include <memory>
#include <string>

class Boss;

namespace View {

class HUDView {
public:
    static HUDView& GetInstance();

    bool Init();
    bool LoadResources(const std::string& atlasJsonPath);
    void Shutdown();
    void ClearEntityReferences();
    void Update(float dt, const Player* player, const Boss* boss = nullptr,
                const Player* secondPlayer = nullptr);
    void Render();

    void SetVisible(bool v) { m_visible = v; }
    void SetPlaytestMode(bool v) { m_isPlaytest = v; }
    bool WantsQuitTest();

private:
    HUDView() = default;
    void LoadAvatar(CharacterClass characterClass);
    void LoadSecondAvatar(CharacterClass characterClass);

    bool m_visible = true;
    bool m_loaded = false;
    bool m_isPlaytest = false;
    bool m_wantsQuitTest = false;
    const Player* m_player = nullptr;
    const Player* m_secondPlayer = nullptr;
    const Boss* m_boss = nullptr;
    const Boss* m_lastBoss = nullptr;

    float m_displayHp = 1.0f;
    float m_damageTrailHp = 1.0f;
    float m_secondDisplayHp = 1.0f;
    float m_secondDamageTrailHp = 1.0f;
    float m_bossDisplayHp = 1.0f;
    float m_bossDamageTrailHp = 1.0f;
    float m_time = 0.0f;

    Texture2D m_panelBrown{};
    Texture2D m_panelInsetBrown{};
    Texture2D m_buttonRoundBrown{};
    Texture2D m_barBackLeft{};
    Texture2D m_barBackMid{};
    Texture2D m_barBackRight{};
    Texture2D m_barRedLeft{};
    Texture2D m_barRedMid{};
    Texture2D m_barRedRight{};
    std::shared_ptr<Animations::TextureAtlas> m_avatarAtlas;
    Rectangle m_avatarSource{};
    int m_avatarClass = -1;
    std::shared_ptr<Animations::TextureAtlas> m_secondAvatarAtlas;
    Rectangle m_secondAvatarSource{};
    int m_secondAvatarClass = -1;
    std::shared_ptr<Animations::TextureAtlas> m_coinAtlas;
    Animations::Animator m_coinAnim;
};

} // namespace View
