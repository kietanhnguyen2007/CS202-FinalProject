#pragma once

#include "Survival3D/Vfx/VfxPackage.h"

#include <array>
#include <cstddef>

namespace Survival3D::Vfx {

enum class SkillPackage : std::size_t {
    KnightVioletEdge,
    KnightAegisCounter,
    KnightShieldRush,
    KnightBastionBreaker,
    KnightSteelStep,
    MageArcBolt,
    MageFrostRing,
    MageGravityWell,
    MageAstralTempest,
    MagePhaseBlink,
    Count
};

constexpr std::size_t kSkillPackageCount =
    static_cast<std::size_t>(SkillPackage::Count);

const std::array<Package, kSkillPackageCount>& SkillPackages() noexcept;
const Package& GetSkillPackage(SkillPackage package) noexcept;

} // namespace Survival3D::Vfx
