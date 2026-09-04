#include "bp_act1.h"
#include "bp_encounter.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

std::uint32_t gT07Checks;
std::uint32_t gT07Failures;

void Check(bool condition, const char* name) noexcept {
    ++gT07Checks;
    if (!condition) {
        ++gT07Failures;
        std::printf("T07_FAIL\t%s\n", name);
    }
}

constexpr std::uint8_t AccountIndex(bp::PhysicalAccount account) noexcept {
    return static_cast<std::uint8_t>(account);
}

constexpr std::uint8_t OwnerIndex(bp::ClaimOwner owner) noexcept {
    return static_cast<std::uint8_t>(owner);
}

constexpr std::uint32_t ActionIndex(bp::PlatformAction action) noexcept {
    return static_cast<std::uint32_t>(action);
}

bp::InputFrame Frame() noexcept {
    bp::InputFrame input{};
    input.focused = 1;
    input.mouseX = 300;
    input.mouseY = 90;
    return input;
}

void Press(bp::InputFrame& input, bp::PlatformAction action) noexcept {
    input.actions[ActionIndex(action)].pressed = 1;
    input.actions[ActionIndex(action)].held = 1;
}

void Hold(bp::InputFrame& input, bp::PlatformAction action) noexcept {
    input.actions[ActionIndex(action)].held = 1;
}

bool TickMany(bp::Act1Runtime& state, std::uint32_t count) noexcept {
    for (std::uint32_t index = 0; index < count; ++index) {
        if (!bp::TickAct1Runtime(state, Frame())) return false;
    }
    return true;
}

bool ReachPlaying(bp::Act1Runtime& state) noexcept {
    bp::InputFrame start = Frame();
    Press(start, bp::PlatformAction::Confirm);
    if (!bp::TickAct1Runtime(state, start)
        || !TickMany(state, bp::kTitleBorrowTicks)
        || !TickMany(state, 1)
        || !TickMany(state, bp::kLedgerRowUnfoldTicks)) {
        return false;
    }
    if (state.scene != bp::Act1Scene::EncounterIntro) return false;
    if (!TickMany(state, bp::kEncounterIntroMinimumTicks - 1)) return false;
    bp::InputFrame skip = Frame();
    Press(skip, bp::PlatformAction::Confirm);
    if (!bp::TickAct1Runtime(state, skip)
        || !TickMany(state, bp::kObjectiveCardTicks)) {
        return false;
    }
    return state.scene == bp::Act1Scene::Playing;
}

bool ReachSettlement(bp::Act1Runtime& state) noexcept {
    if (!bp::ForceAct1EncounterSuccessForTest(state)) return false;
    return TickMany(state, bp::kHitFreezeTicks)
        && TickMany(state, bp::kSettlementWaitTicks)
        && state.scene == bp::Act1Scene::Settlement;
}

bool FocusChoice(bp::Act1Runtime& state, bp::ChoiceId choice) noexcept {
    if (state.focusedChoice == choice) return true;
    bp::InputFrame select = Frame();
    Press(select,
        choice == bp::ChoiceId::First
            ? bp::PlatformAction::SelectLeft
            : (choice == bp::ChoiceId::Second
                ? bp::PlatformAction::SelectUp
                : bp::PlatformAction::SelectRight));
    if (!bp::TickAct1Runtime(state, select)
        || state.scene != bp::Act1Scene::Preview
        || !TickMany(state, bp::kAllocationPreviewTicks)) {
        return false;
    }
    return state.scene == bp::Act1Scene::Settlement && state.focusedChoice == choice;
}

bool CommitChoice(bp::Act1Runtime& state, bp::ChoiceId choice) noexcept {
    if (!FocusChoice(state, choice)) return false;
    bp::InputFrame held = Frame();
    Hold(held, bp::PlatformAction::Confirm);
    if (!bp::TickAct1Runtime(state, held)) return false;
    for (std::uint16_t tick = 0; tick < bp::kConfirmHoldTicks; ++tick) {
        if (!bp::TickAct1Runtime(state, held)) return false;
    }
    return state.scene == bp::Act1Scene::CommitTransit
        && TickMany(state, bp::kCommitTransitTicks);
}

bool FinishResponseToNextPlaying(bp::Act1Runtime& state) noexcept {
    if (state.scene != bp::Act1Scene::EntityResponse
        || !TickMany(state, bp::kEntityResponseMinimumTicks - 1)) return false;
    bp::InputFrame skip = Frame();
    Press(skip, bp::PlatformAction::Confirm);
    if (!bp::TickAct1Runtime(state, skip)
        || !TickMany(state, bp::kLedgerReturnTicks)
        || state.scene != bp::Act1Scene::LedgerFocus
        || !TickMany(state, 1 + bp::kLedgerRowUnfoldTicks)
        || !TickMany(state, bp::kEncounterIntroMinimumTicks - 1)) return false;
    Press(skip, bp::PlatformAction::Confirm);
    if (!bp::TickAct1Runtime(state, skip)
        || !TickMany(state, bp::kObjectiveCardTicks)) return false;
    return state.scene == bp::Act1Scene::Playing;
}

bool ReachVesselPlaying(bp::Act1Runtime& state) noexcept {
    if (!bp::InitializeAct1Runtime(state) || !ReachPlaying(state)) return false;
    if (!ReachSettlement(state)
        || !CommitChoice(state, bp::ChoiceId::Second)
        || !FinishResponseToNextPlaying(state)) return false;
    return ReachSettlement(state)
        && CommitChoice(state, bp::ChoiceId::Second)
        && FinishResponseToNextPlaying(state)
        && state.encounterIndex == 2;
}

