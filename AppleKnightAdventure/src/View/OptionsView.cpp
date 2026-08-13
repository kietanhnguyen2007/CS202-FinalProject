#include "View/OptionsView.h"
#include "View/UIResourceManager.h"
#include "Systems/SoundManager.h"
#include "Model/SaveManager.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace View {
namespace {

struct OptionsLayout {
    Rectangle panel{};
    Rectangle musicSlider{};
    Rectangle sfxSlider{};
    Rectangle fullscreenButton{};
    Rectangle backButton{};
};

OptionsLayout BuildLayout(float animationTime) {
    const float screenW = static_cast<float>(GetScreenWidth());
    const float screenH = static_cast<float>(GetScreenHeight());
    const float panelW = std::min(650.0f, screenW * 0.88f);
    const float panelH = std::min(500.0f, screenH * 0.88f);
    const float finalX = (screenW - panelW) * 0.5f;
    const float finalY = (screenH - panelH) * 0.5f;
    float t = std::clamp(animationTime / 0.30f, 0.0f, 1.0f);
    t = 1.0f - std::pow(1.0f - t, 3.0f);
    const float panelY = -panelH + (finalY + panelH) * t;

    OptionsLayout layout;
    layout.panel = {finalX, panelY, panelW, panelH};
    const float controlX = finalX + panelW * 0.43f;
    const float controlW = panelW * 0.44f;
    const float sliderH = std::max(14.0f, panelH * 0.035f);
    layout.musicSlider = {controlX, panelY + panelH * 0.31f, controlW, sliderH};
    layout.sfxSlider = {controlX, panelY + panelH * 0.48f, controlW, sliderH};
    layout.fullscreenButton = {controlX, panelY + panelH * 0.61f,
                               std::min(170.0f, controlW), std::max(38.0f, panelH * 0.10f)};
    const float backW = std::min(190.0f, panelW * 0.34f);
    const float backH = std::max(38.0f, panelH * 0.105f);
    layout.backButton = {finalX + (panelW - backW) * 0.5f,
                         panelY + panelH * 0.80f, backW, backH};
    return layout;
}

Rectangle Expanded(Rectangle rect, float x, float y) {
    return {rect.x - x, rect.y - y, rect.width + x * 2.0f, rect.height + y * 2.0f};
}

} // namespace

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
    m_selectedRow = 0;
    m_draggingMusic = false;
    m_draggingSFX = false;

    if (!m_fontLoaded && FileExists("assets/fonts/game_font.ttf")) {
        m_font = LoadFont("assets/fonts/game_font.ttf");
        m_fontLoaded = m_font.texture.id != 0;
    }

    SaveManager& save = SaveManager::GetInstance();
    m_musicVolume = std::clamp(save.GetMusicVolume() / 100.0f, 0.0f, 1.0f);
    m_sfxVolume = std::clamp(save.GetSFXVolume() / 100.0f, 0.0f, 1.0f);
    SoundManager::GetInstance().SetMusicVolume(m_musicVolume);
    SoundManager::GetInstance().SetSFXVolume(m_sfxVolume);

    const bool wantsFullscreen = save.IsFullscreenEnabled();
    if (wantsFullscreen != IsFullscreenActive()) ApplyFullscreen(wantsFullscreen);
    m_fullscreen = IsFullscreenActive();
    return true;
}

void OptionsView::Shutdown() {
    if (m_visible) SaveSettings();
    if (m_fontLoaded) {
        UnloadFont(m_font);
        m_font = {};
        m_fontLoaded = false;
    }
    m_visible = false;
    m_wantsBack = false;
}

void OptionsView::SetVisible(bool visible) {
    if (visible && !m_visible) {
        m_animTime = 0.0f;
        m_wantsBack = false;
        m_backHovered = false;
        m_backScale = 1.0f;
        m_backGlowAlpha = 0.0f;
        m_draggingMusic = false;
        m_draggingSFX = false;
        m_musicVolume = SoundManager::GetInstance().GetMusicVolume();
        m_sfxVolume = SoundManager::GetInstance().GetSFXVolume();
        m_fullscreen = IsFullscreenActive();
    } else if (!visible && m_visible) {
        SaveSettings();
    }
    m_visible = visible;
}

void OptionsView::SetMusicVolume(float value, bool /*preview*/) {
    m_musicVolume = std::clamp(value, 0.0f, 1.0f);
    SoundManager::GetInstance().SetMusicVolume(m_musicVolume);
    SaveManager::GetInstance().SetMusicVolume(static_cast<int>(std::round(m_musicVolume * 100.0f)));
}

