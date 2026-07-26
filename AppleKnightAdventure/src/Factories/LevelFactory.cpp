#include "Factories/LevelFactory.h"
#include "Factories/EnemyFactory.h"
#include "Factories/ItemFactory.h"
#include "Model/Boss.h"
#include "Model/Chest.h"
#include "Model/Checkpoint.h"
#include "Model/FakeWall.h"
#include "Model/DualWorldPlayer.h"
#include "Utils/Constants.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <unordered_map>

// ── anonymous helpers ─────────────────────────────────────────────────────────
namespace {
// Read an int field from a LDtk entityInstance fieldInstances array.
int GetEntityFieldInt(const nlohmann::json& ei,
                      const std::string& fieldName, int defaultVal = 0) {
    if (!ei.contains("fieldInstances")) return defaultVal;
    for (auto& f : ei["fieldInstances"]) {
        if (f["__identifier"] == fieldName && !f["__value"].is_null())
            return f["__value"].get<int>();
    }
    return defaultVal;
}
} // namespace

CharacterClass LevelFactory::ParsePlayerClass(const std::string& name) {
    if (name == "fighter"  || name == "Fighter")  return CharacterClass::Fighter;
    if (name == "ninja"    || name == "Ninja")     return CharacterClass::Ninja;
    if (name == "magic_caster" || name == "magiccaster" ||
        name == "MagicCaster")                     return CharacterClass::MagicCaster;
    return CharacterClass::Knight;
}

BackgroundTheme LevelFactory::ParseBackgroundTheme(const std::string& name) {
    if (name == "ColdCorridor") return BackgroundTheme::ColdCorridor;
    if (name == "Underwater")   return BackgroundTheme::Underwater;
    return BackgroundTheme::Forest;
}

void LevelFactory::BuildTileTypeMap(const std::string& ldtkJson,
                                    std::unordered_map<int,int>& out) {
    // Parse just the 'defs' section to map tilesetUid -> tileType (1-based import order).
    // Import order in LDtk must match: Tiles, Buildings, Hive, Interior-01, Props-Rocks, Tree-Assets.
    using json = nlohmann::json;
    std::ifstream f(ldtkJson);
    if (!f.is_open()) return;
    json root;
    try { f >> root; } catch (...) { return; }
    if (!root.contains("defs") || !root["defs"].contains("tilesets")) return;
    // Bug 3 fix: guard against null uid entries in the tileset defs array.
    // Bug 4 fix: map UID directly as tileType (UID 1=Tiles.png…6=Tree-Assets.png)
    //            rather than by positional index, so thumbnail tilesets (uid 7+)
    //            don't corrupt the mapping.
    for (auto& ts : root["defs"]["tilesets"]) {
        if (!ts.contains("uid") || ts["uid"].is_null()) continue;
        int uid = ts["uid"].get<int>();
        if (uid >= 1 && uid <= 6)
            out[uid] = uid; // UID 1-6 map directly to GameView tileType 1-6
    }
}

