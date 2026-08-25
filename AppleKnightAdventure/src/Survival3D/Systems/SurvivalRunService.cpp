#include "Survival3D/Systems/SurvivalRunService.h"
#include "Model/SaveManager.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#include <windows.h>
#include <winhttp.h>
#endif

namespace {

#ifdef _WIN32
std::wstring ToWide(const std::string& value) {
    if (value.empty()) return {};
    const int count = MultiByteToWideChar(CP_UTF8, 0, value.data(), (int)value.size(), nullptr, 0);
    std::wstring result(count, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), (int)value.size(), result.data(), count);
    return result;
}

bool HttpRequest(const std::string& method, const std::string& url,
                 const std::string& body, const std::vector<std::string>& headers,
                 int& status, std::string& response) {
    status = 0;
    response.clear();
    const std::wstring wideUrl = ToWide(url);
    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);
    wchar_t host[256]{};
    wchar_t path[2048]{};
    wchar_t extra[2048]{};
    parts.lpszHostName = host;
    parts.dwHostNameLength = 255;
    parts.lpszUrlPath = path;
    parts.dwUrlPathLength = 2047;
    parts.lpszExtraInfo = extra;
    parts.dwExtraInfoLength = 2047;
    if (!WinHttpCrackUrl(wideUrl.c_str(), 0, 0, &parts)) return false;
    HINTERNET session = WinHttpOpen(L"AegisRift/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) return false;
    WinHttpSetTimeouts(session, 3000, 3000, 3000, 8000);
    HINTERNET connection = WinHttpConnect(session, host, parts.nPort, 0);
    if (!connection) { WinHttpCloseHandle(session); return false; }
    const DWORD flags = parts.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;
    std::wstring requestTarget(parts.lpszUrlPath, parts.dwUrlPathLength);
    if (parts.lpszExtraInfo && parts.dwExtraInfoLength > 0)
        requestTarget.append(parts.lpszExtraInfo, parts.dwExtraInfoLength);
    const std::wstring wideMethod = ToWide(method);
    HINTERNET request = WinHttpOpenRequest(connection, wideMethod.c_str(), requestTarget.c_str(), nullptr,
        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!request) {
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return false;
    }
    std::wstring headerBlock = L"Content-Type: application/json\r\n";
    for (const std::string& header : headers) headerBlock += ToWide(header) + L"\r\n";
    LPVOID requestBody = body.empty() ? WINHTTP_NO_REQUEST_DATA
                                     : const_cast<char*>(body.data());
    const bool sent = WinHttpSendRequest(request, headerBlock.c_str(), (DWORD)-1L,
        requestBody, (DWORD)body.size(), (DWORD)body.size(), 0)
        && WinHttpReceiveResponse(request, nullptr);
    if (sent) {
        DWORD statusCode = 0;
        DWORD statusSize = sizeof(statusCode);
        WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            nullptr, &statusCode, &statusSize, nullptr);
        status = (int)statusCode;
        for (;;) {
            DWORD available = 0;
            if (!WinHttpQueryDataAvailable(request, &available) || available == 0) break;
            const std::size_t offset = response.size();
            response.resize(offset + available);
            DWORD read = 0;
            if (!WinHttpReadData(request, response.data() + offset, available, &read)) break;
            response.resize(offset + read);
        }
    }
    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);
    return sent;
}
#else
bool HttpRequest(const std::string&, const std::string&, const std::string&,
                 const std::vector<std::string>&, int&, std::string&) { return false; }
#endif

} // namespace

