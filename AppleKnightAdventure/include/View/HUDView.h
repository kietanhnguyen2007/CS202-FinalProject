#pragma once

#include "raylib.h"
#include "View/TextureAtlas.h"
#include "View/Animator.h"
#include "Model/Player.h"
#include <string>
#include <memory>

namespace View {

class HUDView {
public:
    static HUDView& GetInstance();

    bool Init();
    bool LoadResources(const std::string& atlasJsonPath);
    void Shutdown();

    void Update(float dt, const Player* player);
    void Render();

    void SetVisible(bool v) { m_visible = v; }

private:
    HUDView() = default;
    bool m_visible = true;
    const Player* m_player = nullptr;
    bool m_loaded = false;

    // Dark Dwellers HUD textures
    Texture2D m_texBarBg{};         // Bar background frame
    Texture2D m_texBarFill{};       // HP bar fill
    Texture2D m_texBarFillMP{};     // MP bar fill
    Texture2D m_texBarFillSP{};     // SP bar fill
    Texture2D m_texBarFillUlt{};    // Ultimate bar fill
    Texture2D m_texStatusSlot{};    // Buff/debuff slot sprite sheet (5 frames)
    Texture2D m_texPortrait{};      // Portrait frame decoration

    int m_statusSlotFrameW = 0;     // Width of a single frame in the status slot sheet

    // Coin icon animation (kept from original)
    std::shared_ptr<Animations::TextureAtlas> m_coinAtlas;
    Animations::Animator m_coinAnim;
};

} // namespace View
