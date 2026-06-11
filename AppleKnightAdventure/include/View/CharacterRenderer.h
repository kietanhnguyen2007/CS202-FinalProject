#pragma once

#include "View/Animator.h"
#include "View/TextureAtlas.h"
#include "View/Renderer.h"
#include "Model/Entity.h"
#include <unordered_map>
#include <string>
#include <memory>
#include <functional>

namespace View {

// Default action constants for use with SetActionClipMap
constexpr int ACTION_IDLE   = 0;
constexpr int ACTION_WALK   = 1;
constexpr int ACTION_ATTACK = 2;
constexpr int ACTION_HURT   = 3;
constexpr int ACTION_DEAD   = 4;
constexpr int ACTION_JUMP   = 5;
constexpr int ACTION_SKILL  = 6;

class CharacterRenderer {
public:
    static CharacterRenderer& GetInstance();

    // Returns true on success, false if atlas fails to load
    bool Register(const Entity* entity,
                  const std::string& atlasPath,
                  const std::string& defaultClip = "idle");
    void Unregister(uint32_t entityId);
    void Clear();

    // Register a clip mapping for an entity type: action → clip name
    void SetActionClipMap(EntityType type, const std::unordered_map<int, std::string>& clips);

    // Register an inference function that returns the current action for an entity
    // Called every frame before Update; fallback infers from velocity if not set
    void SetInferFunction(EntityType type, std::function<int(const Entity*)> inferFn);

    // Update all animators and auto-switch clips based on inferred action
    void UpdateAll(float dt);
    void RenderAll();

    // Direct animator access for manual control
    Animations::Animator* GetAnimator(uint32_t entityId);

private:
    CharacterRenderer() = default;
    ~CharacterRenderer() = default;

    // Default inference: idle if velocity near zero, walk otherwise
    static int DefaultInferAction(const Entity* entity);

    struct ActionConfig {
        std::unordered_map<int, std::string> clipMap;
        std::function<int(const Entity*)> inferAction;
    };

    std::unordered_map<uint32_t, Animations::Animator> m_animators;
    std::unordered_map<uint32_t, const Entity*> m_entities;
    std::unordered_map<std::string, std::shared_ptr<Animations::TextureAtlas>> m_atlasCache;
    std::unordered_map<EntityType, ActionConfig> m_actionConfigs;
    std::unordered_map<uint32_t, int> m_lastActions;
};

} // namespace View
