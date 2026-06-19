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
    m_texBarFillMP = ::LoadTexture("assets/ui/darkDwellers/20251118darkDwellersBarA.png");
    m_texBarFillSP = ::LoadTexture("assets/ui/darkDwellers/20251118darkDwellersBarF.png");
    m_texBarFillUlt= ::LoadTexture("assets/ui/darkDwellers/20251118darkDwellersBarJ.png");
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
    ::UnloadTexture(m_texBarFillMP);
    ::UnloadTexture(m_texBarFillSP);
    ::UnloadTexture(m_texBarFillUlt);
    ::UnloadTexture(m_texStatusSlot);
    ::UnloadTexture(m_texPortrait);

    m_texBarBg    = {};
    m_texBarFill  = {};
    m_texBarFillMP= {};
    m_texBarFillSP= {};
    m_texBarFillUlt={};
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
    // 2. Bars (HP, MP, SP, Ultimate) — to the right of the portrait
    // ======================================================================
    float barGap = w * 0.01f;
    float barW   = w * 0.16f;
    float barH   = h * 0.018f;
    float barSpacing = barH + h * 0.002f;
    
    // Align top of bars with top of portrait
    float startX = portraitPos.x + portraitW + barGap;
    float startY = portraitPos.y; 

    auto drawBar = [&](Texture2D& fillTex, Vector2 pos, float barFrac, const char* text) {
        if (m_texBarBg.id > 0) {
            Rectangle bgSrc = { 0.0f, 0.0f, (float)m_texBarBg.width, (float)m_texBarBg.height };
            float scaleX = barW / (float)m_texBarBg.width;
            float scaleY = barH / (float)m_texBarBg.height;
            r.SubmitSprite(&m_texBarBg, bgSrc, pos, {scaleX, scaleY}, 0.0f, {0, 0}, WHITE, Layer::UI, 0.0f, false, 0);
        }
        if (fillTex.id > 0 && barFrac > 0.0f) {
            Rectangle fillSrc = { 0.0f, 0.0f, (float)fillTex.width * barFrac, (float)fillTex.height };
            float scaleX = barW / (float)fillTex.width;
            float scaleY = barH / (float)fillTex.height;
            r.SubmitSprite(&fillTex, fillSrc, pos, {scaleX, scaleY}, 0.0f, {0, 0}, WHITE, Layer::UI, 0.5f, false, 0);
        }
        int fontSize = (int)(h * 0.015f);
        if (fontSize < 10) fontSize = 10;
        float textX = pos.x + barW * 0.05f;
        float textY = pos.y + (barH - fontSize) * 0.5f;
        r.DrawText(text, {textX, textY}, fontSize, WHITE);
    };

    char buf[64];

    // 1. HP
    snprintf(buf, sizeof(buf), "HP: %d/%d", hp, maxHp);
    drawBar(m_texBarFill, {startX, startY}, frac, buf);

    // 2. MP (chờ Model bổ sung GetMP/GetMaxMP)
    // TODO: int mp = m_player->GetMP();
    // TODO: int maxMp = m_player->GetMaxMP();
    // drawBar(m_texBarFillMP, {startX, startY + barSpacing}, (float)mp/maxMp, buf);

    // 3. SP (chờ Model bổ sung GetSP/GetMaxSP)
    // TODO: int sp = m_player->GetSP();
    // TODO: int maxSp = m_player->GetMaxSP();
    // drawBar(m_texBarFillSP, {startX, startY + barSpacing * 2.0f}, (float)sp/maxSp, buf);

    // 4. Ultimate (chờ Model bổ sung GetUltimateCharge/GetMaxUltimateCharge)
    // TODO: float ult = m_player->GetUltimateCharge();
    // TODO: float maxUlt = m_player->GetMaxUltimateCharge();
    // snprintf(buf, sizeof(buf), "ULT: %d%%", (int)(ult/maxUlt * 100));
    // drawBar(m_texBarFillUlt, {startX, startY + barSpacing * 3.0f}, ult/maxUlt, buf);

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

        // Position: below the bars with a small gap
        float slotY = startY + barSpacing * 4.0f + h * 0.008f;
        float slotStartX = startX;
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
