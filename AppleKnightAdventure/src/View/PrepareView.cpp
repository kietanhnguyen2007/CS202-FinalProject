#include "View/PrepareView.h"
#include "Controller/PrepareController.h"
#include "View/UIResourceManager.h"
#include "Systems/SoundManager.h"
#include "View/Renderer.h"
#include "View/Animator.h"
#include "View/TextureAtlas.h"
#include <raylib.h>
#include <cmath>

namespace View {

PrepareView& PrepareView::GetInstance() {
    static PrepareView instance;
    return instance;
}

bool PrepareView::Init() {
    m_backBtnAnim = {};
    m_startBtnAnim = {};
    m_pulseTime = 0.f;
    return true;
}

void PrepareView::Shutdown() {
    m_charAtlas.reset();
    m_petAtlas.reset();
}

void PrepareView::Show() {
    m_backBtnAnim = {};
    m_startBtnAnim = {};
    m_pulseTime = 0.f;
    m_loadedCharPath = "";
    m_loadedPetPath = "";
}

void PrepareView::Update(float dt) {
    m_pulseTime += dt;
    
    Vector2 mousePos = GetMousePosition();
    bool leftClick = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    
    // Back button
    Rectangle backRect = {20, 20, 100, 40};
    m_backBtnAnim.hovered = CheckCollisionPointRec(mousePos, backRect);
    if (m_backBtnAnim.hovered && leftClick) {
        SoundManager::GetInstance().PlaySound("click");
        PrepareController::GetInstance().SetWantsBack();
    }
    
    auto& ctrl = PrepareController::GetInstance();
    
    // Start button logic in RenderStartButton but interaction handled here
    Rectangle startRect = {GetScreenWidth() / 2.0f - 100.f, GetScreenHeight() - 100.f, 200.f, 60.f};
    m_startBtnAnim.hovered = CheckCollisionPointRec(mousePos, startRect);
    if (m_startBtnAnim.hovered) {
        ctrl.SetFocusColumn(2);
        if (leftClick) {
            ctrl.TryStartGame();
        }
    }
    
    // Grid interactions
    float colW = 300.f;
    float colH = 400.f;
    float startY = 150.f;
    
    Rectangle charGrid = { 50.f, startY, colW, colH };
    if (CheckCollisionPointRec(mousePos, charGrid)) {
        ctrl.SetFocusColumn(0);
        float itemH = 60.f;
        for (int i = 0; i < (int)ctrl.GetCharItems().size(); ++i) {
            Rectangle itemRect = { charGrid.x + 10, charGrid.y + 10 + i * itemH, colW - 20, itemH - 10 };
            if (CheckCollisionPointRec(mousePos, itemRect) && leftClick) {
                ctrl.SetSelectedCharIdx(i);
                SoundManager::GetInstance().PlaySound("click");
            }
        }
    }
    
    Rectangle petGrid = { GetScreenWidth() - colW - 50.f, startY, colW, colH };
    if (CheckCollisionPointRec(mousePos, petGrid)) {
        ctrl.SetFocusColumn(1);
        float itemH = 60.f;
        for (int i = -1; i < (int)ctrl.GetPetItems().size(); ++i) {
            Rectangle itemRect = { petGrid.x + 10, petGrid.y + 10 + (i + 1) * itemH, colW - 20, itemH - 10 };
            if (CheckCollisionPointRec(mousePos, itemRect) && leftClick) {
                ctrl.SetSelectedPetIdx(i);
                SoundManager::GetInstance().PlaySound("click");
            }
        }
    }

    // Load Preview
    int cIdx = ctrl.GetSelectedCharIdx();
    int pIdx = ctrl.GetSelectedPetIdx();
    
    std::string targetCharPath = cIdx >= 0 ? ctrl.GetCharItems()[cIdx].idlePath : "";
    std::string targetPetPath = pIdx >= 0 ? ctrl.GetPetItems()[pIdx].idlePath : "";
    
    LoadPreview(targetCharPath, targetPetPath);
    
    m_charAnim.Update(dt);
    m_petAnim.Update(dt);
}

void PrepareView::Render() {
    auto& renderer = Renderer::GetInstance();
    auto& ui = UIResourceManager::GetInstance();
    
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.7f));
    
    // Title
    auto& ctrl = PrepareController::GetInstance();
    std::string title = "PREPARE: LEVEL " + std::to_string(ctrl.GetLevelId());
    ::DrawText(title.c_str(), GetScreenWidth() / 2 - ::MeasureText(title.c_str(), 40) / 2, 40, 40, WHITE);
    
    // Back button
    Rectangle backRect = {20, 20, 100, 40};
    DrawRectangleRec(backRect, m_backBtnAnim.hovered ? GRAY : DARKGRAY);
    ::DrawText("BACK", backRect.x + 20, backRect.y + 10, 20, WHITE);
    
    float colW = 300.f;
    float colH = 400.f;
    float startY = 150.f;
    
    // Grids
    RenderGrid(0, 50.f, startY, colW, colH);
    RenderGrid(1, GetScreenWidth() - colW - 50.f, startY, colW, colH);
    
    // Preview
    RenderPreview(GetScreenWidth() / 2.0f - 200.f, 200.f, 400.f, 300.f);
    
    // Start Button
    RenderStartButton(GetScreenWidth() / 2.0f - 100.f, GetScreenHeight() - 100.f, 200.f, 60.f);
}

