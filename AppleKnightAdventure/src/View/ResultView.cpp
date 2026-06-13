#include "View/ResultView.h"
#include "View/Renderer.h"
#include <cstdio>

namespace View {

ResultView& ResultView::GetInstance() {
    static ResultView inst;
    return inst;
}

bool ResultView::Init() {
    return true;
}

bool ResultView::LoadResources(const std::string& atlasJsonPath) {
    (void)atlasJsonPath;
    m_uiAtlas = Animations::TextureAtlas::LoadFromJSON("assets/textures/ui_atlas.json");
    if (m_uiAtlas) m_uiAtlas->LoadTexture();
    return true;
}

void ResultView::Shutdown() {
    m_uiAtlas.reset();
}

void ResultView::Dismiss() { m_visible = false; m_gameOver = false; m_anim = 0.0f; }

void ResultView::Show(const LevelResultSnapshot& snap) {
    m_snap = snap;
    m_visible = true;
    m_gameOver = false;
    m_anim = 0.0f;
}

void ResultView::ShowGameOver(const LevelResultSnapshot& snap) {
    m_snap = snap;
    m_visible = true;
    m_gameOver = true;
    m_anim = 0.0f;
}

void ResultView::Update(float dt) {
    if (!m_visible) return;
    m_anim += dt;
    if (m_anim > 1.0f) m_anim = 1.0f;
}

void ResultView::Render() {
    if (!m_visible) return;
    Renderer& r = Renderer::GetInstance();
    int w = r.GetWindowWidth();
    int h = r.GetWindowHeight();

    Vector2 center = { w*0.5f, h*0.4f };
    const float panelW = 440.0f;
    const float panelH = 280.0f;

    // Determine tint/color based on game over vs win
    Color panelColor = m_gameOver ? (Color){40, 10, 10, 220} : (Color){20, 20, 20, 200};

    // Draw panel background using health_bar sprite if available
    if (m_uiAtlas && m_uiAtlas->HasFrame("health_bar")) {
        Rectangle src = m_uiAtlas->GetFrameRect("health_bar");
        Texture2D* tex = m_uiAtlas->GetTexture();
        // Stretch to fill panel
        r.SubmitSprite(tex, src, {center.x - panelW * 0.5f, center.y - panelH * 0.5f},
                       {panelW / src.width, panelH / src.height},
                       0.0f, {0, 0}, panelColor, Layer::UI, 0.0f, false, 0);
    } else {
        r.DrawRectangle({center.x - panelW * 0.5f, center.y - panelH * 0.5f},
                        {panelW, panelH}, panelColor, Layer::UI, 0.0f);
    }

    if (m_gameOver) {
        r.DrawText("GAME OVER", {center.x - 100, center.y - 80}, 40, RED);
        char buf[128];
        snprintf(buf, sizeof(buf), "Score: %d", m_snap.score);
        r.DrawText(buf, {center.x - 180, center.y - 20}, 20, WHITE);
        snprintf(buf, sizeof(buf), "Kills: %d", m_snap.enemiesKilled);
        r.DrawText(buf, {center.x - 180, center.y + 10}, 20, WHITE);
        r.DrawText("Press ENTER to retry", {center.x - 130, center.y + 60}, 18, GRAY);
        return;
    }

    // Stars
    for (int i = 0; i < 3; ++i) {
        float x = center.x - 90 + i * 90;
        const char* star = (i < m_snap.stars) ? "*" : "o";
        char buf[8];
        snprintf(buf, sizeof(buf), "%s", star);
        r.DrawText(buf, {x, center.y - 60}, 48, GOLD);
    }

    char buf[128];
    snprintf(buf, sizeof(buf), "Time: %.2fs", m_snap.clearTime);
    r.DrawText(buf, {center.x - 180, center.y + 10}, 20, WHITE);
    snprintf(buf, sizeof(buf), "Kills: %d", m_snap.enemiesKilled);
    r.DrawText(buf, {center.x - 180, center.y + 40}, 20, WHITE);
    snprintf(buf, sizeof(buf), "Apples: %.0f%%", m_snap.applesPercent * 100.0f);
    r.DrawText(buf, {center.x - 180, center.y + 70}, 20, WHITE);
    snprintf(buf, sizeof(buf), "Score: %d", m_snap.score);
    r.DrawText(buf, {center.x - 180, center.y + 100}, 20, WHITE);
}

} // namespace View
