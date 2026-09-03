#pragma once

#include "bp_input.h"
#include "bp_math.h"
#include "bp_types.h"

#include <cstdint>

namespace bp {

inline constexpr RectPixels kPlayInterior{12, 18, 296, 148};
inline constexpr std::uint16_t kDashCooldownTicks = 144;
inline constexpr std::uint8_t kDashDurationTicks = 8;
inline constexpr std::uint8_t kDashInvulnerableTicks = 5;
inline constexpr std::int32_t kDashStepQ8 = 20 * kQ8One / kDashDurationTicks;

struct GameState {
    ClaimLedger claimLedger;
    PhysicalLedger physicalLedger;
    NarrativeState narrative;
    InputArbiter inputArbiter;
    Vec2Q8 playerPosition;
    Vec2Q8 playerVelocity;
    Vec2Q8 dashDirection;
    Vec2Q8 lastMoveDirection;
    std::uint32_t encounterSeed;
    std::uint32_t globalTick;
    std::uint16_t dashCooldown;
    std::uint8_t dashTicksRemaining;
    std::uint8_t dashInvulnerableRemaining;
    GameplayAction currentAction;
    std::uint8_t paused;
    std::uint8_t impulseInstalled;
    std::uint8_t chargeActive;
    std::uint8_t reclaimActive;
    std::uint8_t hudChannelActive;
};

bool InitializeGameState(
    GameState& state,
    std::uint32_t encounterSeed,
    std::uint32_t choiceBits = 0) noexcept;

bool TickGame(GameState& state, const InputFrame& input) noexcept;
// CROSS_TICKET_GLUE: encounter orchestration resolves input at authority step
// 06 and advances actor motion at steps 09-10 without duplicating T04 rules.
bool AdvanceGameIntent(GameState& state, const GameplayIntent& intent) noexcept;
std::uint32_t GameStateHash(const GameState& state) noexcept;
std::int32_t PlayerMaximumSpeedQ8(std::uint16_t shellMass) noexcept;
// CROSS_TICKET_GLUE: T07 large opponents use the frozen section 32.3 curve.
std::int32_t LargeOpponentMaximumSpeedQ8(std::uint16_t shellMass) noexcept;
bool DashIgnoresProjectiles(const GameState& state) noexcept;

}  // namespace bp
