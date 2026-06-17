#pragma once

#include "raylib.h"

namespace View {

// Centralized manager for common UI textures
class UIResourceManager {
public:
    static UIResourceManager& GetInstance();

    bool Init();
    void Shutdown();

    Texture2D* GetPanelBg() { return &m_texPanelBg; }
    Texture2D* GetSlot() { return &m_texSlot; }
    Texture2D* GetButton() { return &m_texButton; }
    Texture2D* GetCloseBtn() { return &m_texCloseBtn; }
    Texture2D* GetHeader() { return &m_texHeader; }

    // Helpers to get frame widths for horizontal sprite sheets
    float GetSlotFrameWidth() const { return m_slotFrameW; }
    float GetButtonFrameWidth() const { return m_buttonFrameW; }
    float GetCloseBtnFrameWidth() const { return m_closeBtnFrameW; }

private:
    UIResourceManager() = default;
    ~UIResourceManager() = default;

    Texture2D m_texPanelBg{};
    Texture2D m_texSlot{};
    Texture2D m_texButton{};
    Texture2D m_texCloseBtn{};
    Texture2D m_texHeader{};

    float m_slotFrameW = 0.0f;
    float m_buttonFrameW = 0.0f;
    float m_closeBtnFrameW = 0.0f;
};

} // namespace View
