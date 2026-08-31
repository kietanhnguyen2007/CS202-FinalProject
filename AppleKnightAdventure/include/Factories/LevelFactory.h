#ifndef LEVELFACTORY_H
#define LEVELFACTORY_H

#include "Model/GameState.h"
#include "Model/DualWorld.h"
#include "Utils/Types.h"
#include <string>
#include <memory>
#include <unordered_map>

class LegacyLevelAdapter;
class LDtkLevelAdapter;

class LevelFactory {
public:
    // Auto-detect .lvl vs .ldtk by file extension
    static std::unique_ptr<GameState> LoadLevel(const std::string& filepath,
                                               GameMode mode = GameMode::SinglePlayer,
                                               int ldtkLevelIndex = 0,
                                               CharacterClass cls = CharacterClass::Knight);

    static bool SaveLevel(const std::string& filepath, GameState* state);
    static std::unique_ptr<GameState> CreateDefaultLevel(int levelNumber);

    static std::unique_ptr<DualWorld> LoadDualWorld(const std::string& filepath);
    static bool SaveDualWorld(const std::string& filepath, DualWorld* world);

    // LDtk-specific (có thể gọi trực tiếp)
    static std::unique_ptr<GameState> LoadLDtkLevel(const std::string& filepath,
                                                    int levelIndex = 0,
                                                    GameMode mode = GameMode::SinglePlayer,
                                                    CharacterClass cls = CharacterClass::Knight);
    static std::unique_ptr<DualWorld> LoadLDtkDualWorld(const std::string& filepath,
                                                        int levelIndex = 0);

private:
    friend class LegacyLevelAdapter;
    friend class LDtkLevelAdapter;

    static CharacterClass  ParsePlayerClass(const std::string& name);
    static BackgroundTheme ParseBackgroundTheme(const std::string& name);
    static void            BuildTileTypeMap(const std::string& ldtkJson,
                                           std::unordered_map<int,int>& out);
};

#endif
