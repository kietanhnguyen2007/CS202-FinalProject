#pragma once

#include "raylib.h"
#include "View/TextureAtlas.h"
#include "View/Animator.h"
#include "Model/Player.h"
#include <string>
#include <memory>

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

    std::shared_ptr<Animations::TextureAtlas> m_uiAtlas;
    std::shared_ptr<Animations::TextureAtlas> m_coinAtlas;
    Animations::Animator m_coinAnim;
};

} // namespace View
