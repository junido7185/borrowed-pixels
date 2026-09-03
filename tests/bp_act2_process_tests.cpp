#include "bp_act2.h"

#include "bp_claim_ledger.h"
#include "bp_physical_ledger.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace {

std::uint32_t gChecks;
std::uint32_t gFailures;

void Check(bool condition, const char* name) noexcept {
    ++gChecks;
    if (!condition) {
        ++gFailures;
        std::printf("T08A_FAIL\t%s\n", name);
    }
}

constexpr std::uint8_t AccountIndex(bp::PhysicalAccount value) noexcept {
    return static_cast<std::uint8_t>(value);
}

bp::ClaimLedger Act2Claims(bp::NarrativeState& narrative) noexcept {
    bp::ClaimLedger claims = bp::MakeInitialClaimLedger();
    Check(bp::ApplySettlement(claims, narrative, bp::EncounterId::Vessel, bp::ChoiceId::Second),
        "Act 1 continuation claim prepared");
    return claims;
}

bp::InputFrame Input(bool confirmHeld = false) noexcept {
    bp::InputFrame input{};
    input.focused = 1;
    input.device = bp::InputDevice::KeyboardMouse;
    input.mouseX = 300;
    input.mouseY = 90;
    input.actions[static_cast<std::uint8_t>(bp::PlatformAction::Confirm)].held = confirmHeld ? 1u : 0u;
    return input;
}

void TestDefinitionsExact() noexcept {
    constexpr std::uint16_t opponent[3] = {112, 72, 192};
    constexpr std::uint16_t field[3] = {192, 192, 384};
    constexpr std::uint16_t neutral[3] = {64, 48, 96};
    constexpr std::uint16_t duration[3] = {5400, 4500, 18000};
    constexpr std::uint8_t fields[3] = {6, 6, 12};
    constexpr std::uint8_t neutrals[3] = {8, 6, 12};
    for (std::uint8_t index = 0; index < 3; ++index) {
        const bp::Act2EncounterDef& definition = bp::GetAct2EncounterDef(index);
        Check(bp::ValidateAct2EncounterDef(definition), "Act 2 definition validates");
        Check(definition.opponentMass == opponent[index], "opponent mass exact");
        Check(definition.fieldMass == field[index], "field mass exact");
        Check(definition.neutralMass == neutral[index], "neutral mass exact");
        Check(definition.durationTicks == duration[index], "duration exact");
        Check(definition.fieldCount == fields[index], "field count exact");
        Check(definition.neutralCount == neutrals[index], "neutral count exact");
    }
    const bp::Act2EncounterDef& keeper = bp::GetAct2EncounterDef(0);
    Check(keeper.playerX == 58 && keeper.playerY == 92
        && keeper.opponentX == 252 && keeper.opponentY == 90,
        "KEEPER spawns exact");
    Check(keeper.field[0].centerX == 92 && keeper.field[0].centerY == 50
        && keeper.field[5].centerX == 228 && keeper.field[5].centerY == 130,
        "KEEPER field endpoints exact");
    const bp::Act2EncounterDef& adapter = bp::GetAct2EncounterDef(1);
    Check(adapter.playerX == 46 && adapter.opponentX == 274,
        "ADAPTER opposite spawns exact");
    for (std::uint8_t index = 0; index < adapter.neutralCount; ++index) {
        Check(adapter.neutral[index].mass == 8 && adapter.neutral[index].escrow == 1,
            "all six ADAPTER escrow units tagged");
    }
    const bp::Act2EncounterDef& frame = bp::GetAct2EncounterDef(2);
    Check(frame.field[0].centerX == 96 && frame.field[0].centerY == 48
        && frame.field[11].centerX == 208 && frame.field[11].centerY == 108,
        "FRAME panel endpoints exact");
}

