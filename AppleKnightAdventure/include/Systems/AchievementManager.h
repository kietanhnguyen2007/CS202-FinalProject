#pragma once

#include "raylib.h"
#include <deque>
#include <string>
#include <vector>

enum class AchievementIcon {
    Trophy,
    Star,
    Target,
    Coin,
    Potion,
    Hero,
    Coop
};

struct AchievementDefinition {
    std::string id;
    std::string title;
    std::string description;
    std::string category;
    int target = 1;
    int column = 0;
    int row = 0;
    AchievementIcon icon = AchievementIcon::Trophy;
};

class AchievementManager {
public:
    static AchievementManager& GetInstance();

    void Init();
    void Shutdown();
    void Update(float dt);
    void RenderPopup() const;

    void EvaluateExistingSave(bool showPopups = false);
    void OnEnemyDefeated(bool boss);
    void OnCoinCollected(int amount);
    void OnPotionUsed();
    void OnShopPurchase();
    void OnRosterChanged();
    void OnLevelCompleted(int level, int stars, float clearTime, float parTime,
                          bool flawless, bool localCoop);

    const std::vector<AchievementDefinition>& GetDefinitions() const { return m_definitions; }
    int GetUnlockedCount() const;
    int GetProgress(const std::string& id) const;
    bool IsUnlocked(const std::string& id) const;

private:
    AchievementManager();
    const AchievementDefinition* Find(const std::string& id) const;
    bool SetProgress(const std::string& id, int progress, bool showPopup = true,
                     bool persistUnlock = true);
    void QueuePopup(const std::string& id);
    int CountUnlockedRoster() const;
    int CountBestStars() const;

    std::vector<AchievementDefinition> m_definitions;
    std::deque<std::string> m_popupQueue;
    std::string m_activePopup;
    float m_popupTimer = 0.0f;
    bool m_initialized = false;
    Font m_font{};
    Texture2D m_trophyIcon{};
};