void PrepareView::RenderGrid(int col, float x, float y, float w, float h) {
    auto& ui = UIResourceManager::GetInstance();
    auto& renderer = Renderer::GetInstance();
    auto& ctrl = PrepareController::GetInstance();
    
    // Draw Panel
    Texture2D* panelTex = ui.GetPanelBg();
    if (panelTex && panelTex->id != 0) {
        NPatchInfo npi = { {0, 0, (float)panelTex->width, (float)panelTex->height}, 10, 10, 10, 10, 0 };
        ::DrawTextureNPatch(*panelTex, npi, {x, y, w, h}, {0,0}, 0.f, WHITE);
    } else {
        DrawRectangle(x, y, w, h, DARKGRAY);
    }
    
    ::DrawText(col == 0 ? "Characters" : "Pets", x + w/2 - 60, y + 10, 24, WHITE);
    
    bool isFocusedCol = (ctrl.GetFocusColumn() == col);
    if (isFocusedCol) {
        DrawRectangleLinesEx({x-2, y-2, w+4, h+4}, 3.f, YELLOW);
    }
    
    float itemH = 60.f;
    float startY = y + 40.f;
    
    if (col == 0) {
        auto items = ctrl.GetCharItems();
        for (int i = 0; i < (int)items.size(); ++i) {
            Rectangle rect = { x + 10, startY + i * itemH, w - 20, itemH - 10 };
            bool isSelected = (ctrl.GetSelectedCharIdx() == i);
            
            Color bgColor = isSelected ? ORANGE : DARKGRAY;
            if (isFocusedCol && isSelected) bgColor = YELLOW;
            
            DrawRectangleRec(rect, Fade(bgColor, 0.8f));
            ::DrawText(items[i].displayName.c_str(), rect.x + 10, rect.y + 15, 20, WHITE);
            
            if (!items[i].isUnlocked) {
                DrawLockOverlay(rect.x, rect.y, rect.width, rect.height);
            }
        }
    } else {
        auto items = ctrl.GetPetItems();
        for (int i = -1; i < (int)items.size(); ++i) {
            Rectangle rect = { x + 10, startY + (i + 1) * itemH, w - 20, itemH - 10 };
            bool isSelected = (ctrl.GetSelectedPetIdx() == i);
            
            Color bgColor = isSelected ? ORANGE : DARKGRAY;
            if (isFocusedCol && isSelected) bgColor = YELLOW;
            
            DrawRectangleRec(rect, Fade(bgColor, 0.8f));
            std::string name = (i == -1) ? "None" : items[i].displayName;
            ::DrawText(name.c_str(), rect.x + 10, rect.y + 15, 20, WHITE);
            
            if (i >= 0 && !items[i].isUnlocked) {
                DrawLockOverlay(rect.x, rect.y, rect.width, rect.height);
            }
        }
    }
}