void TestAdapterAllResults() noexcept {
    for (std::uint16_t mass = 0; mass <= 48; ++mass) {
        const bp::AdapterResult expected = mass <= 16 ? bp::AdapterResult::Need
            : (mass <= 31 ? bp::AdapterResult::Equal : bp::AdapterResult::Output);
        Check(bp::ClassifyAdapterResult(mass) == expected,
            "ADAPTER 0..48 result classification exhaustive");
        const std::int32_t expectedSpeed = 88 - static_cast<std::int32_t>(mass / 8) * 6;
        Check(bp::AdapterMaximumSpeedPixelsPerSecond(mass)
                == (expectedSpeed < 52 ? 52 : expectedSpeed),
            "ADAPTER 0..48 capability speed exhaustive");
        bp::NarrativeState narrative{};
        bp::ClaimLedger claims = Act2Claims(narrative);
        const std::uint16_t beforePlayer = claims.mass[static_cast<std::uint8_t>(bp::ClaimOwner::Player)];
        Check(bp::ApplyAdapterSettlementForEscrow(claims, narrative, mass),
            "ADAPTER 0..48 settlement applies exhaustively");
        Check(claims.mass[static_cast<std::uint8_t>(bp::ClaimOwner::Frame)] == 192,
            "ADAPTER settlement always transfers exact FRAME 48");
        const std::uint16_t expectedPlayer = expected == bp::AdapterResult::Need ? beforePlayer
            : static_cast<std::uint16_t>(beforePlayer
                + (expected == bp::AdapterResult::Equal ? 24 : 32));
        const std::uint16_t expectedAdapter = static_cast<std::uint16_t>(72
            + (expected == bp::AdapterResult::Need ? 48
                : (expected == bp::AdapterResult::Equal ? 24 : 16)));
        Check(claims.mass[static_cast<std::uint8_t>(bp::ClaimOwner::Player)] == expectedPlayer
            && claims.mass[static_cast<std::uint8_t>(bp::ClaimOwner::Adapter)] == expectedAdapter,
            "ADAPTER 0..48 exact claim destinations exhaustive");
        Check(bp::ClaimLedgerValid(claims), "ADAPTER exhaustive settlement conserves 1440");
    }
    Check(bp::AdapterMaximumSpeedPixelsPerSecond(65535) == 52,
        "ADAPTER speed clamps at 52");
}

bp::Act2Runtime FrameWorld() noexcept {
    bp::NarrativeState narrative{};
    const bp::ClaimLedger claims = Act2Claims(narrative);
    bp::Act2Runtime state{};
    Check(bp::InitializeAct2Runtime(state, claims, narrative), "FRAME action world initializes");
    state.encounterIndex = 2;
    state.scene = bp::Act2Scene::Playing;
    Check(bp::PopulateAct2Encounter(state), "FRAME action world populates");
    return state;
}

