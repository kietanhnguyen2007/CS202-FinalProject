#pragma once
#include "Model/Entity.h"
#include <string>

enum class PortalType {
    Local,
    LevelTransition
};

class TeleportPortal : public Entity {
public:
    TeleportPortal(Vector2 position, PortalType type, int colorId, int targetLevelId = -1);

    void Update(float deltaTime) override;

    PortalType GetPortalType() const;
    int GetColorId() const;
    
    TeleportPortal* GetLinkedPortal() const;
    void SetLinkedPortal(TeleportPortal* portal);
    
    int GetTargetLevelId() const;
    bool CanInteract(const class Player* player) const;

private:
    PortalType m_portalType;
    int m_colorId;
    TeleportPortal* m_linkedPortal;
    int m_targetLevelId;
};
