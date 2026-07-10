#include "Model/Pet.h"
#include "Model/Player.h"
#include "Model/Entity.h"
#include "Systems/SoundManager.h"
#include <cmath>
#include <limits>

// ---------- constructors ----------
Pet::Pet()
    : Character(EntityType::Pet)
    , m_petType(PetType::Skull)
    , m_ownerId(-1)
    , m_followDistance(PET_FOLLOW_DISTANCE)
{
    m_speed = PET_SPEED;
}

Pet::Pet(Vector2 position, PetType type, int ownerId)
    : Character(position, {TILE_SIZE * 0.5f, TILE_SIZE * 0.5f}, EntityType::Pet)
    , m_petType(type)
    , m_ownerId(ownerId)
    , m_followDistance(PET_FOLLOW_DISTANCE)
{
    m_speed = PET_SPEED;
    m_scale = 0.11f; // Render at 1/3 of its previous 1/3 size
    switch (type) {
        case PetType::Skull:      m_damage = 8;  break;
        case PetType::Ghost:      m_damage = 0;  break;
        case PetType::BabyDragon: m_damage = DRAGON_PROJECTILE_DMG; break;
        case PetType::Fairy:      m_damage = 4;  break;
    }
}

// ---------- Update ----------
void Pet::Update(float deltaTime) {
    Character::Update(deltaTime);
}

// ---------- Accessors ----------
PetType  Pet::GetPetType()         const { return m_petType; }
PetState Pet::GetPetState()        const { return m_petState; }
int      Pet::GetOwnerId()         const { return m_ownerId; }
void     Pet::SetOwnerId(int id)         { m_ownerId = id; }
float    Pet::GetFollowDistance()  const { return m_followDistance; }
void     Pet::SetFollowDistance(float d) { m_followDistance = d; }

// Dragon
bool Pet::WantsToFire()    const { return m_wantsToFire; }
void Pet::ResetFireFlag()        { m_wantsToFire = false; }
int  Pet::GetTargetId()    const { return m_targetId; }

// Ghost
int  Pet::GetHealBudget()  const { return m_healBudget; }
void Pet::SetHealBudget(int b)   { m_healBudget = b; }

bool Pet::CanHeal(const Player* player) const {
    if (!player) return false;
    return m_healBudget > 0 && player->GetHealth() < player->GetMaxHealth();
}

void Pet::HealPlayer(Player* player, float dt) {
    if (!player || m_healBudget <= 0) return;

    m_healTickTimer -= dt;
    if (m_healTickTimer > 0.0f) return;
    m_healTickTimer = GHOST_HEAL_TICK;

    int healAmount = static_cast<int>(GHOST_HEAL_RATE * GHOST_HEAL_TICK);
    if (healAmount < 1) healAmount = 1;
    if (healAmount > m_healBudget) healAmount = m_healBudget;

    player->Heal(healAmount);
    SoundManager::GetInstance().PlaySound("pet_ghost_heal");
    m_healBudget -= healAmount;

    if (player->GetHealth() >= player->GetMaxHealth()) {
        m_healBudget = 0;  // Stop once full
    }
}

// ---------- Follow ----------
void Pet::FollowPlayer(Vector2 playerPosition, float deltaTime) {
    m_hoverTime += deltaTime;
    
    // Circular hovering offset
    float hoverRadius = 25.0f;
    float hoverSpeed = 2.0f;
    float offsetX = -60.0f + std::cos(m_hoverTime * hoverSpeed) * hoverRadius;
    float offsetY = -40.0f + std::sin(m_hoverTime * hoverSpeed * 1.5f) * hoverRadius;

    Vector2 targetPos = { playerPosition.x + offsetX, playerPosition.y + offsetY };
    
    float dx = targetPos.x - m_position.x;
    float dy = targetPos.y - m_position.y;
    float dist = std::sqrt(dx * dx + dy * dy);
    
    // Smooth follow with spring-like acceleration
    if (dist > 2.0f) {
        float currentSpeed = m_speed + (dist * 4.0f); 
        Vector2 dir = {dx / dist, dy / dist};
        
        m_position.x += dir.x * currentSpeed * deltaTime;
        m_position.y += dir.y * currentSpeed * deltaTime;
        
        m_direction = (dx > 0) ? Direction::Right : Direction::Left;
    }
    
    // Always stay in following state so it keeps flapping its wings
    m_petState = PetState::Following;
}

// ---------- Main AI ----------
void Pet::UpdateAI(Vector2 playerPosition, float deltaTime,
                   Player* player,
                   const std::vector<Entity*>& enemies,
                   bool inCombat)
{
    // Ghost: if in combat, switch to idle (GameController handles summoning Dragon)
    if (m_petType == PetType::Ghost) {
        if (inCombat) {
            m_petState = PetState::Idle;
            FollowPlayer(playerPosition, deltaTime);
            return;
        }

        if (CanHeal(player)) {
            m_petState = PetState::Healing;
            FollowPlayer(playerPosition, deltaTime);
            HealPlayer(player, deltaTime);
        } else {
            m_petState = PetState::Following;
            FollowPlayer(playerPosition, deltaTime);
        }
        return;
    }

    // Dragon AI
    if (m_petType == PetType::BabyDragon) {
        FollowPlayer(playerPosition, deltaTime);

        // Tick cooldown
        if (m_fireCooldown > 0.0f) {
            m_fireCooldown -= deltaTime;
            if (m_fireCooldown < 0.0f) m_fireCooldown = 0.0f;
        }

        // Charge phase
        if (m_chargeTimer > 0.0f) {
            m_petState = PetState::Charging;
            m_chargeTimer -= deltaTime;
            if (m_chargeTimer <= 0.0f) {
                m_chargeTimer  = 0.0f;
                m_wantsToFire  = true;   // GameController will spawn the projectile
                m_petState     = PetState::Attacking;
                m_fireCooldown = DRAGON_FIRE_COOLDOWN;
            }
            return;
        }

        if (m_petState == PetState::Attacking) {
            // Brief "just fired" state — revert to following
            m_petState = PetState::Following;
        }

        // Find nearest enemy and begin charge
        if (m_fireCooldown <= 0.0f && !enemies.empty()) {
            float nearest = std::numeric_limits<float>::max();
            int   nearestId = -1;

            for (Entity* e : enemies) {
                if (!e || !e->IsActive()) continue;
                float dx = e->GetPosition().x - m_position.x;
                float dy = e->GetPosition().y - m_position.y;
                float d  = std::sqrt(dx * dx + dy * dy);
                if (d < nearest && d < DRAGON_DETECT_RANGE) {
                    nearest   = d;
                    nearestId = e->GetId();
                }
            }

            if (nearestId >= 0) {
                m_targetId    = nearestId;
                m_chargeTimer = DRAGON_CHARGE_TIME;
                m_petState    = PetState::Charging;
            } else {
                m_petState = PetState::Following;
            }
        } else {
            m_petState = PetState::Following;
        }
    }
}
