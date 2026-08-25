#include "Model/SaveManager.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <filesystem>
#include <ctime>
#include <nlohmann/json.hpp>
#include "raylib.h"

namespace {
std::string NormalizeUnlockId(std::string id) {
    if (id == "Knight") return "knight";
    if (id == "Fighter") return "fighter";
    if (id == "Magic Caster" || id == "MagicCaster") return "magic_caster";
    if (id == "Ninja") return "ninja";
    return id;
}

using json = nlohmann::json;

LeaderboardEntry ReadLeaderboardEntry(const json& value) {
    LeaderboardEntry entry;
    if (!value.is_object()) return entry;
    entry.playerName = value.value("playerName", "Player");
    entry.characterIds = value.value("characters", std::vector<std::string>{"knight"});
    entry.score = std::max(0, value.value("score", 0));
    entry.timeMs = std::max(0, value.value("timeMs", 0));
    entry.stars = std::clamp(value.value("stars", 0), 0, 3);
    entry.localCoop = value.value("localCoop", false);
    entry.completedAt = value.value("completedAt", static_cast<std::int64_t>(0));
    return entry;
}

json WriteLeaderboardEntry(const LeaderboardEntry& entry) {
    return json{
        {"playerName", entry.playerName},
        {"characters", entry.characterIds},
        {"score", entry.score},
        {"timeMs", entry.timeMs},
        {"stars", entry.stars},
        {"localCoop", entry.localCoop},
        {"completedAt", entry.completedAt}
    };
}

SurvivalRunRecord ReadSurvivalRun(const json& value) {
    SurvivalRunRecord run;
    if (!value.is_object()) return run;
    run.runId = value.value("runId", std::string{});
    run.playerName = value.value("playerName", std::string("Player"));
    run.characterId = value.value("characterId", std::string("knight"));
    run.configVersion = value.value("configVersion", std::string{});
    run.highestWave = std::clamp(value.value("highestWave", 1), 1, 50);
    run.score = std::max(0, value.value("score", 0));
    run.survivalMs = std::max(0, value.value("survivalMs", 0));
    run.kills = std::max(0, value.value("kills", 0));
    run.bossesKilled = std::clamp(value.value("bossesKilled", 0), 0, 5);
    run.damageTaken = std::max(0, value.value("damageTaken", 0));
    run.coinReward = std::clamp(value.value("coinReward", 0), 0, 30);
    run.completedAt = value.value("completedAt", static_cast<std::int64_t>(0));
    run.victory = value.value("victory", false);
    run.ranked = value.value("ranked", false);
    run.validationStatus = value.value("validationStatus", std::string("local_validated"));
    return run;
}

json WriteSurvivalRun(const SurvivalRunRecord& run) {
    return json{{"runId", run.runId}, {"playerName", run.playerName},
        {"characterId", run.characterId}, {"configVersion", run.configVersion},
        {"highestWave", run.highestWave}, {"score", run.score},
        {"survivalMs", run.survivalMs}, {"kills", run.kills},
        {"bossesKilled", run.bossesKilled}, {"damageTaken", run.damageTaken},
        {"coinReward", run.coinReward}, {"completedAt", run.completedAt},
        {"victory", run.victory}, {"ranked", run.ranked},
        {"validationStatus", run.validationStatus}};
}
}

SaveManager& SaveManager::GetInstance() {
    static SaveManager instance;
    return instance;
}

SaveManager::SaveManager() 
    : playerName("Player")
    , coins(0)
    , isFirstTimePlaying(true)
    , musicVolume(70)
    , sfxVolume(80)
    , fullscreenEnabled(false)
{
    unlockedCharacters.push_back("knight");
}

