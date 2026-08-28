#ifndef BUFFPICKUP_H
#define BUFFPICKUP_H

#include "Entity.h"
#include "Systems/BuffSystem.h"

// A boon orb dropped into the boss arena. It bobs in place, expires if it is
// not collected, and grants its buff to whoever walks into it.
class BuffPickup : public Entity {
public:
    static constexpr float LIFETIME  = 14.0f;
    static constexpr float BOB_SPEED = 2.6f;
    static constexpr float BOB_RANGE = 5.0f;

    BuffPickup(Vector2 position, BuffType type);

    void Update(float deltaTime) override;

    BuffType GetBuffType() const { return m_buffType; }
    float GetLifeRatio() const {
        return LIFETIME > 0.0f ? m_lifeTimer / LIFETIME : 0.0f;
    }
    // Vertical bob offset, applied by the renderer only -- the collision box
    // stays put so the pickup is not harder to reach at the top of its arc.
    float GetBobOffset() const;

private:
    BuffType m_buffType;
    float    m_lifeTimer;
    float    m_bobTimer = 0.0f;
    Vector2  m_anchor;
};

#endif