void TestFrameActions() noexcept {
    bp::Act2Runtime shift = FrameWorld();
    Check(bp::ForceFrameActionForTest(shift, bp::FrameAction::Shift), "FRAME SHIFT begins");
    const bp::RectPixels start = shift.frame.shiftStart;
    for (std::uint8_t tick = 0; tick < 60; ++tick) {
        Check(bp::TickFrameActionForTest(shift), "FRAME SHIFT advances 60 exact ticks");
    }
    const bp::CoverSlot& shifted = shift.combat.covers[shift.frame.shiftPanel.index];
    const std::int32_t dx = shifted.bounds.x - start.x;
    const std::int32_t dy = shifted.bounds.y - start.y;
    Check((dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy) == 20,
        "FRAME SHIFT chooses a collision-free exact 20px route");
    Check(shift.frame.actionCooldown == 30, "FRAME SHIFT 90 cooldown overlaps 60 movement ticks");

    bp::Act2Runtime toll = FrameWorld();
    const std::uint16_t tollCover = toll.movement.physicalLedger.mass[AccountIndex(bp::PhysicalAccount::Cover)];
    const std::uint16_t tollNeutral = toll.movement.physicalLedger.mass[AccountIndex(bp::PhysicalAccount::Neutral)];
    Check(bp::ForceFrameActionForTest(toll, bp::FrameAction::Toll), "FRAME TOLL begins");
    Check(toll.movement.physicalLedger.mass[AccountIndex(bp::PhysicalAccount::Cover)] == tollCover - 16
        && toll.movement.physicalLedger.mass[AccountIndex(bp::PhysicalAccount::Neutral)] == tollNeutral + 16,
        "FRAME TOLL converts exact panel16 to door allocation");
    const std::uint16_t playerBefore = toll.movement.physicalLedger.mass[AccountIndex(bp::PhysicalAccount::PlayerShell)];
    toll.combat.actors[0].position = bp::Vec2Q8{
        (toll.frame.door.x + toll.frame.door.width / 2) * bp::kQ8One,
        (toll.frame.door.y + toll.frame.door.height / 2) * bp::kQ8One};
    Check(bp::TickFrameActionForTest(toll), "FRAME TOLL first contact resolves");
    Check(toll.movement.physicalLedger.mass[AccountIndex(bp::PhysicalAccount::PlayerShell)] == playerBefore - 8,
        "FRAME TOLL first contact strips exact PlayerShell8");
    Check(bp::TickFrameActionForTest(toll), "FRAME TOLL persistent contact resolves");
    Check(toll.movement.physicalLedger.mass[AccountIndex(bp::PhysicalAccount::PlayerShell)] == playerBefore - 8,
        "FRAME TOLL same door cannot retrigger without exit");
    toll.combat.actors[0].position.x += 31 * bp::kQ8One;
    Check(bp::TickFrameActionForTest(toll), "FRAME TOLL rearms beyond 30px");
    toll.combat.actors[0].position.x -= 31 * bp::kQ8One;
    Check(bp::TickFrameActionForTest(toll), "FRAME TOLL reentry resolves");
    Check(toll.movement.physicalLedger.mass[AccountIndex(bp::PhysicalAccount::PlayerShell)] == playerBefore - 16,
        "FRAME TOLL strips again only after reentry");

    bp::Act2Runtime evict = FrameWorld();
    const std::uint16_t evictCover = evict.movement.physicalLedger.mass[AccountIndex(bp::PhysicalAccount::Cover)];
    const std::uint16_t evictShell = evict.movement.physicalLedger.mass[AccountIndex(bp::PhysicalAccount::OpponentShell)];
    Check(bp::ForceFrameActionForTest(evict, bp::FrameAction::Evict), "FRAME EVICT begins");
    Check(evict.frame.structureControlMass == 96 && evict.frame.actionCooldown == 120,
        "FRAME EVICT spends structure32 with cooldown120");
    Check(evict.movement.physicalLedger.mass[AccountIndex(bp::PhysicalAccount::Cover)] == evictCover - 32
        && evict.movement.physicalLedger.mass[AccountIndex(bp::PhysicalAccount::OpponentShell)] == evictShell - 32
        && evict.movement.physicalLedger.mass[AccountIndex(bp::PhysicalAccount::Projectile)] == 32,
        "FRAME EVICT deactivates panel and launches exact mass32 slab");

    bp::Act2Runtime claim = FrameWorld();
    Check(bp::ForceFrameActionForTest(claim, bp::FrameAction::Claim), "FRAME CLAIM begins");
    Check(claim.frame.actionCooldown == 75
        && claim.movement.physicalLedger.mass[AccountIndex(bp::PhysicalAccount::Projectile)] == 32,
        "FRAME CLAIM launches mass32 with cooldown75");
}

