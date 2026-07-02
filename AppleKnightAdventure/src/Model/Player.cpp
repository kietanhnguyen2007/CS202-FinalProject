#include "Model/Player.h"
#include "raylib.h"
#include <cmath>

Player::Player()
    : Character(EntityType::Player)
    , m_score(0)
    , m_skillPoints(0)
    , m_knightSkills(std::make_unique<KnightSkillSet>())
{
    m_speed = PLAYER_SPEED;
    m_maxHealth = PLAYER_MAX_HEALTH;
    m_health = m_maxHealth;
}

Player::Player(Vector2 position)
    : Character(position, {TILE_SIZE * 0.5f, TILE_SIZE * 0.9f}, EntityType::Player)
    , m_score(0)
    , m_skillPoints(0)
    , m_knightSkills(std::make_unique<KnightSkillSet>())
{
    m_speed = PLAYER_SPEED;
    m_maxHealth = PLAYER_MAX_HEALTH;
    m_health = m_maxHealth;
    m_direction = Direction::Right;
}

void Player::Update(float deltaTime) {
    Character::Update(deltaTime);

    // Tick knight skills
    if (m_knightSkills) m_knightSkills->Update(deltaTime);

    // Tick dash cooldown
    if (m_dashCooldown > 0.0f) {
        m_dashCooldown -= deltaTime;
        if (m_dashCooldown < 0.0f) m_dashCooldown = 0.0f;
    }

    // Tick active dash
    if (m_isDashing) {
        m_dashTimer -= deltaTime;
        if (m_dashTimer <= 0.0f) {
            m_isDashing    = false;
            m_isInvincible = false;
            m_dashTimer    = 0.0f;
            // Kill horizontal momentum from lunge
            if (m_dashMoving) {
                Vector2 vel = m_velocity;
                vel.x = 0.0f;
                m_velocity = vel;
            }
        } else if (m_dashMoving) {
            // Keep lunge velocity alive
            Vector2 vel = m_velocity;
            vel.x = m_dashDirX * DASH_SPEED;
            m_velocity = vel;
        }
    }

    // State machine
    if (!IsAlive()) {
        m_state = State::Dead;
    } else if (m_isDashing) {
        m_state = State::Jump;  // Use 'Jump' state during dash to avoid triggering attack animation
    } else if (m_attackTimer > 0.0f) {
        if (m_state != State::Attack2 && m_state != State::Attack3) {
            m_state = State::Attack;
        }
    } else if (m_velocity.y < 0.0f) {
        m_state = State::Jump;
    } else if (m_velocity.y > 0.0f) {
        m_state = State::Fall;
    } else if (std::abs(m_velocity.x) > 1.0f) {
        m_state = State::Walk;
    } else {
        m_state = State::Idle;
    }
}

void Player::Attack() {
    m_attackTimer = m_attackCooldown;
    m_state = State::Attack;
}

void Player::Attack2() {
    m_attackTimer = m_attackCooldown;
    m_state = State::Attack2;
}

void Player::Attack3() {
    m_attackTimer = m_attackCooldown;
    m_state = State::Attack3;
}

// --- Sprint ---
bool Player::IsSprinting() const { return m_isSprinting; }
void Player::SetSprinting(bool sprinting) { m_isSprinting = sprinting; }

// --- Dash ---
bool Player::IsDashing() const    { return m_isDashing; }
bool Player::IsInvincible() const { return m_isInvincible; }
bool Player::CanDash() const      { return !m_isDashing && m_dashCooldown <= 0.0f; }

void Player::StartDash(bool isMoving, float dirX) {
    m_isDashing    = true;
    m_isInvincible = true;
    m_dashMoving   = isMoving;
    m_dashDirX     = dirX;
    m_dashTimer    = DASH_DURATION;
    m_dashCooldown = DASH_COOLDOWN_MAX;
}

// --- Inventory & Score ---
Inventory& Player::GetInventory() { return m_inventory; }
const Inventory& Player::GetInventory() const { return m_inventory; }

int Player::GetScore() const { return m_score; }
void Player::AddScore(int amount) { m_score += amount; }
void Player::SetScore(int score) { m_score = score; }

int Player::GetSkillPoints() const { return m_skillPoints; }
void Player::SetSkillPoints(int points) { m_skillPoints = points; }
void Player::AddSkillPoints(int amount) { m_skillPoints += amount; }

const std::string& Player::GetName() const { return m_name; }
void Player::SetName(const std::string& name) { m_name = name; }
