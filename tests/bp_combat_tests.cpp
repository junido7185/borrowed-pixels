#include "bp_combat.h"
#include "bp_claim_ledger.h"
#include "bp_game.h"
#include "bp_physical_ledger.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

std::uint32_t gT05Checks;
std::uint32_t gT05Failures;

void Check(bool condition, const char* name) noexcept {
    ++gT05Checks;
    if (!condition) {
        ++gT05Failures;
        std::printf("T05_FAIL\t%s\n", name);
    }
}

constexpr std::uint8_t AccountIndex(bp::PhysicalAccount account) noexcept {
    return static_cast<std::uint8_t>(account);
}

constexpr std::uint8_t SideIndex(bp::CombatSide side) noexcept {
    return static_cast<std::uint8_t>(side);
}

constexpr bp::PhysicalAccount ShellAccount(bp::CombatSide side) noexcept {
    return side == bp::CombatSide::Player
        ? bp::PhysicalAccount::PlayerShell
        : bp::PhysicalAccount::OpponentShell;
}

struct World {
    bp::CombatState combat;
    bp::PhysicalLedger ledger;
    bp::ClaimLedger claims;
};

World MakeWorld(
    std::uint16_t playerActive = 240,
    std::uint16_t opponentActive = 64,
    std::uint16_t coverMass = 0,
    std::uint16_t neutralMass = 0,
    bp::Vec2Q8 playerPosition = bp::Vec2Q8{40 * bp::kQ8One, 90 * bp::kQ8One},
    bp::Vec2Q8 opponentPosition = bp::Vec2Q8{200 * bp::kQ8One, 90 * bp::kQ8One}) noexcept {
    World world{};
    Check(bp::InitializePhysicalLedger(
        world.ledger,
        playerActive,
        opponentActive,
        coverMass,
        neutralMass), "world physical ledger initialize");
    bp::InitializeCombatState(world.combat, playerPosition, opponentPosition);
    world.claims = bp::MakeInitialClaimLedger();
    return world;
}

std::uint16_t SeedProjectile(
    World& world,
    bp::CombatSide owner,
    std::uint16_t mass,
    bp::Vec2Q8 position,
    bp::Vec2Q8 velocity) noexcept {
    std::uint16_t index = bp::kInvalidSlotIndex;
    for (std::uint16_t candidate = 0; candidate < bp::kProjectileCapacity; ++candidate) {
        if (world.combat.projectiles[candidate].active == 0) {
            index = candidate;
            break;
        }
    }
    Check(index != bp::kInvalidSlotIndex, "seed projectile free slot");
    if (index == bp::kInvalidSlotIndex) return index;
    Check(bp::PhysicalTransfer(
        world.ledger,
        ShellAccount(owner),
        bp::PhysicalAccount::Projectile,
        mass), "seed projectile ledger transfer");
    bp::ProjectileSlot& slot = world.combat.projectiles[index];
    slot.position = position;
    slot.velocity = velocity;
    slot.stableId = world.combat.nextStableId++;
    slot.generation = static_cast<std::uint16_t>(slot.generation + 1u);
    if (slot.generation == 0) slot.generation = 1;
    slot.mass = mass;
    slot.owner = owner;
    slot.active = 1;
    return index;
}

std::uint16_t FindProjectile(const World& world, bp::CombatSide owner) noexcept {
    for (std::uint16_t index = 0; index < bp::kProjectileCapacity; ++index) {
        if (world.combat.projectiles[index].active != 0
            && world.combat.projectiles[index].owner == owner) return index;
    }
    return bp::kInvalidSlotIndex;
}

std::uint16_t FindNeutral(const World& world) noexcept {
    for (std::uint16_t index = 0; index < bp::kNeutralCapacity; ++index) {
        if (world.combat.neutrals[index].active != 0) return index;
    }
    return bp::kInvalidSlotIndex;
}

void CheckWorld(const World& world, const char* name) noexcept {
    Check(bp::PhysicalLedgerValid(world.ledger), name);
    Check(bp::ClaimLedgerValid(world.claims), "claim total remains 1440");
    Check(bp::PhysicalLedgerTotal(world.ledger) == bp::kLedgerTotal, "physical total remains 1440");
    Check(bp::CombatStateValid(world.combat, world.ledger), "combat slots match aggregate ledger");
}

