#pragma once

#include "Survival3D/Model/SurvivalTypes.h"
#include "Model/SaveManager.h"
#include <cstdint>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace Survival3D {

enum class SyncState : std::uint8_t {
    LocalOnly,
    OfflineQueued,
    Syncing,
    Synced,
    Rejected
};

struct RunResultInput {
    std::string runId;
    CharacterId character = CharacterId::Knight;
    std::string configVersion;
    int highestWave = 1;
    int score = 0;
    int survivalMs = 0;
    int kills = 0;
    int bossesKilled = 0;
    int damageTaken = 0;
    int peakEnemies = 0;
    int peakProjectiles = 0;
    int droppedTicks = 0;
    float averageFrameMs = 0.0f;
    float peakFrameMs = 0.0f;
    bool victory = false;
};

class SurvivalRunService {
public:
    static SurvivalRunService& GetInstance();

    void Init();
    void Update(float dt);
    void Shutdown();
    std::string BeginRun(CharacterId character, const std::string& configVersion);
    bool FinalizeRun(const RunResultInput& input);
    void RequestLeaderboardRefresh();

    const std::vector<SurvivalRunRecord>& GetTopRuns() const { return m_topRuns; }
    const std::vector<SurvivalRunRecord>& GetRemoteTopRuns() const { return m_remoteTopRuns; }
    bool HasRemoteLeaderboard() const { return m_remoteLeaderboardAvailable; }
    const std::string& GetLeaderboardLabel() const { return m_leaderboardLabel; }
    SyncState GetSyncState() const { return m_syncState; }
    const std::string& GetSyncLabel() const { return m_syncLabel; }
    int GetPendingCount() const;
    int GetLastCoinReward() const { return m_lastCoinReward; }
    const std::string& GetLastValidationError() const { return m_lastValidationError; }

private:
    SurvivalRunService() = default;
    bool Validate(const RunResultInput& input, std::string& reason) const;
    void RefreshTopRuns();
    void LoadServiceConfig();
    void WorkerLoop();
    static std::int64_t NowUnixMs();

    struct TransportJob {
        PendingSurvivalSubmission submission;
        std::string playerId;
        std::string playerName;
        bool leaderboardOnly = false;
    };
    struct TransportResult {
        std::string runId;
        std::string playerId;
        std::string message;
        std::vector<SurvivalRunRecord> leaderboard;
        bool leaderboardResult = false;
        int outcome = 0; // 1 success, 2 rejected, 3 retryable
    };

    bool m_initialized = false;
    SyncState m_syncState = SyncState::LocalOnly;
    std::string m_syncLabel = "LOCAL PROFILE";
    std::string m_lastValidationError;
    std::vector<SurvivalRunRecord> m_topRuns;
    std::vector<SurvivalRunRecord> m_remoteTopRuns;
    std::string m_leaderboardLabel = "LOCAL TOP 5";
    bool m_remoteLeaderboardAvailable = false;
    bool m_leaderboardFetchQueued = false;
    int m_lastCoinReward = 0;
    float m_statusRefreshTimer = 0.0f;
    float m_leaderboardRefreshTimer = 0.0f;
    std::uint64_t m_runCounter = 0;
    bool m_serviceEnabled = false;
    std::string m_baseUrl;
    bool m_workerStop = false;
    bool m_transportBusy = false;
    std::thread m_worker;
    std::mutex m_transportMutex;
    std::condition_variable m_transportCv;
    std::deque<TransportJob> m_jobs;
    std::deque<TransportResult> m_results;
};

} // namespace Survival3D
