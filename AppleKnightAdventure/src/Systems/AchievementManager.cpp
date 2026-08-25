#include "Systems/AchievementManager.h"
#include "Model/SaveManager.h"
#include "Systems/SoundManager.h"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <fstream>
#include <nlohmann/json.hpp>

namespace {
float EaseOutBack(float t) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    const float p = t - 1.0f;
    return 1.0f + c3 * p * p * p + c1 * p * p;
}

float LoadAchievementParTime(int level) {
    constexpr float fallback = 240.0f;
    std::ifstream file("assets/config/victory_grades.json");
    if (!file.is_open()) return fallback;
    try {
        nlohmann::json root;
        file >> root;
        const float defaultTime = root.value("defaultParTime", fallback);
        if (!root.contains("levels") || !root["levels"].is_object()) return defaultTime;
        return std::max(1.0f, root["levels"].value(std::to_string(level), defaultTime));
    } catch (...) {
        return fallback;
    }
}
}

AchievementManager& AchievementManager::GetInstance() {
    static AchievementManager instance;
    return instance;
}

AchievementManager::AchievementManager()
    : m_definitions{
        {"tutorial_graduate", "Tutorial Graduate", "Complete Level 1.", "ADVENTURE", 1, 0, 0, AchievementIcon::Trophy},
        {"road_ahead", "Road Ahead", "Complete 3 different campaign levels.", "ADVENTURE", 3, 1, 0, AchievementIcon::Trophy},
        {"realm_defender", "Realm Defender", "Complete all 6 campaign levels.", "ADVENTURE", 6, 2, 0, AchievementIcon::Trophy},
        {"ahead_of_time", "Ahead of Time", "Complete a level under its par time.", "ADVENTURE", 1, 1, 1, AchievementIcon::Target},
        {"flawless_victory", "Flawless Victory", "Complete a level without taking damage.", "ADVENTURE", 1, 2, 1, AchievementIcon::Target},
        {"star_student", "Star Student", "Earn 3 stars in one level.", "ADVENTURE", 1, 3, 0, AchievementIcon::Star},
        {"star_collector", "Star Collector", "Collect 12 best stars across all levels.", "ADVENTURE", 12, 4, 0, AchievementIcon::Star},
        {"first_blood", "First Blood", "Defeat your first enemy.", "COMBAT", 1, 0, 2, AchievementIcon::Target},
        {"monster_hunter", "Monster Hunter", "Defeat 100 unique enemies.", "COMBAT", 100, 1, 2, AchievementIcon::Target},
        {"boss_slayer", "Boss Slayer", "Defeat your first boss.", "COMBAT", 1, 2, 2, AchievementIcon::Trophy},
        {"coin_collector", "Coin Collector", "Collect 100 coins during your lifetime.", "COLLECTION", 100, 0, 3, AchievementIcon::Coin},
        {"potion_taster", "Potion Taster", "Pick up and use a healing potion.", "COLLECTION", 1, 1, 3, AchievementIcon::Potion},
        {"first_purchase", "First Purchase", "Buy an item from the shop.", "COLLECTION", 1, 2, 3, AchievementIcon::Coin},
        {"full_roster", "Full Roster", "Unlock all four playable characters.", "COLLECTION", 4, 3, 3, AchievementIcon::Hero},
        {"together_we_win", "Together We Win", "Complete a campaign level in Local Co-op.", "CO-OP", 1, 4, 3, AchievementIcon::Coop}
    } {}

void AchievementManager::Init() {
    if (m_initialized) return;
    m_font = LoadFont("assets/fonts/game_font.ttf");
    m_trophyIcon = LoadTexture("assets/ui/victory/icon_trophy.png");
    m_initialized = true;
    EvaluateExistingSave(false);
}

void AchievementManager::Shutdown() {
    if (!m_initialized) return;
    if (m_font.texture.id != 0) UnloadFont(m_font);
    if (m_trophyIcon.id != 0) UnloadTexture(m_trophyIcon);
    m_font = {};
    m_trophyIcon = {};
    m_popupQueue.clear();
    m_activePopup.clear();
    m_initialized = false;
}

const AchievementDefinition* AchievementManager::Find(const std::string& id) const {
    const auto it = std::find_if(m_definitions.begin(), m_definitions.end(),
        [&](const AchievementDefinition& value) { return value.id == id; });
    return it == m_definitions.end() ? nullptr : &*it;
}

bool AchievementManager::SetProgress(const std::string& id, int progress, bool showPopup,
                                     bool persistUnlock) {
    const auto* definition = Find(id);
    if (!definition) return false;
    auto& save = SaveManager::GetInstance();
    const int bounded = std::clamp(progress, 0, definition->target);
    save.SetAchievementProgress(id, bounded);
    if (bounded < definition->target || save.GetAchievement(id).unlocked) return false;
    if (!save.UnlockAchievement(id, static_cast<std::int64_t>(std::time(nullptr)))) return false;
    if (showPopup) QueuePopup(id);
    if (persistUnlock) save.Save();
    return true;
}