std::unique_ptr<GameState> LevelFactory::LoadLevel(const std::string& filepath,
                                                   GameMode mode,
                                                   int ldtkLevelIndex) {
    // Auto-detect LDtk format by extension
    if (filepath.size() >= 5 &&
        filepath.substr(filepath.size() - 5) == ".ldtk") {
        return LoadLDtkLevel(filepath, ldtkLevelIndex, mode);
    }
    // --- Legacy .lvl text format ---
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

// ── LDtk Loaders ─────────────────────────────────────────────────────────────

std::unique_ptr<GameState> LevelFactory::LoadLDtkLevel(const std::string& filepath,
                                                       int levelIndex,
                                                       GameMode mode) {
    using json = nlohmann::json;
    auto state = std::make_unique<GameState>(mode);

    std::ifstream file(filepath);
    if (!file.is_open()) return CreateDefaultLevel(1);
    json root;
    try { file >> root; } catch (...) { return CreateDefaultLevel(1); }

    auto& levels = root["levels"];
    if (levelIndex < 0 || levelIndex >= (int)levels.size())
        return CreateDefaultLevel(1);
    auto& lvl = levels[levelIndex];

    // --- Map size ---
    int pxWid = lvl.value("pxWid", 2560);
    int pxHei = lvl.value("pxHei", 1152);
    state->SetMapSize(pxWid / TILE_SIZE, pxHei / TILE_SIZE);

    // --- TilesetUid → tileType map ---
    // Bug 3 & 4 fix: guard null uid entries and map UID directly as tileType.
    // GameView is pre-loaded with LoadTileset(N, ...) where N == LDtk UID for
    // the six gameplay tilesets (UID 1=Tiles.png … UID 6=Tree-Assets.png).
    // Thumbnail/preview tilesets (uid 7+) are ignored.
    std::unordered_map<int,int> uidToTileType;
    if (root.contains("defs") && root["defs"].contains("tilesets")) {
        for (auto& ts : root["defs"]["tilesets"]) {
            if (!ts.contains("uid") || ts["uid"].is_null()) continue;
            int uid = ts["uid"].get<int>();
            if (uid >= 1 && uid <= 6)
                uidToTileType[uid] = uid;
        }
    }

    // --- Level fieldInstances ---
    int totalItems = 0, totalEnemies = 0;
    if (lvl.contains("fieldInstances")) {
        for (auto& fi : lvl["fieldInstances"]) {
            std::string fid = fi["__identifier"];
            if      (fid == "BackgroundTheme" && !fi["__value"].is_null())
                state->SetBackgroundTheme(ParseBackgroundTheme(fi["__value"].get<std::string>()));
            else if (fid == "PlayerClass" && !fi["__value"].is_null())
                state->SetPlayerClass(ParsePlayerClass(fi["__value"].get<std::string>()));
            else if (fid == "TotalItems"   && !fi["__value"].is_null())
                totalItems   = fi["__value"].get<int>();
            else if (fid == "TotalEnemies" && !fi["__value"].is_null())
                totalEnemies = fi["__value"].get<int>();
        }
    }

    if (!lvl.contains("layerInstances")) {
        state->SetTotalItems(totalItems);
        state->SetTotalEnemies(totalEnemies);
        return CreateDefaultLevel(1);
    }

    // --- Pass 1: IntGrid Collision → solidMap ---
    std::unordered_map<int,int> solidMap; // cell linear index → intgrid value
    for (auto& layer : lvl["layerInstances"]) {
        if (layer["__type"] != "IntGrid" || layer["__identifier"] != "Collision") continue;
        int gs = layer.value("__gridSize", TILE_SIZE);
        int cols = std::max(1, pxWid / gs);
        if (!layer.contains("intGridCsv")) continue;
        auto& csv = layer["intGridCsv"];
        for (int i = 0; i < (int)csv.size(); ++i) {
            // Bug 2 fix: LDtk can emit null for empty intgrid cells.
            if (csv[i].is_null()) continue;
            int v = csv[i].get<int>();
            if (v != 0) solidMap[i] = v;
        }
    }

    // --- Pass 2: Tile layers + Entities ---
    bool hasLocalSpawn = false;
    int autoItems = 0, autoEnemies = 0;

    for (auto& layer : lvl["layerInstances"]) {
        std::string ltype = layer["__type"];
        std::string lid   = layer["__identifier"];
        int gs            = layer.value("__gridSize", TILE_SIZE);
        // Bug 1 fix: __tilesetDefUid is JSON null on Entities/IntGrid layers.
        // nlohmann::json::value() throws type_error.302 when the key exists but
        // is null — it only falls back for absent keys. Use explicit null-check.
        int tilesetUid = -1;
        if (layer.contains("__tilesetDefUid") && !layer["__tilesetDefUid"].is_null())
            tilesetUid = layer["__tilesetDefUid"].get<int>();
        int tileType = (uidToTileType.count(tilesetUid)) ? uidToTileType[tilesetUid] : 1;
        int cols     = std::max(1, pxWid / gs);

        // ── Tile layers ──────────────────────────────────────────
        if (ltype == "Tiles" && (lid == "Tiles" || lid == "BG_Tiles")) {
            bool isBG = (lid == "BG_Tiles");
            if (!layer.contains("gridTiles")) continue;
            for (auto& gt : layer["gridTiles"]) {
                int gx = gt["px"][0].get<int>() / gs;
                int gy = gt["px"][1].get<int>() / gs;
                int t  = gt["t"].get<int>();
                int f  = gt.value("f", 0);
                int cellIdx = gy * cols + gx;
                bool solid = !isBG && (solidMap.count(cellIdx) > 0);
                state->AddTile(Tile{gx, gy, tileType, t, solid, f});
            }
            continue;
        }

        // ── Entity layer ─────────────────────────────────────────
        if (ltype != "Entities" || lid != "Entities") continue;
        if (!layer.contains("entityInstances")) continue;

        for (auto& ei : layer["entityInstances"]) {
            std::string eid = ei["__identifier"];
            float wx = ei["px"][0].get<float>();
            float wy = ei["px"][1].get<float>();
            Vector2 pos{wx, wy};

            // ── Spawn points ──────────────────────────────────────
            if (eid == "SpawnSolo" && mode == GameMode::SinglePlayer) {
                state->SetLocalPlayer(std::make_unique<Player>(pos));
                hasLocalSpawn = true;
            } else if (eid == "SpawnGuide" && mode == GameMode::MultiplayerHost) {
                state->SetLocalPlayer(std::make_unique<Player>(pos));
                hasLocalSpawn = true;
            } else if (eid == "SpawnWarrior" && mode == GameMode::MultiplayerClient) {
                state->SetLocalPlayer(std::make_unique<Player>(pos));
                hasLocalSpawn = true;
            } else if (eid == "SpawnDualLight" && mode == GameMode::SinglePlayer) {
                auto p = std::make_unique<DualWorldPlayer>(pos, WorldLayer::Light);
                state->SetLocalPlayer(std::move(p));
                hasLocalSpawn = true;
            } else if (eid == "SpawnDualShadow" && mode == GameMode::SinglePlayer) {
                auto p = std::make_unique<DualWorldPlayer>(pos, WorldLayer::Shadow);
                state->SetLocalPlayer(std::move(p));
                hasLocalSpawn = true;

            // ── Enemies ───────────────────────────────────────────
            } else if (eid == "EnemyMelee") {
                state->AddEntity(EnemyFactory::CreateMelee(pos));
                ++autoEnemies;
            } else if (eid == "EnemyRanged") {
                state->AddEntity(EnemyFactory::CreateRanged(pos));
                ++autoEnemies;
            } else if (eid == "EnemyFlying") {
                state->AddEntity(EnemyFactory::CreateFlying(pos));
                ++autoEnemies;

            // ── Bosses (size distinguishes Boss1/2 from Boss3) ────
            } else if (eid == "Boss1") {
                auto boss = std::make_unique<Boss>(pos, Vector2{96.0f, 96.0f});
                float patrol = (float)GetEntityFieldInt(ei, "PatrolRight", (int)wx + 400);
                boss->SetDetectionRange(patrol - wx);
                state->AddEntity(std::move(boss));
                ++autoEnemies;
            } else if (eid == "Boss2") {
                auto boss = std::make_unique<Boss>(pos, Vector2{96.0f, 96.0f});
                float patrol = (float)GetEntityFieldInt(ei, "PatrolRight", (int)wx + 400);
                boss->SetDetectionRange(patrol - wx);
                state->AddEntity(std::move(boss));
                ++autoEnemies;
            } else if (eid == "Boss3") {
                auto boss = std::make_unique<Boss>(pos, Vector2{128.0f, 128.0f});
                float patrol = (float)GetEntityFieldInt(ei, "PatrolRight", (int)wx + 500);
                boss->SetDetectionRange(patrol - wx);
                state->AddEntity(std::move(boss));
                ++autoEnemies;

            // ── Interactables ─────────────────────────────────────
            } else if (eid == "Chest") {
                state->AddEntity(std::make_unique<Chest>(pos));
                ++autoItems;
            } else if (eid == "CheckpointMid") {
                auto cp = std::make_unique<Checkpoint>(pos);
                cp->SetEndGame(false);
                state->AddEntity(std::move(cp));
            } else if (eid == "CheckpointEnd") {
                auto cp = std::make_unique<Checkpoint>(pos);
                cp->SetEndGame(true);
                state->AddEntity(std::move(cp));
            } else if (eid == "FakeWall") {
                state->AddEntity(std::make_unique<FakeWall>(
                    pos, Vector2{(float)TILE_SIZE, (float)TILE_SIZE}));

            // ── Fixed Items ───────────────────────────────────────
            } else if (eid == "ItemCoin") {
                int amt = GetEntityFieldInt(ei, "Amount", 1);
                state->AddEntity(ItemFactory::CreateCoin(pos, amt));
                ++autoItems;
            } else if (eid == "ItemApple") {
                state->AddEntity(ItemFactory::CreateApple(pos));
                ++autoItems;
            } else if (eid == "ItemKey") {
                state->AddEntity(ItemFactory::CreateKey(pos));
                ++autoItems;
            } else if (eid == "ItemPotion") {
                state->AddEntity(ItemFactory::CreatePotion(pos));
                ++autoItems;
            } else if (eid == "ItemEquipment") {
                state->AddEntity(ItemFactory::CreateEquipment(pos));
                ++autoItems;
            }
        }
    }

    // Use field values if explicitly set, otherwise use auto-counted values
    state->SetTotalItems(totalItems > 0 ? totalItems : autoItems);
    state->SetTotalEnemies(totalEnemies > 0 ? totalEnemies : autoEnemies);

    // Bug 5 fix: warn loudly instead of silently discarding all LDtk tile data.
    // The caller will still get a playable level, but the missing spawn entity in
    // LDtk must be fixed (place a SpawnSolo entity on the Entities layer).
    if (!hasLocalSpawn) {
        TraceLog(LOG_WARNING,
            "LDtk: Level %d has no SpawnSolo/SpawnGuide/SpawnWarrior entity. "
            "Falling back to default level. Add a spawn entity in the LDtk editor.",
            levelIndex);
        return CreateDefaultLevel(1);
    }
    return state;
}

std::unique_ptr<DualWorld> LevelFactory::LoadLDtkDualWorld(const std::string& filepath,
                                                           int levelIndex) {
    using json = nlohmann::json;
    std::ifstream file(filepath);
    if (!file.is_open()) return std::make_unique<DualWorld>();
    json root;
    try { file >> root; } catch (...) { return std::make_unique<DualWorld>(); }

    auto& levels = root["levels"];
    if (levelIndex < 0 || levelIndex >= (int)levels.size())
        return std::make_unique<DualWorld>();
    auto& lvl = levels[levelIndex];

    int pxWid = lvl.value("pxWid", 2560);
    int pxHei = lvl.value("pxHei", 1152);
    auto world = std::make_unique<DualWorld>(pxWid / TILE_SIZE, pxHei / TILE_SIZE);

    if (!lvl.contains("layerInstances")) return world;

    for (auto& layer : lvl["layerInstances"]) {
        std::string lid = layer["__identifier"];
        if (lid != "LightTiles" && lid != "ShadowTiles") continue;
        WorldLayer wl = (lid == "LightTiles") ? WorldLayer::Light : WorldLayer::Shadow;
        int gs = layer.value("__gridSize", TILE_SIZE);
        if (!layer.contains("gridTiles")) continue;
        for (auto& gt : layer["gridTiles"]) {
            Tile tile{};
            tile.x         = gt["px"][0].get<int>() / gs;
            tile.y         = gt["px"][1].get<int>() / gs;
            tile.tileId    = gt["t"].get<int>();
            tile.tileType  = 1;
            tile.solid     = true;
            tile.flipFlags = gt.value("f", 0);
            world->AddTile(wl, tile);
        }
    }
    return world;
}
