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
    WindUp,   // 0.75s telegraph before damage window
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
    BossAttack,
    RangedBomb,
    FlyingProjectile
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
    UltimateFighter,
    UltimateKnight,
    UltimateNinja,
    UltimateMagicCaster
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

enum class CharacterClass {
    Fighter,
    Knight,
    Ninja,
    MagicCaster
};

enum class BossPhase {
    Phase1,
    Phase2,
    Phase3,
    Enraged
};

#endif
