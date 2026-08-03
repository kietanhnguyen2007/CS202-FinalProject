#include "Model/Command.h"

// --- PlaceTileCommand ---
PlaceTileCommand::PlaceTileCommand(MapLayer layer, const Tile& newTile, bool hadPrev, const Tile& prevTile)
    : m_layer(layer), m_newTile(newTile), m_hadPreviousTile(hadPrev), m_previousTile(prevTile) {}

void PlaceTileCommand::Execute(GameState* state) {
    if (state) {
        state->SetTileAt(m_layer, m_newTile.x, m_newTile.y, m_newTile.tileType, m_newTile.tileId, m_newTile.solid, m_newTile.flipFlags);
    }
}

void PlaceTileCommand::Undo(GameState* state) {
    if (state) {
        if (m_hadPreviousTile) {
            state->SetTileAt(m_layer, m_previousTile.x, m_previousTile.y, m_previousTile.tileType, m_previousTile.tileId, m_previousTile.solid, m_previousTile.flipFlags);
        } else {
            state->RemoveTileAt(m_layer, m_newTile.x, m_newTile.y);
        }
    }
}

// --- EraseTileCommand ---
EraseTileCommand::EraseTileCommand(MapLayer layer, int x, int y, bool hadPrev, const Tile& prevTile)
    : m_layer(layer), m_x(x), m_y(y), m_hadPreviousTile(hadPrev), m_previousTile(prevTile) {}

void EraseTileCommand::Execute(GameState* state) {
    if (state) {
        state->RemoveTileAt(m_layer, m_x, m_y);
    }
}

void EraseTileCommand::Undo(GameState* state) {
    if (state && m_hadPreviousTile) {
        state->SetTileAt(m_layer, m_previousTile.x, m_previousTile.y, m_previousTile.tileType, m_previousTile.tileId, m_previousTile.solid, m_previousTile.flipFlags);
    }
}

RemoveEntityCommand::RemoveEntityCommand(int entityId) : m_entityId(entityId) {}

void RemoveEntityCommand::Execute(GameState* state) {
    m_entity = state->ExtractEntity(m_entityId);
}

void RemoveEntityCommand::Undo(GameState* state) {
    if (m_entity) {
        state->AddEntity(std::move(m_entity));
    }
}

PlaceEntityCommand::PlaceEntityCommand(std::unique_ptr<Entity> entity)
    : m_entity(std::move(entity)) {
    m_entityId = m_entity->GetId();
}

void PlaceEntityCommand::Execute(GameState* state) {
    if (m_entity) {
        state->AddEntity(std::move(m_entity));
    }
}

void PlaceEntityCommand::Undo(GameState* state) {
    m_entity = state->ExtractEntity(m_entityId);
}

void CompositeCommand::AddCommand(std::unique_ptr<ICommand> command) {
    m_commands.push_back(std::move(command));
}

void CompositeCommand::Execute(GameState* state) {
    for (auto& cmd : m_commands) {
        cmd->Execute(state);
    }
}

void CompositeCommand::Undo(GameState* state) {
    for (auto it = m_commands.rbegin(); it != m_commands.rend(); ++it) {
        (*it)->Undo(state);
    }
}

// --- CommandManager ---
CommandManager::CommandManager(GameState* state) : m_state(state) {}

void CommandManager::ExecuteCommand(std::unique_ptr<ICommand> command) {
    if (command && m_state) {
        command->Execute(m_state);
        m_undoStack.push_back(std::move(command));
        m_redoStack.clear(); // Clear redo stack on new action
    }
}

void CommandManager::Undo() {
    if (!m_undoStack.empty() && m_state) {
        auto cmd = std::move(m_undoStack.back());
        m_undoStack.pop_back();
        cmd->Undo(m_state);
        m_redoStack.push_back(std::move(cmd));
    }
}

void CommandManager::Redo() {
    if (!m_redoStack.empty() && m_state) {
        auto cmd = std::move(m_redoStack.back());
        m_redoStack.pop_back();
        cmd->Execute(m_state);
        m_undoStack.push_back(std::move(cmd));
    }
}

void CommandManager::Clear() {
    m_undoStack.clear();
    m_redoStack.clear();
}
