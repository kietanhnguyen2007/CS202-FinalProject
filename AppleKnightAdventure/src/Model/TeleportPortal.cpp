#include "Model/TeleportPortal.h"
#include "Utils/Constants.h"
#include "Model/Player.h"
#include <raymath.h>

TeleportPortal::TeleportPortal(Vector2 position, PortalType type, int colorId, int targetLevelId)
    : Entity(position, {283.0f, 308.0f}, EntityType::TeleportPortal),
      m_portalType(type),
      m_colorId(colorId),
      m_linkedPortal(nullptr),
      m_targetLevelId(targetLevelId),
      m_isLocked(false)
{
    m_scale = 0.44f; // Visual size is ~ 124 x 135
}

void TeleportPortal::Update(float deltaTime) {
}

PortalType TeleportPortal::GetPortalType() const {
    return m_portalType;
}

int TeleportPortal::GetColorId() const {
    return m_colorId;
}

TeleportPortal* TeleportPortal::GetLinkedPortal() const {
    return m_linkedPortal;
}

void TeleportPortal::SetLinkedPortal(TeleportPortal* portal) {
    m_linkedPortal = portal;
}

int TeleportPortal::GetTargetLevelId() const {
    return m_targetLevelId;
}

bool TeleportPortal::CanInteract(const Player* player) const {
    if (!player) return false;
    if (m_isLocked) return false;   // cổng thoát BossArena bị khóa

    Rectangle playerBounds = player->GetBoundingBox();
    Rectangle portalBounds = GetBoundingBox();

    return CheckCollisionRecs(playerBounds, portalBounds);
}

bool TeleportPortal::IsLocked() const {
    return m_isLocked;
}

void TeleportPortal::SetLocked(bool locked) {
    m_isLocked = locked;
}
