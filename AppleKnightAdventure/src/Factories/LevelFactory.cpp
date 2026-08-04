#include "Factories/LevelFactory.h"
#include "Factories/EnemyFactory.h"
#include "Factories/ItemFactory.h"
#include "Model/Boss.h"
#include "Model/Chest.h"
#include "Model/Checkpoint.h"
#include "Model/FakeWall.h"
#include "Model/TeleportPortal.h"
#include "Model/DualWorldPlayer.h"
#include "Utils/Constants.h"
#include "Model/TriggerZone.h"
#include "Model/Enemy.h"
#include "Model/Item.h"
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
                                                   int ldtkLevelIndex,
                                                   CharacterClass cls) {
    // Auto-detect LDtk format by extension
    if (filepath.size() >= 5 &&
        filepath.substr(filepath.size() - 5) == ".ldtk") {
        return LoadLDtkLevel(filepath, ldtkLevelIndex, mode, cls);
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
            state->AddTile(MapLayer::Main, tile);
        } else if (token == "tile_layer") {
            int layerIdx = 1;
            Tile tile{};
            int solid = 1;
            file >> layerIdx >> tile.x >> tile.y >> tile.tileType >> tile.tileId >> solid >> tile.flipFlags;
            tile.solid = (solid != 0);
            state->AddTile(static_cast<MapLayer>(layerIdx), tile);
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
        } else if (token == "boss") {
            float tx = 0.0f, ty = 0.0f, sx = 0.0f, sy = 0.0f;
            file >> tx >> ty >> sx >> sy;
            auto boss = std::make_unique<Boss>(Vector2{tx * TILE_SIZE, ty * TILE_SIZE}, Vector2{sx, sy});
            state->AddEntity(std::move(boss));
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
        } else if (token == "item") {
            std::string type;
            float tx = 0.0f, ty = 0.0f;
            int amount = 1;
            file >> type >> tx >> ty >> amount;
            Vector2 pos = {tx * TILE_SIZE, ty * TILE_SIZE};
            if (type == "coin") state->AddEntity(ItemFactory::CreateCoin(pos, amount));
            else if (type == "apple") state->AddEntity(ItemFactory::CreateApple(pos));
            else if (type == "key") state->AddEntity(ItemFactory::CreateKey(pos));
            else if (type == "potion") state->AddEntity(ItemFactory::CreatePotion(pos));
            else if (type == "equipment") state->AddEntity(ItemFactory::CreateEquipment(pos));
        } else if (token == "trigger") {
            float tx = 0.0f, ty = 0.0f;
            std::string target;
            file >> tx >> ty >> target;
            #include "Model/TriggerZone.h"
            auto tz = std::make_unique<TriggerZone>(Vector2{tx * TILE_SIZE, ty * TILE_SIZE}, Vector2{64.0f, 64.0f}, target);
            state->AddEntity(std::move(tz));
        } else if (token == "portal") {
            float tx = 0.0f, ty = 0.0f;
            std::string pType;
            int colorId = 1, targetLevel = -1;
            file >> tx >> ty >> pType >> colorId >> targetLevel;
            PortalType pt = pType == "transition" ? PortalType::LevelTransition : PortalType::Local;
            state->AddEntity(std::make_unique<TeleportPortal>(Vector2{tx * TILE_SIZE, ty * TILE_SIZE}, pt, colorId, targetLevel));
        } else if (token == "fake_wall") {
            float tx = 0.0f, ty = 0.0f;
            file >> tx >> ty;
            state->AddEntity(std::make_unique<FakeWall>(Vector2{tx * TILE_SIZE, ty * TILE_SIZE}, Vector2{64.0f, 64.0f}));
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

    file << "width " << state->GetMapWidth() << "\n";
    file << "height " << state->GetMapHeight() << "\n";
    
    // Convert CharacterClass to string for legacy parser compatibility
    std::string pClass = "Knight";
    switch (state->GetPlayerClass()) {
        case CharacterClass::Fighter: pClass = "fighter"; break;
        case CharacterClass::Ninja: pClass = "ninja"; break;
        case CharacterClass::MagicCaster: pClass = "magic_caster"; break;
        default: pClass = "Knight"; break;
    }
    file << "player_class " << pClass << "\n";

    if (auto* player = state->GetLocalPlayer()) {
        Vector2 pos = player->GetPosition();
        file << "spawn_solo " << (pos.x / TILE_SIZE) << " " << (pos.y / TILE_SIZE) << "\n";
    }

    for (int l = 0; l < static_cast<int>(MapLayer::Count); ++l) {
        for (const auto& tile : state->GetTiles(static_cast<MapLayer>(l))) {
            file << "tile_layer " << l << " " << tile.x << " " << tile.y << " "
                 << tile.tileType << " " << tile.tileId << " " << (tile.solid ? 1 : 0) << " " << tile.flipFlags << "\n";
        }
    }

    for (const auto& entity : state->GetAllEntities()) {
        Vector2 pos = entity->GetPosition();
        float tx = pos.x / TILE_SIZE;
        float ty = pos.y / TILE_SIZE;

        switch (entity->GetType()) {
            case EntityType::Player: {
                file << "spawn_solo " << tx << " " << ty << "\n";
                break;
            }
            case EntityType::Enemy: {
                Enemy* e = static_cast<Enemy*>(entity.get());
                std::string typeStr = "melee";
                if (e->GetEnemyType() == EnemyType::Ranged) typeStr = "ranged";
                else if (e->GetEnemyType() == EnemyType::Flying) typeStr = "flying";
                file << "enemy " << typeStr << " " << tx << " " << ty << "\n";
                break;
            }
            case EntityType::Boss: {
                file << "boss " << tx << " " << ty << " " << entity->GetSize().x << " " << entity->GetSize().y << "\n";
                break;
            }
            case EntityType::Chest: {
                file << "chest " << tx << " " << ty << "\n";
                break;
            }
            case EntityType::Checkpoint: {
                Checkpoint* cp = static_cast<Checkpoint*>(entity.get());
                std::string cpType = cp->IsEndGame() ? "end" : "mid";
                file << "checkpoint " << cpType << " " << tx << " " << ty << "\n";
                break;
            }
            case EntityType::Item: {
                Item* item = static_cast<Item*>(entity.get());
                std::string typeStr = "coin";
                switch(item->GetItemType()) {
                    case ItemType::Apple: typeStr = "apple"; break;
                    case ItemType::Key: typeStr = "key"; break;
                    case ItemType::Potion: typeStr = "potion"; break;
                    case ItemType::Equipment: typeStr = "equipment"; break;
                    default: break;
                }
                file << "item " << typeStr << " " << tx << " " << ty << " " << item->GetAmount() << "\n";
                break;
            }
            case EntityType::TriggerZone: {
                TriggerZone* tz = static_cast<TriggerZone*>(entity.get());
                std::string target = tz->GetTargetLevelId();
                if (target.empty()) target = "none";
                file << "trigger " << tx << " " << ty << " " << target << "\n";
                break;
            }
            case EntityType::FakeWall: {
                file << "fake_wall " << tx << " " << ty << "\n";
                break;
            }
            case EntityType::TeleportPortal: {
                TeleportPortal* portal = static_cast<TeleportPortal*>(entity.get());
                std::string typeStr = portal->GetPortalType() == PortalType::Local ? "local" : "transition";
                file << "portal " << tx << " " << ty << " " << typeStr << " " << portal->GetColorId() << " " << portal->GetTargetLevelId() << "\n";
                break;
            }
            default: break;
        }
    }

    file << "scoring " << state->GetTotalItems() << " " << state->GetTotalEnemies() << "\n";

    file.close();
    return true;
}

std::unique_ptr<GameState> LevelFactory::CreateDefaultLevel(int levelNumber) {
    auto state = std::make_unique<GameState>(GameMode::SinglePlayer);
    state->SetCurrentLevel(levelNumber);
    state->SetPlayerClass(CharacterClass::Knight);
    state->SetMapSize(35, 15);

    for (int x = 0; x < 35; ++x) {
        state->AddTile(MapLayer::Main, Tile{x, 14, 1, 25, true});
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
                                                       GameMode mode,
                                                       CharacterClass cls) {
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
    // Read ldtkGs from first layer so this works with any gridSize (16 or 64)
    int pxWid   = lvl.value("pxWid", 2560);
    int pxHei   = lvl.value("pxHei", 1152);
    int ldtkGs  = 16; // default: tileset native size
    if (lvl.contains("layerInstances") && !lvl["layerInstances"].empty())
        ldtkGs = lvl["layerInstances"][0].value("__gridSize", 16);
    // Map size in game tiles: (map pixels / ldtk grid) gives cell count,
    // which equals game-tile count because 1 LDtk cell = 1 game tile.
    state->SetMapSize(pxWid / ldtkGs, pxHei / ldtkGs);

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
        int gs = layer.value("__gridSize", 16);
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
        int gs            = layer.value("__gridSize", 16);
        // Bug 1 fix: __tilesetDefUid is JSON null on Entities/IntGrid layers.
        // nlohmann::json::value() throws type_error.302 when the key exists but
        // is null — it only falls back for absent keys. Use explicit null-check.
        int tilesetUid = -1;
        if (layer.contains("__tilesetDefUid") && !layer["__tilesetDefUid"].is_null())
            tilesetUid = layer["__tilesetDefUid"].get<int>();
        int tileType = (uidToTileType.count(tilesetUid)) ? uidToTileType[tilesetUid] : 1;
        int cols     = std::max(1, pxWid / gs);

        // ── Tile layers (including AutoLayer from IntGrid) ───────
        if (lid == "Tiles" || lid == "BG_Tiles" || lid == "Collision") {
            bool isBG = (lid == "BG_Tiles");
            
            auto processTiles = [&](const std::string& arrayName) {
                if (!layer.contains(arrayName)) return;
                for (auto& gt : layer[arrayName]) {
                    int gx = gt["px"][0].get<int>() / gs;
                    int gy = gt["px"][1].get<int>() / gs;
                    int t  = gt["t"].get<int>();
                    int f  = gt.value("f", 0);
                    int cellIdx = gy * cols + gx;
                    bool solid = !isBG && (solidMap.count(cellIdx) > 0);
                    state->AddTile(isBG ? MapLayer::Background : MapLayer::Main, Tile{gx, gy, tileType, t, solid, f});
                }
            };
            
            processTiles("gridTiles");
            processTiles("autoLayerTiles");
            
            if (lid != "Collision") continue;
        }

        // ── Entity layer ─────────────────────────────────────────
        if (ltype != "Entities" || lid != "Entities") continue;
        if (!layer.contains("entityInstances")) continue;

        for (auto& ei : layer["entityInstances"]) {
            std::string eid = ei["__identifier"];
            // LDtk px is in ldtk-pixel space (gs=16). Tiles render at gx*TILE_SIZE.
            // Must apply the same scale: gamePos = (ldtkPx / gs) * TILE_SIZE
            const float scale = (float)TILE_SIZE / (float)gs;
            float wx = ei["px"][0].get<float>() * scale;
            float wy = ei["px"][1].get<float>() * scale;
            Vector2 pos{wx, wy};

            // ── Spawn points ──────────────────────────────────────────────────
            if (eid == "SpawnSolo" && mode == GameMode::SinglePlayer) {
                state->SetLocalPlayer(std::make_unique<Player>(pos, cls));
                hasLocalSpawn = true;
            } else if (eid == "SpawnGuide" && mode == GameMode::MultiplayerHost) {
                state->SetLocalPlayer(std::make_unique<Player>(pos, cls));
                hasLocalSpawn = true;
            } else if (eid == "SpawnWarrior" && mode == GameMode::MultiplayerClient) {
                state->SetLocalPlayer(std::make_unique<Player>(pos, cls));
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
            } else if (eid == "Portal") {
                std::string pTypeStr = "Local";
                if (ei.contains("fieldInstances")) {
                    for (auto& f : ei["fieldInstances"]) {
                        if (f["__identifier"] == "PortalType" && !f["__value"].is_null())
                            pTypeStr = f["__value"].get<std::string>();
                    }
                }
                PortalType pType;
                if      (pTypeStr == "LevelTransition") pType = PortalType::LevelTransition;
                else if (pTypeStr == "BossArena")       pType = PortalType::BossArena;
                else                                    pType = PortalType::Local;
                int colorId    = GetEntityFieldInt(ei, "ColorId", 1);
                int targetLevel = GetEntityFieldInt(ei, "TargetLevelId", -1);

                state->AddEntity(std::make_unique<TeleportPortal>(pos, pType, colorId, targetLevel));
            }
        }
    }

    // Link local portals
    std::vector<TeleportPortal*> localPortals;
    for (auto& entity : state->GetAllEntities()) {
        if (entity->GetType() == EntityType::TeleportPortal) {
            auto* portal = static_cast<TeleportPortal*>(entity.get());
            if (portal->GetPortalType() == PortalType::Local) {
                localPortals.push_back(portal);
            }
        }
    }
    
    // Connect portals with the same colorId
    for (size_t i = 0; i < localPortals.size(); ++i) {
        for (size_t j = i + 1; j < localPortals.size(); ++j) {
            if (localPortals[i]->GetColorId() == localPortals[j]->GetColorId()) {
                localPortals[i]->SetLinkedPortal(localPortals[j]);
                localPortals[j]->SetLinkedPortal(localPortals[i]);
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

    int pxWid  = lvl.value("pxWid", 2560);
    int pxHei  = lvl.value("pxHei", 1152);
    int ldtkGs = 16;
    if (lvl.contains("layerInstances") && !lvl["layerInstances"].empty())
        ldtkGs = lvl["layerInstances"][0].value("__gridSize", 16);
    auto world = std::make_unique<DualWorld>(pxWid / ldtkGs, pxHei / ldtkGs);

    if (!lvl.contains("layerInstances")) return world;

    for (auto& layer : lvl["layerInstances"]) {
        std::string lid = layer["__identifier"];
        if (lid != "LightTiles" && lid != "ShadowTiles") continue;
        WorldLayer wl = (lid == "LightTiles") ? WorldLayer::Light : WorldLayer::Shadow;
        int gs = layer.value("__gridSize", 16);
        auto processTiles = [&](const std::string& arrayName) {
            if (!layer.contains(arrayName)) return;
            for (auto& gt : layer[arrayName]) {
                Tile tile{};
                tile.x         = gt["px"][0].get<int>() / gs;
                tile.y         = gt["px"][1].get<int>() / gs;
                tile.tileId    = gt["t"].get<int>();
                tile.tileType  = 1;
                tile.solid     = true;
                tile.flipFlags = gt.value("f", 0);
                world->AddTile(wl, tile);
            }
        };
        processTiles("gridTiles");
        processTiles("autoLayerTiles");
    }
    return world;
}
