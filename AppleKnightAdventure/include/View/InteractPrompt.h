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

    void Render();

private:
    InteractPrompt() = default;
    ~InteractPrompt() = default;

    bool m_visible = false;
    std::string m_text;
};

} // namespace View