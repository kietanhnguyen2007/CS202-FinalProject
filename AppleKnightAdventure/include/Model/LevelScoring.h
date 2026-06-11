#ifndef LEVELSCORING_H
#define LEVELSCORING_H

#include <string>
#include <vector>

struct ScoreEntry {
    std::string playerName;
    int score;
    int stars;
};

class LevelScoring {
protected:
    int m_currentScore;
    int m_highScore;
    int m_stars;
    int m_collectedItems;
    int m_totalItems;
    int m_defeatedEnemies;
    int m_totalEnemies;
    float m_clearTime;
    std::vector<ScoreEntry> m_leaderboard;

public:
    LevelScoring();

    int GetCurrentScore() const;
    void AddScore(int points);
    int GetHighScore() const;
    void SetHighScore(int score);

    int GetStars() const;
    void CalculateStars();

    void CollectItem();
    void DefeatEnemy();
    void SetClearTime(float time);
    void SetTotals(int items, int enemies);

    float GetClearTime() const;
    int GetCollectedItems() const;
    int GetDefeatedEnemies() const;

    bool IsNewHighScore() const;
    void SaveScore(const std::string& playerName);
    const std::vector<ScoreEntry>& GetLeaderboard() const;
};

#endif
