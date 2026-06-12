#pragma once

#include "raylib.h"
#include <cstdint>
#include <unordered_map>
#include "Utils/Types.h"

namespace View {

class ElementalFX {
public:
    static ElementalFX& GetInstance();

    void SetElementTint(uint32_t entityId, DamageType type);
    void ClearElementTint(uint32_t entityId);

    // Called by CharacterRenderer when submitting sprite to get tint
    Color GetTintForEntity(uint32_t entityId) const;

private:
    ElementalFX() = default;
    std::unordered_map<uint32_t, DamageType> m_map;
};

} // namespace View