bool SaveManager::Load(const std::string& path) {
    auto resetDefaults = [&]() {
        playerName = "Player";
        coins = 0;
        isFirstTimePlaying = true;
        unlockedCharacters = {"knight"};
        levelHighScores.clear();
        levelBestStars.clear();
        levelBestTimesMs.clear();
        leaderboards.clear();
        achievements.clear();
        lifetimeStats = {};
        exploredMinimapCells.clear();
        selectedChar = "knight";
        selectedPet.clear();
        musicVolume = 70;
        sfxVolume = 80;
        fullscreenEnabled = false;
        survivalRuns.clear();
        pendingSurvivalSubmissions.clear();
        claimedSurvivalRunIds.clear();
        survivalServicePlayerId.clear();
        survivalHighContrast = false;
        survivalReducedMotion = false;
        survivalUiScale = 1.0f;
    };

    resetDefaults();
    json root;
    bool loaded = false;
    for (const std::string candidate : {path, path + ".bak"}) {
        char* data = LoadFileText(candidate.c_str());
        if (!data) continue;
        try {
            root = json::parse(data);
            loaded = root.is_object();
        } catch (const std::exception& ex) {
            TraceLog(LOG_WARNING, "SAVE: Invalid JSON in %s: %s", candidate.c_str(), ex.what());
        }
        UnloadFileText(data);
        if (loaded) break;
    }
    if (!loaded) return false;

    playerName = root.value("playerName", "Player");
    coins = std::max(0, root.value("coins", 0));
    isFirstTimePlaying = root.value("isFirstTimePlaying", true);
    musicVolume = std::clamp(root.value("musicVolume", 70), 0, 100);
    sfxVolume = std::clamp(root.value("sfxVolume", 80), 0, 100);
    fullscreenEnabled = root.value("fullscreenEnabled", false);
    unlockedCharacters = root.value("unlockedCharacters", std::vector<std::string>{"knight"});
    if (unlockedCharacters.empty()) unlockedCharacters = {"knight"};
    for (std::string& id : unlockedCharacters) id = NormalizeUnlockId(id);
    if (std::find(unlockedCharacters.begin(), unlockedCharacters.end(), "knight") == unlockedCharacters.end())
        unlockedCharacters.push_back("knight");
    std::sort(unlockedCharacters.begin(), unlockedCharacters.end());
    unlockedCharacters.erase(std::unique(unlockedCharacters.begin(), unlockedCharacters.end()), unlockedCharacters.end());

    auto readIntMap = [&](const char* key, std::map<int, int>& out) {
        if (!root.contains(key) || !root[key].is_object()) return;
        for (auto it = root[key].begin(); it != root[key].end(); ++it) {
            try { out[std::stoi(it.key())] = it.value().get<int>(); }
            catch (...) { /* Ignore a malformed legacy entry, not the whole save. */ }
        }
    };
    readIntMap("levelHighScores", levelHighScores);
    readIntMap("levelBestStars", levelBestStars);
    readIntMap("levelBestTimesMs", levelBestTimesMs);

    selectedChar = NormalizeUnlockId(root.value("selectedChar", "knight"));
    selectedPet = root.value("selectedPet", "");

    if (root.contains("leaderboards") && root["leaderboards"].is_object()) {
        for (auto it = root["leaderboards"].begin(); it != root["leaderboards"].end(); ++it) {
            int level = 0;
            try { level = std::stoi(it.key()); } catch (...) { continue; }
            if (level < 1 || level > 6 || !it.value().is_object()) continue;
            LevelLeaderboard board;
            for (const auto& value : it.value().value("topScores", json::array()))
                board.topScores.push_back(ReadLeaderboardEntry(value));
            for (const auto& value : it.value().value("topTimes", json::array()))
                board.topTimes.push_back(ReadLeaderboardEntry(value));
            if (board.topScores.size() > 5) board.topScores.resize(5);
            if (board.topTimes.size() > 5) board.topTimes.resize(5);
            leaderboards[level] = std::move(board);
        }
    }

    if (root.contains("achievements") && root["achievements"].is_object()) {
        for (auto it = root["achievements"].begin(); it != root["achievements"].end(); ++it) {
            if (!it.value().is_object()) continue;
            AchievementSaveData state;
            state.unlocked = it.value().value("unlocked", false);
            state.progress = std::max(0, it.value().value("progress", 0));
            state.unlockedAt = it.value().value("unlockedAt", static_cast<std::int64_t>(0));
            achievements[it.key()] = state;
        }
    }

    if (root.contains("lifetimeStats") && root["lifetimeStats"].is_object()) {
        const auto& stats = root["lifetimeStats"];
        lifetimeStats.enemiesDefeated = std::max(0, stats.value("enemiesDefeated", 0));
        lifetimeStats.bossesDefeated = std::max(0, stats.value("bossesDefeated", 0));
        lifetimeStats.coinsCollected = std::max(0, stats.value("coinsCollected", 0));
        lifetimeStats.shopPurchases = std::max(0, stats.value("shopPurchases", 0));
        lifetimeStats.completedLevels = stats.value("completedLevels", std::vector<int>{});
        lifetimeStats.completedLevels.erase(
            std::remove_if(lifetimeStats.completedLevels.begin(), lifetimeStats.completedLevels.end(),
                [](int level) { return level < 1 || level > 6; }),
            lifetimeStats.completedLevels.end());
        std::sort(lifetimeStats.completedLevels.begin(), lifetimeStats.completedLevels.end());
        lifetimeStats.completedLevels.erase(
            std::unique(lifetimeStats.completedLevels.begin(), lifetimeStats.completedLevels.end()),
            lifetimeStats.completedLevels.end());
    }

    if (root.contains("minimapExploration") && root["minimapExploration"].is_object()) {
        for (auto it = root["minimapExploration"].begin(); it != root["minimapExploration"].end(); ++it) {
            int level = 0;
            try { level = std::stoi(it.key()); } catch (...) { continue; }
            if (level < 1 || level > 6 || !it.value().is_array()) continue;
            auto& cells = exploredMinimapCells[level];
            for (const auto& value : it.value()) {
                if (!value.is_number_integer()) continue;
                const int encoded = value.get<int>();
                if (encoded >= 0) cells.insert(encoded);
            }
        }
    }

    if (root.contains("survival3d") && root["survival3d"].is_object()) {
        const auto& survival = root["survival3d"];
        for (const auto& value : survival.value("runs", json::array())) {
            SurvivalRunRecord run = ReadSurvivalRun(value);
            if (!run.runId.empty()) survivalRuns.push_back(std::move(run));
        }
        if (survivalRuns.size() > 20) survivalRuns.resize(20);
        for (const auto& value : survival.value("pendingSubmissions", json::array())) {
            if (!value.is_object()) continue;
            PendingSurvivalSubmission submission;
            submission.idempotencyKey = value.value("idempotencyKey", std::string{});
            submission.runId = value.value("runId", std::string{});
            submission.payload = value.value("payload", std::string{});
            submission.retryCount = std::clamp(value.value("retryCount", 0), 0, 20);
            submission.nextAttemptUnixMs = value.value("nextAttemptUnixMs", static_cast<std::int64_t>(0));
            if (!submission.runId.empty() && !submission.idempotencyKey.empty())
                pendingSurvivalSubmissions.push_back(std::move(submission));
        }
        if (pendingSurvivalSubmissions.size() > 50) pendingSurvivalSubmissions.resize(50);
        claimedSurvivalRunIds = survival.value("claimedRewardRunIds", std::vector<std::string>{});
        if (claimedSurvivalRunIds.size() > 100) claimedSurvivalRunIds.resize(100);
        survivalServicePlayerId = survival.value("servicePlayerId", std::string{});
        const auto& accessibility = survival.value("accessibility", json::object());
        survivalHighContrast = accessibility.value("highContrast", false);
        survivalReducedMotion = accessibility.value("reducedMotion", false);
        survivalUiScale = std::clamp(accessibility.value("uiScale", 1.0f), 0.85f, 1.20f);
    }

    return true;
}

