#pragma once

#include "raylib.h"
#include "Utils/Types.h"
#include "View/Animator.h"
#include "View/TextureAtlas.h"
#include <array>
#include <memory>
#include <string>
#include <vector>

namespace View {

struct LevelResultSnapshot {
    int levelNumber = 1;
    int stars = 1;
    float performance = 0.0f;
    float clearTime = 0.0f;
    float parTime = 240.0f;
    int enemiesKilled = 0;
    int totalEnemies = 0;
    int itemsCollected = 0;
    int totalItems = 0;
    int score = 0;
    float healthPercent = 1.0f;
    bool newHighScore = false;
    bool newBestStars = false;
    bool newBestTime = false;
    CharacterClass characterClass = CharacterClass::Knight;
};

enum class ResultAction {
    None,
    Continue,
    Retry,
    LevelSelect
};

class ResultView {
public:
    static ResultView& GetInstance();

    bool Init();
    bool LoadResources(const std::string& atlasJsonPath = {});
    void Shutdown();

    void Show(const LevelResultSnapshot& snap);
    void ShowGameOver(const LevelResultSnapshot& snap);
    void Dismiss();
    void Update(float dt);
    void Render();
    ResultAction ConsumeAction();
    bool IsVisible() const { return m_visible; }
    bool IsGameOver() const { return m_visible && m_gameOver; }

private:
    struct CelebrationParticle {
        Vector2 position{};
        Vector2 velocity{};
        float age = 0.0f;
        float lifetime = 1.0f;
        float size = 8.0f;
        float rotation = 0.0f;
        float rotationSpeed = 0.0f;
        Color color = WHITE;
        int shape = 0;
    };

    ResultView() = default;
    void LoadAvatar(CharacterClass cls);
    void ResetCelebration();
    void SpawnConfetti(int count);
    void UpdateParticles(float dt);
    Rectangle ContinueButtonRect() const;
    Rectangle RetryButtonRect() const;

    LevelResultSnapshot m_snap;
    bool m_visible = false;
    bool m_gameOver = false;
    bool m_loaded = false;
    float m_elapsed = 0.0f;
    float m_confettiTimer = 0.0f;
    ResultAction m_action = ResultAction::None;
    std::array<bool, 3> m_starSoundPlayed{false, false, false};
    std::vector<CelebrationParticle> m_particles;

    Texture2D m_panel{};
    Texture2D m_panelInset{};
    Texture2D m_roundButton{};
    Texture2D m_medal{};
    Texture2D m_lightRing{};
    Texture2D m_sparkle{};
    Texture2D m_starBurst{};
    Texture2D m_iconTrophy{};
    Texture2D m_iconTarget{};
    Texture2D m_iconStar{};
    Texture2D m_trophy{};
    Font m_font{};
    std::shared_ptr<Animations::TextureAtlas> m_avatarAtlas;
    Animations::Animator m_avatarAnimator;
};

} // namespace View
