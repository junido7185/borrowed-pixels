#include "bp_hud_loan.h"
#include "bp_physical_ledger.h"
#include "bp_reclaim.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

std::uint32_t gT06Checks;
std::uint32_t gT06Failures;

void Check(bool condition, const char* name) noexcept {
    ++gT06Checks;
    if (!condition) {
        ++gT06Failures;
        std::printf("T06_FAIL\t%s\n", name);
    }
}

constexpr std::uint8_t AccountIndex(bp::PhysicalAccount account) noexcept {
    return static_cast<std::uint8_t>(account);
}

struct CombatWorld {
    bp::CombatState combat;
    bp::PhysicalLedger ledger;
    bp::ReclaimState reclaim;
};

CombatWorld MakeCombatWorld(
    std::uint16_t neutralMass,
    std::uint16_t coverMass = 0,
    bp::Vec2Q8 player = bp::Vec2Q8{40 * bp::kQ8One, 90 * bp::kQ8One},
    bp::Vec2Q8 opponent = bp::Vec2Q8{200 * bp::kQ8One, 90 * bp::kQ8One}) noexcept {
    CombatWorld world{};
    Check(bp::InitializePhysicalLedger(world.ledger, 240, 240, coverMass, neutralMass),
        "T06 physical ledger initialize");
    bp::InitializeCombatState(world.combat, player, opponent);
    bp::InitializeReclaimState(world.reclaim);
    return world;
}

std::uint16_t AddNeutral(CombatWorld& world, bp::Vec2Q8 position, std::uint16_t mass,
    std::uint16_t playerMiss = 0, std::uint16_t opponentMiss = 0) noexcept {
    bp::SlotHandle handle{};
    Check(bp::AddNeutral(world.combat, position, mass, playerMiss, opponentMiss, handle),
        "T06 add neutral");
    return handle.index;
}

std::uint16_t ActiveNeutralCount(const CombatWorld& world) noexcept {
    std::uint16_t count = 0;
    for (const bp::NeutralSlot& neutral : world.combat.neutrals) {
        if (neutral.active != 0) ++count;
    }
    return count;
}

void CheckCombatWorld(const CombatWorld& world, const char* name) noexcept {
    Check(bp::PhysicalLedgerValid(world.ledger), name);
    Check(bp::PhysicalLedgerTotal(world.ledger) == bp::kLedgerTotal,
        "T06 physical total remains 1440");
    Check(bp::CombatStateValid(world.combat, world.ledger),
        "T06 combat aggregate matches ledger");
}

bp::PhysicalLedger MakeHudLedger(std::uint16_t neutralMass = 0) noexcept {
    bp::PhysicalLedger ledger{};
    Check(bp::InitializePhysicalLedger(ledger, 240, 64, 0, neutralMass),
        "HUD physical ledger initialize");
    return ledger;
}

void CheckHud(const bp::HudLoanSystem& hud, const bp::PhysicalLedger& ledger,
    const char* name) noexcept {
    Check(bp::HudLoanSystemValid(hud, ledger), name);
    Check(bp::PhysicalLedgerTotal(ledger) == bp::kLedgerTotal,
        "HUD physical total remains 1440");
}

bool TickHudForTest(
    bp::HudLoanSystem& hud,
    bp::PhysicalLedger& ledger,
    bool held,
    bool shellHit,
    std::uint16_t reclaimedFromPreviousTick) noexcept {
    return bp::AdvanceHudLoanEarly(
            hud, ledger, held, reclaimedFromPreviousTick)
        && bp::ResolvePendingHudCommit(hud, ledger, shellHit);
}

void HoldHud(bp::HudLoanSystem& hud, bp::PhysicalLedger& ledger,
    std::uint16_t ticks, bool shellHitOnLast = false) noexcept {
    for (std::uint16_t tick = 0; tick < ticks; ++tick) {
        Check(TickHudForTest(
            hud,
            ledger,
            true,
            shellHitOnLast && tick + 1 == ticks,
            0), "HUD hold tick");
        CheckHud(hud, ledger, "HUD hold invariant");
    }
}