void TestFormulaContract() noexcept {
    Check(bp::kProjectileCapacity == 32, "projectile capacity 32");
    Check(bp::kNeutralCapacity == 64, "neutral capacity 64");
    Check(bp::kCoverCapacity == 32, "cover capacity 32");
    for (std::uint16_t mass = 8; mass <= 48; ++mass) {
        const std::int32_t expectedSpeed = bp::PixelsPerSecondToQ8PerTick(
            170 - static_cast<std::int32_t>(mass - 8) * 5 / 4);
        Check(bp::ProjectileSpeedQ8(mass) == expectedSpeed, "partial projectile speed formula");
        Check(bp::ProjectileShellDamage(mass) == mass * 3 / 2, "partial shell damage formula");
        Check(bp::ProjectileCoverDamage(mass) == mass, "partial cover damage formula");
        Check(bp::ProjectileRadiusPixels(mass)
                == 1 + static_cast<std::int32_t>(bp::IntegerSqrt(static_cast<std::uint64_t>(mass) * 9u) / 8u),
            "partial projectile radius formula");
        Check(bp::RecoilQ8(mass)
                == static_cast<std::int32_t>((static_cast<std::uint32_t>(mass) * 8960u + 30000u) / 60000u),
            "recoil formula");
    }
    Check(bp::ProjectileSpeedQ8(8) == bp::PixelsPerSecondToQ8PerTick(170), "TAP speed");
    Check(bp::ProjectileSpeedQ8(16) == bp::PixelsPerSecondToQ8PerTick(160), "CUT speed");
    Check(bp::ProjectileSpeedQ8(32) == bp::PixelsPerSecondToQ8PerTick(140), "CLAIM speed");
    Check(bp::ProjectileSpeedQ8(48) == bp::PixelsPerSecondToQ8PerTick(120), "FORECLOSE speed");
}

void TestChargeLifecycle() noexcept {
    World world = MakeWorld();
    CheckWorld(world, "initial charge world valid");
    const std::uint16_t initialShell = world.ledger.mass[AccountIndex(bp::PhysicalAccount::PlayerShell)];
    Check(bp::BeginCharge(
        world.combat,
        world.ledger,
        bp::CombatSide::Player,
        bp::Vec2Q8{bp::kInputUnit, 0}), "begin player charge");
    Check(world.combat.charges[SideIndex(bp::CombatSide::Player)].mass == 8, "charge starts at mass eight");
    Check(world.ledger.mass[AccountIndex(bp::PhysicalAccount::PlayerShell)] == initialShell - 8,
        "charge exact-or-zero shell debit");
    for (std::uint16_t tick = 1; tick <= 75; ++tick) {
        Check(bp::AdvanceCharge(world.combat, world.ledger, bp::CombatSide::Player), "advance charge tick");
        const std::uint16_t expected = tick >= 75 ? 48 : (tick >= 48 ? 32 : (tick >= 21 ? 16 : 8));
        Check(world.combat.charges[SideIndex(bp::CombatSide::Player)].mass == expected,
            "charge mass changes only at exact stage");
        CheckWorld(world, "charge tick mass conservation");
    }
    const std::uint32_t chargeStableId =
        world.combat.charges[SideIndex(bp::CombatSide::Player)].stableId;
    Check(bp::ReleaseCharge(world.combat, world.ledger, bp::CombatSide::Player), "release charge");
    const std::uint16_t projectileIndex = FindProjectile(world, bp::CombatSide::Player);
    Check(projectileIndex != bp::kInvalidSlotIndex, "release creates projectile");
    if (projectileIndex != bp::kInvalidSlotIndex) {
        const bp::ProjectileSlot& projectile = world.combat.projectiles[projectileIndex];
        Check(projectile.mass == 48, "release preserves charge mass");
        Check(projectile.stableId == chargeStableId, "release preserves logical stable id");
        Check(projectile.velocity.x == bp::ProjectileSpeedQ8(48) && projectile.velocity.y == 0,
            "release installs formula velocity");
    }
    Check(world.combat.actors[SideIndex(bp::CombatSide::Player)].velocity.x == -bp::RecoilQ8(48),
        "release applies signed-nearest opposite recoil");
    Check(world.combat.actors[SideIndex(bp::CombatSide::Player)].shotCooldownTicks == 34,
        "FORECLOSE cooldown 34");
    CheckWorld(world, "released charge world valid");
    bp::TickCombatCountdowns(world.combat);
    Check(world.combat.actors[SideIndex(bp::CombatSide::Player)].shotCooldownTicks == 33,
        "existing cooldown decrements at tick start");

    World rollback = MakeWorld();
    const bp::PhysicalLedger rollbackInitial = rollback.ledger;
    Check(bp::BeginCharge(
        rollback.combat,
        rollback.ledger,
        bp::CombatSide::Player,
        bp::Vec2Q8{0, bp::kInputUnit}), "begin rollback charge");
    Check(bp::RollbackCharge(rollback.combat, rollback.ledger, bp::CombatSide::Player),
        "explicit charge rollback");
    Check(std::memcmp(&rollback.ledger, &rollbackInitial, sizeof(rollbackInitial)) == 0,
        "charge rollback restores ledger byte-identical");
    CheckWorld(rollback, "rollback world valid");

    World insufficient = MakeWorld(64, 64);
    Check(bp::BeginCharge(
        insufficient.combat,
        insufficient.ledger,
        bp::CombatSide::Player,
        bp::Vec2Q8{bp::kInputUnit, 0}), "begin low-shell charge");
    Check(bp::PhysicalTransfer(
        insufficient.ledger,
        bp::PhysicalAccount::PlayerShell,
        bp::PhysicalAccount::LockedBands,
        40), "remove low-shell reserve for stage test");
    for (std::uint16_t tick = 0; tick < 21; ++tick) {
        Check(bp::AdvanceCharge(insufficient.combat, insufficient.ledger, bp::CombatSide::Player),
            "insufficient charge advance");
    }
    Check(insufficient.combat.charges[0].mass == 8, "insufficient shell retains complete stage");

    const std::uint16_t stageTicks[4] = {0, 21, 48, 75};
    const std::uint16_t stageMass[4] = {8, 16, 32, 48};
    const std::uint16_t stageCooldown[4] = {15, 19, 27, 34};
    for (std::uint16_t stage = 0; stage < 4; ++stage) {
        World standard = MakeWorld();
        Check(bp::BeginCharge(
            standard.combat,
            standard.ledger,
            bp::CombatSide::Player,
            bp::Vec2Q8{bp::kInputUnit, 0}), "begin standard charge stage");
        for (std::uint16_t tick = 0; tick < stageTicks[stage]; ++tick) {
            Check(bp::AdvanceCharge(standard.combat, standard.ledger, bp::CombatSide::Player),
                "advance standard charge stage");
        }
        Check(bp::ReleaseCharge(standard.combat, standard.ledger, bp::CombatSide::Player),
            "release standard charge stage");
        const std::uint16_t standardShot = FindProjectile(standard, bp::CombatSide::Player);
        Check(standardShot != bp::kInvalidSlotIndex
                && standard.combat.projectiles[standardShot].mass == stageMass[stage],
            "standard stage emits exact mass");
        Check(standard.combat.actors[0].shotCooldownTicks == stageCooldown[stage],
            "standard stage installs exact cooldown");
        CheckWorld(standard, "standard stage world valid");
    }
}

