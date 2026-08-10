// =============================================================================
// ShopView.cpp — Apple Knight Adventure
// Two-panel shop: Left = scrollable grid, Right = detail + buy.
// Assets: darkDwellers UI set (tabs, portrait frame, 9-slice panel).
// Lock overlay drawn procedurally with raylib primitives.
// =============================================================================
#include "View/ShopView.h"
#include "View/Renderer.h"
#include "View/UIHelpers.h"
#include "View/UIResourceManager.h"
#include "Systems/SoundManager.h"
#include <cstdio>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

using namespace View;

// ─────────────────────────────────────────────────────────────────────────────
static inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }
static inline float EaseOutCubic(float t) { float f = 1.f - t; return 1.f - f*f*f; }
static inline float Clamp01(float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }

// ─────────────────────────────────────────────────────────────────────────────
// Singleton
// ─────────────────────────────────────────────────────────────────────────────
ShopView& ShopView::GetInstance() {
    static ShopView inst;
    return inst;
}

// ─────────────────────────────────────────────────────────────────────────────
bool ShopView::Init() {
    // Load tab sprites
    const char* tabNPath = "assets/ui/darkDwellers/20251117darkDwellersTabB1-Sheet.png";
    const char* tabAPath = "assets/ui/darkDwellers/20251117darkDwellersTabA1-Sheet.png";
    if (::FileExists(tabNPath)) { m_texTabNormal = ::LoadTexture(tabNPath); m_tabTexLoaded = true; }
    if (::FileExists(tabAPath)) { m_texTabActive = ::LoadTexture(tabAPath); }

    // Portrait frame
    const char* portPath = "assets/ui/darkDwellers/20251125portraitFrameA.png";
    if (::FileExists(portPath)) { m_texPortrait = ::LoadTexture(portPath); m_portraitLoaded = true; }

    // 9-slice panel
    const char* panelPath = "assets/ui/darkDwellers/20251029darkDwellers9SlicesA.png";
    if (::FileExists(panelPath)) { m_texPanel = ::LoadTexture(panelPath); m_panelLoaded = true; }

    m_visible      = false;
    m_wantsBack    = false;
    m_wantsBuy     = false;
    m_selectedIdx  = 0;
    m_activeTab    = ShopTab::Characters;
    m_scrollY      = 0.f;
    m_scrollYDisp  = 0.f;
    m_slideT       = 0.f;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
void ShopView::Shutdown() {
    if (m_tabTexLoaded)   { ::UnloadTexture(m_texTabNormal); ::UnloadTexture(m_texTabActive); m_tabTexLoaded = false; }
    if (m_portraitLoaded) { ::UnloadTexture(m_texPortrait);  m_portraitLoaded = false; }
    if (m_panelLoaded)    { ::UnloadTexture(m_texPanel);     m_panelLoaded = false; }
    m_previewAtlas.reset();
    m_loadedPreviewPath.clear();
    m_visible = false;
}

// ─────────────────────────────────────────────────────────────────────────────
void ShopView::Show(int currentCoins) {
    m_currentCoins = currentCoins;
    m_visible      = true;
    m_wantsBack    = false;
    m_wantsBuy     = false;
    m_slideT       = 0.f;
    m_selectedIdx  = 0;
    m_scrollY      = 0.f;
    m_scrollYDisp  = 0.f;
    m_shakeTimer   = 0.f;

    // Load preview for first item
    const auto& items = (m_activeTab == ShopTab::Characters) ? m_charItems : m_petItems;
    if (!items.empty()) {
        LoadPreview(items[0].idleAtlasPath);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void ShopView::MarkSelectedUnlocked() {
    auto& items = (m_activeTab == ShopTab::Characters) ? m_charItems : m_petItems;
    if (m_selectedIdx >= 0 && m_selectedIdx < (int)items.size()) {
        items[m_selectedIdx].isUnlocked = true;
    }
}

void ShopView::TriggerBuyShake() { m_shakeTimer = 0.35f; }

// ─────────────────────────────────────────────────────────────────────────────
// UpdateBtnAnim — hover lerp helper
// ─────────────────────────────────────────────────────────────────────────────
void ShopView::UpdateBtnAnim(BtnAnim& btn, Rectangle rect, float dt) {
    Vector2 mouse = ::GetMousePosition();
    btn.hovered     = ::CheckCollisionPointRec(mouse, rect);
    float tgt_scale = btn.hovered ? 1.10f : 1.0f;
    float tgt_glow  = btn.hovered ? 0.55f : 0.0f;
    float spd = 10.0f * dt;
    btn.scale     = Lerp(btn.scale,     tgt_scale, std::min(1.f, spd));
    btn.glowAlpha = Lerp(btn.glowAlpha, tgt_glow,  std::min(1.f, spd));
}

// ─────────────────────────────────────────────────────────────────────────────
void ShopView::DrawGlowBorder(Rectangle r, float glowAlpha, Color col) {
    if (glowAlpha < 0.01f) return;
    for (float t = 0; t < 8.f; t += 1.5f) {
        float fade = (1.f - t / 8.f) * glowAlpha;
        unsigned char a = (unsigned char)(fade * 255.f);
        ::DrawRectangleLinesEx({r.x-t, r.y-t, r.width+2*t, r.height+2*t}, 1.5f,
                               Color{col.r, col.g, col.b, a});
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// LoadPreview — load idle atlas for the selected entity
// ─────────────────────────────────────────────────────────────────────────────
void ShopView::LoadPreview(const std::string& atlasPath) {
    if (atlasPath == m_loadedPreviewPath) return;
    m_loadedPreviewPath = atlasPath;
    m_previewAtlas.reset();
    m_previewAnim = Animations::Animator{};

    if (atlasPath.empty() || !::FileExists(atlasPath.c_str())) return;

    auto atlas = Animations::TextureAtlas::LoadFromJSON(atlasPath);
    if (atlas && atlas->LoadTexture()) {
        m_previewAtlas = atlas;
        // Use LoadClipsFromAtlas to bind texture + all clips at once
        m_previewAnim.LoadClipsFromAtlas(*atlas);
        // Try common clip names
        for (const char* name : {"idle", "Idle", "IDLE", "walk", "move"}) {
            if (m_previewAnim.HasClip(name)) {
                m_previewAnim.Play(name);
                break;
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Update
// ─────────────────────────────────────────────────────────────────────────────
void ShopView::Update(float dt) {
    if (!m_visible) return;

    m_animTime += dt;
    m_slideT    = Clamp01(m_slideT + dt / 0.30f); // 0.30s slide-in

    // Smooth scroll
    m_scrollYDisp = Lerp(m_scrollYDisp, m_scrollY, std::min(1.f, 15.f * dt));

    // Update preview animator
    if (m_previewAtlas && m_previewAnim.IsPlaying()) {
        m_previewAnim.Update(dt);
    }

    // Shake timer
    if (m_shakeTimer > 0.f) {
        m_shakeTimer -= dt;
        m_shakeOffset = sinf(m_shakeTimer * 60.f) * 6.f;
        if (m_shakeTimer < 0.f) { m_shakeTimer = 0.f; m_shakeOffset = 0.f; }
    }

    int sw = ::GetScreenWidth();
    int sh = ::GetScreenHeight();

    // ── Layout constants ──────────────────────────────────────────────────
    float panelSlide   = EaseOutCubic(m_slideT);
    float totalPanelW  = sw * 0.88f;
    float totalPanelH  = sh * 0.82f;
    float panelX       = (sw - totalPanelW) * 0.5f;
    float panelY       = (sh - totalPanelH) * 0.5f + (1.f - panelSlide) * sh * 0.5f;

    float gridW  = totalPanelW * 0.50f;
    float detailX = panelX + gridW;
    float detailW = totalPanelW - gridW;

    float tabH = sh * 0.06f;
    float gridY = panelY + tabH + sh * 0.02f;

    // Cols/rows for grid
    constexpr int kCols = 3;
    float cellPad = gridW * 0.03f;
    float cellW   = (gridW - cellPad * (kCols + 1)) / kCols;
    float cellH   = cellW * 1.2f;

    const auto& items = (m_activeTab == ShopTab::Characters) ? m_charItems : m_petItems;
    int kRows  = ((int)items.size() + kCols - 1) / kCols;
    float totalGridH = kRows * (cellH + cellPad) + cellPad;
    float gridViewH  = totalPanelH - tabH - sh * 0.02f - sh * 0.05f; // minus bottom bar

    // Mouse scroll
    float wheel = ::GetMouseWheelMove();
    m_scrollY -= wheel * cellH * 0.5f;
    m_scrollY  = std::max(0.f, std::min(m_scrollY, std::max(0.f, totalGridH - gridViewH)));

    // ── Tab clicks ───────────────────────────────────────────────────────
    float tabW = gridW * 0.45f;
    Rectangle tabCharRect = { panelX + gridW * 0.02f, panelY, tabW, tabH };
    Rectangle tabPetRect  = { panelX + gridW * 0.02f + tabW + gridW * 0.02f, panelY, tabW, tabH };

    UpdateBtnAnim(m_tabBtnAnim[0], tabCharRect, dt);
    UpdateBtnAnim(m_tabBtnAnim[1], tabPetRect, dt);

    if (::IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (m_tabBtnAnim[0].hovered && m_activeTab != ShopTab::Characters) {
            m_activeTab   = ShopTab::Characters;
            m_selectedIdx = 0;
            m_scrollY     = 0.f;
            const auto& citems = m_charItems;
            if (!citems.empty()) LoadPreview(citems[0].idleAtlasPath);
        } else if (m_tabBtnAnim[1].hovered && m_activeTab != ShopTab::Pets) {
            m_activeTab   = ShopTab::Pets;
            m_selectedIdx = 0;
            m_scrollY     = 0.f;
            const auto& pitems = m_petItems;
            if (!pitems.empty()) LoadPreview(pitems[0].idleAtlasPath);
        }
    }

    // ── Grid cell clicks ─────────────────────────────────────────────────
    if (::IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse = ::GetMousePosition();
        for (int i = 0; i < (int)items.size(); ++i) {
            int col = i % kCols;
            int row = i / kCols;
            float cx = panelX + cellPad + col * (cellW + cellPad);
            float cy = gridY + cellPad + row * (cellH + cellPad) - m_scrollYDisp;
            Rectangle cr = { cx, cy, cellW, cellH };
            if (::CheckCollisionPointRec(mouse, cr) && cy > gridY - cellH && cy < gridY + gridViewH) {
                if (m_selectedIdx != i) {
                    m_selectedIdx = i;
                    LoadPreview(items[i].idleAtlasPath);
                }
            }
        }
    }

    // ── Keyboard Navigation ──────────────────────────────────────────────
    static float inputCooldown = 0.0f;
    if (inputCooldown > 0.0f) inputCooldown -= dt;

    if (inputCooldown <= 0.0f) {
        int dx = 0, dy = 0;
        if (::IsKeyPressed(KEY_LEFT) || ::IsKeyPressed(KEY_A)) dx = -1;
        if (::IsKeyPressed(KEY_RIGHT) || ::IsKeyPressed(KEY_D)) dx = 1;
        if (::IsKeyPressed(KEY_UP) || ::IsKeyPressed(KEY_W)) dy = -1;
        if (::IsKeyPressed(KEY_DOWN) || ::IsKeyPressed(KEY_S)) dy = 1;

        if (dx != 0 || dy != 0) {
            int newIdx = m_selectedIdx + dx + dy * kCols;
            // Prevent wrapping rows horizontally
            if (dx == -1 && (m_selectedIdx % kCols) == 0) newIdx = m_selectedIdx;
            if (dx == 1 && ((m_selectedIdx + 1) % kCols) == 0) newIdx = m_selectedIdx;

            if (newIdx >= 0 && newIdx < (int)items.size() && newIdx != m_selectedIdx) {
                m_selectedIdx = newIdx;
                LoadPreview(items[newIdx].idleAtlasPath);
                
                // Auto scroll
                int newRow = newIdx / kCols;
                float cellTop = cellPad + newRow * (cellH + cellPad);
                float cellBot = cellTop + cellH + cellPad;
                
                if (cellTop < m_scrollY) m_scrollY = cellTop;
                else if (cellBot > m_scrollY + gridViewH) m_scrollY = cellBot - gridViewH;
                
                inputCooldown = 0.15f;
            }
        }
    }

    // ── Back button ───────────────────────────────────────────────────────
    float backW = sw * 0.12f, backH = sh * 0.055f;
    float backX = panelX + totalPanelW - backW - sw * 0.01f;
    float backY = panelY + totalPanelH - backH - sh * 0.015f;
    UpdateBtnAnim(m_backBtnAnim, {backX, backY, backW, backH}, dt);

    if ((m_backBtnAnim.hovered && ::IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        || ::IsKeyPressed(KEY_ESCAPE)) {
        m_wantsBack = true;
    }

    // ── Buy button ────────────────────────────────────────────────────────
    float buyW = detailW * 0.55f, buyH = sh * 0.062f;
    float buyX = detailX + (detailW - buyW) * 0.5f + m_shakeOffset;
    float buyY = panelY + totalPanelH - buyH - sh * 0.02f - backH - sh * 0.01f;
    UpdateBtnAnim(m_buyBtnAnim, {buyX, buyY, buyW, buyH}, dt);

    if (m_buyBtnAnim.hovered && ::IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        m_wantsBuy = true;
    }
}

// =============================================================================
// Render
// =============================================================================
void ShopView::Render() {
    if (!m_visible) return;

    RenderBackground();
    RenderTabBar();
    RenderGrid();
    RenderDetailPanel();
    RenderBuyButton();
    RenderBackButton();
    RenderCoinBar();
}

// ─────────────────────────────────────────────────────────────────────────────
void ShopView::RenderBackground() {
    int sw = ::GetScreenWidth(), sh = ::GetScreenHeight();
    // Full-screen dark tinted overlay
    ::DrawRectangle(0, 0, sw, sh, Color{8, 5, 18, 245});

    float panelSlide  = EaseOutCubic(m_slideT);
    float totalPanelW = sw * 0.88f;
    float totalPanelH = sh * 0.82f;
    float panelX      = (sw - totalPanelW) * 0.5f;
    float panelY      = (sh - totalPanelH) * 0.5f + (1.f - panelSlide) * sh * 0.5f;

    // Main panel body
    if (m_panelLoaded && m_texPanel.id != 0) {
        int corner = m_texPanel.width / 3;
        NPatchInfo npi;
        npi.source = {0.f, 0.f, (float)m_texPanel.width, (float)m_texPanel.height};
        npi.left = npi.top = npi.right = npi.bottom = corner;
        npi.layout = 0;
        ::DrawTextureNPatch(m_texPanel, npi,
                            {panelX, panelY, totalPanelW, totalPanelH},
                            {0,0}, 0.f, WHITE);
    } else {
        ::DrawRectangle((int)panelX, (int)panelY, (int)totalPanelW, (int)totalPanelH,
                        Color{18, 12, 32, 240});
        ::DrawRectangleLinesEx({panelX, panelY, totalPanelW, totalPanelH}, 2.f,
                               Color{110, 80, 160, 200});
    }

    // Vertical divider between grid and detail
    float gridW  = totalPanelW * 0.50f;
    float divX   = panelX + gridW;
    ::DrawRectangle((int)divX, (int)(panelY+2), 2, (int)(totalPanelH-4),
                    Color{80, 60, 120, 160});

    // Panel title
    const char* title = "SHOP";
    int tfs = (int)(sh * 0.045f); if (tfs < 14) tfs = 14;
    int tw  = ::MeasureText(title, tfs);
    ::DrawText(title, sw/2 - tw/2, (int)(panelY - tfs - sh*0.012f), tfs,
               Color{255, 220, 80, 255});
}

// ─────────────────────────────────────────────────────────────────────────────
void ShopView::RenderCoinBar() {
    int sw = ::GetScreenWidth(), sh = ::GetScreenHeight();
    float panelSlide  = EaseOutCubic(m_slideT);
    float totalPanelW = sw * 0.88f;
    float totalPanelH = sh * 0.82f;
    float panelX      = (sw - totalPanelW) * 0.5f;
    float panelY      = (sh - totalPanelH) * 0.5f + (1.f - panelSlide) * sh * 0.5f;

    // Coin badge: top-right corner of panel
    float badgeW = totalPanelW * 0.18f;
    float badgeH = sh * 0.045f;
    float badgeX = panelX + totalPanelW - badgeW - sw * 0.005f;
    float badgeY = panelY + sh * 0.005f;

    ::DrawRectangle((int)badgeX, (int)badgeY, (int)badgeW, (int)badgeH,
                    Color{30, 22, 50, 230});
    ::DrawRectangleLinesEx({badgeX, badgeY, badgeW, badgeH}, 1.5f,
                           Color{200, 170, 60, 200});

    // Coin icon (circle)
    float iconR = badgeH * 0.32f;
    ::DrawCircle((int)(badgeX + iconR + 6), (int)(badgeY + badgeH/2), iconR,
                 Color{255, 210, 30, 255});
    ::DrawCircleLines((int)(badgeX + iconR + 6), (int)(badgeY + badgeH/2), iconR,
                      Color{200, 150, 20, 200});

    char coinBuf[32];
    snprintf(coinBuf, sizeof(coinBuf), "%d", m_currentCoins);
    int cfs = (int)(badgeH * 0.58f); if (cfs < 10) cfs = 10;
    int cw  = ::MeasureText(coinBuf, cfs);
    ::DrawText(coinBuf, (int)(badgeX + badgeW - cw - 8),
               (int)(badgeY + (badgeH - cfs) * 0.5f), cfs, Color{255, 235, 80, 255});
}

// ─────────────────────────────────────────────────────────────────────────────
void ShopView::RenderTabBar() {
    int sw = ::GetScreenWidth(), sh = ::GetScreenHeight();
    float panelSlide  = EaseOutCubic(m_slideT);
    float totalPanelW = sw * 0.88f;
    float totalPanelH = sh * 0.82f;
    float panelX      = (sw - totalPanelW) * 0.5f;
    float panelY      = (sh - totalPanelH) * 0.5f + (1.f - panelSlide) * sh * 0.5f;
    float gridW       = totalPanelW * 0.50f;
    float tabH        = sh * 0.06f;
    float tabW        = gridW * 0.45f;

    const char* labels[] = {"Characters", "Pets"};
    float tabXs[] = { panelX + gridW * 0.02f,
                      panelX + gridW * 0.02f + tabW + gridW * 0.02f };
    ShopTab tabs[] = { ShopTab::Characters, ShopTab::Pets };

    for (int i = 0; i < 2; ++i) {
        float tx = tabXs[i], ty = panelY;
        bool  active = (m_activeTab == tabs[i]);

        float sc = m_tabBtnAnim[i].scale;
        float cx = tx + tabW * 0.5f, cy = ty + tabH * 0.5f;
        float sW = tabW * sc, sH = tabH * sc;
        Rectangle dr = { cx - sW*0.5f, cy - sH*0.5f, sW, sH };

        if (m_tabTexLoaded) {
            Texture2D& tex = active ? m_texTabActive : m_texTabNormal;
            if (tex.id != 0) {
                ::DrawTexturePro(tex, {0,0,(float)tex.width,(float)tex.height},
                                 dr, {0,0}, 0.f, WHITE);
            }
        } else {
            Color bg  = active ? Color{80, 55, 140, 230} : Color{35, 28, 55, 200};
            Color brd = active ? Color{220, 180, 80, 220} : Color{80, 60, 110, 180};
            ::DrawRectangleRec(dr, bg);
            ::DrawRectangleLinesEx(dr, active ? 2.f : 1.5f, brd);
        }

        DrawGlowBorder(dr, m_tabBtnAnim[i].glowAlpha, Color{220, 200, 80, 255});

        int tfs = (int)(tabH * 0.42f); if (tfs < 10) tfs = 10;
        int tw  = ::MeasureText(labels[i], tfs);
        Color tcol = active ? Color{255, 230, 80, 255} : Color{200, 190, 220, 200};
        ::DrawText(labels[i], (int)(cx - tw*0.5f), (int)(cy - tfs*0.5f), tfs, tcol);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void ShopView::RenderGrid() {
    int sw = ::GetScreenWidth(), sh = ::GetScreenHeight();
    float panelSlide  = EaseOutCubic(m_slideT);
    float totalPanelW = sw * 0.88f;
    float totalPanelH = sh * 0.82f;
    float panelX      = (sw - totalPanelW) * 0.5f;
    float panelY      = (sh - totalPanelH) * 0.5f + (1.f - panelSlide) * sh * 0.5f;
    float gridW       = totalPanelW * 0.50f;
    float tabH        = sh * 0.06f;
    float gridY       = panelY + tabH + sh * 0.015f;
    float gridViewH   = totalPanelH - tabH - sh * 0.085f;

    constexpr int kCols = 3;
    float cellPad = gridW * 0.025f;
    float cellW   = (gridW - cellPad * (kCols + 1)) / kCols;
    float cellH   = cellW * 1.25f;

    const auto& items = (m_activeTab == ShopTab::Characters) ? m_charItems : m_petItems;

    // Scissor test to clip grid to its viewport
    ::BeginScissorMode((int)panelX, (int)gridY, (int)gridW, (int)gridViewH);

    for (int i = 0; i < (int)items.size(); ++i) {
        int col = i % kCols;
        int row = i / kCols;
        float cx = panelX + cellPad + col * (cellW + cellPad);
        float cy = gridY  + cellPad + row * (cellH + cellPad) - m_scrollYDisp;

        // Skip off-screen cells
        if (cy + cellH < gridY || cy > gridY + gridViewH) continue;

        bool selected  = (m_selectedIdx == i);
        bool unlocked  = items[i].isUnlocked;

        if (m_panelLoaded && m_texPanel.id != 0) {
            int corner = m_texPanel.width / 3;
            NPatchInfo npi = { {0.f, 0.f, (float)m_texPanel.width, (float)m_texPanel.height}, corner, corner, corner, corner, 0 };
            Color tint = selected ? WHITE : (unlocked ? Color{200, 200, 200, 255} : Color{100, 100, 100, 200});
            ::DrawTextureNPatch(m_texPanel, npi, {cx, cy, cellW, cellH}, {0,0}, 0.f, tint);
            if (selected) {
                ::DrawRectangleLinesEx({cx, cy, cellW, cellH}, 2.5f, Color{255, 210, 50, 230});
            }
        } else {
            Color bg  = selected  ? Color{65, 48, 110, 235}
                      : unlocked  ? Color{30, 22, 50,  215}
                      :             Color{18, 14, 30,  200};
            Color brd = selected  ? Color{255, 210, 50, 230}
                      : unlocked  ? Color{120, 90, 160, 180}
                      :             Color{50,  40, 70,  160};

            ::DrawRectangle((int)cx, (int)cy, (int)cellW, (int)cellH, bg);
            ::DrawRectangleLinesEx({cx, cy, cellW, cellH}, selected ? 2.5f : 1.5f, brd);
        }

        // Load and draw idle sprite (first frame, static)
        if (!items[i].idleAtlasPath.empty() && ::FileExists(items[i].idleAtlasPath.c_str())) {
            // We only show the texture that's already loaded in previewAtlas if this is selected
            // Otherwise draw a placeholder character silhouette
        }

        // Name label
        int nfs = (int)(cellH * 0.115f); if (nfs < 9) nfs = 9;
        int nw  = ::MeasureText(items[i].displayName.c_str(), nfs);
        ::DrawText(items[i].displayName.c_str(),
                   (int)(cx + (cellW - nw) * 0.5f),
                   (int)(cy + cellH * 0.80f), nfs,
                   unlocked ? WHITE : Color{140,130,160,200});

        // Price or Unlocked badge
        if (unlocked) {
            const char* tag = "Owned";
            int tfs = (int)(cellH * 0.10f); if (tfs < 8) tfs = 8;
            int tw  = ::MeasureText(tag, tfs);
            ::DrawText(tag, (int)(cx + (cellW - tw)*0.5f),
                       (int)(cy + cellH*0.90f), tfs, Color{80,220,100,255});
        } else {
            char pBuf[24];
            snprintf(pBuf, sizeof(pBuf), "%d", items[i].price);
            int tfs = (int)(cellH * 0.10f); if (tfs < 8) tfs = 8;
            int tw  = ::MeasureText(pBuf, tfs);
            
            float coinRadius = cellH * 0.05f;
            float spacing = 4.0f;
            float totalWidth = tw + spacing + coinRadius * 2.0f;
            float startX = cx + (cellW - totalWidth) * 0.5f;
            
            ::DrawText(pBuf, (int)startX, (int)(cy + cellH*0.90f), tfs, Color{255,210,50,255});
            
            float iconCx = startX + tw + spacing + coinRadius;
            float iconCy = cy + cellH*0.90f + tfs*0.5f;
            ::DrawCircle((int)iconCx, (int)iconCy, (int)coinRadius, Color{255,200,30,200});
            ::DrawCircleLines((int)iconCx, (int)iconCy, (int)coinRadius, Color{200,150,20,200});
        }

        // Lock overlay for locked items
        if (!unlocked) {
            DrawLockOverlay(cx, cy, cellW, cellH * 0.76f);
        }
    }

    ::EndScissorMode();
}

// ─────────────────────────────────────────────────────────────────────────────
// DrawLockOverlay — draws a padlock shape procedurally (RPG style chain box)
// ─────────────────────────────────────────────────────────────────────────────
void ShopView::DrawLockOverlay(float x, float y, float w, float h) {
    // Dark semi-transparent tint
    ::DrawRectangle((int)x, (int)y, (int)w, (int)h, Color{0, 0, 0, 120});

    // Padlock centered
    float cx   = x + w * 0.5f;
    float cy   = y + h * 0.45f;
    float lW   = w * 0.26f;   // lock body width
    float lH   = h * 0.28f;   // lock body height
    float lBx  = cx - lW * 0.5f;
    float lBy  = cy;

    // Lock body (rectangle)
    ::DrawRectangle((int)lBx, (int)lBy, (int)lW, (int)lH, Color{60, 45, 90, 220});
    ::DrawRectangleLinesEx({lBx, lBy, lW, lH}, 2.f, Color{180, 150, 220, 240});

    // Lock shackle (top arch — drawn as thick circle arc approximated with lines)
    float shR  = lW * 0.36f;
    float shCx = cx;
    float shCy = lBy;
    int segments = 12;
    for (int s = 0; s < segments; ++s) {
        float a0 = (float)(180 + s * 180 / segments) * DEG2RAD;
        float a1 = (float)(180 + (s+1) * 180 / segments) * DEG2RAD;
        float x0 = shCx + cosf(a0) * shR, y0 = shCy + sinf(a0) * shR;
        float x1 = shCx + cosf(a1) * shR, y1 = shCy + sinf(a1) * shR;
        ::DrawLineEx({x0,y0},{x1,y1}, 3.f, Color{180,150,220,240});
    }

    // Keyhole dot in center of lock body
    ::DrawCircle((int)cx, (int)(lBy + lH * 0.40f), lW * 0.10f, Color{20, 15, 35, 255});
}

// ─────────────────────────────────────────────────────────────────────────────
void ShopView::RenderDetailPanel() {
    int sw = ::GetScreenWidth(), sh = ::GetScreenHeight();
    float panelSlide  = EaseOutCubic(m_slideT);
    float totalPanelW = sw * 0.88f;
    float totalPanelH = sh * 0.82f;
    float panelX      = (sw - totalPanelW) * 0.5f;
    float panelY      = (sh - totalPanelH) * 0.5f + (1.f - panelSlide) * sh * 0.5f;
    float gridW       = totalPanelW * 0.50f;
    float detailX     = panelX + gridW + 2;
    float detailW     = totalPanelW - gridW - 2;

    const auto& items = (m_activeTab == ShopTab::Characters) ? m_charItems : m_petItems;
    if (items.empty()) return;
    int idx = std::min(m_selectedIdx, (int)items.size()-1);
    const ShopItemData& item = items[idx];

    float innerX = detailX + detailW * 0.06f;
    float innerW = detailW * 0.88f;

    // ── Portrait area ─────────────────────────────────────────────────────
    float previewSize = std::min(detailW * 0.70f, totalPanelH * 0.38f);
    float previewX    = detailX + (detailW - previewSize) * 0.5f;
    float previewY    = panelY + totalPanelH * 0.04f;

    // Dark preview bg (fits perfectly under the frame, no extra borders)
    ::DrawRectangle((int)previewX, (int)previewY, (int)previewSize, (int)previewSize,
                    Color{15, 10, 25, 200});

    // Draw animated idle frame
    if (m_previewAtlas && m_previewAtlas->IsTextureLoaded() && m_previewAnim.HasTexture()) {
        Rectangle src  = m_previewAnim.GetCurrentSrcRect();
        bool flipX     = m_previewAnim.GetFlipX();

        float fw = fabsf(src.width), fh = fabsf(src.height);
        // Reduce scale to 0.55f so large characters (Ninja, Dragon) don't clip the frame
        float scale = (fw > 0 && fh > 0)
            ? std::min(previewSize / fw, previewSize / fh) * 0.55f
            : 1.f;
        float dw = fw * scale, dh = fh * scale;
        float dx = previewX + (previewSize - dw) * 0.5f;
        float dy = previewY + (previewSize - dh) * 0.5f;

        Texture2D* tex = m_previewAnim.GetCurrentTexture();
        if (tex) {
            Rectangle drawSrc = src;
            if (flipX) drawSrc.width = -drawSrc.width;
            ::DrawTexturePro(*tex, drawSrc, {dx, dy, dw, dh}, {0,0}, 0.f, WHITE);
        }
    } else {
        // Placeholder silhouette
        const char* ph = "?";
        int pfs = (int)(previewSize * 0.4f);
        int pw  = ::MeasureText(ph, pfs);
        ::DrawText(ph, (int)(previewX + (previewSize-pw)*0.5f),
                   (int)(previewY + (previewSize-pfs)*0.5f), pfs, Color{80,70,100,200});
    }

    // Portrait frame on top (exact same size as preview bg)
    if (m_portraitLoaded && m_texPortrait.id != 0) {
        ::DrawTexturePro(m_texPortrait,
                         {0,0,(float)m_texPortrait.width,(float)m_texPortrait.height},
                         {previewX, previewY, previewSize, previewSize},
                         {0,0}, 0.f, WHITE);
    }

    // ── Name ──────────────────────────────────────────────────────────────
    float nameY = previewY + previewSize + sh * 0.012f;
    int nfs = (int)(sh * 0.038f); if (nfs < 12) nfs = 12;
    int nw  = ::MeasureText(item.displayName.c_str(), nfs);
    ::DrawText(item.displayName.c_str(), (int)(innerX + (innerW-nw)*0.5f),
               (int)nameY, nfs, Color{255, 230, 80, 255});

    // ── Stats ─────────────────────────────────────────────────────────────
    float statY  = nameY + nfs + sh * 0.015f;
    int   sfs    = (int)(sh * 0.025f); if (sfs < 10) sfs = 10;
    float lineH  = sfs * 1.6f;

    auto drawStatRow = [&](const char* label, int val, Color barColor) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%s", label);
        ::DrawText(buf, (int)innerX, (int)statY, sfs, Color{200,190,220,220});

        // Value bar
        float barX = innerX + innerW * 0.42f;
        float barW = innerW * 0.56f;
        float barH = sfs * 0.65f;
        float fill = std::min((float)val / 200.f, 1.f);
        ::DrawRectangle((int)barX, (int)(statY + (sfs-barH)*0.5f),
                        (int)barW, (int)barH, Color{30,22,50,180});
        ::DrawRectangleGradientH((int)barX, (int)(statY + (sfs-barH)*0.5f),
                                 (int)(barW*fill), (int)barH,
                                 Color{(unsigned char)(barColor.r/2), (unsigned char)(barColor.g/2), (unsigned char)(barColor.b/2), 220},
                                 barColor);
        // Value number on right
        char vBuf[16]; snprintf(vBuf, sizeof(vBuf), "%d", val);
        int vw = ::MeasureText(vBuf, sfs);
        ::DrawText(vBuf, (int)(barX + barW + 6), (int)statY, sfs, WHITE);
        (void)vw;

        statY += lineH;
    };

    if (m_activeTab == ShopTab::Characters) {
        drawStatRow("HP",    item.statHP,    Color{80, 200, 100, 255});
        drawStatRow("ATK",   item.statATK,   Color{220, 80,  80,  255});
        drawStatRow("Speed", item.statSpeed, Color{80, 180, 220, 255});
    }

    // ── Description ───────────────────────────────────────────────────────
    if (!item.description.empty()) {
        statY += sh * 0.01f;
        int dfs = (int)(sh * 0.020f); if (dfs < 9) dfs = 9;
        ::DrawText(item.description.c_str(), (int)innerX, (int)statY, dfs,
                   Color{160,150,190,200});
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void ShopView::RenderBuyButton() {
    int sw = ::GetScreenWidth(), sh = ::GetScreenHeight();
    float panelSlide  = EaseOutCubic(m_slideT);
    float totalPanelW = sw * 0.88f;
    float totalPanelH = sh * 0.82f;
    float panelX      = (sw - totalPanelW) * 0.5f;
    float panelY      = (sh - totalPanelH) * 0.5f + (1.f - panelSlide) * sh * 0.5f;
    float gridW       = totalPanelW * 0.50f;
    float detailX     = panelX + gridW;
    float detailW     = totalPanelW - gridW;
    float backH       = sh * 0.055f;

    float buyW  = detailW * 0.55f;
    float buyH  = sh * 0.062f;
    float buyX  = detailX + (detailW - buyW) * 0.5f + m_shakeOffset;
    float buyY  = panelY + totalPanelH - buyH - sh * 0.02f - backH - sh * 0.01f;

    const auto& items = (m_activeTab == ShopTab::Characters) ? m_charItems : m_petItems;
    if (items.empty()) return;
    int idx = std::min(m_selectedIdx, (int)items.size()-1);
    bool unlocked  = items[idx].isUnlocked;
    bool canAfford = (m_currentCoins >= items[idx].price);

    // Scale around center
    float sc = m_buyBtnAnim.scale;
    float cx = buyX + buyW * 0.5f, cy = buyY + buyH * 0.5f;
    Rectangle dr = { cx - buyW*sc*0.5f, cy - buyH*sc*0.5f, buyW*sc, buyH*sc };

    Texture2D* btnTex = UIResourceManager::GetInstance().GetButton();
    float btnFrameW = UIResourceManager::GetInstance().GetButtonFrameWidth();

    if (unlocked) {
        // "Equipped" / "Select" state
        if (btnTex && btnTex->id != 0 && btnFrameW > 0) {
            Rectangle src = { 0.0f, 0.0f, btnFrameW, (float)btnTex->height };
            ::DrawTexturePro(*btnTex, src, dr, {0,0}, 0.f, Color{150, 255, 150, 255}); // greenish tint
        } else {
            ::DrawRectangleGradientV((int)dr.x,(int)dr.y,(int)dr.width,(int)dr.height,
                                     Color{40,100,50,200}, Color{60,160,80,200});
            ::DrawRectangleLinesEx(dr, 1.5f, Color{80,200,100,200});
        }
        const char* label = "Owned";
        int lfs = (int)(buyH * 0.50f); if (lfs < 10) lfs = 10;
        int lw  = ::MeasureText(label, lfs);
        ::DrawText(label, (int)(dr.x+(dr.width-lw)*0.5f),
                   (int)(dr.y+(dr.height-lfs)*0.5f), lfs, WHITE);
    } else {
        if (btnTex && btnTex->id != 0 && btnFrameW > 0) {
            int frame = (canAfford && m_buyBtnAnim.hovered) ? 1 : 0;
            Rectangle src = { (float)(frame * btnFrameW), 0.0f, btnFrameW, (float)btnTex->height };
            Color tint = canAfford ? WHITE : Color{120, 120, 120, 255};
            ::DrawTexturePro(*btnTex, src, dr, {0,0}, 0.f, tint);
        } else {
            Color bgTop = canAfford ? Color{80, 55, 140, 230} : Color{45, 38, 60, 180};
            Color bgBot = canAfford ? Color{130,90, 190, 230} : Color{55, 46, 70, 180};
            ::DrawRectangleGradientV((int)dr.x,(int)dr.y,(int)dr.width,(int)dr.height,
                                     bgTop, bgBot);
            Color brd = canAfford
                ? (m_buyBtnAnim.hovered ? Color{255,210,50,230} : Color{130,100,180,200})
                : Color{70,60,90,150};
            ::DrawRectangleLinesEx(dr, canAfford ? 2.f : 1.5f, brd);
        }

        if (canAfford) DrawGlowBorder(dr, m_buyBtnAnim.glowAlpha, Color{220,200,80,255});

        char lBuf[32];
        snprintf(lBuf, sizeof(lBuf), "%d", items[idx].price);
        int lfs = (int)(buyH * 0.48f); if (lfs < 10) lfs = 10;
        int lw  = ::MeasureText(lBuf, lfs);
        
        float coinRadius = buyH * 0.20f;
        float spacing = 8.0f;
        float totalWidth = lw + spacing + coinRadius * 2.0f;
        
        float startX = dr.x + (dr.width - totalWidth) * 0.5f;
        
        Color textCol = canAfford ? Color{255,235,80,255} : Color{130,120,150,200};
        ::DrawText(lBuf, (int)startX, (int)(dr.y+(dr.height-lfs)*0.5f), lfs, textCol);

        // Coin icon directly after text
        float iconCx = startX + lw + spacing + coinRadius;
        ::DrawCircle((int)iconCx, (int)(dr.y + dr.height*0.5f),
                     (int)coinRadius, Color{255,200,30,200});
        ::DrawCircleLines((int)iconCx, (int)(dr.y + dr.height*0.5f),
                     (int)coinRadius, Color{200,150,20,200});
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void ShopView::RenderBackButton() {
    int sw = ::GetScreenWidth(), sh = ::GetScreenHeight();
    float panelSlide  = EaseOutCubic(m_slideT);
    float totalPanelW = sw * 0.88f;
    float totalPanelH = sh * 0.82f;
    float panelX      = (sw - totalPanelW) * 0.5f;
    float panelY      = (sh - totalPanelH) * 0.5f + (1.f - panelSlide) * sh * 0.5f;

    float backW = sw * 0.12f, backH = sh * 0.055f;
    float backX = panelX + totalPanelW - backW - sw * 0.01f;
    float backY = panelY + totalPanelH - backH - sh * 0.015f;

    float sc = m_backBtnAnim.scale;
    float cx = backX + backW * 0.5f, cy = backY + backH * 0.5f;
    Rectangle dr = { cx - backW*sc*0.5f, cy - backH*sc*0.5f, backW*sc, backH*sc };

    Texture2D* btnTex = UIResourceManager::GetInstance().GetButton();
    float btnFrameW = UIResourceManager::GetInstance().GetButtonFrameWidth();
    
    if (btnTex && btnTex->id != 0 && btnFrameW > 0) {
        int frame = m_backBtnAnim.hovered ? 1 : 0;
        Rectangle src = { (float)(frame * btnFrameW), 0.0f, btnFrameW, (float)btnTex->height };
        ::DrawTexturePro(*btnTex, src, dr, {0,0}, 0.f, WHITE);
    } else {
        ::DrawRectangleGradientV((int)dr.x,(int)dr.y,(int)dr.width,(int)dr.height,
                                 Color{50,38,72,220}, Color{70,52,100,220});
        Color brd = m_backBtnAnim.hovered ? Color{220,180,80,220} : Color{100,80,140,180};
        ::DrawRectangleLinesEx(dr, 1.5f, brd);
    }
    DrawGlowBorder(dr, m_backBtnAnim.glowAlpha, Color{220,200,80,255});

    const char* label = "< Back";
    int lfs = (int)(backH * 0.52f); if (lfs < 10) lfs = 10;
    int lw  = ::MeasureText(label, lfs);
    ::DrawText(label, (int)(dr.x+(dr.width-lw)*0.5f),
               (int)(dr.y+(dr.height-lfs)*0.5f), lfs,
               m_backBtnAnim.hovered ? Color{255,230,80,255} : WHITE);
}