void IdleHud(bp::HudLoanSystem& hud, bp::PhysicalLedger& ledger,
    std::uint16_t ticks) noexcept {
    for (std::uint16_t tick = 0; tick < ticks; ++tick) {
        Check(TickHudForTest(hud, ledger, false, false, 0), "HUD idle tick");
        CheckHud(hud, ledger, "HUD idle invariant");
    }
}

void TestHudConstantsAndAllStates() noexcept {
    Check(bp::HudModuleMass(bp::HudModuleId::Vital) == 48, "VITAL mass 48");
    Check(bp::HudModuleMass(bp::HudModuleId::Vector) == 80, "VECTOR mass 80");
    Check(bp::HudModuleMass(bp::HudModuleId::Impulse) == 96, "IMPULSE mass 96");
    Check(bp::HudModuleSelectNumber(bp::HudModuleId::Vital) == 1, "VITAL select 1");
    Check(bp::HudModuleSelectNumber(bp::HudModuleId::Vector) == 2, "VECTOR select 2");
    Check(bp::HudModuleSelectNumber(bp::HudModuleId::Impulse) == 3, "IMPULSE select 3");
    Check(bp::kHudDismantleTicks == 45 && bp::kHudRestoreTicks == 60
            && bp::kHudTransitTicks == 36,
        "HUD exact channel and transit ticks");
    Check(bp::kHudMaturityTicks == 720 && bp::kHudWarningTicks == 120,
        "HUD exact maturity and warning ticks");
    Check(bp::kReclaimMovementPercent == 75, "reclaim movement is 75 percent");

    for (std::uint8_t moduleIndex = 0; moduleIndex < bp::kHudModuleCount; ++moduleIndex) {
        const bp::HudModuleId module = static_cast<bp::HudModuleId>(moduleIndex);
        bp::PhysicalLedger ledger = MakeHudLedger();
        bp::HudLoanSystem hud{};
        bp::InitializeHudLoanSystem(hud);
        Check(bp::SelectHudModule(hud, module), "select each HUD module");
        Check(bp::HudModuleFullyInstalled(hud, module), "state INSTALLED for each module");
        Check(bp::HudVisibleMaterialMass(hud, module) == bp::HudModuleMass(module),
            "installed HUD shows full material mass");

        HoldHud(hud, ledger, 1);
        Check(hud.modules[moduleIndex].state == bp::HudLoanState::Dismantling,
            "state DISMANTLING for each module");
        Check(bp::HudFireLocked(hud), "HUD channel wins over fire");
        HoldHud(hud, ledger, 44);
        Check(hud.modules[moduleIndex].state == bp::HudLoanState::ToShellTransit,
            "state TO_SHELL_TRANSIT for each module");
        Check(ledger.mass[AccountIndex(bp::PhysicalAccount::Transit)] == bp::HudModuleMass(module),
            "dismantle commits InstalledHUD to Transit exact");
        Check(bp::HudVisibleMaterialMass(hud, module) == 0,
            "to-shell transit has zero HUD material");

        IdleHud(hud, ledger, 36);
        Check(hud.modules[moduleIndex].state == bp::HudLoanState::Inactive,
            "state INACTIVE for each module");
        Check(bp::OpenHudLoanCount(hud) == 1, "exactly one open HUD loan");
        Check(!bp::HudModuleFullyInstalled(hud, module), "loan removes module feature");

        HoldHud(hud, ledger, 1);
        Check(hud.modules[moduleIndex].state == bp::HudLoanState::Restoring,
            "state RESTORING for each module");
        Check(bp::ShortenOpenHudMaturity(hud, bp::kHudMaturityTicks),
            "maturity can be shortened by policy hook");
        Check(hud.modules[moduleIndex].state == bp::HudLoanState::Due,
            "state DUE for each module");
        Check(bp::HudFireLocked(hud) && bp::HudLoanWarning(hud),
            "DUE warns and locks firing");

        Check(TickHudForTest(
            hud, ledger, false, false, bp::HudModuleMass(module)),
            "DUE automatic full repayment");
        Check(hud.modules[moduleIndex].state == bp::HudLoanState::ToHudTransit,
            "state TO_HUD_TRANSIT for each module");
        IdleHud(hud, ledger, 36);
        Check(hud.modules[moduleIndex].state == bp::HudLoanState::Installed,
            "TO_HUD_TRANSIT returns each module to INSTALLED");
        Check(bp::OpenHudLoanCount(hud) == 0, "restoration closes HUD loan");
        CheckHud(hud, ledger, "seven-state module cycle invariant");
    }
}

