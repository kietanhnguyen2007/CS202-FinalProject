# CODEGRAPH — Agent Quick Reference

## Include root
All includes use paths relative to `include/`:
```cpp
#include "Model/Entity.h"
#include "Utils/Types.h"
#include "Systems/Quadtree.h"
```

## Class hierarchy
```
Entity                                (include/Model/Entity.h)
├── Character                         (include/Model/Character.h)
│   ├── Player                        (include/Model/Player.h)
│   │   └── DualWorldPlayer           (include/Model/DualWorldPlayer.h)
│   ├── Enemy                         (include/Model/Enemy.h)
│   ├── Boss                          (include/Model/Boss.h)
│   └── Pet                           (include/Model/Pet.h)
├── Item                              (include/Model/Item.h)
├── Projectile                        (include/Model/Projectile.h)
├── Checkpoint                        (include/Model/Checkpoint.h)
├── FakeWall                          (include/Model/FakeWall.h)
└── Chest                             (include/Model/Chest.h)
```

## Standalone classes
- Inventory           (include/Model/Inventory.h)
- GameState           (include/Model/GameState.h)
- LevelScoring        (include/Model/LevelScoring.h)
- DualWorld           (include/Model/DualWorld.h)
- CrossWorldManager   (include/Model/CrossWorldManager.h)

## Key Entity members
```
m_id:int, m_type:EntityType, m_position:Vector2, m_size:Vector2
m_velocity:Vector2, m_rotation:float, m_scale:float, m_active:bool
```

## Key Character (extends Entity)
```
m_health, m_maxHealth:int, m_speed:float, m_direction:Direction
m_attackCooldown, m_attackTimer:float
Move(dir:Vector2, dt:float), TakeDamage(dmg:int), Heal(amt:int)
Attack(), CanAttack(), GetBoundingBox()->Rectangle
GetAttackBoundingBox()->Rectangle  // forward extension by direction
```

## Key Enumerations (include/Utils/Types.h)
| Enum | Values |
|------|--------|
| `EntityType` | Player, DualWorldPlayer, Enemy, Boss, Projectile, Item, Checkpoint, Chest, FakeWall, Pet, Particle, Effect |
| `Direction` | None, Up, Down, Left, Right |
| `EnemyType` | Melee, Ranged, Flying |
| `EnemyState` | Idle, Patrol, Chase, Attack, Hurt, Dead |
| `ItemType` | Coin, Apple, Key, Potion, Equipment |
| `ProjectileType` | Arrow, Magic, BossAttack |
| `PetType` | Skull, Ghost, BabyDragon, Fairy |
| `DamageType` | Physical, Fire, Water, Thunder |
| `StatusEffect` | None, Burn, Wet, Shocked |
| `GameMode` | SinglePlayer, MultiplayerHost, MultiplayerClient |
| `BossPhase` | Phase1, Phase2, Phase3, Enraged |
| `WorldLayer` | Light, Shadow |

## Key Constants (include/Utils/Constants.h)
```
SCREEN_WIDTH=1280, SCREEN_HEIGHT=720, TILE_SIZE=64, GRAVITY=980.0
PLAYER_SPEED=200, PLAYER_JUMP_FORCE=-400, PLAYER_MAX_HEALTH=100
ENEMY_MELEE_SPEED=120, ENEMY_RANGED_SPEED=80, ENEMY_FLYING_SPEED=100
BOSS_MAX_HEALTH=500, BOSS_SPEED=60
PROJECTILE_SPEED=400, PARTICLE_LIFETIME=1.0
INVENTORY_MAX_SLOTS=24, NETWORK_PORT=54000, NETWORK_BUFFER_SIZE=4096
ATTACK_COOLDOWN=0.5, BOSS_ATTACK_COOLDOWN=1.5
PET_FOLLOW_DISTANCE=80, PET_SPEED=180
FAKE_WALL_HEALTH=3, CHEST_MIN_LOOT=1, CHEST_MAX_LOOT=3
```

## Systems

### ObjectPool<T>  (include/Systems/ObjectPool.h, header-only)
```
Acquire()->T*, Release(T*)->bool, ReleaseAll(), Reserve(n)
GetActiveCount()->size_t, GetPoolSize()->size_t
```