void TestAuthorityData() noexcept {
    Check(bp::LargeOpponentMaximumSpeedQ8(0) == bp::PixelsPerSecondToQ8PerTick(98),
        "large opponent Shell zero speed is 98px/s");
    Check(bp::LargeOpponentMaximumSpeedQ8(384) == bp::PixelsPerSecondToQ8PerTick(60)
        && bp::LargeOpponentMaximumSpeedQ8(65535) == bp::PixelsPerSecondToQ8PerTick(60),
        "large opponent Shell 384+ speed is 60px/s");
    constexpr std::int16_t playerX[3] = {48, 48, 42};
    constexpr std::int16_t playerY[3] = {92, 92, 92};
    constexpr std::int16_t opponentX[3] = {270, 248, 264};
    constexpr std::int16_t opponentY[3] = {90, 90, 90};
    constexpr std::uint16_t opponentMass[3] = {48, 64, 224};
    constexpr std::uint16_t fieldMass[3] = {160, 160, 320};
    constexpr std::uint16_t neutralMass[3] = {64, 64, 128};
    constexpr std::uint8_t fieldCount[3] = {5, 5, 10};
    for (std::uint8_t index = 0; index < bp::kAct1EncounterCount; ++index) {
        const bp::Act1EncounterDef& definition = bp::GetAct1EncounterDef(index);
        Check(bp::ValidateAct1EncounterDef(definition), "act1 definition validates");
        Check(definition.playerX == playerX[index] && definition.playerY == playerY[index], "act1 player spawn exact");
        Check(definition.opponentX == opponentX[index] && definition.opponentY == opponentY[index], "act1 opponent spawn exact");
        Check(definition.opponentMass == opponentMass[index], "act1 opponent mass exact");
        Check(definition.fieldMass == fieldMass[index], "act1 field mass exact");
        Check(definition.neutralMass == neutralMass[index], "act1 neutral mass exact");
        Check(definition.fieldCount == fieldCount[index], "act1 field count exact");

        bp::ClaimLedger claims = bp::MakeInitialClaimLedger();
        bp::PhysicalLedger physical{};
        bp::CombatState combat{};
        bp::SlotHandle neutrals[bp::kAct1NeutralCount]{};
        Check(bp::InitializeAct1Physical(physical, claims, definition), "act1 physical initialization");
        Check(bp::PhysicalLedgerTotal(physical) == 1440, "act1 physical total exact");
        Check(bp::PopulateAct1Combat(combat, physical, definition, neutrals), "act1 entities populate");
        Check(bp::CombatStateValid(combat, physical), "act1 populated combat valid");
    }
    const bp::Act1EncounterDef& patch = bp::GetAct1EncounterDef(0);
    Check(patch.field[0].centerX == 160 && patch.field[0].centerY == 90
        && patch.field[0].width == 14 && patch.field[0].height == 14, "patch P0 exact");
    Check(patch.neutral[0].x == 234 && patch.neutral[0].y == 54
        && patch.neutral[3].x == 234 && patch.neutral[3].y == 126, "patch feed coordinates exact");
    const bp::Act1EncounterDef& pair = bp::GetAct1EncounterDef(1);
    Check(pair.field[2].centerX == 118 && pair.field[2].height == 42, "pair P2 exact");
    Check(pair.neutral[7].x == 172 && pair.neutral[7].y == 90, "pair neutral exact");
    const bp::Act1EncounterDef& vessel = bp::GetAct1EncounterDef(2);
    Check(vessel.field[9].centerX == 194 && vessel.field[9].centerY == 108, "vessel P9 exact");
    Check(vessel.neutral[7].x == 222 && vessel.neutral[7].y == 112, "vessel neutral exact");
}

void TestSceneTiming() noexcept {
    bp::Act1Runtime state{};
    Check(bp::InitializeAct1Runtime(state), "runtime initializes");
    Check(state.scene == bp::Act1Scene::Title && state.initializedEncounter == 0, "title has no player entity");
    bp::InputFrame input = Frame();
    Press(input, bp::PlatformAction::Confirm);
    Check(bp::TickAct1Runtime(state, input), "title accepts borrow input");
    Check(state.scene == bp::Act1Scene::BorrowAnimation, "title enters borrow animation");
    Check(TickMany(state, bp::kTitleBorrowTicks - 1), "borrow runs before exact end");
    Check(state.scene == bp::Act1Scene::BorrowAnimation, "borrow holds for 72 ticks");
    Check(TickMany(state, 1), "borrow exact final tick");
    Check(state.scene == bp::Act1Scene::LedgerFocus && state.initializedEncounter != 0, "borrow builds player after 72 ticks");
    Check(TickMany(state, 1 + bp::kLedgerRowUnfoldTicks), "ledger row unfolds exact");
    Check(state.scene == bp::Act1Scene::EncounterIntro, "row unfold enters intro");
    input = Frame();
    Press(input, bp::PlatformAction::Confirm);
    Check(bp::TickAct1Runtime(state, input), "early intro confirm accepted as input only");
    Check(state.scene == bp::Act1Scene::EncounterIntro, "first intro cannot skip before 90");
    Check(TickMany(state, bp::kEncounterIntroMinimumTicks - 2), "intro reaches tick 89");
    Check(state.scene == bp::Act1Scene::EncounterIntro, "intro remains through tick 89");
    input = Frame(); Press(input, bp::PlatformAction::Confirm);
    Check(bp::TickAct1Runtime(state, input), "intro confirm at minimum");
    Check(state.scene == bp::Act1Scene::ObjectiveCard, "intro skips at tick 90");
    Check(TickMany(state, bp::kObjectiveCardTicks), "objective exact timing");
    Check(state.scene == bp::Act1Scene::Playing, "objective enters play");
}

