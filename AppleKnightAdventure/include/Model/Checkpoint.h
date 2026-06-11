#ifndef CHECKPOINT_H
#define CHECKPOINT_H

#include "Entity.h"

class Checkpoint : public Entity {
protected:
    bool m_activated;

public:
    Checkpoint();
    explicit Checkpoint(Vector2 position);

    void Update(float deltaTime) override;

    bool IsActivated() const;
    void Activate();
    void Deactivate();
};

#endif
