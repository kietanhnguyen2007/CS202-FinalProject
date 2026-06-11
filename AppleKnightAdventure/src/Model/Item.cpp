#include "Model/Item.h"
#include "Systems/Renderer.h"
#include "Utils/Constants.h"

Item::Item()
    : Entity(EntityType::Item)
    , m_itemType(ItemType::Coin)
    , m_amount(1)
{
}

Item::Item(Vector2 position, ItemType type, int amount)
    : Entity(position, {TILE_SIZE * 0.5f, TILE_SIZE * 0.5f}, EntityType::Item)
    , m_itemType(type)
    , m_amount(amount)
{
    switch (type) {
        case ItemType::Coin: m_name = "Coin"; break;
        case ItemType::Apple: m_name = "Apple"; break;
        case ItemType::Key: m_name = "Key"; break;
        case ItemType::Potion: m_name = "Potion"; break;
        case ItemType::Equipment: m_name = "Equipment"; break;
    }
}

void Item::Update(float deltaTime) {
}

void Item::Render() {
    SubmitRender();
}

ItemType Item::GetItemType() const { return m_itemType; }
int Item::GetAmount() const { return m_amount; }
void Item::SetAmount(int amount) { m_amount = amount; }
const std::string& Item::GetItemName() const { return m_name; }