void TestPatchFeedTransit() noexcept {
    bp::Act1Runtime state{};
    Check(bp::InitializeAct1Runtime(state) && ReachPlaying(state), "patch reaches play");
    bp::SlotHandle extra{};
    Check(bp::AddNeutral(state.combat, bp::Vec2Q8{270 * bp::kQ8One, 90 * bp::kQ8One}, 8, 0, 0, extra), "patch test adds stripped shell entity");
    Check(bp::PhysicalTransfer(state.movement.physicalLedger, bp::PhysicalAccount::OpponentShell, bp::PhysicalAccount::Neutral, 8), "patch test strips exact shell threshold");
    Check(bp::CombatStateValid(state.combat, state.movement.physicalLedger), "patch stripped state valid");
    Check(TickMany(state, 1), "patch starts nearest feed transit");
    Check(state.patch.transitActive != 0
        && state.movement.physicalLedger.mass[AccountIndex(bp::PhysicalAccount::Transit)] == 8, "patch feed is 8 transit");
    Check(TickMany(state, bp::kPatchFeedTransitTicks - 1), "patch feed completes in 18 ticks");
    Check(state.patch.feedsUsed == 1 && state.patch.transitActive == 0, "patch feed completion count exact");
    Check(bp::PhysicalLedgerTotal(state.movement.physicalLedger) == 1440, "patch feed preserves physical total");

    bp::Act1Runtime partial{};
    Check(bp::InitializeAct1Runtime(partial) && ReachPlaying(partial),
        "partial feed path reaches patch");
    const std::uint16_t nearestFeed = partial.patch.feed[1].index;
    partial.combat.neutrals[nearestFeed].mass = 7;
    partial.combat.neutrals[nearestFeed].patchFeedMass = 7;
    Check(bp::PhysicalTransfer(partial.movement.physicalLedger,
        bp::PhysicalAccount::Neutral, bp::PhysicalAccount::PlayerShell, 1),
        "partial feed models one reclaimed tile");
    bp::SlotHandle partialStrip{};
    Check(bp::AddNeutral(partial.combat,
        partial.combat.actors[1].position, 8, 0, 0, partialStrip),
        "partial feed stores opponent shell loss");
    Check(bp::PhysicalTransfer(partial.movement.physicalLedger,
        bp::PhysicalAccount::OpponentShell, bp::PhysicalAccount::Neutral, 8),
        "partial feed strips threshold shell");
    Check(TickMany(partial, 1), "partial feed skips undersized nearest feed");
    Check(partial.scene == bp::Act1Scene::Playing
        && partial.patch.transitActive != 0
        && partial.combat.neutrals[partial.patch.transitNeutralIndex].patchFeedMass == 0,
        "partial feed selects next complete tagged eight without Fatal");

    bp::Act1Runtime merged{};
    Check(bp::InitializeAct1Runtime(merged) && ReachPlaying(merged),
        "feed merge path reaches patch");
    const std::uint16_t feedA = merged.patch.feed[0].index;
    const std::uint16_t feedB = merged.patch.feed[1].index;
    merged.combat.neutrals[feedB].position = merged.combat.neutrals[feedA].position;
    Check(TickMany(merged, 1), "patch executes common neutral merge step");
    std::uint16_t mergedFeedMass = 0;
    for (const bp::NeutralSlot& neutral : merged.combat.neutrals) {
        if (neutral.active != 0 && neutral.patchFeedMass > mergedFeedMass) {
            mergedFeedMass = neutral.patchFeedMass;
        }
    }
    Check(mergedFeedMass == 16, "patch merge preserves both feed tags in survivor");
}

void TestSettlementFlowAndVesselMinimum() noexcept {
    bp::Act1Runtime state{};
    Check(bp::InitializeAct1Runtime(state) && ReachPlaying(state), "flow reaches patch");
    Check(ReachSettlement(state), "patch reaches settlement through 5+42");
    Check(CommitChoice(state, bp::ChoiceId::First), "patch commits reclaim origin after 30+36");
    Check(state.scene == bp::Act1Scene::EntityResponse, "patch response follows commit");
    Check(state.claimLedger.mass[OwnerIndex(bp::ClaimOwner::Player)] == 16, "patch transfers 16 claim to player");
    Check(state.narrative.role == 1, "patch settlement assigns CLAIMANT role");
    Check(FinishResponseToNextPlaying(state) && state.encounterIndex == 1, "flow reaches pair playing");
    Check(ReachSettlement(state), "pair reaches settlement");
    Check(CommitChoice(state, bp::ChoiceId::Second), "pair commits separate both");
    Check(state.narrative.entityStatus[1] == 2, "pair status TWO recorded");
    Check(FinishResponseToNextPlaying(state) && state.encounterIndex == 2, "flow reaches vessel playing");
    Check(ReachSettlement(state), "vessel reaches minimum settlement");
    Check(state.settlementStage == 1, "vessel minimum is distinct settlement stage");
    Check(CommitChoice(state, bp::ChoiceId::Second), "vessel chooses continue after minimum");
    Check(state.scene == bp::Act1Scene::Settlement && state.settlementStage == 2, "continue opens additional 64 settlement");
    Check(state.claimLedger.mass[OwnerIndex(bp::ClaimOwner::Player)] == 144, "vessel minimum makes player exactly 144");
    Check(state.claimLedger.mass[OwnerIndex(bp::ClaimOwner::System)] == 256, "vessel minimum moves title claim 64 from system");
    Check(state.narrative.role == 2 && state.narrative.encounterIndex == 3,
        "vessel minimum assigns HOLDER and advances encounter record");
    Check(CommitChoice(state, bp::ChoiceId::Third), "vessel commits release");
    Check(state.claimLedger.mass[OwnerIndex(bp::ClaimOwner::Player)] == 144, "release preserves player minimum");
    Check(state.claimLedger.mass[OwnerIndex(bp::ClaimOwner::Common)] == 64, "release moves additional vessel 64 to common");
    Check(bp::ClaimLedgerTotal(state.claimLedger) == 1440, "full act1 flow preserves claim total");
    Check(state.scene == bp::Act1Scene::EntityResponse, "vessel response reachable");

    bp::Act1Runtime enough{};
    Check(bp::InitializeAct1Runtime(enough) && ReachPlaying(enough), "enough path reaches patch");
    Check(ReachSettlement(enough) && CommitChoice(enough, bp::ChoiceId::Second)
        && FinishResponseToNextPlaying(enough), "enough path clears patch");
    Check(ReachSettlement(enough) && CommitChoice(enough, bp::ChoiceId::Second)
        && FinishResponseToNextPlaying(enough), "enough path clears pair");
    Check(ReachSettlement(enough) && CommitChoice(enough, bp::ChoiceId::First), "remain commits at vessel minimum");
    Check(enough.scene == bp::Act1Scene::EnoughEnding, "remain reaches enough ending");
    Check(enough.claimLedger.mass[OwnerIndex(bp::ClaimOwner::Player)] == 144, "enough ending has exact 144 player claim");
    Check((enough.narrative.endingsSeen & 1u) != 0, "enough ending bit recorded");
}

