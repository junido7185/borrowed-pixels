#include "bp_claim_ledger.h"
#include "bp_game.h"
#include "bp_input.h"
#include "bp_math.h"
#include "bp_physical_ledger.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

std::uint32_t gT04Checks;
std::uint32_t gT04Failures;

void Check(bool condition, const char* name) noexcept {
    ++gT04Checks;
    if (!condition) {
        ++gT04Failures;
        std::printf("T04_FAIL\t%s\n", name);
    }
}

std::uint32_t ActionIndex(bp::PlatformAction action) noexcept {
    return static_cast<std::uint32_t>(action);
}

bp::InputFrame Frame(std::uint32_t tick, std::int16_t moveX = 0, std::int16_t moveY = 0) noexcept {
    bp::InputFrame frame{};
    frame.tick = tick;
    frame.focused = 1;
    frame.moveX = moveX;
    frame.moveY = moveY;
    return frame;
}

void Press(bp::InputFrame& frame, bp::PlatformAction action) noexcept {
    frame.actions[ActionIndex(action)].pressed = 1;
    frame.actions[ActionIndex(action)].held = 1;
}

void Hold(bp::InputFrame& frame, bp::PlatformAction action) noexcept {
    frame.actions[ActionIndex(action)].held = 1;
}

void TestMathContract() noexcept {
    Check(bp::PixelsPerSecondToQ8PerTick(112) == 478, "112px/s Q8 conversion");
    Check(bp::PixelsPerSecondToQ8PerTick(-112) == -478, "negative px/s conversion");
    Check(bp::PixelsPerSecondSquaredToQ8PerTickSquared(500) == 36, "500px/s2 Q8 conversion");
    Check(bp::PixelsPerSecondSquaredToQ8PerTickSquared(-500) == -36, "negative px/s2 conversion");
    Check(bp::TruncMultiplyDivide(-99, 86, 100) == -85, "friction truncates toward zero");
    Check(bp::RoundDivideSigned(-5, 2) == -3, "signed nearest rounding");
    Check(bp::ActorRadiusPixels(0) == 4, "radius shell zero");
    Check(bp::ActorRadiusPixels(48) == 7, "radius shell 48");
    Check(bp::ActorRadiusPixels(320) == 14, "radius shell 320");
    Check(bp::PlayerMaximumSpeedQ8(0) == bp::PixelsPerSecondToQ8PerTick(112),
        "empty shell maximum speed");
    Check(bp::PlayerMaximumSpeedQ8(160) == bp::PixelsPerSecondToQ8PerTick(92),
        "half shell linearly interpolated speed");
    Check(bp::PlayerMaximumSpeedQ8(320) == bp::PixelsPerSecondToQ8PerTick(72),
        "full shell maximum speed");
    Check(bp::PlayerMaximumSpeedQ8(65535) == bp::PixelsPerSecondToQ8PerTick(72),
        "oversized shell speed clamps at full shell");
    for (std::uint32_t value = 0; value < 100000; ++value) {
        const std::uint32_t root = bp::IntegerSqrt(value);
        Check(static_cast<std::uint64_t>(root) * root <= value, "isqrt lower bound");
        Check(static_cast<std::uint64_t>(root + 1) * (root + 1) > value, "isqrt upper bound");
    }

    bp::Vec2Q8 diagonal = bp::NormalizeInput(32767, 32767);
    const std::uint32_t diagonalLength = bp::IntegerSqrt(
        static_cast<std::uint64_t>(static_cast<std::int64_t>(diagonal.x) * diagonal.x)
        + static_cast<std::uint64_t>(static_cast<std::int64_t>(diagonal.y) * diagonal.y));
    Check(diagonalLength >= 32766 && diagonalLength <= 32768, "diagonal input normalized");

    bp::Vec2Q8 center{8 * bp::kQ8One, 20 * bp::kQ8One};
    const bp::CollisionResolution side = bp::ResolveCircleAgainstAabb(
        center, 4, bp::RectPixels{10, 10, 10, 20});
    Check(side.collided && side.shiftXQ8 < 0 && side.shiftYQ8 == 0, "circle AABB side resolution");
    center.x += side.shiftXQ8;
    center.y += side.shiftYQ8;
    Check(!bp::ResolveCircleAgainstAabb(center, 4, bp::RectPixels{10, 10, 10, 20}).collided,
        "circle AABB side fully separated");
    center = bp::Vec2Q8{9 * bp::kQ8One, 9 * bp::kQ8One};
    const bp::CollisionResolution corner = bp::ResolveCircleAgainstAabb(
        center, 4, bp::RectPixels{10, 10, 10, 20});
    Check(corner.collided && corner.shiftXQ8 < 0 && corner.shiftYQ8 < 0, "circle AABB corner resolution");
    center.x += corner.shiftXQ8;
    center.y += corner.shiftYQ8;
    Check(!bp::ResolveCircleAgainstAabb(center, 4, bp::RectPixels{10, 10, 10, 20}).collided,
        "circle AABB corner fully separated");

    const bp::RectPixels obstacle{10, 10, 10, 20};
    for (std::int32_t radius = 1; radius <= 8; ++radius) {
        for (std::int32_t y = 2; y <= 38; ++y) {
            for (std::int32_t x = 2; x <= 28; ++x) {
                bp::Vec2Q8 sample{x * bp::kQ8One, y * bp::kQ8One};
                const bp::CollisionResolution resolution =
                    bp::ResolveCircleAgainstAabb(sample, radius, obstacle);
                sample.x += resolution.shiftXQ8;
                sample.y += resolution.shiftYQ8;
                Check(!resolution.collided
                        || !bp::ResolveCircleAgainstAabb(sample, radius, obstacle).collided,
                    "circle AABB grid resolution fully separates");
            }
        }
    }
}

