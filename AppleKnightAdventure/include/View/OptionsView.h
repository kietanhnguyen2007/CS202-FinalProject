#pragma once
#include "raylib.h"
#include <string>

namespace View {

class OptionsView {
public:
    static OptionsView& GetInstance();

    bool Init();
    void Shutdown();

    void Update(float dt);
    void Render();
    void SetVisible(bool v);
    bool IsVisible() const { return m_visible; }

    // Returns true once user clicked/pressed Back
    bool WantsBack() const { return m_wantsBack; }
    void ClearWantsBack() { m_wantsBack = false; }

private:
    OptionsView() = default;
    OptionsView(const OptionsView&) = delete;
    OptionsView& operator=(const OptionsView&) = delete;

    bool m_visible   = false;
    bool m_wantsBack = false;
    float m_animTime = 0.0f;

    // Back button hover lerp
    float m_backScale     = 1.0f;
    float m_backGlowAlpha = 0.0f;
    bool  m_backHovered   = false;
    int   m_selectedRow   = 0;
    bool  m_draggingMusic = false;
    bool  m_draggingSFX   = false;
    float m_musicVolume   = 0.70f;
    float m_sfxVolume     = 0.80f;
    bool  m_fullscreen    = false;
    Font  m_font{};
    bool  m_fontLoaded    = false;

    void SetMusicVolume(float value, bool preview);
    void SetSFXVolume(float value, bool preview);
    bool IsFullscreenActive() const;
    void ApplyFullscreen(bool enabled);
    void ToggleFullscreenSetting();
    void SaveSettings();
};

} // namespace View
