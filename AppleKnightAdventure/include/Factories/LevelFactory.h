#ifndef LEVELFACTORY_H
#define LEVELFACTORY_H

#include "Model/GameState.h"
#include "Model/DualWorld.h"
#include <string>
#include <memory>

class LevelFactory {
public:
    static std::unique_ptr<GameState> LoadLevel(const std::string& filepath);
    static bool SaveLevel(const std::string& filepath, GameState* state);

    static std::unique_ptr<GameState> CreateDefaultLevel(int levelNumber);
    static std::unique_ptr<DualWorld> LoadDualWorld(const std::string& filepath);
    static bool SaveDualWorld(const std::string& filepath, DualWorld* world);
};

#endif
