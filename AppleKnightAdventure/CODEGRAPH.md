# CODEGRAPH — Backend API Reference for View Team

## How to include (CMake include root is `include/`)
```cpp
#include "Model/Entity.h"
#include "Utils/Types.h"
#include "Systems/CollisionSystem.h"
#include "Factories/EnemyFactory.h"
#include "Network/Packet.h"
```

---

## Enums (`include/Utils/Types.h`)

| Enum | Values | Usage |
|------|--------|-------|
| `Direction` | `None, Up, Down, Left, Right` | Character facing, projectile direction, Move() param |
| `EntityType` | `Player, DualWorldPlayer, Enemy, Boss, Projectile, Item, Checkpoint, Chest, FakeWall, Pet, Particle, Effect` | Entity::GetType() — switch on this to render different sprites |
| `EnemyType` | `Melee, Ranged, Flying` | Enemy::GetEnemyType() — different AI + stats |
| `EnemyState` | `Idle, Patrol, Chase, Attack, Hurt, Dead` | Enemy AI state machine |
| `ItemType` | `Coin, Apple, Key, Potion, Equipment` | Item::GetItemType() — render different icons |
| `ProjectileType` | `Arrow, Magic, BossAttack` | Projectile type for render variant |
| `PetType` | `Skull, Ghost, BabyDragon, Fairy` | Pet variant |
| `DamageType` | `Physical, Fire, Water, Thunder` | ElementalSystem damage type |
| `StatusEffect` | `None, Burn, Wet, Shocked` | Status effect on entities |
| `GameMode` | `SinglePlayer, MultiplayerHost, MultiplayerClient` | GameState mode |
| `BossPhase` | `Phase1, Phase2, Phase3, Enraged` | Boss behavior phase |
| `WorldLayer` | `Light, Shadow` | DualWorld and DualWorldPlayer layer |

---

## Constants (`include/Utils/Constants.h`)

| Name | Value | Used By |
|------|-------|---------|
| `SCREEN_WIDTH` | 1280 | Window size |
| `SCREEN_HEIGHT` | 720 | Window size |
| `TILE_SIZE` | 64 | All entity sizes are multiples of this |
| `GRAVITY` | 980.0 | Physics |
| `PLAYER_SPEED` | 200.0 | Player default speed |
| `PLAYER_JUMP_FORCE` | -400.0 | Player jump velocity |
| `PLAYER_MAX_HEALTH` | 100 | Player max HP |
| `ENEMY_MELEE_SPEED` | 120.0 | Melee enemy speed |
| `ENEMY_RANGED_SPEED` | 80.0 | Ranged enemy speed |
| `ENEMY_FLYING_SPEED` | 100.0 | Flying enemy speed |
| `BOSS_MAX_HEALTH` | 500 | Boss max HP |
| `BOSS_SPEED` | 60.0 | Boss speed |
| `PROJECTILE_SPEED` | 400.0 | Projectile travel speed |
| `INVENTORY_MAX_SLOTS` | 24 | Inventory item capacity |
| `NETWORK_PORT` | 54000 | Default TCP port for Radmin VPN |
| `ATTACK_COOLDOWN` | 0.5 | Standard attack cooldown (seconds) |
| `BOSS_ATTACK_COOLDOWN` | 1.5 | Boss attack cooldown (seconds) |
| `PET_FOLLOW_DISTANCE` | 80.0 | Distance pet stays from owner |
| `PET_SPEED` | 180.0 | Pet movement speed |
| `FAKE_WALL_HEALTH` | 3 | Hits to break a fake wall |
| `CHEST_MIN_LOOT` / `CHEST_MAX_LOOT` | 1 / 3 | Items per chest |

---

## Class Hierarchy

```
Entity  ← abstract base (Model/Entity.h)
├── Character  ← movement + combat (Model/Character.h)
│   ├── Player  ← inventory + score (Model/Player.h)
│   │   └── DualWorldPlayer  ← WorldLayer (Model/DualWorldPlayer.h)
│   ├── Enemy  ← AI state machine (Model/Enemy.h)
│   ├── Boss  ← phases + enrage (Model/Boss.h)
│   └── Pet  ← follow AI (Model/Pet.h)
├── Item  ← collectible (Model/Item.h)
├── Projectile  ← physics projectile (Model/Projectile.h)
├── Checkpoint  ← respawn point (Model/Checkpoint.h)
├── FakeWall  ← breakable wall (Model/FakeWall.h)
└── Chest  ← random loot (Model/Chest.h)
```

