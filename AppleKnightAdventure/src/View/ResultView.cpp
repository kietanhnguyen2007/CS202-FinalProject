#include "View/ResultView.h"
#include "View/Renderer.h"
#include "View/UIResourceManager.h"
#include "Systems/SoundManager.h"
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
    return true;
}

void ResultView::Shutdown() {
}

void ResultView::Dismiss() {
    m_visible  = false;
    m_gameOver = false;
    m_anim     = 0.0f;
}

void ResultView::Show(const LevelResultSnapshot& snap) {
    m_snap     = snap;
    m_visible  = true;
    m_gameOver = false;
    m_anim     = 0.0f;
    SoundManager::GetInstance().PlaySound("level_complete");
}

void ResultView::ShowGameOver(const LevelResultSnapshot& snap) {
    m_snap     = snap;
    m_visible  = true;
    m_gameOver = true;
    m_anim     = 0.0f;
    SoundManager::GetInstance().PlaySound("game_over");
}

void ResultView::Update(float dt) {
    if (!m_visible) return;
    // Animate from 0 → 1 over ~0.5 seconds (slide-in)
    if (m_anim < 1.0f) {
        m_anim += dt * 2.0f;
        if (m_anim > 1.0f) m_anim = 1.0f;
    }
}

void ResultView::Render() {
    if (!m_visible) return;

    Renderer& r = Renderer::GetInstance();
    float w = static_cast<float>(r.GetWindowWidth());
    float h = static_cast<float>(r.GetWindowHeight());

    // ── Ease-out for smooth slide-in ──
    // t goes 0→1; eased gives deceleration feel
    float t = m_anim;
    float eased = 1.0f - (1.0f - t) * (1.0f - t); // ease-out quad

    // ── Semi-transparent overlay (full screen) ──
    {
        Color overlay = {0, 0, 0, static_cast<unsigned char>(150.0f * eased)};
        r.DrawRectangle({0.0f, 0.0f}, {w, h}, overlay, Layer::UI, 0.0f);
    }

    // ── Panel dimensions (% of screen) ──
    float panelW = w * 0.34f;
    float panelH = h * 0.50f;
    float centerX = w * 0.5f;
    float centerY = h * 0.5f;

    // Slide-up from bottom: offset decreases as eased goes 0→1
    float slideOffset = (1.0f - eased) * h * 0.5f;
    float panelX = centerX - panelW * 0.5f;
    float panelY = centerY - panelH * 0.5f + slideOffset;

    auto& res = UIResourceManager::GetInstance();
    Texture2D* texPanel = res.GetPanelBg();
    Texture2D* texHeader = res.GetHeader();
    Texture2D* texSlot = res.GetSlot();
    Texture2D* texBtn = res.GetButton();
    Texture2D* texStar = res.GetStarIcon();
    float slotFrameW = res.GetSlotFrameWidth();
    float btnFrameW = res.GetButtonFrameWidth();

    // ── 1) Panel background ──
    if (texPanel && texPanel->id > 0) {
        int corner = texPanel->width / 3;
        NPatchInfo npi;
        npi.source = {0.0f, 0.0f, (float)texPanel->width, (float)texPanel->height};
        npi.left = corner; npi.top = corner; npi.right = corner; npi.bottom = corner;
        npi.layout = 0; // NPATCH_NINE_PATCH

        r.SubmitNPatch(texPanel, npi, {panelX, panelY, panelW, panelH}, WHITE, Layer::UI, 1.0f);
    } else {
        Color fallback = m_gameOver ? Color{40, 10, 10, 220} : Color{20, 20, 20, 200};
        r.DrawRectangle({panelX, panelY}, {panelW, panelH}, fallback, Layer::UI, 1.0f);
    }

    // ── 2) Decorative header across top of panel ──
    if (texHeader && texHeader->id > 0) {
        Rectangle hdrSrc = {0.0f, 0.0f, static_cast<float>(texHeader->width), static_cast<float>(texHeader->height)};
        // Stretch header to ~90% of panel width, keep aspect for height
        float headerDrawW = panelW * 0.90f;
        float headerDrawH = headerDrawW * (static_cast<float>(texHeader->height) / static_cast<float>(texHeader->width));
        float hdrScaleX = headerDrawW / static_cast<float>(texHeader->width);
        float hdrScaleY = headerDrawH / static_cast<float>(texHeader->height);
        float hdrX = panelX + (panelW - headerDrawW) * 0.5f;
        float hdrY = panelY + panelH * 0.02f;
        
        Color tint = m_gameOver ? Color{255, 100, 100, 255} : WHITE;
        r.SubmitSprite(texHeader, hdrSrc, {hdrX, hdrY},
                       {hdrScaleX, hdrScaleY}, 0.0f, {0, 0}, tint, Layer::UI, 2.0f, false, 0);
    }

    // ── 3) Title text ──
    {
        const char* title = m_gameOver ? "GAME OVER" : "LEVEL COMPLETE";
        int titleFontSize = static_cast<int>(h * 0.045f);
        // Approximate centering: measure roughly
        float titleW = static_cast<float>(::MeasureText(title, titleFontSize));
        float titleX = panelX + (panelW - titleW) * 0.5f;
        float titleY = panelY + panelH * 0.10f;
        Color titleColor = m_gameOver ? RED : GOLD;
        r.DrawText(title, {titleX, titleY}, titleFontSize, titleColor);
    }

    // ── 4) Stars row (victory only) ──
    float statsStartY = panelY + panelH * 0.28f; // default stats start
    if (!m_gameOver) {
        float starSize = panelW * 0.12f; // each star icon size
        float starSpacing = panelW * 0.04f;
        float totalStarsW = 3.0f * starSize + 2.0f * starSpacing;
        float starStartX = panelX + (panelW - totalStarsW) * 0.5f;
        float starY = panelY + panelH * 0.18f;

        for (int i = 0; i < 3; ++i) {
            float sx = starStartX + static_cast<float>(i) * (starSize + starSpacing);

            if (texStar && texStar->id > 0) {
                Color tint = (i < m_snap.stars) ? WHITE : DARKGRAY;
                Rectangle src = { 0.0f, 0.0f, (float)texStar->width, (float)texStar->height };
                float scaleX = starSize / (float)texStar->width;
                float scaleY = starSize / (float)texStar->height;
                r.SubmitSprite(texStar, src, {sx, starY},
                               {scaleX, scaleY}, 0.0f, {0, 0}, tint, Layer::UI, 3.0f, false, 0);
            } else {
                if (i < m_snap.stars) {
                    if (texSlot && texSlot->id > 0 && slotFrameW > 0) {
                        Rectangle src = { slotFrameW, 0.0f, slotFrameW, static_cast<float>(texSlot->height) };
                        float scaleX = starSize / slotFrameW;
                        float scaleY = starSize / static_cast<float>(texSlot->height);
                        r.SubmitSprite(texSlot, src, {sx, starY}, {scaleX, scaleY}, 0.0f, {0, 0}, GOLD, Layer::UI, 3.0f, false, 0);
                    } else {
                        r.DrawText("*", {sx, starY}, static_cast<int>(starSize), GOLD);
                    }
                } else {
                    if (texSlot && texSlot->id > 0 && slotFrameW > 0) {
                        Rectangle src = { 0.0f, 0.0f, slotFrameW, static_cast<float>(texSlot->height) };
                        float scaleX = starSize / slotFrameW;
                        float scaleY = starSize / static_cast<float>(texSlot->height);
                        r.SubmitSprite(texSlot, src, {sx, starY}, {scaleX, scaleY}, 0.0f, {0, 0}, GRAY, Layer::UI, 3.0f, false, 0);
                    } else {
                        r.DrawText("o", {sx, starY}, static_cast<int>(starSize), GRAY);
                    }
                }
            }
        }
        statsStartY = panelY + panelH * 0.38f;
    }

    // ── 5) Stats rows ──
    {
        int statFontSize = static_cast<int>(h * 0.028f);
        float statLineH = h * 0.045f;
        float statX = panelX + panelW * 0.15f;
        float curY = statsStartY;
        char buf[128];

        snprintf(buf, sizeof(buf), "Time:    %.2fs", m_snap.clearTime);
        r.DrawText(buf, {statX, curY}, statFontSize, WHITE);
        curY += statLineH;

        snprintf(buf, sizeof(buf), "Kills:   %d", m_snap.enemiesKilled);
        r.DrawText(buf, {statX, curY}, statFontSize, WHITE);
        curY += statLineH;

        snprintf(buf, sizeof(buf), "Apples:  %.0f%%", m_snap.applesPercent * 100.0f);
        r.DrawText(buf, {statX, curY}, statFontSize, WHITE);
        curY += statLineH;

        snprintf(buf, sizeof(buf), "Score:   %d", m_snap.score);
        r.DrawText(buf, {statX, curY}, statFontSize, GOLD);
    }

    // ── 6) Bottom button — "Press ENTER" ──
    {
        float btnDrawW = panelW * 0.50f;
        float btnDrawH = btnDrawW * 0.30f; // roughly keep button aspect
        float btnX = panelX + (panelW - btnDrawW) * 0.5f;
        float btnY = panelY + panelH * 0.82f;

        if (texBtn && texBtn->id > 0 && btnFrameW > 0) {
            // Frame 0 = normal state
            Rectangle btnSrc = {
                0.0f, 0.0f,
                btnFrameW, static_cast<float>(texBtn->height)
            };
            float btnScaleX = btnDrawW / btnFrameW;
            float btnScaleY = btnDrawH / static_cast<float>(texBtn->height);
            r.SubmitSprite(texBtn, btnSrc, {btnX, btnY},
                           {btnScaleX, btnScaleY}, 0.0f, {0, 0}, WHITE, Layer::UI, 3.0f, false, 0);
        } else {
            r.DrawRectangle({btnX, btnY}, {btnDrawW, btnDrawH}, Color{60, 60, 80, 200}, Layer::UI, 3.0f);
        }

        // Button label text
        const char* btnLabel = m_gameOver ? "Press ENTER to retry" : "Press ENTER to continue";
        int btnFontSize = static_cast<int>(h * 0.024f);
        float labelW = static_cast<float>(::MeasureText(btnLabel, btnFontSize));
        float labelX = btnX + (btnDrawW - labelW) * 0.5f;
        float labelY = btnY + (btnDrawH - static_cast<float>(btnFontSize)) * 0.5f;
        r.DrawText(btnLabel, {labelX, labelY}, btnFontSize, WHITE);
    }
}

} // namespace View
