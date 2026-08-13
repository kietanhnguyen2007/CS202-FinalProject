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
    , m_parTime(240.0f)
    , m_performance(0.0f)
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
    // Completion always earns one star. Optional mastery is based on combat,
    // exploration and a data-driven par time; high score is deliberately not
    // used, so first-time players are graded fairly.
    m_performance = GetEnemyRatio() * 0.40f
                  + GetItemRatio() * 0.35f
                  + GetTimeRatio() * 0.25f;
    m_performance = std::clamp(m_performance, 0.0f, 1.0f);
    m_stars = 1;
    if (m_performance >= 0.60f) m_stars = 2;
    if (m_performance >= 0.85f) m_stars = 3;
}

void LevelScoring::CollectItem() { m_collectedItems++; }
void LevelScoring::DefeatEnemy() { m_defeatedEnemies++; }
void LevelScoring::SetClearTime(float time) { m_clearTime = time; }
void LevelScoring::SetParTime(float time) { m_parTime = std::max(1.0f, time); }
void LevelScoring::SetTotals(int items, int enemies) {
    m_totalItems = items;
    m_totalEnemies = enemies;
}

float LevelScoring::GetClearTime() const { return m_clearTime; }
int LevelScoring::GetCollectedItems() const { return m_collectedItems; }
int LevelScoring::GetDefeatedEnemies() const { return m_defeatedEnemies; }
int LevelScoring::GetTotalItems() const { return m_totalItems; }
int LevelScoring::GetTotalEnemies() const { return m_totalEnemies; }
float LevelScoring::GetParTime() const { return m_parTime; }
float LevelScoring::GetItemRatio() const {
    if (m_totalItems <= 0) return 1.0f;
    return std::clamp(static_cast<float>(m_collectedItems) / m_totalItems, 0.0f, 1.0f);
}
float LevelScoring::GetEnemyRatio() const {
    if (m_totalEnemies <= 0) return 1.0f;
    return std::clamp(static_cast<float>(m_defeatedEnemies) / m_totalEnemies, 0.0f, 1.0f);
}
float LevelScoring::GetTimeRatio() const {
    if (m_clearTime <= m_parTime) return 1.0f;
    return std::clamp(1.0f - (m_clearTime - m_parTime) / m_parTime, 0.0f, 1.0f);
}
float LevelScoring::GetPerformance() const { return m_performance; }

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