void TestSlotCapacityAndHandles() noexcept {
    World world = MakeWorld();
    for (std::uint16_t index = 0; index < 31; ++index) {
        const bp::CombatSide side = index < 16 ? bp::CombatSide::Player : bp::CombatSide::Opponent;
        SeedProjectile(
            world,
            side,
            1,
            bp::Vec2Q8{static_cast<std::int32_t>(20 + index) * bp::kQ8One, 40 * bp::kQ8One},
            bp::Vec2Q8{0, 0});
    }
    CheckWorld(world, "31 projectile pool valid");
    Check(bp::BeginCharge(
        world.combat,
        world.ledger,
        bp::CombatSide::Player,
        bp::Vec2Q8{bp::kInputUnit, 0}), "charge can begin with one projectile slot");
    SeedProjectile(
        world,
        bp::CombatSide::Opponent,
        1,
        bp::Vec2Q8{100 * bp::kQ8One, 40 * bp::kQ8One},
        bp::Vec2Q8{0, 0});
    const std::uint16_t shellBeforeRelease =
        world.ledger.mass[AccountIndex(bp::PhysicalAccount::PlayerShell)];
    Check(!bp::ReleaseCharge(world.combat, world.ledger, bp::CombatSide::Player),
        "projectile cap blocks release");
    Check(world.combat.charges[0].active == 0, "cap failure rolls charge back");
    Check(world.ledger.mass[AccountIndex(bp::PhysicalAccount::PlayerShell)] == shellBeforeRelease + 8,
        "cap rollback returns all charge mass");
    CheckWorld(world, "projectile cap rollback valid");

    World handles = MakeWorld(240, 64, 8, 8);
    bp::SlotHandle cover{};
    bp::SlotHandle neutral{};
    Check(bp::AddCover(handles.combat, bp::RectPixels{100, 80, 8, 8}, 8, cover),
        "add cover handle");
    Check(bp::AddNeutral(
        handles.combat,
        bp::Vec2Q8{80 * bp::kQ8One, 90 * bp::kQ8One},
        8,
        3,
        2,
        neutral), "add neutral handle");
    Check(bp::CoverHandleValid(handles.combat, cover), "cover handle valid");
    Check(bp::NeutralHandleValid(handles.combat, neutral), "neutral handle valid");
    bp::SlotHandle staleNeutral = neutral;
    ++staleNeutral.generation;
    Check(!bp::NeutralHandleValid(handles.combat, staleNeutral), "generation mismatch rejects neutral handle");
    Check(!bp::ProjectileHandleValid(
        handles.combat,
        bp::SlotHandle{bp::kProjectileCapacity, 1}), "out-of-range projectile handle rejected");
    std::uint16_t resolvedIndex = bp::kInvalidSlotIndex;
    Check(bp::ResolveNeutralHandle(handles.combat, neutral, resolvedIndex)
        && resolvedIndex == neutral.index, "valid handle resolves exact index");
    World invalidHandle = handles;
    resolvedIndex = 0;
    Check(!bp::ResolveNeutralHandle(invalidHandle.combat, staleNeutral, resolvedIndex),
        "stale handle access returns failure");
    Check(invalidHandle.combat.phase == bp::CombatPhase::Fatal
        && invalidHandle.combat.error == bp::CombatError::InvalidHandle
        && resolvedIndex == bp::kInvalidSlotIndex,
        "stale handle access sets explicit fatal code");
    CheckWorld(handles, "materialized handles world valid");

    World exhaustedId = MakeWorld();
    exhaustedId.combat.nextStableId = 0xFFFFFFFFu;
    Check(!bp::BeginCharge(
        exhaustedId.combat,
        exhaustedId.ledger,
        bp::CombatSide::Player,
        bp::Vec2Q8{bp::kInputUnit, 0}), "stable id exhaustion rejects charge");
    Check(exhaustedId.combat.phase == bp::CombatPhase::Fatal
        && exhaustedId.combat.error == bp::CombatError::StableIdExhausted,
        "stable id exhaustion sets fatal before wrap");
}

