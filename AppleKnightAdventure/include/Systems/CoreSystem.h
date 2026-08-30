#ifndef CORESYSTEM_H
#define CORESYSTEM_H

#include "raylib.h"
#include "Utils/Types.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

// Run-long upgrades drafted three at a time, the campaign counterpart of the
// Survival3D upgrade table. Distinct from the boss-arena boons in BuffSystem:
// a boon is a timer that runs out, a core is kept for the rest of the level and
// stacks with every other copy of itself.
enum class CoreRarity : std::uint8_t {
    Common,
    Rare,
    Epic,
    Legendary,
    Count
};

enum class CoreId : std::uint8_t {
    VitalCore,        // +max HP, heals for the same amount
    SwiftStep,        // move faster
    TemperedGuard,    // take less damage
    QuickHands,       // cooldowns recover faster
    RiftEssence,      // small damage + max HP, stacks almost forever
    RiftPower,        // flat damage increase
    Execution,        // bonus damage to wounded targets
    EmberEdge,        // chance to set the target burning
    GravityLens,      // projectiles pass through an extra target
    ChainSpark,       // every Nth hit arcs to nearby enemies
    ElementalMastery, // elemental reactions hit harder
    SecondWind,       // one revive per run
    GlassRift,        // much more damage, much less health
    // ---- Class-locked. Only ever offered to the class named in classLock. ----
    IronLunge,        // Knight:      the lunge hits harder and travels further
    BulwarkOath,      // Knight:      parry stops damage outright
    Berserker,        // Fighter:     more damage while wounded
    OverchargedOrb,   // Fighter:     the ultimate orb hits far harder
    ShadowStep,       // Ninja:       teleport comes back much sooner
    TwinBlades,       // Ninja:       Blade Rush throws a second blade
    LingeringAura,    // MagicCaster: elemental auras last longer
    RiftConflux,      // MagicCaster: reactions splash twice as far and hard
    Count
};

// Mirrors CharacterClass, which is what CoreDefinition::classLock is compared
// against. Named here so the table does not carry bare integers.
enum class CoreClassLock : int {
    Any         = -1,
    Fighter     = 0,
    Knight      = 1,
    Ninja       = 2,
    MagicCaster = 3
};

// classLock is compared against a CharacterClass cast to int, so the two enums
// must stay in lockstep. Reordering CharacterClass without touching this would
// silently hand the Knight's cores to the Fighter.
static_assert(static_cast<int>(CoreClassLock::Fighter)
                  == static_cast<int>(CharacterClass::Fighter),
              "CoreClassLock must mirror CharacterClass");
static_assert(static_cast<int>(CoreClassLock::Knight)
                  == static_cast<int>(CharacterClass::Knight),
              "CoreClassLock must mirror CharacterClass");
static_assert(static_cast<int>(CoreClassLock::Ninja)
                  == static_cast<int>(CharacterClass::Ninja),
              "CoreClassLock must mirror CharacterClass");
static_assert(static_cast<int>(CoreClassLock::MagicCaster)
                  == static_cast<int>(CharacterClass::MagicCaster),
              "CoreClassLock must mirror CharacterClass");

struct CoreDefinition {
    CoreId      id;
    const char* key;          // stable identifier, for saves and debugging
    const char* name;
    const char* description;
    CoreRarity  rarity;
    int         maxStacks;
    // Restricts the core to one class, or -1 for every class.
    int         classLock;
    // Meaning depends on the core; see CoreDefs() in the .cpp.
    float       magnitude;
};

const std::vector<CoreDefinition>& CoreDefs();
const CoreDefinition& GetCoreDef(CoreId id);
Color RarityColor(CoreRarity rarity);
const char* RarityName(CoreRarity rarity);

// Everything the player has drafted this run, plus the derived numbers the rest
// of the game reads. Owned by Player so it travels with them.
class CoreLoadout {
public:
    // How many hits between Chain Spark arcs.
    static constexpr int CHAIN_SPARK_PERIOD = 6;

    void Clear();
    void Add(CoreId id);
    int  GetStack(CoreId id) const;
    bool Has(CoreId id) const { return GetStack(id) > 0; }
    bool IsMaxed(CoreId id) const;
    // Cores held, in the order they were first drafted -- for the HUD.
    const std::vector<CoreId>& Owned() const { return m_owned; }
    bool Empty() const { return m_owned.empty(); }

    // ---- Derived modifiers ----
    float DamageMultiplier() const;
    float DamageTakenMultiplier() const;
    float SpeedMultiplier() const;
    float CooldownRateMultiplier() const;
    // Extra max HP granted by cores, as a flat bonus and a fraction.
    int   BonusMaxHealthFlat() const;
    float MaxHealthMultiplier() const;
    // Damage multiplier against a target already below the Execution threshold.
    float ExecutionMultiplier() const;
    float ExecutionHealthThreshold() const;
    // Chance in [0,1] that a hit sets the target burning.
    float EmberEdgeChance() const;
    int   ProjectilePierceBonus() const;
    float ReactionMultiplier() const;
    // ---- Class-locked effects ----
    // Damage bonus that only applies while the holder is below half health.
    float WoundedDamageMultiplier(float healthFraction) const;
    // Fraction of incoming damage a parry lets through. 1.0 means the core is
    // not held and the caller keeps its own parry maths.
    bool  ParryBlocksEverything() const { return Has(CoreId::BulwarkOath); }
    // How much longer an elemental aura sticks, as a multiplier.
    float AuraDurationMultiplier() const;
    // Reaction splash scaling, applied to both the radius and the damage share.
    float SplashScale() const;
    bool  BladeRushIsTwin() const { return Has(CoreId::TwinBlades); }

    // Revive bookkeeping for Second Wind.
    bool  CanRevive() const { return Has(CoreId::SecondWind) && !m_reviveUsed; }
    void  ConsumeRevive() { m_reviveUsed = true; }
    float ReviveHealthFraction() const;

    // Chain Spark: counts hits and reports the ones that should arc.
    bool  RegisterHitAndCheckChain();

    // Draft `count` distinct cores. bossReward tilts the roll toward the top
    // rarities the way clearing a boss wave does in Survival3D.
    std::vector<CoreId> RollDraft(int count, bool bossReward, int classLock);

private:
    std::array<int, static_cast<std::size_t>(CoreId::Count)> m_stacks{};
    std::vector<CoreId> m_owned;
    bool m_reviveUsed = false;
    int  m_hitCounter = 0;
    // Pity timer: after this many drafts with nothing above Rare, the next
    // draft is forced to lead with an Epic.
    int  m_draftsWithoutEpic = 0;
};

#endif
