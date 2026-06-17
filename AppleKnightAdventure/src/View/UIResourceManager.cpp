#include "View/UIResourceManager.h"
#include <iostream>

namespace View {

UIResourceManager& UIResourceManager::GetInstance() {
    static UIResourceManager inst;
    return inst;
}

bool UIResourceManager::Init() {
    // Load Dark Dwellers UI textures
    m_texPanelBg  = ::LoadTexture("assets/ui/darkDwellers/20251029darkDwellers9SlicesC.png");
    m_texSlot     = ::LoadTexture("assets/ui/darkDwellers/20251124emptyFrameA1-Sheet.png");
    m_texButton   = ::LoadTexture("assets/ui/darkDwellers/20251029darkDwellersButtonA1-Sheet.png");
    m_texCloseBtn = ::LoadTexture("assets/ui/darkDwellers/20251125closeButton1-Sheet.png");
    m_texHeader   = ::LoadTexture("assets/ui/darkDwellers/20251117darkDwellersHeaderC.png");

    // Calculate per-frame widths from horizontal sprite sheets
    if (m_texSlot.id != 0) {
        m_slotFrameW = m_texSlot.width / 5.0f;   // 5 frames in the sheet
    }
    if (m_texButton.id != 0) {
        m_buttonFrameW = m_texButton.width / 4.0f; // 4 frames in the sheet
    }
    if (m_texCloseBtn.id != 0) {
        m_closeBtnFrameW = m_texCloseBtn.width / 4.0f; // 4 frames in the sheet
    }

    return true;
}

void UIResourceManager::Shutdown() {
    if (m_texPanelBg.id != 0)  ::UnloadTexture(m_texPanelBg);
    if (m_texSlot.id != 0)     ::UnloadTexture(m_texSlot);
    if (m_texButton.id != 0)   ::UnloadTexture(m_texButton);
    if (m_texCloseBtn.id != 0) ::UnloadTexture(m_texCloseBtn);
    if (m_texHeader.id != 0)   ::UnloadTexture(m_texHeader);

    m_texPanelBg  = {};
    m_texSlot     = {};
    m_texButton   = {};
    m_texCloseBtn = {};
    m_texHeader   = {};

    m_slotFrameW = 0.0f;
    m_buttonFrameW = 0.0f;
    m_closeBtnFrameW = 0.0f;
}

} // namespace View
