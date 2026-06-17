#pragma once

#include "raylib.h"
#include <string>

namespace View {

struct LevelResultSnapshot {
    int stars = 0; // 0..3
    float clearTime = 0.0f;
    int enemiesKilled = 0;
    float applesPercent = 0.0f;
    int score = 0;
};

class ResultView {
public:
    static ResultView& GetInstance();

    bool Init();
    bool LoadResources(const std::string& atlasJsonPath);
    void Shutdown();

    void Show(const LevelResultSnapshot& snap);
    void ShowGameOver(const LevelResultSnapshot& snap);
    void Dismiss();
    void Update(float dt);
    void Render();
    bool IsVisible() const { return m_visible; }
    bool IsGameOver() const { return m_visible && m_gameOver; }

private:
    ResultView() = default;
    LevelResultSnapshot m_snap;
    bool m_visible = false;
    bool m_gameOver = false;
    float m_anim = 0.0f;

    // Dark Dwellers textures
    Texture2D m_texPanel{};       // 9-slice panel background
    Texture2D m_texHeaderWin{};   // victory decorative header
    Texture2D m_texHeaderLose{};  // game over decorative header
    Texture2D m_texStarFilled{};  // star filled sprite sheet (5 frames)
    Texture2D m_texStarEmpty{};   // star empty sprite sheet (5 frames)
    Texture2D m_texBtn{};         // button sprite sheet (4 frames)

    // Cached frame widths for sprite sheets
    int m_starFilledFrameW = 0;
    int m_starEmptyFrameW = 0;
    int m_btnFrameW = 0;
};

} // namespace View
