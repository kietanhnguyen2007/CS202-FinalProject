#ifndef ENEMYFACTORY_H
#define ENEMYFACTORY_H

#include "raylib.h"
#include "Model/Enemy.h"
#include <memory>

class EnemyFactory {
public:
    static std::unique_ptr<Enemy> CreateEnemy(Vector2 position, EnemyType type);
    static std::unique_ptr<Enemy> CreateMelee(Vector2 position);
    static std::unique_ptr<Enemy> CreateRanged(Vector2 position);
    static std::unique_ptr<Enemy> CreateFlying(Vector2 position);
};

#endif
