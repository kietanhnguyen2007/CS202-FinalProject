#include "Model/Inventory.h"
#include <algorithm>

Inventory::Inventory()
    : m_maxSlots(INVENTORY_MAX_SLOTS)
    , m_coins(0)
    , m_apples(0)
    , m_keys(0)
{
    m_items.reserve(m_maxSlots);
}

Inventory::Inventory(int maxSlots)
    : m_maxSlots(maxSlots)
    , m_coins(0)
    , m_apples(0)
    , m_keys(0)
{
    m_items.reserve(m_maxSlots);
}

bool Inventory::AddItem(std::unique_ptr<Item> item) {
    if (m_items.size() >= static_cast<size_t>(m_maxSlots)) {
        return false;
    }
    m_items.push_back(std::move(item));
    return true;
}

std::unique_ptr<Item> Inventory::RemoveItem(int index) {
    if (index < 0 || index >= static_cast<int>(m_items.size())) {
        return nullptr;
    }
    std::unique_ptr<Item> item = std::move(m_items[index]);
    m_items.erase(m_items.begin() + index);
    return item;
}

Item* Inventory::GetItem(int index) const {
    if (index < 0 || index >= static_cast<int>(m_items.size())) {
        return nullptr;
    }
    return m_items[index].get();
}

int Inventory::GetItemCount() const { return static_cast<int>(m_items.size()); }
int Inventory::GetMaxSlots() const { return m_maxSlots; }
bool Inventory::IsFull() const { return m_items.size() >= static_cast<size_t>(m_maxSlots); }

void Inventory::Clear() {
    m_items.clear();
    m_coins = 0;
    m_apples = 0;
    m_keys = 0;
}

int Inventory::GetCoins() const { return m_coins; }
void Inventory::AddCoins(int amount) { m_coins += std::max(0, amount); }
bool Inventory::SpendCoins(int amount) {
    if (m_coins >= amount) {
        m_coins -= amount;
        return true;
    }
    return false;
}

int Inventory::GetApples() const { return m_apples; }
void Inventory::AddApples(int amount) { m_apples += std::max(0, amount); }
bool Inventory::UseApple() {
    if (m_apples > 0) {
        m_apples--;
        return true;
    }
    return false;
}

int Inventory::GetKeys() const { return m_keys; }
void Inventory::AddKeys(int amount) { m_keys += std::max(0, amount); }
bool Inventory::UseKey() {
    if (m_keys > 0) {
        m_keys--;
        return true;
    }
    return false;
}
