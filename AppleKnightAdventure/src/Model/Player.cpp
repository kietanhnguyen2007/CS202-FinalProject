#include "Model/Player.h"
#include "Systems/Renderer.h"
#include "raylib.h"

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
    // Smoke test: if renderer initialized, submit a small procedural sprite.
    static bool s_texInit = false;
    static Texture2D s_texture = {0};
    if (!s_texInit) {
        Image img = GenImageChecked(32, 32, 8, 8, RED, BLUE);
        s_texture = LoadTextureFromImage(img);
        UnloadImage(img);
        s_texInit = true;
    }

    Systems::Renderer& r = Systems::Renderer::GetInstance();
    Rectangle src = {0,0,(float)s_texture.width,(float)s_texture.height};
    if (r.IsInitialized()) {
        r.SubmitSprite(&s_texture, src, m_position, {1.0f,1.0f}, 0.0f, {(float)s_texture.width*0.5f,(float)s_texture.height*0.5f}, WHITE, Systems::Layer::World, 0.0f, false, 0);
    } else {
        DrawTexture(s_texture, (int)(m_position.x - s_texture.width/2), (int)(m_position.y - s_texture.height/2), WHITE);
    }
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
