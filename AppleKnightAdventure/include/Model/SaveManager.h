#ifndef SAVEMANAGER_H
#define SAVEMANAGER_H

#include <string>
#include <vector>
#include <map>

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

    std::string GetSelectedChar() const;
    void SetSelectedChar(const std::string& charId);
    
    std::string GetSelectedPet() const;
    void SetSelectedPet(const std::string& petId);

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
    std::string selectedChar = "knight";
    std::string selectedPet = ""; // empty means no pet
};

#endif // SAVEMANAGER_H