void PrepareView::RenderPreview(float x, float y, float w, float h) {
    // Draw shadow
    DrawEllipse(x + w/2, y + h - 20, w * 0.4f, 20.f, Fade(BLACK, 0.5f));
    
    // Draw animations
    float charScale = 3.f;
    Vector2 charPos = {x + w/2 - 40, y + h/2};
    if (m_charAtlas && m_charAtlas->IsTextureLoaded() && m_charAnim.HasTexture()) {
        Rectangle src = m_charAnim.GetCurrentSrcRect();
        Texture2D* tex = m_charAnim.GetCurrentTexture();
        if (tex) {
            float fw = fabsf(src.width) * charScale;
            float fh = fabsf(src.height) * charScale;
            ::DrawTexturePro(*tex, src, {charPos.x - fw/2, charPos.y - fh/2, fw, fh}, {0,0}, 0.f, WHITE);
        }
    }
    
    float petScale = 0.75f;
    Vector2 petPos = { charPos.x - 150.f, y + h/2 };
    if (m_petAtlas && m_petAtlas->IsTextureLoaded() && m_petAnim.HasTexture()) {
        Rectangle src = m_petAnim.GetCurrentSrcRect();
        Texture2D* tex = m_petAnim.GetCurrentTexture();
        if (tex) {
            float fw = fabsf(src.width) * petScale;
            float fh = fabsf(src.height) * petScale;
            ::DrawTexturePro(*tex, src, {petPos.x - fw/2, petPos.y - fh/2, fw, fh}, {0,0}, 0.f, WHITE);
        }
    }
}

void PrepareView::RenderStartButton(float x, float y, float w, float h) {
    auto& ui = UIResourceManager::GetInstance();
    auto& renderer = Renderer::GetInstance();
    auto& ctrl = PrepareController::GetInstance();
    
    Texture2D* btnTex = ui.GetButton();
    float scale = 1.0f;
    
    if (ctrl.GetFocusColumn() == 2) {
        scale = 1.0f + sinf(m_pulseTime * 5.f) * 0.05f;
    } else if (m_startBtnAnim.hovered) {
        scale = 1.05f;
    }
    
    float dw = w * scale;
    float dh = h * scale;
    float dx = x - (dw - w) / 2.0f;
    float dy = y - (dh - h) / 2.0f;
    if (btnTex && btnTex->id != 0) {
        float frameW = ui.GetButtonFrameWidth();
        float frameH = (float)btnTex->height;
        // Frame 0: normal, Frame 1: hover/focus
        float srcX = 0.0f;
        if (ctrl.GetFocusColumn() == 2 || m_startBtnAnim.hovered) {
            srcX = frameW; 
        }
        
        Rectangle src = { srcX, 0, frameW, frameH };
        ::DrawTexturePro(*btnTex, src, Rectangle{dx, dy, dw, dh}, {0,0}, 0.f, WHITE);
    } else {
        DrawRectangle(dx, dy, dw, dh, DARKGRAY);
    }
                     
    if (ctrl.GetFocusColumn() == 2) {
        DrawRectangleLinesEx({dx-2, dy-2, dw+4, dh+4}, 3.f, YELLOW);
    }
    
    const char* text = "START";
    int textWidth = ::MeasureText(text, 30);
    ::DrawText(text, dx + (dw - textWidth) / 2.0f, dy + (dh - 30) / 2.0f, 30, WHITE);
}

void PrepareView::DrawLockOverlay(float x, float y, float w, float h) {
    DrawRectangle(x, y, w, h, Fade(BLACK, 0.6f));
    ::DrawText("LOCKED", x + w/2 - 40, y + h/2 - 10, 20, RED);
}

void PrepareView::LoadPreview(const std::string& charPath, const std::string& petPath) {
    if (charPath != m_loadedCharPath) {
        m_loadedCharPath = charPath;
        if (!charPath.empty()) {
            m_charAtlas = Animations::TextureAtlas::LoadFromJSON(charPath);
            if (m_charAtlas && m_charAtlas->LoadTexture()) {
                m_charAnim.LoadClipsFromAtlas(*m_charAtlas);
                m_charAnim.Play("idle", true);
            }
        } else {
            m_charAtlas.reset();
        }
    }
    
    if (petPath != m_loadedPetPath) {
        m_loadedPetPath = petPath;
        if (!petPath.empty()) {
            m_petAtlas = Animations::TextureAtlas::LoadFromJSON(petPath);
            if (m_petAtlas && m_petAtlas->LoadTexture()) {
                m_petAnim.LoadClipsFromAtlas(*m_petAtlas);
                m_petAnim.Play("idle", true);
            }
        } else {
            m_petAtlas.reset();
        }
    }
}

} // namespace View