void SaveManager::Save(const std::string& path) {
    json root;
    root["saveVersion"] = 2;
    root["playerName"] = playerName;
    root["coins"] = coins;
    root["isFirstTimePlaying"] = isFirstTimePlaying;
    root["musicVolume"] = musicVolume;
    root["sfxVolume"] = sfxVolume;
    root["fullscreenEnabled"] = fullscreenEnabled;
    root["unlockedCharacters"] = unlockedCharacters;
    root["selectedChar"] = selectedChar;
    root["selectedPet"] = selectedPet;

    root["levelHighScores"] = json::object();
    for (const auto& [level, value] : levelHighScores)
        root["levelHighScores"][std::to_string(level)] = value;
    root["levelBestStars"] = json::object();
    for (const auto& [level, value] : levelBestStars)
        root["levelBestStars"][std::to_string(level)] = value;
    root["levelBestTimesMs"] = json::object();
    for (const auto& [level, value] : levelBestTimesMs)
        root["levelBestTimesMs"][std::to_string(level)] = value;

    root["leaderboards"] = json::object();
    for (const auto& [level, board] : leaderboards) {
        json scores = json::array();
        json times = json::array();
        for (const auto& entry : board.topScores) scores.push_back(WriteLeaderboardEntry(entry));
        for (const auto& entry : board.topTimes) times.push_back(WriteLeaderboardEntry(entry));
        root["leaderboards"][std::to_string(level)] = {
            {"topScores", std::move(scores)}, {"topTimes", std::move(times)}
        };
    }

    root["achievements"] = json::object();
    for (const auto& [id, state] : achievements) {
        root["achievements"][id] = {
            {"unlocked", state.unlocked},
            {"progress", state.progress},
            {"unlockedAt", state.unlockedAt}
        };
    }
    root["lifetimeStats"] = {
        {"enemiesDefeated", lifetimeStats.enemiesDefeated},
        {"bossesDefeated", lifetimeStats.bossesDefeated},
        {"coinsCollected", lifetimeStats.coinsCollected},
        {"shopPurchases", lifetimeStats.shopPurchases},
        {"completedLevels", lifetimeStats.completedLevels}
    };
    root["minimapExploration"] = json::object();
    for (const auto& [level, cells] : exploredMinimapCells)
        root["minimapExploration"][std::to_string(level)] =
            std::vector<int>(cells.begin(), cells.end());

    root["survival3d"] = {
        {"schemaVersion", 1},
        {"runs", json::array()},
        {"pendingSubmissions", json::array()},
        {"claimedRewardRunIds", claimedSurvivalRunIds},
        {"servicePlayerId", survivalServicePlayerId},
        {"accessibility", {
            {"highContrast", survivalHighContrast},
            {"reducedMotion", survivalReducedMotion},
            {"uiScale", survivalUiScale}
        }}
    };
    for (const SurvivalRunRecord& run : survivalRuns)
        root["survival3d"]["runs"].push_back(WriteSurvivalRun(run));
    for (const PendingSurvivalSubmission& submission : pendingSurvivalSubmissions) {
        root["survival3d"]["pendingSubmissions"].push_back({
            {"idempotencyKey", submission.idempotencyKey},
            {"runId", submission.runId},
            {"payload", submission.payload},
            {"retryCount", submission.retryCount},
            {"nextAttemptUnixMs", submission.nextAttemptUnixMs}
        });
    }

    std::string jsonStr = root.dump(2) + "\n";
    const std::string tempPath = path + ".tmp";
    const std::string backupPath = path + ".bak";
    if (!SaveFileText(tempPath.c_str(), const_cast<char*>(jsonStr.c_str()))) {
        TraceLog(LOG_ERROR, "SAVE: Could not write temporary save file");
        return;
    }

    std::error_code error;
    if (std::filesystem::exists(path, error)) {
        std::filesystem::copy_file(path, backupPath,
            std::filesystem::copy_options::overwrite_existing, error);
        error.clear();
    }

    // The backup makes the two-step replacement recoverable on platforms
    // where rename cannot overwrite an existing file (notably Windows).
    std::filesystem::remove(path, error);
    error.clear();
    std::filesystem::rename(tempPath, path, error);
    const bool replaced = !error;
    if (!replaced) {
        // Last-resort fallback; the backup remains available for the next load.
        std::filesystem::copy_file(tempPath, path,
            std::filesystem::copy_options::overwrite_existing, error);
        std::filesystem::remove(tempPath, error);
    }
}

