#include "Model/LevelCompleteCup.h"
#include "Model/Player.h"
#include "Utils/Constants.h"

LevelCompleteCup::LevelCompleteCup(Vector2 position)
    : Entity(position, {TILE_SIZE * 2.25f, TILE_SIZE * 2.4f}, EntityType::LevelCompleteCup) {
}

void LevelCompleteCup::Update(float deltaTime) {
    (void)deltaTime;
}

bool LevelCompleteCup::CanInteract(const Player* player) const {
    if (!player || m_activated) return false;
    Rectangle range = GetBoundingBox();
    range.x -= TILE_SIZE * 0.75f;
    range.width += TILE_SIZE * 1.5f;
    return CheckCollisionRecs(player->GetBoundingBox(), range);
}

bool LevelCompleteCup::IsActivated() const { return m_activated; }
void LevelCompleteCup::Activate() { m_activated = true; }