void TestSweptShellAndCore() noexcept {
    World shell = MakeWorld();
    const std::uint16_t projectile = SeedProjectile(
        shell,
        bp::CombatSide::Player,
        8,
        bp::Vec2Q8{40 * bp::kQ8One, 90 * bp::kQ8One},
        bp::Vec2Q8{200 * bp::kQ8One, 0});
    CheckWorld(shell, "pre-shell-hit world valid");
    const bp::ImpactCandidate shellImpact = bp::FindEarliestImpact(shell.combat, shell.ledger, projectile);
    Check(shellImpact.kind == bp::ImpactKind::Shell, "swept segment finds shell without tunneling");
    Check(shellImpact.timeQ16 > 0 && shellImpact.timeQ16 < 65535, "shell TOI lies inside segment");
    Check(bp::StepCombatProjectiles(shell.combat, shell.ledger, shell.claims), "process shell impact");
    Check(shell.ledger.mass[AccountIndex(bp::PhysicalAccount::OpponentShell)] == 36,
        "TAP strips exact 12 shell");
    Check(shell.ledger.mass[AccountIndex(bp::PhysicalAccount::Neutral)] == 20,
        "shell impact neutral contains projectile plus stripped shell");
    Check(shell.combat.actors[SideIndex(bp::CombatSide::Opponent)].exposureGraceTicks == 15,
        "41-plus to 40-or-less transition starts grace 15");
    const std::uint16_t neutral = FindNeutral(shell);
    Check(neutral != bp::kInvalidSlotIndex, "shell impact creates neutral slot");
    if (neutral != bp::kInvalidSlotIndex) {
        Check(shell.combat.neutrals[neutral].reclaimLockTicks == 12,
            "shell neutral reclaim lock formula");
        Check(shell.combat.neutrals[neutral].immunityTicks == 45,
            "shell neutral immunity 45");
    }
    CheckWorld(shell, "post-shell-hit world valid");

    const std::uint16_t graceShot = SeedProjectile(
        shell,
        bp::CombatSide::Player,
        8,
        bp::Vec2Q8{40 * bp::kQ8One, 90 * bp::kQ8One},
        bp::Vec2Q8{200 * bp::kQ8One, 0});
    Check(bp::FindEarliestImpact(shell.combat, shell.ledger, graceShot).kind == bp::ImpactKind::Shell,
        "grace suppresses exposed core candidate");
    for (std::uint16_t tick = 0; tick < 15; ++tick) bp::TickCombatCountdowns(shell.combat);
    Check(shell.combat.actors[1].exposureGraceTicks == 0, "grace expires after 15 starts");
    Check(bp::FindEarliestImpact(shell.combat, shell.ledger, graceShot).kind == bp::ImpactKind::Core,
        "core candidate wins same path after grace");
    const bp::PhysicalLedger beforeCore = shell.ledger;
    Check(bp::StepCombatProjectiles(shell.combat, shell.ledger, shell.claims), "process core hit");
    Check(shell.combat.phase == bp::CombatPhase::HitFreeze, "core hit enters HIT_FREEZE");
    Check(shell.combat.actors[1].coreHit != 0, "opponent core hit flag");
    Check(shell.combat.settlement.valid != 0
        && shell.combat.settlement.hitSide == bp::CombatSide::Opponent,
        "core hit publishes settlement snapshot");
    Check(std::memcmp(&shell.combat.settlement.physicalLedger, &beforeCore, sizeof(beforeCore)) == 0,
        "core snapshot captures pre-settlement ledger");
    Check(std::memcmp(
            &shell.combat.settlement.claimLedger,
            &shell.claims,
            sizeof(shell.claims)) == 0,
        "core snapshot freezes current claim ledger");
    Check(std::memcmp(&shell.ledger, &beforeCore, sizeof(beforeCore)) == 0,
        "core hit itself creates or destroys no mass");
}

