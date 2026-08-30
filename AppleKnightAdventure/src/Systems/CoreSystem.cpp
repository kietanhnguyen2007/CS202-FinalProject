#include "Systems/CoreSystem.h"
#include <algorithm>
#include <cstdlib>

namespace {

// Roll a rarity for one draft slot. Mirrors the Survival3D weighting: ordinary
// draws are mostly Common, a boss draw is guaranteed to start Epic or better.
CoreRarity RollRarity(int slot, bool bossReward, bool pityDue) {
    const float roll = static_cast<float>(rand() % 1000) / 1000.0f;
    if (bossReward) {
        if (slot == 0) return roll < 0.18f ? CoreRarity::Legendary : CoreRarity::Epic;
        if (roll < 0.10f) return CoreRarity::Legendary;
        if (roll < 0.42f) return CoreRarity::Epic;
        return CoreRarity::Rare;
    }
    if (slot == 0 && pityDue) return CoreRarity::Epic;
    if (roll < 0.62f) return CoreRarity::Common;
    if (roll < 0.90f) return CoreRarity::Rare;
    if (roll < 0.99f) return CoreRarity::Epic;
    return CoreRarity::Legendary;
}

} // namespace

const std::vector<CoreDefinition>& CoreDefs() {
    // magnitude means:
    //   VitalCore        flat max HP per stack
    //   SwiftStep        extra move speed per stack
    //   TemperedGuard    damage reduction per stack
    //   QuickHands       extra cooldown rate per stack
    //   RiftEssence      damage AND max HP fraction per stack
    //   RiftPower        extra damage per stack
    //   Execution        bonus damage against wounded targets
    //   EmberEdge        chance to apply Burn on hit
    //   GravityLens      extra pierce per stack
    //   ChainSpark       share of damage each arc carries
    //   ElementalMastery extra reaction damage
    //   SecondWind       fraction of max HP restored on revive
    //   GlassRift        extra damage (max HP cost is hardcoded alongside it)
    static const std::vector<CoreDefinition> defs = {
        {CoreId::VitalCore, "vital_core", "LOI SINH MENH",
         "+12 mau toi da, hoi 12 mau", CoreRarity::Common, 8, -1, 12.0f},
        {CoreId::SwiftStep, "swift_step", "LOI PHONG TUC",
         "+4% toc do di chuyen", CoreRarity::Common, 5, -1, 0.04f},
        {CoreId::TemperedGuard, "tempered_guard", "LOI CUONG GIAP",
         "Nhan it hon 4% sat thuong", CoreRarity::Common, 5, -1, 0.04f},
        {CoreId::QuickHands, "quick_hands", "LOI TOC THU",
         "Hoi chieu nhanh hon 4%", CoreRarity::Common, 6, -1, 0.04f},
        {CoreId::RiftEssence, "rift_essence", "LOI HU TINH",
         "+3% sat thuong va +3% mau toi da", CoreRarity::Common, 99, -1, 0.03f},

        {CoreId::RiftPower, "rift_power", "LOI HU LUC",
         "+18% sat thuong moi nguon", CoreRarity::Rare, 5, -1, 0.18f},
        {CoreId::Execution, "execution", "LOI TRUY KICH",
         "+20% sat thuong len muc tieu duoi 25% mau", CoreRarity::Rare, 1, -1, 0.20f},
        {CoreId::EmberEdge, "ember_edge", "LOI HOA NHAN",
         "20% don danh gay Thieu Dot", CoreRarity::Rare, 1, -1, 0.20f},
        {CoreId::GravityLens, "gravity_lens", "LOI TRONG THAU",
         "Dan xuyen them mot muc tieu", CoreRarity::Rare, 2, -1, 1.0f},

        {CoreId::ChainSpark, "chain_spark", "LOI LOI CHUYEN",
         "Moi don thu 6 lan sang quai xung quanh", CoreRarity::Epic, 1, -1, 0.60f},
        {CoreId::ElementalMastery, "elemental_mastery", "LOI NGUYEN TO",
         "+35% sat thuong phan ung nguyen to", CoreRarity::Epic, 1, -1, 0.35f},

        {CoreId::SecondWind, "second_wind", "LOI HOI SINH",
         "Hoi sinh mot lan voi 35% mau", CoreRarity::Legendary, 1, -1, 0.35f},
        {CoreId::GlassRift, "glass_rift", "LOI THUY TINH",
         "+45% sat thuong nhung -25% mau toi da", CoreRarity::Legendary, 1, -1, 0.45f},

        // ---- Class-locked ----
        // These never appear for another class, so each one can lean on that
        // class's own kit instead of being a generic stat bump.
        {CoreId::IronLunge, "iron_lunge", "LOI THIET XUNG",
         "Lao (K) +50% sat thuong va lao xa hon", CoreRarity::Epic, 1,
         static_cast<int>(CoreClassLock::Knight), 0.50f},
        {CoreId::BulwarkOath, "bulwark_oath", "LOI THE UOC THUAN",
         "Do don (P) chan hoan toan sat thuong", CoreRarity::Legendary, 1,
         static_cast<int>(CoreClassLock::Knight), 1.0f},

        {CoreId::Berserker, "berserker", "LOI CUONG NO",
         "+30% sat thuong khi duoi 50% mau", CoreRarity::Epic, 1,
         static_cast<int>(CoreClassLock::Fighter), 0.30f},
        {CoreId::OverchargedOrb, "overcharged_orb", "LOI CAU QUA TAI",
         "Ultimate (H) +60% sat thuong", CoreRarity::Epic, 1,
         static_cast<int>(CoreClassLock::Fighter), 0.60f},

        {CoreId::ShadowStep, "shadow_step", "LOI ANH BO",
         "Dich chuyen (U) giam 40% hoi chieu", CoreRarity::Rare, 1,
         static_cast<int>(CoreClassLock::Ninja), 0.40f},
        {CoreId::TwinBlades, "twin_blades", "LOI SONG NHAN",
         "Blade Rush (K) ban them mot luoi thu hai", CoreRarity::Epic, 1,
         static_cast<int>(CoreClassLock::Ninja), 1.0f},

        {CoreId::LingeringAura, "lingering_aura", "LOI TRUONG TON",
         "Aura nguyen to keo dai them 50%", CoreRarity::Rare, 1,
         static_cast<int>(CoreClassLock::MagicCaster), 0.50f},
        {CoreId::RiftConflux, "rift_conflux", "LOI CONG HUONG",
         "Phan ung lan xa gap doi va manh gap doi", CoreRarity::Legendary, 1,
         static_cast<int>(CoreClassLock::MagicCaster), 2.0f},
    };
    return defs;
}

