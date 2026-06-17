#include "View/InteractPrompt.h"
#include "View/Renderer.h"
#include "View/UIResourceManager.h"

namespace View {

InteractPrompt& InteractPrompt::GetInstance() {
    static InteractPrompt inst;
    return inst;
}

bool InteractPrompt::LoadResources(const std::string& atlasJsonPath) {
    (void)atlasJsonPath;
    m_texIcon = ::LoadTexture("assets/ui/darkDwellers/20251125helpButton1-Sheet.png");
    if (m_texIcon.id != 0) {
        m_iconFrameW = m_texIcon.width / 4;
    }
    return true;
}

void InteractPrompt::Shutdown() {
    ::UnloadTexture(m_texIcon);
    m_texIcon = {};
    m_iconFrameW = 0;
}

void InteractPrompt::Show(const std::string& text) {
    m_text = text;
    m_visible = true;
}

void InteractPrompt::Hide() {
    m_visible = false;
    m_text.clear();
}

bool InteractPrompt::IsVisible() const {
    return m_visible;
}

void InteractPrompt::Render() {
    if (!m_visible) return;
    Renderer& r = Renderer::GetInstance();
    int w = r.GetWindowWidth();
    int h = r.GetWindowHeight();

    Vector2 size = { 320.0f, 40.0f };
    Vector2 pos = { w * 0.5f - size.x * 0.5f, h * 0.85f };

    Texture2D* texPanel = UIResourceManager::GetInstance().GetPanelBg();
    if (texPanel && texPanel->id != 0) {
        int corner = texPanel->width / 3;
        NPatchInfo npi;
        npi.source = {0.0f, 0.0f, (float)texPanel->width, (float)texPanel->height};
        npi.left = corner; npi.top = corner; npi.right = corner; npi.bottom = corner;
        npi.layout = 0; // NPATCH_NINE_PATCH
        r.SubmitNPatch(texPanel, npi, {pos.x, pos.y, size.x, size.y}, WHITE, Layer::UI, 0.0f);
    } else {
        r.DrawRectangle(pos, size, { 0, 0, 0, 180 }, Layer::UI, 0.0f);
    }

    float textX = pos.x + 16;
    if (m_texIcon.id != 0 && m_iconFrameW > 0) {
        float iconSize = size.y * 0.8f;
        Rectangle iconSrc = { 0.0f, 0.0f, (float)m_iconFrameW, (float)m_texIcon.height };
        float iScaleX = iconSize / (float)m_iconFrameW;
        float iScaleY = iconSize / (float)m_texIcon.height;
        r.SubmitSprite(&m_texIcon, iconSrc, {pos.x + 8, pos.y + (size.y - iconSize) * 0.5f}, {iScaleX, iScaleY}, 0.0f, {0,0}, WHITE, Layer::UI, 0.1f, false, 0);
        textX += iconSize + 8;
    }

    r.DrawText(m_text.c_str(), { textX, pos.y + 8 }, 18, WHITE);
}

} // namespace View