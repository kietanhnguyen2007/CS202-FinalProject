#include "Factories/EnemyFactory.h"
#include "Utils/Constants.h"

std::unique_ptr<Enemy> EnemyFactory::CreateEnemy(Vector2 position, EnemyType type) {
    switch (type) {
        case EnemyType::Melee: return CreateMelee(position);
        case EnemyType::Ranged: return CreateRanged(position);
        case EnemyType::Flying: return CreateFlying(position);
    }
    return CreateMelee(position);
}

std::unique_ptr<Enemy> EnemyFactory::CreateMelee(Vector2 position) {
    auto enemy = std::make_unique<Enemy>(position, EnemyType::Melee);
    enemy->SetHealth(50);
    enemy->SetMaxHealth(50);
    enemy->SetSpeed(ENEMY_MELEE_SPEED);
    enemy->SetDamage(15);
    enemy->SetDetectionRange(200.0f);
    enemy->SetAttackRange(ENEMY_MELEE_RANGE);
    enemy->SetPatrolRange(100.0f);
    return enemy;
}

std::unique_ptr<Enemy> EnemyFactory::CreateRanged(Vector2 position) {
    auto enemy = std::make_unique<Enemy>(position, EnemyType::Ranged);
    enemy->SetHealth(30);
    enemy->SetMaxHealth(30);
    enemy->SetSpeed(ENEMY_RANGED_SPEED);
    enemy->SetDamage(10);
    enemy->SetDetectionRange(300.0f);
    enemy->SetAttackRange(ENEMY_RANGED_RANGE);
    enemy->SetPatrolRange(120.0f);
    return enemy;
}

std::unique_ptr<Enemy> EnemyFactory::CreateFlying(Vector2 position) {
    auto enemy = std::make_unique<Enemy>(position, EnemyType::Flying);
    enemy->SetHealth(40);
    enemy->SetMaxHealth(40);
    enemy->SetSpeed(ENEMY_FLYING_SPEED);
    enemy->SetDamage(12);
    enemy->SetDetectionRange(250.0f);
    enemy->SetAttackRange(ENEMY_MELEE_RANGE);
    enemy->SetPatrolRange(80.0f);
    return enemy;
}