void TestHudTimingCancellationAndPartialRepayment() noexcept {
    bp::PhysicalLedger cancelledLedger = MakeHudLedger();
    const bp::PhysicalLedger cancelledBefore = cancelledLedger;
    bp::HudLoanSystem cancelled{};
    bp::InitializeHudLoanSystem(cancelled);
    Check(bp::SelectHudModule(cancelled, bp::HudModuleId::Vital),
        "select VITAL for hit cancellation");
    HoldHud(cancelled, cancelledLedger, 44);
    Check(bp::AdvanceHudLoanEarly(cancelled, cancelledLedger, true, 0),
        "tick 04-06 reserves pending HUD commit before collision");
    Check(bp::HudHasPendingCommit(cancelled),
        "45th dismantle tick remains pending through collision phase");
    Check(std::memcmp(&cancelledLedger, &cancelledBefore, sizeof(cancelledBefore)) == 0,
        "pending pre-collision HUD commit does not mutate physical ledger");
    Check(!bp::SelectHudModule(cancelled, bp::HudModuleId::Vector),
        "selection change is rejected while dismantle commit is pending");
    Check(bp::ResolvePendingHudCommit(cancelled, cancelledLedger, true),
        "tick 17 cancels pending HUD commit after PLAYER Shell hit");
    Check(cancelled.modules[0].state == bp::HudLoanState::Installed,
        "PLAYER Shell hit cancels pending HUD commit");
    Check(std::memcmp(&cancelledLedger, &cancelledBefore, sizeof(cancelledBefore)) == 0,
        "cancelled pending HUD transaction is byte-identical");

    bp::PhysicalLedger releasedLedger = MakeHudLedger();
    bp::HudLoanSystem released{};
    bp::InitializeHudLoanSystem(released);
    Check(TickHudForTest(released, releasedLedger, true, false, 0),
        "begin HUD dismantle for release cancel");
    Check(TickHudForTest(released, releasedLedger, false, false, 0),
        "release HUD dismantle hold");
    Check(released.modules[0].state == bp::HudLoanState::Installed,
        "released dismantle returns installed");

    bp::PhysicalLedger acceleratedLedger = MakeHudLedger();
    bp::HudLoanSystem accelerated{};
    bp::InitializeHudLoanSystem(accelerated);
    Check(TickHudForTest(accelerated, acceleratedLedger, true, false, 0),
        "begin HUD loan before maturity advance");
    Check(bp::ShortenOpenHudMaturity(accelerated, 120),
        "open DISMANTLING loan retains 120-tick maturity advance");
    HoldHud(accelerated, acceleratedLedger, 44);
    Check(accelerated.modules[0].state == bp::HudLoanState::ToShellTransit,
        "accelerated loan enters to-shell transit");
    Check(bp::ShortenOpenHudMaturity(accelerated, 120),
        "open TO_SHELL_TRANSIT loan retains another maturity advance");
    IdleHud(accelerated, acceleratedLedger, 36);
    Check(accelerated.modules[0].maturityRemaining == 479,
        "pre-INACTIVE maturity advances apply before same-tick maturity step");

    bp::PhysicalLedger pendingDueLedger = MakeHudLedger();
    bp::HudLoanSystem pendingDue{};
    bp::InitializeHudLoanSystem(pendingDue);
    HoldHud(pendingDue, pendingDueLedger, 45);
    IdleHud(pendingDue, pendingDueLedger, 36);
    Check(bp::ShortenOpenHudMaturity(pendingDue, 539),
        "prepare exact 180-tick restore maturity boundary");
    Check(pendingDue.modules[0].maturityRemaining == 180,
        "restore boundary starts with maturity 180");
    HoldHud(pendingDue, pendingDueLedger, 59);
    Check(bp::AdvanceHudLoanEarly(pendingDue, pendingDueLedger, true, 0),
        "60th restore early stage reserves pending commit");
    Check(bp::HudHasPendingCommit(pendingDue)
            && pendingDue.modules[0].maturityRemaining == 120,
        "restore pending and maturity 120 coexist after step 6");
    Check(bp::ShortenOpenHudMaturity(pendingDue, 120),
        "step 7 shortening reaches DUE on pending restore");
    Check(pendingDue.modules[0].state == bp::HudLoanState::Due
            && !bp::HudHasPendingCommit(pendingDue),
        "DUE transition atomically cancels stale restore pending commit");
    Check(bp::HudLoanSystemValid(pendingDue, pendingDueLedger),
        "pending-to-DUE shortening preserves HUD invariant");
    Check(bp::ResolvePendingHudCommit(pendingDue, pendingDueLedger, false),
        "tick 17 is a no-op after policy cancels restore pending");

    bp::PhysicalLedger ledger = MakeHudLedger();
    bp::HudLoanSystem hud{};
    bp::InitializeHudLoanSystem(hud);
    Check(bp::SelectHudModule(hud, bp::HudModuleId::Vector), "select VECTOR partial due");
    HoldHud(hud, ledger, 45);
    IdleHud(hud, ledger, 36);
    Check(hud.modules[1].maturityRemaining == 719,
        "maturity begins on completed to-shell arrival tick");
    IdleHud(hud, ledger, 599);
    Check(hud.modules[1].maturityRemaining == 120 && bp::HudLoanWarning(hud),
        "last 120 maturity ticks enable warning");
    IdleHud(hud, ledger, 120);
    Check(hud.modules[1].state == bp::HudLoanState::Due,
        "maturity 720 reaches DUE exactly");

    const std::uint16_t shellBefore = ledger.mass[AccountIndex(bp::PhysicalAccount::PlayerShell)];
    Check(TickHudForTest(hud, ledger, false, false, 32), "DUE partial repayment 32");
    Check(hud.modules[1].state == bp::HudLoanState::Due
            && hud.modules[1].transitMass == 32,
        "partial repayment remains DUE with exact Transit mass");
    Check(ledger.mass[AccountIndex(bp::PhysicalAccount::PlayerShell)] == shellBefore - 32,
        "partial repayment debits only reclaimed Shell amount");
    Check(TickHudForTest(hud, ledger, false, false, 47), "DUE partial repayment 47");
    Check(hud.modules[1].transitMass == 79, "DUE repayment accumulates exact partial mass");
    Check(TickHudForTest(hud, ledger, false, false, 1), "DUE final repayment tile");
    Check(hud.modules[1].state == bp::HudLoanState::ToHudTransit,
        "final repaid tile starts reverse transit");
    IdleHud(hud, ledger, 18);
    Check(hud.modules[1].arrivedMass == 40
            && bp::HudVisibleMaterialMass(hud, bp::HudModuleId::Vector) == 40,
        "reverse transit exposes exact arrived proportion");
    IdleHud(hud, ledger, 18);
    Check(hud.modules[1].state == bp::HudLoanState::Installed,
        "partial repayment completes restoration");
    Check(hud.reallocationLockTicks == bp::kHudReallocationLockTicks,
        "restoration applies 90-tick reallocation lock");

    Check(bp::SelectHudModule(hud, bp::HudModuleId::Vital), "select during reallocation lock");
    Check(TickHudForTest(hud, ledger, true, false, 0), "hold during reallocation lock");
    Check(hud.modules[0].state == bp::HudLoanState::Installed,
        "reallocation lock blocks new dismantle");
    IdleHud(hud, ledger, bp::kHudReallocationLockTicks - 1);
    Check(TickHudForTest(hud, ledger, true, false, 0), "hold after lock expiry");
    Check(hud.modules[0].state == bp::HudLoanState::Dismantling,
        "new dismantle starts when reallocation lock expires");
}