void TestInputArbitration() noexcept {
    bp::InputArbiter arbiter{};
    bp::InputFrame frame = Frame(0, 32767, 0);
    Press(frame, bp::PlatformAction::Pause);
    Press(frame, bp::PlatformAction::HudReallocate);
    Press(frame, bp::PlatformAction::Dash);
    Press(frame, bp::PlatformAction::Fire);
    Press(frame, bp::PlatformAction::Reclaim);
    Check(bp::ResolveGameplayInput(arbiter, frame, true, true).action == bp::GameplayAction::Pause,
        "pause input priority");

    arbiter = {};
    frame = Frame(0, 32767, 0);
    Press(frame, bp::PlatformAction::HudReallocate);
    Press(frame, bp::PlatformAction::Dash);
    Press(frame, bp::PlatformAction::Fire);
    Check(bp::ResolveGameplayInput(arbiter, frame, true, true).action == bp::GameplayAction::HudReallocate,
        "HUD input priority");

    arbiter = {};
    frame = Frame(0, 32767, 0);
    Press(frame, bp::PlatformAction::Dash);
    Press(frame, bp::PlatformAction::Fire);
    Check(bp::ResolveGameplayInput(arbiter, frame, true, true).action == bp::GameplayAction::Dash,
        "dash input priority");

    arbiter = {};
    frame = Frame(0);
    Press(frame, bp::PlatformAction::Fire);
    Press(frame, bp::PlatformAction::Reclaim);
    Check(bp::ResolveGameplayInput(arbiter, frame, false, false).action == bp::GameplayAction::Fire,
        "same poll Fire beats Reclaim");
    Check(arbiter.switchLockTicks == 9, "action switch lock starts at nine");

    for (std::uint32_t tick = 0; tick < 8; ++tick) {
        frame = Frame(tick + 1);
        if (tick == 0) {
            Press(frame, bp::PlatformAction::Reclaim);
        } else {
            Hold(frame, bp::PlatformAction::Reclaim);
        }
        Check(bp::ResolveGameplayInput(arbiter, frame, false, false).action == bp::GameplayAction::None,
            "opposite action locked");
        Check(arbiter.lastFireReclaim == bp::GameplayAction::Fire, "locked opposite edge cannot replace priority");
    }
    frame = Frame(9);
    Hold(frame, bp::PlatformAction::Reclaim);
    Check(bp::ResolveGameplayInput(arbiter, frame, false, false).action == bp::GameplayAction::Reclaim,
        "opposite action available after lock");

    frame = Frame(10, 123, -456);
    Check(bp::ResolveGameplayInput(arbiter, frame, false, false).action == bp::GameplayAction::Move,
        "move is lowest priority");
    frame.focused = 0;
    Check(bp::ResolveGameplayInput(arbiter, frame, false, false).focusLost,
        "focus loss pauses intent");
}

