#pragma once

#include "View/Animator.h"
#include "View/TextureAtlas.h"
#include "View/Renderer.h"
#include "Model/Entity.h"
#include <unordered_map>
#include <string>
#include <memory>

namespace View {

class CharacterRenderer {
public:
    static CharacterRenderer& GetInstance();

    void Register(const Entity* entity,
                  const std::string& atlasPath,
                  const std::string& defaultClip = "idle");
    void Unregister(uint32_t entityId);
    void Clear();

    void UpdateAll(float dt);
    void RenderAll();

    Animations::Animator* GetAnimator(uint32_t entityId);

private:
    CharacterRenderer() = default;
    ~CharacterRenderer() = default;

    std::unordered_map<uint32_t, Animations::Animator> m_animators;
    std::unordered_map<uint32_t, const Entity*> m_entities;
    std::unordered_map<std::string, std::shared_ptr<Animations::TextureAtlas>> m_atlasCache;
};

} // namespace View
