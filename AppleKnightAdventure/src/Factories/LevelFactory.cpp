#include "Factories/LevelFactory.h"
#include "Factories/EnemyFactory.h"
#include "Model/Chest.h"
#include "Model/Checkpoint.h"
#include "Utils/Constants.h"
#include <fstream>
#include <sstream>

CharacterClass LevelFactory::ParsePlayerClass(const std::string& name) {
    if (name == "fighter") return CharacterClass::Fighter;
    if (name == "ninja") return CharacterClass::Ninja;
    if (name == "magic_caster" || name == "magiccaster") return CharacterClass::MagicCaster;
    return CharacterClass::Knight;
}

std::unique_ptr<GameState> LevelFactory::LoadLevel(const std::string& filepath, GameMode mode) {
    auto state = std::make_unique<GameState>(mode);
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return CreateDefaultLevel(1);
    }

    bool hasLocalSpawn = false;
    std::string token;
    while (file >> token) {
        if (token == "maptype") {
            int mapType = 0;
            file >> mapType;
            (void)mapType;
        } else if (token == "width") {
            int width = 0;
            file >> width;
            state->SetMapSize(width, state->GetMapHeight());
        } else if (token == "height") {
            int height = 0;
            file >> height;
            state->SetMapSize(state->GetMapWidth(), height);
        } else if (token == "player_class") {
            std::string className;
            file >> className;
            state->SetPlayerClass(ParsePlayerClass(className));
        } else if (token == "tile") {
            Tile tile{};
            int solid = 1;
            file >> tile.x >> tile.y >> tile.tileType >> tile.tileId >> solid;
            tile.solid = (solid != 0);
            state->AddTile(tile);
        } else if (token == "spawn_solo") {
            float tx = 0.0f;
            float ty = 0.0f;
            file >> tx >> ty;
            if (mode == GameMode::SinglePlayer) {
                auto player = std::make_unique<Player>(Vector2{tx * TILE_SIZE, ty * TILE_SIZE});
                state->SetLocalPlayer(std::move(player));
                hasLocalSpawn = true;
            }
        } else if (token == "spawn_guide") {
            float tx = 0.0f;
            float ty = 0.0f;
            file >> tx >> ty;
            if (mode == GameMode::MultiplayerHost) {
                auto player = std::make_unique<Player>(Vector2{tx * TILE_SIZE, ty * TILE_SIZE});
                state->SetLocalPlayer(std::move(player));
                hasLocalSpawn = true;
            }
        } else if (token == "spawn_warrior") {
            float tx = 0.0f;
            float ty = 0.0f;
            file >> tx >> ty;
            if (mode == GameMode::MultiplayerClient) {
                auto player = std::make_unique<Player>(Vector2{tx * TILE_SIZE, ty * TILE_SIZE});
                state->SetLocalPlayer(std::move(player));
                hasLocalSpawn = true;
            }
        } else if (token == "enemy") {
            std::string type;
            float tx = 0.0f;
            float ty = 0.0f;
            file >> type >> tx >> ty;
            EnemyType enemyType = EnemyType::Melee;
            if (type == "ranged") {
                enemyType = EnemyType::Ranged;
            } else if (type == "flying") {
                enemyType = EnemyType::Flying;
            }
            state->AddEntity(EnemyFactory::CreateEnemy({tx * TILE_SIZE, ty * TILE_SIZE}, enemyType));
        } else if (token == "chest") {
            float tx = 0.0f;
            float ty = 0.0f;
            file >> tx >> ty;
            state->AddEntity(std::make_unique<Chest>(Vector2{tx * TILE_SIZE, ty * TILE_SIZE}));
        } else if (token == "checkpoint") {
            std::string cpType;
            float tx = 0.0f;
            float ty = 0.0f;
            file >> cpType >> tx >> ty;
            auto cp = std::make_unique<Checkpoint>(Vector2{tx * TILE_SIZE, ty * TILE_SIZE});
            if (cpType == "end") {
                cp->SetEndGame(true);
            }
            state->AddEntity(std::move(cp));
        } else if (token == "scoring") {
            int items = 0;
            int enemies = 0;
            file >> items >> enemies;
            state->SetTotalItems(items);
            state->SetTotalEnemies(enemies);
        }
    }

    if (!hasLocalSpawn) {
        return CreateDefaultLevel(1);
    }

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
    state->SetPlayerClass(CharacterClass::Knight);
    state->SetMapSize(35, 15);

    for (int x = 0; x < 35; ++x) {
        state->AddTile(Tile{x, 14, 1, 25, true});
    }

    auto player = std::make_unique<Player>(Vector2{4.0f * TILE_SIZE, 13.0f * TILE_SIZE});
    state->SetLocalPlayer(std::move(player));
    state->AddEntity(EnemyFactory::CreateMelee({20.0f * TILE_SIZE, 13.0f * TILE_SIZE}));
    state->AddEntity(EnemyFactory::CreateMelee({25.0f * TILE_SIZE, 13.0f * TILE_SIZE}));
    state->AddEntity(std::make_unique<Chest>(Vector2{10.0f * TILE_SIZE, 13.0f * TILE_SIZE}));
    state->AddEntity(std::make_unique<Checkpoint>(Vector2{5.0f * TILE_SIZE, 13.0f * TILE_SIZE}));
    state->SetTotalItems(5);
    state->SetTotalEnemies(2);
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
