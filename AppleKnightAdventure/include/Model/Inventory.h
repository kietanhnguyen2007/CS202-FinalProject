#ifndef INVENTORY_H
#define INVENTORY_H

#include "Item.h"
#include "../Utils/Constants.h"
#include <vector>
#include <memory>

class Inventory {
protected:
    std::vector<std::unique_ptr<Item>> m_items;
    int m_maxSlots;
    int m_coins;
    int m_apples;
    int m_keys;

public:
    Inventory();
    explicit Inventory(int maxSlots);

    bool AddItem(std::unique_ptr<Item> item);
    std::unique_ptr<Item> RemoveItem(int index);
    Item* GetItem(int index) const;
    int GetItemCount() const;
    int GetMaxSlots() const;
    bool IsFull() const;
    void Clear();

    int GetCoins() const;
    void AddCoins(int amount);
    bool SpendCoins(int amount);

    int GetApples() const;
    void AddApples(int amount);
    bool UseApple();

    int GetKeys() const;
    void AddKeys(int amount);
    bool UseKey();
};

#endif
