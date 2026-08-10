#include "View/OptionsView.h"
#include <math.h>

namespace View {

OptionsView& OptionsView::GetInstance() {
    static OptionsView instance;
    return instance;
}

bool OptionsView::Init() {
    m_visible = false;
    m_wantsBack = false;
    m_animTime = 0.0f;
    m_backScale = 1.0f;
    m_backGlowAlpha = 0.0f;
    m_backHovered = false;
    return true;
}

void OptionsView::Shutdown() {
    m_visible = false;
    m_wantsBack = false;
}

void OptionsView::Update(float dt) {
    if (!m_visible) return;

    m_animTime += dt;
    
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    // Panel size and position base
    float panelW = 500.0f;
    float panelH = 400.0f;
    
    // Lerp slide in from top
    float t = m_animTime / 0.35f;
    if (t > 1.0f) t = 1.0f;
    // EaseOutCubic
    t = 1.0f - powf(1.0f - t, 3.0f);
    
    float panelX = (screenWidth - panelW) / 2.0f;
    float panelY = (screenHeight - panelH) / 2.0f;
    
    // Slide in effect applied to panelY
    panelY = -panelH + (panelY - (-panelH)) * t;

    // Back button rect logic for interaction
    float btnW = 140.0f;
    float btnH = 40.0f;
    float btnX = panelX + (panelW - btnW) / 2.0f;
    float btnY = panelY + panelH * 0.8f - btnH / 2.0f;
    
    Rectangle backBtnRect = { btnX, btnY, btnW, btnH };
    
    Vector2 mousePos = GetMousePosition();
    m_backHovered = CheckCollisionPointRec(mousePos, backBtnRect);
    
    // Lerp hover effects
    float targetScale = m_backHovered ? 1.12f : 1.0f;
    float targetGlow = m_backHovered ? 0.55f : 0.0f;
    
    m_backScale += (targetScale - m_backScale) * 10.0f * dt;
    m_backGlowAlpha += (targetGlow - m_backGlowAlpha) * 10.0f * dt;
    
    if (m_backHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        m_wantsBack = true;
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        m_wantsBack = true;
    }
}

void OptionsView::Render() {
    if (!m_visible) return;

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    // Dark overlay
    DrawRectangle(0, 0, screenWidth, screenHeight, Color{10, 8, 22, 240});

    // Panel size and pos
    float panelW = 500.0f;
    float panelH = 400.0f;
    
    float t = m_animTime / 0.35f;
    if (t > 1.0f) t = 1.0f;
    t = 1.0f - powf(1.0f - t, 3.0f); // EaseOutCubic
    
    float panelX = (screenWidth - panelW) / 2.0f;
    float panelY = (screenHeight - panelH) / 2.0f;
    panelY = -panelH + (panelY - (-panelH)) * t;

    // Draw panel background and border
    DrawRectangle(panelX, panelY, panelW, panelH, Color{25, 18, 45, 230});
    DrawRectangleLinesEx({panelX, panelY, panelW, panelH}, 2.0f, Color{120, 90, 170, 200});

    // Title
    const char* title = "OPTIONS";
    int titleFontSize = 40;
    int titleWidth = MeasureText(title, titleFontSize);
    DrawText(title, panelX + (panelW - titleWidth) / 2, panelY + 20, titleFontSize, GOLD);
    
    // Rows
    int labelFontSize = 20;
    int phFontSize = 20;
    float startY = panelY + 100.0f;
    float rowSpacing = 50.0f;

    const char* labels[] = { "Music Volume", "SFX Volume", "Fullscreen" };
    const char* placeholders[] = { "[Coming Soon]", "[Coming Soon]", "[Coming Soon]" };
    
    for (int i = 0; i < 3; ++i) {
        DrawText(labels[i], panelX + 50, startY + i * rowSpacing, labelFontSize, RAYWHITE);
        DrawText(placeholders[i], panelX + panelW - 50 - MeasureText(placeholders[i], phFontSize), startY + i * rowSpacing, phFontSize, GRAY);
    }

    // Back button
    float baseBtnW = 140.0f;
    float baseBtnH = 40.0f;
    float scaledBtnW = baseBtnW * m_backScale;
    float scaledBtnH = baseBtnH * m_backScale;
    float btnX = panelX + (panelW - scaledBtnW) / 2.0f;
    float btnY = panelY + panelH * 0.8f - scaledBtnH / 2.0f;
    
    Rectangle backRect = { btnX, btnY, scaledBtnW, scaledBtnH };

    // Gradient fill
    DrawRectangleGradientV(btnX, btnY, scaledBtnW, scaledBtnH, Color{60, 40, 90, 255}, Color{100, 70, 140, 255});
    
    // Border
    Color borderColor = m_backHovered ? Color{200, 180, 80, 200} : Color{80, 60, 110, 180};
    DrawRectangleLinesEx(backRect, 2.0f, borderColor);

    // Glow effect
    if (m_backGlowAlpha > 0.0f) {
        for (int i = 1; i <= 3; ++i) {
            float expand = i * 2.0f;
            Rectangle glowRect = { btnX - expand, btnY - expand, scaledBtnW + expand * 2, scaledBtnH + expand * 2 };
            Color glowColor = {200, 180, 80, (unsigned char)(255.0f * m_backGlowAlpha / i)};
            DrawRectangleLinesEx(glowRect, 1.0f, glowColor);
        }
    }

    // Button Text
    const char* btnText = "Back";
    int btnFontSize = 20;
    int btnTextW = MeasureText(btnText, btnFontSize);
    DrawText(btnText, btnX + (scaledBtnW - btnTextW) / 2.0f, btnY + (scaledBtnH - btnFontSize) / 2.0f, btnFontSize, WHITE);
}

} // namespace View
