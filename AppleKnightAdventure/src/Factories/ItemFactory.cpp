#include "Factories/ItemFactory.h"

std::unique_ptr<Item> ItemFactory::CreateItem(Vector2 position, ItemType type, int amount) {
    switch (type) {
        case ItemType::Coin: return CreateCoin(position, amount);
        case ItemType::Apple: return CreateApple(position, amount);
        case ItemType::Key: return CreateKey(position, amount);
        case ItemType::Potion: return CreatePotion(position, amount);
        case ItemType::Equipment: return CreateEquipment(position, amount);
    }
    return CreateCoin(position, amount);
}

std::unique_ptr<Item> ItemFactory::CreateCoin(Vector2 position, int amount) {
    return std::make_unique<Item>(position, ItemType::Coin, amount);
}

std::unique_ptr<Item> ItemFactory::CreateApple(Vector2 position, int amount) {
    return std::make_unique<Item>(position, ItemType::Apple, amount);
}

std::unique_ptr<Item> ItemFactory::CreateKey(Vector2 position, int amount) {
    return std::make_unique<Item>(position, ItemType::Key, amount);
}

std::unique_ptr<Item> ItemFactory::CreatePotion(Vector2 position, int amount) {
    return std::make_unique<Item>(position, ItemType::Potion, amount);
}

std::unique_ptr<Item> ItemFactory::CreateEquipment(Vector2 position, int amount) {
    return std::make_unique<Item>(position, ItemType::Equipment, amount);
}
