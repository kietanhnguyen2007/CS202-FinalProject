#include "View/ElementalFX.h"

namespace View {

ElementalFX& ElementalFX::GetInstance() {
    static ElementalFX inst;
    return inst;
}

void ElementalFX::SetElementTint(uint32_t entityId, DamageType type) {
    m_map[entityId] = type;
}

void ElementalFX::ClearElementTint(uint32_t entityId) {
    m_map.erase(entityId);
}

Color ElementalFX::GetTintForEntity(uint32_t entityId) const {
    auto it = m_map.find(entityId);
    if (it == m_map.end()) return WHITE;
    switch (it->second) {
        case DamageType::Fire: return (Color){255,160,64,255};
        case DamageType::Water: return (Color){128,200,255,255};
        case DamageType::Thunder: return (Color){255,255,128,255};
        default: return WHITE;
    }
}

} // namespace View
