#ifndef COMMAND_H
#define COMMAND_H

#include "Model/GameState.h"
#include <vector>
#include <memory>

class ICommand {
public:
    virtual ~ICommand() = default;
    virtual void Execute(GameState* state) = 0;
    virtual void Undo(GameState* state) = 0;
};

class PlaceTileCommand : public ICommand {
    MapLayer m_layer;
    Tile m_newTile;
    bool m_hadPreviousTile;
    Tile m_previousTile;
public:
    PlaceTileCommand(MapLayer layer, const Tile& newTile, bool hadPrev, const Tile& prevTile);
    void Execute(GameState* state) override;
    void Undo(GameState* state) override;
};

class EraseTileCommand : public ICommand {
    MapLayer m_layer;
    int m_x, m_y;
    bool m_hadPreviousTile;
    Tile m_previousTile;
public:
    EraseTileCommand(MapLayer layer, int x, int y, bool hadPrev, const Tile& prevTile);
    void Execute(GameState* state) override;
    void Undo(GameState* state) override;
};

class RemoveEntityCommand : public ICommand {
    std::unique_ptr<Entity> m_entity;
    int m_entityId;
public:
    explicit RemoveEntityCommand(int entityId);
    void Execute(GameState* state) override;
    void Undo(GameState* state) override;
};

class PlaceEntityCommand : public ICommand {
    std::unique_ptr<Entity> m_entity;
    int m_entityId;
public:
    explicit PlaceEntityCommand(std::unique_ptr<Entity> entity);
    void Execute(GameState* state) override;
    void Undo(GameState* state) override;
};

class CompositeCommand : public ICommand {
    std::vector<std::unique_ptr<ICommand>> m_commands;
public:
    void AddCommand(std::unique_ptr<ICommand> command);
    void Execute(GameState* state) override;
    void Undo(GameState* state) override;
};

class CommandManager {
    std::vector<std::unique_ptr<ICommand>> m_undoStack;
    std::vector<std::unique_ptr<ICommand>> m_redoStack;
    GameState* m_state;
public:
    explicit CommandManager(GameState* state);
    void ExecuteCommand(std::unique_ptr<ICommand> command);
    void Undo();
    void Redo();
    void Clear();
};

#endif
