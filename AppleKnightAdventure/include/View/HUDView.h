#pragma once

#include "raylib.h"
#include "Model/Player.h"
#include <string>

namespace View {

class HUDView {
public:
    static HUDView& GetInstance();

    bool Init();
    bool LoadResources(const std::string& atlasJsonPath);
    void Shutdown();

    void Update(float dt, const Player* player);
    void Render();

    void SetVisible(bool v) { m_visible = v; }

private:
    HUDView() = default;
    bool m_visible = true;
    const Player* m_player = nullptr;
    bool m_loaded = false;
};

} // namespace View