void TestPairAndVesselMechanics() noexcept {
    bp::Act1Runtime pair{};
    Check(bp::InitializeAct1Runtime(pair) && ReachPlaying(pair), "pair mechanic path reaches patch");
    Check(ReachSettlement(pair)
        && CommitChoice(pair, bp::ChoiceId::Second)
        && FinishResponseToNextPlaying(pair), "pair mechanic path reaches pair");
    std::uint32_t pairTicks = 0;
    while (pair.scene == bp::Act1Scene::Playing && pairTicks < bp::kPairDurationTicks) {
        if (!bp::TickAct1Runtime(pair, Frame())) break;
        ++pairTicks;
    }
    Check(pair.scene == bp::Act1Scene::HitFreeze, "pair probe contact ends in hit freeze");
    Check(pair.pair.forcedSelect != 0 && pair.focusedChoice == bp::ChoiceId::First, "pair core escrow forces SELECT ONE");
    Check(pair.movement.physicalLedger.mass[AccountIndex(bp::PhysicalAccount::OpponentCore)] == 0
        && pair.movement.physicalLedger.mass[AccountIndex(bp::PhysicalAccount::Transit)] == 16,
        "pair contact escrows core 16");
    Check(pairTicks < bp::kPairDurationTicks, "pair probe can reach a core before duration");

    bp::Act1Runtime duration{};
    Check(bp::InitializeAct1Runtime(duration) && ReachPlaying(duration), "pair duration path reaches patch");
    Check(ReachSettlement(duration)
        && CommitChoice(duration, bp::ChoiceId::Second)
        && FinishResponseToNextPlaying(duration), "pair duration path reaches pair");
    for (bp::PairProbe& probe : duration.pair.probes) {
        probe.active = 0;
        probe.respawnTicks = 65535;
        probe.protocolNeutral = bp::SlotHandle{bp::kInvalidSlotIndex, 0};
    }
    Check(TickMany(duration, bp::kPairDurationTicks - 1), "pair duration runs through tick 2699");
    Check(duration.scene == bp::Act1Scene::Playing, "pair does not end before 2700");
    Check(TickMany(duration, 1), "pair duration final tick");
    Check(duration.scene == bp::Act1Scene::HitFreeze && duration.pair.forcedSelect == 0,
        "pair ends at 2700 with all choices unlocked");

    bp::Act1Runtime protocol{};
    Check(bp::InitializeAct1Runtime(protocol) && ReachPlaying(protocol),
        "protocol path reaches patch");
    Check(ReachSettlement(protocol)
        && CommitChoice(protocol, bp::ChoiceId::Second)
        && FinishResponseToNextPlaying(protocol), "protocol path reaches pair");
    protocol.pair.probes[1].active = 0;
    protocol.pair.probes[1].respawnTicks = 65535;
    const bp::Vec2Q8 probePosition = protocol.pair.probes[0].position;
    Check(bp::SpawnScriptedProjectile(
        protocol.combat,
        protocol.movement.physicalLedger,
        bp::CombatSide::Player,
        16,
        bp::Vec2Q8{probePosition.x - 30 * bp::kQ8One, probePosition.y},
        bp::Vec2Q8{60 * bp::kQ8One, 0},
        60 * bp::kQ8One), "protocol path seeds swept probe hit");
    Check(TickMany(protocol, 1), "swept shot crosses pair probe");
    Check(protocol.pair.probes[0].active == 0
        && protocol.pair.probes[0].respawnTicks == 120,
        "probe hit becomes inactive for exact 120 ticks");
    std::uint16_t taggedMass = 0;
    for (const bp::NeutralSlot& neutral : protocol.combat.neutrals) {
        taggedMass = static_cast<std::uint16_t>(taggedMass + neutral.protocolTagMass);
    }
    Check(taggedMass == 16, "probe destruction creates exact PROTOCOL_TAG 16");
    Check(TickMany(protocol, 119), "protocol tag waits first 119 ticks");
    Check(protocol.pair.probes[0].active == 0
        && protocol.pair.probes[0].respawnTicks == 1,
        "probe remains absent through tick 119");
    Check(TickMany(protocol, 1), "protocol tag attempts rebuild at tick 120");
    Check(protocol.pair.probes[0].active != 0
        && protocol.movement.physicalLedger.mass[AccountIndex(bp::PhysicalAccount::ProtocolActive)] == 32,
        "probe rebuilds from tagged neutral at exact tick 120");

    bp::Act1Runtime vessel{};
    Check(ReachVesselPlaying(vessel), "vessel mechanic path reaches vessel");
    vessel.vessel.action = bp::VesselAction::Stamp;
    vessel.vessel.actionTicks = 0;
    vessel.vessel.stampTelegraph = bp::kVesselStampTelegraphTicks;
    vessel.vessel.stampCooldown = 0;
    Check(TickMany(vessel, bp::kVesselStampTelegraphTicks), "vessel stamp telegraph runs 30 ticks");
    bool foundSlab = false;
    for (const bp::ProjectileSlot& shot : vessel.combat.projectiles) {
        if (shot.active != 0 && shot.owner == bp::CombatSide::Opponent
            && shot.mass == bp::kVesselStampMass
            && shot.widthPixels == bp::kVesselStampWidth
            && shot.heightPixels == bp::kVesselStampHeight) {
            foundSlab = true;
        }
    }
    Check(foundSlab, "vessel stamp creates exact 32 mass 24x10 slab");
    Check(vessel.vessel.stampCooldown == bp::kVesselStampCooldownTicks,
        "vessel stamp starts exact 120 tick cooldown");

    bp::Act1Runtime press{};
    Check(ReachVesselPlaying(press), "vessel press path reaches vessel");
    press.vessel.action = bp::VesselAction::Press;
    press.vessel.actionTicks = 0;
    press.vessel.pressCooldown = 0;
    Check(TickMany(press, bp::kVesselPressTicks - 1), "vessel press runs first 59 ticks");
    Check(press.vessel.pressCooldown == 0, "vessel press cooldown waits for duration");
    Check(TickMany(press, 1), "vessel press runs tick 60");
    Check(press.vessel.pressCooldown == bp::kVesselPressCooldownTicks,
        "vessel press starts exact 90 tick cooldown after duration");

    bp::Act1Runtime largeMotion{};
    Check(ReachVesselPlaying(largeMotion), "large opponent motion path reaches vessel");
    largeMotion.vessel.action = bp::VesselAction::Press;
    largeMotion.vessel.actionTicks = 0;
    largeMotion.vessel.pressCooldown = 0;
    largeMotion.vessel.pressDirectionQ15 = bp::Vec2Q8{-bp::kInputUnit, 0};
    largeMotion.combat.actors[1].velocity = bp::Vec2Q8{0, 0};
    Check(TickMany(largeMotion, 1), "large opponent first press tick");
    Check(largeMotion.combat.actors[1].velocity.x
            == -bp::PixelsPerSecondSquaredToQ8PerTickSquared(420),
        "vessel uses exact large-opponent 420 acceleration");
    largeMotion.vessel.action = bp::VesselAction::Press;
    largeMotion.vessel.actionTicks = bp::kVesselPressTicks;
    largeMotion.vessel.pressCooldown = 1;
    largeMotion.encounterTick = 1;
    largeMotion.combat.actors[1].velocity = bp::Vec2Q8{0, 0};
    const bp::Vec2Q8 stoppedPosition = largeMotion.combat.actors[1].position;
    Check(TickMany(largeMotion, 1), "press cooldown expires between decisions");
    Check(largeMotion.combat.actors[1].position.x == stoppedPosition.x
        && largeMotion.combat.actors[1].position.y == stoppedPosition.y
        && largeMotion.vessel.pressCooldown == 0,
        "expired press direction stays inert until a new decision");

    bp::Act1Runtime blockedThreat{};
    Check(ReachVesselPlaying(blockedThreat), "blocked threat path reaches vessel");
    Check(bp::PhysicalTransfer(blockedThreat.movement.physicalLedger,
        bp::PhysicalAccount::LockedBands, bp::PhysicalAccount::Cover, 8),
        "blocked threat funds test cover");
    bp::SlotHandle threatCover{};
    Check(bp::AddCover(blockedThreat.combat, bp::RectPixels{220, 85, 8, 10}, 8, threatCover),
        "blocked threat adds earlier cover");
    Check(bp::SpawnScriptedProjectile(blockedThreat.combat,
        blockedThreat.movement.physicalLedger, bp::CombatSide::Player, 16,
        bp::Vec2Q8{200 * bp::kQ8One, 90 * bp::kQ8One},
        bp::Vec2Q8{4 * bp::kQ8One, 0}, 4 * bp::kQ8One),
        "blocked threat seeds covered trajectory");
    blockedThreat.vessel.action = bp::VesselAction::Press;
    blockedThreat.vessel.actionTicks = bp::kVesselPressTicks;
    blockedThreat.vessel.pressCooldown = 0;
    blockedThreat.vessel.braceCooldown = 0;
    blockedThreat.encounterTick = 0;
    Check(TickMany(blockedThreat, 1), "covered threat decision tick");
    Check(blockedThreat.vessel.action != bp::VesselAction::Brace,
        "cover impact prevents false 30-tick BRACE threat");

    bp::Act1Runtime brace{};
    Check(ReachVesselPlaying(brace), "vessel brace path reaches vessel");
    brace.vessel.action = bp::VesselAction::Brace;
    brace.vessel.actionTicks = 0;
    brace.vessel.braceCooldown = bp::kVesselBraceCooldownTicks;
    const bp::Vec2Q8 vesselPosition = brace.combat.actors[1].position;
    Check(bp::SpawnScriptedProjectile(
        brace.combat,
        brace.movement.physicalLedger,
        bp::CombatSide::Player,
        16,
        bp::Vec2Q8{vesselPosition.x - 40 * bp::kQ8One, vesselPosition.y},
        bp::Vec2Q8{80 * bp::kQ8One, 0},
        80 * bp::kQ8One), "brace path seeds crossing projectile");
    Check(TickMany(brace, 1), "brace processes crossing projectile");
    bool deflectedNeutral = false;
    for (const bp::NeutralSlot& neutral : brace.combat.neutrals) {
        if (neutral.active != 0 && neutral.playerMissMass == 16
            && neutral.velocity.y != 0) deflectedNeutral = true;
    }
    Check(deflectedNeutral, "brace converts swept front contact to tangent neutral");

    bp::Act1Runtime braceCover{};
    Check(ReachVesselPlaying(braceCover), "brace cover-order path reaches vessel");
    Check(bp::PhysicalTransfer(braceCover.movement.physicalLedger,
        bp::PhysicalAccount::LockedBands, bp::PhysicalAccount::Cover, 8),
        "brace cover-order funds test cover");
    bp::SlotHandle braceCoverHandle{};
    Check(bp::AddCover(braceCover.combat, bp::RectPixels{220, 85, 8, 10}, 8, braceCoverHandle),
        "brace cover-order adds earlier cover");
    Check(bp::SpawnScriptedProjectile(braceCover.combat,
        braceCover.movement.physicalLedger, bp::CombatSide::Player, 16,
        bp::Vec2Q8{180 * bp::kQ8One, 90 * bp::kQ8One},
        bp::Vec2Q8{100 * bp::kQ8One, 0}, 100 * bp::kQ8One),
        "brace cover-order seeds crossing shot");
    braceCover.vessel.action = bp::VesselAction::Brace;
    braceCover.vessel.actionTicks = 0;
    braceCover.vessel.braceCooldown = bp::kVesselBraceCooldownTicks;
    Check(TickMany(braceCover, 1), "brace respects common earliest impact");
    Check(!bp::CoverHandleValid(braceCover.combat, braceCoverHandle),
        "earlier cover receives shot before BRACE ring");
}