void TestHudSingleOpenAndRestoreRollback() noexcept {
    bp::PhysicalLedger ledger = MakeHudLedger();
    bp::HudLoanSystem hud{};
    bp::InitializeHudLoanSystem(hud);
    HoldHud(hud, ledger, 45);
    IdleHud(hud, ledger, 36);
    Check(bp::SelectHudModule(hud, bp::HudModuleId::Vector),
        "select installed module while another loan open");
    HoldHud(hud, ledger, 10);
    Check(hud.modules[1].state == bp::HudLoanState::Installed,
        "one-open-loan rule blocks second dismantle");

    Check(bp::SelectHudModule(hud, bp::HudModuleId::Vital), "reselect open VITAL loan");
    Check(TickHudForTest(hud, ledger, true, false, 0),
        "begin manual restoring channel");
    Check(hud.modules[0].state == bp::HudLoanState::Restoring,
        "open module enters restoring before switch-away check");
    Check(!bp::SelectHudModule(hud, bp::HudModuleId::Vector),
        "selection change is rejected while restore channel is active");
    Check(TickHudForTest(hud, ledger, false, false, 0),
        "release restoring channel after rejected switch");
    Check(hud.modules[0].state == bp::HudLoanState::Inactive,
        "restore channel cannot become stranded by selection change");
    const std::uint16_t shell = ledger.mass[AccountIndex(bp::PhysicalAccount::PlayerShell)];
    Check(bp::PhysicalTransfer(
        ledger,
        bp::PhysicalAccount::PlayerShell,
        bp::PhysicalAccount::LockedBands,
        static_cast<std::uint16_t>(shell - 47)), "leave insufficient Shell for VITAL restore");
    const bp::PhysicalLedger beforeRestore = ledger;
    HoldHud(hud, ledger, 60);
    Check(hud.modules[0].state == bp::HudLoanState::Inactive,
        "insufficient Shell cancels manual restore exact-or-zero");
    Check(std::memcmp(&ledger, &beforeRestore, sizeof(ledger)) == 0,
        "failed manual restore preserves physical ledger byte-identical");
}