### ObservableList<T>  (include/Systems/ObservableList.h, header-only)
```
Add(item), RemoveAt(index), Remove(item), Clear()
Size(), Contains(item), operator[], begin()/end()
OnItemAdded:function, OnItemRemoved:function, OnCleared:function
```

### Quadtree  (include/Systems/Quadtree.h)
```
Insert(Entity*), InsertBulk(vector<Entity*>), Query(Rectangle)->vector<Entity*>
Clear()
```

### CollisionSystem  (include/Systems/CollisionSystem.h)
```
Rebuild(entities), CheckCollision(a,b)->bool
GetNearbyEntities(entity)->vector<Entity*>
ForEachCollision(entities, callback(entity,entity))
```

### ParticleSystem  (include/Systems/ParticleSystem.h)
```
Emit(pos,vel,color,lifetime,size) — single
Emit(pos,vel,startColor,endColor,lifetime,startSize,endSize) — gradient
EmitBurst(pos,count,color,lifetime,size,spread)
Update(dt), Render(), Clear()
```

### SoundManager  (include/Systems/SoundManager.h) — singleton
```
GetInstance()->SoundManager&
LoadSound(name,filepath), PlaySound(name), StopSound(name)...
SetSFXVolume(f), SetMusicVolume(f)
```

### ElementalSystem  (include/Systems/ElementalSystem.h)
```
ApplyStatusEffect(id,effect,duration)
ApplyReaction(id,packet)->ReactionResult
CheckReaction(existing,packet)->ReactionResult
```
Reactions: Vaporize(Fire↔Water,1x), Conduct(Wet+Thunder→Shocked,1.5x), Overload(Shocked+Fire or Burn+Thunder,1.8-2x)

## Factories  (include/Factories/)

### EnemyFactory (static)
```
CreateEnemy(pos, EnemyType)->unique_ptr<Enemy>
CreateMelee(pos)->unique_ptr<Enemy>, CreateRanged(pos)->unique_ptr<Enemy>
CreateFlying(pos)->unique_ptr<Enemy>
```

### ItemFactory (static)
```
CreateItem(pos, ItemType, amount)->unique_ptr<Item>
CreateCoin/Apple/Key/Potion/Equipment(pos, amount)->unique_ptr<Item>
```

### LevelFactory (static)
```
LoadLevel(filepath)->unique_ptr<GameState>
SaveLevel(filepath, GameState*)->bool
CreateDefaultLevel(levelNumber)->unique_ptr<GameState>
LoadDualWorld(filepath)->unique_ptr<DualWorld>
SaveDualWorld(filepath, DualWorld*)->bool
```

## Network  (include/Network/)

### Packet
```
AppendInt(int32_t), AppendFloat(float), AppendBool(bool), AppendString(str)
ReadInt()->int32_t, ReadFloat()->float, ReadBool()->bool, ReadString()->string
GetData()->const char*, GetSize()->size_t, Clear()
```
Custom binary: int (4 bytes LE), float (4 bytes), bool (1 byte), string (4-byte length + data)

### Server
```
Start(port)->bool, Stop(), IsRunning()->bool
AcceptClient()->bool, DisconnectClient(index), DisconnectAll(), GetClientCount()->int
SendToClient(index, packet)->bool, Broadcast(packet, exclude=-1)->bool
ReceiveFromClient(index, packet)->bool
```
Platform: Winsock2 (Windows) / POSIX sockets (Linux). `#ifdef _WIN32` guards.

### Client
```
Connect(address, port)->bool, Disconnect(), IsConnected()->bool
Send(packet)->bool, Receive(packet)->bool
```

### NetworkManager
```
StartServer(port)->bool, ConnectToServer(address, port)->bool, Disconnect()
IsHost()->bool, IsConnected()->bool, GetMode()->GameMode
SendToAll(packet)->bool, SendToServer(packet)->bool
Broadcast(packet, exclude)->bool, ReceiveFromServer(packet)->bool
ReceiveFromClient(index, packet)->bool
AcceptClients()->bool, GetClientCount()->int, DisconnectClient(index)
Update()
```
