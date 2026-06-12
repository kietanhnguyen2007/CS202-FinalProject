#pragma once

#include "raylib.h"
#include <vector>
#include <string>

namespace View {

class MenuView {
public:
    static MenuView& GetInstance();

    void Init();
    bool LoadResources(const std::string& atlasJsonPath);
    void Shutdown();

    void Update(float dt, int selectedIndex);
    void Render();
    void SetVisible(bool v) { m_visible = v; }

private:
    MenuView() = default;
    bool m_visible = true;
    std::vector<std::string> m_items = { "Start", "Options", "Quit" };
    bool m_loaded = false;
    int m_selected = 0;
};

} // namespace View