void AchievementManager::QueuePopup(const std::string& id) {
    if (id.empty()) return;
    if (m_activePopup == id || std::find(m_popupQueue.begin(), m_popupQueue.end(), id) != m_popupQueue.end()) return;
    m_popupQueue.push_back(id);
}

int AchievementManager::CountUnlockedRoster() const {
    const auto& save = SaveManager::GetInstance();
    int count = 0;
    for (const char* id : {"knight", "fighter", "magic_caster", "ninja"})
        if (save.IsCharUnlocked(id)) ++count;
    return count;
}

int AchievementManager::CountBestStars() const {
    const auto& save = SaveManager::GetInstance();
    int total = 0;
    for (int level = 1; level <= 6; ++level) total += std::clamp(save.GetLevelBestStars(level), 0, 3);
    return total;
}

void AchievementManager::EvaluateExistingSave(bool showPopups) {
    auto& save = SaveManager::GetInstance();
    // Migrate progression that existed before lifetimeStats was introduced.
    bool hasThreeStarLevel = false;
    bool hasUnderParLevel = false;
    for (int level = 1; level <= 6; ++level) {
        if (save.GetLevelHighScore(level) > 0 || save.GetLevelBestStars(level) > 0)
            save.MarkLevelCompleted(level);
        if (save.GetLevelBestStars(level) >= 3) hasThreeStarLevel = true;
        const float bestTime = save.GetLevelBestTime(level);
        if (bestTime > 0.0f && bestTime <= LoadAchievementParTime(level))
            hasUnderParLevel = true;
    }
    const auto& stats = save.GetLifetimeStats();
    const int completed = static_cast<int>(stats.completedLevels.size());
    bool unlockedAny = false;
    unlockedAny |= SetProgress("tutorial_graduate",
        std::find(stats.completedLevels.begin(), stats.completedLevels.end(), 1) != stats.completedLevels.end(),
        showPopups, false);
    unlockedAny |= SetProgress("road_ahead", completed, showPopups, false);
    unlockedAny |= SetProgress("realm_defender", completed, showPopups, false);
    unlockedAny |= SetProgress("ahead_of_time", hasUnderParLevel ? 1 : 0, showPopups, false);
    unlockedAny |= SetProgress("star_student", hasThreeStarLevel ? 1 : 0, showPopups, false);
    unlockedAny |= SetProgress("star_collector", CountBestStars(), showPopups, false);
    unlockedAny |= SetProgress("first_blood", stats.enemiesDefeated, showPopups, false);
    unlockedAny |= SetProgress("monster_hunter", stats.enemiesDefeated, showPopups, false);
    unlockedAny |= SetProgress("boss_slayer", stats.bossesDefeated, showPopups, false);
    unlockedAny |= SetProgress("coin_collector", stats.coinsCollected, showPopups, false);
    unlockedAny |= SetProgress("first_purchase", stats.shopPurchases, showPopups, false);
    unlockedAny |= SetProgress("full_roster", CountUnlockedRoster(), showPopups, false);
    if (unlockedAny) save.Save();
}

void AchievementManager::OnEnemyDefeated(bool boss) {
    auto& stats = SaveManager::GetInstance().GetLifetimeStats();
    ++stats.enemiesDefeated;
    if (boss) ++stats.bossesDefeated;
    SetProgress("first_blood", stats.enemiesDefeated);
    SetProgress("monster_hunter", stats.enemiesDefeated);
    if (boss) SetProgress("boss_slayer", stats.bossesDefeated);
}

void AchievementManager::OnCoinCollected(int amount) {
    auto& stats = SaveManager::GetInstance().GetLifetimeStats();
    stats.coinsCollected += std::max(0, amount);
    SetProgress("coin_collector", stats.coinsCollected);
}

void AchievementManager::OnPotionUsed() { SetProgress("potion_taster", 1); }

void AchievementManager::OnShopPurchase() {
    auto& stats = SaveManager::GetInstance().GetLifetimeStats();
    ++stats.shopPurchases;
    SetProgress("first_purchase", stats.shopPurchases, true, false);
    SetProgress("full_roster", CountUnlockedRoster(), true, false);
}

void AchievementManager::OnRosterChanged() {
    SetProgress("full_roster", CountUnlockedRoster());
}