const CoreDefinition& GetCoreDef(CoreId id) {
    const auto& defs = CoreDefs();
    for (const auto& d : defs) {
        if (d.id == id) return d;
    }
    return defs.front();
}

Color RarityColor(CoreRarity rarity) {
    switch (rarity) {
        case CoreRarity::Common:    return Color{185, 190, 205, 255};
        case CoreRarity::Rare:      return Color{ 95, 175, 255, 255};
        case CoreRarity::Epic:      return Color{190, 120, 255, 255};
        case CoreRarity::Legendary: return Color{255, 190,  75, 255};
        default:                    return WHITE;
    }
}

const char* RarityName(CoreRarity rarity) {
    switch (rarity) {
        case CoreRarity::Common:    return "THUONG";
        case CoreRarity::Rare:      return "HIEM";
        case CoreRarity::Epic:      return "SU THI";
        case CoreRarity::Legendary: return "HUYEN THOAI";
        default:                    return "";
    }
}

void CoreLoadout::Clear() {
    m_stacks.fill(0);
    m_owned.clear();
    m_reviveUsed = false;
    m_hitCounter = 0;
    m_draftsWithoutEpic = 0;
}

void CoreLoadout::Add(CoreId id) {
    const CoreDefinition& def = GetCoreDef(id);
    int& stack = m_stacks[static_cast<std::size_t>(id)];
    if (stack >= def.maxStacks) return;
    if (stack == 0) m_owned.push_back(id);
    ++stack;
}

int CoreLoadout::GetStack(CoreId id) const {
    return m_stacks[static_cast<std::size_t>(id)];
}

bool CoreLoadout::IsMaxed(CoreId id) const {
    return GetStack(id) >= GetCoreDef(id).maxStacks;
}

float CoreLoadout::DamageMultiplier() const {
    float mult = 1.0f;
    mult += GetStack(CoreId::RiftPower)   * GetCoreDef(CoreId::RiftPower).magnitude;
    mult += GetStack(CoreId::RiftEssence) * GetCoreDef(CoreId::RiftEssence).magnitude;
    if (Has(CoreId::GlassRift)) mult += GetCoreDef(CoreId::GlassRift).magnitude;
    return mult;
}

float CoreLoadout::DamageTakenMultiplier() const {
    const float reduction =
        GetStack(CoreId::TemperedGuard) * GetCoreDef(CoreId::TemperedGuard).magnitude;
    // Floored so stacking guard can never make the player untouchable.
    return std::max(0.35f, 1.0f - reduction);
}

