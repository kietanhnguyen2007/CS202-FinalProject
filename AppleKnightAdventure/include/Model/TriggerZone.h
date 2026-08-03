#ifndef TRIGGERZONE_H
#define TRIGGERZONE_H

#include "Model/Entity.h"
#include <string>

class TriggerZone : public Entity {
private:
    std::string m_targetLevelId;
    bool m_triggered;

public:
    TriggerZone(Vector2 position, Vector2 size, const std::string& targetLevelId = "");
    ~TriggerZone() override = default;

    void Update(float deltaTime) override;

    std::string GetTargetLevelId() const;
    void SetTargetLevelId(const std::string& targetLevelId);

    bool IsTriggered() const;
    void ResetTrigger();
};

#endif