void TestCoverAndBoundaryTransactions() noexcept {
    World cover = MakeWorld(240, 64, 8, 0);
    bp::SlotHandle coverHandle{};
    Check(bp::AddCover(cover.combat, bp::RectPixels{100, 80, 10, 20}, 8, coverHandle),
        "materialize cover");
    const std::uint16_t shot = SeedProjectile(
        cover,
        bp::CombatSide::Player,
        8,
        bp::Vec2Q8{40 * bp::kQ8One, 90 * bp::kQ8One},
        bp::Vec2Q8{100 * bp::kQ8One, 0});
    Check(bp::FindEarliestImpact(cover.combat, cover.ledger, shot).kind == bp::ImpactKind::Cover,
        "swept segment finds thin cover");
    Check(bp::StepCombatProjectiles(cover.combat, cover.ledger, cover.claims), "process cover hit");
    Check(cover.ledger.mass[AccountIndex(bp::PhysicalAccount::Cover)] == 0,
        "cover hit strips exact projectile mass");
    Check(cover.ledger.mass[AccountIndex(bp::PhysicalAccount::Neutral)] == 16,
        "cover hit neutral combines cover and projectile");
    Check(!bp::CoverHandleValid(cover.combat, coverHandle), "destroyed cover handle invalid");
    CheckWorld(cover, "cover transaction conserves mass");
    const std::uint16_t rebuildSource = FindNeutral(cover);
    Check(rebuildSource != bp::kInvalidSlotIndex, "cover rebuild neutral source exists");
    if (rebuildSource != bp::kInvalidSlotIndex) {
        cover.combat.neutrals[rebuildSource].mass = static_cast<std::uint16_t>(
            cover.combat.neutrals[rebuildSource].mass - 8);
    }
    Check(bp::PhysicalTransfer(
        cover.ledger,
        bp::PhysicalAccount::Neutral,
        bp::PhysicalAccount::Cover,
        8), "rematerialize cover ledger mass");
    bp::SlotHandle replacementCover{};
    Check(bp::AddCover(cover.combat, bp::RectPixels{110, 80, 10, 20}, 8, replacementCover),
        "reuse destroyed cover slot");
    Check(replacementCover.index == coverHandle.index
        && replacementCover.generation != coverHandle.generation,
        "cover slot reuse increments generation");
    Check(!bp::CoverHandleValid(cover.combat, coverHandle)
        && bp::CoverHandleValid(cover.combat, replacementCover),
        "stale cover handle rejected after reuse");
    CheckWorld(cover, "reused cover slot valid");

    World boundary = MakeWorld();
    const std::uint16_t boundaryShot = SeedProjectile(
        boundary,
        bp::CombatSide::Player,
        8,
        bp::Vec2Q8{300 * bp::kQ8One, 90 * bp::kQ8One},
        bp::Vec2Q8{20 * bp::kQ8One, 0});
    Check(bp::FindEarliestImpact(boundary.combat, boundary.ledger, boundaryShot).kind
            == bp::ImpactKind::Boundary,
        "swept segment finds boundary");
    Check(bp::StepCombatProjectiles(boundary.combat, boundary.ledger, boundary.claims),
        "process projectile boundary");
    const std::uint16_t boundaryNeutral = FindNeutral(boundary);
    Check(boundaryNeutral != bp::kInvalidSlotIndex, "boundary creates neutral");
    if (boundaryNeutral != bp::kInvalidSlotIndex) {
        const bp::NeutralSlot& neutral = boundary.combat.neutrals[boundaryNeutral];
        Check(neutral.position.x <= (bp::kPlayInterior.x + bp::kPlayInterior.width - 2) * bp::kQ8One,
            "boundary neutral is at least two pixels inside");
        Check(neutral.playerMissMass == 8 && neutral.opponentMissMass == 0,
            "boundary preserves player miss provenance");
        Check(neutral.velocity.x == -4 * bp::kQ8One && neutral.velocity.y == 0,
            "boundary reflects normal velocity at twenty percent");
    }
    CheckWorld(boundary, "boundary transaction conserves mass");
}

