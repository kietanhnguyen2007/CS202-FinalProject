#include "Factories/LevelFactory.h"
#include "Utils/Constants.h"
#include <fstream>
#include <sstream>

std::unique_ptr<GameState> LevelFactory::LoadLevel(const std::string& filepath) {
    auto state = std::make_unique<GameState>();
    std::ifstream file(filepath);
    if (!file.is_open()) return CreateDefaultLevel(1);

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;
    }
    file.close();
    return state;
}

bool LevelFactory::SaveLevel(const std::string& filepath, GameState* state) {
    if (!state) return false;
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "mode " << static_cast<int>(state->GetMode()) << "\n";
    file << "level " << state->GetCurrentLevel() << "\n";

    if (auto* player = state->GetLocalPlayer()) {
        Vector2 pos = player->GetPosition();
        file << "player " << pos.x << " " << pos.y << " "
             << player->GetHealth() << " " << player->GetMaxHealth() << "\n";
    }

    for (const auto& entity : state->GetAllEntities()) {
        Vector2 pos = entity->GetPosition();
        Vector2 size = entity->GetSize();
        file << "entity " << static_cast<int>(entity->GetType()) << " "
             << pos.x << " " << pos.y << " "
             << size.x << " " << size.y << " "
             << entity->IsActive() << "\n";
    }

    file.close();
    return true;
}

std::unique_ptr<GameState> LevelFactory::CreateDefaultLevel(int levelNumber) {
    auto state = std::make_unique<GameState>(GameMode::SinglePlayer);
    state->SetCurrentLevel(levelNumber);
    return state;
}

std::unique_ptr<DualWorld> LevelFactory::LoadDualWorld(const std::string& filepath) {
    auto world = std::make_unique<DualWorld>();
    std::ifstream file(filepath);
    if (!file.is_open()) return world;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "size") {
            int w, h;
            iss >> w >> h;
            world = std::make_unique<DualWorld>(w, h);
        } else if (type == "light_tile" || type == "shadow_tile") {
            Tile tile;
            iss >> tile.x >> tile.y >> tile.tileType;
            WorldLayer layer = (type == "light_tile") ? WorldLayer::Light : WorldLayer::Shadow;
            world->AddTile(layer, tile);
        }
    }
    file.close();
    return world;
}

bool LevelFactory::SaveDualWorld(const std::string& filepath, DualWorld* world) {
    if (!world) return false;
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "size " << world->GetWidth() << " " << world->GetHeight() << "\n";

    for (const auto& tile : world->GetTiles(WorldLayer::Light)) {
        file << "light_tile " << tile.x << " " << tile.y << " " << tile.tileType << "\n";
    }
    for (const auto& tile : world->GetTiles(WorldLayer::Shadow)) {
        file << "shadow_tile " << tile.x << " " << tile.y << " " << tile.tileType << "\n";
    }

    file.close();
    return true;
}
