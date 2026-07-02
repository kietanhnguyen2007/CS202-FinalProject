#include "Model/Checkpoint.h"
#include "Utils/Constants.h"

Checkpoint::Checkpoint()
    : Entity(EntityType::Checkpoint)
    , m_activated(false)
    , m_isEndGame(false)
{
}

Checkpoint::Checkpoint(Vector2 position) 
    : Entity(position, {TILE_SIZE * 0.6f, TILE_SIZE * 0.8f}, EntityType::Checkpoint)
    , m_activated(false)
    , m_isEndGame(false)
{
}

void Checkpoint::Update(float deltaTime) {
}


bool Checkpoint::IsActivated() const { return m_activated; }
void Checkpoint::Activate() { m_activated = true; }
void Checkpoint::Deactivate() { m_activated = false; }

bool Checkpoint::IsEndGame() const { return m_isEndGame; }
void Checkpoint::SetEndGame(bool isEndGame) { m_isEndGame = isEndGame; }