void TestMovementAndBoundaries() noexcept {
    bp::GameState state{};
    Check(bp::InitializeGameState(state, 0x12345678u), "initialize game state");
    Check(bp::ClaimLedgerValid(state.claimLedger), "game claim ledger valid");
    Check(bp::PhysicalLedgerValid(state.physicalLedger), "game physical ledger valid");
    const bp::ClaimLedger initialClaims = state.claimLedger;
    const bp::PhysicalLedger initialPhysical = state.physicalLedger;
    const std::int32_t startX = state.playerPosition.x;
    for (std::uint32_t tick = 0; tick < 60; ++tick) {
        bp::InputFrame frame = Frame(state.globalTick, 32767, 0);
        Check(bp::TickGame(state, frame), "fixed movement tick");
        Check(bp::ClaimLedgerValid(state.claimLedger), "claim invariant during movement");
        Check(bp::PhysicalLedgerValid(state.physicalLedger), "physical invariant during movement");
    }
    Check(state.globalTick == 60, "fixed tick increments exactly once");
    Check(state.playerPosition.x > startX, "player moved right");
    const std::uint32_t velocityLength = bp::IntegerSqrt(
        static_cast<std::uint64_t>(static_cast<std::int64_t>(state.playerVelocity.x) * state.playerVelocity.x)
        + static_cast<std::uint64_t>(static_cast<std::int64_t>(state.playerVelocity.y) * state.playerVelocity.y));
    Check(velocityLength <= static_cast<std::uint32_t>(bp::PlayerMaximumSpeedQ8(48)), "player speed capped by shell");

    for (std::uint32_t tick = 0; tick < 100; ++tick) {
        Check(bp::TickGame(state, Frame(state.globalTick)), "friction tick");
    }
    Check(state.playerVelocity.x == 0 && state.playerVelocity.y == 0, "friction reaches zero");
    Check(std::memcmp(&state.claimLedger, &initialClaims, sizeof(initialClaims)) == 0,
        "movement does not mutate claim ledger");
    Check(std::memcmp(&state.physicalLedger, &initialPhysical, sizeof(initialPhysical)) == 0,
        "movement does not mutate physical ledger");

    bp::GameState before = state;
    bp::InputFrame invalid = Frame(state.globalTick + 1, 32767, 0);
    Check(!bp::TickGame(state, invalid), "out of sequence input rejected");
    Check(std::memcmp(&state, &before, sizeof(state)) == 0, "rejected tick rolls back byte-identical");

    std::uint32_t random = 0xA341316Cu;
    for (std::uint32_t tick = 0; tick < 20000; ++tick) {
        random ^= random << 13; random ^= random >> 17; random ^= random << 5;
        const std::int16_t x = (random & 1u) ? 32767 : ((random & 2u) ? -32767 : 0);
        const std::int16_t y = (random & 4u) ? 32767 : ((random & 8u) ? -32767 : 0);
        bp::InputFrame frame = Frame(state.globalTick, x, y);
        if (tick % 173 == 5) Press(frame, bp::PlatformAction::Dash);
        Check(bp::TickGame(state, frame), "boundary replay tick");
        const std::int32_t radiusQ8 = bp::ActorRadiusPixels(48) * bp::kQ8One;
        Check(state.playerPosition.x >= bp::kPlayInterior.x * bp::kQ8One + radiusQ8,
            "left boundary respected");
        Check(state.playerPosition.x <= (bp::kPlayInterior.x + bp::kPlayInterior.width) * bp::kQ8One - radiusQ8,
            "right boundary respected");
        Check(state.playerPosition.y >= bp::kPlayInterior.y * bp::kQ8One + radiusQ8,
            "top boundary respected");
        Check(state.playerPosition.y <= (bp::kPlayInterior.y + bp::kPlayInterior.height) * bp::kQ8One - radiusQ8,
            "bottom boundary respected");
    }
}

