#ifndef SAVEMANAGER_H
#define SAVEMANAGER_H

#include <string>
#include <vector>
#include <map>
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
    std::string selectedChar = "knight";
    std::string selectedPet = ""; // empty means no pet
    int musicVolume = 70;
    int sfxVolume = 80;
    bool fullscreenEnabled = false;
};

#endif // SAVEMANAGER_H
