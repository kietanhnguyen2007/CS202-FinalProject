#ifndef FAKEWALL_H
#define FAKEWALL_H

#include "Entity.h"
#include "Utils/Constants.h"

class FakeWall : public Entity {
protected:
    bool m_destroyed;
    int m_health;

public:
    FakeWall();
    explicit FakeWall(Vector2 position, Vector2 size);

    void Update(float deltaTime) override;
    void Render() override;

    bool IsDestroyed() const;
    void TakeDamage(int damage);
    int GetHealth() const;
};

#endif
