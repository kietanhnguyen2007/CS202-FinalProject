#pragma once

#include "Survival3D/Model/SurvivalTypes.h"
#include "Survival3D/Vfx/VfxRuntime.h"
#include "raylib.h"
#include <array>
#include <cstdint>

namespace Survival3D {

class SurvivalController;

struct WeaponWorldPose {
    bool valid = false;
    Vector3 grip{};
    Vector3 bladeRoot{};
    Vector3 bladeTip{};
};

struct BladeTrailSample {
    Vector3 root{};
    Vector3 tip{};
    double time = 0.0;
};

struct BladeTrailState {
    // 32 samples cover the full 0.22 s ribbon at the fixed 120 Hz visual
    // sampling rate without changing trail length on high-refresh displays.
    std::array<BladeTrailSample, 32> samples{};
    int count = 0;
    std::uint32_t actionSerial = 0;
    std::uint32_t contactSerial = 0;
    Vector3 contactPoint{};
    double contactTime = -1.0;
};

class SurvivalView {
public:
    static SurvivalView& GetInstance();

    bool Init();
    void Shutdown();
    void Render(const SurvivalController& controller) const;

private:
    struct AnimatedModelAsset {
        Model model{};
        ModelAnimation* animations = nullptr;
        int animationCount = 0;
        int weaponSocketBone = -1;
        bool ready = false;
    };

    struct AnimationPlayback {
        const char* clipName = nullptr;
        // Seconds for a looping clip; normalized 0..1 progress otherwise.
        float position = 0.0f;
        float speed = 1.0f;
        bool looping = false;
    };

    struct WeaponModelAsset {
        Model model{};
        Matrix gripOffset{
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
        Vector3 bladeRootLocal{};
        Vector3 bladeTipLocal{};
        bool bladeSegmentValid = false;
        bool ready = false;
    };

    // Skill assets are real low-poly GLB geometry.  They complement the
    // animated actor clip and VFX texture; a billboard alone is never treated
    // as a complete gameplay ability asset.
    struct SkillModelAsset {
        Model model{};
        ModelAnimation* animations = nullptr;
        int animationCount = 0;
        int activateAnimation = -1;
        bool ready = false;
    };

    struct SkillParticle3D {
        Vector3 position{};
        Vector3 velocity{};
        Color color{};
        float age = 0.0f;
        float lifetime = 0.0f;
        float size = 0.0f;
        bool active = false;
    };

    SurvivalView() = default;
    void DrawArenaBackdrop(float time, bool reducedMotion) const;
    void RenderArena(float time, bool reducedMotion, bool showroom = false) const;
    void RenderPlayer(const PlayerState& player, bool reducedMotion = false,
                      bool highContrast = false) const;
    void RenderEnemy(const EnemyState& enemy, float time, bool reducedMotion,
                     bool lowDetail, bool highContrast = false) const;
    void RenderHUD(const SurvivalController& controller) const;
    void RenderCharacterSelect(const SurvivalController& controller) const;
    void RenderUpgradeChoice(const SurvivalController& controller) const;
    void RenderResult(const SurvivalController& controller) const;
    void RenderRecords(const SurvivalController& controller) const;
    void RenderPerformance(const SurvivalController& controller) const;
    void DrawCentered(const char* text, float y, float size, Color color) const;
    bool LoadAnimatedModel(std::size_t slot, const char* path);
    bool LoadWeaponModel(std::size_t slot, const char* path);
    bool LoadSkillModel(std::size_t slot, const char* path);
    void UnloadAnimatedModels();
    void UnloadWeaponModels();
    void UnloadSkillModels();
    void RenderSkillGeometry(const SurvivalController& controller,
                             float time) const;
    void UpdateSkillVfxRuntime(const SurvivalController& controller,
                               float frameDt) const;
    void RenderSkillVfxRuntime(bool reducedMotion,
                               bool highContrast) const;
    void SpawnSkillParticleBurst(Vfx::PackageId package, Vector3 position,
                                 Vector3 forward, float radius,
                                 float intensity, int count) const;
    void UpdateSkillParticlePool(float frameDt) const;
    void RenderSkillParticlePool(bool reducedMotion,
                                 bool highContrast) const;
    WeaponWorldPose DrawAnimatedModel(std::size_t slot, Vector3 position,
                                      Vector3 facing,
                                      const AnimationPlayback& poseA,
                                      const AnimationPlayback& poseB,
                                      float poseBlend,
                                      float scale = 1.0f, Color tint = WHITE,
                                      WeaponId weapon = WeaponId::None) const;

    bool m_initialized = false;
    Font m_font{};
    Texture2D m_knightSwordArcTexture{};
    Texture2D m_knightGuardTexture{};
    Texture2D m_knightRushTexture{};
    Texture2D m_knightStormTexture{};
    Texture2D m_dashStreakTexture{};
    Texture2D m_mageSigilTexture{};
    Texture2D m_mageBoltTexture{};
    Texture2D m_mageFrostTexture{};
    Texture2D m_mageGravityTexture{};
    Texture2D m_mageUltimateTexture{};
    Texture2D m_arenaBackdropTexture{};
    Texture2D m_upgradeCardFrameTexture{};
    Texture2D m_resultLedgerFrameTexture{};
    Model m_arenaEnvironmentModel{};
    bool m_arenaEnvironmentReady = false;
    mutable BladeTrailState m_knightBasicTrail{};
    mutable Vfx::Runtime m_skillVfxRuntime{};
    mutable std::uint32_t m_lastSkillVfxCueSerial = 0;
    mutable std::array<SkillParticle3D, 512> m_skillParticles{};
    mutable std::uint16_t m_skillParticleCursor = 0;
    mutable std::uint32_t m_skillParticleSerial = 1;
    std::array<AnimatedModelAsset, 10> m_animatedModels{};
    // Player weapons occupy slots 0-1. Slot 2 is the Hex Archer's modular bow.
    std::array<WeaponModelAsset, 3> m_weaponModels{};
    // Knight: Basic/K/U/H/Dash, Mage: Basic/K/U/H/Dash.
    std::array<SkillModelAsset, 10> m_skillModels{};
};

} // namespace Survival3D
