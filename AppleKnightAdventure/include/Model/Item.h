#ifndef ITEM_H
#define ITEM_H

#include "Entity.h"
#include "Utils/Types.h"
#include <string>

class Item : public Entity {
protected:
    ItemType m_itemType;
    int m_amount;
    std::string m_name;

public:
    Item();
    Item(Vector2 position, ItemType type, int amount = 1);

    void Update(float deltaTime) override;

    ItemType GetItemType() const;
    int GetAmount() const;
    void SetAmount(int amount);
    const std::string& GetItemName() const;
};

#endif