Standalone: `Inventory`, `GameState`, `LevelScoring`, `DualWorld`, `CrossWorldManager`

---

## Entity (`include/Model/Entity.h`)
Abstract base. Every game object inherits from this.

### Fields (protected — access via getters)
| Field | Type | Meaning |
|-------|------|---------|
| `m_id` | `int` | Unique ID (0 = unset). Used for network sync and entity lookup |
| `m_type` | `EntityType` | Runtime type. Set by constructor. No `dynamic_cast` needed |
| `m_position` | `Vector2` | World position (x, y). Renderer reads this for draw calls |
| `m_size` | `Vector2` | Base size (width, height). Gets scaled by `m_scale` |
| `m_velocity` | `Vector2` | Movement per second. Character::Update() uses this |
| `m_rotation` | `float` | Degrees. Used by Projectile and sprite rotation |
| `m_scale` | `float` | Uniform scale. Default 1.0. Boss enraged = 1.3 |
| `m_active` | `bool` | true = update/render, false = skip (don't delete from container) |

### Constructors
```
Entity()                              → type=Effect, pos/size=(0,0), active=true
Entity(EntityType type)               → set type, rest defaults
Entity(Vector2 pos, Vector2 size, EntityType type)
~Entity() = default                   → virtual destructor
```

### Pure virtuals (MUST override in subclass)
```
Update(float deltaTime) → void        ← per-frame logic, called by GameState
Render() → void                       ← draw call. View team implements this
```

### Getters/Setters (all inline)
```
GetId() / SetId(int)
GetType() → EntityType                ← use for switch-based rendering
GetPosition() / SetPosition(Vector2)
GetSize() / SetSize(Vector2)
GetVelocity() / SetVelocity(Vector2)
GetRotation() / SetRotation(float)
GetScale() / SetScale(float)
IsActive() / SetActive(bool)
```

### Collision
```
GetBoundingBox() → Rectangle
    returns {position.x, position.y, size.x * scale, size.y * scale}
    directly usable with raylib CheckCollisionRecs()
```

---

## Character (`include/Model/Character.h`)
Extends Entity. Base for all living things (Player, Enemy, Boss, Pet).

### Additional fields
| Field | Type | Default | Meaning |
|-------|------|---------|---------|
| `m_health` | int | 100 | Current HP |
| `m_maxHealth` | int | 100 | Max HP |
| `m_speed` | float | 0 | Movement speed (px/sec) |
| `m_direction` | Direction | None | Facing direction |
| `m_attackCooldown` | float | ATTACK_COOLDOWN | Seconds between attacks |
| `m_attackTimer` | float | 0 | Counts up from 0 to cooldown |

### Health
```
GetHealth() → int                     ← for HUD health bar
SetHealth(int)                        ← clamped to [0, maxHealth]
GetMaxHealth() / SetMaxHealth(int)
TakeDamage(int damage)                ← subtracts from health, sets active=false if <=0
Heal(int amount)                      ← health = min(health+amount, maxHealth)
IsAlive() → bool                      ← health > 0
```

### Movement
```
GetSpeed() / SetSpeed(float)
GetDirection() / SetDirection(Direction)  ← View reads this for sprite flip
Move(Vector2 dir, float deltaTime)    ← dir = {dx, dy} normalized, velocity = dir * speed
```

### Combat
```
GetAttackCooldown() / SetAttackCooldown(float)
CanAttack() → bool                    ← attackTimer <= 0
Attack() → void                       ← virtual. Sets attackTimer = cooldown. Override for custom.
ResetAttackTimer()
GetAttackBoundingBox() → Rectangle    ← forward extension in facing direction. Used by CollisionSystem.
```

---

## Player (`include/Model/Player.h`)
Extends Character. One per user.

### Additional fields
| Field | Type | Meaning |
|-------|------|---------|
| `m_inventory` | Inventory | Item storage |
| `m_score` | int | Accumulated score |
| `m_skillPoints` | int | Unspent skill points |
| `m_name` | string | Player name (for network/HUD) |

### Constructors
```
Player() → default, position=(0,0), speed=PLAYER_SPEED, health=PLAYER_MAX_HEALTH
Player(Vector2 position) → position, size = {TILE_SIZE*0.8, TILE_SIZE*0.9}
```

### API
```
GetInventory() / GetInventory() const → Inventory& / const Inventory&
    ← View reads this for inventory UI
GetScore() / AddScore(int) / SetScore(int)
GetSkillPoints() / SetSkillPoints(int) / AddSkillPoints(int)
GetName() / SetName(const string&)
```

---

## Enemy (`include/Model/Enemy.h`)
Extends Character. AI-driven.

### Constructors
```
Enemy() → default, type=Melee
Enemy(Vector2 position, EnemyType type)  ← use EnemyFactory instead
```

### AI
```
GetEnemyType() / GetState() / SetState(EnemyState)
UpdateAI(Vector2 playerPosition, float deltaTime)  ← virtual. Call in Update().
    Runs state machine: Idle→Patrol→Chase→Attack.
    ↑ Called by Controller/GameState each frame
Patrol(float deltaTime)                ← sin-wave patrol around spawn
Chase(Vector2 playerPos, float dt)     ← moves toward player
TakeDamage(int)                        ← reduces health, sets state=Hurt if alive
```

---

## Boss (`include/Model/Boss.h`)
Extends Character. Multi-phase boss with enrage.

### Constructors
```
Boss() → default phase=Phase1, health=BOSS_MAX_HEALTH
Boss(Vector2 pos, Vector2 size)
```

### Phase API
```
GetPhase() → BossPhase                 ← View reads for phase-specific effects
SetPhase(BossPhase)
TransitionToNextPhase()                ← auto-walks phaseOrder
IsEnraged() → bool                     ← phase == Enraged

OnPhaseEnter(BossPhase)                ← virtual hook. Override for phase FX
    Default: Phase2 (damage+10, speed*1.2), Phase3 (damage+15, cd*0.8),
    Enraged (damage*2, speed*1.5, cd*0.6, scale=1.3)
ExecutePhaseBehavior(float dt)         ← virtual. Override for phase attack patterns
```

---

## Pet (`include/Model/Pet.h`)
Extends Character. Auto-follows player.

```
Pet(Vector2 pos, PetType type, int ownerId)
GetPetType()
GetOwnerId() / SetOwnerId(int)
GetFollowDistance() / SetFollowDistance(float)
FollowPlayer(Vector2 playerPos, float dt)  ← moves toward player if distance > followDistance
UpdateAI(Vector2 playerPos, float dt)      ← calls FollowPlayer()
```

---

## Item (`include/Model/Item.h`)
Extends Entity. Collectible on the ground.

```
Item(Vector2 pos, ItemType type, int amount = 1)
GetItemType() → ItemType              ← View: render different sprite per type
GetAmount() / SetAmount(int)          ← Coin amount varies
GetItemName() → const string&         ← "Coin", "Apple", "Key", "Potion", "Equipment"
```

---

## Projectile (`include/Model/Projectile.h`)
Extends Entity. Travels in a direction, expires after lifetime or on hit.

```
Projectile(Vector2 pos, Vector2 size, ProjectileType type,
           Direction direction, int damage, int ownerId)
GetProjectileType()
GetDamage()
GetDirection()
GetOwnerId()                          ← who shot it (avoid self-collision)
GetLifetime()                         ← max lifetime in seconds
HasExpired() → bool                   ← expired by time or hit
OnHit()                               ← deactivates projectile
```

---

## Inventory (`include/Model/Inventory.h`)
Standalone. Player's inventory.

```
Inventory() → maxSlots=INVENTORY_MAX_SLOTS (24)
Inventory(int maxSlots)

AddItem(unique_ptr<Item>) → bool      ← false if full
RemoveItem(int index) → unique_ptr<Item>
GetItem(int index) → Item*            ← nullptr if out of range
GetItemCount() / GetMaxSlots()
IsFull() / Clear()

GetCoins() / AddCoins(int) / SpendCoins(int) → bool
GetApples() / AddApples(int) / UseApple() → bool
GetKeys() / AddKeys(int) / UseKey() → bool
```

---

## Checkpoint (`include/Model/Checkpoint.h`)
```
Checkpoint(Vector2 pos)               ← size = {TILE_SIZE*0.6, TILE_SIZE*0.8}
IsActivated() / Activate() / Deactivate()
```

---

## Chest (`include/Model/Chest.h`)
```
Chest(Vector2 pos)                    ← size = {TILE_SIZE*0.8, TILE_SIZE*0.6}
IsOpened() → bool
Open() → vector<unique_ptr<Item>>     ← generates loot if not yet generated, then returns it
GenerateLoot()                        ← weighted random: 40% Coin, 25% Apple, 15% Key, 15% Potion, 5% Equipment
```

---

## FakeWall (`include/Model/FakeWall.h`)
```
FakeWall(Vector2 pos, Vector2 size)
IsDestroyed()
TakeDamage(int)                       ← health -= damage, destroy when <=0
GetHealth()                           ← FAKE_WALL_HEALTH (3) default
```

---

## GameState (`include/Model/GameState.h`)
Central state holder. Controller creates one, populates it, passes to View for rendering.

```
GameState() / GameState(GameMode mode)
GetMode() / SetMode(GameMode)         ← SinglePlayer / MultiplayerHost / MultiplayerClient

GetLocalPlayer() → Player*            ← nullptr if not set. View reads for HUD/camera
SetLocalPlayer(unique_ptr<Player>)

AddEntity(unique_ptr<Entity>)         ← adds to entity list
RemoveEntity(int entityId)            ← removes by ID
GetEntity(int entityId) → Entity*     ← lookup by ID
GetAllEntities() → const vector<unique_ptr<Entity>>&
    ← View iterates this to render all entities each frame

GetCurrentLevel() / SetCurrentLevel(int)
GetTotalLevels() → int

Update(float deltaTime)               ← calls Update() on localPlayer + all entities
Clear()                               ← removes everything
```

### Typical render loop for View team
```cpp
// In GameView::Render():
// 1. Render background/tiles
// 2. For each entity in gameState->GetAllEntities():
//      switch (entity->GetType()) {
//          case EntityType::Player:  draw player sprite; break;
//          case EntityType::Enemy:   draw enemy sprite; break;
//          ...
//      }
// 3. Render HUD using GetLocalPlayer()->GetHealth(), etc.
```

---

## DualWorld (`include/Model/DualWorld.h`)
Dual-layer map for co-op levels.

```
DualWorld() / DualWorld(int width, int height)
GetActiveLayer() / SetActiveLayer(WorldLayer) / SwitchLayer()
GetWidth() / GetHeight()
GetTiles(WorldLayer) → const vector<Tile>&   ← tiles for rendering
AddTile(WorldLayer, Tile)
IsTileSolid(int x, int y, WorldLayer) → bool
```

## DualWorldPlayer (`include/Model/DualWorldPlayer.h`)
```
DualWorldPlayer(Vector2 pos, WorldLayer layer = Light)
GetLayer() / SetLayer(WorldLayer) / SwitchLayer()
```

## CrossWorldManager (`include/Model/CrossWorldManager.h`)
```
CrossWorldManager(DualWorld* world)
SetWorld() / GetWorld()
RegisterPlayer(DualWorldPlayer*) / UnregisterPlayer()
GetPlayers() / GetPlayersInLayer(WorldLayer)
Update(float dt)                      ← updates all registered players
CanPlayerCross(DualWorldPlayer*) → bool
TeleportPlayerToOtherWorld(DualWorldPlayer*) → bool    ← switches player + world layer
```

---

## LevelScoring (`include/Model/LevelScoring.h`)
```
GetCurrentScore() / AddScore(int)
GetHighScore() / SetHighScore(int)
GetStars() / CalculateStars()         ← 1-3 stars based on itemRatio, enemyRatio, scoreBonus
CollectItem() / DefeatEnemy()
SetClearTime(float) / SetTotals(int items, int enemies)
GetClearTime() / GetCollectedItems() / GetDefeatedEnemies()
IsNewHighScore() → bool
SaveScore(const string& playerName)   ← adds to leaderboard sorted by score
GetLeaderboard() → const vector<ScoreEntry>&
```

---

## ObjectPool<T> (`include/Systems/ObjectPool.h`)
Header-only template. Pre-allocates contiguous storage + free-list.

```
ObjectPool(size_t initialSize = 64)
Acquire() → T*                        ← get object (grows pool if empty)
Release(T*) → bool                    ← return to pool
ReleaseAll()                          ← reset all slots
Reserve(size_t)                       ← pre-grow capacity
GetActiveCount() → size_t             ← pool.size() - available.size()
GetPoolSize() → size_t
```

Used internally by ParticleSystem and can be used for Projectile/Effect pools.

---

## ObservableList<T> (`include/Systems/ObservableList.h`)
Header-only template. Reactive list with auto-notify callbacks.

```
Add(const T&)                         ← triggers OnItemAddedCallback
RemoveAt(size_t index)                ← triggers OnItemRemovedCallback
Remove(const T&)                      ← find and remove, triggers callback
Clear()                               ← triggers OnClearedCallback
At(index) / operator[]                ← access
Size() / IsEmpty() / Contains()
begin() / end()                       ← range-based for loop

Callbacks (std::function):
    OnItemAddedCallback
    OnItemRemovedCallback
    OnClearedCallback
```

---

## Quadtree (`include/Systems/Quadtree.h`)
Spatial partitioning. Each node subdivides at capacity=4.

```
Quadtree() / Quadtree(Rectangle bounds, int capacity = 4)
SetBounds(Rectangle)
Insert(Entity*)
InsertBulk(const vector<Entity*>&)
Query(Rectangle range) → vector<Entity*>    ← O(n log n) spatial query
Clear()
```

---

## CollisionSystem (`include/Systems/CollisionSystem.h`)
High-level collision. Wraps Quadtree internally.

```
CollisionSystem() / CollisionSystem(Rectangle worldBounds)

SetWorldBounds(Rectangle)
GetWorldBounds() → Rectangle

Rebuild(const vector<Entity*>&)              ← rebuild quadtree (call every frame)

CheckCollision(Entity* a, Entity* b) → bool  ← rectangle overlap
GetNearbyEntities(Entity*) → vector<Entity*>  ← expanded-bounds query (3x size)

// O(n log n) pair checking with duplicate avoidance:
ForEachCollision(entities, callback(entity,entity))
    callback = [](Entity* a, Entity* b) { /* handle collision */ }
```

---

## ParticleSystem (`include/Systems/ParticleSystem.h`)
ObjectPool-backed particle effects.

```
ParticleSystem() / ParticleSystem(size_t initialSize)

Update(float dt)                      ← moves particles, removes expired
Render()                              ← draws all active particles as circles

Emit(Vector2 pos, Vector2 vel, Color color, float lifetime, float size)
Emit(Vector2 pos, Vector2 vel, Color startColor, Color endColor,
     float lifetime, float startSize, float endSize)  ← gradient variant
EmitBurst(Vector2 pos, int count, Color color,
          float lifetime, float size, float spread)   ← radial burst

Clear()
GetActiveCount() → size_t
```

Particle struct fields:
```
position, velocity, color, startColor, endColor
lifetime, lifeTimer, size, startSize, endSize, active
```
Used by View to do custom particle rendering if needed.

---

## SoundManager (`include/Systems/SoundManager.h`)
Meyers singleton. Uses raylib audio device.

```
GetInstance() → SoundManager&         ← only way to access

InitAudio() → bool                    ← InitAudioDevice()
CloseAudio() / IsAudioInitialized()

LoadSound(string name, string filepath) → bool    ← stores by name
LoadMusic(string name, string filepath) → bool

PlaySound(name) / StopSound(name) / PauseSound(name) / ResumeSound(name)
IsSoundPlaying(name) → bool

PlayMusic(name) / StopMusic(name) / PauseMusic(name) / ResumeMusic(name)
UpdateMusicStream(name)
IsMusicPlaying(name) → bool

SetSFXVolume(float) / GetSFXVolume()
SetMusicVolume(float) / GetMusicVolume()
UnloadAll()
```

---

## ElementalSystem (`include/Systems/ElementalSystem.h`)
Elemental reactions system.

### Structs
```
StatusEffectInstance { type:StatusEffect, duration:float, timer:float }
    Update(float dt), IsExpired()

DamagePacket { damage:int, damageType:DamageType, statusEffect:StatusEffect, effectDuration:float }
    DamagePacket(damage, type, effect=None, duration=0)

ReactionResult { bonusDamage:int, resultingEffect:StatusEffect, effectDuration:float, reactionName:string }
```

### API
```
ApplyStatusEffect(int entityId, StatusEffect, float duration)
RemoveStatusEffect(int entityId, StatusEffect)
ClearEffects(int entityId)
HasStatusEffect(int entityId, StatusEffect) → bool
GetActiveEffects(int entityId) → const vector<StatusEffectInstance>*

ApplyReaction(int entityId, const DamagePacket&) → ReactionResult
    ← checks existing effects, applies reaction, returns bonus + new status

CheckReaction(StatusEffect existing, const DamagePacket&) → ReactionResult  ← static
    Vaporize:  Fire + Water  = bonus damage (1x-1.2x)
    Conduct:   Wet + Thunder = 1.5x + Shocked status
    Overload:  Shocked+Fire or Burn+Thunder = 1.8x-2x

Update(float dt)                      ← tick status timers, remove expired
CreateDamagePacket(damage, type, effect, duration) → DamagePacket
```

---

## Factories (`include/Factories/`)

### EnemyFactory (all static)
```
CreateEnemy(Vector2 pos, EnemyType type) → unique_ptr<Enemy>
CreateMelee(Vector2 pos)  → hp=50, speed=120, dmg=15, range=40
CreateRanged(Vector2 pos) → hp=30, speed=80, dmg=10, range=300
CreateFlying(Vector2 pos) → hp=40, speed=100, dmg=12, range=40
```

### ItemFactory (all static)
```
CreateItem(Vector2 pos, ItemType type, int amount=1) → unique_ptr<Item>
CreateCoin(pos, amt)  CreateApple(pos, amt)  CreateKey(pos, amt)
CreatePotion(pos, amt)  CreateEquipment(pos, amt)
```

### LevelFactory (all static)
```
LoadLevel(string filepath) → unique_ptr<GameState>       ← text-based level parser
SaveLevel(string filepath, GameState*) → bool            ← serializes mode/player/entities
CreateDefaultLevel(int levelNumber) → unique_ptr<GameState>
LoadDualWorld(string filepath) → unique_ptr<DualWorld>
SaveDualWorld(string filepath, DualWorld*) → bool
```

Save format (text):
```
mode 0          // 0=SinglePlayer, 1=Host, 2=Client
level 1
player 400 300 100 100
entity 2 500 200 44 51 1
```

---

## Network (`include/Network/`)

### Architecture: Radmin VPN (virtual LAN)
- Host chooses Radmin VPN IP (e.g. 26.x.x.x), starts server on port NETWORK_PORT
- Client connects to that IP:port
- Max players: 2 (host + 1 client)
- TCP, custom binary protocol

### Packet (`include/Network/Packet.h`)
Binary serialization buffer.

```
Packet()                              ← empty buffer, readPos=0
Packet(vector<char> data)             ← wrap received data

// Write (appends to buffer):
AppendInt(int32_t)                    ← 4 bytes, little-endian
AppendFloat(float)                    ← 4 bytes
AppendBool(bool)                      ← 1 byte (0/1)
AppendString(const string&)           ← 4-byte length prefix + UTF-8 data

// Read (advances readPos):
ReadInt() → int32_t
ReadFloat() → float
ReadBool() → bool
ReadString() → string

// Buffer access:
GetData() → const char* / char*
GetSize() → size_t
GetReadPos() / SetReadPos(size_t)
IsEmpty() / Clear()
```

Usage pattern:
```cpp
// Sender:
Packet p;
p.AppendInt(MSG_PLAYER_MOVE);
p.AppendFloat(player->GetPosition().x);
p.AppendFloat(player->GetPosition().y);
networkManager.SendToServer(p);

// Receiver:
Packet p;
if (networkManager.ReceiveFromServer(p)) {
    int msgType = p.ReadInt();
    float x = p.ReadFloat();
    float y = p.ReadFloat();
}
```

### Server (`include/Network/Server.h`)
TCP server. Cross-platform (Winsock2 on Windows, POSIX on Linux).

```
Server()
~Server()                             ← calls Stop()

Start(int port, int maxClients=1) → bool   ← bind + listen
Stop()
IsRunning() / GetPort()
GetClientCount()

AcceptClient() → bool                 ← false if maxClients reached
DisconnectClient(int index)
DisconnectAll()

SendToClient(int index, const Packet&) → bool
Broadcast(const Packet&, int excludeClient=-1) → bool
ReceiveFromClient(int index, Packet&) → bool   ← blocking recv
```

### Client (`include/Network/Client.h`)
TCP client. Cross-platform.

```
Client()
~Client()

Connect(string address, int port) → bool     ← connect to server
Disconnect()
IsConnected()
GetServerAddress() / GetPort()

Send(const Packet&) → bool
Receive(Packet&) → bool             ← blocking recv, auto-disconnect on failure
```

### NetworkManager (`include/Network/NetworkManager.h`)
Unified API. Host creates Server, client creates Client.

```
NetworkManager()
~NetworkManager()

StartServer(int port, int maxClients=1) → bool     ← creates internal Server
ConnectToServer(string address, int port) → bool    ← creates internal Client
Disconnect()                                        ← stops both

GetMode() → GameMode                                 ← SinglePlayer / Host / Client
IsHost() / IsConnected()

// Host sends to all connected clients:
SendToAll(const Packet&) → bool
Broadcast(const Packet&, int excludeClient) → bool

// Client sends to server:
SendToServer(const Packet&) → bool
ReceiveFromServer(Packet&) → bool

// Host receives from specific client:
ReceiveFromClient(int index, Packet&) → bool

// Client management (host only):
GetClientCount()
AcceptClients()
DisconnectClient(int index)

// Call every frame (host: accepts pending clients):
Update()
```

### Setup for 2-player over Radmin VPN

**Host (Player 1):**
```cpp
NetworkManager net;
net.StartServer(NETWORK_PORT, 1);  // max 1 client
// After client connects:
// net.AcceptClients() returns true
```

**Client (Player 2):**
```cpp
NetworkManager net;
net.ConnectToServer("26.x.x.x", NETWORK_PORT);  // Radmin VPN IP
// Then send/receive packets
```

---

## Directory Structure

```
include/
├── Model/          ← data + business logic (backend)
│   ├── Entity.h, Character.h, Player.h, Enemy.h, Boss.h, Pet.h
│   ├── Item.h, Projectile.h, Inventory.h
│   ├── Checkpoint.h, Chest.h, FakeWall.h
│   ├── GameState.h, LevelScoring.h
│   └── DualWorld.h, DualWorldPlayer.h, CrossWorldManager.h
├── Systems/        ← cross-cutting game systems (backend)
│   ├── ObjectPool.h, ObservableList.h     (header-only templates)
│   ├── Quadtree.h, CollisionSystem.h
│   ├── ParticleSystem.h, SoundManager.h
│   └── ElementalSystem.h
├── Factories/      ← object creation (backend)
│   ├── EnemyFactory.h, ItemFactory.h, LevelFactory.h
├── Network/        ← TCP multiplayer (backend)
│   ├── Packet.h, Server.h, Client.h, NetworkManager.h
├── Utils/          ← shared types and constants
│   ├── Types.h, Constants.h
├── View/           ← rendering (YOUR CODE)
│   ├── Renderer.h, HUDView.h, InventoryView.h
│   ├── MenuView.h, GameView.h
└── Controller/     ← game loop (YOUR CODE)
    ├── InputController.h
    ├── GameController.h, MenuController.h

src/
├── Model/          ← implementations matching include/Model/
├── Systems/        ← implementations matching include/Systems/
├── Factories/      ← implementations matching include/Factories/
├── Network/        ← implementations matching include/Network/
├── View/           ← YOUR implementations
├── Controller/     ← YOUR implementations
└── main.cpp        ← entry point (YOUR CODE)
```
