#pragma once

#include <cstddef>
#include <filesystem>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

namespace SurvivalBackend {

class ValidationError final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class SurvivalServerCore final {
public:
    explicit SurvivalServerCore(std::filesystem::path dataPath);

    nlohmann::json createGuest(const std::string& displayName = "Player");
    nlohmann::json submitResult(
        const std::string& playerId,
        const std::string& runId,
        const std::string& idempotencyKey,
        const nlohmann::json& payload);
    std::optional<nlohmann::json> profile(const std::string& playerId) const;
    nlohmann::json leaderboard(
        const std::optional<std::string>& characterId = std::nullopt,
        int limit = 50) const;

    std::size_t runCountForTests() const;

private:
    static void validateResult(const nlohmann::json& payload);
    static long long recomputeScore(const nlohmann::json& payload);
    static nlohmann::json runResponse(const nlohmann::json& run, bool duplicate);

    void load();
    void saveLocked() const;
    bool playerExistsLocked(const std::string& playerId) const;

    std::filesystem::path dataPath_;
    nlohmann::json data_;
    mutable std::mutex mutex_;
};

} // namespace SurvivalBackend
