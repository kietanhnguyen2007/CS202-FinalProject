#ifndef CHARACTER_H
#define CHARACTER_H

#include "Entity.h"
#include "Utils/Types.h"
#include "Utils/Constants.h"

class Character : public Entity {
public:
    enum class State {
        Idle,
        Walk,
        Run,       // Sprint / Dash animation
        Jump,
        Fall,
        Dash,      // Invincibility dash
        Attack,
        Attack2,
        Attack3,
        Ultimate,  // Ultimate skill
        Parry,     // Block (reduces incoming damage)
        Hurt,
        Dead,
        Skill
    };

protected:
    int m_health;
    int m_maxHealth;
    float m_speed;
    float m_speedScale = 1.0f;
    Direction m_direction;
    float m_attackCooldown;
    float m_attackTimer;
    State m_state;

public:
    Character();
    explicit Character(EntityType type);
    Character(Vector2 position, Vector2 size, EntityType type);

    void Update(float deltaTime) override;

    int GetHealth() const;
    void SetHealth(int health);
    int GetMaxHealth() const;
    void SetMaxHealth(int maxHealth);
    virtual void TakeDamage(int damage);
    void Heal(int amount);
    bool IsAlive() const;

    float GetSpeed() const;
    void SetSpeed(float speed);

    // Multiplier applied on top of the base speed by status effects (a Wet or
    // Shocked target moves slower). Kept separate from m_speed so a slow can be
    // lifted without having to remember what the base value was.
    float GetSpeedScale() const { return m_speedScale; }
    void SetSpeedScale(float scale) { m_speedScale = scale; }
    float EffectiveSpeed() const { return m_speed * m_speedScale; }

    Direction GetDirection() const;
    void SetDirection(Direction direction);

    State GetState() const;
    void SetState(State state);

    void Move(Vector2 dir, float deltaTime);
    void MoveX(float dirX, float deltaTime); // Horizontal-only move, preserves gravity velocity

    float GetAttackCooldown() const;
    void SetAttackCooldown(float cooldown);
    bool CanAttack() const;
    virtual void Attack();
    void ResetAttackTimer();

    Rectangle GetAttackBoundingBox() const;
};

#endif