void TestDashContract() noexcept {
    bp::GameState state{};
    Check(bp::InitializeGameState(state, 7), "dash state initialize");
    const bp::PhysicalLedger physicalBefore = state.physicalLedger;
    const std::int32_t startX = state.playerPosition.x;
    for (std::uint32_t tick = 0; tick < 8; ++tick) {
        bp::InputFrame frame = Frame(state.globalTick, 32767, 0);
        if (tick == 0) Press(frame, bp::PlatformAction::Dash);
        Check(bp::TickGame(state, frame), "dash tick");
        Check(bp::DashIgnoresProjectiles(state) == (tick < 5), "dash first five movement ticks invulnerable state");
    }
    Check(state.playerPosition.x - startX == 20 * bp::kQ8One, "dash travels exact 20px in eight ticks");
    Check(state.dashTicksRemaining == 0, "dash ends after eight ticks");
    Check(state.dashCooldown == 137, "dash cooldown countdown semantics");
    Check(std::memcmp(&state.physicalLedger, &physicalBefore, sizeof(physicalBefore)) == 0,
        "dash consumes no mass");

    while (state.dashCooldown > 1) {
        Check(bp::TickGame(state, Frame(state.globalTick)), "dash cooldown countdown tick");
    }
    bp::InputFrame readyDash = Frame(state.globalTick, 32767, 0);
    Press(readyDash, bp::PlatformAction::Dash);
    Check(bp::TickGame(state, readyDash), "dash ready on countdown-to-zero tick");
    Check(state.dashTicksRemaining == 7 && state.dashCooldown == bp::kDashCooldownTicks,
        "dash restarts exactly when cooldown reaches zero");

    bp::InputFrame duringDash = Frame(state.globalTick, 32767, 0);
    Press(duringDash, bp::PlatformAction::Fire);
    Press(duringDash, bp::PlatformAction::Reclaim);
    Press(duringDash, bp::PlatformAction::HudReallocate);
    Check(bp::TickGame(state, duringDash), "active dash ignores competing action tick");
    Check(state.currentAction == bp::GameplayAction::Dash
        && state.chargeActive == 0
        && state.reclaimActive == 0
        && state.hudChannelActive == 0,
        "active dash cannot start fire reclaim or HUD channel");

    while (state.dashTicksRemaining > 0) {
        Check(bp::TickGame(state, Frame(state.globalTick)), "finish restarted dash");
    }

    bp::InputFrame blocked = Frame(state.globalTick, 32767, 0);
    Press(blocked, bp::PlatformAction::Dash);
    const std::int32_t beforeBlocked = state.playerPosition.x;
    Check(bp::TickGame(state, blocked), "cooldown dash input tick");
    Check(state.dashTicksRemaining == 0 && state.playerPosition.x != beforeBlocked,
        "cooldown blocks dash but allows movement");

    bp::GameState constrained{};
    Check(bp::InitializeGameState(constrained, 9), "constrained dash initialize");
    constrained.chargeActive = 1;
    blocked = Frame(constrained.globalTick, 32767, 0);
    Press(blocked, bp::PlatformAction::Dash);
    Check(bp::TickGame(constrained, blocked), "charge constrained tick");
    Check(constrained.dashTicksRemaining == 0, "charge blocks dash");

    bp::GameState boundary{};
    Check(bp::InitializeGameState(boundary, 11), "boundary dash initialize");
    const std::int32_t radiusQ8 = bp::ActorRadiusPixels(48) * bp::kQ8One;
    boundary.playerPosition.x = (bp::kPlayInterior.x + bp::kPlayInterior.width) * bp::kQ8One - radiusQ8 - bp::kQ8One;
    bp::InputFrame edgeDash = Frame(boundary.globalTick, 32767, 0);
    Press(edgeDash, bp::PlatformAction::Dash);
    Check(bp::TickGame(boundary, edgeDash), "boundary dash tick");
    Check(boundary.playerPosition.x == (bp::kPlayInterior.x + bp::kPlayInterior.width) * bp::kQ8One - radiusQ8,
        "dash clamps to boundary");
    Check(boundary.dashTicksRemaining == 0, "boundary discards remaining dash distance");
}

