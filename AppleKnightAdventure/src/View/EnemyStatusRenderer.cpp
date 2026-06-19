#include "View/EnemyStatusRenderer.h"
#include "View/EntityRenderer.h"
#include "View/Renderer.h"
#include "View/FloatingText.h"
#include "View/TextureAtlas.h"
#include "View/UIResourceManager.h"

using namespace View::Animations;

namespace View {

EnemyStatusRenderer& EnemyStatusRenderer::GetInstance() {
    static EnemyStatusRenderer inst;
    return inst;
}

bool EnemyStatusRenderer::LoadResources(const std::string& atlasJsonPath) {
    if (atlasJsonPath.empty()) { m_loaded = true; return true; }
    m_atlas = TextureAtlas::LoadFromJSON(atlasJsonPath);
    if (!m_atlas) {
        m_loaded = false;
        return false;
    }
    if (!m_atlas->LoadTexture()) {
        m_loaded = false;
        return false;
    }
    m_loaded = true;
    return true;
}

void EnemyStatusRenderer::Update(float dt) {
    // nothing for now
}

void EnemyStatusRenderer::Render(const Camera2D& camera) {
    Renderer& r = Renderer::GetInstance();
    auto& res = UIResourceManager::GetInstance();
    Texture2D* texFire = res.GetFireIcon();
    Texture2D* texWater = res.GetWaterIcon();
    Texture2D* texLightning = res.GetLightningIcon();

    for (const auto& kv : m_status) {
        uint32_t id = kv.first;
        const auto& flags = kv.second;
        const Entity* e = EntityRenderer::GetInstance().GetEntityPtr(id);
        if (!e) continue;
        Vector2 pos = e->GetPosition();

        float iconSize = 16.0f;
        float offsetX = -16.0f;

        auto drawIcon = [&](Texture2D* tex, bool flag, const char* fallback, Color fbColor) {
            if (flag) {
                if (tex && tex->id > 0) {
                    Rectangle src = {0.0f, 0.0f, (float)tex->width, (float)tex->height};
                    r.SubmitSprite(tex, src, {pos.x + offsetX, pos.y - 24}, {iconSize/src.width, iconSize/src.height}, 0.0f, {0,0}, WHITE, Layer::Foreground, 0.0f, false, id);
                    offsetX += iconSize + 2.0f;
                } else {
                    r.DrawRectangle({pos.x + offsetX, pos.y - 24}, {iconSize, iconSize}, fbColor, Layer::Foreground, 0.0f);
                    offsetX += iconSize + 2.0f;
                }
            }
        };

        drawIcon(texFire, flags.burn, "B", ORANGE);
        drawIcon(texWater, flags.wet, "W", SKYBLUE);
        drawIcon(texLightning, flags.shocked, "S", YELLOW);
    }
}

void EnemyStatusRenderer::SetStatus(uint32_t entityId, const Vector2& worldPos, bool burn, bool wet, bool shocked) {
    auto& s = m_status[entityId];
    s.burn = burn; s.wet = wet; s.shocked = shocked;
    if (burn) FloatingTextManager::GetInstance().Emit(worldPos, "BURN", ORANGE, 0.8f);
    if (wet) FloatingTextManager::GetInstance().Emit(worldPos, "WET", SKYBLUE, 0.8f);
    if (shocked) FloatingTextManager::GetInstance().Emit(worldPos, "SHOCKED", YELLOW, 0.8f);
}

void EnemyStatusRenderer::ClearStatus(uint32_t entityId) {
    m_status.erase(entityId);
}

} // namespace View
