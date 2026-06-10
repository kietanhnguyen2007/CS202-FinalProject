#include "Model/Player.h"

Player::Player()
    : Character(EntityType::Player)
    , m_score(0)
    , m_skillPoints(0)
{
    m_speed = PLAYER_SPEED;
    m_maxHealth = PLAYER_MAX_HEALTH;
    m_health = m_maxHealth;
}

Player::Player(Vector2 position)
    : Character(position, {TILE_SIZE * 0.8f, TILE_SIZE * 0.9f}, EntityType::Player)
    , m_score(0)
    , m_skillPoints(0)
{
    m_speed = PLAYER_SPEED;
    m_maxHealth = PLAYER_MAX_HEALTH;
    m_health = m_maxHealth;
}

void Player::Update(float deltaTime) {
    Character::Update(deltaTime);
}

void Player::Render() {
}

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
