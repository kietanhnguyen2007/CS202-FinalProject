#ifndef SAVEMANAGER_H
#define SAVEMANAGER_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstdint>

struct LeaderboardEntry {
    std::string playerName = "Player";
    std::vector<std::string> characterIds;
    int score = 0;
    int timeMs = 0;
    int stars = 0;
    bool localCoop = false;
    std::int64_t completedAt = 0;
};

struct LevelLeaderboard {
    std::vector<LeaderboardEntry> topScores;
    std::vector<LeaderboardEntry> topTimes;
};

struct AchievementSaveData {
    bool unlocked = false;
    int progress = 0;
    std::int64_t unlockedAt = 0;
};

struct LifetimeStats {
    int enemiesDefeated = 0;
    int bossesDefeated = 0;
    int coinsCollected = 0;
    int shopPurchases = 0;
    std::vector<int> completedLevels;
};

struct SurvivalRunRecord {
    std::string runId;
    std::string playerName = "Player";
    std::string characterId = "knight";
    std::string configVersion;
    int highestWave = 1;
    int score = 0;
    int survivalMs = 0;
    int kills = 0;
    int bossesKilled = 0;
    int damageTaken = 0;
    int coinReward = 0;
    std::int64_t completedAt = 0;
    bool victory = false;
    bool ranked = false;
    std::string validationStatus = "local_validated";
};

struct PendingSurvivalSubmission {
    std::string idempotencyKey;
    std::string runId;
    std::string payload;
    int retryCount = 0;
    std::int64_t nextAttemptUnixMs = 0;
};

class SaveManager {
public:
    static SaveManager& GetInstance();

    bool Load(const std::string& path = "save.json");
    void Save(const std::string& path = "save.json");

    std::string GetPlayerName() const;
    void SetPlayerName(const std::string& name);

    int GetCoins() const;
    void AddCoins(int amount);
    void SpendCoins(int amount);

    bool IsCharUnlocked(const std::string& charName) const;
    void UnlockChar(const std::string& charName);

    bool IsFirstTimePlaying() const;
    void SetFirstTimePlaying(bool v);

    int GetLevelHighScore(int level) const;
    void SetLevelHighScore(int level, int score);

    int GetLevelBestStars(int level) const;
    void SetLevelBestStars(int level, int stars);

    float GetLevelBestTime(int level) const;
    void SetLevelBestTime(int level, float seconds);

    void RecordLevelResult(int level, const LeaderboardEntry& entry);
    const std::vector<LeaderboardEntry>& GetTopScores(int level) const;
    const std::vector<LeaderboardEntry>& GetTopTimes(int level) const;

    AchievementSaveData GetAchievement(const std::string& id) const;
    void SetAchievementProgress(const std::string& id, int progress);
    bool UnlockAchievement(const std::string& id, std::int64_t unlockedAt);
    const std::map<std::string, AchievementSaveData>& GetAchievements() const;

    LifetimeStats& GetLifetimeStats();
    const LifetimeStats& GetLifetimeStats() const;
    void MarkLevelCompleted(int level);
    void MarkMinimapCellExplored(int level, int encodedCell);
    const std::set<int>& GetExploredMinimapCells(int level) const;

    std::string GetSelectedChar() const;
    void SetSelectedChar(const std::string& charId);
    
    std::string GetSelectedPet() const;
    void SetSelectedPet(const std::string& petId);

    int GetMusicVolume() const;
    void SetMusicVolume(int percent);
    int GetSFXVolume() const;
    void SetSFXVolume(int percent);
    bool IsFullscreenEnabled() const;
    void SetFullscreenEnabled(bool enabled);

    bool RecordSurvivalRun(const SurvivalRunRecord& run);
    const std::vector<SurvivalRunRecord>& GetSurvivalRuns() const;
    int GetSurvivalHighestWave(const std::string& characterId) const;
    int GetSurvivalBestScore(const std::string& characterId) const;
    bool HasClaimedSurvivalReward(const std::string& runId) const;
    void MarkSurvivalRewardClaimed(const std::string& runId);
    void EnqueueSurvivalSubmission(const PendingSurvivalSubmission& submission);
    std::vector<PendingSurvivalSubmission>& GetPendingSurvivalSubmissions();
    const std::vector<PendingSurvivalSubmission>& GetPendingSurvivalSubmissions() const;
    const std::string& GetSurvivalServicePlayerId() const;
    void SetSurvivalServicePlayerId(const std::string& playerId);
    bool GetSurvivalHighContrast() const;
    void SetSurvivalHighContrast(bool enabled);
    bool GetSurvivalReducedMotion() const;
    void SetSurvivalReducedMotion(bool enabled);
    float GetSurvivalUiScale() const;
    void SetSurvivalUiScale(float scale);
    void SetSurvivalRunValidationStatus(const std::string& runId, const std::string& status,
                                        bool ranked);

private:
    SaveManager();
    ~SaveManager() = default;
    SaveManager(const SaveManager&) = delete;
    SaveManager& operator=(const SaveManager&) = delete;

    std::string playerName;
    int coins;
    std::vector<std::string> unlockedCharacters;
    bool isFirstTimePlaying;
    std::map<int, int> levelHighScores;
    std::map<int, int> levelBestStars;
    std::map<int, int> levelBestTimesMs;
    std::map<int, LevelLeaderboard> leaderboards;
    std::map<std::string, AchievementSaveData> achievements;
    LifetimeStats lifetimeStats;
    std::map<int, std::set<int>> exploredMinimapCells;
    std::string selectedChar = "knight";
    std::string selectedPet = ""; // empty means no pet
    int musicVolume = 70;
    int sfxVolume = 80;
    bool fullscreenEnabled = false;
    std::vector<SurvivalRunRecord> survivalRuns;
    std::vector<PendingSurvivalSubmission> pendingSurvivalSubmissions;
    std::vector<std::string> claimedSurvivalRunIds;
    std::string survivalServicePlayerId;
    bool survivalHighContrast = false;
    bool survivalReducedMotion = false;
    float survivalUiScale = 1.0f;
};

#endif // SAVEMANAGER_H
