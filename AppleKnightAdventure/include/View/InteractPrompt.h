#pragma once

#include "raylib.h"
#include <string>

namespace View {

class InteractPrompt {
public:
    static InteractPrompt& GetInstance();

    void Show(const std::string& text);
    void Hide();
    bool IsVisible() const;

    bool LoadResources(const std::string& atlasJsonPath = "");
    void Shutdown();

    void Render();

private:
    InteractPrompt() = default;
    ~InteractPrompt() = default;

    bool m_visible = false;
    std::string m_text;

    Texture2D m_texIcon{};
    int m_iconFrameW = 0;
};

} // namespace View