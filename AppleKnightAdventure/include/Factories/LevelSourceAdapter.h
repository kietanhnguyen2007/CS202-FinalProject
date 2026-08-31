#ifndef LEVELSOURCEADAPTER_H
#define LEVELSOURCEADAPTER_H

#include "Utils/Types.h"
#include <memory>
#include <string>

class GameState;

struct LevelLoadRequest {
    std::string filepath;
    GameMode mode = GameMode::SinglePlayer;
    int levelIndex = 0;
    CharacterClass playerClass = CharacterClass::Knight;
};

// Target interface used by LevelFactory. Each adapter translates one source
// format into the same GameState representation consumed by gameplay/editor.
class ILevelSourceAdapter {
public:
    virtual ~ILevelSourceAdapter() = default;

    virtual bool CanLoad(const std::string& filepath) const = 0;
    virtual std::unique_ptr<GameState> Load(const LevelLoadRequest& request) const = 0;
    virtual const char* GetFormatName() const = 0;
};

class LegacyLevelAdapter final : public ILevelSourceAdapter {
public:
    bool CanLoad(const std::string& filepath) const override;
    std::unique_ptr<GameState> Load(const LevelLoadRequest& request) const override;
    const char* GetFormatName() const override { return "Legacy LVL"; }
};

class LDtkLevelAdapter final : public ILevelSourceAdapter {
public:
    bool CanLoad(const std::string& filepath) const override;
    std::unique_ptr<GameState> Load(const LevelLoadRequest& request) const override;
    const char* GetFormatName() const override { return "LDtk"; }
};

#endif
