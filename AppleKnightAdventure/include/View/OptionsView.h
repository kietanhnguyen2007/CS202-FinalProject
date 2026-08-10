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
    void SetVisible(bool v) { m_visible = v; }
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
};

} // namespace View