void TestRetryHashAndVesselPixels() noexcept {
    bp::Act1Runtime retry{};
    Check(bp::InitializeAct1Runtime(retry) && ReachPlaying(retry), "retry path reaches play");
    bp::SlotHandle stripped{};
    Check(bp::AddNeutral(
        retry.combat,
        retry.combat.actors[0].position,
        24, 0, 0, stripped), "retry path stores stripped shell");
    Check(bp::PhysicalTransfer(
        retry.movement.physicalLedger,
        bp::PhysicalAccount::PlayerShell,
        bp::PhysicalAccount::Neutral,
        24), "retry path exposes player core");
    Check(bp::SpawnScriptedProjectile(
        retry.combat,
        retry.movement.physicalLedger,
        bp::CombatSide::Opponent,
        8,
        retry.combat.actors[0].position,
        bp::Vec2Q8{-bp::kInputUnit, 0},
        bp::ProjectileSpeedQ8(8)), "retry path seeds core contact");
    Check(bp::TickAct1Runtime(retry, Frame()), "player core hit tick succeeds");
    Check(retry.scene == bp::Act1Scene::HitFreeze && retry.retryPending != 0,
        "player core hit enters retry freeze without immediate reset");
    Check(retry.audioCoreHitSerial==1,
        "Act1 core contact increments the platform audio observer exactly once");
    Check(TickMany(retry, bp::kHitFreezeTicks), "retry freeze lasts exact five ticks");
    Check(retry.scene == bp::Act1Scene::EncounterRetry, "retry uses explicit encounter retry scene");
    Check(TickMany(retry, 1), "retry rebuilds encounter once");
    Check(retry.scene == bp::Act1Scene::EncounterIntro && retry.introRevisit != 0,
        "retry enters revisit intro");
    Check(TickMany(retry, bp::kEncounterRevisitIntroMinimumTicks - 1),
        "revisit intro holds before tick eighteen");
    bp::InputFrame confirm = Frame(); Press(confirm, bp::PlatformAction::Confirm);
    Check(bp::TickAct1Runtime(retry, confirm)
        && retry.scene == bp::Act1Scene::ObjectiveCard,
        "revisit intro confirms at exact tick eighteen");

    bp::Act1Runtime hashed{};
    Check(bp::InitializeAct1Runtime(hashed) && ReachPlaying(hashed), "hash path reaches play");
    const std::uint32_t baseline = bp::Act1RuntimeHash(hashed);
    bp::Act1Runtime changed = hashed;
    ++changed.claimLedger.mass[0];
    Check(bp::Act1RuntimeHash(changed) != baseline, "hash includes claim ledger");
    changed = hashed; ++changed.narrative.role;
    Check(bp::Act1RuntimeHash(changed) != baseline, "hash includes narrative state");
    changed = hashed; ++changed.movement.dashCooldown;
    Check(bp::Act1RuntimeHash(changed) != baseline, "hash includes movement state");
    changed = hashed; ++changed.combat.charges[0].generation;
    Check(bp::Act1RuntimeHash(changed) != baseline, "hash includes charge slots");
    changed = hashed; ++changed.combat.projectiles[0].generation;
    Check(bp::Act1RuntimeHash(changed) != baseline, "hash includes projectile slots");
    changed = hashed; ++changed.combat.neutrals[0].velocity.x;
    Check(bp::Act1RuntimeHash(changed) != baseline, "hash includes neutral velocity");
    changed = hashed; ++changed.combat.covers[0].generation;
    Check(bp::Act1RuntimeHash(changed) != baseline, "hash includes cover slots");
    changed = hashed; ++changed.combat.actors[0].position.x;
    Check(bp::Act1RuntimeHash(changed) != baseline, "hash includes combat actors");
    changed = hashed; ++changed.combat.settlement.tick;
    Check(bp::Act1RuntimeHash(changed) != baseline, "hash includes settlement snapshot");
    changed = hashed; ++changed.combat.shellDamageThisTick[0];
    Check(bp::Act1RuntimeHash(changed) != baseline, "hash includes shell-hit events");
    changed = hashed; ++changed.combat.nextStableId;
    Check(bp::Act1RuntimeHash(changed) != baseline, "hash includes combat metadata");
    changed = hashed; ++changed.reclaim.accumulator[0];
    Check(bp::Act1RuntimeHash(changed) != baseline, "hash includes reclaim state");
    changed = hashed; ++changed.hudLoan.modules[0].maturityRemaining;
    Check(bp::Act1RuntimeHash(changed) != baseline, "hash includes HUD maturity");
    changed = hashed; ++changed.neutralHandles[0].generation;
    Check(bp::Act1RuntimeHash(changed) != baseline, "hash includes encounter handles");
    changed = hashed; ++changed.patch.transitTicks;
    Check(bp::Act1RuntimeHash(changed) != baseline, "hash includes patch transit");
    changed = hashed; ++changed.pair.probes[0].respawnTicks;
    Check(bp::Act1RuntimeHash(changed) != baseline, "hash includes pair state");
    changed = hashed; ++changed.vessel.actionTicks;
    Check(bp::Act1RuntimeHash(changed) != baseline, "hash includes vessel state");
    changed = hashed; ++changed.retryPending;
    Check(bp::Act1RuntimeHash(changed) != baseline, "hash includes scene control state");

    bp::Act1Runtime vessel{};
    Check(ReachVesselPlaying(vessel), "vessel pixel path reaches play");
    std::uint32_t pixels[bp::kRenderWidth * bp::kRenderHeight]{};
    const bp::PixelBuffer buffer{pixels, bp::kRenderWidth, bp::kRenderHeight, bp::kRenderWidth};
    vessel.vessel.action = bp::VesselAction::Stamp;
    vessel.vessel.stampTelegraph = 1;
    bp::RenderAct1Runtime(buffer, vessel);
    const std::int32_t midX = (vessel.combat.actors[0].position.x
        + vessel.combat.actors[1].position.x) / (2 * bp::kQ8One);
    const std::int32_t midY = (vessel.combat.actors[0].position.y
        + vessel.combat.actors[1].position.y) / (2 * bp::kQ8One);
    Check(pixels[midY * bp::kRenderWidth + midX] == bp::Color(bp::PaletteColor::Warning),
        "stamp telegraph renders warning centerline");
    vessel.vessel.action = bp::VesselAction::Brace;
    bp::RenderAct1Runtime(buffer, vessel);
    const std::int32_t radius = bp::ActorRadiusPixels(
        vessel.movement.physicalLedger.mass[AccountIndex(bp::PhysicalAccount::OpponentShell)]);
    const std::int32_t ringX = vessel.combat.actors[1].position.x / bp::kQ8One - radius - 2;
    const std::int32_t ringY = vessel.combat.actors[1].position.y / bp::kQ8One - radius - 2;
    Check(pixels[ringY * bp::kRenderWidth + ringX] == bp::Color(bp::PaletteColor::Warning),
        "brace renders rectangular warning ring");
}