bp::InputFrame ReplayFrame(std::uint32_t tick, bool variant) noexcept {
    const std::uint32_t phase = tick % 240;
    std::int16_t x = phase < 60 ? 32767 : (phase < 120 ? 0 : (phase < 180 ? -32767 : 0));
    std::int16_t y = phase < 60 ? 0 : (phase < 120 ? 32767 : (phase < 180 ? 0 : -32767));
    if (variant && tick == 500) x = static_cast<std::int16_t>(-x);
    bp::InputFrame frame = Frame(tick, x, y);
    if (tick % 181 == 17) Press(frame, bp::PlatformAction::Dash);
    if (tick % 97 >= 80 && tick % 97 < 84) Hold(frame, bp::PlatformAction::Fire);
    if (tick % 131 >= 110 && tick % 131 < 115) Hold(frame, bp::PlatformAction::Reclaim);
    return frame;
}

void TestDeterministicReplay() noexcept {
    bp::GameState first{};
    bp::GameState second{};
    bp::GameState variant{};
    Check(bp::InitializeGameState(first, 0xC001D00Du, 0x00015555u), "replay first initialize");
    Check(bp::InitializeGameState(second, 0xC001D00Du, 0x00015555u), "replay second initialize");
    Check(bp::InitializeGameState(variant, 0xC001D00Du, 0x00015555u), "replay variant initialize");
    for (std::uint32_t tick = 0; tick < 4096; ++tick) {
        Check(bp::TickGame(first, ReplayFrame(tick, false)), "replay first tick");
        Check(bp::TickGame(second, ReplayFrame(tick, false)), "replay second tick");
        Check(bp::TickGame(variant, ReplayFrame(tick, true)), "replay variant tick");
        Check(bp::GameStateHash(first) == bp::GameStateHash(second), "same replay hash each tick");
        Check(bp::ClaimLedgerValid(first.claimLedger), "replay claim ledger invariant");
        Check(bp::PhysicalLedgerValid(first.physicalLedger), "replay physical ledger invariant");
    }
    const std::uint32_t hash = bp::GameStateHash(first);
    std::printf("T04_INFO\treplay=%08X\tticks=4096\n", hash);
    Check(hash == 0xDD90E82Du, "replay golden hash");
    Check(bp::GameStateHash(variant) != hash, "changed input changes replay hash");
}

struct T04TestRunner {
    T04TestRunner() noexcept {
        TestMathContract();
        TestInputArbitration();
        TestMovementAndBoundaries();
        TestDashContract();
        TestDeterministicReplay();
        if (gT04Failures != 0) {
            std::printf("T04_FAIL\tchecks=%u\tfailures=%u\n", gT04Checks, gT04Failures);
            std::exit(1);
        }
        std::printf("T04_PASS\tchecks=%u\treplay_ticks=4096\n", gT04Checks);
    }
};

T04TestRunner gT04TestRunner;

}  // namespace
