#include "SurvivalServerCore.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <limits>
#include <random>
#include <sstream>
#include <vector>

namespace SurvivalBackend {
namespace {

long long nowSeconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string trim(std::string value) {
    const auto notSpace = [](unsigned char c) { return std::isspace(c) == 0; };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    return value;
}

std::string makeId() {
    static std::atomic<unsigned long long> sequence{0};
    static std::mt19937_64 randomEngine(std::random_device{}());
    static std::mutex randomMutex;

    unsigned long long randomPart = 0;
    {
        std::lock_guard<std::mutex> lock(randomMutex);
        randomPart = randomEngine();
    }
    const auto ticks = static_cast<unsigned long long>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());
    const auto counter = sequence.fetch_add(1, std::memory_order_relaxed);
    std::ostringstream id;
    id << std::hex << std::setfill('0')
       << std::setw(16) << (ticks ^ randomPart)
       << std::setw(8) << static_cast<unsigned int>(counter);
    return id.str();
}

long long integerField(const nlohmann::json& payload, const char* name) {
    const auto it = payload.find(name);
    if (it == payload.end() || !it->is_number_integer()) {
        throw ValidationError(std::string(name) + " must be an integer");
    }
    try {
        return it->get<long long>();
    } catch (const nlohmann::json::exception&) {
        throw ValidationError(std::string(name) + " outside accepted range");
    }
}

std::string stringField(const nlohmann::json& payload, const char* name) {
    const auto it = payload.find(name);
    if (it == payload.end() || !it->is_string()) {
        throw ValidationError(std::string(name) + " must be a string");
    }
    return it->get<std::string>();
}

bool runIsBetter(const nlohmann::json& left, const nlohmann::json& right) {
    if (left["score"] != right["score"]) {
        return left["score"].get<long long>() > right["score"].get<long long>();
    }
    if (left["highestWave"] != right["highestWave"]) {
        return left["highestWave"].get<int>() > right["highestWave"].get<int>();
    }
    return left["survivalMs"].get<long long>() < right["survivalMs"].get<long long>();
}

} // namespace

SurvivalServerCore::SurvivalServerCore(std::filesystem::path dataPath)
    : dataPath_(std::move(dataPath)) {
    load();
}

void SurvivalServerCore::load() {
    std::lock_guard<std::mutex> lock(mutex_);
    data_ = {
        {"schemaVersion", 1},
        {"players", nlohmann::json::array()},
        {"runs", nlohmann::json::array()}
    };

    if (!std::filesystem::exists(dataPath_)) {
        return;
    }

    std::ifstream input(dataPath_, std::ios::binary);
    nlohmann::json loaded;
    try {
        input >> loaded;
    } catch (const nlohmann::json::exception& error) {
        throw std::runtime_error("Cannot parse server data: " + std::string(error.what()));
    }
    if (!loaded.is_object() || !loaded.contains("players") || !loaded["players"].is_array()
        || !loaded.contains("runs") || !loaded["runs"].is_array()) {
        throw std::runtime_error("Server data has an invalid schema");
    }
    data_ = std::move(loaded);
}

void SurvivalServerCore::saveLocked() const {
    if (dataPath_.has_parent_path()) {
        std::filesystem::create_directories(dataPath_.parent_path());
    }
    const std::filesystem::path temporary = dataPath_.string() + ".tmp";
    const std::filesystem::path backup = dataPath_.string() + ".bak";
    {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output) {
            throw std::runtime_error("Cannot write server data");
        }
        output << data_.dump(2);
        output.flush();
        if (!output) {
            throw std::runtime_error("Cannot finish writing server data");
        }
    }

    std::error_code error;
    std::filesystem::remove(backup, error);
    error.clear();
    if (std::filesystem::exists(dataPath_)) {
        std::filesystem::rename(dataPath_, backup, error);
        if (error) {
            std::filesystem::remove(temporary);
            throw std::runtime_error("Cannot rotate server data: " + error.message());
        }
    }
    std::filesystem::rename(temporary, dataPath_, error);
    if (error) {
        if (std::filesystem::exists(backup)) {
            std::error_code restoreError;
            std::filesystem::rename(backup, dataPath_, restoreError);
        }
        std::filesystem::remove(temporary);
        throw std::runtime_error("Cannot replace server data: " + error.message());
    }
    std::filesystem::remove(backup, error);
}