void OptionsView::SetSFXVolume(float value, bool preview) {
    m_sfxVolume = std::clamp(value, 0.0f, 1.0f);
    SoundManager::GetInstance().SetSFXVolume(m_sfxVolume);
    SaveManager::GetInstance().SetSFXVolume(static_cast<int>(std::round(m_sfxVolume * 100.0f)));
    if (preview) SoundManager::GetInstance().PlaySound("ui_confirm");
}

bool OptionsView::IsFullscreenActive() const {
    return IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
}

void OptionsView::ApplyFullscreen(bool enabled) {
    if (enabled != IsFullscreenActive()) {
        // Borderless fullscreen is considerably more reliable than exclusive
        // fullscreen on mixed-DPI/multi-monitor Windows setups. Raylib also
        // restores the previous window rectangle when toggled back.
        ToggleBorderlessWindowed();
    }
    m_fullscreen = IsFullscreenActive();
}

void OptionsView::ToggleFullscreenSetting() {
    ApplyFullscreen(!IsFullscreenActive());
    SaveManager::GetInstance().SetFullscreenEnabled(m_fullscreen);
    SoundManager::GetInstance().PlaySound("ui_confirm");
    SaveSettings();
}

void OptionsView::SaveSettings() {
    SaveManager& save = SaveManager::GetInstance();
    save.SetMusicVolume(static_cast<int>(std::round(m_musicVolume * 100.0f)));
    save.SetSFXVolume(static_cast<int>(std::round(m_sfxVolume * 100.0f)));
    save.SetFullscreenEnabled(IsFullscreenActive());
    save.Save();
}

void OptionsView::Update(float dt) {
    if (!m_visible) return;
    m_animTime += dt;

    const OptionsLayout layout = BuildLayout(m_animTime);
    const Vector2 mouse = GetMousePosition();
    const Rectangle musicHit = Expanded(layout.musicSlider, 8.0f, 18.0f);
    const Rectangle sfxHit = Expanded(layout.sfxSlider, 8.0f, 18.0f);
    const bool hoverMusic = CheckCollisionPointRec(mouse, musicHit);
    const bool hoverSFX = CheckCollisionPointRec(mouse, sfxHit);
    const bool hoverFullscreen = CheckCollisionPointRec(mouse, layout.fullscreenButton);
    const bool wasBackHovered = m_backHovered;
    m_backHovered = CheckCollisionPointRec(mouse, layout.backButton);

    int hoveredRow = -1;
    if (hoverMusic) hoveredRow = 0;
    else if (hoverSFX) hoveredRow = 1;
    else if (hoverFullscreen) hoveredRow = 2;
    else if (m_backHovered) hoveredRow = 3;
    if (hoveredRow >= 0 && hoveredRow != m_selectedRow) {
        m_selectedRow = hoveredRow;
        SoundManager::GetInstance().PlaySound("ui_hover");
    } else if (m_backHovered && !wasBackHovered) {
        SoundManager::GetInstance().PlaySound("ui_hover");
    }

    const float targetScale = m_backHovered ? 1.08f : 1.0f;
    const float targetGlow = m_backHovered ? 0.55f : 0.0f;
    m_backScale += (targetScale - m_backScale) * std::min(1.0f, 10.0f * dt);
    m_backGlowAlpha += (targetGlow - m_backGlowAlpha) * std::min(1.0f, 10.0f * dt);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (hoverMusic) {
            m_selectedRow = 0;
            m_draggingMusic = true;
        } else if (hoverSFX) {
            m_selectedRow = 1;
            m_draggingSFX = true;
        } else if (hoverFullscreen) {
            m_selectedRow = 2;
            ToggleFullscreenSetting();
        } else if (m_backHovered) {
            m_selectedRow = 3;
            SoundManager::GetInstance().PlaySound("ui_confirm");
            SaveSettings();
            m_wantsBack = true;
        }
    }

    if (m_draggingMusic && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        SetMusicVolume((mouse.x - layout.musicSlider.x) / layout.musicSlider.width, false);
    }
    if (m_draggingSFX && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        SetSFXVolume((mouse.x - layout.sfxSlider.x) / layout.sfxSlider.width, false);
    }
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        if (m_draggingSFX) SoundManager::GetInstance().PlaySound("ui_confirm");
        if (m_draggingMusic || m_draggingSFX) SaveSettings();
        m_draggingMusic = false;
        m_draggingSFX = false;
    }

    const int vertical = (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
        - (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W));
    if (vertical != 0) {
        m_selectedRow = (m_selectedRow + vertical + 4) % 4;
        SoundManager::GetInstance().PlaySound("ui_hover");
    }

    const int horizontal = (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D))
        - (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A));
    if (horizontal != 0) {
        if (m_selectedRow == 0) {
            SetMusicVolume(m_musicVolume + horizontal * 0.05f, false);
            SaveSettings();
        } else if (m_selectedRow == 1) {
            SetSFXVolume(m_sfxVolume + horizontal * 0.05f, true);
            SaveSettings();
        } else if (m_selectedRow == 2) {
            ToggleFullscreenSetting();
        }
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        if (m_selectedRow == 2) {
            ToggleFullscreenSetting();
        } else if (m_selectedRow == 3) {
            SoundManager::GetInstance().PlaySound("ui_confirm");
            SaveSettings();
            m_wantsBack = true;
        }
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        SoundManager::GetInstance().PlaySound("ui_confirm");
        SaveSettings();
        m_wantsBack = true;
    }
}

