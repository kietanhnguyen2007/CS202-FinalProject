#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstddef>

constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;

// Base resolution used as reference for UI scaling (do not change)
constexpr int BASE_SCREEN_WIDTH  = SCREEN_WIDTH;
constexpr int BASE_SCREEN_HEIGHT = SCREEN_HEIGHT;

constexpr float TILE_SIZE = 64.0f * (2.0f / 3.0f);
constexpr float GRAVITY = 980.0f;
// Light baseline buff: a little more mobility without changing jump physics.
constexpr float PLAYER_SPEED = 210.0f;
constexpr float PLAYER_JUMP_FORCE = -600.0f;
constexpr int PLAYER_MAX_HEALTH = 110;
constexpr int ENEMY_MELEE_RANGE = 100;
constexpr int ENEMY_RANGED_RANGE = 300;
constexpr float ENEMY_MELEE_SPEED = 120.0f;
constexpr float ENEMY_RANGED_SPEED = 80.0f;
constexpr float ENEMY_FLYING_SPEED = 100.0f;
constexpr int BOSS_MAX_HEALTH = 500;
constexpr float BOSS_SPEED = 90.0f;
// Boss collision box, shared by every spawn path so the body, the melee hitbox
// and the sprite stay in proportion. BOSS_BODY_SCALE trims the oversized 1.75
// art factor; CharacterRenderer applies the same factor to the sprite. 0.75 * 0.9
// -- bosses read 10% smaller than before without touching any of the art.
constexpr float BOSS_BODY_SCALE = 0.675f;
constexpr float BOSS_BODY_WIDTH  = TILE_SIZE * 0.85f  * 1.75f * BOSS_BODY_SCALE;
constexpr float BOSS_BODY_HEIGHT = TILE_SIZE * 1.683f * 1.75f * BOSS_BODY_SCALE;
// How far a melee swing reaches past the body, as a fraction of the boss's
// engage range, clamped to a multiple of its own width.
constexpr float BOSS_MELEE_REACH_RATIO = 0.55f;
// Chase tuning: how much faster the boss closes distance when it is far away,
// and how long it must wait between jumps.
constexpr float BOSS_CHASE_SPEED_MULT = 1.55f;
constexpr float BOSS_JUMP_COOLDOWN = 0.55f;
// While a melee boss waits out its attack cooldown it keeps walking in until it
// is this deep inside its own attack range, so being hit never roots it.
constexpr float BOSS_HUG_RANGE_RATIO = 0.45f;
// Distance the caster boss drifts back to between skills.
constexpr float BOSS2_PREFERRED_RANGE = 320.0f;
// Navigation brain. A boss that has asked to walk but covered less than
// BOSS_STUCK_DISTANCE for BOSS_STUCK_TIME is wedged, and commits to an unstick
// shove for BOSS_UNSTICK_TIME. Reachability is re-answered on an interval
// because the flood fill is the expensive part of the whole AI.
constexpr float BOSS_STUCK_DISTANCE = 2.0f;    // px covered before it counts as moving
constexpr float BOSS_STUCK_TIME     = 0.55f;   // seconds of no progress = wedged
constexpr float BOSS_UNSTICK_TIME   = 0.7f;    // seconds committed to the shove
constexpr float BOSS_NAV_RECHECK_INTERVAL = 0.35f;
// How long the boss keeps backing away once it decides the player is holed up
// somewhere it can neither reach nor shoot into.
constexpr float BOSS_RETREAT_TIME   = 2.0f;
constexpr float PROJECTILE_SPEED = 400.0f;
constexpr float PARTICLE_LIFETIME = 1.0f;
constexpr float GRAVITY_PROJECTILE = 200.0f;
constexpr float PET_FOLLOW_DISTANCE = 80.0f;
constexpr float PET_SPEED = 180.0f;
constexpr int CHEST_MIN_LOOT = 1;
constexpr int CHEST_MAX_LOOT = 3;
constexpr int FAKE_WALL_HEALTH = 3;
// Occasional wandering enemies. Kept deliberately rare -- this is flavour, not
// a spawner: at most a few alive at once, and never on top of the player.
constexpr float RANDOM_SPAWN_MIN_INTERVAL = 22.0f;  // seconds between attempts
constexpr float RANDOM_SPAWN_MAX_INTERVAL = 38.0f;
constexpr int   RANDOM_SPAWN_MAX_ALIVE    = 3;      // cap on live random enemies
constexpr float RANDOM_SPAWN_MIN_DISTANCE = 420.0f; // never closer than this
constexpr float RANDOM_SPAWN_MAX_DISTANCE = 1100.0f;

// Boss-arena boons: how often an orb drops and how many may wait at once.
constexpr float BUFF_SPAWN_INTERVAL = 10.0f;
constexpr int   BUFF_MAX_ON_FIELD   = 2;

// Boon draft: while a boss fight is running, the fight freezes every so often
// and offers three boons picked with 1/2/3. The first draft comes as soon as
// the arena is entered plus this delay, then it repeats.
// Elemental reactions land ten times harder on a boss. A boss carries thousands
// of HP against a mob's tens, so without this a reaction that shreds a mob is
// invisible on a boss and the whole element system stops mattering in the fight
// it was built for. Plain elemental hits and damage-over-time are untouched --
// only the reaction payoff is scaled.
constexpr float ELEMENTAL_REACTION_BOSS_MULT = 10.0f;

// A reaction splashes onto everything else standing this close to the target.
// Splash damage is a share of the reaction, and it does not re-react -- it is
// collateral, not a chain that could recurse.
constexpr float ELEMENTAL_SPLASH_RADIUS = 96.0f;
constexpr float ELEMENTAL_SPLASH_SHARE  = 0.45f;

// Hit-stop windows, in seconds. Long enough to read, short enough that input
// never feels swallowed.
constexpr float HITSTOP_MAX             = 0.14f;
constexpr float HITSTOP_REACTION        = 0.075f;
constexpr float HITSTOP_REACTION_BOSS   = 0.11f;
constexpr float HITSTOP_HEAVY_HIT       = 0.045f;
// A normal hit only freezes if it is at least this big, so chip damage does not
// stutter the whole fight.
constexpr int   HITSTOP_HEAVY_THRESHOLD = 40;

// Core draft: how many ordinary kills earn a draft, and how many cores are
// offered each time. A boss kill always earns one on its own, at boss rarity.
constexpr float CHAIN_SPARK_RADIUS     = 150.0f;
// Twin Blades: the second blade is offset and slower so the two read as a pair
// sweeping a lane, not as one blade drawn twice.
constexpr float TWIN_BLADE_OFFSET_Y    = 18.0f;
constexpr float TWIN_BLADE_SPEED_SCALE = 0.78f;
constexpr int CORE_DRAFT_KILL_INTERVAL = 8;
constexpr int CORE_DRAFT_COUNT         = 3;

// How close the player must be to a living boss for the fight to count as
// engaged, which is what starts the boon timers.
constexpr float BOSS_FIGHT_ENGAGE_RANGE = 900.0f;

constexpr float BUFF_OFFER_MIN_DELAY = 10.0f;
constexpr float BUFF_OFFER_MAX_DELAY = 15.0f;
constexpr int   BUFF_OFFER_COUNT     = 3;

// Largest step the simulation will take. A hitch is absorbed as a slow frame
// rather than a teleport.
constexpr float MAX_FRAME_DELTA = 1.0f / 20.0f;

constexpr float ATTACK_COOLDOWN = 0.5f;
constexpr float BOSS_ATTACK_COOLDOWN = 1.5f;

#endif