std::string SaveManager::GetPlayerName() const { return playerName; }
void SaveManager::SetPlayerName(const std::string& name) { playerName = name; }

int SaveManager::GetCoins() const { return coins; }
void SaveManager::AddCoins(int amount) { coins += amount; }
void SaveManager::SpendCoins(int amount) { 
    coins -= amount;
    if (coins < 0) coins = 0;
}

bool SaveManager::IsCharUnlocked(const std::string& charName) const {
    const std::string normalized = NormalizeUnlockId(charName);
    return std::find(unlockedCharacters.begin(), unlockedCharacters.end(), normalized) != unlockedCharacters.end();
}

void SaveManager::UnlockChar(const std::string& charName) {
    const std::string normalized = NormalizeUnlockId(charName);
    if (!IsCharUnlocked(normalized)) {
        unlockedCharacters.push_back(normalized);
    }
}

bool SaveManager::IsFirstTimePlaying() const { return isFirstTimePlaying; }
void SaveManager::SetFirstTimePlaying(bool v) { isFirstTimePlaying = v; }

int SaveManager::GetLevelHighScore(int level) const {
    auto it = levelHighScores.find(level);
    if (it != levelHighScores.end()) return it->second;
    return 0;
}

void SaveManager::SetLevelHighScore(int level, int score) {
    levelHighScores[level] = std::max(GetLevelHighScore(level), score);
}

