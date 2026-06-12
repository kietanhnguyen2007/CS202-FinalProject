#pragma once

#include "raylib.h"
#include <unordered_map>
#include <memory>

namespace View::Animations { class TextureAtlas; }

namespace View {

class EnemyStatusRenderer {
public:
    static EnemyStatusRenderer& GetInstance();

    bool LoadResources(const std::string& atlasJsonPath);
    void Update(float dt);
    void Render(const Camera2D& camera);

    void SetStatus(uint32_t entityId, const Vector2& worldPos, bool burn, bool wet, bool shocked);
    void ClearStatus(uint32_t entityId);

private:
    struct StatusFlags { bool burn=false; bool wet=false; bool shocked=false; };
    std::unordered_map<uint32_t, StatusFlags> m_status;
    // atlas frames would be queried by name when rendering; for now track that resources loaded
    bool m_loaded = false;
    std::shared_ptr<View::Animations::TextureAtlas> m_atlas;
};

} // namespace View
