#include "Model/TriggerZone.h"

TriggerZone::TriggerZone(Vector2 position, Vector2 size, const std::string& targetLevelId)
    : Entity(position, size, EntityType::TriggerZone)
    , m_targetLevelId(targetLevelId)
    , m_triggered(false)
{
}

void TriggerZone::Update(float /*deltaTime*/) {
    // Logic for checking collision is handled in GameController
}

std::string TriggerZone::GetTargetLevelId() const {
    return m_targetLevelId;
}

void TriggerZone::SetTargetLevelId(const std::string& targetLevelId) {
    m_targetLevelId = targetLevelId;
}

bool TriggerZone::IsTriggered() const {
    return m_triggered;
}

void TriggerZone::ResetTrigger() {
    m_triggered = false;
}