void TestProjectileAndChargeInterception() noexcept {
    World projectiles = MakeWorld(240, 240);
    const std::uint16_t playerShot = SeedProjectile(
        projectiles,
        bp::CombatSide::Player,
        16,
        bp::Vec2Q8{100 * bp::kQ8One, 70 * bp::kQ8One},
        bp::Vec2Q8{30 * bp::kQ8One, 0});
    const std::uint16_t opponentShot = SeedProjectile(
        projectiles,
        bp::CombatSide::Opponent,
        8,
        bp::Vec2Q8{140 * bp::kQ8One, 70 * bp::kQ8One},
        bp::Vec2Q8{-30 * bp::kQ8One, 0});
    Check(bp::FindEarliestImpact(projectiles.combat, projectiles.ledger, playerShot).kind
            == bp::ImpactKind::Projectile,
        "relative swept segment finds opposing projectile");
    Check(bp::StepCombatProjectiles(projectiles.combat, projectiles.ledger, projectiles.claims),
        "process projectile cancellation");
    Check(projectiles.combat.projectiles[playerShot].active != 0
        && projectiles.combat.projectiles[playerShot].mass == 8,
        "larger projectile retains unmatched mass");
    Check(projectiles.combat.projectiles[opponentShot].active == 0,
        "smaller projectile becomes neutral");
    Check(projectiles.ledger.mass[AccountIndex(bp::PhysicalAccount::Projectile)] == 8,
        "projectile aggregate retains unmatched mass");
    Check(projectiles.ledger.mass[AccountIndex(bp::PhysicalAccount::Neutral)] == 16,
        "projectile common mass doubles into neutral");
    CheckWorld(projectiles, "projectile cancellation conserves mass");

    World charge = MakeWorld(240, 80);
    Check(bp::BeginCharge(
        charge.combat,
        charge.ledger,
        bp::CombatSide::Opponent,
        bp::Vec2Q8{-bp::kInputUnit, 0}), "begin opponent charge for intercept");
    for (std::uint16_t tick = 0; tick < 21; ++tick) {
        Check(bp::AdvanceCharge(charge.combat, charge.ledger, bp::CombatSide::Opponent),
            "advance opponent charge to CUT");
    }
    const std::uint16_t interceptShot = SeedProjectile(
        charge,
        bp::CombatSide::Player,
        8,
        bp::Vec2Q8{40 * bp::kQ8One, 90 * bp::kQ8One},
        bp::Vec2Q8{200 * bp::kQ8One, 0});
    Check(bp::FindEarliestImpact(charge.combat, charge.ledger, interceptShot).kind
            == bp::ImpactKind::Charge,
        "same-time charge priority precedes shell");
    Check(bp::StepCombatProjectiles(charge.combat, charge.ledger, charge.claims),
        "process charge intercept");
    Check(charge.combat.charges[SideIndex(bp::CombatSide::Opponent)].mass == 8,
        "charge retains mass at or above eight");
    Check(charge.ledger.mass[AccountIndex(bp::PhysicalAccount::Neutral)] == 16,
        "charge intercept common mass becomes neutral");
    CheckWorld(charge, "charge interception conserves mass");

    World partial = MakeWorld(240, 80);
    Check(bp::BeginCharge(
        partial.combat,
        partial.ledger,
        bp::CombatSide::Opponent,
        bp::Vec2Q8{-bp::kInputUnit, 0}), "begin partial intercept charge");
    for (std::uint16_t tick = 0; tick < 21; ++tick) {
        Check(bp::AdvanceCharge(partial.combat, partial.ledger, bp::CombatSide::Opponent),
            "advance partial intercept charge");
    }
    SeedProjectile(
        partial,
        bp::CombatSide::Player,
        4,
        bp::Vec2Q8{40 * bp::kQ8One, 90 * bp::kQ8One},
        bp::Vec2Q8{200 * bp::kQ8One, 0});
    Check(bp::StepCombatProjectiles(partial.combat, partial.ledger, partial.claims),
        "first partial charge intercept");
    Check(partial.combat.charges[1].mass == 12, "charge may retain partial mass at or above eight");
    World partialRelease = partial;
    Check(bp::ReleaseCharge(
        partialRelease.combat,
        partialRelease.ledger,
        bp::CombatSide::Opponent), "release intercepted partial charge");
    const std::uint16_t partialProjectile = FindProjectile(partialRelease, bp::CombatSide::Opponent);
    Check(partialProjectile != bp::kInvalidSlotIndex
        && partialRelease.combat.projectiles[partialProjectile].mass == 12,
        "partial charge releases current intercepted mass");
    Check(partialRelease.combat.actors[1].shotCooldownTicks == 19,
        "partial charge keeps cooldown of last complete CUT stage");
    CheckWorld(partialRelease, "partial charge release conserves mass");
    SeedProjectile(
        partial,
        bp::CombatSide::Player,
        8,
        bp::Vec2Q8{40 * bp::kQ8One, 90 * bp::kQ8One},
        bp::Vec2Q8{200 * bp::kQ8One, 0});
    const std::uint16_t shellBeforeRollback =
        partial.ledger.mass[AccountIndex(bp::PhysicalAccount::OpponentShell)];
    Check(bp::StepCombatProjectiles(partial.combat, partial.ledger, partial.claims),
        "second partial charge intercept");
    Check(partial.combat.charges[1].active == 0
        && partial.ledger.mass[AccountIndex(bp::PhysicalAccount::Charge)] == 0,
        "intercepted charge below eight cancels");
    Check(partial.ledger.mass[AccountIndex(bp::PhysicalAccount::OpponentShell)]
            == shellBeforeRollback + 4,
        "sub-eight charge remainder rolls back to owner shell");
    Check(partial.ledger.mass[AccountIndex(bp::PhysicalAccount::Neutral)] == 24,
        "two partial intercepts conserve doubled common mass");
    CheckWorld(partial, "partial charge cancellation conserves mass");

    World corePriority = MakeWorld(240, 64);
    Check(bp::BeginCharge(
        corePriority.combat,
        corePriority.ledger,
        bp::CombatSide::Opponent,
        bp::Vec2Q8{-bp::kInputUnit, 0}), "begin exposed charge priority setup");
    const std::uint16_t corePriorityShot = SeedProjectile(
        corePriority,
        bp::CombatSide::Player,
        8,
        bp::Vec2Q8{40 * bp::kQ8One, 90 * bp::kQ8One},
        bp::Vec2Q8{200 * bp::kQ8One, 0});
    Check(bp::FindEarliestImpact(
            corePriority.combat,
            corePriority.ledger,
            corePriorityShot).kind == bp::ImpactKind::Charge,
        "earlier Charge contact precedes later exposed Core contact");
}

