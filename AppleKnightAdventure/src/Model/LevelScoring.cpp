#include "Model/LevelScoring.h"
#include <algorithm>

LevelScoring::LevelScoring()
    : m_currentScore(0)
    , m_highScore(0)
    , m_stars(0)
    , m_collectedItems(0)
    , m_totalItems(0)
    , m_defeatedEnemies(0)
    , m_totalEnemies(0)
    , m_clearTime(0.0f)
{
}

int LevelScoring::GetCurrentScore() const { return m_currentScore; }

void LevelScoring::AddScore(int points) {
    m_currentScore += std::max(0, points);
}

int LevelScoring::GetHighScore() const { return m_highScore; }
void LevelScoring::SetHighScore(int score) { m_highScore = std::max(m_highScore, score); }

int LevelScoring::GetStars() const { return m_stars; }

void LevelScoring::CalculateStars() {
    float itemRatio = (m_totalItems > 0) ?
        static_cast<float>(m_collectedItems) / m_totalItems : 0.0f;
    float enemyRatio = (m_totalEnemies > 0) ?
        static_cast<float>(m_defeatedEnemies) / m_totalEnemies : 0.0f;
    float scoreBonus = (m_highScore > 0) ?
        static_cast<float>(m_currentScore) / m_highScore : 1.0f;
    float performance = (itemRatio + enemyRatio + scoreBonus) / 3.0f;
    if (performance >= 0.9f) {
        m_stars = 3;
    } else if (performance >= 0.6f) {
        m_stars = 2;
    } else if (performance >= 0.3f) {
        m_stars = 1;
    } else {
        m_stars = 0;
    }
}

void LevelScoring::CollectItem() { m_collectedItems++; }
void LevelScoring::DefeatEnemy() { m_defeatedEnemies++; }
void LevelScoring::SetClearTime(float time) { m_clearTime = time; }
void LevelScoring::SetTotals(int items, int enemies) {
    m_totalItems = items;
    m_totalEnemies = enemies;
}

float LevelScoring::GetClearTime() const { return m_clearTime; }
int LevelScoring::GetCollectedItems() const { return m_collectedItems; }
int LevelScoring::GetDefeatedEnemies() const { return m_defeatedEnemies; }

bool LevelScoring::IsNewHighScore() const {
    return m_currentScore > m_highScore;
}

void LevelScoring::SaveScore(const std::string& playerName) {
    if (m_currentScore > m_highScore) {
        m_highScore = m_currentScore;
    }
    m_leaderboard.push_back({playerName, m_currentScore, m_stars});
    std::sort(m_leaderboard.begin(), m_leaderboard.end(),
        [](const ScoreEntry& a, const ScoreEntry& b) {
            return a.score > b.score;
        });
}

const std::vector<ScoreEntry>& LevelScoring::GetLeaderboard() const {
    return m_leaderboard;
}
