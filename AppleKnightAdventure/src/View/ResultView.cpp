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

    // Load Dark Dwellers textures
    m_texPanel      = ::LoadTexture("assets/ui/darkDwellers/20251029darkDwellers9SlicesD.png");
    m_texHeaderWin  = ::LoadTexture("assets/ui/darkDwellers/20251117darkDwellersHeaderA.png");
    m_texHeaderLose = ::LoadTexture("assets/ui/darkDwellers/20251117darkDwellersHeaderB.png");
    m_texStarFilled = ::LoadTexture("assets/ui/darkDwellers/20251124emptyFrameA1-Sheet.png");
    m_texStarEmpty  = ::LoadTexture("assets/ui/darkDwellers/20251124emptyFrameC1-Sheet.png");
    m_texBtn        = ::LoadTexture("assets/ui/darkDwellers/20251029darkDwellersButtonB1-Sheet.png");

    // Cache frame widths for horizontal sprite sheets
    // Star filled sheet: 5 frames
    m_starFilledFrameW = (m_texStarFilled.width > 0) ? m_texStarFilled.width / 5 : 0;
    // Star empty sheet: 5 frames
    m_starEmptyFrameW  = (m_texStarEmpty.width > 0) ? m_texStarEmpty.width / 5 : 0;
    // Button sheet: 4 frames (Normal / Hover / Pressed / Disabled)
    m_btnFrameW        = (m_texBtn.width > 0) ? m_texBtn.width / 4 : 0;

    return true;
}

void ResultView::Shutdown() {
    ::UnloadTexture(m_texPanel);
    ::UnloadTexture(m_texHeaderWin);
    ::UnloadTexture(m_texHeaderLose);
    ::UnloadTexture(m_texStarFilled);
    ::UnloadTexture(m_texStarEmpty);
    ::UnloadTexture(m_texBtn);

    m_texPanel      = {};
    m_texHeaderWin  = {};
    m_texHeaderLose = {};
    m_texStarFilled = {};
    m_texStarEmpty  = {};
    m_texBtn        = {};
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
}

void ResultView::ShowGameOver(const LevelResultSnapshot& snap) {
    m_snap     = snap;
    m_visible  = true;
    m_gameOver = true;
    m_anim     = 0.0f;
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

    // ── 1) Panel background ──
    if (m_texPanel.id > 0) {
        Rectangle panelSrc = {0.0f, 0.0f, static_cast<float>(m_texPanel.width), static_cast<float>(m_texPanel.height)};
        float scaleX = panelW / static_cast<float>(m_texPanel.width);
        float scaleY = panelH / static_cast<float>(m_texPanel.height);
        r.SubmitSprite(&m_texPanel, panelSrc, {panelX, panelY},
                       {scaleX, scaleY}, 0.0f, {0, 0}, WHITE, Layer::UI, 1.0f, false, 0);
    } else {
        Color fallback = m_gameOver ? Color{40, 10, 10, 220} : Color{20, 20, 20, 200};
        r.DrawRectangle({panelX, panelY}, {panelW, panelH}, fallback, Layer::UI, 1.0f);
    }

    // ── 2) Decorative header across top of panel ──
    {
        Texture2D& headerTex = m_gameOver ? m_texHeaderLose : m_texHeaderWin;
        if (headerTex.id > 0) {
            Rectangle hdrSrc = {0.0f, 0.0f, static_cast<float>(headerTex.width), static_cast<float>(headerTex.height)};
            // Stretch header to ~90% of panel width, keep aspect for height
            float headerDrawW = panelW * 0.90f;
            float headerDrawH = headerDrawW * (static_cast<float>(headerTex.height) / static_cast<float>(headerTex.width));
            float hdrScaleX = headerDrawW / static_cast<float>(headerTex.width);
            float hdrScaleY = headerDrawH / static_cast<float>(headerTex.height);
            float hdrX = panelX + (panelW - headerDrawW) * 0.5f;
            float hdrY = panelY + panelH * 0.02f;
            r.SubmitSprite(&headerTex, hdrSrc, {hdrX, hdrY},
                           {hdrScaleX, hdrScaleY}, 0.0f, {0, 0}, WHITE, Layer::UI, 2.0f, false, 0);
        }
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

            if (i < m_snap.stars) {
                // Filled star: frame 1 (orange/highlighted) from 5-frame sheet
                if (m_texStarFilled.id > 0 && m_starFilledFrameW > 0) {
                    Rectangle src = {
                        static_cast<float>(1 * m_starFilledFrameW), 0.0f,
                        static_cast<float>(m_starFilledFrameW), static_cast<float>(m_texStarFilled.height)
                    };
                    float scale = starSize / static_cast<float>(m_starFilledFrameW);
                    r.SubmitSprite(&m_texStarFilled, src, {sx, starY},
                                   {scale, scale}, 0.0f, {0, 0}, WHITE, Layer::UI, 3.0f, false, 0);
                } else {
                    r.DrawText("*", {sx, starY}, static_cast<int>(starSize), GOLD);
                }
            } else {
                // Empty star: frame 0 from 5-frame sheet
                if (m_texStarEmpty.id > 0 && m_starEmptyFrameW > 0) {
                    Rectangle src = {
                        0.0f, 0.0f,
                        static_cast<float>(m_starEmptyFrameW), static_cast<float>(m_texStarEmpty.height)
                    };
                    float scale = starSize / static_cast<float>(m_starEmptyFrameW);
                    r.SubmitSprite(&m_texStarEmpty, src, {sx, starY},
                                   {scale, scale}, 0.0f, {0, 0}, WHITE, Layer::UI, 3.0f, false, 0);
                } else {
                    r.DrawText("o", {sx, starY}, static_cast<int>(starSize), GRAY);
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

        if (m_texBtn.id > 0 && m_btnFrameW > 0) {
            // Frame 0 = normal state
            Rectangle btnSrc = {
                0.0f, 0.0f,
                static_cast<float>(m_btnFrameW), static_cast<float>(m_texBtn.height)
            };
            float btnScaleX = btnDrawW / static_cast<float>(m_btnFrameW);
            float btnScaleY = btnDrawH / static_cast<float>(m_texBtn.height);
            r.SubmitSprite(&m_texBtn, btnSrc, {btnX, btnY},
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