bool RunDeterministicBoundaryTap(World& world, bp::CombatSide side, std::int32_t y) noexcept {
    const std::uint8_t sideIndex = SideIndex(side);
    while (world.combat.actors[sideIndex].shotCooldownTicks > 0) {
        bp::TickCombatCountdowns(world.combat);
    }
    const bp::Vec2Q8 direction = side == bp::CombatSide::Player
        ? bp::Vec2Q8{bp::kInputUnit, 0}
        : bp::Vec2Q8{-bp::kInputUnit, 0};
    if (!bp::BeginCharge(world.combat, world.ledger, side, direction)
        || !bp::ReleaseCharge(world.combat, world.ledger, side)) return false;
    const std::uint16_t shot = FindProjectile(world, side);
    if (shot == bp::kInvalidSlotIndex) return false;
    world.combat.projectiles[shot].position = side == bp::CombatSide::Player
        ? bp::Vec2Q8{300 * bp::kQ8One, y * bp::kQ8One}
        : bp::Vec2Q8{20 * bp::kQ8One, y * bp::kQ8One};
    world.combat.projectiles[shot].velocity = side == bp::CombatSide::Player
        ? bp::Vec2Q8{20 * bp::kQ8One, 0}
        : bp::Vec2Q8{-20 * bp::kQ8One, 0};
    return bp::StepCombatProjectiles(world.combat, world.ledger, world.claims);
}

void TestDeterministicCombatReplay() noexcept {
    World first = MakeWorld(240, 240);
    World second = MakeWorld(240, 240);
    for (std::uint16_t shot = 0; shot < 32; ++shot) {
        const bp::CombatSide side = (shot & 1u) == 0
            ? bp::CombatSide::Player
            : bp::CombatSide::Opponent;
        const std::int32_t y = 30 + shot % 12;
        Check(RunDeterministicBoundaryTap(first, side, y), "first deterministic combat step");
        Check(RunDeterministicBoundaryTap(second, side, y), "second deterministic combat step");
        Check(std::memcmp(&first.combat, &second.combat, sizeof(first.combat)) == 0,
            "identical combat inputs yield byte-identical state");
        Check(std::memcmp(&first.ledger, &second.ledger, sizeof(first.ledger)) == 0,
            "identical combat inputs yield byte-identical physical ledger");
        CheckWorld(first, "deterministic combat replay invariant");
    }
}

void TestSweptCoverGridAndStress() noexcept {
    for (std::int32_t coverX = 60; coverX <= 270; ++coverX) {
        World world = MakeWorld(
            240,
            64,
            8,
            0,
            bp::Vec2Q8{20 * bp::kQ8One, 60 * bp::kQ8One},
            bp::Vec2Q8{290 * bp::kQ8One, 120 * bp::kQ8One});
        bp::SlotHandle handle{};
        Check(bp::AddCover(world.combat, bp::RectPixels{coverX, 56, 1, 8}, 8, handle),
            "swept grid add one-pixel cover");
        const std::uint16_t shot = SeedProjectile(
            world,
            bp::CombatSide::Player,
            8,
            bp::Vec2Q8{20 * bp::kQ8One, 60 * bp::kQ8One},
            bp::Vec2Q8{260 * bp::kQ8One, 0});
        Check(bp::FindEarliestImpact(world.combat, world.ledger, shot).kind == bp::ImpactKind::Cover,
            "swept grid cannot tunnel through one-pixel cover");
        Check(bp::StepCombatProjectiles(world.combat, world.ledger, world.claims),
            "swept grid cover transaction");
        CheckWorld(world, "swept grid mass conservation");
    }

    World stress = MakeWorld(240, 240);
    for (std::uint16_t shotNumber = 0; shotNumber < 56; ++shotNumber) {
        const bp::CombatSide side = (shotNumber & 1u) == 0
            ? bp::CombatSide::Player
            : bp::CombatSide::Opponent;
        const std::uint8_t sideIndex = SideIndex(side);
        while (stress.combat.actors[sideIndex].shotCooldownTicks > 0) {
            bp::TickCombatCountdowns(stress.combat);
        }
        const bp::Vec2Q8 direction = side == bp::CombatSide::Player
            ? bp::Vec2Q8{bp::kInputUnit, 0}
            : bp::Vec2Q8{-bp::kInputUnit, 0};
        Check(bp::BeginCharge(stress.combat, stress.ledger, side, direction),
            "stress begin TAP charge");
        Check(bp::ReleaseCharge(stress.combat, stress.ledger, side),
            "stress release TAP charge");
        const std::uint16_t shot = FindProjectile(stress, side);
        Check(shot != bp::kInvalidSlotIndex, "stress projectile exists");
        if (shot != bp::kInvalidSlotIndex) {
            stress.combat.projectiles[shot].position = side == bp::CombatSide::Player
                ? bp::Vec2Q8{300 * bp::kQ8One, 40 * bp::kQ8One}
                : bp::Vec2Q8{20 * bp::kQ8One, 140 * bp::kQ8One};
            stress.combat.projectiles[shot].velocity = side == bp::CombatSide::Player
                ? bp::Vec2Q8{20 * bp::kQ8One, 0}
                : bp::Vec2Q8{-20 * bp::kQ8One, 0};
        }
        Check(bp::StepCombatProjectiles(stress.combat, stress.ledger, stress.claims),
            "stress swept boundary transaction");
        CheckWorld(stress, "stress every transaction keeps both aggregates valid");
    }
    Check(stress.ledger.mass[AccountIndex(bp::PhysicalAccount::Neutral)] == 56 * 8,
        "stress preserves all fired mass as neutral");
}