void TestNeutralDynamicsAndMerge() noexcept {
    CombatWorld boundary = MakeCombatWorld(8);
    const std::uint16_t boundaryIndex = AddNeutral(
        boundary,
        bp::Vec2Q8{307 * bp::kQ8One, 80 * bp::kQ8One},
        8);
    boundary.combat.neutrals[boundaryIndex].velocity = bp::Vec2Q8{10 * bp::kQ8One, 3 * bp::kQ8One};
    boundary.combat.neutrals[boundaryIndex].inertiaTicks = 15;
    Check(bp::TickNeutralDynamics(boundary.combat, boundary.ledger),
        "neutral inertia boundary tick");
    Check(boundary.combat.neutrals[boundaryIndex].position.x == 308 * bp::kQ8One,
        "neutral remains inside play boundary");
    Check(boundary.combat.neutrals[boundaryIndex].velocity.x == -2 * bp::kQ8One
            && boundary.combat.neutrals[boundaryIndex].velocity.y == 3 * bp::kQ8One,
        "neutral boundary reflects normal at 20 percent and preserves tangent");
    Check(boundary.combat.neutrals[boundaryIndex].inertiaTicks == 15,
        "neutral dynamics does not reorder countdown stage");
    CheckCombatWorld(boundary, "neutral boundary invariant");

    CombatWorld damping = MakeCombatWorld(8);
    const std::uint16_t dampingIndex = AddNeutral(
        damping,
        bp::Vec2Q8{100 * bp::kQ8One, 80 * bp::kQ8One},
        8);
    damping.combat.neutrals[dampingIndex].velocity = bp::Vec2Q8{1000, -1000};
    damping.combat.neutrals[dampingIndex].inertiaTicks = 0;
    Check(bp::TickNeutralDynamics(damping.combat, damping.ledger), "neutral damping tick");
    Check(damping.combat.neutrals[dampingIndex].velocity.x == 1000 * 65067 / 65536
            && damping.combat.neutrals[dampingIndex].velocity.y == -1000 * 65067 / 65536,
        "neutral damping uses exact per-axis 65067/65536 truncation");

    CombatWorld merge = MakeCombatWorld(24);
    const std::uint16_t first = AddNeutral(
        merge, bp::Vec2Q8{100 * bp::kQ8One, 80 * bp::kQ8One}, 8, 3, 1);
    const std::uint16_t second = AddNeutral(
        merge, bp::Vec2Q8{106 * bp::kQ8One, 80 * bp::kQ8One}, 16, 2, 4);
    const std::uint32_t firstId = merge.combat.neutrals[first].stableId;
    Check(bp::MergeNeutralClusters(merge.combat, merge.ledger), "ordinary neutral merge");
    Check(ActiveNeutralCount(merge) == 1, "eligible pair merges within six pixels");
    Check(merge.combat.neutrals[first].active != 0
            && merge.combat.neutrals[first].stableId == firstId
            && merge.combat.neutrals[first].mass == 24,
        "smaller stable id absorbs merged mass");
    Check(merge.combat.neutrals[first].playerMissMass == 5
            && merge.combat.neutrals[first].opponentMissMass == 5,
        "neutral merge preserves provenance totals");
    Check(!bp::NeutralHandleValid(merge.combat, bp::SlotHandle{
            second, merge.combat.neutrals[second].generation}),
        "released merge slot is inactive");
    CheckCombatWorld(merge, "ordinary merge invariant");

    CombatWorld fast = MakeCombatWorld(16);
    AddNeutral(fast, bp::Vec2Q8{100 * bp::kQ8One, 80 * bp::kQ8One}, 8);
    const std::uint16_t fastSecond = AddNeutral(
        fast, bp::Vec2Q8{105 * bp::kQ8One, 80 * bp::kQ8One}, 8);
    fast.combat.neutrals[fastSecond].velocity.x =
        bp::PixelsPerSecondToQ8PerTick(13);
    Check(bp::MergeNeutralClusters(fast.combat, fast.ledger), "fast pair merge pass");
    Check(ActiveNeutralCount(fast) == 2, "relative speed above 12px/s blocks merge");
}

