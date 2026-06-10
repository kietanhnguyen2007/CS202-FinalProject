#ifndef CHARACTER_H
#define CHARACTER_H

#include "Entity.h"
#include "Utils/Types.h"
#include "Utils/Constants.h"
#include "Systems/AnimationSystem.h"

class Character : public Entity {
protected:
    int m_health;
    int m_maxHealth;
    float m_speed;
    Direction m_direction;
    float m_attackCooldown;
    float m_attackTimer;
    // Animator component for rendering
    Systems::Animator m_animator;

public:
    Character();
    explicit Character(EntityType type);
    Character(Vector2 position, Vector2 size, EntityType type);

    void Update(float deltaTime) override;
    void Render() override;

    // Animator access
    Systems::Animator& GetAnimator();

    int GetHealth() const;
    void SetHealth(int health);
    int GetMaxHealth() const;
    void SetMaxHealth(int maxHealth);
    void TakeDamage(int damage);
    void Heal(int amount);
    bool IsAlive() const;

    float GetSpeed() const;
    void SetSpeed(float speed);

    Direction GetDirection() const;
    void SetDirection(Direction direction);

    void Move(Vector2 dir, float deltaTime);

    float GetAttackCooldown() const;
    void SetAttackCooldown(float cooldown);
    bool CanAttack() const;
    virtual void Attack();
    void ResetAttackTimer();

    Rectangle GetAttackBoundingBox() const;
};

#endif