void TestFrameScoresExhaustive() noexcept {
    for (std::uint16_t bits = 0; bits < 1024; ++bits) {
        const bp::FrameObservation o{
            static_cast<std::uint8_t>((bits >> 0) & 1),
            static_cast<std::uint8_t>((bits >> 1) & 1),
            static_cast<std::uint8_t>((bits >> 2) & 1),
            static_cast<std::uint8_t>((bits >> 3) & 1),
            static_cast<std::uint8_t>((bits >> 4) & 1),
            static_cast<std::uint8_t>((bits >> 5) & 1),
            static_cast<std::uint8_t>((bits >> 6) & 1),
            static_cast<std::uint8_t>((bits >> 7) & 1),
            static_cast<std::uint8_t>((bits >> 8) & 1),
            static_cast<std::uint8_t>((bits >> 9) & 1),
        };
        const std::int32_t scores[6] = {
            80*o.incomingThreat + 40*o.blockedChoice + 20*o.playerNearPanel,
            70*o.playerNearPanel + 30*o.playerLowShell,
            60*o.coverBlocks + 30*o.playerChannel - 20*o.selfLowStructure,
            45*o.lineOfSight + 35*o.playerChannel + 20*o.rangeFit,
            55*o.selfLowStructure + 40*o.neutralNear - 50*o.incomingThreat,
            30 + 40*(1-o.rangeFit),
        };
        for (std::uint8_t action = 0; action < 6; ++action) {
            Check(bp::FrameActionScore(static_cast<bp::FrameAction>(action), o) == scores[action],
                "FRAME score exhaustive boolean observation");
        }
        for (std::uint8_t mask = 1; mask < 64; ++mask) {
            std::uint8_t expected = 0;
            std::int32_t best = -32768;
            for (std::uint8_t action = 0; action < 6; ++action) {
                if ((mask & (1u << action)) != 0 && scores[action] > best) {
                    expected = action;
                    best = scores[action];
                }
            }
            Check(static_cast<std::uint8_t>(bp::ChooseFrameAction(o, mask)) == expected,
                "FRAME availability and tie order exhaustive");
        }
    }
}

void TestProcessesAndPopulation() noexcept {
    bp::NarrativeState narrative{};
    const bp::ClaimLedger claims = Act2Claims(narrative);
    bp::Act2Runtime state{};
    Check(bp::InitializeAct2Runtime(state, claims, narrative), "Act 2 initializes");
    Check(bp::PopulateAct2Encounter(state), "KEEPER population succeeds");
    Check(state.movement.physicalLedger.mass[AccountIndex(bp::PhysicalAccount::Cover)] == 192,
        "KEEPER field exact in physical ledger");
    Check(state.movement.physicalLedger.mass[AccountIndex(bp::PhysicalAccount::OpponentShell)] == 96,
        "KEEPER shell includes 80 structure and 16 body");
    Check(bp::StartSweeperWave(state.combat, state.movement.physicalLedger, state.keeper.sweeper),
        "SWEEPER eight-segment wave starts");
    for (std::uint8_t index = 0; index < bp::kSweeperSegmentCount; ++index) {
        const bp::CoverSlot& segment = state.combat.covers[state.keeper.sweeper.segments[index].index];
        Check(segment.mass == 4 && segment.bounds.width == 4 && segment.bounds.height == 8,
            "SWEEPER segment is exact 4 by 8 mass4");
    }
    for (std::uint16_t tick = 0; tick < 90; ++tick) {
        Check(bp::TickSweeperWave(state.combat, state.movement.physicalLedger, state.keeper.sweeper),
            "SWEEPER advances each crossing tick");
    }
    Check(state.keeper.sweeper.returning == 1, "SWEEPER enters 36-tick return phase");
    for (std::uint16_t tick = 0; tick < 36; ++tick) {
        Check(bp::TickSweeperWave(state.combat, state.movement.physicalLedger, state.keeper.sweeper),
            "SWEEPER return advances each tick");
    }
    Check(state.keeper.sweeper.returning == 0
        && state.movement.physicalLedger.mass[AccountIndex(bp::PhysicalAccount::ProtocolActive)] == 32,
        "SWEEPER protocol mass fully restored");

    state.encounterIndex = 1;
    Check(bp::PopulateAct2Encounter(state), "ADAPTER population succeeds");
    Check(bp::StartCompactor(state.combat, state.movement.physicalLedger, state.adapter.compactor),
        "COMPACTOR starts with two boundaries");
    Check(state.combat.covers[state.adapter.compactor.walls[0].index].bounds.width == 16
        && state.combat.covers[state.adapter.compactor.walls[0].index].bounds.height == 2,
        "COMPACTOR exact 16 by 2 shape");
    for (std::uint16_t tick = 0; tick < 180; ++tick) {
        Check(bp::TickCompactor(state.combat, state.movement.physicalLedger, state.adapter.compactor),
            "COMPACTOR close phase advances");
    }
    Check(state.combat.covers[state.adapter.compactor.walls[0].index].bounds.y == 42,
        "COMPACTOR closes by exact 24 pixels");
    for (std::uint16_t tick = 0; tick < 60; ++tick) {
        Check(bp::TickCompactor(state.combat, state.movement.physicalLedger, state.adapter.compactor),
            "COMPACTOR hold phase advances");
    }
    for (std::uint16_t tick = 0; tick < 60; ++tick) {
        Check(bp::TickCompactor(state.combat, state.movement.physicalLedger, state.adapter.compactor),
            "COMPACTOR return phase advances");
    }
    Check(state.adapter.compactor.active == 0, "COMPACTOR completes at 300 ticks");

    state.encounterIndex = 2;
    state.keeper.destroyedStructures = 2;
    Check(bp::PopulateAct2Encounter(state), "FRAME population succeeds");
    Check(state.movement.physicalLedger.mass[AccountIndex(bp::PhysicalAccount::Cover)] == 320
        && state.movement.physicalLedger.mass[AccountIndex(bp::PhysicalAccount::Neutral)] == 160,
        "two destroyed KEEPER structures open two FRAME panels without mass loss");
}