bool SurvivalServerCore::playerExistsLocked(const std::string& playerId) const {
    return std::any_of(data_["players"].begin(), data_["players"].end(),
        [&playerId](const nlohmann::json& player) {
            return player.value("id", "") == playerId;
        });
}

nlohmann::json SurvivalServerCore::createGuest(const std::string& displayName) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string cleanName = trim(displayName);
    if (cleanName.empty()) {
        cleanName = "Player";
    }
    if (cleanName.size() > 24) {
        cleanName.resize(24);
    }
    const std::string playerId = makeId();
    data_["players"].push_back({
        {"id", playerId},
        {"displayName", cleanName},
        {"createdAt", nowSeconds()}
    });
    saveLocked();
    return {{"playerId", playerId}, {"displayName", cleanName}, {"guest", true}};
}

void SurvivalServerCore::validateResult(const nlohmann::json& payload) {
    if (!payload.is_object()) {
        throw ValidationError("request body must be an object");
    }
    const std::string character = stringField(payload, "characterId");
    if (character != "knight" && character != "magic_caster") {
        throw ValidationError("invalid characterId");
    }
    if (trim(stringField(payload, "configVersion")).empty()) {
        throw ValidationError("configVersion required");
    }

    const long long wave = integerField(payload, "highestWave");
    const long long score = integerField(payload, "score");
    const long long survivalMs = integerField(payload, "survivalMs");
    const long long kills = integerField(payload, "kills");
    const long long bosses = integerField(payload, "bossesKilled");
    const long long damage = integerField(payload, "damageTaken");
    if (wave < 1 || wave > 50) throw ValidationError("highestWave outside 1..50");
    if (score < 0 || score > 100000000) throw ValidationError("score outside accepted range");
    if (survivalMs < 0 || survivalMs > 8LL * 60 * 60 * 1000) throw ValidationError("survivalMs outside accepted range");
    if (kills < 0 || kills > 100000) throw ValidationError("kills outside accepted range");
    if (bosses < 0 || bosses > 5 || bosses > wave / 10) throw ValidationError("bossesKilled inconsistent with wave");
    if (damage < 0 || damage > 10000000) throw ValidationError("damageTaken outside accepted range");

    const auto victory = payload.find("victory");
    if (victory != payload.end() && !victory->is_boolean()) {
        throw ValidationError("victory must be a boolean");
    }
    if (victory != payload.end() && victory->get<bool>() && wave != 50) {
        throw ValidationError("victory requires wave 50");
    }
}

long long SurvivalServerCore::recomputeScore(const nlohmann::json& payload) {
    return integerField(payload, "kills") * 10
        + integerField(payload, "bossesKilled") * 10000
        + integerField(payload, "highestWave") * 500;
}

nlohmann::json SurvivalServerCore::runResponse(const nlohmann::json& run, bool duplicate) {
    return {
        {"runId", run["id"]},
        {"validationStatus", run["validationStatus"]},
        {"score", run["score"]},
        {"highestWave", run["highestWave"]},
        {"duplicate", duplicate}
    };
}