void TestReclaimSelectionForceAndProvenance() noexcept {
    CombatWorld selection = MakeCombatWorld(32);
    const std::uint16_t small = AddNeutral(
        selection, bp::Vec2Q8{60 * bp::kQ8One, 90 * bp::kQ8One}, 8);
    const std::uint16_t large = AddNeutral(
        selection, bp::Vec2Q8{80 * bp::kQ8One, 90 * bp::kQ8One}, 16);
    const std::uint16_t offCone = AddNeutral(
        selection, bp::Vec2Q8{60 * bp::kQ8One, 110 * bp::kQ8One}, 8);
    Check(bp::FindReclaimTarget(
            selection.combat,
            bp::CombatSide::Player,
            bp::Vec2Q8{bp::kInputUnit, 0}) == large,
        "reclaim chooses highest mass-distance score inside 18-degree cone");
    selection.combat.neutrals[large].reclaimLockTicks = 1;
    Check(bp::FindReclaimTarget(
            selection.combat,
            bp::CombatSide::Player,
            bp::Vec2Q8{bp::kInputUnit, 0}) == small,
        "reclaim lock blocks force and selection");
    Check(offCone != small, "off-cone test slot distinct");

    CombatWorld centered = MakeCombatWorld(1);
    const std::uint16_t centeredNeutral = AddNeutral(
        centered, centered.combat.actors[0].position, 1);
    Check(bp::FindReclaimTarget(
            centered.combat,
            bp::CombatSide::Player,
            bp::Vec2Q8{0, bp::kInputUnit}) == centeredNeutral,
        "neutral at actor center remains reclaimable independent of aim angle");

    CombatWorld blocked = MakeCombatWorld(8, 8);
    bp::SlotHandle cover{};
    Check(bp::AddCover(blocked.combat, bp::RectPixels{50, 86, 4, 8}, 8, cover),
        "add reclaim line-of-sight cover");
    AddNeutral(blocked, bp::Vec2Q8{70 * bp::kQ8One, 90 * bp::kQ8One}, 8);
    Check(bp::FindReclaimTarget(
            blocked.combat,
            bp::CombatSide::Player,
            bp::Vec2Q8{bp::kInputUnit, 0}) == bp::kInvalidSlotIndex,
        "Cover blocks reclaim Bresenham line of sight");

    CombatWorld absorption = MakeCombatWorld(3);
    const std::uint16_t target = AddNeutral(
        absorption,
        bp::Vec2Q8{41 * bp::kQ8One, 90 * bp::kQ8One},
        3,
        2,
        1);
    absorption.combat.neutrals[target].inertiaTicks = 0;
    bp::ReclaimTickResult result{};
    Check(bp::TickReclaim(
        absorption.combat, absorption.ledger, absorption.reclaim, true, false, result),
        "first player reclaim tick");
    Check(result.playerMass == 0 && absorption.reclaim.accumulator[0] == 32,
        "reclaim accumulator adds 32 per contact tick");
    Check(absorption.combat.neutrals[target].velocity.x < 0,
        "player reclaim applies 180px/s2 attraction toward actor");
    Check(bp::TickReclaim(
        absorption.combat, absorption.ledger, absorption.reclaim, true, false, result),
        "second player reclaim tick");
    Check(result.playerMass == 1 && absorption.reclaim.accumulator[0] == 4,
        "reclaim moves one TILE whenever accumulator reaches 60");
    Check(absorption.combat.neutrals[target].mass == 2
            && absorption.combat.neutrals[target].playerMissMass == 1
            && absorption.combat.neutrals[target].opponentMissMass == 1,
        "player reclaim consumes own-miss provenance first");
    Check(absorption.ledger.mass[AccountIndex(bp::PhysicalAccount::Neutral)] == 2,
        "reclaim debits Neutral ledger exactly");
    CheckCombatWorld(absorption, "reclaim absorption invariant");

    CombatWorld contested = MakeCombatWorld(
        1,
        0,
        bp::Vec2Q8{100 * bp::kQ8One, 90 * bp::kQ8One},
        bp::Vec2Q8{102 * bp::kQ8One, 90 * bp::kQ8One});
    AddNeutral(contested, bp::Vec2Q8{101 * bp::kQ8One, 90 * bp::kQ8One}, 1);
    contested.combat.actors[0].aimDirectionQ15 = bp::Vec2Q8{bp::kInputUnit, 0};
    contested.combat.actors[1].aimDirectionQ15 = bp::Vec2Q8{-bp::kInputUnit, 0};
    Check(bp::TickReclaim(
        contested.combat, contested.ledger, contested.reclaim, true, true, result),
        "contested reclaim first tick");
    Check(bp::TickReclaim(
        contested.combat, contested.ledger, contested.reclaim, true, true, result),
        "contested reclaim second tick");
    Check(result.playerMass == 1 && result.opponentMass == 0,
        "equidistant last TILE tie goes to PLAYER");
    CheckCombatWorld(contested, "contested reclaim invariant");
}

