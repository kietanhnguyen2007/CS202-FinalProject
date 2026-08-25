#include "View/CharacterRenderer.h"
#include "View/ElementalFX.h"
#include "Model/Character.h"
#include "Model/Enemy.h"
#include "View/AssetManager.h"
#include <iostream>
#include <cmath>
#include <cassert>
#include <algorithm>
#include <filesystem>

namespace View {

CharacterRenderer& CharacterRenderer::GetInstance() {
    static CharacterRenderer instance;
    return instance;
}

bool CharacterRenderer::PreloadAtlas(const std::string& atlasPath) {
    if (m_atlasCache.find(atlasPath) != m_atlasCache.end()) {
        return true; // Đã load rồi
    }

    auto atlas = AssetManager::GetInstance().GetAtlas(atlasPath);
    if (!atlas) {
        std::cerr << "[CharacterRenderer] Preload failed: " << atlasPath << "\n";
        return false;
    }
    
    m_atlasCache.emplace(atlasPath, std::move(atlas));
    return true;
}

bool CharacterRenderer::Register(const Entity* entity,
                                  const std::string& atlasPath,
                                  const std::string& defaultClip) {
    if (!entity) {
        std::cerr << "[CharacterRenderer] Register: null entity\n";
        return false;
    }
    int rawId = entity->GetId();
    assert(rawId >= 0);
    uint32_t id = static_cast<uint32_t>(rawId);

    // Check for double-register (warn and skip if same entity already registered)
    if (m_entities.find(id) != m_entities.end()) {
        // Already registered — this is safe to ignore (re-registration with same id)
        return true;
    }

    // Load or retrieve atlas from cache
    auto atlasIt = m_atlasCache.find(atlasPath);
    if (atlasIt == m_atlasCache.end()) {
        auto loaded = AssetManager::GetInstance().GetAtlas(atlasPath);
        if (!loaded) return false;
        m_atlasCache[atlasPath] = loaded;
        atlasIt = m_atlasCache.find(atlasPath);
    }

    auto& animator = m_animators[id];
    animator.SetTexture(atlasIt->second->GetTexture());

    // Forward all clips from atlas to animator
    for (const auto& clipName : atlasIt->second->GetClipNames()) {
        auto clip = atlasIt->second->GetClip(clipName);
        if (clip) animator.AddClip(clip);
    }

    m_entities[id] = entity;
    m_entityAtlas[id] = atlasIt->second;

    // If entity is a Pet, ensure default action clip map and infer function exist
    if (entity->GetType() == EntityType::Pet) {
        ActionConfig& cfg = m_actionConfigs[EntityType::Pet];
        if (cfg.clipMap.empty()) {
            cfg.clipMap[ACTION_IDLE] = "idle";
            cfg.clipMap[ACTION_WALK] = "move";
            cfg.clipMap[ACTION_ATTACK] = "attack";
            cfg.clipMap[ACTION_DEAD] = "dead";
            cfg.clipMap[ACTION_SKILL] = "healing";
        }
        if (!cfg.inferAction) {
            cfg.inferAction = [](const Entity* e)->int {
                if (!e || !e->IsActive()) return ACTION_DEAD;
                auto vel = e->GetVelocity();
                if (std::abs(vel.x) > 0.1f || std::abs(vel.y) > 0.1f) return ACTION_WALK;
                return ACTION_IDLE;
            };
        }
    }

    // Auto-set boss asset root from atlas path pattern
    if (entity->GetType() == EntityType::Boss) {
        auto pos = atlasPath.find("/phase");
        if (pos != std::string::npos) {
            m_bossAssetRoot[id] = atlasPath.substr(0, pos + 1);
        }
    }

    // Try to play default clip if it exists
    if (!defaultClip.empty()) {
        if (animator.HasClip(defaultClip)) {
            animator.Play(defaultClip);
        } else {
            std::cerr << "[CharacterRenderer] Default clip not found: " << defaultClip
                      << " for entity " << id << "\n";
        }
    }

    return true;
}

bool CharacterRenderer::MergeAtlas(uint32_t entityId, const std::string& atlasPath, const std::string& clipAlias) {
    auto entityIt = m_entities.find(entityId);
    if (entityIt == m_entities.end()) return false;

    auto atlasIt = m_atlasCache.find(atlasPath);
    if (atlasIt == m_atlasCache.end()) {
        if (!PreloadAtlas(atlasPath)) return false;
        atlasIt = m_atlasCache.find(atlasPath);
    }

    auto& animator = m_animators[entityId];
    for (const auto& clipName : atlasIt->second->GetClipNames()) {
        auto clip = atlasIt->second->GetClip(clipName);
        if (clip) {
            if (!clipAlias.empty()) {
                auto cloned = std::make_shared<Animations::AnimationClip>(*clip);
                cloned->name = clipAlias;
                animator.AddClip(cloned);
            } else {
                animator.AddClip(clip);
            }
        }
    }

    return true;
}

void CharacterRenderer::Unregister(uint32_t entityId) {
    // Fire callback before removing (so callback can read entity data if needed)
    auto cbIt = m_removeCallbacks.find(entityId);
    if (cbIt != m_removeCallbacks.end()) {
        auto cb = std::move(cbIt->second);
        m_removeCallbacks.erase(cbIt);
        cb(entityId);
    }
    m_animators.erase(entityId);
    m_entities.erase(entityId);
    m_lastActions.erase(entityId);
    m_bossPhases.erase(entityId);
    m_bossAssetRoot.erase(entityId);
}

void CharacterRenderer::Clear() {
    m_animators.clear();
    m_entities.clear();
    m_atlasCache.clear();
    m_actionConfigs.clear();
    m_lastActions.clear();
    m_bossPhases.clear();
    m_bossAssetRoot.clear();
    m_removeCallbacks.clear();
}

bool CharacterRenderer::IsRegistered(uint32_t entityId) const {
    return m_entities.find(entityId) != m_entities.end();
}

void CharacterRenderer::SetOnEntityRemovedCallback(uint32_t entityId, std::function<void(uint32_t)> cb) {
    if (cb) m_removeCallbacks[entityId] = std::move(cb);
}

void CharacterRenderer::ClearOnEntityRemovedCallback(uint32_t entityId) {
    m_removeCallbacks.erase(entityId);
}

void CharacterRenderer::SetActionClipMap(EntityType type,
                                          const std::unordered_map<int, std::string>& clips) {
    m_actionConfigs[type].clipMap = clips;
}

void CharacterRenderer::SetInferFunction(EntityType type,
                                          std::function<int(const Entity*)> inferFn) {
    m_actionConfigs[type].inferAction = std::move(inferFn);
}

int CharacterRenderer::DefaultInferAction(const Entity* entity) {
    if (!entity || !entity->IsActive()) return ACTION_DEAD;
    auto vel = entity->GetVelocity();
    
    // Trục Y trong 2D thông thường: hướng lên là âm, hướng xuống là dương
    if (vel.y < -0.1f) return ACTION_JUMP;   // Giả sử có ACTION_JUMP
    if (vel.y > 0.1f) return ACTION_FALL;    // Giả sử có ACTION_FALL
    
    if (std::abs(vel.x) > 0.1f) return ACTION_WALK;
    return ACTION_IDLE;
}

void CharacterRenderer::UpdateAll(float dt) {
    for (auto& [id, animator] : m_animators) {
        auto entityIt = m_entities.find(id);
        if (entityIt == m_entities.end()) continue;

        const Entity* entity = entityIt->second;
        if (!entity || !entity->IsActive()) continue;

        EntityType type = entity->GetType();
        if (type == EntityType::Player || type == EntityType::DualWorldPlayer || 
            type == EntityType::Enemy || type == EntityType::Pet || type == EntityType::Boss) {
            
            assert(dynamic_cast<const Character*>(entity) != nullptr && "Entity type filter mismatch with Character hierarchy");
            const Character* character = static_cast<const Character*>(entity);
            Character::State state = character->GetState();
            
            animator.SetFlipX(character->GetDirection() == Direction::Left);

            std::string clipName = "idle";
            switch (state) {
                case Character::State::Idle:    clipName = "idle";          break;
                case Character::State::Walk:    clipName = "walk";          break;
                case Character::State::Run:     clipName = "walk";          break; // Sprint shares walk asset
                case Character::State::Dash:    clipName = "run";           break; // Dash uses run.json
                case Character::State::Jump:    clipName = "jump";          break;
                case Character::State::Fall:    clipName = "fall";          break;
                case Character::State::Attack:  clipName = "attack";        break;
                case Character::State::Attack2: clipName = "attack_2";      break;
                case Character::State::Attack3: clipName = "attack_3";      break;
                case Character::State::Ultimate:clipName = "ultimate_skill";break;
                case Character::State::Parry:   clipName = "parry";         break;
                case Character::State::Hurt:    clipName = "hurt";          break;
                case Character::State::Dead:    clipName = "dead";          break;
                case Character::State::Skill:   clipName = "skill";         break;
            }

            auto prevIt = m_lastActions.find(id);
            int prevAction = (prevIt != m_lastActions.end()) ? prevIt->second : -1;
            int currentAction = static_cast<int>(state);

            // Clip fallback chains for assets that use different naming
            if (!animator.HasClip(clipName)) {
                static const std::unordered_map<std::string, std::vector<std::string>> fallback = {
                    {"idle",          {"fly", "default"}},
                    {"walk",          {"run", "fly", "idle"}},
                    {"run",           {"walk", "idle"}},
                    {"jump",          {"jump_fall", "idle"}},
                    {"fall",          {"jump_fall", "idle"}},
                    {"hurt",          {"hit", "idle"}},
                    {"dead",          {"death", "idle"}},
                    {"skill",         {"attack", "idle"}},
                    {"parry",         {"idle"}},
                    {"ultimate_skill",{"attack", "skill", "idle"}},
                    {"attack",        {"attack_1", "idle"}},
                    {"attack_2",      {"attack", "attack_1", "idle"}},
                    {"attack_3",      {"attack", "attack_1", "idle"}},
                };
                auto it = fallback.find(clipName);
                if (it != fallback.end()) {
                    for (const auto& fb : it->second) {
                        if (animator.HasClip(fb)) {
                            clipName = fb;
                            break;
                        }
                    }
                }
            }


            // Switch clip if action changed or animator stopped (except when dead or parrying)
            // Parry: play once, then hold last frame until state changes
            bool parryHolding = (state == Character::State::Parry && !animator.IsPlaying()
                                 && animator.CurrentClip() == clipName);
            if (currentAction != prevAction || (!animator.IsPlaying() && state != Character::State::Dead && !parryHolding)) {
                animator.Play(clipName, 1.0f, true);
                m_lastActions[id] = currentAction;
            }
        }

        animator.Update(dt);
    }
}

void CharacterRenderer::RenderAll() {
    // In RenderAll(), iterate through all active characters (Player, Enemies, Pets) from the Model (using DualWorld::GetInstance()).
    // Note: Since DualWorld::GetInstance() is not implemented in the current headers to provide entities,
    // we use the local registered m_entities which reflects the active characters from the Model.
    for (auto& [id, animator] : m_animators) {
        auto it = m_entities.find(id);
        if (it == m_entities.end()) continue;

        const Entity* entity = it->second;
        if (!entity || !entity->IsActive() || !animator.HasTexture()) continue;

        Vector2 pos = entity->GetPosition();
        Vector2 size = entity->GetSize();
        float baseScale = entity->GetScale();

        // Calculate visual scale to keep Knight, enemies, and bosses proportioned to 32x32 tiles
        float visualScale = baseScale;
        if (entity->GetType() == EntityType::Player || entity->GetType() == EntityType::DualWorldPlayer) {
            visualScale = 0.6f;      // Player: smaller character
        } else if (entity->GetType() == EntityType::Enemy) {
            visualScale = 0.6f * 1.7f; // Enemy: large, same as big tiles
        } else if (entity->GetType() == EntityType::Boss) {
            visualScale = 0.77f * 1.7f * 0.5f;
            auto phaseIt = m_bossPhases.find(id);
            if (phaseIt != m_bossPhases.end()) {
                if (phaseIt->second == BossPhase::Phase4) {
                    visualScale *= 2.085f; // Scale up Phase 4 character size to match Phase 3
                } else if (phaseIt->second == BossPhase::Phase3) {
                    visualScale *= 1.35f; // Scale up Phase 3 character size to match Phase 2
                } else if (phaseIt->second == BossPhase::Phase1) {
                    visualScale *= 0.74f; // Shrink Phase 1 character size to match Phase 2
                }
            } else {
                visualScale *= 0.74f; // Default (Phase 1)
            }
        }

        // Apply per-clip scale (e.g. oversized boss hurt frames normalized to idle size)
        visualScale *= animator.GetCurrentClipScale();

        // Bounding box bottom center
        Vector2 bottomCenter = { pos.x + size.x * 0.5f * baseScale, pos.y + size.y * baseScale };
        Rectangle srcRect = animator.GetCurrentSrcRect();
        
        float originYFactor = 1.0f;
        if (entity->GetType() == EntityType::Enemy) {
            originYFactor = 0.6667f; // Adjust for empty space at the bottom of the sprite frames
        }
        // Boss uses 1.0f by default, removing the 0.6667f which caused them to sink
        
        Vector2 originPixels = {srcRect.width * 0.5f, srcRect.height * originYFactor};
        if (entity->GetType() == EntityType::Boss) {
            originPixels = animator.GetCurrentGroundOrigin();
        }
        Vector2 customOrigin = {originPixels.x * visualScale, originPixels.y * visualScale};

        // If aura frame exists in the entity atlas for current element, draw it behind the character
        auto tint = View::ElementalFX::GetInstance().GetTintForEntity(id);
        
        // Add flashing effect for dying enemies
        if (entity->GetType() == EntityType::Enemy) {
            const Enemy* enemy = static_cast<const Enemy*>(entity);
            if (enemy->GetState() == EnemyState::Dead) {
                float t = enemy->GetStateTimer();
                // Blink effect for 0.5s: toggle alpha
                if (t < 0.5f) {
                    if (std::fmod(t, 0.1f) < 0.05f) {
                        tint.a = 50; // transparent blink
                    } else {
                        tint.a = 255;
                    }
                }
            }
        }
        auto atlasIt = m_entityAtlas.find(id);
        if (atlasIt != m_entityAtlas.end()) {
            auto atlas = atlasIt->second;
            if (atlas) {
                std::string frameName;
                if (tint.r == 255 && tint.g == 255 && tint.b == 255) {
                    // Không có hiệu ứng nguyên tố
                } else {
                    if (tint.r > 240 && tint.g < 200) frameName = "aura/fire";
                    else if (tint.b > 200 && tint.r < 150) frameName = "aura/water";
                    else if (tint.r > 240 && tint.g > 240) frameName = "aura/thunder";
                }

                if (!frameName.empty() && atlas->HasFrame(frameName)) {
                    Rectangle src = atlas->GetFrameRect(frameName);
                    Texture2D* tex = atlas->GetTexture();
                    Vector2 auraOrigin = { src.width * 0.5f, src.height };
                    View::Renderer::GetInstance().SubmitSprite(tex, src, bottomCenter, {visualScale, visualScale}, 0.0f, auraOrigin, WHITE, View::Layer::World, -0.01f, false, id);
                }
            }
        }

        // Boss phase visual overlay (drawn after the character sprite)
        if (entity->GetType() == EntityType::Boss) {
            RenderBossPhaseOverlay(id, entity);
        }

        // Draw the character to the screen using Renderer::GetInstance().SubmitSprite()
        View::Renderer::GetInstance().SubmitSprite(
            animator.GetCurrentTexture(),
            srcRect,
            bottomCenter,
            {visualScale, visualScale},
            entity->GetRotation(),
            customOrigin,
            tint,
            View::Layer::World,
            0.0f,
            animator.GetFlipX(),
            id);
    }
}

Animations::Animator* CharacterRenderer::GetAnimator(uint32_t entityId) {
    auto it = m_animators.find(entityId);
    if (it == m_animators.end()) return nullptr;
    return &it->second;
}

void CharacterRenderer::SetBossPhase(uint32_t entityId, BossPhase phase) {
    m_bossPhases[entityId] = phase;
}

BossPhase CharacterRenderer::GetBossPhase(uint32_t entityId) const {
    auto it = m_bossPhases.find(entityId);
    if (it != m_bossPhases.end()) {
        return it->second;
    }
    return BossPhase::Phase1;
}

void CharacterRenderer::ClearBossPhase(uint32_t entityId) {
    m_bossPhases.erase(entityId);
}

void CharacterRenderer::SetBossAssetRoot(uint32_t entityId, const std::string& rootPath) {
    m_bossAssetRoot[entityId] = rootPath;
}

static const char* PhaseSubdir(BossPhase phase) {
    switch (phase) {
        case BossPhase::Phase1:   return "phase1";
        case BossPhase::Phase2:   return "phase2";
        case BossPhase::Phase3:   return "phase3";
        case BossPhase::Phase4:
        case BossPhase::Enraged:  return "phase4";
        default:                  return "phase1";
    }
}

bool CharacterRenderer::SwitchPhase(uint32_t entityId, BossPhase phase) {
    auto animIt = m_animators.find(entityId);
    if (animIt == m_animators.end()) return false;

    auto rootIt = m_bossAssetRoot.find(entityId);
    if (rootIt == m_bossAssetRoot.end()) return false;

    std::string phaseDir = rootIt->second;
    phaseDir += PhaseSubdir(phase);
    phaseDir += "/";

    static const char* clipFiles[] = {
        "idle", "walk", "run", "jump",
        "attack_1", "attack_2", "attack_3",
        "projectile_attack1", "projectile_attack2",
        "hurt", "dead", "healing", "skill",
        "fall", "parry", "ultimate_skill"
    };

    auto& animator = animIt->second;
    animator.ClearClips();

    bool anyLoaded = false;
    float referenceHeight = 0.0f; // reference clip height (idle) for normalizing oversized frames
    std::vector<std::pair<std::string, std::shared_ptr<Animations::AnimationClip>>> loadedClips;

    auto clipMaxHeight = [](const Animations::AnimationClip& c) {
        float h = 0.0f;
        for (const auto& f : c.frames) h = std::max(h, f.src.height);
        return h;
    };

    auto collectAtlas = [&](const std::string& path) {
        if (!std::filesystem::exists(path)) return;
        auto cacheIt = m_atlasCache.find(path);
        if (cacheIt == m_atlasCache.end()) {
            if (!PreloadAtlas(path)) return;
            cacheIt = m_atlasCache.find(path);
        }
        for (const auto& rawClip : cacheIt->second->GetClipNames()) {
            auto clip = cacheIt->second->GetClip(rawClip);
            if (!clip) continue;
            // Alias the raw clip so the state->clip map in UpdateAll keeps working.
            // attack_1 -> attack, transition -> skill; others keep their names.
            std::string alias;
            if (rawClip == "attack_1") alias = "attack";
            else if (rawClip.rfind("transition", 0) == 0) alias = "skill";
            else if (rawClip == "healing") alias = "attack_2";
            if (!alias.empty()) {
                auto cloned = std::make_shared<Animations::AnimationClip>(*clip);
                cloned->name = alias;
                if (alias == "skill" && !cloned->frames.empty()) {
                    constexpr float transitionDuration = 1.5f;
                    const float frameDuration = transitionDuration /
                        static_cast<float>(cloned->frames.size());
                    for (auto& frame : cloned->frames) {
                        frame.duration = frameDuration;
                    }
                    cloned->loop = false;
                    cloned->totalDuration = transitionDuration;
                }
                loadedClips.emplace_back(alias, cloned);
            } else {
                loadedClips.emplace_back(rawClip, clip);
            }
            anyLoaded = true;
        }
    };

    for (const char* name : clipFiles) {
        collectAtlas(phaseDir + name + ".json");
    }

    // Clean Boss 1/2 transformation sheets live in dedicated folders. Boss 3
    // stores each transformation in the source phase, while this method is
    // called after the model has already switched to the destination phase.
    const bool isBoss3 = rootIt->second.find("/boss3/") != std::string::npos;
    if (phase == BossPhase::Phase2) {
        collectAtlas(rootIt->second + (isBoss3
            ? "phase1/transition.json"
            : "transition/transition.json"));
    } else if (phase == BossPhase::Phase3) {
        collectAtlas(rootIt->second + (isBoss3
            ? "phase2/transition.json"
            : "transition_2_to_3/transition_2_to_3.json"));
    } else if ((phase == BossPhase::Phase4 || phase == BossPhase::Enraged) && isBoss3) {
        collectAtlas(rootIt->second + "phase3/transition.json");
    }

    // Normalize oversized clips (e.g. boss2 hurt at 284x259 vs idle 123x226) so they
    // render at the same size as the idle clip. Falls back to walk/attack when no idle.
    std::string refKey = "idle";
    if (std::find_if(loadedClips.begin(), loadedClips.end(),
            [&](const auto& c){ return c.first == refKey; }) == loadedClips.end()) {
        if (std::find_if(loadedClips.begin(), loadedClips.end(),
                [&](const auto& c){ return c.first == "walk"; }) != loadedClips.end()) refKey = "walk";
        else if (!loadedClips.empty()) refKey = loadedClips.front().first;
    }
    for (const auto& c : loadedClips) {
        if (c.first == refKey) {
            referenceHeight = clipMaxHeight(*c.second);
            break;
        }
    }
    for (auto& c : loadedClips) {
        float h = clipMaxHeight(*c.second);
        if (referenceHeight > 0.0f && h > referenceHeight) {
            c.second->scale = referenceHeight / h;
        }
        animator.AddClip(c.second);
    }

    if (!anyLoaded) {
        if (phase == BossPhase::Phase3) {
            return SwitchPhase(entityId, BossPhase::Phase2);
        }
        return false;
    }

    m_bossPhases[entityId] = phase;
    return true;
}

void CharacterRenderer::RenderBossPhaseOverlay(uint32_t entityId, const Entity* entity) {
    auto phaseIt = m_bossPhases.find(entityId);
    if (phaseIt == m_bossPhases.end()) return;
    if (phaseIt->second == BossPhase::Phase1) return; // No overlay for base phase

    auto atlasIt = m_entityAtlas.find(entityId);
    if (atlasIt == m_entityAtlas.end()) return;
    auto atlas = atlasIt->second;
    if (!atlas) return;

    std::string glowFrame;
    Color glowColor = WHITE;
    switch (phaseIt->second) {
        case BossPhase::Phase2:
            glowFrame = "boss/phase2_glow";
            glowColor = (Color){255,180,80,180};
            break;
        case BossPhase::Phase3:
            glowFrame = "boss/phase3_glow";
            glowColor = (Color){255,80,80,200};
            break;
        case BossPhase::Enraged:
            glowFrame = "boss/enraged_glow";
            glowColor = (Color){255,40,40,220};
            break;
        default: return;
    }

    if (atlas->HasFrame(glowFrame)) {
        Rectangle src = atlas->GetFrameRect(glowFrame);
        Texture2D* tex = atlas->GetTexture();
        
        Vector2 pos = entity->GetPosition();
        Vector2 size = entity->GetSize();
        float baseScale = entity->GetScale();
        
        float visualScale = 0.77f * 1.7f * 0.5f; // Boss visual scale matches RenderAll
        if (phaseIt->second == BossPhase::Enraged) visualScale *= 1.3f;
        
        Vector2 bottomCenter = { pos.x + size.x * 0.5f * baseScale, pos.y + size.y * baseScale };
        
        float originYFactor = 1.0f;
        if (entity->GetType() == EntityType::Boss) { // Technically always true here
            originYFactor = 0.6667f;
        }
        Vector2 overlayOrigin = { src.width * 0.5f * visualScale, src.height * originYFactor * visualScale };
        
        // Lấy thông tin flipX từ animator để overlay lật mặt trùng với Boss
        auto animIt = m_animators.find(entityId);
        bool isFlipped = (animIt != m_animators.end()) ? animIt->second.GetFlipX() : false;

        View::Renderer::GetInstance().SubmitSprite(
            tex, src, bottomCenter,
            {visualScale, visualScale}, 0.0f,
            overlayOrigin,
            glowColor, View::Layer::Foreground, -0.02f, isFlipped, entityId);
    }
}

void CharacterRenderer::PlayAction(uint32_t entityId, int action) {
    auto it = m_animators.find(entityId);
    if (it == m_animators.end()) return;
    auto& animator = it->second;

    // map action to clip name using action config if available
    const Entity* entityPtr = nullptr;
    auto entIt = m_entities.find(entityId);
    if (entIt != m_entities.end()) entityPtr = entIt->second;

    if (entityPtr) {
        EntityType type = entityPtr->GetType();
        auto cfgIt = m_actionConfigs.find(type);
        if (cfgIt != m_actionConfigs.end()) {
            auto clipIt = cfgIt->second.clipMap.find(action);
            if (clipIt != cfgIt->second.clipMap.end()) {
                animator.Play(clipIt->second);
                return;
            }
        }
    }

    // fallback: try well-known clip names
    auto tryPlay = [&](const std::string& name) {
        if (animator.HasClip(name)) { animator.Play(name); return true; }
        return false;
    };
    switch (action) {
        case ACTION_ATTACK:
            if (!tryPlay("attack")) tryPlay("idle");
            break;
        case ACTION_ATTACK2:
            if (!tryPlay("attack_2")) tryPlay("idle");
            break;
        case ACTION_ATTACK3:
            if (!tryPlay("attack_3")) tryPlay("idle");
            break;
        case ACTION_HURT:
            if (!tryPlay("hurt")) { if (!tryPlay("hit")) tryPlay("idle"); }
            break;
        case ACTION_SKILL:
            if (!tryPlay("skill")) { if (!tryPlay("healing")) tryPlay("attack"); }
            break;
        default: break;
    }
}

} // namespace View
