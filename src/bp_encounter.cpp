#include "bp_encounter.h"

#include "bp_font.h"

#include <cstdint>

namespace bp {
namespace {

constexpr std::uint8_t AccountIndex(PhysicalAccount account) noexcept {
    return static_cast<std::uint8_t>(account);
}

constexpr std::uint8_t SideIndex(CombatSide side) noexcept {
    return static_cast<std::uint8_t>(side);
}

constexpr std::uint8_t ChoiceIndex(ChoiceId choice) noexcept {
    return static_cast<std::uint8_t>(choice);
}

constexpr std::uint32_t ActionIndex(PlatformAction action) noexcept {
    return static_cast<std::uint32_t>(action);
}

bool Pressed(const InputFrame& input, PlatformAction action) noexcept {
    return input.actions[ActionIndex(action)].pressed != 0;
}

bool Held(const InputFrame& input, PlatformAction action) noexcept {
    return input.actions[ActionIndex(action)].held != 0;
}

bool AnyInputPressed(const InputFrame& input) noexcept {
    if (input.moveX != 0 || input.moveY != 0 || input.aimX != 0 || input.aimY != 0) {
        return true;
    }
    for (std::uint32_t index = 0; index < kPlatformActionCount; ++index) {
        if (input.actions[index].pressed != 0) {
            return true;
        }
    }
    return false;
}

std::uint64_t DistanceSquared(Vec2Q8 left, Vec2Q8 right) noexcept {
    const std::int64_t dx = static_cast<std::int64_t>(left.x) - right.x;
    const std::int64_t dy = static_cast<std::int64_t>(left.y) - right.y;
    return static_cast<std::uint64_t>(dx * dx + dy * dy);
}

bool SweptWithinRadius(
    Vec2Q8 start,
    Vec2Q8 delta,
    Vec2Q8 target,
    std::int32_t radiusQ8) noexcept {
    const std::int64_t rx = static_cast<std::int64_t>(target.x) - start.x;
    const std::int64_t ry = static_cast<std::int64_t>(target.y) - start.y;
    const std::int64_t vx = delta.x;
    const std::int64_t vy = delta.y;
    const std::int64_t lengthSquared = vx * vx + vy * vy;
    const std::int64_t projection = rx * vx + ry * vy;
    Vec2Q8 closest = start;
    if (lengthSquared != 0 && projection > 0) {
        if (projection >= lengthSquared) {
            closest.x += delta.x;
            closest.y += delta.y;
        } else {
            closest.x += static_cast<std::int32_t>(vx * projection / lengthSquared);
            closest.y += static_cast<std::int32_t>(vy * projection / lengthSquared);
        }
    }
    return DistanceSquared(closest, target)
        <= static_cast<std::uint64_t>(radiusQ8) * radiusQ8;
}

Vec2Q8 NormalizeDirection(Vec2Q8 value) noexcept {
    const std::uint64_t squared =
        static_cast<std::uint64_t>(static_cast<std::int64_t>(value.x) * value.x)
        + static_cast<std::uint64_t>(static_cast<std::int64_t>(value.y) * value.y);
    const std::uint32_t length = IntegerSqrt(squared);
    if (length == 0) return Vec2Q8{kInputUnit, 0};
    return Vec2Q8{
        RoundDivideSigned(static_cast<std::int64_t>(value.x) * kInputUnit, static_cast<std::int32_t>(length)),
        RoundDivideSigned(static_cast<std::int64_t>(value.y) * kInputUnit, static_cast<std::int32_t>(length)),
    };
}

Vec2Q8 ScaleDirectionLocal(Vec2Q8 directionQ15, std::int32_t magnitude) noexcept {
    directionQ15 = NormalizeDirection(directionQ15);
    return Vec2Q8{
        RoundDivideSigned(static_cast<std::int64_t>(directionQ15.x) * magnitude, kInputUnit),
        RoundDivideSigned(static_cast<std::int64_t>(directionQ15.y) * magnitude, kInputUnit),
    };
}

bool ResolveActorCoverCollisions(
    CombatActor& actor,
    std::int32_t radiusPixels,
    const CombatState& combat) noexcept {
    bool collided = false;
    for (const CoverSlot& cover : combat.covers) {
        if (cover.active == 0) continue;
        const CollisionResolution collision = ResolveCircleAgainstAabb(
            actor.position,
            radiusPixels,
            cover.bounds);
        if (!collision.collided) continue;
        collided = true;
        actor.position.x += collision.shiftXQ8;
        actor.position.y += collision.shiftYQ8;
        if (collision.shiftXQ8 != 0) actor.velocity.x = 0;
        if (collision.shiftYQ8 != 0) actor.velocity.y = 0;
    }
    return collided;
}

void SetScene(Act1Runtime& state, Act1Scene scene) noexcept {
    state.scene = scene;
    state.sceneTicks = 0;
}

void MarkFatal(Act1Runtime& state) noexcept {
    state.failure = 1;
    SetScene(state, Act1Scene::Fatal);
}

bool EnterEncounter(Act1Runtime& state) noexcept {
    if (state.encounterIndex >= kAct1EncounterCount) {
        SetScene(state, Act1Scene::ContinueReady);
        return true;
    }
    const Act1EncounterDef& definition = GetAct1EncounterDef(state.encounterIndex);
    GameState movement{};
    if (!InitializeGameState(movement, 0x42503101u + state.encounterIndex)
        || !InitializeAct1Physical(movement.physicalLedger, state.claimLedger, definition)) {
        return false;
    }
    movement.claimLedger = state.claimLedger;
    movement.narrative = state.narrative;
    movement.playerPosition = Vec2Q8{definition.playerX * kQ8One, definition.playerY * kQ8One};
    movement.playerVelocity = Vec2Q8{0, 0};
    movement.globalTick = 0;

    CombatState combat{};
    SlotHandle neutralHandles[kAct1NeutralCount]{};
    if (!PopulateAct1Combat(combat, movement.physicalLedger, definition, neutralHandles)) {
        return false;
    }

    state.movement = movement;
    state.combat = combat;
    for (std::uint8_t index = 0; index < kAct1NeutralCount; ++index) {
        state.neutralHandles[index] = neutralHandles[index];
    }
    InitializeReclaimState(state.reclaim);
    InitializeHudLoanSystem(state.hudLoan);
    state.patch = PatchRuntime{};
    state.pair = PairRuntime{};
    state.vessel = VesselRuntime{};
    state.focusedChoice = ChoiceId::Second;
    state.committedChoice = ChoiceId::Second;
    state.encounterTick = 0;
    state.previousPlayerReclaim = 0;
    state.settlementStage = 0;
    state.initializedEncounter = 1;
    state.retryPending = 0;
    state.introRevisit = 0;

    if (definition.id == EncounterId::Patch) {
        for (std::uint8_t index = 0; index < kPatchSuccessFeedCount; ++index) {
            state.patch.feed[index] = neutralHandles[index];
        }
        state.patch.shellBaseline =
            state.movement.physicalLedger.mass[AccountIndex(PhysicalAccount::OpponentShell)];
        state.patch.transitNeutralIndex = kInvalidSlotIndex;
    } else if (definition.id == EncounterId::Pair) {
        state.pair.coreX[0] = 248;
        state.pair.coreY[0] = 70;
        state.pair.coreX[1] = 248;
        state.pair.coreY[1] = 110;
        state.pair.coreAliveMask = 3;
        constexpr std::int16_t spawnX[2] = {160, 160};
        constexpr std::int16_t spawnY[2] = {56, 124};
        for (std::uint8_t index = 0; index < 2; ++index) {
            PairProbe& probe = state.pair.probes[index];
            probe.position = Vec2Q8{spawnX[index] * kQ8One, spawnY[index] * kQ8One};
            probe.targetCore = index;
            probe.active = 1;
            probe.protocolNeutral = SlotHandle{kInvalidSlotIndex, 0};
            probe.directionQ15 = NormalizeDirection(Vec2Q8{
                static_cast<std::int32_t>(state.pair.coreX[index]) * kQ8One - probe.position.x,
                static_cast<std::int32_t>(state.pair.coreY[index]) * kQ8One - probe.position.y,
            });
        }
    } else {
        state.vessel.action = VesselAction::Press;
        state.vessel.pressDirectionQ15 = Vec2Q8{-kInputUnit, 0};
        state.vessel.actionTicks = kVesselPressTicks;
    }
    return HudLoanSystemValid(state.hudLoan, state.movement.physicalLedger)
        && CombatStateValid(state.combat, state.movement.physicalLedger);
}

Vec2Q8 AimDirection(const Act1Runtime& state, const InputFrame& input) noexcept {
    Vec2Q8 raw{};
    if (input.device == InputDevice::KeyboardMouse) {
        raw.x = static_cast<std::int32_t>(input.mouseX) * kQ8One
            - state.combat.actors[SideIndex(CombatSide::Player)].position.x;
        raw.y = static_cast<std::int32_t>(input.mouseY) * kQ8One
            - state.combat.actors[SideIndex(CombatSide::Player)].position.y;
    } else {
        raw = Vec2Q8{input.aimX, input.aimY};
    }
    if (raw.x == 0 && raw.y == 0) {
        return state.combat.actors[SideIndex(CombatSide::Player)].aimDirectionQ15;
    }
    return NormalizeDirection(raw);
}

bool BeginPatchFeed(Act1Runtime& state) noexcept {
    std::uint16_t selected = kInvalidSlotIndex;
    std::uint64_t bestDistance = UINT64_MAX;
    const Vec2Q8 patchPosition = state.combat.actors[SideIndex(CombatSide::Opponent)].position;
    for (std::uint16_t slotIndex = 0; slotIndex < kNeutralCapacity; ++slotIndex) {
        const NeutralSlot& candidate = state.combat.neutrals[slotIndex];
        if (candidate.active == 0 || candidate.mass < 8 || candidate.patchFeedMass < 8) continue;
        const std::uint64_t distance = DistanceSquared(
            candidate.position,
            patchPosition);
        if (distance < bestDistance
            || (distance == bestDistance
                && (selected == kInvalidSlotIndex
                    || state.combat.neutrals[slotIndex].stableId
                        < state.combat.neutrals[selected].stableId))) {
            selected = slotIndex;
            bestDistance = distance;
        }
    }
    if (selected == kInvalidSlotIndex) return true;

    CombatState combat = state.combat;
    PhysicalLedger ledger = state.movement.physicalLedger;
    NeutralSlot& feed = combat.neutrals[selected];
    if (!PhysicalTransfer(ledger, PhysicalAccount::Neutral, PhysicalAccount::Transit, 8)) {
        return false;
    }
    feed.mass = static_cast<std::uint16_t>(feed.mass - 8);
    feed.patchFeedMass = static_cast<std::uint16_t>(feed.patchFeedMass - 8);
    if (feed.mass == 0) {
        feed.active = 0;
    }
    if (!CombatStateValid(combat, ledger)) {
        return false;
    }
    state.combat = combat;
    state.movement.physicalLedger = ledger;
    state.patch.transitActive = 1;
    state.patch.transitTicks = 1;
    state.patch.transitNeutralIndex = selected;
    state.patch.shellBaseline = ledger.mass[AccountIndex(PhysicalAccount::OpponentShell)];
    return true;
}

bool AdvancePatchTransit(Act1Runtime& state) noexcept {
    PhysicalLedger& ledger = state.movement.physicalLedger;
    const std::uint16_t shell = ledger.mass[AccountIndex(PhysicalAccount::OpponentShell)];
    if (state.patch.transitActive != 0) {
        ++state.patch.transitTicks;
        if (state.patch.shellBaseline >= shell + kPatchFeedTriggerLoss) {
            state.patch.pendingReservation = 1;
        }
        if (state.patch.transitTicks >= kPatchFeedTransitTicks) {
            if (!PhysicalTransfer(ledger, PhysicalAccount::Transit, PhysicalAccount::OpponentShell, 8)) {
                return false;
            }
            state.patch.transitActive = 0;
            state.patch.transitTicks = 0;
            state.patch.transitNeutralIndex = kInvalidSlotIndex;
            ++state.patch.feedsUsed;
            state.patch.shellBaseline =
                ledger.mass[AccountIndex(PhysicalAccount::OpponentShell)];
        }
    }
    return true;
}

bool DecidePatch(Act1Runtime& state) noexcept {
    PhysicalLedger& ledger = state.movement.physicalLedger;
    const std::uint16_t shell = ledger.mass[AccountIndex(PhysicalAccount::OpponentShell)];
    if (state.patch.transitActive == 0
        && state.patch.feedsUsed < kPatchSuccessFeedCount
        && (state.patch.pendingReservation != 0
            || state.patch.shellBaseline >= shell + kPatchFeedTriggerLoss)) {
        state.patch.pendingReservation = 0;
        if (!BeginPatchFeed(state)) {
            return false;
        }
    }
    if (state.encounterTick != 0
        && state.encounterTick % kPatchTapCooldownTicks == 0
        && state.combat.actors[SideIndex(CombatSide::Opponent)].shotCooldownTicks == 0
        && ledger.mass[AccountIndex(PhysicalAccount::OpponentShell)] >= 8) {
        const Vec2Q8 from = state.combat.actors[SideIndex(CombatSide::Opponent)].position;
        const Vec2Q8 to = state.combat.actors[SideIndex(CombatSide::Player)].position;
        if (!SpawnScriptedProjectile(
                state.combat,
                ledger,
                CombatSide::Opponent,
                8,
                from,
                Vec2Q8{to.x - from.x, to.y - from.y},
                ProjectileSpeedQ8(8))) {
            return false;
        }
        state.combat.actors[SideIndex(CombatSide::Opponent)].shotCooldownTicks =
            kPatchTapCooldownTicks;
    }
    return true;
}

Vec2Q8 TurnAtMostSixDegrees(Vec2Q8 current, Vec2Q8 target) noexcept {
    current = NormalizeDirection(current);
    target = NormalizeDirection(target);
    const std::int64_t dot = static_cast<std::int64_t>(current.x) * target.x
        + static_cast<std::int64_t>(current.y) * target.y;
    constexpr std::int64_t cosSixThreshold =
        static_cast<std::int64_t>(32688) * kInputUnit;
    if (dot >= cosSixThreshold) {
        return target;
    }
    const std::int64_t cross = static_cast<std::int64_t>(current.x) * target.y
        - static_cast<std::int64_t>(current.y) * target.x;
    constexpr std::int32_t cosSix = 32688;
    constexpr std::int32_t sinSix = 3425;
    const std::int32_t sign = cross >= 0 ? 1 : -1;
    return NormalizeDirection(Vec2Q8{
        RoundDivideSigned(
            static_cast<std::int64_t>(current.x) * cosSix
                - static_cast<std::int64_t>(current.y) * sinSix * sign,
            kInputUnit),
        RoundDivideSigned(
            static_cast<std::int64_t>(current.y) * cosSix
                + static_cast<std::int64_t>(current.x) * sinSix * sign,
            kInputUnit),
    });
}

bool DisableProbeFromProjectile(
    Act1Runtime& state,
    std::uint8_t probeIndex,
    std::uint16_t projectileIndex) noexcept {
    CombatState combat = state.combat;
    PhysicalLedger ledger = state.movement.physicalLedger;
    ProjectileSlot& shot = combat.projectiles[projectileIndex];
    PairProbe& probe = state.pair.probes[probeIndex];
    const std::uint16_t neutralMass = static_cast<std::uint16_t>(shot.mass + 16);
    SlotHandle handle{};
    if (!AddNeutral(combat, probe.position, neutralMass, shot.mass, 0, handle)
        || !PhysicalTransfer(ledger, PhysicalAccount::Projectile, PhysicalAccount::Neutral, shot.mass)
        || !PhysicalTransfer(ledger, PhysicalAccount::ProtocolActive, PhysicalAccount::Neutral, 16)) {
        return false;
    }
    combat.neutrals[handle.index].protocolTagMass = 16;
    shot.mass = 0;
    shot.active = 0;
    if (!CombatStateValid(combat, ledger)) {
        return false;
    }
    state.combat = combat;
    state.movement.physicalLedger = ledger;
    probe.active = 0;
    probe.respawnTicks = 120;
    probe.protocolNeutral = handle;
    return true;
}

bool TryRespawnProbe(Act1Runtime& state, PairProbe& probe, std::uint8_t probeIndex) noexcept {
    if (probe.active != 0) {
        return true;
    }
    if (probe.respawnTicks > 0) {
        --probe.respawnTicks;
        if (probe.respawnTicks > 0) return true;
    }
    std::uint16_t totalTagged = 0;
    for (const NeutralSlot& neutral : state.combat.neutrals) {
        if (neutral.active != 0) totalTagged = static_cast<std::uint16_t>(totalTagged + neutral.protocolTagMass);
    }
    if (totalTagged < 16) return true;
    CombatState combat = state.combat;
    PhysicalLedger ledger = state.movement.physicalLedger;
    constexpr std::int16_t spawnY[2] = {56, 124};
    const Vec2Q8 spawn{160 * kQ8One, spawnY[probeIndex] * kQ8One};
    std::uint16_t remaining = 16;
    while (remaining > 0) {
        std::uint16_t selected = kInvalidSlotIndex;
        std::uint64_t nearest = UINT64_MAX;
        std::uint32_t stableId = UINT32_MAX;
        for (std::uint16_t index = 0; index < kNeutralCapacity; ++index) {
            const NeutralSlot& neutral = combat.neutrals[index];
            if (neutral.active == 0 || neutral.protocolTagMass == 0) continue;
            const std::uint64_t distance = DistanceSquared(neutral.position, spawn);
            if (selected == kInvalidSlotIndex || distance < nearest
                || (distance == nearest && neutral.stableId < stableId)) {
                selected = index;
                nearest = distance;
                stableId = neutral.stableId;
            }
        }
        if (selected == kInvalidSlotIndex) return false;
        NeutralSlot& neutral = combat.neutrals[selected];
        const std::uint16_t amount = neutral.protocolTagMass < remaining
            ? neutral.protocolTagMass : remaining;
        neutral.protocolTagMass = static_cast<std::uint16_t>(neutral.protocolTagMass - amount);
        neutral.mass = static_cast<std::uint16_t>(neutral.mass - amount);
        remaining = static_cast<std::uint16_t>(remaining - amount);
        if (neutral.mass == 0) neutral.active = 0;
    }
    if (!PhysicalTransfer(ledger, PhysicalAccount::Neutral, PhysicalAccount::ProtocolActive, 16)
        || !CombatStateValid(combat, ledger)) {
        return false;
    }
    state.combat = combat;
    state.movement.physicalLedger = ledger;
    probe.position = spawn;
    probe.active = 1;
    probe.protocolNeutral = SlotHandle{kInvalidSlotIndex, 0};
    return true;
}

bool TickPair(Act1Runtime& state) noexcept {
    for (std::uint8_t probeIndex = 0; probeIndex < 2; ++probeIndex) {
        PairProbe& probe = state.pair.probes[probeIndex];
        if (!TryRespawnProbe(state, probe, probeIndex) || probe.active == 0) {
            continue;
        }
        probe.targetCore = static_cast<std::uint8_t>(
            ((state.encounterTick / kPairProbeRetargetTicks) + probeIndex) & 1u);
        const Vec2Q8 target{
            static_cast<std::int32_t>(state.pair.coreX[probe.targetCore]) * kQ8One,
            static_cast<std::int32_t>(state.pair.coreY[probe.targetCore]) * kQ8One,
        };
        probe.directionQ15 = TurnAtMostSixDegrees(
            probe.directionQ15,
            Vec2Q8{target.x - probe.position.x, target.y - probe.position.y});
        const Vec2Q8 velocity = ScaleDirectionLocal(
            probe.directionQ15,
            PixelsPerSecondToQ8PerTick(kPairProbeSpeedPixelsPerSecond));
        probe.position.x += velocity.x;
        probe.position.y += velocity.y;

        for (std::uint16_t shotIndex = 0; shotIndex < kProjectileCapacity; ++shotIndex) {
            const ProjectileSlot& shot = state.combat.projectiles[shotIndex];
            if (shot.active == 0 || shot.owner != CombatSide::Player) {
                continue;
            }
            const std::int32_t radius = ProjectileRadiusPixels(shot.mass) + 4;
            if (SweptWithinRadius(
                    shot.position,
                    shot.velocity,
                    probe.position,
                    radius * kQ8One)) {
                if (!DisableProbeFromProjectile(state, probeIndex, shotIndex)) {
                    return false;
                }
                break;
            }
        }
        if (probe.active != 0
            && DistanceSquared(probe.position, target)
                <= static_cast<std::uint64_t>(5 * kQ8One) * (5 * kQ8One)) {
            if (!PhysicalTransfer(
                    state.movement.physicalLedger,
                    PhysicalAccount::OpponentCore,
                    PhysicalAccount::Transit,
                    16)) {
                return false;
            }
            state.pair.coreAliveMask = static_cast<std::uint8_t>(
                state.pair.coreAliveMask & ~(1u << probe.targetCore));
            state.pair.forcedSelect = 1;
            state.focusedChoice = ChoiceId::First;
            return true;
        }
    }
    return true;
}

Vec2Q8 DirectionBetween(Vec2Q8 from, Vec2Q8 to) noexcept {
    return NormalizeDirection(Vec2Q8{to.x - from.x, to.y - from.y});
}

bool VesselIncomingThreat(const Act1Runtime& state) noexcept {
    for (std::uint16_t projectileIndex = 0; projectileIndex < kProjectileCapacity; ++projectileIndex) {
        const ProjectileSlot& shot = state.combat.projectiles[projectileIndex];
        if (shot.active == 0 || shot.owner != CombatSide::Player || shot.mass < 16) {
            continue;
        }
        CombatState projected = state.combat;
        for (std::uint8_t tick = 0; tick < 30; ++tick) {
            projected.actors[SideIndex(CombatSide::Opponent)].position.x +=
                projected.actors[SideIndex(CombatSide::Opponent)].velocity.x;
            projected.actors[SideIndex(CombatSide::Opponent)].position.y +=
                projected.actors[SideIndex(CombatSide::Opponent)].velocity.y;
            const ImpactCandidate impact = FindEarliestImpact(
                projected, state.movement.physicalLedger, projectileIndex);
            if (impact.kind != ImpactKind::None) {
                if ((impact.kind == ImpactKind::Core || impact.kind == ImpactKind::Shell)
                    && impact.targetIndex == SideIndex(CombatSide::Opponent)) {
                    return true;
                }
                break;
            }
            for (ProjectileSlot& moving : projected.projectiles) {
                if (moving.active == 0) continue;
                moving.position.x += moving.velocity.x;
                moving.position.y += moving.velocity.y;
            }
        }
    }
    return false;
}

bool VesselLineOfSight(const Act1Runtime& state) noexcept {
    const Vec2Q8 fromQ8 = state.combat.actors[SideIndex(CombatSide::Opponent)].position;
    const Vec2Q8 toQ8 = state.combat.actors[SideIndex(CombatSide::Player)].position;
    std::int32_t x0 = fromQ8.x / kQ8One;
    std::int32_t y0 = fromQ8.y / kQ8One;
    const std::int32_t x1 = toQ8.x / kQ8One;
    const std::int32_t y1 = toQ8.y / kQ8One;
    const std::int32_t dx = x1 > x0 ? x1 - x0 : x0 - x1;
    const std::int32_t sx = x0 < x1 ? 1 : -1;
    const std::int32_t dyAbs = y1 > y0 ? y1 - y0 : y0 - y1;
    const std::int32_t dy = -dyAbs;
    const std::int32_t sy = y0 < y1 ? 1 : -1;
    std::int32_t error = dx + dy;
    for (;;) {
        for (const CoverSlot& cover : state.combat.covers) {
            if (cover.active != 0
                && x0 >= cover.bounds.x && x0 < cover.bounds.x + cover.bounds.width
                && y0 >= cover.bounds.y && y0 < cover.bounds.y + cover.bounds.height) {
                return false;
            }
        }
        if (x0 == x1 && y0 == y1) break;
        const std::int32_t twice = error * 2;
        if (twice >= dy) { error += dy; x0 += sx; }
        if (twice <= dx) { error += dx; y0 += sy; }
    }
    return true;
}

std::uint16_t NeutralMassNear(const Act1Runtime& state, Vec2Q8 point, std::int32_t range) noexcept {
    std::uint32_t total = 0;
    const std::uint64_t maximum = static_cast<std::uint64_t>(range * kQ8One) * (range * kQ8One);
    for (const NeutralSlot& neutral : state.combat.neutrals) {
        if (neutral.active != 0 && DistanceSquared(neutral.position, point) <= maximum) {
            total += neutral.mass;
        }
    }
    return static_cast<std::uint16_t>(total > 65535 ? 65535 : total);
}

Vec2Q8 NearestNeutralDirection(const Act1Runtime& state, Vec2Q8 from) noexcept {
    const NeutralSlot* selected = nullptr;
    std::uint64_t best = UINT64_MAX;
    for (const NeutralSlot& neutral : state.combat.neutrals) {
        if (neutral.active == 0) {
            continue;
        }
        const std::uint64_t distance = DistanceSquared(from, neutral.position);
        if (distance < best
            || (distance == best && selected != nullptr && neutral.stableId < selected->stableId)) {
            selected = &neutral;
            best = distance;
        }
    }
    return selected == nullptr ? Vec2Q8{-kInputUnit, 0} : DirectionBetween(from, selected->position);
}

void ChooseVesselAction(Act1Runtime& state) noexcept {
    VesselRuntime& vesselState = state.vessel;
    const Vec2Q8 vessel = state.combat.actors[SideIndex(CombatSide::Opponent)].position;
    const Vec2Q8 player = state.combat.actors[SideIndex(CombatSide::Player)].position;
    const std::uint16_t shell =
        state.movement.physicalLedger.mass[AccountIndex(PhysicalAccount::OpponentShell)];
    const std::uint64_t distance = DistanceSquared(vessel, player);
    const std::uint64_t farDistance =
        static_cast<std::uint64_t>(132 * kQ8One) * (132 * kQ8One);
    const std::uint64_t nearDistance =
        static_cast<std::uint64_t>(72 * kQ8One) * (72 * kQ8One);

    const bool pressAvailable = vesselState.pressCooldown == 0;
    if (vesselState.braceCooldown == 0 && VesselIncomingThreat(state)) {
        vesselState.action = VesselAction::Brace;
        vesselState.actionTicks = 0;
        vesselState.braceCooldown = kVesselBraceCooldownTicks;
    } else if (shell <= 96 && NeutralMassNear(state, vessel, kVesselReclaimRangePixels) >= 16) {
        vesselState.action = VesselAction::Reclaim;
        vesselState.actionTicks = 0;
    } else if (distance > farDistance && pressAvailable) {
        vesselState.action = VesselAction::Press;
        vesselState.actionTicks = 0;
        vesselState.pressDirectionQ15 = DirectionBetween(vessel, player);
    } else if (VesselLineOfSight(state) && shell >= 80 && vesselState.stampCooldown == 0) {
        vesselState.action = VesselAction::Stamp;
        vesselState.actionTicks = 0;
        vesselState.stampTelegraph = kVesselStampTelegraphTicks;
    } else if (distance < nearDistance && pressAvailable) {
        vesselState.action = VesselAction::Press;
        vesselState.actionTicks = 0;
        const Vec2Q8 toward = DirectionBetween(vessel, player);
        vesselState.pressDirectionQ15 = Vec2Q8{-toward.x, -toward.y};
    } else if (pressAvailable) {
        vesselState.action = VesselAction::Press;
        vesselState.actionTicks = 0;
        vesselState.pressDirectionQ15 = NearestNeutralDirection(state, vessel);
    }
}

bool DeflectVesselProjectiles(Act1Runtime& state) noexcept {
    CombatState combat = state.combat;
    PhysicalLedger ledger = state.movement.physicalLedger;
    const Vec2Q8 vessel = combat.actors[SideIndex(CombatSide::Opponent)].position;
    bool changed = false;
    for (std::uint16_t index = 0; index < kProjectileCapacity; ++index) {
        ProjectileSlot& shot = combat.projectiles[index];
        if (shot.active == 0 || shot.owner != CombatSide::Player) {
            continue;
        }
        const ImpactCandidate impact = FindEarliestImpact(combat, ledger, index);
        if ((impact.kind != ImpactKind::Core && impact.kind != ImpactKind::Shell)
            || impact.targetIndex != SideIndex(CombatSide::Opponent)) {
            continue;
        }
        const Vec2Q8 front = DirectionBetween(vessel, combat.actors[SideIndex(CombatSide::Player)].position);
        const Vec2Q8 incoming = NormalizeDirection(Vec2Q8{-shot.velocity.x, -shot.velocity.y});
        const std::int64_t dot = static_cast<std::int64_t>(front.x) * incoming.x
            + static_cast<std::int64_t>(front.y) * incoming.y;
        constexpr std::int64_t cosThirtyFive = static_cast<std::int64_t>(26842) * kInputUnit;
        if (dot < cosThirtyFive) {
            continue;
        }
        const std::uint32_t shotStableId = shot.stableId;
        const Vec2Q8 shotVelocity = shot.velocity;
        const std::uint16_t shotMass = shot.mass;
        SlotHandle neutralHandle{};
        if (!AddNeutral(combat, impact.point, shotMass, shotMass, 0, neutralHandle)
            || !PhysicalTransfer(ledger, PhysicalAccount::Projectile, PhysicalAccount::Neutral, shot.mass)) {
            return false;
        }
        constexpr std::int32_t cosSeventy = 11207;
        constexpr std::int32_t sinSeventy = 30792;
        const std::int32_t sign = (shotStableId & 1u) == 0 ? 1 : -1;
        combat.neutrals[neutralHandle.index].velocity = Vec2Q8{
            RoundDivideSigned(
                static_cast<std::int64_t>(shotVelocity.x) * cosSeventy
                    - static_cast<std::int64_t>(shotVelocity.y) * sinSeventy * sign,
                kInputUnit),
            RoundDivideSigned(
                static_cast<std::int64_t>(shotVelocity.y) * cosSeventy
                    + static_cast<std::int64_t>(shotVelocity.x) * sinSeventy * sign,
                kInputUnit),
        };
        shot.mass = 0;
        shot.active = 0;
        changed = true;
    }
    if (changed && !CombatStateValid(combat, ledger)) {
        return false;
    }
    state.combat = combat;
    state.movement.physicalLedger = ledger;
    return true;
}

void MoveVessel(Act1Runtime& state, std::uint8_t speedPercent) noexcept {
    CombatActor& actor = state.combat.actors[SideIndex(CombatSide::Opponent)];
    const std::int32_t acceleration = PixelsPerSecondSquaredToQ8PerTickSquared(420);
    actor.velocity.x += RoundDivideSigned(
        static_cast<std::int64_t>(state.vessel.pressDirectionQ15.x) * acceleration,
        kInputUnit);
    actor.velocity.y += RoundDivideSigned(
        static_cast<std::int64_t>(state.vessel.pressDirectionQ15.y) * acceleration,
        kInputUnit);
    std::int32_t maximum = LargeOpponentMaximumSpeedQ8(
        state.movement.physicalLedger.mass[AccountIndex(PhysicalAccount::OpponentShell)]);
    maximum = TruncMultiplyDivide(maximum, speedPercent, 100);
    ClampVectorLength(actor.velocity, maximum);
    actor.position.x += actor.velocity.x;
    actor.position.y += actor.velocity.y;
    const CollisionResolution boundary = ClampCircleToRect(
        actor.position,
        ActorRadiusPixels(state.movement.physicalLedger.mass[AccountIndex(PhysicalAccount::OpponentShell)]),
        kPlayInterior);
    if (boundary.shiftXQ8 != 0) actor.velocity.x = 0;
    if (boundary.shiftYQ8 != 0) actor.velocity.y = 0;
    ResolveActorCoverCollisions(
        actor,
        ActorRadiusPixels(state.movement.physicalLedger.mass[AccountIndex(PhysicalAccount::OpponentShell)]),
        state.combat);
}

void SeparateActorsByMass(Act1Runtime& state) noexcept {
    CombatActor& player = state.combat.actors[SideIndex(CombatSide::Player)];
    CombatActor& opponent = state.combat.actors[SideIndex(CombatSide::Opponent)];
    const std::uint16_t playerShell = state.movement.physicalLedger.mass[
        AccountIndex(PhysicalAccount::PlayerShell)];
    const std::uint16_t opponentShell = state.movement.physicalLedger.mass[
        AccountIndex(PhysicalAccount::OpponentShell)];
    const std::int32_t playerRadius = ActorRadiusPixels(playerShell);
    const std::int32_t opponentRadius = ActorRadiusPixels(opponentShell);
    const std::uint32_t playerMass = static_cast<std::uint32_t>(playerShell)
        + state.movement.physicalLedger.mass[AccountIndex(PhysicalAccount::PlayerCore)];
    const std::uint32_t opponentMass = static_cast<std::uint32_t>(opponentShell)
        + state.movement.physicalLedger.mass[AccountIndex(PhysicalAccount::OpponentCore)];
    const std::uint32_t totalMass = playerMass + opponentMass;
    if (totalMass == 0) return;

    auto constrain = [&](CombatActor& actor, std::int32_t radius) noexcept {
        ClampCircleToRect(actor.position, radius, kPlayInterior);
        ResolveActorCoverCollisions(actor, radius, state.combat);
    };

    // A boundary or Cover can cancel one actor's weighted displacement. After
    // the required mass-proportional move, assign any cancelled remainder to
    // each actor in deterministic player/opponent order. The bounded outer pass
    // handles direction changes caused by a Cover corner without geometric-only
    // convergence at extreme mass ratios.
    for (std::uint8_t pass = 0; pass < 32; ++pass) {
        std::int64_t dx = static_cast<std::int64_t>(opponent.position.x) - player.position.x;
        std::int64_t dy = static_cast<std::int64_t>(opponent.position.y) - player.position.y;
        std::uint32_t distance = IntegerSqrt(static_cast<std::uint64_t>(dx * dx + dy * dy));
        std::int32_t overlap = (playerRadius + opponentRadius) * kQ8One
            - static_cast<std::int32_t>(distance);
        if (overlap <= 0) break;
        Vec2Q8 direction = distance == 0
            ? Vec2Q8{kInputUnit, 0}
            : NormalizeDirection(Vec2Q8{static_cast<std::int32_t>(dx), static_cast<std::int32_t>(dy)});
        const std::int32_t playerShift = static_cast<std::int32_t>(
            static_cast<std::int64_t>(overlap) * opponentMass / totalMass);
        const std::int32_t opponentShift = overlap - playerShift;
        player.position.x -= RoundDivideSigned(
            static_cast<std::int64_t>(direction.x) * playerShift, kInputUnit);
        player.position.y -= RoundDivideSigned(
            static_cast<std::int64_t>(direction.y) * playerShift, kInputUnit);
        opponent.position.x += RoundDivideSigned(
            static_cast<std::int64_t>(direction.x) * opponentShift, kInputUnit);
        opponent.position.y += RoundDivideSigned(
            static_cast<std::int64_t>(direction.y) * opponentShift, kInputUnit);
        constrain(player, playerRadius);
        constrain(opponent, opponentRadius);

        dx = static_cast<std::int64_t>(opponent.position.x) - player.position.x;
        dy = static_cast<std::int64_t>(opponent.position.y) - player.position.y;
        distance = IntegerSqrt(static_cast<std::uint64_t>(dx * dx + dy * dy));
        overlap = (playerRadius + opponentRadius) * kQ8One
            - static_cast<std::int32_t>(distance);
        if (overlap <= 0) break;
        direction = distance == 0
            ? Vec2Q8{kInputUnit, 0}
            : NormalizeDirection(Vec2Q8{static_cast<std::int32_t>(dx), static_cast<std::int32_t>(dy)});
        player.position.x -= RoundDivideSigned(
            static_cast<std::int64_t>(direction.x) * overlap, kInputUnit);
        player.position.y -= RoundDivideSigned(
            static_cast<std::int64_t>(direction.y) * overlap, kInputUnit);
        constrain(player, playerRadius);

        dx = static_cast<std::int64_t>(opponent.position.x) - player.position.x;
        dy = static_cast<std::int64_t>(opponent.position.y) - player.position.y;
        distance = IntegerSqrt(static_cast<std::uint64_t>(dx * dx + dy * dy));
        overlap = (playerRadius + opponentRadius) * kQ8One
            - static_cast<std::int32_t>(distance);
        if (overlap <= 0) break;
        direction = distance == 0
            ? Vec2Q8{kInputUnit, 0}
            : NormalizeDirection(Vec2Q8{static_cast<std::int32_t>(dx), static_cast<std::int32_t>(dy)});
        opponent.position.x += RoundDivideSigned(
            static_cast<std::int64_t>(direction.x) * overlap, kInputUnit);
        opponent.position.y += RoundDivideSigned(
            static_cast<std::int64_t>(direction.y) * overlap, kInputUnit);
        constrain(opponent, opponentRadius);
    }
    state.movement.playerPosition = player.position;
}

bool AdvanceVesselScheduledEvents(Act1Runtime& state) noexcept {
    VesselRuntime& vessel = state.vessel;
    if (vessel.pressCooldown > 0) --vessel.pressCooldown;
    if (vessel.braceCooldown > 0) --vessel.braceCooldown;
    if (vessel.stampCooldown > 0) --vessel.stampCooldown;
    if (vessel.action == VesselAction::Stamp && vessel.stampTelegraph > 0) {
        --vessel.stampTelegraph;
        if (vessel.stampTelegraph == 0) {
            const Vec2Q8 from = state.combat.actors[SideIndex(CombatSide::Opponent)].position;
            const Vec2Q8 to = state.combat.actors[SideIndex(CombatSide::Player)].position;
            if (!SpawnScriptedProjectile(
                    state.combat,
                    state.movement.physicalLedger,
                    CombatSide::Opponent,
                    kVesselStampMass,
                    from,
                    Vec2Q8{to.x - from.x, to.y - from.y},
                    PixelsPerSecondToQ8PerTick(kVesselStampSpeedPixelsPerSecond),
                    static_cast<std::uint8_t>(kVesselStampWidth),
                    static_cast<std::uint8_t>(kVesselStampHeight))) {
                return false;
            }
            vessel.stampCooldown = kVesselStampCooldownTicks;
        }
    }
    return true;
}

void DecideVessel(Act1Runtime& state) noexcept {
    VesselRuntime& vessel = state.vessel;
    const std::uint16_t minimumActionTicks = vessel.action == VesselAction::Brace
        ? kVesselBraceTicks
        : (vessel.action == VesselAction::Press ? kVesselPressTicks : 30);
    if (state.encounterTick % 30 == 0 && vessel.actionTicks >= minimumActionTicks) {
        ChooseVesselAction(state);
    }
}

void AdvanceVesselMovement(Act1Runtime& state) noexcept {
    VesselRuntime& vessel = state.vessel;
    switch (vessel.action) {
    case VesselAction::Brace:
        MoveVessel(state, kVesselBraceSpeedPercent);
        break;
    case VesselAction::Stamp:
    case VesselAction::Reclaim:
        break;
    case VesselAction::Press:
    default:
        if (vessel.pressCooldown == 0 && vessel.actionTicks < kVesselPressTicks) {
            MoveVessel(state, 100);
        }
        break;
    }
    if (vessel.actionTicks < UINT16_MAX) ++vessel.actionTicks;
    if (vessel.action == VesselAction::Press
        && vessel.actionTicks == kVesselPressTicks
        && vessel.pressCooldown == 0) {
        vessel.pressCooldown = kVesselPressCooldownTicks;
    }
}

void BeginSuccess(Act1Runtime& state) noexcept {
    state.retryPending = 0;
    state.combat.phase = CombatPhase::HitFreeze;
    SetScene(state, Act1Scene::HitFreeze);
}

void BeginRetry(Act1Runtime& state) noexcept {
    state.retryPending = 1;
    state.combat.phase = CombatPhase::HitFreeze;
    SetScene(state, Act1Scene::HitFreeze);
}

void ClearInterruptedActions(Act1Runtime& state) noexcept {
    state.movement.chargeActive = 0;
    state.movement.reclaimActive = 0;
    state.movement.hudChannelActive = 0;
    ChargeSlot& charge = state.combat.charges[SideIndex(CombatSide::Player)];
    if (charge.active != 0) {
        RollbackCharge(state.combat, state.movement.physicalLedger, CombatSide::Player);
    }
}

bool CanStartDash(const Act1Runtime& state) noexcept {
    return state.movement.impulseInstalled != 0
        && state.movement.dashCooldown == 0
        && state.movement.dashTicksRemaining == 0
        && state.movement.chargeActive == 0
        && state.movement.reclaimActive == 0
        && state.movement.hudChannelActive == 0;
}

void ClearAction(InputFrame& input, PlatformAction action) noexcept {
    input.actions[ActionIndex(action)] = ButtonState{};
}

bool TickPlaying(Act1Runtime& state, const InputFrame& input) noexcept {
    PhysicalLedger& ledger = state.movement.physicalLedger;
    const Vec2Q8 aim = AimDirection(state, input);

    // An already-paused simulation remains frozen. Its pause edge only resumes
    // the gate; simulation work begins on the following call.
    if (state.movement.paused != 0) {
        if (input.focused != 0 && Pressed(input, PlatformAction::Pause)) {
            state.movement.paused = 0;
            ClearInterruptedActions(state);
        }
        return true;
    }

    // Authority 32.9 step 02: countdowns and scheduled encounter events.
    if (state.movement.dashCooldown > 0) --state.movement.dashCooldown;
    if (state.movement.dashInvulnerableRemaining > 0) --state.movement.dashInvulnerableRemaining;
    TickCombatCountdowns(state.combat);
    if (state.encounterIndex == static_cast<std::uint8_t>(EncounterId::Patch)) {
        if (!AdvancePatchTransit(state)) return false;
    } else if (state.encounterIndex == static_cast<std::uint8_t>(EncounterId::Vessel)) {
        if (!AdvanceVesselScheduledEvents(state)) return false;
    }

    // Authority 32.9 step 03 follows the one step-02 countdown pass on a new
    // pause/focus-loss edge, then gates every later simulation step.
    if (input.focused == 0 || Pressed(input, PlatformAction::Pause)) {
        state.movement.paused = 1;
        ClearInterruptedActions(state);
        return true;
    }

    InputFrame effectiveInput = input;
    const bool activeDash = state.movement.dashTicksRemaining > 0;
    if (activeDash) {
        ClearAction(effectiveInput, PlatformAction::HudReallocate);
        ClearAction(effectiveInput, PlatformAction::Dash);
        ClearAction(effectiveInput, PlatformAction::Fire);
        ClearAction(effectiveInput, PlatformAction::Reclaim);
    }
    state.movement.claimLedger = state.claimLedger;
    state.movement.narrative = state.narrative;
    state.movement.impulseInstalled = HudModuleFullyInstalled(state.hudLoan, HudModuleId::Impulse) ? 1u : 0u;
    GameplayIntent intent = ResolveGameplayInput(
        state.movement.inputArbiter,
        effectiveInput,
        CanStartDash(state),
        true);

    // Steps 04-06: HUD transit/maturity/repayment, then input channeling.
    if (!AdvanceHudLoanEarly(
            state.hudLoan,
            ledger,
            intent.action == GameplayAction::HudReallocate,
            state.previousPlayerReclaim)) {
        return false;
    }
    state.previousPlayerReclaim = 0;
    if (Pressed(input, PlatformAction::HudCycle)) {
        const std::uint8_t next = static_cast<std::uint8_t>(
            (static_cast<std::uint8_t>(state.hudLoan.selected) + 1u) % kHudModuleCount);
        SelectHudModule(state.hudLoan, static_cast<HudModuleId>(next));
    }

    // Step 07: encounter policy decisions.
    if (state.encounterIndex == static_cast<std::uint8_t>(EncounterId::Patch)) {
        if (!DecidePatch(state)) return false;
    } else if (state.encounterIndex == static_cast<std::uint8_t>(EncounterId::Vessel)) {
        DecideVessel(state);
    }

    // Step 08: charge reservation and release.
    CombatActor& player = state.combat.actors[SideIndex(CombatSide::Player)];
    player.aimDirectionQ15 = aim;
    ChargeSlot& playerCharge = state.combat.charges[SideIndex(CombatSide::Player)];
    const bool fireHeld = intent.action == GameplayAction::Fire
        && !HudFireLocked(state.hudLoan);
    if (intent.action == GameplayAction::HudReallocate && playerCharge.active != 0) {
        if (!RollbackCharge(state.combat, ledger, CombatSide::Player)) return false;
    } else if (fireHeld) {
        if (playerCharge.active == 0) {
            if (player.shotCooldownTicks == 0
                && ledger.mass[AccountIndex(PhysicalAccount::PlayerShell)] >= 8
                && !BeginCharge(state.combat, ledger, CombatSide::Player, aim)) {
                return false;
            }
        } else if (!AdvanceCharge(state.combat, ledger, CombatSide::Player)) {
            return false;
        }
    } else if (playerCharge.active != 0) {
        if (Held(input, PlatformAction::Fire) || input.actions[ActionIndex(PlatformAction::Fire)].released != 0) {
            if (!ReleaseCharge(state.combat, ledger, CombatSide::Player)) {
                return false;
            }
        } else if (!RollbackCharge(state.combat, ledger, CombatSide::Player)) {
            return false;
        }
    }

    // Steps 09-10: acceleration, movement, dash, cover and body separation.
    state.movement.chargeActive = playerCharge.active != 0 ? 1u : 0u;
    if (!AdvanceGameIntent(state.movement, intent)) {
        return false;
    }
    player.position = state.movement.playerPosition;
    player.velocity = state.movement.playerVelocity;
    const bool playerHitCover = ResolveActorCoverCollisions(
        player,
        ActorRadiusPixels(ledger.mass[AccountIndex(PhysicalAccount::PlayerShell)]),
        state.combat);
    if (playerHitCover && state.movement.dashTicksRemaining > 0) {
        state.movement.dashTicksRemaining = 0;
        state.movement.dashInvulnerableRemaining = 0;
        state.movement.playerVelocity = Vec2Q8{0, 0};
        player.velocity = Vec2Q8{0, 0};
    }
    state.movement.playerPosition = player.position;
    state.movement.playerVelocity = player.velocity;
    if (state.encounterIndex == static_cast<std::uint8_t>(EncounterId::Vessel)) {
        AdvanceVesselMovement(state);
    }
    SeparateActorsByMass(state);
    player.projectileImmune = DashIgnoresProjectiles(state.movement) ? 1u : 0u;

    // Steps 11-12: swept projectile movement and deterministic TOI resolution.
    if (state.encounterIndex == static_cast<std::uint8_t>(EncounterId::Pair)) {
        if (!TickPair(state)) return false;
    } else if (state.encounterIndex == static_cast<std::uint8_t>(EncounterId::Vessel)
        && state.vessel.action == VesselAction::Brace
        && !DeflectVesselProjectiles(state)) {
        return false;
    }
    if (!StepCombatProjectiles(state.combat, ledger, state.claimLedger)) {
        return false;
    }

    // Step 13: a core snapshot ends this tick before neutral/HUD/success work.
    const bool playerHit = state.combat.actors[SideIndex(CombatSide::Player)].coreHit != 0;
    const bool opponentHit = state.combat.actors[SideIndex(CombatSide::Opponent)].coreHit != 0;
    if (playerHit||opponentHit) ++state.audioCoreHitSerial;
    if (playerHit || opponentHit || state.pair.forcedSelect != 0) {
        ++state.movement.globalTick;
        ++state.encounterTick;
        ++state.globalTick;
        if (!PhysicalLedgerValid(ledger)
            || !ClaimLedgerValid(state.claimLedger)
            || !CombatStateValid(state.combat, ledger)
            || !HudLoanSystemValid(state.hudLoan, ledger)) {
            return false;
        }
        if (playerHit) {
            BeginRetry(state);
        } else if (state.pair.forcedSelect != 0
            || state.encounterIndex == static_cast<std::uint8_t>(EncounterId::Vessel)) {
            BeginSuccess(state);
        } else {
            state.combat.phase = CombatPhase::Active;
            state.combat.actors[SideIndex(CombatSide::Opponent)].coreHit = 0;
        }
        return true;
    }

    // Steps 14-16: neutral inertia, reclaim, then merge/slot recovery.
    if (!TickNeutralDynamics(state.combat, ledger)) return false;
    ReclaimTickResult reclaimResult{};
    if (!TickReclaim(
            state.combat,
            ledger,
            state.reclaim,
            intent.action == GameplayAction::Reclaim,
            state.encounterIndex == static_cast<std::uint8_t>(EncounterId::Vessel)
                && state.vessel.action == VesselAction::Reclaim,
            reclaimResult)) {
        return false;
    }
    state.previousPlayerReclaim = reclaimResult.playerMass;
    if (!MergeNeutralClusters(state.combat, ledger)) {
        return false;
    }

    // Step 17: pending HUD commits are cancelled by a same-tick shell hit.
    const bool playerShellHit = state.combat.shellDamageThisTick[SideIndex(CombatSide::Player)] != 0;
    if (!ResolvePendingHudCommit(state.hudLoan, ledger, playerShellHit)) {
        return false;
    }
    state.movement.playerPosition = player.position;
    state.movement.playerVelocity = player.velocity;

    // Steps 18-20: success, invariant/hash boundary, then tick advance.
    if (!PhysicalLedgerValid(ledger)
        || !ClaimLedgerValid(state.claimLedger)
        || !CombatStateValid(state.combat, ledger)
        || !HudLoanSystemValid(state.hudLoan, ledger)) {
        return false;
    }

    if (state.encounterIndex == static_cast<std::uint8_t>(EncounterId::Patch)
        && state.patch.feedsUsed >= kPatchSuccessFeedCount) {
        BeginSuccess(state);
    } else if (state.encounterIndex == static_cast<std::uint8_t>(EncounterId::Pair)
        && (state.pair.forcedSelect != 0 || state.encounterTick + 1u >= kPairDurationTicks)) {
        BeginSuccess(state);
    } else if (state.encounterIndex == static_cast<std::uint8_t>(EncounterId::Vessel)
        && opponentHit) {
        BeginSuccess(state);
    } else if (state.combat.phase == CombatPhase::HitFreeze) {
        state.combat.phase = CombatPhase::Active;
        state.combat.actors[SideIndex(CombatSide::Opponent)].coreHit = 0;
    }
    ++state.movement.globalTick;
    ++state.encounterTick;
    ++state.globalTick;
    return true;
}

bool ApplyCommittedChoice(Act1Runtime& state) noexcept {
    const EncounterId encounter = static_cast<EncounterId>(state.encounterIndex);
    if (encounter == EncounterId::Vessel && state.settlementStage == 1) {
        if (!ApplyVesselMinimumClaim(state.claimLedger, state.narrative)) {
            return false;
        }
        state.narrative.encounterIndex = 3;
        if (state.committedChoice == ChoiceId::First) {
            state.narrative.endingsSeen = static_cast<std::uint8_t>(state.narrative.endingsSeen | 1u);
            SetScene(state, Act1Scene::EnoughEnding);
        } else {
            state.settlementStage = 2;
            state.focusedChoice = ChoiceId::Second;
            SetScene(state, Act1Scene::Settlement);
        }
        return true;
    }
    if (encounter == EncounterId::Vessel && state.settlementStage == 2) {
        if (!ApplyVesselContinuationClaim(
                state.claimLedger,
                state.narrative,
                state.committedChoice)) {
            return false;
        }
    } else if (!ApplySettlement(
            state.claimLedger,
            state.narrative,
            encounter,
            state.committedChoice)) {
        return false;
    }
    if (encounter == EncounterId::Patch || encounter == EncounterId::Pair) {
        state.narrative.role = 1;
    }
    state.narrative.encounterIndex = static_cast<std::uint8_t>(state.encounterIndex + 1);
    state.narrative.entityStatus[state.encounterIndex] =
        static_cast<std::uint8_t>(ChoiceIndex(state.committedChoice) + 1);
    state.movement.claimLedger = state.claimLedger;
    state.movement.narrative = state.narrative;
    return ClaimLedgerValid(state.claimLedger);
}

ChoiceId RequestedChoice(const InputFrame& input, ChoiceId current, bool vesselMinimum) noexcept {
    if (Pressed(input, PlatformAction::SelectLeft)) return ChoiceId::First;
    if (Pressed(input, PlatformAction::SelectRight)) {
        return vesselMinimum ? ChoiceId::Second : ChoiceId::Third;
    }
    if (Pressed(input, PlatformAction::SelectUp)) return ChoiceId::Second;
    return current;
}

void DrawCentered(PixelBuffer buffer, std::int32_t y, const char* text, PaletteColor color) noexcept {
    const std::int32_t width = MeasureLatin(text);
    DrawLatin(buffer, (kRenderWidth - width) / 2, y, text, color);
}

void DrawSplitLines(
    PixelBuffer buffer,
    std::int32_t y,
    const char* text,
    PaletteColor color) noexcept {
    char first[96]{};
    std::uint8_t count = 0;
    while (text[count] != 0 && text[count] != '|' && count < 95) {
        first[count] = text[count];
        ++count;
    }
    DrawCentered(buffer, y, first, color);
    if (text[count] == '|') {
        DrawCentered(buffer, y + 12, text + count + 1, color);
    }
}

void RenderIntro(PixelBuffer buffer, const Act1Runtime& state) noexcept {
    Clear(buffer, PaletteColor::Background);
    DrawLedgerGrid(buffer);
    StrokeRect(buffer, RectI{38, 34, 244, 100}, PaletteColor::LedgerGrid);
    const Act1EncounterDef& definition = GetAct1EncounterDef(state.encounterIndex);
    DrawLatin(buffer, 48, 42, "ACT 1", PaletteColor::UiInactive);
    DrawLatin(buffer, 48, 56, definition.name, PaletteColor::Opponent);
    DrawLatin(buffer, 48, 80, definition.introLine1, PaletteColor::Neutral);
    DrawLatin(buffer, 48, 92, definition.introLine2, PaletteColor::Neutral);
    DrawLatin(buffer, 48, 112, definition.objective, PaletteColor::Warning);
}

void RenderCombat(PixelBuffer buffer, const Act1Runtime& state) noexcept {
    Clear(buffer, PaletteColor::Background);
    DrawLedgerGrid(buffer);
    StrokeRect(buffer, RectI{11, 17, 298, 150}, PaletteColor::LedgerGrid);
    for (const CoverSlot& cover : state.combat.covers) {
        if (cover.active != 0) {
            StrokeRect(buffer, RectI{cover.bounds.x, cover.bounds.y, cover.bounds.width, cover.bounds.height}, PaletteColor::Cover);
        }
    }
    for (const NeutralSlot& neutral : state.combat.neutrals) {
        if (neutral.active != 0) {
            const std::int32_t x = neutral.position.x / kQ8One;
            const std::int32_t y = neutral.position.y / kQ8One;
            FillChecker(buffer, RectI{x - 2, y - 2, 4, 4}, PaletteColor::Neutral, static_cast<std::uint8_t>(neutral.stableId));
        }
    }
    for (const ProjectileSlot& shot : state.combat.projectiles) {
        if (shot.active != 0) {
            if (shot.widthPixels != 0 && shot.heightPixels != 0) {
                FillRect(buffer, RectI{
                    shot.position.x / kQ8One - shot.widthPixels / 2,
                    shot.position.y / kQ8One - shot.heightPixels / 2,
                    shot.widthPixels,
                    shot.heightPixels,
                }, shot.owner == CombatSide::Player ? PaletteColor::Player : PaletteColor::Opponent);
            } else {
                const std::int32_t radius = ProjectileRadiusPixels(shot.mass);
                FillRect(buffer, RectI{
                    shot.position.x / kQ8One - radius,
                    shot.position.y / kQ8One - radius,
                    radius * 2 + 1,
                    radius * 2 + 1,
                }, shot.owner == CombatSide::Player ? PaletteColor::Player : PaletteColor::Opponent);
            }
        }
    }
    if (state.encounterIndex == static_cast<std::uint8_t>(EncounterId::Pair)) {
        for (const PairProbe& probe : state.pair.probes) {
            if (probe.active != 0) {
                StrokeRect(buffer, RectI{probe.position.x / kQ8One - 3, probe.position.y / kQ8One - 3, 7, 7}, PaletteColor::Warning);
            }
        }
        if ((state.pair.coreAliveMask & 1u) != 0) FillRect(buffer, RectI{246, 68, 5, 5}, PaletteColor::Core);
        if ((state.pair.coreAliveMask & 2u) != 0) FillRect(buffer, RectI{246, 108, 5, 5}, PaletteColor::Core);
    } else {
        const CombatActor& opponent = state.combat.actors[SideIndex(CombatSide::Opponent)];
        const std::int32_t radius = ActorRadiusPixels(state.movement.physicalLedger.mass[AccountIndex(PhysicalAccount::OpponentShell)]);
        StrokeRect(buffer, RectI{opponent.position.x / kQ8One - radius, opponent.position.y / kQ8One - radius, radius * 2 + 1, radius * 2 + 1}, PaletteColor::Opponent);
        if (state.encounterIndex == static_cast<std::uint8_t>(EncounterId::Vessel)
            && state.vessel.action == VesselAction::Brace) {
            StrokeRect(buffer, RectI{
                opponent.position.x / kQ8One - radius - 2,
                opponent.position.y / kQ8One - radius - 2,
                radius * 2 + 5,
                radius * 2 + 5,
            }, PaletteColor::Warning);
        }
        if (state.encounterIndex == static_cast<std::uint8_t>(EncounterId::Vessel)
            && state.vessel.action == VesselAction::Stamp
            && state.vessel.stampTelegraph > 0) {
            const CombatActor& target = state.combat.actors[SideIndex(CombatSide::Player)];
            DrawLine(
                buffer,
                opponent.position.x / kQ8One,
                opponent.position.y / kQ8One,
                target.position.x / kQ8One,
                target.position.y / kQ8One,
                PaletteColor::Warning);
        }
        FillRect(buffer, RectI{opponent.position.x / kQ8One - 2, opponent.position.y / kQ8One - 2, 5, 5}, PaletteColor::Core);
    }
    const CombatActor& player = state.combat.actors[SideIndex(CombatSide::Player)];
    const std::int32_t playerRadius = ActorRadiusPixels(state.movement.physicalLedger.mass[AccountIndex(PhysicalAccount::PlayerShell)]);
    StrokeRect(buffer, RectI{player.position.x / kQ8One - playerRadius, player.position.y / kQ8One - playerRadius, playerRadius * 2 + 1, playerRadius * 2 + 1}, PaletteColor::Player);
    FillRect(buffer, RectI{player.position.x / kQ8One - 2, player.position.y / kQ8One - 2, 5, 5}, PaletteColor::Core);
    const Act1EncounterDef& definition = GetAct1EncounterDef(state.encounterIndex);
    DrawLatin(buffer, 2, 168, definition.objective, PaletteColor::Warning);
    DrawLatin(buffer, 246, 168, "FIRE / RECLAIM", PaletteColor::UiInactive);
}

void RenderSettlement(PixelBuffer buffer, const Act1Runtime& state) noexcept {
    RenderUiReference(buffer, UiReferenceScreen::Settlement);
    const EncounterId encounter = static_cast<EncounterId>(state.encounterIndex);
    if (encounter == EncounterId::Vessel && state.settlementStage == 1) {
        DrawLatin(buffer, 48, 44, "MINIMUM PERSISTENCE: MET", PaletteColor::Warning);
        DrawLatin(buffer, 52, 74, "REMAIN", PaletteColor::Neutral);
        DrawLatin(buffer, 52, 98, "CONTINUE", PaletteColor::Neutral);
    } else {
        DrawLatin(buffer, 48, 44, GetAct1EncounterDef(state.encounterIndex).name, PaletteColor::Opponent);
        for (std::uint8_t choice = 0; choice < 3; ++choice) {
            DrawLatin(buffer, 52, 74 + choice * 24, Act1ChoiceText(encounter, static_cast<ChoiceId>(choice)), PaletteColor::Neutral);
        }
    }
    const std::int32_t focusY = 68 + ChoiceIndex(state.focusedChoice) * 24;
    DrawLatin(buffer, 42, focusY + 5, "[", PaletteColor::Warning);
    DrawLatin(buffer, 270, focusY + 5, "]", PaletteColor::Warning);
}

}  // namespace

bool InitializeAct1Runtime(Act1Runtime& state) noexcept {
    state = Act1Runtime{};
    state.claimLedger = MakeInitialClaimLedger();
    state.scene = Act1Scene::Title;
    state.focusedChoice = ChoiceId::Second;
    state.committedChoice = ChoiceId::Second;
    return ClaimLedgerValid(state.claimLedger);
}

bool TickAct1Runtime(Act1Runtime& state, const InputFrame& input) noexcept {
    if (state.scene == Act1Scene::Fatal
        || !ClaimLedgerValid(state.claimLedger)) {
        return state.scene != Act1Scene::Fatal;
    }
    if (input.focused == 0 && state.scene != Act1Scene::Playing) return true;
    const bool playingAtEntry = state.scene == Act1Scene::Playing;
    switch (state.scene) {
    case Act1Scene::Title:
        if (AnyInputPressed(input)) SetScene(state, Act1Scene::BorrowAnimation);
        break;
    case Act1Scene::BorrowAnimation:
        if (++state.sceneTicks >= kTitleBorrowTicks) {
            if (!EnterEncounter(state)) {
                MarkFatal(state);
                return false;
            }
            SetScene(state, Act1Scene::LedgerFocus);
        }
        break;
    case Act1Scene::LedgerFocus:
        SetScene(state, Act1Scene::RowUnfold);
        break;
    case Act1Scene::RowUnfold:
        if (++state.sceneTicks >= kLedgerRowUnfoldTicks) SetScene(state, Act1Scene::EncounterIntro);
        break;
    case Act1Scene::EncounterIntro:
        ++state.sceneTicks;
        if (state.sceneTicks >= kEncounterIntroAutomaticTicks
            || (state.sceneTicks >= (state.introRevisit != 0
                    ? kEncounterRevisitIntroMinimumTicks
                    : kEncounterIntroMinimumTicks)
                && Pressed(input, PlatformAction::Confirm))) {
            SetScene(state, Act1Scene::ObjectiveCard);
        }
        break;
    case Act1Scene::ObjectiveCard:
        if (++state.sceneTicks >= kObjectiveCardTicks) SetScene(state, Act1Scene::Playing);
        break;
    case Act1Scene::Playing:
        if (!TickPlaying(state, input)) {
            MarkFatal(state);
            return false;
        }
        break;
    case Act1Scene::HitFreeze:
        if (++state.sceneTicks >= kHitFreezeTicks) {
            SetScene(state, state.retryPending != 0
                ? Act1Scene::EncounterRetry
                : Act1Scene::SettlementWait);
        }
        break;
    case Act1Scene::EncounterRetry:
        if (!EnterEncounter(state)) {
            MarkFatal(state);
            return false;
        }
        state.introRevisit = 1;
        SetScene(state, Act1Scene::EncounterIntro);
        break;
    case Act1Scene::SettlementWait:
        if (++state.sceneTicks >= kSettlementWaitTicks) {
            state.settlementStage = state.encounterIndex == static_cast<std::uint8_t>(EncounterId::Vessel) ? 1u : 0u;
            if (state.pair.forcedSelect != 0) state.focusedChoice = ChoiceId::First;
            SetScene(state, Act1Scene::Settlement);
        }
        break;
    case Act1Scene::Settlement: {
        const bool vesselMinimum = state.encounterIndex == static_cast<std::uint8_t>(EncounterId::Vessel)
            && state.settlementStage == 1;
        const ChoiceId requested = state.pair.forcedSelect != 0
            ? ChoiceId::First
            : RequestedChoice(input, state.focusedChoice, vesselMinimum);
        if (requested != state.focusedChoice) {
            state.focusedChoice = requested;
            SetScene(state, Act1Scene::Preview);
        } else if (Held(input, PlatformAction::Confirm)) {
            SetScene(state, Act1Scene::ConfirmHold);
        }
        break;
    }
    case Act1Scene::Preview:
        if (++state.sceneTicks >= kAllocationPreviewTicks) SetScene(state, Act1Scene::Settlement);
        break;
    case Act1Scene::ConfirmHold:
        if (!Held(input, PlatformAction::Confirm)) {
            SetScene(state, Act1Scene::Settlement);
        } else if (++state.sceneTicks >= kConfirmHoldTicks) {
            state.committedChoice = state.focusedChoice;
            SetScene(state, Act1Scene::CommitTransit);
        }
        break;
    case Act1Scene::CommitTransit:
        if (++state.sceneTicks >= kCommitTransitTicks) {
            if (!ApplyCommittedChoice(state)) {
                MarkFatal(state);
                return false;
            }
            if (state.scene == Act1Scene::CommitTransit) SetScene(state, Act1Scene::EntityResponse);
        }
        break;
    case Act1Scene::EntityResponse:
        ++state.sceneTicks;
        if (state.sceneTicks >= kEntityResponseAutomaticTicks
            || (state.sceneTicks >= kEntityResponseMinimumTicks && Pressed(input, PlatformAction::Confirm))) {
            SetScene(state, Act1Scene::LedgerReturn);
        }
        break;
    case Act1Scene::LedgerReturn:
        if (++state.sceneTicks >= kLedgerReturnTicks) {
            ++state.encounterIndex;
            state.initializedEncounter = 0;
            if (!EnterEncounter(state)) {
                MarkFatal(state);
                return false;
            }
            if (state.scene != Act1Scene::ContinueReady) SetScene(state, Act1Scene::LedgerFocus);
        }
        break;
    case Act1Scene::EnoughEnding:
    case Act1Scene::ContinueReady:
    case Act1Scene::Fatal:
        break;
    }
    if (!playingAtEntry) ++state.globalTick;
    return true;
}

void RenderAct1Runtime(PixelBuffer buffer, const Act1Runtime& state) noexcept {
    switch (state.scene) {
    case Act1Scene::Title:
    case Act1Scene::BorrowAnimation:
        RenderUiReference(buffer, UiReferenceScreen::Title);
        if (state.scene == Act1Scene::BorrowAnimation) {
            DrawCentered(buffer, 140, "INPUT DETECTED", PaletteColor::Warning);
        }
        break;
    case Act1Scene::LedgerFocus:
    case Act1Scene::RowUnfold:
    case Act1Scene::LedgerReturn:
        RenderUiReference(buffer, UiReferenceScreen::GlobalLedger);
        if (state.encounterIndex < kAct1EncounterCount) {
            DrawCentered(buffer, 156, GetAct1EncounterDef(state.encounterIndex).name, PaletteColor::Opponent);
        }
        break;
    case Act1Scene::EncounterIntro:
    case Act1Scene::ObjectiveCard:
        RenderIntro(buffer, state);
        break;
    case Act1Scene::Playing:
    case Act1Scene::HitFreeze:
    case Act1Scene::EncounterRetry:
    case Act1Scene::SettlementWait:
        RenderCombat(buffer, state);
        break;
    case Act1Scene::Settlement:
    case Act1Scene::Preview:
    case Act1Scene::ConfirmHold:
    case Act1Scene::CommitTransit:
        RenderSettlement(buffer, state);
        break;
    case Act1Scene::EntityResponse:
        Clear(buffer, PaletteColor::Background);
        DrawLedgerGrid(buffer);
        DrawSplitLines(
            buffer,
            76,
            Act1ResponseText(static_cast<EncounterId>(state.encounterIndex), state.committedChoice),
            PaletteColor::Neutral);
        break;
    case Act1Scene::EnoughEnding:
        RenderUiReference(buffer, UiReferenceScreen::EndingOpenLedger);
        DrawCentered(buffer, 42, "ENOUGH", PaletteColor::Warning);
        DrawCentered(buffer, 92, "PLAYER CLAIM: 144", PaletteColor::Player);
        break;
    case Act1Scene::ContinueReady:
        RenderUiReference(buffer, UiReferenceScreen::GlobalLedger);
        DrawCentered(buffer, 156, "ACT 1 COMPLETE", PaletteColor::Warning);
        break;
    case Act1Scene::Fatal:
        Clear(buffer, PaletteColor::Background);
        DrawCentered(buffer, 86, "LEDGER ERROR", PaletteColor::Warning);
        break;
    }
}

std::uint32_t Act1RuntimeHash(const Act1Runtime& state) noexcept {
    std::uint32_t hash = 2166136261u;
    const auto add = [&hash](std::uint32_t value) noexcept {
        for (std::uint8_t index = 0; index < 4; ++index) {
            hash ^= static_cast<std::uint8_t>(value >> (index * 8));
            hash *= 16777619u;
        }
    };
    const auto addVec = [&add](Vec2Q8 value) noexcept {
        add(static_cast<std::uint32_t>(value.x));
        add(static_cast<std::uint32_t>(value.y));
    };
    const auto addHandle = [&add](SlotHandle handle) noexcept {
        add(handle.index); add(handle.generation);
    };

    for (std::uint8_t index = 0; index < kClaimOwnerCount; ++index) add(state.claimLedger.mass[index]);
    add(state.narrative.choiceBits); add(state.narrative.adjudicatedMass);
    add(state.narrative.encounterIndex); add(state.narrative.role);
    for (std::uint8_t value : state.narrative.entityStatus) add(value);
    add(state.narrative.endingsSeen);
    add(GameStateHash(state.movement));

    for (const ChargeSlot& charge : state.combat.charges) {
        addVec(charge.position); addVec(charge.directionQ15);
        add(charge.stableId); add(charge.generation); add(charge.mass);
        add(charge.stageMass); add(charge.ageTicks); add(charge.active);
    }
    for (const ProjectileSlot& shot : state.combat.projectiles) {
        addVec(shot.position); addVec(shot.velocity);
        add(shot.stableId); add(shot.generation); add(shot.mass);
        add(shot.widthPixels); add(shot.heightPixels);
        add(static_cast<std::uint32_t>(shot.owner)); add(shot.active);
    }
    for (const NeutralSlot& neutral : state.combat.neutrals) {
        addVec(neutral.position); addVec(neutral.velocity);
        add(neutral.stableId); add(neutral.generation); add(neutral.mass);
        add(neutral.reclaimLockTicks); add(neutral.immunityTicks);
        add(neutral.playerMissMass); add(neutral.opponentMissMass);
        add(neutral.protocolTagMass); add(neutral.patchFeedMass);
        add(neutral.adapterEscrowMass); add(neutral.structureMass); add(neutral.unscoredMass);
        add(neutral.inertiaTicks); add(neutral.active);
    }
    for (const CoverSlot& cover : state.combat.covers) {
        add(static_cast<std::uint32_t>(cover.bounds.x));
        add(static_cast<std::uint32_t>(cover.bounds.y));
        add(static_cast<std::uint32_t>(cover.bounds.width));
        add(static_cast<std::uint32_t>(cover.bounds.height));
        add(cover.stableId); add(cover.generation); add(cover.mass);
        add(static_cast<std::uint32_t>(cover.backingAccount));
        add(static_cast<std::uint32_t>(cover.behavior)); add(cover.active);
    }
    for (const CombatActor& actor : state.combat.actors) {
        addVec(actor.position); addVec(actor.velocity); addVec(actor.aimDirectionQ15);
        add(actor.exposureGraceTicks); add(actor.shotCooldownTicks);
        add(actor.projectileImmune); add(actor.coreHit);
    }
    for (std::uint8_t index = 0; index < kPhysicalAccountCount; ++index) {
        add(state.combat.settlement.physicalLedger.mass[index]);
    }
    for (std::uint8_t index = 0; index < kClaimOwnerCount; ++index) {
        add(state.combat.settlement.claimLedger.mass[index]);
    }
    add(state.combat.settlement.tick);
    add(static_cast<std::uint32_t>(state.combat.settlement.hitSide));
    add(state.combat.settlement.valid);
    add(state.combat.shellDamageThisTick[0]);
    add(state.combat.shellDamageThisTick[1]);
    add(state.combat.nextStableId); add(state.combat.globalTick);
    add(static_cast<std::uint32_t>(state.combat.phase));
    add(static_cast<std::uint32_t>(state.combat.error));

    for (std::uint16_t value : state.reclaim.accumulator) add(value);
    for (std::uint16_t value : state.reclaim.targetIndex) add(value);
    for (const HudModuleLoan& module : state.hudLoan.modules) {
        add(module.mass); add(module.channelTicks); add(module.stateTicks);
        add(module.maturityRemaining); add(module.maturityAdvanceTicks);
        add(module.transitMass); add(module.arrivedMass);
        add(static_cast<std::uint32_t>(module.state)); add(module.warning);
    }
    add(static_cast<std::uint32_t>(state.hudLoan.selected));
    add(static_cast<std::uint32_t>(state.hudLoan.pendingModule));
    add(state.hudLoan.reallocationLockTicks); add(state.hudLoan.pendingCommit);

    for (SlotHandle handle : state.neutralHandles) addHandle(handle);
    for (SlotHandle handle : state.patch.feed) addHandle(handle);
    add(state.patch.shellBaseline); add(state.patch.transitTicks);
    add(state.patch.transitNeutralIndex); add(state.patch.feedsUsed);
    add(state.patch.transitActive); add(state.patch.pendingReservation);
    for (const PairProbe& probe : state.pair.probes) {
        addVec(probe.position); addVec(probe.directionQ15); addHandle(probe.protocolNeutral);
        add(probe.respawnTicks); add(probe.targetCore); add(probe.active);
    }
    for (std::uint16_t value : state.pair.coreX) add(value);
    for (std::uint16_t value : state.pair.coreY) add(value);
    add(state.pair.coreAliveMask); add(state.pair.forcedSelect);
    add(static_cast<std::uint32_t>(state.vessel.action));
    addVec(state.vessel.pressDirectionQ15); add(state.vessel.actionTicks);
    add(state.vessel.pressCooldown); add(state.vessel.braceCooldown);
    add(state.vessel.stampCooldown); add(state.vessel.stampTelegraph);
    add(state.vessel.reclaimAccumulator);

    add(static_cast<std::uint32_t>(state.scene));
    add(static_cast<std::uint32_t>(state.focusedChoice));
    add(static_cast<std::uint32_t>(state.committedChoice));
    add(state.globalTick); add(state.encounterTick); add(state.sceneTicks);
    add(state.previousPlayerReclaim); add(state.encounterIndex);
    add(state.settlementStage); add(state.initializedEncounter); add(state.failure);
    add(state.retryPending); add(state.introRevisit);
    return hash;
}

#if defined(BP_TEST)
bool ForceAct1EncounterSuccessForTest(Act1Runtime& state) noexcept {
    if (state.scene != Act1Scene::Playing || state.encounterIndex >= kAct1EncounterCount) {
        return false;
    }
    if (state.encounterIndex == static_cast<std::uint8_t>(EncounterId::Patch)) {
        state.patch.feedsUsed = kPatchSuccessFeedCount;
    } else if (state.encounterIndex == static_cast<std::uint8_t>(EncounterId::Pair)) {
        state.encounterTick = kPairDurationTicks;
    } else {
        state.combat.actors[SideIndex(CombatSide::Opponent)].coreHit = 1;
    }
    BeginSuccess(state);
    return true;
}
#endif

}  // namespace bp
