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

    m_uiAtlas = Animations::TextureAtlas::LoadFromJSON("assets/textures/ui_atlas.json");
    if (m_uiAtlas) m_uiAtlas->LoadTexture();

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
    m_coinAnim.Stop();
    m_uiAtlas.reset();
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

    int hp = m_player->GetHealth();
    int maxHp = m_player->GetMaxHealth();
    float frac = (maxHp > 0) ? (float)hp / (float)maxHp : 0.0f;

    Vector2 pos = ScreenPercent(0.02f, 0.02f, w, h);
    const float barW = 200.0f;
    const float barH = 18.0f;

    // HP bar background
    r.DrawRectangle(pos, {barW, barH}, DARKGRAY, Layer::UI, 0.0f);

    // HP bar fill using health_bar sprite
    if (m_uiAtlas && m_uiAtlas->HasFrame("health_bar")) {
        Rectangle src = m_uiAtlas->GetFrameRect("health_bar");
        Texture2D* tex = m_uiAtlas->GetTexture();
        float scaleX = barW / src.width;
        float scaleY = barH / src.height;
        // Background layer (dark)
        r.SubmitSprite(tex, src, pos, {scaleX, scaleY}, 0.0f, {0, 0},
                       (Color){60, 60, 60, 255}, Layer::UI, 0.0f, false, 0);
        // Fill layer
        Rectangle fillSrc = src;
        fillSrc.width *= frac;
        r.SubmitSprite(tex, fillSrc, pos, {scaleX, scaleY}, 0.0f, {0, 0},
                       WHITE, Layer::UI, 0.0f, false, 0);
    } else {
        // Fallback: red rectangle
        r.DrawRectangle(pos, {barW * frac, barH}, RED, Layer::UI, 0.0f);
    }

    // HP text
    char buf[64];
    snprintf(buf, sizeof(buf), "HP: %d/%d", hp, maxHp);
    r.DrawText(buf, {pos.x + 4, pos.y - 2}, 14, WHITE);

    // Coins display (icon + text)
    Vector2 right = ScreenPercent(0.85f, 0.02f, w, h);
    if (m_coinAnim.IsPlaying() && m_coinAnim.HasTexture()) {
        Rectangle src = m_coinAnim.GetCurrentSrcRect();
        Vector2 origin = m_coinAnim.GetCurrentOrigin();
        float iconSize = 20.0f;
        r.SubmitSprite(m_coinAnim.GetTexture(), src, right,
                       {iconSize / src.width, iconSize / src.height},
                       0.0f, origin, WHITE, Layer::UI, 0.0f, false, 0);
        right.x += iconSize + 4;
    }
    char buf2[64];
    snprintf(buf2, sizeof(buf2), "Coins: %d", m_player->GetInventory().GetCoins());
    r.DrawText(buf2, right, 14, YELLOW);
}

} // namespace View