int SaveManager::GetLevelBestStars(int level) const {
    auto it = levelBestStars.find(level);
    if (it != levelBestStars.end()) return it->second;
    return 0;
}

void SaveManager::SetLevelBestStars(int level, int stars) {
    levelBestStars[level] = std::max(GetLevelBestStars(level), stars);
}

float SaveManager::GetLevelBestTime(int level) const {
    auto it = levelBestTimesMs.find(level);
    if (it == levelBestTimesMs.end() || it->second <= 0) return 0.0f;
    return it->second / 1000.0f;
}

void SaveManager::SetLevelBestTime(int level, float seconds) {
    if (seconds <= 0.0f) return;
    const int milliseconds = std::max(1, static_cast<int>(seconds * 1000.0f + 0.5f));
    auto it = levelBestTimesMs.find(level);
    if (it == levelBestTimesMs.end() || milliseconds < it->second) {
        levelBestTimesMs[level] = milliseconds;
    }
}

void SaveManager::RecordLevelResult(int level, const LeaderboardEntry& source) {
    if (level < 1 || level > 6 || source.timeMs <= 0) return;
    LeaderboardEntry entry = source;
    entry.score = std::max(0, entry.score);
    entry.stars = std::clamp(entry.stars, 0, 3);
    if (entry.playerName.empty()) entry.playerName = "Player";
    if (entry.characterIds.empty()) entry.characterIds.push_back("knight");
    if (entry.completedAt <= 0) entry.completedAt = static_cast<std::int64_t>(std::time(nullptr));

    auto& board = leaderboards[level];
    board.topScores.push_back(entry);
    std::stable_sort(board.topScores.begin(), board.topScores.end(),
        [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
            if (a.score != b.score) return a.score > b.score;
            if (a.timeMs != b.timeMs) return a.timeMs < b.timeMs;
            return a.completedAt < b.completedAt;
        });
    if (board.topScores.size() > 5) board.topScores.resize(5);

    board.topTimes.push_back(entry);
    std::stable_sort(board.topTimes.begin(), board.topTimes.end(),
        [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
            if (a.timeMs != b.timeMs) return a.timeMs < b.timeMs;
            if (a.score != b.score) return a.score > b.score;
            return a.completedAt < b.completedAt;
        });
    if (board.topTimes.size() > 5) board.topTimes.resize(5);
}

