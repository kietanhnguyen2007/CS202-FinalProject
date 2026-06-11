#ifndef ENEMY_H
#define ENEMY_H

#include "Character.h"
#include "Utils/Types.h"
#include "Utils/Constants.h"

class Enemy : public Character {
protected:
    EnemyType m_enemyType;
    EnemyState m_state;
    int m_damage;
    float m_detectionRange;
    float m_attackRange;
    float m_patrolRange;
    Vector2 m_spawnPosition;
    float m_stateTimer;

public:
    Enemy();
    Enemy(Vector2 position, EnemyType type);

    void Update(float deltaTime) override;

    EnemyType GetEnemyType() const;
    EnemyState GetState() const;
    void SetState(EnemyState state);

    int GetDamage() const;
    void SetDamage(int damage);

    float GetDetectionRange() const;
    void SetDetectionRange(float range);
    float GetAttackRange() const;
    void SetAttackRange(float range);
    float GetPatrolRange() const;
    void SetPatrolRange(float range);

    Vector2 GetSpawnPosition() const;

    virtual void UpdateAI(Vector2 playerPosition, float deltaTime);
    void Patrol(float deltaTime);
    void Chase(Vector2 playerPosition, float deltaTime);
    void Attack() override;
    void TakeDamage(int damage);
};

#endif