namespace Survival3D {

SurvivalRunService& SurvivalRunService::GetInstance() {
    static SurvivalRunService instance;
    return instance;
}

std::int64_t SurvivalRunService::NowUnixMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

void SurvivalRunService::Init() {
    if (m_initialized) return;
    m_initialized = true;
    LoadServiceConfig();
    RefreshTopRuns();
    const int pending = GetPendingCount();
    m_syncState = pending > 0 ? SyncState::OfflineQueued : SyncState::LocalOnly;
    m_syncLabel = pending > 0 ? "OFFLINE - SUBMISSIONS QUEUED" : "LOCAL PROFILE";
    if (m_serviceEnabled) {
        {
            std::lock_guard<std::mutex> lock(m_transportMutex);
            m_workerStop = false;
            m_transportBusy = false;
            m_leaderboardFetchQueued = false;
            m_jobs.clear();
            m_results.clear();
        }
        m_worker = std::thread(&SurvivalRunService::WorkerLoop, this);
        RequestLeaderboardRefresh();
    }
}

void SurvivalRunService::LoadServiceConfig() {
    m_serviceEnabled = false;
    m_baseUrl.clear();
    try {
        std::ifstream file("assets/survival3d/config/services.json");
        if (!file) return;
        nlohmann::json root;
        file >> root;
        m_serviceEnabled = root.value("enabled", false);
        m_baseUrl = root.value("baseUrl", std::string{});
        while (!m_baseUrl.empty() && m_baseUrl.back() == '/') m_baseUrl.pop_back();
        if (m_baseUrl.rfind("http://", 0) != 0 && m_baseUrl.rfind("https://", 0) != 0)
            m_serviceEnabled = false;
    } catch (...) {
        m_serviceEnabled = false;
        m_baseUrl.clear();
    }
}

void SurvivalRunService::Update(float dt) {
    if (!m_initialized) return;
    const float safeDt = std::max(0.0f, dt);
    m_statusRefreshTimer += safeDt;
    m_leaderboardRefreshTimer += safeDt;
    if (m_statusRefreshTimer < 1.0f) return;
    m_statusRefreshTimer = 0.0f;
    auto& save = SaveManager::GetInstance();
    auto& queue = save.GetPendingSurvivalSubmissions();
    const std::int64_t now = NowUnixMs();

    std::deque<TransportResult> completed;
    {
        std::lock_guard<std::mutex> lock(m_transportMutex);
        completed.swap(m_results);
    }
    bool refreshLeaderboardAfterSync = false;
    for (const TransportResult& result : completed) {
        if (result.leaderboardResult) {
            m_leaderboardFetchQueued = false;
            if (result.outcome == 1) {
                m_remoteTopRuns = result.leaderboard;
                m_remoteLeaderboardAvailable = true;
                m_leaderboardLabel = "GLOBAL RANKED - LIVE";
            } else {
                m_leaderboardLabel = m_remoteLeaderboardAvailable
                    ? "GLOBAL RANKED - CACHED" : "GLOBAL OFFLINE - LOCAL FALLBACK";
            }
            continue;
        }
        m_transportBusy = false;
        if (!result.playerId.empty()) save.SetSurvivalServicePlayerId(result.playerId);
        auto pending = std::find_if(queue.begin(), queue.end(),
            [&](const PendingSurvivalSubmission& item) { return item.runId == result.runId; });
        if (pending == queue.end()) continue;
        if (result.outcome == 1 || result.outcome == 2) {
            save.SetSurvivalRunValidationStatus(result.runId,
                result.outcome == 1 ? "validated" : "rejected", result.outcome == 1);
            queue.erase(pending);
            m_syncState = result.outcome == 1 ? SyncState::Synced : SyncState::Rejected;
            m_syncLabel = result.outcome == 1 ? "RANKED - SYNCED" : "UNRANKED - REJECTED";
            refreshLeaderboardAfterSync = result.outcome == 1;
        } else {
            pending->retryCount = std::min(20, pending->retryCount + 1);
            const int shift = std::min(7, pending->retryCount);
            const std::int64_t delay = std::min<std::int64_t>(300000, 2000ll << shift);
            pending->nextAttemptUnixMs = now + delay;
            m_syncState = SyncState::OfflineQueued;
            m_syncLabel = "NETWORK ERROR - RETRY QUEUED";
        }
        save.Save();
    }
    if (refreshLeaderboardAfterSync || m_leaderboardRefreshTimer >= 30.0f) {
        m_leaderboardRefreshTimer = 0.0f;
        RequestLeaderboardRefresh();
    }

    if (m_serviceEnabled && !m_transportBusy) {
        const auto due = std::find_if(queue.begin(), queue.end(),
            [&](const PendingSurvivalSubmission& item) { return item.nextAttemptUnixMs <= now; });
        if (due != queue.end()) {
            TransportJob job{*due, save.GetSurvivalServicePlayerId(), save.GetPlayerName()};
            {
                std::lock_guard<std::mutex> lock(m_transportMutex);
                m_jobs.push_back(std::move(job));
                m_transportBusy = true;
            }
            m_syncState = SyncState::Syncing;
            m_syncLabel = "SYNCING RIFT RECORD";
            m_transportCv.notify_one();
        }
    }
    const int pending = static_cast<int>(queue.size());
    if (m_transportBusy) {
        m_syncState = SyncState::Syncing;
        m_syncLabel = "SYNCING RIFT RECORD";
    } else if (pending > 0 && m_syncState != SyncState::Rejected) {
        m_syncState = SyncState::OfflineQueued;
        m_syncLabel = (m_serviceEnabled ? "RETRY - " : "OFFLINE - ")
                    + std::to_string(pending) + " QUEUED";
    } else {
        m_syncState = SyncState::LocalOnly;
        m_syncLabel = "LOCAL PROFILE";
    }
}

void SurvivalRunService::Shutdown() {
    if (!m_initialized) return;
    if (m_worker.joinable()) {
        {
            std::lock_guard<std::mutex> lock(m_transportMutex);
            m_workerStop = true;
        }
        m_transportCv.notify_all();
        m_worker.join();
    }
    SaveManager::GetInstance().Save();
    {
        std::lock_guard<std::mutex> lock(m_transportMutex);
        m_jobs.clear();
        m_results.clear();
        m_transportBusy = false;
        m_leaderboardFetchQueued = false;
    }
    m_initialized = false;
}

void SurvivalRunService::RequestLeaderboardRefresh() {
    if (!m_serviceEnabled || !m_initialized || m_leaderboardFetchQueued) return;
    TransportJob job;
    job.leaderboardOnly = true;
    {
        std::lock_guard<std::mutex> lock(m_transportMutex);
        if (m_workerStop) return;
        m_jobs.push_back(std::move(job));
        m_leaderboardFetchQueued = true;
    }
    if (!m_remoteLeaderboardAvailable) m_leaderboardLabel = "GLOBAL RANKED - CONNECTING";
    m_transportCv.notify_one();
}

void SurvivalRunService::WorkerLoop() {
    for (;;) {
        TransportJob job;
        {
            std::unique_lock<std::mutex> lock(m_transportMutex);
            m_transportCv.wait(lock, [&] { return m_workerStop || !m_jobs.empty(); });
            if (m_workerStop) return;
            job = std::move(m_jobs.front());
            m_jobs.pop_front();
        }

        TransportResult result;
        if (job.leaderboardOnly) {
            result.leaderboardResult = true;
            int status = 0;
            std::string response;
            if (!HttpRequest("GET", m_baseUrl + "/v1/leaderboards/score?limit=5",
                             {}, {}, status, response)
                || status < 200 || status >= 300) {
                result.outcome = 3;
            } else {
                try {
                    const auto root = nlohmann::json::parse(response);
                    const auto& entries = root.at("entries");
                    if (!entries.is_array()) throw std::runtime_error("entries must be an array");
                    for (const auto& entry : entries) {
                        SurvivalRunRecord run;
                        run.playerName = entry.value("player", std::string("Player"));
                        run.characterId = entry.value("characterId", std::string("knight"));
                        run.highestWave = entry.value("wave", 1);
                        run.score = entry.value("score", 0);
                        run.survivalMs = entry.value("timeMs", 0);
                        run.ranked = true;
                        run.validationStatus = "validated";
                        result.leaderboard.push_back(std::move(run));
                    }
                    result.outcome = 1;
                } catch (...) {
                    result.leaderboard.clear();
                    result.outcome = 3;
                }
            }
            {
                std::lock_guard<std::mutex> lock(m_transportMutex);
                m_results.push_back(std::move(result));
            }
            continue;
        }
        result.runId = job.submission.runId;
        result.playerId = job.playerId;
        int status = 0;
        std::string response;
        if (result.playerId.empty()) {
            const std::string authBody = nlohmann::json{{"displayName", job.playerName}}.dump();
            if (!HttpRequest("POST", m_baseUrl + "/v1/auth/guest", authBody, {}, status, response)
                || status < 200 || status >= 300) {
                result.outcome = status >= 400 && status < 500 ? 2 : 3;
            } else {
                try { result.playerId = nlohmann::json::parse(response).value("playerId", std::string{}); }
                catch (...) { result.playerId.clear(); }
                if (result.playerId.empty()) result.outcome = 3;
            }
        }
        if (result.outcome == 0) {
            response.clear();
            const std::string endpoint = m_baseUrl + "/v1/runs/"
                + job.submission.runId + "/complete";
            const std::vector<std::string> headers{
                "X-Player-Id: " + result.playerId,
                "Idempotency-Key: " + job.submission.idempotencyKey
            };
            if (!HttpRequest("POST", endpoint, job.submission.payload, headers, status, response)) result.outcome = 3;
            else if (status >= 200 && status < 300) result.outcome = 1;
            else if (status >= 400 && status < 500) result.outcome = 2;
            else result.outcome = 3;
        }
        result.message = response;
        {
            std::lock_guard<std::mutex> lock(m_transportMutex);
            m_results.push_back(std::move(result));
        }
    }
}

std::string SurvivalRunService::BeginRun(CharacterId character,
                                         const std::string& configVersion) {
    if (!m_initialized) Init();
    const std::uint64_t now = static_cast<std::uint64_t>(NowUnixMs());
    std::uint64_t hash = 1469598103934665603ull;
    const std::string material = std::to_string(now) + ":" + std::to_string(++m_runCounter)
        + ":" + configVersion + ":" + (character == CharacterId::Knight ? "knight" : "magic_caster");
    for (unsigned char byte : material) {
        hash ^= byte;
        hash *= 1099511628211ull;
    }
    std::ostringstream id;
    id << std::hex << std::setfill('0') << std::setw(16) << now
       << std::setw(16) << hash;
    m_lastCoinReward = 0;
    m_lastValidationError.clear();
    return id.str();
}

bool SurvivalRunService::Validate(const RunResultInput& input, std::string& reason) const {
    if (input.runId.size() < 16 || input.runId.size() > 64) reason = "INVALID RUN ID";
    else if (input.configVersion.empty()) reason = "MISSING BALANCE VERSION";
    else if (input.highestWave < 1 || input.highestWave > 50) reason = "INVALID WAVE";
    else if (input.score < 0 || input.score > 100000000) reason = "INVALID SCORE";
    else if (input.survivalMs < 0 || input.survivalMs > 8 * 60 * 60 * 1000) reason = "INVALID TIME";
    else if (input.kills < 0 || input.kills > 100000) reason = "INVALID KILL COUNT";
    else if (input.bossesKilled < 0 || input.bossesKilled > 5) reason = "INVALID BOSS COUNT";
    else if (input.bossesKilled > input.highestWave / 10) reason = "BOSS/WAVE MISMATCH";
    else if (input.victory && input.highestWave != 50) reason = "INVALID VICTORY STATE";
    else if (input.damageTaken < 0 || input.damageTaken > 10000000) reason = "INVALID DAMAGE";
    else return true;
    return false;
}

bool SurvivalRunService::FinalizeRun(const RunResultInput& input) {
    if (!m_initialized) Init();
    std::string reason;
    if (!Validate(input, reason)) {
        m_lastValidationError = reason;
        m_syncState = SyncState::Rejected;
        m_syncLabel = "UNRANKED - " + reason;
        return false;
    }

    auto& save = SaveManager::GetInstance();
    SurvivalRunRecord record;
    record.runId = input.runId;
    record.playerName = save.GetPlayerName();
    record.characterId = input.character == CharacterId::Knight ? "knight" : "magic_caster";
    record.configVersion = input.configVersion;
    record.highestWave = input.highestWave;
    record.score = input.score;
    record.survivalMs = input.survivalMs;
    record.kills = input.kills;
    record.bossesKilled = input.bossesKilled;
    record.damageTaken = input.damageTaken;
    record.completedAt = NowUnixMs() / 1000;
    record.victory = input.victory;
    record.ranked = false;
    record.validationStatus = "local_validated";
    record.coinReward = std::min(30, 2 + input.highestWave / 5 + input.bossesKilled * 2);

    if (!save.RecordSurvivalRun(record)) return false;
    if (!save.HasClaimedSurvivalReward(record.runId)) {
        save.AddCoins(record.coinReward);
        save.MarkSurvivalRewardClaimed(record.runId);
        m_lastCoinReward = record.coinReward;
    }
    auto& lifetime = save.GetLifetimeStats();
    lifetime.enemiesDefeated += record.kills;
    lifetime.bossesDefeated += record.bossesKilled;

    nlohmann::json payload = {
        {"runId", record.runId}, {"characterId", record.characterId},
        {"configVersion", record.configVersion}, {"highestWave", record.highestWave},
        {"score", record.score}, {"survivalMs", record.survivalMs},
        {"kills", record.kills}, {"bossesKilled", record.bossesKilled},
        {"damageTaken", record.damageTaken}, {"victory", record.victory},
        {"performance", {{"peakEnemies", input.peakEnemies},
            {"peakProjectiles", input.peakProjectiles}, {"droppedTicks", input.droppedTicks},
            {"averageFrameMs", input.averageFrameMs}, {"peakFrameMs", input.peakFrameMs}}}
    };
    PendingSurvivalSubmission submission;
    submission.idempotencyKey = record.runId;
    submission.runId = record.runId;
    submission.payload = payload.dump();
    submission.nextAttemptUnixMs = NowUnixMs() + 2000;
    save.EnqueueSurvivalSubmission(submission);
    save.Save();

    std::error_code telemetryError;
    std::filesystem::create_directories("telemetry", telemetryError);
    std::ofstream telemetry("telemetry/survival_runs.jsonl", std::ios::app);
    if (telemetry) {
        payload["completedAt"] = record.completedAt;
        payload["coinReward"] = record.coinReward;
        telemetry << payload.dump() << '\n';
    }

    RefreshTopRuns();
    m_syncState = SyncState::OfflineQueued;
    m_syncLabel = "OFFLINE - SUBMISSION QUEUED";
    return true;
}

void SurvivalRunService::RefreshTopRuns() {
    m_topRuns = SaveManager::GetInstance().GetSurvivalRuns();
    std::stable_sort(m_topRuns.begin(), m_topRuns.end(),
        [](const SurvivalRunRecord& a, const SurvivalRunRecord& b) {
            if (a.score != b.score) return a.score > b.score;
            if (a.highestWave != b.highestWave) return a.highestWave > b.highestWave;
            if (a.survivalMs != b.survivalMs) return a.survivalMs < b.survivalMs;
            return a.completedAt < b.completedAt;
        });
    if (m_topRuns.size() > 5) m_topRuns.resize(5);
}

int SurvivalRunService::GetPendingCount() const {
    return static_cast<int>(SaveManager::GetInstance().GetPendingSurvivalSubmissions().size());
}

} // namespace Survival3D