void TestNeutralSaturationAndDeterministicReplay() noexcept {
    CombatWorld saturated = MakeCombatWorld(64);
    for (std::uint16_t index = 0; index < bp::kNeutralCapacity; ++index) {
        const std::int32_t x = index < 2
            ? 100 + static_cast<std::int32_t>(index) * 5
            : 20 + static_cast<std::int32_t>(index % 16) * 16;
        const std::int32_t y = index < 2
            ? 80
            : 24 + static_cast<std::int32_t>(index / 16) * 32;
        AddNeutral(saturated, bp::Vec2Q8{x * bp::kQ8One, y * bp::kQ8One}, 1);
    }
    Check(ActiveNeutralCount(saturated) == bp::kNeutralCapacity,
        "Neutral saturation reaches exact capacity 64");
    Check(bp::MergeNeutralClusters(saturated.combat, saturated.ledger),
        "saturated ordinary merge deterministic pass");
    Check(ActiveNeutralCount(saturated) < bp::kNeutralCapacity,
        "eligible saturated pair releases at least one slot without mass loss");
    Check(saturated.ledger.mass[AccountIndex(bp::PhysicalAccount::Neutral)] == 64,
        "saturated merge preserves all Neutral mass");
    CheckCombatWorld(saturated, "saturated neutral invariant");

    CombatWorld first = MakeCombatWorld(24);
    CombatWorld second = MakeCombatWorld(24);
    for (std::uint16_t index = 0; index < 3; ++index) {
        const bp::Vec2Q8 position{
            static_cast<std::int32_t>(70 + index * 20) * bp::kQ8One,
            static_cast<std::int32_t>(70 + index * 4) * bp::kQ8One,
        };
        const std::uint16_t a = AddNeutral(first, position, 8, index, 0);
        const std::uint16_t b = AddNeutral(second, position, 8, index, 0);
        first.combat.neutrals[a].velocity = bp::Vec2Q8{30 - index * 4, 12 + index};
        second.combat.neutrals[b].velocity = first.combat.neutrals[a].velocity;
        first.combat.neutrals[a].inertiaTicks = 0;
        second.combat.neutrals[b].inertiaTicks = 0;
    }
    bp::ReclaimTickResult firstResult{};
    bp::ReclaimTickResult secondResult{};
    for (std::uint16_t tick = 0; tick < 128; ++tick) {
        bp::TickCombatCountdowns(first.combat);
        bp::TickCombatCountdowns(second.combat);
        Check(bp::TickNeutralDynamics(first.combat, first.ledger), "first replay neutral dynamics");
        Check(bp::TickNeutralDynamics(second.combat, second.ledger), "second replay neutral dynamics");
        const bool reclaimActive = (tick % 4) < 2;
        Check(bp::TickReclaim(
            first.combat, first.ledger, first.reclaim, reclaimActive, false, firstResult),
            "first replay reclaim");
        Check(bp::TickReclaim(
            second.combat, second.ledger, second.reclaim, reclaimActive, false, secondResult),
            "second replay reclaim");
        Check(bp::MergeNeutralClusters(first.combat, first.ledger), "first replay merge");
        Check(bp::MergeNeutralClusters(second.combat, second.ledger), "second replay merge");
        Check(std::memcmp(&first.combat, &second.combat, sizeof(first.combat)) == 0,
            "Neutral replay combat state byte-identical");
        Check(std::memcmp(&first.ledger, &second.ledger, sizeof(first.ledger)) == 0,
            "Neutral replay ledger byte-identical");
        Check(std::memcmp(&first.reclaim, &second.reclaim, sizeof(first.reclaim)) == 0,
            "Neutral replay reclaim accumulator byte-identical");
    }
}

struct T06TestRunner {
    T06TestRunner() noexcept {
        TestHudConstantsAndAllStates();
        TestHudTimingCancellationAndPartialRepayment();
        TestHudSingleOpenAndRestoreRollback();
        TestNeutralDynamicsAndMerge();
        TestReclaimSelectionForceAndProvenance();
        TestNeutralSaturationAndDeterministicReplay();
        if (gT06Failures != 0) {
            std::printf("T06_FAIL\tchecks=%u\tfailures=%u\n", gT06Checks, gT06Failures);
            std::exit(1);
        }
        std::printf("T06_PASS\tchecks=%u\thud_states=21\tneutral_capacity=64\n", gT06Checks);
    }
};

T06TestRunner gT06TestRunner;

}  // namespace
