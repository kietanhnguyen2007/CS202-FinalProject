#ifndef TYPES_H
#define TYPES_H

enum class Direction {
    None,
    Up,
    Down,
    Left,
    Right
};

enum class EnemyType {
    Melee,
    Ranged,
    Flying
};

enum class EnemyState {
    Idle,
    Patrol,
    Chase,
    Attack,
    Hurt,
    Dead
};

enum class ItemType {
    Coin,
    Apple,
    Key,
    Potion,
    Equipment
};

enum class ProjectileType {
    Arrow,
    Magic,
    BossAttack
};

enum class PetType {
    Skull,
    Ghost,
    BabyDragon,
    Fairy
};

enum class WorldLayer {
    Light,
    Shadow
};

enum class WeaponType {
    Sword,
    Bow,
    Staff
};

enum class SkillType {
    Fireball,
    Heal,
    Dash,
    Shield
};

enum class DamageType {
    Physical,
    Fire,
    Water,
    Thunder
};

enum class StatusEffect {
    None,
    Burn,
    Wet,
    Shocked
};

enum class GameMode {
    SinglePlayer,
    MultiplayerHost,
    MultiplayerClient
};

enum class BossPhase {
    Phase1,
    Phase2,
    Phase3,
    Enraged
};

#endif
