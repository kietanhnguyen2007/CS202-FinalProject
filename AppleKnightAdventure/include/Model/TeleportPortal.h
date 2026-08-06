#pragma once
#include "Model/Entity.h"
#include <string>

enum class PortalType {
    Local,
    LevelTransition,
    BossArena       // vào boss arena; targetLevelId=-1 = cổng thoát về level cũ
};

class TeleportPortal : public Entity {
public:
    TeleportPortal(Vector2 position, PortalType type, int colorId, int targetLevelId = -1);

    void Update(float deltaTime) override;
    virtual float GetZIndex() const override { return -0.05f; } // Render behind players

    PortalType GetPortalType() const;
    int GetColorId() const;
    
    TeleportPortal* GetLinkedPortal() const;
    void SetLinkedPortal(TeleportPortal* portal);

    int GetTargetLevelId() const;
    bool CanInteract(const class Player* player) const;

    // Lock/unlock — dùng cho cổng thoát BossArena (khóa đến khi boss chết)
    bool IsLocked() const;
    void SetLocked(bool locked);

private:
    PortalType m_portalType;
    int m_colorId;
    TeleportPortal* m_linkedPortal;
    int m_targetLevelId;
    bool m_isLocked = false;
};
