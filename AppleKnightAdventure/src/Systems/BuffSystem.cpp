#include "Systems/BuffSystem.h"
#include <cstdlib>

const std::vector<BuffDef>& BuffDefs() {
    // magnitude means:
    //   Vigor       fraction of max HP restored
    //   SecondWind  HP restored per second
    //   Focus       unused (clears cooldowns)
    //   Adrenaline  extra cooldown rate (0.5 = cooldowns tick 50% faster)
    //   Haste       extra move speed   (0.35 = +35%)
    //   Power       extra damage dealt (0.30 = +30%)
    //   Aegis       damage taken reduction (0.40 = -40%)
    //   Bloodthirst share of damage dealt returned as health
    static const std::vector<BuffDef> defs = {
        {BuffType::Vigor,       "Vigor",       "Restore 30% health",
         Color{ 86, 220, 120, 255}, 0.0f,  0.30f, 18},
        {BuffType::SecondWind,  "Second Wind", "Regenerate health for 8s",
         Color{120, 230, 170, 255}, 8.0f,  6.0f,  14},
        {BuffType::Focus,       "Focus",       "All skill cooldowns ready",
         Color{120, 200, 255, 255}, 0.0f,  0.0f,  12},
        {BuffType::Adrenaline,  "Adrenaline",  "Cooldowns recover 60% faster",
         Color{ 90, 170, 255, 255}, 15.0f, 0.60f, 12},
        {BuffType::Haste,       "Haste",       "Move 35% faster",
         Color{255, 225, 110, 255}, 12.0f, 0.35f, 14},
        {BuffType::Power,       "Power",       "Deal 30% more damage",
         Color{255, 140,  90, 255}, 12.0f, 0.30f, 14},
        {BuffType::Aegis,       "Aegis",       "Take 40% less damage",
         Color{190, 160, 255, 255}, 10.0f, 0.40f, 12},
        {BuffType::Bloodthirst, "Bloodthirst", "Heal for 15% of damage dealt",
         Color{235,  90, 120, 255}, 12.0f, 0.15f, 10},
        // Infusions. magnitude is unused -- the element is carried by the type
        // itself. Weighted a little lower each so the four of them together do
        // not crowd out the stat boons in a three-card draft.
        {BuffType::InfuseFire,    "Hoa Phu",  "Attacks deal Fire for 14s",
         Color{255, 120,  48, 255}, 14.0f, 0.0f, 8},
        {BuffType::InfuseWater,   "Thuy Phu", "Attacks deal Water for 14s",
         Color{ 90, 180, 255, 255}, 14.0f, 0.0f, 8},
        {BuffType::InfuseThunder, "Loi Phu",  "Attacks deal Thunder for 14s",
         Color{255, 235, 110, 255}, 14.0f, 0.0f, 8},
        {BuffType::InfuseVoid,    "Hu Phu",   "Attacks deal Void for 14s",
         Color{180, 110, 255, 255}, 14.0f, 0.0f, 6},
    };
    return defs;
}

const BuffDef& GetBuffDef(BuffType type) {
    const auto& defs = BuffDefs();
    for (const auto& d : defs) {
        if (d.type == type) return d;
    }
    return defs.front();
}

BuffType RollBuff() {
    const auto& defs = BuffDefs();
    int total = 0;
    for (const auto& d : defs) total += d.weight;
    if (total <= 0) return defs.front().type;

    int roll = rand() % total;
    for (const auto& d : defs) {
        roll -= d.weight;
        if (roll < 0) return d.type;
    }
    return defs.back().type;
}