const std::vector<LeaderboardEntry>& SaveManager::GetTopScores(int level) const {
    static const std::vector<LeaderboardEntry> empty;
    const auto it = leaderboards.find(level);
    return it == leaderboards.end() ? empty : it->second.topScores;
}

const std::vector<LeaderboardEntry>& SaveManager::GetTopTimes(int level) const {
    static const std::vector<LeaderboardEntry> empty;
    const auto it = leaderboards.find(level);
    return it == leaderboards.end() ? empty : it->second.topTimes;
}

AchievementSaveData SaveManager::GetAchievement(const std::string& id) const {
    const auto it = achievements.find(id);
    return it == achievements.end() ? AchievementSaveData{} : it->second;
}

void SaveManager::SetAchievementProgress(const std::string& id, int progress) {
    auto& state = achievements[id];
    state.progress = std::max(state.progress, std::max(0, progress));
}

bool SaveManager::UnlockAchievement(const std::string& id, std::int64_t unlockedAt) {
    auto& state = achievements[id];
    if (state.unlocked) return false;
    state.unlocked = true;
    state.unlockedAt = unlockedAt > 0 ? unlockedAt : static_cast<std::int64_t>(std::time(nullptr));
    return true;
}

const std::map<std::string, AchievementSaveData>& SaveManager::GetAchievements() const {
    return achievements;
}

LifetimeStats& SaveManager::GetLifetimeStats() { return lifetimeStats; }
const LifetimeStats& SaveManager::GetLifetimeStats() const { return lifetimeStats; }

void SaveManager::MarkLevelCompleted(int level) {
    if (level < 1 || level > 6) return;
    if (std::find(lifetimeStats.completedLevels.begin(), lifetimeStats.completedLevels.end(), level)
        == lifetimeStats.completedLevels.end()) {
        lifetimeStats.completedLevels.push_back(level);
        std::sort(lifetimeStats.completedLevels.begin(), lifetimeStats.completedLevels.end());
    }
}

void SaveManager::MarkMinimapCellExplored(int level, int encodedCell) {
    if (level < 1 || level > 6 || encodedCell < 0) return;
    exploredMinimapCells[level].insert(encodedCell);
}

const std::set<int>& SaveManager::GetExploredMinimapCells(int level) const {
    static const std::set<int> empty;
    const auto it = exploredMinimapCells.find(level);
    return it == exploredMinimapCells.end() ? empty : it->second;
}

std::string SaveManager::GetSelectedChar() const { return selectedChar; }
void SaveManager::SetSelectedChar(const std::string& charId) { selectedChar = charId; }

std::string SaveManager::GetSelectedPet() const { return selectedPet; }
void SaveManager::SetSelectedPet(const std::string& petId) { selectedPet = petId; }

int SaveManager::GetMusicVolume() const { return musicVolume; }
void SaveManager::SetMusicVolume(int percent) {
    musicVolume = std::clamp(percent, 0, 100);
}

int SaveManager::GetSFXVolume() const { return sfxVolume; }
void SaveManager::SetSFXVolume(int percent) {
    sfxVolume = std::clamp(percent, 0, 100);
}

bool SaveManager::IsFullscreenEnabled() const { return fullscreenEnabled; }
void SaveManager::SetFullscreenEnabled(bool enabled) { fullscreenEnabled = enabled; }

