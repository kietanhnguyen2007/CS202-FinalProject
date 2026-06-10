#include "../../include/Model/Chest.h"
#include "../../include/Model/Item.h"
#include "../../include/Utils/Constants.h"
#include <cstdlib>

Chest::Chest()
    : Entity(EntityType::Chest)
    , m_opened(false)
    , m_lootGenerated(false)
{
}

Chest::Chest(Vector2 position)
    : Entity(position, {TILE_SIZE * 0.8f, TILE_SIZE * 0.6f}, EntityType::Chest)
    , m_opened(false)
    , m_lootGenerated(false)
{
}

void Chest::Update(float deltaTime) {
}

void Chest::Render() {
}

bool Chest::IsOpened() const { return m_opened; }

std::vector<std::unique_ptr<Item>> Chest::Open() {
    if (!m_opened) {
        m_opened = true;
        if (!m_lootGenerated) {
            GenerateLoot();
        }
    }
    std::vector<std::unique_ptr<Item>> loot = std::move(m_loot);
    m_loot.clear();
    return loot;
}

void Chest::GenerateLoot() {
    if (m_lootGenerated) return;
    m_lootGenerated = true;
    int lootCount = CHEST_MIN_LOOT + std::rand() % (CHEST_MAX_LOOT - CHEST_MIN_LOOT + 1);
    for (int i = 0; i < lootCount; ++i) {
        Vector2 lootPos = {m_position.x + i * 20.0f, m_position.y - TILE_SIZE * 0.5f};
        int roll = std::rand() % 100;
        if (roll < 40) {
            m_loot.push_back(std::make_unique<Item>(lootPos, ItemType::Coin, 1 + std::rand() % 5));
        } else if (roll < 65) {
            m_loot.push_back(std::make_unique<Item>(lootPos, ItemType::Apple, 1));
        } else if (roll < 80) {
            m_loot.push_back(std::make_unique<Item>(lootPos, ItemType::Key, 1));
        } else if (roll < 95) {
            m_loot.push_back(std::make_unique<Item>(lootPos, ItemType::Potion, 1));
        } else {
            m_loot.push_back(std::make_unique<Item>(lootPos, ItemType::Equipment, 1));
        }
    }
}