float CoreLoadout::SpeedMultiplier() const {
    return 1.0f + GetStack(CoreId::SwiftStep) * GetCoreDef(CoreId::SwiftStep).magnitude;
}

float CoreLoadout::CooldownRateMultiplier() const {
    return 1.0f + GetStack(CoreId::QuickHands) * GetCoreDef(CoreId::QuickHands).magnitude;
}

int CoreLoadout::BonusMaxHealthFlat() const {
    return static_cast<int>(GetStack(CoreId::VitalCore)
                            * GetCoreDef(CoreId::VitalCore).magnitude);
}

float CoreLoadout::MaxHealthMultiplier() const {
    float mult = 1.0f + GetStack(CoreId::RiftEssence)
                            * GetCoreDef(CoreId::RiftEssence).magnitude;
    if (Has(CoreId::GlassRift)) mult -= 0.25f;
    return std::max(0.30f, mult);
}

float CoreLoadout::ExecutionMultiplier() const {
    return Has(CoreId::Execution)
        ? 1.0f + GetCoreDef(CoreId::Execution).magnitude : 1.0f;
}

float CoreLoadout::ExecutionHealthThreshold() const {
    return 0.25f;
}

float CoreLoadout::EmberEdgeChance() const {
    return Has(CoreId::EmberEdge) ? GetCoreDef(CoreId::EmberEdge).magnitude : 0.0f;
}

int CoreLoadout::ProjectilePierceBonus() const {
    return static_cast<int>(GetStack(CoreId::GravityLens)
                            * GetCoreDef(CoreId::GravityLens).magnitude);
}

float CoreLoadout::ReactionMultiplier() const {
    return Has(CoreId::ElementalMastery)
        ? 1.0f + GetCoreDef(CoreId::ElementalMastery).magnitude : 1.0f;
}

float CoreLoadout::WoundedDamageMultiplier(float healthFraction) const {
    if (!Has(CoreId::Berserker) || healthFraction > 0.5f) return 1.0f;
    return 1.0f + GetCoreDef(CoreId::Berserker).magnitude;
}

float CoreLoadout::AuraDurationMultiplier() const {
    return Has(CoreId::LingeringAura)
        ? 1.0f + GetCoreDef(CoreId::LingeringAura).magnitude : 1.0f;
}

float CoreLoadout::SplashScale() const {
    return Has(CoreId::RiftConflux) ? GetCoreDef(CoreId::RiftConflux).magnitude : 1.0f;
}

float CoreLoadout::ReviveHealthFraction() const {
    return GetCoreDef(CoreId::SecondWind).magnitude;
}

bool CoreLoadout::RegisterHitAndCheckChain() {
    if (!Has(CoreId::ChainSpark)) return false;
    ++m_hitCounter;
    if (m_hitCounter < CHAIN_SPARK_PERIOD) return false;
    m_hitCounter = 0;
    return true;
}

std::vector<CoreId> CoreLoadout::RollDraft(int count, bool bossReward, int classLock) {
    std::vector<CoreId> picked;
    if (count <= 0) return picked;
    picked.reserve(count);

    const auto& defs = CoreDefs();
    const bool pityDue = m_draftsWithoutEpic >= 7;
    bool offeredEpic = false;

    auto alreadyPicked = [&](CoreId id) {
        return std::find(picked.begin(), picked.end(), id) != picked.end();
    };
    auto eligible = [&](const CoreDefinition& d) {
        if (d.classLock >= 0 && d.classLock != classLock) return false;
        if (IsMaxed(d.id)) return false;
        return !alreadyPicked(d.id);
    };

    for (int slot = 0; slot < count; ++slot) {
        const CoreRarity desired = RollRarity(slot, bossReward, pityDue);

        std::vector<const CoreDefinition*> candidates;
        for (const auto& d : defs) {
            if (d.rarity == desired && eligible(d)) candidates.push_back(&d);
        }
        // That rarity can be exhausted by max stacks. Widen to anything still
        // legal rather than dropping a card from the draft.
        if (candidates.empty()) {
            for (const auto& d : defs) {
                if (eligible(d)) candidates.push_back(&d);
            }
        }
        if (candidates.empty()) break;   // everything is maxed out

        const CoreDefinition* chosen =
            candidates[static_cast<size_t>(rand()) % candidates.size()];
        picked.push_back(chosen->id);
        offeredEpic |= chosen->rarity == CoreRarity::Epic
                    || chosen->rarity == CoreRarity::Legendary;
    }

    m_draftsWithoutEpic = offeredEpic ? 0 : m_draftsWithoutEpic + 1;
    return picked;
}
