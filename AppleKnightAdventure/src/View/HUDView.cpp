#include "View/HUDView.h"
#include "View/Renderer.h"
#include "View/UIHelpers.h"
#include "Utils/Constants.h"
#include <string>

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
    // atlas usage TBD; for now we rely on Renderer::DrawText / DrawRectangle
    (void)atlasJsonPath;
    m_loaded = true;
    return true;
}

void HUDView::Shutdown() {
    m_loaded = false;
}

void HUDView::Update(float dt, const Player* player) {
    (void)dt;
    m_player = player;
}

void HUDView::Render() {
    if (!m_visible) return;
    if (!m_loaded) return;

    Renderer& r = Renderer::GetInstance();
    int w = r.GetWindowWidth();
    int h = r.GetWindowHeight();

    // Draw simple HUD at top-left: HP bar and coins/apples/keys
    int padding = 10;
    int barW = 200;
    int barH = 18;

    if (m_player) {
        int hp = m_player->GetHealth();
        int maxHp = m_player->GetMaxHealth();
        float frac = (maxHp > 0) ? (float)hp / (float)maxHp : 0.0f;

        Vector2 pos = ScreenPercent(0.02f, 0.02f, w, h);
        r.DrawRectangle(pos, {(float)barW, (float)barH}, DARKGRAY, Layer::UI, 0.0f);
        r.DrawRectangle(pos, {(float)(barW * frac), (float)barH}, RED, Layer::UI, 0.0f);
        char buf[64];
        snprintf(buf, sizeof(buf), "HP: %d/%d", hp, maxHp);
        r.DrawText(buf, {pos.x + 4, pos.y - 2}, 14, WHITE);

        // Draw coins/apples/keys on the right
        Vector2 right = ScreenPercent(0.85f, 0.02f, w, h);
        char buf2[64];
        snprintf(buf2, sizeof(buf2), "Coins: %d", m_player->GetInventory().GetCoins());
        r.DrawText(buf2, right, 14, YELLOW);
    }
}

} // namespace View