bool SaveManager::RecordSurvivalRun(const SurvivalRunRecord& source) {
    if (source.runId.empty()) return false;
    const auto duplicate = std::find_if(survivalRuns.begin(), survivalRuns.end(),
        [&](const SurvivalRunRecord& run) { return run.runId == source.runId; });
    if (duplicate != survivalRuns.end()) return false;
    SurvivalRunRecord run = source;
    run.highestWave = std::clamp(run.highestWave, 1, 50);
    run.score = std::max(0, run.score);
    run.survivalMs = std::max(0, run.survivalMs);
    run.kills = std::max(0, run.kills);
    if (run.playerName.empty()) run.playerName = playerName.empty() ? "Player" : playerName;
    if (run.completedAt <= 0) run.completedAt = static_cast<std::int64_t>(std::time(nullptr));
    survivalRuns.insert(survivalRuns.begin(), std::move(run));
    if (survivalRuns.size() > 20) survivalRuns.resize(20);
    return true;
}

const std::vector<SurvivalRunRecord>& SaveManager::GetSurvivalRuns() const {
    return survivalRuns;
}

int SaveManager::GetSurvivalHighestWave(const std::string& characterId) const {
    int best = 0;
    for (const auto& run : survivalRuns)
        if (run.characterId == characterId) best = std::max(best, run.highestWave);
    return best;
}

int SaveManager::GetSurvivalBestScore(const std::string& characterId) const {
    int best = 0;
    for (const auto& run : survivalRuns)
        if (run.characterId == characterId) best = std::max(best, run.score);
    return best;
}

bool SaveManager::HasClaimedSurvivalReward(const std::string& runId) const {
    return std::find(claimedSurvivalRunIds.begin(), claimedSurvivalRunIds.end(), runId)
        != claimedSurvivalRunIds.end();
}

void SaveManager::MarkSurvivalRewardClaimed(const std::string& runId) {
    if (runId.empty() || HasClaimedSurvivalReward(runId)) return;
    claimedSurvivalRunIds.insert(claimedSurvivalRunIds.begin(), runId);
    if (claimedSurvivalRunIds.size() > 100) claimedSurvivalRunIds.resize(100);
}

void SaveManager::EnqueueSurvivalSubmission(const PendingSurvivalSubmission& submission) {
    if (submission.runId.empty() || submission.idempotencyKey.empty()) return;
    const auto duplicate = std::find_if(pendingSurvivalSubmissions.begin(), pendingSurvivalSubmissions.end(),
        [&](const PendingSurvivalSubmission& pending) {
            return pending.idempotencyKey == submission.idempotencyKey;
        });
    if (duplicate != pendingSurvivalSubmissions.end()) return;
    pendingSurvivalSubmissions.push_back(submission);
    if (pendingSurvivalSubmissions.size() > 50)
        pendingSurvivalSubmissions.erase(pendingSurvivalSubmissions.begin());
}

std::vector<PendingSurvivalSubmission>& SaveManager::GetPendingSurvivalSubmissions() {
    return pendingSurvivalSubmissions;
}

const std::vector<PendingSurvivalSubmission>& SaveManager::GetPendingSurvivalSubmissions() const {
    return pendingSurvivalSubmissions;
}

const std::string& SaveManager::GetSurvivalServicePlayerId() const {
    return survivalServicePlayerId;
}

void SaveManager::SetSurvivalServicePlayerId(const std::string& playerId) {
    survivalServicePlayerId = playerId.substr(0, 64);
}

bool SaveManager::GetSurvivalHighContrast() const { return survivalHighContrast; }
void SaveManager::SetSurvivalHighContrast(bool enabled) { survivalHighContrast = enabled; }
bool SaveManager::GetSurvivalReducedMotion() const { return survivalReducedMotion; }
void SaveManager::SetSurvivalReducedMotion(bool enabled) { survivalReducedMotion = enabled; }
float SaveManager::GetSurvivalUiScale() const { return survivalUiScale; }
void SaveManager::SetSurvivalUiScale(float scale) {
    survivalUiScale = std::clamp(scale, 0.85f, 1.20f);
}

void SaveManager::SetSurvivalRunValidationStatus(const std::string& runId,
                                                 const std::string& status,
                                                 bool ranked) {
    for (SurvivalRunRecord& run : survivalRuns) {
        if (run.runId != runId) continue;
        run.validationStatus = status;
        run.ranked = ranked;
        return;
    }
}
