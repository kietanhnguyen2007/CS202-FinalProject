#include "View/HUDView.h"
#include "View/Renderer.h"
#include "View/UIHelpers.h"
#include "Utils/Constants.h"
#include <string>
#include <cstdio>

namespace View {

HUDView& HUDView::GetInstance() {
    static HUDView inst;
    return inst;
}

bool HUDView::Init() {
    m_loaded = true;
    return true;
}

bool HUDView::LoadResources(const std::string& atlasJsonPath) {
    (void)atlasJsonPath;

    // ---- Dark Dwellers HUD textures ----
    m_texBarBg     = ::LoadTexture("assets/ui/darkDwellers/20251118darkDwellersBarD.png");
    m_texBarFill   = ::LoadTexture("assets/ui/darkDwellers/20251118darkDwellersBarC.png");
    m_texStatusSlot = ::LoadTexture("assets/ui/darkDwellers/20251124emptyFrameB1-Sheet.png");
    m_texPortrait  = ::LoadTexture("assets/ui/darkDwellers/20251125portraitFrameA.png");

    // The status slot sheet has 5 equal frames laid out horizontally
    if (m_texStatusSlot.width > 0) {
        m_statusSlotFrameW = m_texStatusSlot.width / 5;
    }

    // ---- Coin atlas (kept from original) ----
    m_coinAtlas = Animations::TextureAtlas::LoadFromJSON("assets/textures/items/coin.json");
    if (m_coinAtlas) {
        m_coinAtlas->LoadTexture();
        m_coinAnim.SetTexture(m_coinAtlas->GetTexture());
        if (m_coinAtlas->HasClip("spin")) {
            m_coinAnim.AddClip(m_coinAtlas->GetClip("spin"));
            m_coinAnim.Play("spin");
        }
    }

    m_loaded = true;
    return true;
}

void HUDView::Shutdown() {
    // Unload Dark Dwellers textures
    ::UnloadTexture(m_texBarBg);
    ::UnloadTexture(m_texBarFill);
    ::UnloadTexture(m_texStatusSlot);
    ::UnloadTexture(m_texPortrait);

    m_texBarBg    = {};
    m_texBarFill  = {};
    m_texStatusSlot = {};
    m_texPortrait = {};
    m_statusSlotFrameW = 0;

    // Coin cleanup
    m_coinAnim.Stop();
    m_coinAtlas.reset();

    m_loaded = false;
}

void HUDView::Update(float dt, const Player* player) {
    m_player = player;
    m_coinAnim.Update(dt);
}

void HUDView::Render() {
    if (!m_visible || !m_loaded) return;

    Renderer& r = Renderer::GetInstance();
    int w = r.GetWindowWidth();
    int h = r.GetWindowHeight();

    if (!m_player) return;

    int hp    = m_player->GetHealth();
    int maxHp = m_player->GetMaxHealth();
    float frac = (maxHp > 0) ? (float)hp / (float)maxHp : 0.0f;
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;

    // ======================================================================
    // 1. Portrait Frame — top-left corner
    // ======================================================================
    // Position: 2% from left, 2% from top
    // Size: ~5% of screen width (maintain aspect ratio)
    Vector2 portraitPos = ScreenPercent(0.02f, 0.02f, w, h);
    float portraitW = w * 0.05f;
    float portraitAspect = (m_texPortrait.height > 0)
        ? (float)m_texPortrait.height / (float)m_texPortrait.width
        : 1.0f;
    float portraitH = portraitW * portraitAspect;

    if (m_texPortrait.id > 0) {
        Rectangle portraitSrc = {
            0.0f, 0.0f,
            (float)m_texPortrait.width,
            (float)m_texPortrait.height
        };
        float scaleX = portraitW / (float)m_texPortrait.width;
        float scaleY = portraitH / (float)m_texPortrait.height;
        r.SubmitSprite(&m_texPortrait, portraitSrc, portraitPos,
                       {scaleX, scaleY}, 0.0f, {0, 0},
                       WHITE, Layer::UI, 1.0f, false, 0);
    }

    // ======================================================================
    // 2. HP Bar Background — to the right of the portrait
    // ======================================================================
    // Positioned right after portrait with a small gap (~1% screen width)
    // Size: ~18% screen width, ~2.5% screen height
    float barGap = w * 0.01f;
    float barW   = w * 0.18f;
    float barH   = h * 0.025f;
    // Vertically center the bar relative to the portrait
    float barY = portraitPos.y + (portraitH - barH) * 0.5f;
    Vector2 barPos = { portraitPos.x + portraitW + barGap, barY };

    if (m_texBarBg.id > 0) {
        Rectangle bgSrc = {
            0.0f, 0.0f,
            (float)m_texBarBg.width,
            (float)m_texBarBg.height
        };
        float scaleX = barW / (float)m_texBarBg.width;
        float scaleY = barH / (float)m_texBarBg.height;
        r.SubmitSprite(&m_texBarBg, bgSrc, barPos,
                       {scaleX, scaleY}, 0.0f, {0, 0},
                       WHITE, Layer::UI, 0.0f, false, 0);
    }

    // ======================================================================
    // 3. HP Bar Fill — clipped by health fraction, drawn on top of BG
    // ======================================================================
    if (m_texBarFill.id > 0 && frac > 0.0f) {
        Rectangle fillSrc = {
            0.0f, 0.0f,
            (float)m_texBarFill.width * frac,
            (float)m_texBarFill.height
        };
        float scaleX = barW / (float)m_texBarFill.width;
        float scaleY = barH / (float)m_texBarFill.height;
        r.SubmitSprite(&m_texBarFill, fillSrc, barPos,
                       {scaleX, scaleY}, 0.0f, {0, 0},
                       WHITE, Layer::UI, 0.5f, false, 0);
    }

    // ======================================================================
    // 4. HP Text — on top of the bar
    // ======================================================================
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "HP: %d/%d", hp, maxHp);
        // Font size scales with screen height (~2% of height, minimum 10)
        int fontSize = (int)(h * 0.02f);
        if (fontSize < 10) fontSize = 10;
        // Center text vertically within the bar, slight horizontal padding
        float textX = barPos.x + barW * 0.05f;
        float textY = barPos.y + (barH - fontSize) * 0.5f;
        r.DrawText(buf, {textX, textY}, fontSize, WHITE);
    }

    // ======================================================================
    // 5. Coin Icon — right side of screen
    // ======================================================================
    Vector2 coinPos = ScreenPercent(0.85f, 0.02f, w, h);
    float coinIconSize = w * 0.025f; // ~2.5% of screen width

    if (m_coinAnim.IsPlaying() && m_coinAnim.HasTexture()) {
        Rectangle src = m_coinAnim.GetCurrentSrcRect();
        Vector2 origin = m_coinAnim.GetCurrentOrigin();
        float scaleX = coinIconSize / src.width;
        float scaleY = coinIconSize / src.height;
        r.SubmitSprite(m_coinAnim.GetTexture(), src, coinPos,
                       {scaleX, scaleY}, 0.0f, origin,
                       WHITE, Layer::UI, 0.0f, false, 0);
        coinPos.x += coinIconSize + w * 0.005f;
    }

    // ======================================================================
    // 6. Coin Text
    // ======================================================================
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "Coins: %d", m_player->GetInventory().GetCoins());
        int fontSize = (int)(h * 0.02f);
        if (fontSize < 10) fontSize = 10;
        r.DrawText(buf, coinPos, fontSize, YELLOW);
    }

    // ======================================================================
    // 7. Status Slots — row of 4 empty buff/debuff slots below HP bar
    // ======================================================================
    if (m_texStatusSlot.id > 0 && m_statusSlotFrameW > 0) {
        // Use frame 0 of the 5-frame horizontal sheet
        int frameH = m_texStatusSlot.height;
        Rectangle slotSrc = {
            0.0f, 0.0f,
            (float)m_statusSlotFrameW,
            (float)frameH
        };

        // Each slot is ~3% of screen width (square)
        float slotSize = w * 0.03f;
        float scaleX = slotSize / (float)m_statusSlotFrameW;
        float scaleY = slotSize / (float)frameH;

        // Position: below the HP bar with a small gap
        float slotY = barPos.y + barH + h * 0.008f;
        float slotStartX = barPos.x;
        float slotSpacing = w * 0.035f; // ~3.5% apart center-to-center

        for (int i = 0; i < 4; ++i) {
            Vector2 slotPos = { slotStartX + i * slotSpacing, slotY };
            r.SubmitSprite(&m_texStatusSlot, slotSrc, slotPos,
                           {scaleX, scaleY}, 0.0f, {0, 0},
                           WHITE, Layer::UI, 0.0f, false, 0);
        }
    }
}

} // namespace View
