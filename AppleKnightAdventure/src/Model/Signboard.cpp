#include "Model/Signboard.h"
#include "Model/Player.h"
#include "Utils/Constants.h"

Signboard::Signboard(Vector2 position, std::string message)
    : Entity(position, {TILE_SIZE * 1.5f, TILE_SIZE * 1.5f}, EntityType::Signboard)
    , m_message(std::move(message)) {
}

void Signboard::Update(float deltaTime) {
    (void)deltaTime;
}

const std::string& Signboard::GetMessage() const { return m_message; }

bool Signboard::CanInteract(const Player* player) const {
    if (!player) return false;
    Rectangle range = GetBoundingBox();
    range.x -= TILE_SIZE * 0.75f;
    range.width += TILE_SIZE * 1.5f;
    return CheckCollisionRecs(player->GetBoundingBox(), range);
}