void TestNeutralSaturationRecoveryAndInvariantFailure() noexcept {
    World full = MakeWorld(240, 64, 0, 64);
    for (std::uint16_t index = 0; index < bp::kNeutralCapacity; ++index) {
        bp::SlotHandle handle{};
        Check(bp::AddNeutral(
            full.combat,
            bp::Vec2Q8{
                static_cast<std::int32_t>(20 + index % 16) * bp::kQ8One,
                static_cast<std::int32_t>(30 + index / 16) * bp::kQ8One},
            1,
            0,
            0,
            handle), "fill neutral capacity");
    }
    const std::uint16_t shot = SeedProjectile(
        full,
        bp::CombatSide::Player,
        8,
        bp::Vec2Q8{40 * bp::kQ8One, 90 * bp::kQ8One},
        bp::Vec2Q8{200 * bp::kQ8One, 0});
    Check(bp::FindEarliestImpact(full.combat, full.ledger, shot).kind == bp::ImpactKind::Shell,
        "saturation setup reaches shell");
    Check(bp::StepCombatProjectiles(full.combat, full.ledger, full.claims),
        "neutral saturation deterministically recovers a slot before collision");
    std::uint16_t activeNeutrals = 0;
    std::uint16_t stableThreeMass = 0;
    bool stableFourRemains = false;
    for (const bp::NeutralSlot& neutral : full.combat.neutrals) {
        if (neutral.active == 0) continue;
        ++activeNeutrals;
        if (neutral.stableId == 3) stableThreeMass = neutral.mass;
        if (neutral.stableId == 4) stableFourRemains = true;
    }
    Check(activeNeutrals == bp::kNeutralCapacity,
        "recovery merge plus collision refills exactly 64 slots");
    Check(stableThreeMass == 2 && !stableFourRemains,
        "nearest-pair tie uses lexicographically smallest stable ids");
    Check(full.ledger.mass[AccountIndex(bp::PhysicalAccount::Neutral)] == 84,
        "saturated collision preserves merged and newly neutralized mass");
    CheckWorld(full, "saturated recovery keeps both ledgers valid");

    World invalidClaim = MakeWorld();
    ++invalidClaim.claims.mass[0];
    Check(!bp::StepCombatProjectiles(
        invalidClaim.combat,
        invalidClaim.ledger,
        invalidClaim.claims), "invalid claim ledger rejects combat step");
    Check(invalidClaim.combat.phase == bp::CombatPhase::Fatal
        && invalidClaim.combat.error == bp::CombatError::InvariantFailure,
        "invalid claim ledger publishes invariant fatal code");
}

struct T05TestRunner {
    T05TestRunner() noexcept {
        TestFormulaContract();
        TestChargeLifecycle();
        TestSlotCapacityAndHandles();
        TestSweptShellAndCore();
        TestCoverAndBoundaryTransactions();
        TestProjectileAndChargeInterception();
        TestDeterministicCombatReplay();
        TestSweptCoverGridAndStress();
        TestNeutralSaturationRecoveryAndInvariantFailure();
        if (gT05Failures != 0) {
            std::printf("T05_FAIL\tchecks=%u\tfailures=%u\n", gT05Checks, gT05Failures);
            std::exit(1);
        }
        std::printf("T05_PASS\tchecks=%u\tprojectiles=32\tneutrals=64\n", gT05Checks);
    }
};

T05TestRunner gT05TestRunner;

}  // namespace
