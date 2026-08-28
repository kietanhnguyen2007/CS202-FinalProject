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

// Boons offered during a boss fight. Instant ones fire once on pickup; the rest
// run for a duration and are read back as multipliers.
enum class BuffType {
    Vigor,        // instant: restore a chunk of max HP
    SecondWind,   // regenerate HP over time
    Focus,        // instant: clear every skill cooldown
    Adrenaline,   // skill cooldowns tick faster
    Haste,        // move faster
    Power,        // deal more damage
    Aegis,        // take less damage
    Bloodthirst,  // heal for a share of damage dealt
    Count
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
    Phase4,
    Enraged
};

enum class BossState {
    Idle,
    Walk,
    Hurt,
    Die,
    Transition,
    Skill1,
    Skill2,
    Skill3,
    Skill4
};


#endif