nlohmann::json SurvivalServerCore::submitResult(
    const std::string& playerId,
    const std::string& runId,
    const std::string& idempotencyKey,
    const nlohmann::json& payload) {
    validateResult(payload);
    if (playerId.empty()) throw ValidationError("X-Player-Id required");
    if (runId.empty()) throw ValidationError("runId required");
    if (idempotencyKey.empty()) throw ValidationError("Idempotency-Key required");

    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& run : data_["runs"]) {
        if (run.value("playerId", "") == playerId
            && run.value("idempotencyKey", "") == idempotencyKey) {
            return runResponse(run, true);
        }
    }
    if (!playerExistsLocked(playerId)) {
        throw ValidationError("unknown player");
    }
    for (const auto& run : data_["runs"]) {
        if (run.value("id", "") == runId) {
            throw ValidationError("runId already exists");
        }
    }

    nlohmann::json run = {
        {"id", runId},
        {"playerId", playerId},
        {"idempotencyKey", idempotencyKey},
        {"characterId", payload["characterId"]},
        {"configVersion", payload["configVersion"]},
        {"highestWave", integerField(payload, "highestWave")},
        {"score", recomputeScore(payload)},
        {"clientScore", integerField(payload, "score")},
        {"survivalMs", integerField(payload, "survivalMs")},
        {"kills", integerField(payload, "kills")},
        {"bossesKilled", integerField(payload, "bossesKilled")},
        {"damageTaken", integerField(payload, "damageTaken")},
        {"victory", payload.value("victory", false)},
        {"validationStatus", "validated"},
        {"payload", payload},
        {"completedAt", nowSeconds()}
    };
    data_["runs"].push_back(run);
    saveLocked();
    return runResponse(run, false);
}

std::optional<nlohmann::json> SurvivalServerCore::profile(const std::string& playerId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto player = std::find_if(data_["players"].begin(), data_["players"].end(),
        [&playerId](const nlohmann::json& candidate) {
            return candidate.value("id", "") == playerId;
        });
    if (player == data_["players"].end()) {
        return std::nullopt;
    }

    long long runs = 0;
    long long highestWave = 0;
    long long bestScore = 0;
    long long lifetimeKills = 0;
    long long bossesKilled = 0;
    for (const auto& run : data_["runs"]) {
        if (run.value("playerId", "") != playerId) continue;
        ++runs;
        highestWave = std::max(highestWave, run.value("highestWave", 0LL));
        bestScore = std::max(bestScore, run.value("score", 0LL));
        lifetimeKills += run.value("kills", 0LL);
        bossesKilled += run.value("bossesKilled", 0LL);
    }
    return nlohmann::json{
        {"playerId", playerId},
        {"displayName", player->value("displayName", "Player")},
        {"progress", {
            {"runs", runs},
            {"highestWave", highestWave},
            {"bestScore", bestScore},
            {"lifetimeKills", lifetimeKills},
            {"bossesKilled", bossesKilled}
        }}
    };
}

nlohmann::json SurvivalServerCore::leaderboard(
    const std::optional<std::string>& characterId,
    int limit) const {
    std::lock_guard<std::mutex> lock(mutex_);
    limit = std::clamp(limit, 1, 100);
    std::vector<nlohmann::json> bestRuns;
    for (const auto& run : data_["runs"]) {
        if (run.value("validationStatus", "") != "validated") continue;
        if (characterId && run.value("characterId", "") != *characterId) continue;
        const std::string playerId = run.value("playerId", "");
        auto current = std::find_if(bestRuns.begin(), bestRuns.end(),
            [&playerId](const nlohmann::json& candidate) {
                return candidate.value("playerId", "") == playerId;
            });
        if (current == bestRuns.end()) {
            bestRuns.push_back(run);
        } else if (runIsBetter(run, *current)) {
            *current = run;
        }
    }
    std::sort(bestRuns.begin(), bestRuns.end(), runIsBetter);

    nlohmann::json entries = nlohmann::json::array();
    const std::size_t count = std::min(bestRuns.size(), static_cast<std::size_t>(limit));
    for (std::size_t index = 0; index < count; ++index) {
        const auto& run = bestRuns[index];
        std::string displayName = "Player";
        for (const auto& player : data_["players"]) {
            if (player.value("id", "") == run.value("playerId", "")) {
                displayName = player.value("displayName", "Player");
                break;
            }
        }
        entries.push_back({
            {"rank", index + 1},
            {"player", displayName},
            {"score", run["score"]},
            {"wave", run["highestWave"]},
            {"timeMs", run["survivalMs"]},
            {"characterId", run["characterId"]}
        });
    }
    return entries;
}

std::size_t SurvivalServerCore::runCountForTests() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_["runs"].size();
}

} // namespace SurvivalBackend
