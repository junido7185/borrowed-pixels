#pragma once

#include "bp_combat.h"

#include <cstdint>

namespace bp {

inline constexpr std::uint16_t kReclaimRateNumerator = 32;
inline constexpr std::uint16_t kReclaimRateDenominator = 60;
inline constexpr std::int32_t kPlayerReclaimRangePixels = 80;
inline constexpr std::int32_t kOpponentReclaimRangePixels = 72;
inline constexpr std::int32_t kNeutralMergeDistancePixels = 6;
inline constexpr std::int32_t kNeutralMergeRelativeSpeedPixelsPerSecond = 12;
inline constexpr std::uint8_t kReclaimMovementPercent = 75;

struct ReclaimState {
    std::uint16_t accumulator[2];
    std::uint16_t targetIndex[2];
};

struct ReclaimTickResult {
    std::uint16_t playerMass;
    std::uint16_t opponentMass;
    std::uint16_t playerAdapterEscrowMass;
    std::uint16_t opponentAdapterEscrowMass;
    std::uint16_t playerStructureMass;
    std::uint16_t opponentStructureMass;
    std::uint16_t playerTarget;
    std::uint16_t opponentTarget;
};

void InitializeReclaimState(ReclaimState& state) noexcept;
bool TickNeutralDynamics(CombatState& state, const PhysicalLedger& ledger) noexcept;
bool MergeNeutralClusters(CombatState& state, const PhysicalLedger& ledger) noexcept;

std::uint16_t FindReclaimTarget(
    const CombatState& state,
    CombatSide side,
    Vec2Q8 aimDirectionQ15) noexcept;

bool TickReclaim(
    CombatState& state,
    PhysicalLedger& ledger,
    ReclaimState& reclaim,
    bool playerActive,
    bool opponentActive,
    ReclaimTickResult& result,
    bool opponentStructurePriority = false,
    std::uint32_t opponentPlayerMissPriorityStableId = 0) noexcept;

}  // namespace bp