void AchievementManager::OnLevelCompleted(int level, int stars, float clearTime,
                                           float parTime, bool flawless, bool localCoop) {
    if (level < 1 || level > 6) return;
    auto& save = SaveManager::GetInstance();
    save.MarkLevelCompleted(level);
    const int completed = static_cast<int>(save.GetLifetimeStats().completedLevels.size());
    SetProgress("tutorial_graduate", level == 1 || IsUnlocked("tutorial_graduate"), true, false);
    SetProgress("road_ahead", completed, true, false);
    SetProgress("realm_defender", completed, true, false);
    if (clearTime > 0.0f && parTime > 0.0f && clearTime <= parTime)
        SetProgress("ahead_of_time", 1, true, false);
    if (flawless) SetProgress("flawless_victory", 1, true, false);
    if (stars >= 3) SetProgress("star_student", 1, true, false);
    SetProgress("star_collector", CountBestStars(), true, false);
    if (localCoop) SetProgress("together_we_win", 1, true, false);
}

int AchievementManager::GetUnlockedCount() const {
    int count = 0;
    for (const auto& definition : m_definitions) if (IsUnlocked(definition.id)) ++count;
    return count;
}

int AchievementManager::GetProgress(const std::string& id) const {
    return SaveManager::GetInstance().GetAchievement(id).progress;
}

bool AchievementManager::IsUnlocked(const std::string& id) const {
    return SaveManager::GetInstance().GetAchievement(id).unlocked;
}

void AchievementManager::Update(float dt) {
    if (!m_initialized) return;
    if (m_activePopup.empty() && !m_popupQueue.empty()) {
        m_activePopup = m_popupQueue.front();
        m_popupQueue.pop_front();
        m_popupTimer = 0.0f;
        if (SoundManager::GetInstance().IsAudioInitialized())
            SoundManager::GetInstance().PlaySound("victory_star");
    }
    if (m_activePopup.empty()) return;
    m_popupTimer += std::max(0.0f, dt);
    if (m_popupTimer >= 4.2f) {
        m_activePopup.clear();
        m_popupTimer = 0.0f;
    }
}

void AchievementManager::RenderPopup() const {
    if (!m_initialized || m_activePopup.empty()) return;
    const auto* definition = Find(m_activePopup);
    if (!definition) return;
    const float sw = static_cast<float>(GetScreenWidth());
    const float sh = static_cast<float>(GetScreenHeight());
    const float width = std::clamp(sw * 0.34f, 330.0f, 480.0f);
    const float height = std::clamp(sh * 0.13f, 92.0f, 118.0f);
    const float enter = std::clamp(m_popupTimer / 0.48f, 0.0f, 1.0f);
    const float leave = std::clamp((4.2f - m_popupTimer) / 0.42f, 0.0f, 1.0f);
    const float visibility = std::min(enter, leave);
    const float x = sw - 18.0f - width + (1.0f - EaseOutBack(enter)) * (width + 30.0f);
    const float y = std::max(18.0f, sh * 0.055f);
    const unsigned char alpha = static_cast<unsigned char>(255.0f * visibility);
    const Rectangle panel{x, y, width, height};
    DrawRectangleRounded({x + 6.0f, y + 8.0f, width, height}, 0.16f, 12, Color{0, 0, 0, static_cast<unsigned char>(130 * visibility)});
    DrawRectangleRounded(panel, 0.16f, 12, Color{23, 16, 39, alpha});
    DrawRectangleRoundedLinesEx(panel, 0.16f, 12, 3.0f, Color{247, 197, 78, alpha});
    const float iconSize = height - 28.0f;
    const Rectangle iconBack{x + 14.0f, y + 14.0f, iconSize, iconSize};
    DrawRectangleRounded(iconBack, 0.25f, 10, Color{80, 54, 112, alpha});
    if (m_trophyIcon.id != 0) {
        DrawTexturePro(m_trophyIcon,
            {0, 0, static_cast<float>(m_trophyIcon.width), static_cast<float>(m_trophyIcon.height)},
            {iconBack.x + 7.0f, iconBack.y + 7.0f, iconBack.width - 14.0f, iconBack.height - 14.0f},
            {0, 0}, 0.0f, Color{255, 255, 255, alpha});
    }
    const Font font = m_font.texture.id != 0 ? m_font : GetFontDefault();
    const float textX = iconBack.x + iconBack.width + 15.0f;
    const float labelSize = std::clamp(height * 0.15f, 13.0f, 17.0f);
    const float titleSize = std::clamp(height * 0.20f, 18.0f, 24.0f);
    DrawTextEx(font, "ACHIEVEMENT UNLOCKED", {textX, y + 15.0f}, labelSize, 1.0f, Color{247, 197, 78, alpha});
    DrawTextEx(font, definition->title.c_str(), {textX, y + 39.0f}, titleSize, 1.0f, Color{255, 248, 222, alpha});
    float descriptionSize = labelSize;
    const float descriptionWidth = x + width - 14.0f - textX;
    while (descriptionSize > 9.0f &&
           MeasureTextEx(font, definition->description.c_str(), descriptionSize, 0.5f).x > descriptionWidth)
        descriptionSize -= 0.5f;
    DrawTextEx(font, definition->description.c_str(), {textX, y + height - 29.0f},
               descriptionSize, 0.5f, Color{203, 194, 222, alpha});
}