void TestPauseDashAndBodyIntegration() noexcept {
    bp::Act1Runtime paused{};
    Check(bp::InitializeAct1Runtime(paused) && ReachPlaying(paused),
        "pause path reaches patch");
    paused.patch.transitActive = 1;
    paused.patch.transitTicks = 5;
    const std::uint32_t encounterBeforePause = paused.encounterTick;
    const std::uint32_t globalBeforePause = paused.globalTick;
    bp::InputFrame pause = Frame(); Press(pause, bp::PlatformAction::Pause);
    Check(bp::TickAct1Runtime(paused, pause), "pause edge accepted");
    Check(TickMany(paused, 30), "paused calls remain valid");
    Check(paused.encounterTick == encounterBeforePause
        && paused.globalTick == globalBeforePause
        && paused.patch.transitTicks == 6,
        "pause entry runs step-02 transit once, then freezes simulation");
    Check(bp::TickAct1Runtime(paused, pause) && paused.movement.paused == 0,
        "pause edge resumes without consuming simulation time");

    bp::Act1Runtime pairPause{};
    Check(bp::InitializeAct1Runtime(pairPause) && ReachPlaying(pairPause)
        && ReachSettlement(pairPause)
        && CommitChoice(pairPause, bp::ChoiceId::Second)
        && FinishResponseToNextPlaying(pairPause), "pair pause path reaches pair");
    const std::uint32_t pairTickBefore = pairPause.encounterTick;
    Check(bp::TickAct1Runtime(pairPause, pause) && TickMany(pairPause, 120),
        "pair pause holds for 120 calls");
    Check(pairPause.encounterTick == pairTickBefore
        && pairPause.scene == bp::Act1Scene::Playing,
        "pause cannot consume PAIR 2700-tick duration");

    bp::Act1Runtime vesselPause{};
    Check(ReachVesselPlaying(vesselPause), "vessel pause path reaches vessel");
    vesselPause.vessel.action = bp::VesselAction::Stamp;
    vesselPause.vessel.stampTelegraph = 17;
    vesselPause.vessel.actionTicks = 4;
    Check(bp::TickAct1Runtime(vesselPause, pause) && TickMany(vesselPause, 60),
        "vessel pause holds for 60 calls");
    Check(vesselPause.vessel.stampTelegraph == 16
        && vesselPause.vessel.actionTicks == 4,
        "pause entry runs step-02 telegraph once, then freezes action age");

    bp::Act1Runtime dashCover{};
    Check(bp::InitializeAct1Runtime(dashCover) && ReachPlaying(dashCover),
        "dash cover path reaches patch");
    dashCover.movement.playerPosition = bp::Vec2Q8{144 * bp::kQ8One, 90 * bp::kQ8One};
    dashCover.combat.actors[0].position = dashCover.movement.playerPosition;
    bp::InputFrame dash = Frame(); dash.moveX = 32767;
    Press(dash, bp::PlatformAction::Dash);
    Check(bp::TickAct1Runtime(dashCover, dash), "dash enters adjacent cover");
    Check(dashCover.movement.dashTicksRemaining == 0
        && dashCover.movement.dashInvulnerableRemaining == 0,
        "cover contact discards remaining dash and immunity");

    bp::Act1Runtime dashImmune{};
    Check(bp::InitializeAct1Runtime(dashImmune) && ReachPlaying(dashImmune),
        "dash immunity path reaches patch");
    const std::uint16_t shellBefore = dashImmune.movement.physicalLedger.mass[
        AccountIndex(bp::PhysicalAccount::PlayerShell)];
    Check(bp::SpawnScriptedProjectile(
        dashImmune.combat, dashImmune.movement.physicalLedger,
        bp::CombatSide::Opponent, 8, dashImmune.combat.actors[0].position,
        bp::Vec2Q8{-bp::kInputUnit, 0}, 1), "dash immunity seeds overlapping projectile");
    dash = Frame(); dash.moveX = 32767; Press(dash, bp::PlatformAction::Dash);
    Check(bp::TickAct1Runtime(dashImmune, dash), "dash immunity tick succeeds");
    Check(dashImmune.movement.physicalLedger.mass[AccountIndex(bp::PhysicalAccount::PlayerShell)]
            == shellBefore
        && dashImmune.combat.actors[0].projectileImmune != 0,
        "first dash tick ignores projectile without shell damage");

    bp::Act1Runtime overlap{};
    Check(bp::InitializeAct1Runtime(overlap) && ReachPlaying(overlap),
        "body separation path reaches patch");
    const bp::Vec2Q8 shared{70 * bp::kQ8One, 90 * bp::kQ8One};
    overlap.movement.playerPosition = shared;
    overlap.combat.actors[0].position = shared;
    overlap.combat.actors[1].position = shared;
    Check(TickMany(overlap, 1), "overlapping actors tick");
    const std::int32_t playerShift = shared.x - overlap.combat.actors[0].position.x;
    const std::int32_t opponentShift = overlap.combat.actors[1].position.x - shared.x;
    Check(playerShift > 0 && opponentShift > playerShift,
        "body separation uses opposite mass-weighted displacement");
    const std::int32_t combinedRadius = bp::ActorRadiusPixels(
        overlap.movement.physicalLedger.mass[AccountIndex(bp::PhysicalAccount::PlayerShell)])
        + bp::ActorRadiusPixels(
            overlap.movement.physicalLedger.mass[AccountIndex(bp::PhysicalAccount::OpponentShell)]);
    Check(static_cast<std::uint32_t>(opponentShift + playerShift)
            >= static_cast<std::uint32_t>(combinedRadius * bp::kQ8One - 2),
        "body separation removes overlap without contact damage");

    bp::Act1Runtime pairOverlap{};
    Check(bp::InitializeAct1Runtime(pairOverlap) && ReachPlaying(pairOverlap)
        && ReachSettlement(pairOverlap)
        && CommitChoice(pairOverlap, bp::ChoiceId::Second)
        && FinishResponseToNextPlaying(pairOverlap),
        "pair body-separation path reaches pair");
    pairOverlap.movement.playerPosition = shared;
    pairOverlap.combat.actors[0].position = shared;
    pairOverlap.combat.actors[1].position = shared;
    Check(TickMany(pairOverlap, 1), "pair overlapping actors tick");
    Check(pairOverlap.combat.actors[0].position.x
            != pairOverlap.combat.actors[1].position.x,
        "PAIR applies common actor body separation");

    bp::Act1Runtime vesselOverlap{};
    Check(ReachVesselPlaying(vesselOverlap),
        "vessel body-separation path reaches vessel");
    vesselOverlap.movement.playerPosition = shared;
    vesselOverlap.combat.actors[0].position = shared;
    vesselOverlap.combat.actors[1].position = shared;
    vesselOverlap.vessel.action = bp::VesselAction::Stamp;
    vesselOverlap.vessel.actionTicks = 1;
    vesselOverlap.vessel.stampTelegraph = 0;
    Check(TickMany(vesselOverlap, 1), "vessel overlapping actors tick");
    Check(vesselOverlap.combat.actors[0].position.x
            != vesselOverlap.combat.actors[1].position.x,
        "VESSEL applies common actor body separation");

    bp::Act1Runtime boundaryOverlap{};
    Check(ReachVesselPlaying(boundaryOverlap),
        "boundary-pinned body-separation path reaches vessel");
    const std::uint16_t vesselShell = boundaryOverlap.movement.physicalLedger.mass[
        AccountIndex(bp::PhysicalAccount::OpponentShell)];
    Check(bp::PhysicalTransfer(
        boundaryOverlap.movement.physicalLedger,
        bp::PhysicalAccount::OpponentShell,
        bp::PhysicalAccount::PlayerShell,
        vesselShell),
        "boundary-pinned case transfers all VESSEL Shell to large player");
    Check(boundaryOverlap.movement.physicalLedger.mass[
            AccountIndex(bp::PhysicalAccount::OpponentShell)] == 0,
        "boundary-pinned case uses zero-Shell VESSEL extreme mass ratio");
    const std::int32_t boundaryPlayerRadius = bp::ActorRadiusPixels(
        boundaryOverlap.movement.physicalLedger.mass[AccountIndex(bp::PhysicalAccount::PlayerShell)]);
    const std::int32_t boundaryOpponentRadius = bp::ActorRadiusPixels(
        boundaryOverlap.movement.physicalLedger.mass[AccountIndex(bp::PhysicalAccount::OpponentShell)]);
    const std::int32_t boundaryCombinedRadius = boundaryPlayerRadius + boundaryOpponentRadius;
    const bp::Vec2Q8 pinnedOpponent{
        (bp::kPlayInterior.x + bp::kPlayInterior.width - boundaryOpponentRadius) * bp::kQ8One,
        90 * bp::kQ8One};
    boundaryOverlap.combat.actors[1].position = pinnedOpponent;
    boundaryOverlap.movement.playerPosition = bp::Vec2Q8{
        pinnedOpponent.x - (boundaryCombinedRadius * bp::kQ8One) / 2,
        pinnedOpponent.y};
    boundaryOverlap.combat.actors[0].position = boundaryOverlap.movement.playerPosition;
    boundaryOverlap.vessel.action = bp::VesselAction::Stamp;
    boundaryOverlap.vessel.actionTicks = 1;
    boundaryOverlap.vessel.stampTelegraph = 0;
    Check(TickMany(boundaryOverlap, 1), "boundary-pinned overlapping actors tick");
    const std::int32_t boundaryDistance = boundaryOverlap.combat.actors[1].position.x
        - boundaryOverlap.combat.actors[0].position.x;
    Check(boundaryOverlap.combat.actors[1].position.x == pinnedOpponent.x
        && boundaryDistance >= boundaryCombinedRadius * bp::kQ8One - 2,
        "body separation redistributes boundary-clamped VESSEL displacement");
}

