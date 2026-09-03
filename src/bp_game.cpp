#include "bp_game.h"

#include "bp_claim_ledger.h"
#include "bp_physical_ledger.h"

namespace bp {
namespace {

constexpr std::uint8_t AccountIndex(PhysicalAccount account) noexcept {
    return static_cast<std::uint8_t>(account);
}

constexpr std::uint8_t ActionIndex(PlatformAction action) noexcept {
    return static_cast<std::uint8_t>(action);
}

void ClearAction(InputFrame& input, PlatformAction action) noexcept {
    input.actions[ActionIndex(action)] = ButtonState{};
}

bool CanStartDash(const GameState& state) noexcept {
    return state.impulseInstalled != 0
        && state.dashCooldown == 0
        && state.dashTicksRemaining == 0
        && state.chargeActive == 0
        && state.reclaimActive == 0
        && state.hudChannelActive == 0;
}

void HashByte(std::uint32_t& hash, std::uint8_t value) noexcept {
    hash ^= value;
    hash *= 16777619u;
}

void HashU16(std::uint32_t& hash, std::uint16_t value) noexcept {
    HashByte(hash, static_cast<std::uint8_t>(value));
    HashByte(hash, static_cast<std::uint8_t>(value >> 8));
}

void HashU32(std::uint32_t& hash, std::uint32_t value) noexcept {
    HashU16(hash, static_cast<std::uint16_t>(value));
    HashU16(hash, static_cast<std::uint16_t>(value >> 16));
}

void HashS32(std::uint32_t& hash, std::int32_t value) noexcept {
    HashU32(hash, static_cast<std::uint32_t>(value));
}

void ClearInterruptedActions(GameState& state) noexcept {
    state.chargeActive = 0;
    state.reclaimActive = 0;
    state.hudChannelActive = 0;
}

void BeginDash(GameState& state, const GameplayIntent& intent) noexcept {
    Vec2Q8 direction = NormalizeInput(intent.moveX, intent.moveY);
    if (direction.x == 0 && direction.y == 0) {
        direction = state.lastMoveDirection;
    }
    if (direction.x == 0 && direction.y == 0) {
        direction = Vec2Q8{kInputUnit, 0};
    }
    state.dashDirection = direction;
    state.dashTicksRemaining = kDashDurationTicks;
    state.dashInvulnerableRemaining = kDashInvulnerableTicks;
    state.dashCooldown = kDashCooldownTicks;
    state.playerVelocity = Vec2Q8{0, 0};
}

void MoveDash(GameState& state) noexcept {
    state.playerPosition.x += RoundDivideSigned(
        static_cast<std::int64_t>(state.dashDirection.x) * kDashStepQ8,
        kInputUnit);
    state.playerPosition.y += RoundDivideSigned(
        static_cast<std::int64_t>(state.dashDirection.y) * kDashStepQ8,
        kInputUnit);
    const std::uint16_t shellMass =
        state.physicalLedger.mass[AccountIndex(PhysicalAccount::PlayerShell)];
    const CollisionResolution boundary = ClampCircleToRect(
        state.playerPosition,
        ActorRadiusPixels(shellMass),
        kPlayInterior);
    if (boundary.collided) {
        state.dashTicksRemaining = 0;
        state.dashInvulnerableRemaining = 0;
        state.playerVelocity = Vec2Q8{0, 0};
        return;
    }
    if (state.dashTicksRemaining > 0) {
        --state.dashTicksRemaining;
    }
}

void MoveNormally(GameState& state, const GameplayIntent& intent) noexcept {
    const Vec2Q8 direction = NormalizeInput(intent.moveX, intent.moveY);
    if (direction.x != 0 || direction.y != 0) {
        state.lastMoveDirection = direction;
    }
    constexpr std::int32_t acceleration = PixelsPerSecondSquaredToQ8PerTickSquared(500);
    if (direction.x == 0) {
        state.playerVelocity.x = TruncMultiplyDivide(state.playerVelocity.x, 86, 100);
    } else {
        state.playerVelocity.x += RoundDivideSigned(
            static_cast<std::int64_t>(direction.x) * acceleration,
            kInputUnit);
    }
    if (direction.y == 0) {
        state.playerVelocity.y = TruncMultiplyDivide(state.playerVelocity.y, 86, 100);
    } else {
        state.playerVelocity.y += RoundDivideSigned(
            static_cast<std::int64_t>(direction.y) * acceleration,
            kInputUnit);
    }

    const std::uint16_t shellMass =
        state.physicalLedger.mass[AccountIndex(PhysicalAccount::PlayerShell)];
    std::int32_t maximumSpeed = PlayerMaximumSpeedQ8(shellMass);
    if (intent.action == GameplayAction::Reclaim) {
        maximumSpeed = TruncMultiplyDivide(maximumSpeed, 75, 100);
    } else if (intent.action == GameplayAction::Fire) {
        maximumSpeed = TruncMultiplyDivide(maximumSpeed, 85, 100);
    } else if (intent.action == GameplayAction::HudReallocate) {
        maximumSpeed = TruncMultiplyDivide(maximumSpeed, 50, 100);
    }
    ClampVectorLength(state.playerVelocity, maximumSpeed);
    state.playerPosition.x += state.playerVelocity.x;
    state.playerPosition.y += state.playerVelocity.y;
    const CollisionResolution boundary = ClampCircleToRect(
        state.playerPosition,
        ActorRadiusPixels(shellMass),
        kPlayInterior);
    if (boundary.shiftXQ8 != 0) {
        state.playerVelocity.x = 0;
    }
    if (boundary.shiftYQ8 != 0) {
        state.playerVelocity.y = 0;
    }
}

}  // namespace

std::int32_t PlayerMaximumSpeedQ8(std::uint16_t shellMass) noexcept {
    const std::uint16_t clampedShell = shellMass > 320 ? 320 : shellMass;
    const std::int32_t speedPixelsPerSecond =
        112 - static_cast<std::int32_t>(clampedShell) * 40 / 320;
    return PixelsPerSecondToQ8PerTick(speedPixelsPerSecond);
}

bool InitializeGameState(
    GameState& state,
    std::uint32_t encounterSeed,
    std::uint32_t choiceBits) noexcept {
    if (encounterSeed == 0) {
        return false;
    }
    GameState candidate{};
    candidate.claimLedger = MakeInitialClaimLedger();
    if (!InitializePhysicalLedger(candidate.physicalLedger, 64, 48, 160, 64)) {
        return false;
    }
    candidate.narrative.choiceBits = choiceBits;
    candidate.playerPosition = Vec2Q8{48 * kQ8One, 92 * kQ8One};
    candidate.lastMoveDirection = Vec2Q8{kInputUnit, 0};
    candidate.encounterSeed = encounterSeed;
    candidate.impulseInstalled = 1;
    if (!ClaimLedgerValid(candidate.claimLedger) || !PhysicalLedgerValid(candidate.physicalLedger)) {
        return false;
    }
    state = candidate;
    return true;
}

std::int32_t LargeOpponentMaximumSpeedQ8(std::uint16_t shellMass) noexcept {
    const std::uint16_t clampedShell = shellMass > 384 ? 384 : shellMass;
    return PixelsPerSecondToQ8PerTick(
        98 - static_cast<std::int32_t>(clampedShell) * 38 / 384);
}

bool AdvanceGameIntent(GameState& state, const GameplayIntent& intent) noexcept {
    if (!ClaimLedgerValid(state.claimLedger)
        || !PhysicalLedgerValid(state.physicalLedger)
        || state.paused != 0
        || intent.focusLost) {
        return false;
    }
    GameState next = state;
    const bool activeDash = next.dashTicksRemaining > 0;
    next.currentAction = activeDash ? GameplayAction::Dash : intent.action;
    next.chargeActive = intent.action == GameplayAction::Fire ? 1u : 0u;
    next.reclaimActive = intent.action == GameplayAction::Reclaim ? 1u : 0u;
    next.hudChannelActive = intent.action == GameplayAction::HudReallocate ? 1u : 0u;
    if (intent.action == GameplayAction::Dash && CanStartDash(next)) {
        BeginDash(next, intent);
    }
    if (next.dashTicksRemaining > 0) MoveDash(next);
    else MoveNormally(next, intent);
    if (!ClaimLedgerValid(next.claimLedger) || !PhysicalLedgerValid(next.physicalLedger)) {
        return false;
    }
    state = next;
    return true;
}

bool TickGame(GameState& state, const InputFrame& input) noexcept {
    if (input.tick != state.globalTick
        || !ClaimLedgerValid(state.claimLedger)
        || !PhysicalLedgerValid(state.physicalLedger)) {
        return false;
    }
    GameState next = state;
    if (next.dashCooldown > 0) {
        --next.dashCooldown;
    }
    if (next.dashInvulnerableRemaining > 0) {
        --next.dashInvulnerableRemaining;
    }
    const bool activeDash = next.dashTicksRemaining > 0;
    InputFrame effectiveInput = input;
    if (activeDash) {
        ClearAction(effectiveInput, PlatformAction::HudReallocate);
        ClearAction(effectiveInput, PlatformAction::Dash);
        ClearAction(effectiveInput, PlatformAction::Fire);
        ClearAction(effectiveInput, PlatformAction::Reclaim);
    }
    GameplayIntent intent = ResolveGameplayInput(
        next.inputArbiter,
        effectiveInput,
        CanStartDash(next),
        true);
    next.currentAction = intent.action;
    if (intent.focusLost) {
        next.paused = 1;
        ClearInterruptedActions(next);
        ++next.globalTick;
        state = next;
        return true;
    }
    if (intent.action == GameplayAction::Pause) {
        next.paused = next.paused == 0 ? 1u : 0u;
        ClearInterruptedActions(next);
        ++next.globalTick;
        state = next;
        return true;
    }
    if (next.paused != 0) {
        ++next.globalTick;
        state = next;
        return true;
    }
    if (activeDash) {
        next.currentAction = GameplayAction::Dash;
    }

    if (!AdvanceGameIntent(next, intent)) {
        return false;
    }
    ++next.globalTick;
    state = next;
    return true;
}

bool DashIgnoresProjectiles(const GameState& state) noexcept {
    return state.dashInvulnerableRemaining > 0;
}

std::uint32_t GameStateHash(const GameState& state) noexcept {
    std::uint32_t hash = 2166136261u;
    for (std::uint8_t index = 0; index < kClaimOwnerCount; ++index) {
        HashU16(hash, state.claimLedger.mass[index]);
    }
    for (std::uint8_t index = 0; index < kPhysicalAccountCount; ++index) {
        HashU16(hash, state.physicalLedger.mass[index]);
    }
    HashU32(hash, state.narrative.choiceBits);
    HashU16(hash, state.narrative.adjudicatedMass);
    HashByte(hash, state.narrative.encounterIndex);
    HashByte(hash, state.narrative.role);
    for (std::uint8_t status : state.narrative.entityStatus) HashByte(hash, status);
    HashByte(hash, state.narrative.endingsSeen);
    HashByte(hash, static_cast<std::uint8_t>(state.inputArbiter.lastFireReclaim));
    HashByte(hash, static_cast<std::uint8_t>(state.inputArbiter.lockedFireReclaim));
    HashByte(hash, state.inputArbiter.switchLockTicks);
    HashS32(hash, state.playerPosition.x); HashS32(hash, state.playerPosition.y);
    HashS32(hash, state.playerVelocity.x); HashS32(hash, state.playerVelocity.y);
    HashS32(hash, state.dashDirection.x); HashS32(hash, state.dashDirection.y);
    HashS32(hash, state.lastMoveDirection.x); HashS32(hash, state.lastMoveDirection.y);
    HashU32(hash, state.encounterSeed); HashU32(hash, state.globalTick);
    HashU16(hash, state.dashCooldown);
    HashByte(hash, state.dashTicksRemaining);
    HashByte(hash, state.dashInvulnerableRemaining);
    HashByte(hash, static_cast<std::uint8_t>(state.currentAction));
    HashByte(hash, state.paused); HashByte(hash, state.impulseInstalled);
    HashByte(hash, state.chargeActive); HashByte(hash, state.reclaimActive);
    HashByte(hash, state.hudChannelActive);
    return hash;
}

}  // namespace bp
