#ifndef CHEST_H
#define CHEST_H

#include "Entity.h"
#include "Item.h"
#include <vector>
#include <memory>

class Chest : public Entity {
protected:
    bool m_opened;
    std::vector<std::unique_ptr<Item>> m_loot;
    bool m_lootGenerated;

public:
    Chest();
    explicit Chest(Vector2 position);

    void Update(float deltaTime) override;
    void Render() override;

    bool IsOpened() const;
    std::vector<std::unique_ptr<Item>> Open();
    void GenerateLoot();
};

#endif