bp::InputFrame ReplayInput(std::uint32_t tick) noexcept {
    bp::InputFrame input = Frame();
    input.moveX = (tick / 45u) % 2u == 0 ? 32767 : -32767;
    input.moveY = (tick / 75u) % 2u == 0 ? 16000 : -16000;
    if (tick % 120u >= 10u && tick % 120u < 38u) Hold(input, bp::PlatformAction::Fire);
    if (tick % 120u == 10u) Press(input, bp::PlatformAction::Fire);
    if (tick % 120u == 38u) input.actions[ActionIndex(bp::PlatformAction::Fire)].released = 1;
    return input;
}

void TestDeterminismAndRendering() noexcept {
    bp::Act1Runtime first{};
    bp::Act1Runtime second{};
    Check(bp::InitializeAct1Runtime(first) && ReachPlaying(first), "determinism first ready");
    Check(bp::InitializeAct1Runtime(second) && ReachPlaying(second), "determinism second ready");
    for (std::uint32_t tick = 0; tick < 360; ++tick) {
        const bp::InputFrame input = ReplayInput(tick);
        const bool firstOk = bp::TickAct1Runtime(first, input);
        const bool secondOk = bp::TickAct1Runtime(second, input);
        if (!firstOk || !secondOk) {
            std::printf("T07_INFO\tfirst_failure_tick=%u\tscene=%u\tcombat_error=%u\tphase=%u\n",
                tick,
                static_cast<unsigned>(first.scene),
                static_cast<unsigned>(first.combat.error),
                static_cast<unsigned>(first.combat.phase));
        }
        Check(firstOk, "determinism first tick");
        Check(secondOk, "determinism second tick");
        if (!firstOk || !secondOk) break;
        Check(bp::Act1RuntimeHash(first) == bp::Act1RuntimeHash(second), "determinism hash each tick");
        Check(bp::ClaimLedgerTotal(first.claimLedger) == 1440, "determinism claim total");
        Check(bp::PhysicalLedgerTotal(first.movement.physicalLedger) == 1440, "determinism physical total");
    }

    std::uint32_t pixels[bp::kRenderWidth * bp::kRenderHeight]{};
    const bp::PixelBuffer buffer{pixels, bp::kRenderWidth, bp::kRenderHeight, bp::kRenderWidth};
    constexpr bp::Act1Scene scenes[] = {
        bp::Act1Scene::Title, bp::Act1Scene::BorrowAnimation,
        bp::Act1Scene::LedgerFocus, bp::Act1Scene::RowUnfold,
        bp::Act1Scene::EncounterIntro,
        bp::Act1Scene::ObjectiveCard, bp::Act1Scene::Playing,
        bp::Act1Scene::HitFreeze, bp::Act1Scene::EncounterRetry,
        bp::Act1Scene::SettlementWait, bp::Act1Scene::Settlement,
        bp::Act1Scene::Preview, bp::Act1Scene::ConfirmHold,
        bp::Act1Scene::CommitTransit, bp::Act1Scene::EntityResponse,
        bp::Act1Scene::LedgerReturn, bp::Act1Scene::EnoughEnding,
        bp::Act1Scene::ContinueReady, bp::Act1Scene::Fatal,
    };
    for (bp::Act1Scene scene : scenes) {
        first.scene = scene;
        bp::RenderAct1Runtime(buffer, first);
        std::uint32_t nonBackground = 0;
        bool paletteOnly = true;
        for (std::uint32_t pixel : pixels) {
            paletteOnly = paletteOnly && bp::IsPaletteColor(pixel);
            if (pixel != bp::Color(bp::PaletteColor::Background)) ++nonBackground;
        }
        Check(paletteOnly, "act1 scene uses palette only");
        Check(nonBackground > 0, "act1 scene renders visible pixels");
    }
}

struct T07TestRunner {
    T07TestRunner() noexcept {
        TestAuthorityData();
        TestSceneTiming();
        TestPatchFeedTransit();
        TestSettlementFlowAndVesselMinimum();
        TestPairAndVesselMechanics();
        TestRetryHashAndVesselPixels();
        TestPauseDashAndBodyIntegration();
        TestDeterminismAndRendering();
        if (gT07Failures != 0) {
            std::printf("T07_FAIL\tchecks=%u\tfailures=%u\n", gT07Checks, gT07Failures);
            std::exit(1);
        }
        std::printf("T07_PASS\tchecks=%u\tencounters=3\tact1_ledgers=1440\n", gT07Checks);
    }
};

T07TestRunner gT07TestRunner;

}  // namespace