void TestRuntimeReachabilityAndPixels() noexcept {
    bp::NarrativeState narrative{};
    const bp::ClaimLedger claims = Act2Claims(narrative);
    bp::Act2Runtime state{};
    Check(bp::InitializeAct2Runtime(state, claims, narrative), "runtime reachability initializes");
    bp::InputFrame input = Input();
    for (std::uint16_t tick = 0; tick < 270; ++tick) {
        Check(bp::TickAct2Runtime(state, input), "runtime intro and objective advance");
    }
    Check(state.scene == bp::Act2Scene::Playing, "KEEPER becomes playable");
    Check(bp::ForceAct2EncounterSuccessForTest(state), "KEEPER forced deterministic success");
    input = Input(true);
    for (std::uint16_t tick = 0; tick < 31; ++tick) Check(bp::TickAct2Runtime(state, input), "KEEPER confirm hold");
    input = Input();
    for (std::uint16_t tick = 0; tick < 216; ++tick) Check(bp::TickAct2Runtime(state, input), "KEEPER response return");
    Check(state.encounterIndex == 1, "runtime reaches ADAPTER");

    std::uint32_t pixels[bp::kRenderWidth * bp::kRenderHeight]{};
    bp::PixelBuffer buffer{pixels, bp::kRenderWidth, bp::kRenderHeight, bp::kRenderWidth};
    bp::RenderAct2Runtime(buffer, state);
    std::uint32_t nonBackground = 0;
    for (std::uint32_t pixel : pixels) if (pixel != bp::Color(bp::PaletteColor::Background)) ++nonBackground;
    Check(nonBackground > 100, "Act 2 runtime emits visible exact-palette pixels");

    bp::Act2Runtime clone = state;
    Check(bp::Act2RuntimeHash(state) == bp::Act2RuntimeHash(clone),
        "Act 2 state hash deterministic for identical state");
    ++clone.globalTick;
    Check(bp::Act2RuntimeHash(state) != bp::Act2RuntimeHash(clone),
        "Act 2 state hash covers runtime tick");
}

struct Runner {
    Runner() noexcept {
        TestDefinitionsExact();
        TestAdapterAllResults();
        TestFrameScoresExhaustive();
        TestFrameActions();
        TestProcessesAndPopulation();
        TestRuntimeReachabilityAndPixels();
        if (gFailures != 0) {
            std::printf("T08A_FAIL\tchecks=%u\tfailures=%u\n", gChecks, gFailures);
            std::exit(1);
        }
        std::printf("T08A_PASS\tchecks=%u\tadapter_ranges=49\tframe_observations=1024\n", gChecks);
    }
};

Runner gRunner;

}  // namespace