void OptionsView::Render() {
    if (!m_visible) return;

    const OptionsLayout layout = BuildLayout(m_animTime);
    const Rectangle panel = layout.panel;
    const Font font = m_fontLoaded ? m_font : GetFontDefault();
    const float titleSize = std::max(24.0f, panel.height * 0.09f);
    const float labelSize = std::max(14.0f, panel.height * 0.043f);
    const float smallSize = std::max(12.0f, panel.height * 0.034f);
    const float spacing = 1.0f;

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{7, 5, 16, 225});
    DrawRectangleRounded({panel.x + 8, panel.y + 10, panel.width, panel.height}, 0.055f, 12,
                         Color{4, 2, 10, 175});
    DrawRectangleRounded(panel, 0.055f, 12, Color{25, 18, 45, 252});
    DrawRectangleRoundedLinesEx(panel, 0.055f, 12, 3.0f, Color{184, 136, 62, 245});
    DrawRectangleRoundedLinesEx({panel.x + 9, panel.y + 9, panel.width - 18, panel.height - 18},
                                0.045f, 12, 1.0f, Color{103, 75, 145, 190});

    const char* title = "AUDIO & DISPLAY";
    Vector2 titleMeasure = MeasureTextEx(font, title, titleSize, spacing);
    DrawTextEx(font, title,
               {panel.x + (panel.width - titleMeasure.x) * 0.5f, panel.y + panel.height * 0.075f},
               titleSize, spacing, Color{255, 218, 105, 255});

    auto drawLabel = [&](const char* text, float y, bool selected) {
        DrawTextEx(font, text, {panel.x + panel.width * 0.09f, y}, labelSize, spacing,
                   selected ? Color{255, 235, 151, 255} : Color{226, 215, 242, 245});
    };

    auto drawSlider = [&](Rectangle rect, float value, bool selected) {
        const float centerY = rect.y + rect.height * 0.5f;
        Rectangle track = {rect.x, centerY - rect.height * 0.32f, rect.width, rect.height * 0.64f};
        DrawRectangleRounded(track, 0.5f, 10, Color{10, 8, 22, 255});
        DrawRectangleRounded({track.x, track.y, track.width * value, track.height}, 0.5f, 10,
                             selected ? Color{255, 194, 72, 255} : Color{126, 91, 190, 255});
        DrawRectangleRoundedLinesEx(track, 0.5f, 10, selected ? 2.0f : 1.0f,
                                    selected ? Color{255, 231, 143, 255} : Color{113, 88, 148, 220});
        const float knobRadius = std::max(8.0f, rect.height * 0.72f);
        const float knobX = rect.x + rect.width * value;
        if (selected) DrawCircleV({knobX, centerY}, knobRadius + 5.0f, Color{255, 196, 72, 55});
        DrawCircleV({knobX, centerY}, knobRadius, selected ? Color{255, 224, 126, 255} : RAYWHITE);
        DrawCircleLines(static_cast<int>(knobX), static_cast<int>(centerY), knobRadius,
                        Color{77, 48, 105, 255});
    };

    const float musicLabelY = layout.musicSlider.y - labelSize * 0.40f;
    const float sfxLabelY = layout.sfxSlider.y - labelSize * 0.40f;
    drawLabel("MUSIC VOLUME", musicLabelY, m_selectedRow == 0);
    drawLabel("SFX VOLUME", sfxLabelY, m_selectedRow == 1);
    drawSlider(layout.musicSlider, m_musicVolume, m_selectedRow == 0);
    drawSlider(layout.sfxSlider, m_sfxVolume, m_selectedRow == 1);

    char percent[16];
    std::snprintf(percent, sizeof(percent), "%d%%", static_cast<int>(std::round(m_musicVolume * 100.0f)));
    DrawTextEx(font, percent,
               {layout.musicSlider.x + layout.musicSlider.width - MeasureTextEx(font, percent, smallSize, 1.0f).x,
                layout.musicSlider.y - smallSize - 10.0f}, smallSize, 1.0f, Color{185, 172, 211, 255});
    std::snprintf(percent, sizeof(percent), "%d%%", static_cast<int>(std::round(m_sfxVolume * 100.0f)));
    DrawTextEx(font, percent,
               {layout.sfxSlider.x + layout.sfxSlider.width - MeasureTextEx(font, percent, smallSize, 1.0f).x,
                layout.sfxSlider.y - smallSize - 10.0f}, smallSize, 1.0f, Color{185, 172, 211, 255});

    drawLabel("FULLSCREEN", layout.fullscreenButton.y + (layout.fullscreenButton.height - labelSize) * 0.5f,
              m_selectedRow == 2);
    const bool fullSelected = m_selectedRow == 2;
    DrawRectangleRounded(layout.fullscreenButton, 0.25f, 10,
                         fullSelected ? Color{76, 52, 112, 255} : Color{39, 29, 62, 255});
    DrawRectangleRoundedLinesEx(layout.fullscreenButton, 0.25f, 10, fullSelected ? 2.5f : 1.5f,
                                fullSelected ? Color{255, 216, 100, 255} : Color{119, 91, 151, 220});
    const char* fullText = m_fullscreen ? "ON" : "OFF";
    const Vector2 fullMeasure = MeasureTextEx(font, fullText, labelSize, spacing);
    DrawTextEx(font, fullText,
               {layout.fullscreenButton.x + (layout.fullscreenButton.width - fullMeasure.x) * 0.5f,
                layout.fullscreenButton.y + (layout.fullscreenButton.height - fullMeasure.y) * 0.5f},
               labelSize, spacing, m_fullscreen ? Color{132, 245, 177, 255} : Color{226, 151, 151, 255});

    Rectangle backRect = layout.backButton;
    const float cx = backRect.x + backRect.width * 0.5f;
    const float cy = backRect.y + backRect.height * 0.5f;
    backRect.width *= m_backScale;
    backRect.height *= m_backScale;
    backRect.x = cx - backRect.width * 0.5f;
    backRect.y = cy - backRect.height * 0.5f;
    const bool backSelected = m_selectedRow == 3 || m_backHovered;
    if (m_backGlowAlpha > 0.01f || backSelected) {
        DrawRectangleRounded({backRect.x - 7, backRect.y - 7, backRect.width + 14, backRect.height + 14},
                             0.25f, 10, Color{255, 196, 72, static_cast<unsigned char>(45 + 40 * m_backGlowAlpha)});
    }
    DrawRectangleRounded(backRect, 0.22f, 10,
                         backSelected ? Color{86, 57, 126, 255} : Color{42, 30, 67, 255});
    DrawRectangleRoundedLinesEx(backRect, 0.22f, 10, backSelected ? 2.5f : 1.5f,
                                backSelected ? Color{255, 216, 100, 255} : Color{119, 91, 151, 220});
    const char* backText = "BACK";
    const Vector2 backMeasure = MeasureTextEx(font, backText, labelSize, spacing);
    DrawTextEx(font, backText,
               {backRect.x + (backRect.width - backMeasure.x) * 0.5f,
                backRect.y + (backRect.height - backMeasure.y) * 0.5f},
               labelSize, spacing, backSelected ? Color{255, 238, 174, 255} : RAYWHITE);

    const char* hint = "Mouse drag  |  Arrow keys / WASD  |  ESC back";
    const Vector2 hintMeasure = MeasureTextEx(font, hint, smallSize, 1.0f);
    DrawTextEx(font, hint,
               {panel.x + (panel.width - hintMeasure.x) * 0.5f, panel.y + panel.height * 0.94f},
               smallSize, 1.0f, Color{154, 140, 180, 210});
}

} // namespace View
