#pragma once

#include "raylib.h"
#include <memory>
#include <string>
#include <vector>

class Entity;

namespace View {

class TutorialRenderer {
public:
    static TutorialRenderer& GetInstance();

    bool Init();
    void Shutdown();
    void RenderAll(const std::vector<std::unique_ptr<Entity>>& entities, float dt);

    void ShowDialog(const std::string& text);
    void HideDialog();
    bool IsDialogVisible() const;
    void RenderDialog() const;

private:
    TutorialRenderer() = default;

    bool GetKeySource(const std::string& key, int frame, Rectangle& source) const;
    void DrawWrappedCenteredText(const std::string& text, Rectangle bounds,
                                 float fontSize, float spacing, float lineHeight,
                                 Color color) const;

    Texture2D m_signTexture{};
    Texture2D m_cupTexture{};
    Texture2D m_keyTexture{};
    Texture2D m_panelBrown{};
    Texture2D m_panelInsetBrown{};
    Texture2D m_buttonRoundBrown{};
    Font m_font{};
    float m_time = 0.0f;
    bool m_initialized = false;
    bool m_dialogVisible = false;
    std::string m_dialogText;
};

} // namespace View
