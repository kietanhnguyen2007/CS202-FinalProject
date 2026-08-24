#include "SurvivalServerCore.h"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using SurvivalBackend::SurvivalServerCore;
using SurvivalBackend::ValidationError;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

nlohmann::json result(int wave = 10, int kills = 40) {
    return {
        {"characterId", "knight"}, {"configVersion", "m6-cpp-1"},
        {"highestWave", wave}, {"score", 1000}, {"survivalMs", 60000},
        {"kills", kills}, {"bossesKilled", wave / 10}, {"damageTaken", 25},
        {"victory", wave == 50}
    };
}

void testIdempotency(const std::filesystem::path& dataPath) {
    SurvivalServerCore core(dataPath);
    const auto player = core.createGuest("Aegis");
    const auto first = core.submitResult(player["playerId"], "run-1", "key-1", result());
    const auto second = core.submitResult(player["playerId"], "run-1", "key-1", result());
    require(!first["duplicate"].get<bool>(), "first submission marked duplicate");
    require(second["duplicate"].get<bool>(), "second submission not marked duplicate");
    require(core.runCountForTests() == 1, "idempotency created more than one run");
    const auto profile = core.profile(player["playerId"]);
    require(profile && (*profile)["progress"]["runs"] == 1, "profile run count incorrect");
}

void testImpossibleVictory(const std::filesystem::path& dataPath) {
    SurvivalServerCore core(dataPath);
    const auto player = core.createGuest("Validator");
    auto payload = result(20);
    payload["victory"] = true;
    bool rejected = false;
    try {
        core.submitResult(player["playerId"], "run-bad", "key-bad", payload);
    } catch (const ValidationError&) {
        rejected = true;
    }
    require(rejected, "impossible victory was accepted");
}

void testLeaderboardBestRun(const std::filesystem::path& dataPath) {
    SurvivalServerCore core(dataPath);
    const auto player = core.createGuest("Ranker");
    core.submitResult(player["playerId"], "run-low", "key-low", result(10, 40));
    core.submitResult(player["playerId"], "run-high", "key-high", result(20, 100));
    const auto board = core.leaderboard();
    require(board.size() == 1, "leaderboard contains multiple runs for one player");
    require(board[0]["wave"] == 20, "leaderboard did not keep the best run");
}

} // namespace

int main() {
    const auto unique = std::to_string(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());
    const std::filesystem::path testDirectory =
        std::filesystem::temp_directory_path() / ("aegis-rift-server-tests-" + unique);
    std::filesystem::create_directories(testDirectory);
    try {
        testIdempotency(testDirectory / "idempotency.json");
        std::cout << "[PASS] idempotent run submission\n";
        testImpossibleVictory(testDirectory / "validation.json");
        std::cout << "[PASS] impossible victory validation\n";
        testLeaderboardBestRun(testDirectory / "leaderboard.json");
        std::cout << "[PASS] best run per player leaderboard\n";
        std::filesystem::remove_all(testDirectory);
        std::cout << "All C++ server tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        std::error_code cleanupError;
        std::filesystem::remove_all(testDirectory, cleanupError);
        return 1;
    }
}
