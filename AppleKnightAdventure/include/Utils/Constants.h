#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstddef>

constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;
constexpr int TILE_SIZE = 64;
constexpr float GRAVITY = 980.0f;
constexpr float PLAYER_SPEED = 200.0f;
constexpr float PLAYER_JUMP_FORCE = -400.0f;
constexpr int PLAYER_MAX_HEALTH = 100;
constexpr int ENEMY_MELEE_RANGE = 40;
constexpr int ENEMY_RANGED_RANGE = 300;
constexpr float ENEMY_MELEE_SPEED = 120.0f;
constexpr float ENEMY_RANGED_SPEED = 80.0f;
constexpr float ENEMY_FLYING_SPEED = 100.0f;
constexpr int BOSS_MAX_HEALTH = 500;
constexpr float BOSS_SPEED = 60.0f;
constexpr int INVENTORY_MAX_SLOTS = 24;
constexpr int NETWORK_PORT = 54000;
constexpr size_t NETWORK_BUFFER_SIZE = 4096;
constexpr float PROJECTILE_SPEED = 400.0f;
constexpr float PARTICLE_LIFETIME = 1.0f;
constexpr float GRAVITY_PROJECTILE = 200.0f;
constexpr float PET_FOLLOW_DISTANCE = 80.0f;
constexpr float PET_SPEED = 180.0f;
constexpr int CHEST_MIN_LOOT = 1;
constexpr int CHEST_MAX_LOOT = 3;
constexpr int FAKE_WALL_HEALTH = 3;
constexpr float ATTACK_COOLDOWN = 0.5f;
constexpr float BOSS_ATTACK_COOLDOWN = 1.5f;

#endif
