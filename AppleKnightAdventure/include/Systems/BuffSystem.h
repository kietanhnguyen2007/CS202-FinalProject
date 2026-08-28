#ifndef BUFFSYSTEM_H
#define BUFFSYSTEM_H

#include "raylib.h"
#include "Utils/Types.h"
#include <string>
#include <vector>

// Static description of one boon: what it is called, how it reads on screen,
// how long it lasts and how strong it is. Instant boons have duration 0.
struct BuffDef {
    BuffType    type;
    const char* name;
    const char* description;
    Color       color;
    float       duration;   // 0 = applied once on pickup
    float       magnitude;  // meaning depends on the type, see BuffDefs()
    int         weight;     // relative chance of being offered
};

const std::vector<BuffDef>& BuffDefs();
const BuffDef& GetBuffDef(BuffType type);
// Picks a boon using the weights above.
BuffType RollBuff();

// One boon currently running on a player.
struct ActiveBuff {
    BuffType type;
    float    timer;
    float    duration;
    float    tickAccumulator = 0.0f;  // used by regeneration
};

#endif
