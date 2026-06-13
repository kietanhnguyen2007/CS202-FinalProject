#pragma once

#include "raylib.h"
#include "View/TextureAtlas.h"
#include <string>
#include <memory>

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
    std::shared_ptr<Animations::TextureAtlas> m_uiAtlas;
};

} // namespace View
