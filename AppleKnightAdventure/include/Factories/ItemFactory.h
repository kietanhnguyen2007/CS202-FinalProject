#ifndef ITEMFACTORY_H
#define ITEMFACTORY_H

#include "raylib.h"
#include "Model/Item.h"
#include <memory>

class ItemFactory {
public:
    static std::unique_ptr<Item> CreateItem(Vector2 position, ItemType type, int amount = 1);
    static std::unique_ptr<Item> CreateCoin(Vector2 position, int amount = 1);
    static std::unique_ptr<Item> CreateApple(Vector2 position, int amount = 1);
    static std::unique_ptr<Item> CreateKey(Vector2 position, int amount = 1);
    static std::unique_ptr<Item> CreatePotion(Vector2 position, int amount = 1);
    static std::unique_ptr<Item> CreateEquipment(Vector2 position, int amount = 1);
};

#endif
