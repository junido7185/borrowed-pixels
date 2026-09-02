#pragma once

#include "bp_types.h"

#include <cstdint>

namespace bp {

inline constexpr std::uint8_t kHudModuleCount = 3;
inline constexpr std::uint16_t kHudDismantleTicks = 45;
inline constexpr std::uint16_t kHudRestoreTicks = 60;
inline constexpr std::uint16_t kHudTransitTicks = 36;
inline constexpr std::uint16_t kHudMaturityTicks = 720;
inline constexpr std::uint16_t kHudWarningTicks = 120;
inline constexpr std::uint16_t kHudReallocationLockTicks = 90;

enum class HudModuleId : std::uint8_t {
    Vital = 0,
    Vector,
    Impulse,
    Count,
};

enum class HudLoanState : std::uint8_t {
    Installed = 0,
    Dismantling,
    ToShellTransit,
    Inactive,
    Restoring,
    Due,
    ToHudTransit,
    Count,
};

struct HudModuleLoan {
    std::uint16_t mass;
    std::uint16_t channelTicks;
    std::uint16_t stateTicks;
    std::uint16_t maturityRemaining;
    std::uint16_t transitMass;
    std::uint16_t arrivedMass;
    HudLoanState state;
    std::uint8_t warning;
};

struct HudLoanSystem {
    HudModuleLoan modules[kHudModuleCount];
    HudModuleId selected;
    std::uint16_t reallocationLockTicks;
};

std::uint16_t HudModuleMass(HudModuleId module) noexcept;
std::uint8_t HudModuleSelectNumber(HudModuleId module) noexcept;
void InitializeHudLoanSystem(HudLoanSystem& system) noexcept;
bool HudLoanSystemValid(const HudLoanSystem& system, const PhysicalLedger& ledger) noexcept;
bool SelectHudModule(HudLoanSystem& system, HudModuleId module) noexcept;

bool TickHudLoan(
    HudLoanSystem& system,
    PhysicalLedger& ledger,
    bool reallocateHeld,
    bool playerShellHitThisTick,
    std::uint16_t playerReclaimedMassThisTick) noexcept;

bool ShortenOpenHudMaturity(HudLoanSystem& system, std::uint16_t ticks) noexcept;
bool HudModuleFullyInstalled(const HudLoanSystem& system, HudModuleId module) noexcept;
bool HudFireLocked(const HudLoanSystem& system) noexcept;
bool HudLoanWarning(const HudLoanSystem& system) noexcept;
std::uint16_t HudVisibleMaterialMass(const HudLoanSystem& system, HudModuleId module) noexcept;
std::uint8_t OpenHudLoanCount(const HudLoanSystem& system) noexcept;

}  // namespace bp
