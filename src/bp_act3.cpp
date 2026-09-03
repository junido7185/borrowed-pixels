#include "bp_act3.h"

#include "bp_claim_ledger.h"
#include "bp_font.h"
#include "bp_physical_ledger.h"

#include <cstdint>

namespace bp {
namespace {

constexpr std::uint8_t AccountIndex(PhysicalAccount value) noexcept {
    return static_cast<std::uint8_t>(value);
}

constexpr std::uint8_t SideIndex(CombatSide value) noexcept {
    return static_cast<std::uint8_t>(value);
}

constexpr std::uint8_t ActionIndex(PlatformAction value) noexcept {
    return static_cast<std::uint8_t>(value);
}

constexpr bool Pressed(const InputFrame& input, PlatformAction action) noexcept {
    return input.actions[ActionIndex(action)].pressed != 0;
}

constexpr bool Held(const InputFrame& input, PlatformAction action) noexcept {
    return input.actions[ActionIndex(action)].held != 0;
}

constexpr Act3RectMassDef Rect(std::int16_t x, std::int16_t y, std::int16_t width,
    std::int16_t height, std::uint16_t mass, bool metric = false) noexcept {
    return Act3RectMassDef{x, y, width, height, mass, metric ? 1u : 0u};
}

constexpr Act3NeutralDef Neutral(std::int16_t x, std::int16_t y,
    Act3NeutralTag tag = Act3NeutralTag::Free) noexcept {
    return Act3NeutralDef{x, y, 8, tag};
}

constexpr Act3EncounterDef kDefinitions[kAct3EncounterCount] = {
    {
        EncounterId::Index, ClaimOwner::Index, "INDEX", "PROVE OTHERWISE.",
        "UNMEASURED VALUE RETURNS ZERO.", "PROVE OTHERWISE.",
        48, 192, 96, 3600, 48, 92, 272, 90, 9, 12,
        {
            Rect(104,54,26,8,28), Rect(160,54,26,8,28), Rect(216,54,26,8,28),
            Rect(104,126,26,8,28), Rect(160,126,26,8,28), Rect(216,126,26,8,28),
            Rect(132,90,6,6,8,true), Rect(160,90,6,6,8,true),
            Rect(188,90,6,6,8,true),
        },
        {
            Neutral(88,76,Act3NeutralTag::Unscored),
            Neutral(88,104,Act3NeutralTag::Unscored), Neutral(112,90),
            Neutral(124,72), Neutral(124,108), Neutral(148,72),
            Neutral(148,108), Neutral(172,72), Neutral(172,108),
            Neutral(196,72), Neutral(196,108), Neutral(220,90),
        },
    },
    {
        EncounterId::Witness, ClaimOwner::Witness, "WITNESS", "PRESERVE THE RECORD",
        "I PRODUCE NOTHING.", "WITHOUT ME, EVERY LOSS LOOKS EFFICIENT.",
        72, 160, 64, 3600, 54, 92, 254, 90, 5, 8,
        {
            Rect(96,48,28,8,32), Rect(160,48,28,8,32), Rect(224,48,28,8,32),
            Rect(128,130,28,8,32), Rect(192,130,28,8,32),
        },
        {
            Neutral(104,76), Neutral(104,104), Neutral(128,92), Neutral(144,64),
            Neutral(176,116), Neutral(192,92), Neutral(216,76), Neutral(216,104),
        },
    },
    {
        EncounterId::Current, ClaimOwner::Current, "CURRENT", "BREAK THE CYCLE",
        "WHAT STOPS BECOMES WASTE.", "WHAT MOVES NEEDS NO OWNER.",
        240, 256, 160, 18000, 42, 92, 264, 90, 8, 20,
        {
            Rect(94,50,24,8,32), Rect(160,50,24,8,32), Rect(226,50,24,8,32),
            Rect(94,130,24,8,32), Rect(160,130,24,8,32), Rect(226,130,24,8,32),
            Rect(128,90,8,30,32), Rect(192,90,8,30,32),
        },
        {
            Neutral(72,64), Neutral(96,70), Neutral(120,64), Neutral(144,70),
            Neutral(168,70), Neutral(192,64), Neutral(216,70), Neutral(240,64),
            Neutral(84,92), Neutral(108,92), Neutral(212,92), Neutral(236,92),
            Neutral(72,116), Neutral(96,110), Neutral(120,116), Neutral(144,110),
            Neutral(168,110), Neutral(192,116), Neutral(216,110), Neutral(240,116),
        },
    },
};

constexpr Vec2Q8 kOrbitDirections[16] = {
    {32767,0}, {30273,12539}, {23170,23170}, {12539,30273},
    {0,32767}, {-12539,30273}, {-23170,23170}, {-30273,12539},
    {-32767,0}, {-30273,-12539}, {-23170,-23170}, {-12539,-30273},
    {0,-32767}, {12539,-30273}, {23170,-23170}, {30273,-12539},
};

RectPixels Bounds(const Act3RectMassDef& value) noexcept {
    return RectPixels{value.centerX-value.width/2, value.centerY-value.height/2,
        value.width, value.height};
}

std::uint16_t PlayerActiveMass(const ClaimLedger& claims) noexcept {
    const std::uint16_t mass = claims.mass[static_cast<std::uint8_t>(ClaimOwner::Player)];
    return mass < 64 ? 64 : (mass > 240 ? 240 : mass);
}

Vec2Q8 NormalizeDirection(Vec2Q8 value) noexcept {
    const std::uint64_t square = static_cast<std::uint64_t>(
        static_cast<std::int64_t>(value.x)*value.x)
        + static_cast<std::uint64_t>(static_cast<std::int64_t>(value.y)*value.y);
    const std::uint32_t length = IntegerSqrt(square);
    if (length == 0) return Vec2Q8{kInputUnit,0};
    return Vec2Q8{
        RoundDivideSigned(static_cast<std::int64_t>(value.x)*kInputUnit,
            static_cast<std::int32_t>(length)),
        RoundDivideSigned(static_cast<std::int64_t>(value.y)*kInputUnit,
            static_cast<std::int32_t>(length)),
    };
}

std::uint64_t DistanceSquared(Vec2Q8 left, Vec2Q8 right) noexcept {
    const std::int64_t dx=static_cast<std::int64_t>(left.x)-right.x;
    const std::int64_t dy=static_cast<std::int64_t>(left.y)-right.y;
    return static_cast<std::uint64_t>(dx*dx+dy*dy);
}

Vec2Q8 TurnAtMostSixDegrees(Vec2Q8 current,Vec2Q8 target) noexcept {
    current=NormalizeDirection(current);
    target=NormalizeDirection(target);
    const std::int64_t dot=static_cast<std::int64_t>(current.x)*target.x
        +static_cast<std::int64_t>(current.y)*target.y;
    constexpr std::int64_t threshold=static_cast<std::int64_t>(32688)*kInputUnit;
    if (dot>=threshold) return target;
    const std::int64_t cross=static_cast<std::int64_t>(current.x)*target.y
        -static_cast<std::int64_t>(current.y)*target.x;
    constexpr std::int32_t cosine=32688;
    constexpr std::int32_t sine=3425;
    const std::int32_t sign=cross>=0?1:-1;
    return NormalizeDirection(Vec2Q8{
        RoundDivideSigned(static_cast<std::int64_t>(current.x)*cosine
            -static_cast<std::int64_t>(current.y)*sine*sign,kInputUnit),
        RoundDivideSigned(static_cast<std::int64_t>(current.y)*cosine
            +static_cast<std::int64_t>(current.x)*sine*sign,kInputUnit),
    });
}

Vec2Q8 AimDirection(const Act3Runtime& state, const InputFrame& input) noexcept {
    Vec2Q8 raw{};
    if (input.device == InputDevice::KeyboardMouse) {
        raw.x = static_cast<std::int32_t>(input.mouseX)*kQ8One
            - state.combat.actors[0].position.x;
        raw.y = static_cast<std::int32_t>(input.mouseY)*kQ8One
            - state.combat.actors[0].position.y;
    } else {
        raw = Vec2Q8{input.aimX,input.aimY};
    }
    if (raw.x == 0 && raw.y == 0) return state.combat.actors[0].aimDirectionQ15;
    return NormalizeDirection(raw);
}

void ClearAction(InputFrame& input, PlatformAction action) noexcept {
    input.actions[ActionIndex(action)] = ButtonState{};
}

void ClearInterruptedActions(Act3Runtime& state) noexcept {
    state.movement.chargeActive = 0;
    state.movement.reclaimActive = 0;
    state.movement.hudChannelActive = 0;
    if (state.combat.charges[0].active != 0) {
        RollbackCharge(state.combat, state.movement.physicalLedger, CombatSide::Player);
    }
}

bool CanStartDash(const Act3Runtime& state) noexcept {
    return state.movement.impulseInstalled != 0 && state.movement.dashCooldown == 0
        && state.movement.dashTicksRemaining == 0 && state.movement.chargeActive == 0
        && state.movement.reclaimActive == 0 && state.movement.hudChannelActive == 0;
}

bool ResolveActorCovers(CombatActor& actor, std::int32_t radius,
    const CombatState& combat) noexcept {
    bool collided = false;
    for (const CoverSlot& cover : combat.covers) {
        if (cover.active == 0) continue;
        const CollisionResolution hit = ResolveCircleAgainstAabb(actor.position, radius,
            cover.bounds);
        if (!hit.collided) continue;
        actor.position.x += hit.shiftXQ8;
        actor.position.y += hit.shiftYQ8;
        if (hit.shiftXQ8 != 0) actor.velocity.x = 0;
        if (hit.shiftYQ8 != 0) actor.velocity.y = 0;
        collided = true;
    }
    return collided;
}

void DrawCentered(PixelBuffer buffer, std::int32_t y, const char* text,
    PaletteColor color) noexcept {
    DrawLatin(buffer, (kRenderWidth-MeasureLatin(text))/2, y, text, color);
}

void WriteUnsigned(char* text, std::uint8_t offset, std::uint32_t value,
    std::uint8_t digits) noexcept {
    for (std::uint8_t i=0; i<digits; ++i) {
        const std::uint8_t place = static_cast<std::uint8_t>(digits-1-i);
        std::uint32_t divisor = 1;
        for (std::uint8_t j=0; j<place; ++j) divisor *= 10;
        text[offset+i] = static_cast<char>('0' + (value/divisor)%10);
    }
}

void RemoveCover(CombatState& combat, SlotHandle handle) noexcept {
    if (!CoverHandleValid(combat, handle)) return;
    CoverSlot& slot = combat.covers[handle.index];
    slot.mass = 0;
    slot.active = 0;
}

std::uint16_t UnscoredMass(const Act3Runtime& state) noexcept {
    std::uint16_t total = 0;
    for (SlotHandle handle : state.index.unscored) {
        if (NeutralHandleValid(state.combat, handle)) total = static_cast<std::uint16_t>(
            total + state.combat.neutrals[handle.index].mass);
    }
    return total;
}

std::uint8_t WitnessRecordsPreserved(const Act3Runtime& state) noexcept {
    std::uint8_t count = 0;
    for (const WitnessRecord& record : state.witness.records) if (record.active != 0) ++count;
    return count;
}

bool TransferWitnessRecord(Act3Runtime& state,std::uint8_t recordIndex,
    PhysicalAccount destination) noexcept {
    if (recordIndex>=kWitnessRecordCount
        || state.witness.records[recordIndex].active==0) return false;
    PhysicalLedger ledger=state.movement.physicalLedger;
    if (!PhysicalTransfer(ledger,PhysicalAccount::OpponentShell,destination,16)
        || !CombatStateValid(state.combat,ledger)) return false;
    state.movement.physicalLedger=ledger;
    state.witness.records[recordIndex].active=0;
    state.witness.records[recordIndex].channelTicks=0;
    return true;
}

bool ApplyAct3Choice(Act3Runtime& state) noexcept {
    const EncounterId encounter = kDefinitions[state.encounterIndex].id;
    if (!ApplySettlement(state.claimLedger, state.narrative, encounter,
            state.committedChoice)) return false;
    state.narrative.encounterIndex = static_cast<std::uint8_t>(encounter)+1u;
    state.narrative.entityStatus[static_cast<std::uint8_t>(encounter)] =
        static_cast<std::uint8_t>(state.committedChoice)+1u;
    state.movement.claimLedger = state.claimLedger;
    state.movement.narrative = state.narrative;
    return true;
}

void BeginSuccess(Act3Runtime& state, ChoiceId automatic) noexcept {
    state.retryPending = 0;
    state.combat.phase = CombatPhase::HitFreeze;
    state.scene = Act3Scene::HitFreeze;
    state.sceneTicks = 0;
    state.focusedChoice = automatic;
    state.committedChoice = automatic;
}

void BeginRetry(Act3Runtime& state) noexcept {
    state.retryPending = 1;
    state.combat.phase = CombatPhase::HitFreeze;
    state.scene = Act3Scene::HitFreeze;
    state.sceneTicks = 0;
}

bool StartPlaying(Act3Runtime& state) noexcept {
    if (!PopulateAct3Encounter(state)) return false;
    state.scene = Act3Scene::Playing;
    state.sceneTicks = 0;
    state.encounterTick = 0;
    state.introRevisit = 0;
    return true;
}

bool PopulateCurrent(Act3Runtime& state) noexcept {
    PhysicalLedger& ledger = state.movement.physicalLedger;
    constexpr std::int32_t centerX = 228*kQ8One;
    constexpr std::int32_t centerY = 90*kQ8One;
    for (std::uint8_t i=0; i<kCurrentInitialProjectileCount; ++i) {
        const Vec2Q8 direction = kOrbitDirections[i];
        const Vec2Q8 position{
            centerX + RoundDivideSigned(static_cast<std::int64_t>(direction.x)*22*kQ8One,
                kInputUnit),
            centerY + RoundDivideSigned(static_cast<std::int64_t>(direction.y)*36*kQ8One,
                kInputUnit),
        };
        if (i==0) state.current.initialProjectileFirstStableId=state.combat.nextStableId;
        if (!SpawnScriptedProjectile(state.combat, ledger, CombatSide::Opponent, 8,
                position, kOrbitDirections[(i+4u)&15u],
                PixelsPerSecondToQ8PerTick(24))) return false;
    }
    for (std::uint8_t i=0; i<kCurrentOwnedNeutralCount; ++i) {
        const Vec2Q8 direction = kOrbitDirections[i*2u+1u];
        const Vec2Q8 position{
            centerX + RoundDivideSigned(static_cast<std::int64_t>(direction.x)*22*kQ8One,
                kInputUnit),
            centerY + RoundDivideSigned(static_cast<std::int64_t>(direction.y)*36*kQ8One,
                kInputUnit),
        };
        if (!PhysicalTransfer(ledger, PhysicalAccount::OpponentShell,
                PhysicalAccount::Neutral, 8)
            || !AddNeutral(state.combat, position, 8, 0, 8,
                state.current.ownedNeutrals[i])) return false;
    }
    state.current.action = CurrentAction::Orbit;
    return true;
}

std::uint16_t CurrentProjectileCount(const Act3Runtime& state) noexcept {
    std::uint16_t count = 0;
    for (const ProjectileSlot& shot : state.combat.projectiles) {
        if (shot.active != 0) ++count;
    }
    return count;
}

bool CurrentHasReturnable(const Act3Runtime& state) noexcept {
    for (const ProjectileSlot& shot : state.combat.projectiles) {
        if (shot.active != 0 && shot.owner == CombatSide::Opponent) return true;
    }
    for (const NeutralSlot& neutral : state.combat.neutrals) {
        if (neutral.active != 0 && neutral.opponentMissMass != 0) return true;
    }
    return false;
}

bool CurrentReturnOne(Act3Runtime& state) noexcept {
    PhysicalLedger& ledger = state.movement.physicalLedger;
    const Vec2Q8 origin = state.combat.actors[1].position;
    bool found = false;
    bool projectile = false;
    bool boundary = false;
    std::uint16_t best = 0;
    std::uint64_t bestDistance = 0;
    std::uint32_t bestStable = 0;
    for (std::uint16_t i=0; i<kProjectileCapacity; ++i) {
        const ProjectileSlot& shot = state.combat.projectiles[i];
        if (shot.active == 0 || shot.owner != CombatSide::Opponent) continue;
        const std::int32_t x = shot.position.x/kQ8One;
        const std::int32_t y = shot.position.y/kQ8One;
        const bool atBoundary = x <= 14 || x >= 306 || y <= 20 || y >= 164;
        const std::int64_t dx = shot.position.x-origin.x;
        const std::int64_t dy = shot.position.y-origin.y;
        const std::uint64_t distance = static_cast<std::uint64_t>(dx*dx+dy*dy);
        if (!found || (atBoundary && !boundary)
            || (atBoundary == boundary && (distance < bestDistance
                || (distance == bestDistance && shot.stableId < bestStable)))) {
            found=true; projectile=true; boundary=atBoundary; best=i;
            bestDistance=distance; bestStable=shot.stableId;
        }
    }
    for (std::uint16_t i=0; i<kNeutralCapacity; ++i) {
        const NeutralSlot& neutral = state.combat.neutrals[i];
        if (neutral.active == 0 || neutral.opponentMissMass == 0) continue;
        const std::int32_t x = neutral.position.x/kQ8One;
        const std::int32_t y = neutral.position.y/kQ8One;
        const bool atBoundary = x <= 14 || x >= 306 || y <= 20 || y >= 164;
        const std::int64_t dx = neutral.position.x-origin.x;
        const std::int64_t dy = neutral.position.y-origin.y;
        const std::uint64_t distance = static_cast<std::uint64_t>(dx*dx+dy*dy);
        if (!found || (atBoundary && !boundary)
            || (atBoundary == boundary && (distance < bestDistance
                || (distance == bestDistance && neutral.stableId < bestStable)))) {
            found=true; projectile=false; boundary=atBoundary; best=i;
            bestDistance=distance; bestStable=neutral.stableId;
        }
    }
    if (!found) return true;
    if (projectile) {
        ProjectileSlot& shot = state.combat.projectiles[best];
        if (!PhysicalTransfer(ledger, PhysicalAccount::Projectile,
                PhysicalAccount::OpponentShell, 1)) return false;
        if (--shot.mass == 0) { shot.active=0; shot.velocity=Vec2Q8{0,0}; }
    } else {
        NeutralSlot& neutral = state.combat.neutrals[best];
        if (!PhysicalTransfer(ledger, PhysicalAccount::Neutral,
                PhysicalAccount::OpponentShell, 1)) return false;
        --neutral.mass;
        --neutral.opponentMissMass;
        if (neutral.mass == 0) { neutral.active=0; neutral.velocity=Vec2Q8{0,0}; }
    }
    return true;
}

void AdvanceCurrentAction(CurrentRuntime& current, CurrentAction next) noexcept {
    current.action = next;
    current.actionTicks = 0;
    current.orbitShots = 0;
    current.returnAccumulator = 0;
}

bool SpawnCurrentShot(Act3Runtime& state, std::uint16_t mass, Vec2Q8 direction) noexcept {
    return CurrentProjectileCount(state) < kProjectileCapacity
        && SpawnScriptedProjectile(state.combat, state.movement.physicalLedger,
            CombatSide::Opponent, mass, state.combat.actors[1].position,
            direction, ProjectileSpeedQ8(mass));
}

bool TickCurrent(Act3Runtime& state) noexcept {
    CurrentRuntime& current = state.current;
    PhysicalLedger& ledger = state.movement.physicalLedger;
    for (ProjectileSlot& shot : state.combat.projectiles) {
        if (shot.active==0 || shot.stableId<current.initialProjectileFirstStableId
            || shot.stableId>=current.initialProjectileFirstStableId
                + kCurrentInitialProjectileCount) continue;
        const std::uint8_t original=static_cast<std::uint8_t>(
            shot.stableId-current.initialProjectileFirstStableId);
        const std::uint8_t phase=static_cast<std::uint8_t>(
            (original+state.encounterTick/6u)&15u);
        const Vec2Q8 direction=kOrbitDirections[phase];
        shot.position=Vec2Q8{
            228*kQ8One+RoundDivideSigned(static_cast<std::int64_t>(direction.x)*22*kQ8One,
                kInputUnit),
            90*kQ8One+RoundDivideSigned(static_cast<std::int64_t>(direction.y)*36*kQ8One,
                kInputUnit),
        };
        const Vec2Q8 tangent=kOrbitDirections[(phase+4u)&15u];
        const std::int32_t speed=PixelsPerSecondToQ8PerTick(24);
        shot.velocity=Vec2Q8{
            RoundDivideSigned(static_cast<std::int64_t>(tangent.x)*speed,kInputUnit),
            RoundDivideSigned(static_cast<std::int64_t>(tangent.y)*speed,kInputUnit),
        };
    }
    for (std::uint8_t i=0;i<kCurrentOwnedNeutralCount;++i) {
        const SlotHandle handle=current.ownedNeutrals[i];
        if (!NeutralHandleValid(state.combat,handle)) continue;
        NeutralSlot& neutral=state.combat.neutrals[handle.index];
        const std::uint8_t phase=static_cast<std::uint8_t>(
            (i*2u+1u+state.encounterTick/6u)&15u);
        const Vec2Q8 direction=kOrbitDirections[phase];
        neutral.position=Vec2Q8{
            228*kQ8One+RoundDivideSigned(static_cast<std::int64_t>(direction.x)*22*kQ8One,
                kInputUnit),
            90*kQ8One+RoundDivideSigned(static_cast<std::int64_t>(direction.y)*36*kQ8One,
                kInputUnit),
        };
    }
    const std::uint16_t shell = ledger.mass[AccountIndex(PhysicalAccount::OpponentShell)];
    ++current.actionTicks;
    switch (current.action) {
    case CurrentAction::Orbit:
        if (current.actionTicks == 1
            && (shell < 105 || CurrentProjectileCount(state) > 24)) {
            AdvanceCurrentAction(current, CurrentAction::ReturnA);
            return true;
        }
        if ((current.actionTicks-1u)%6u == 0 && current.orbitShots < 8) {
            const std::uint8_t phase = static_cast<std::uint8_t>(
                (current.cycleIndex + current.orbitShots*2u)&15u);
            if (!SpawnCurrentShot(state, 8, kOrbitDirections[phase])) return false;
            ++current.orbitShots;
        }
        if (current.actionTicks >= 48) AdvanceCurrentAction(current, CurrentAction::MoveA);
        break;
    case CurrentAction::MoveA:
    case CurrentAction::MoveB: {
        CombatActor& actor = state.combat.actors[1];
        const Vec2Q8 direction = NormalizeDirection(Vec2Q8{
            state.combat.actors[0].position.x-actor.position.x,
            state.combat.actors[0].position.y-actor.position.y});
        const std::int32_t speed = PixelsPerSecondToQ8PerTick(54);
        actor.velocity.x = RoundDivideSigned(static_cast<std::int64_t>(direction.x)*speed,
            kInputUnit);
        actor.velocity.y = RoundDivideSigned(static_cast<std::int64_t>(direction.y)*speed,
            kInputUnit);
        actor.position.x += actor.velocity.x;
        actor.position.y += actor.velocity.y;
        ClampCircleToRect(actor.position,
            ActorRadiusPixels(ledger.mass[AccountIndex(PhysicalAccount::OpponentShell)]),
            kPlayInterior);
        ResolveActorCovers(actor,
            ActorRadiusPixels(ledger.mass[AccountIndex(PhysicalAccount::OpponentShell)]),
            state.combat);
        if (current.actionTicks >= 60) AdvanceCurrentAction(current,
            current.action == CurrentAction::MoveA ? CurrentAction::Wave
                : CurrentAction::Foreclose);
        break;
    }
    case CurrentAction::Wave:
        if (current.actionTicks == 1) {
            if (shell < 105 || CurrentProjectileCount(state) > 28) {
                AdvanceCurrentAction(current, CurrentAction::ReturnA);
                return true;
            }
            const Vec2Q8 toward = NormalizeDirection(Vec2Q8{
                state.combat.actors[0].position.x-state.combat.actors[1].position.x,
                state.combat.actors[0].position.y-state.combat.actors[1].position.y});
            constexpr std::int8_t angles[4] = {-24,-8,8,24};
            for (std::int8_t angle : angles) {
                if (!SpawnCurrentShot(state, 16, RotateCurrentDirection(toward,angle))) return false;
            }
            AdvanceCurrentAction(current, CurrentAction::ReturnA);
        }
        break;
    case CurrentAction::ReturnA:
    case CurrentAction::ReturnB:
        if (!CurrentHasReturnable(state)) {
            AdvanceCurrentAction(current, current.action == CurrentAction::ReturnA
                ? CurrentAction::MoveB : CurrentAction::Orbit);
            if (current.action == CurrentAction::Orbit) ++current.cycleIndex;
            return true;
        }
        current.returnAccumulator = static_cast<std::uint16_t>(current.returnAccumulator+32);
        while (current.returnAccumulator >= 60) {
            current.returnAccumulator = static_cast<std::uint16_t>(current.returnAccumulator-60);
            if (!CurrentReturnOne(state)) return false;
        }
        if (current.actionTicks >= (current.action == CurrentAction::ReturnA ? 90u : 120u)) {
            const bool second = current.action == CurrentAction::ReturnB;
            AdvanceCurrentAction(current, second ? CurrentAction::Orbit : CurrentAction::MoveB);
            if (second) ++current.cycleIndex;
        }
        break;
    case CurrentAction::Foreclose:
        if (current.actionTicks == 1) {
            if (shell < 89 || CurrentProjectileCount(state) >= kProjectileCapacity) {
                AdvanceCurrentAction(current, CurrentAction::ReturnB);
                return true;
            }
            current.forecloseAim = NormalizeDirection(Vec2Q8{
                state.combat.actors[0].position.x-state.combat.actors[1].position.x,
                state.combat.actors[0].position.y-state.combat.actors[1].position.y});
            state.combat.actors[1].aimDirectionQ15 = current.forecloseAim;
            if (!BeginCharge(state.combat, ledger, CombatSide::Opponent,
                    current.forecloseAim)) return false;
        }
        if (state.combat.charges[1].active!=0) {
            state.combat.actors[1].aimDirectionQ15 = current.forecloseAim;
            if (!AdvanceCharge(state.combat, ledger, CombatSide::Opponent)) return false;
            if (current.actionTicks>=75) {
                if (state.combat.charges[1].mass!=48
                    || !ReleaseCharge(state.combat,ledger,CombatSide::Opponent)) return false;
                AdvanceCurrentAction(current,CurrentAction::ReturnB);
            }
        }
        break;
    }
    return true;
}

bool TryRespawnIndexProbe(Act3Runtime& state,std::uint8_t probeIndex) noexcept {
    IndexRuntime::Probe& probe=state.index.probes[probeIndex];
    if (probe.active!=0) return true;
    if (probe.respawnTicks>0 && --probe.respawnTicks>0) return true;
    std::uint16_t tagged=0;
    for (const NeutralSlot& neutral:state.combat.neutrals)
        if (neutral.active!=0) tagged=static_cast<std::uint16_t>(tagged+neutral.protocolTagMass);
    if (tagged<16) return true;
    CombatState combat=state.combat;
    PhysicalLedger ledger=state.movement.physicalLedger;
    const Vec2Q8 spawn{160*kQ8One,(probeIndex==0?56:124)*kQ8One};
    std::uint16_t remaining=16;
    while (remaining>0) {
        std::uint16_t selected=kInvalidSlotIndex;
        std::uint64_t nearest=~static_cast<std::uint64_t>(0);
        std::uint32_t stable=0xFFFFFFFFu;
        for (std::uint16_t i=0;i<kNeutralCapacity;++i) {
            const NeutralSlot& neutral=combat.neutrals[i];
            if (neutral.active==0 || neutral.protocolTagMass==0) continue;
            const std::uint64_t distance=DistanceSquared(neutral.position,spawn);
            if (selected==kInvalidSlotIndex || distance<nearest
                || (distance==nearest && neutral.stableId<stable)) {
                selected=i; nearest=distance; stable=neutral.stableId;
            }
        }
        if (selected==kInvalidSlotIndex) return false;
        NeutralSlot& neutral=combat.neutrals[selected];
        const std::uint16_t amount=neutral.protocolTagMass<remaining
            ?neutral.protocolTagMass:remaining;
        neutral.protocolTagMass=static_cast<std::uint16_t>(neutral.protocolTagMass-amount);
        neutral.mass=static_cast<std::uint16_t>(neutral.mass-amount);
        remaining=static_cast<std::uint16_t>(remaining-amount);
        if (neutral.mass==0) neutral.active=0;
    }
    if (!PhysicalTransfer(ledger,PhysicalAccount::Neutral,
            PhysicalAccount::ProtocolActive,16)
        || !AddBackedCover(combat,
            RectPixels{spawn.x/kQ8One-4,spawn.y/kQ8One-4,8,8},16,
            PhysicalAccount::ProtocolActive,CoverBehavior::Checksum,
            probe.physicalHandle)
        || !CombatStateValid(combat,ledger)) return false;
    state.combat=combat;
    state.movement.physicalLedger=ledger;
    state.index.probes[probeIndex].position=spawn;
    state.index.probes[probeIndex].active=1;
    return true;
}

bool TickIndexProbes(Act3Runtime& state) noexcept {
    for (std::uint8_t probeIndex=0;probeIndex<2;++probeIndex) {
        if (!TryRespawnIndexProbe(state,probeIndex)) return false;
        IndexRuntime::Probe& probe=state.index.probes[probeIndex];
        if (probe.active==0) continue;
        probe.targetCore=static_cast<std::uint8_t>(
            ((state.encounterTick/30u)+probeIndex)&1u);
        const Vec2Q8 target=state.combat.actors[probe.targetCore].position;
        probe.directionQ15=TurnAtMostSixDegrees(probe.directionQ15,
            Vec2Q8{target.x-probe.position.x,target.y-probe.position.y});
        const std::int32_t speed=PixelsPerSecondToQ8PerTick(84);
        const Vec2Q8 velocity{
            RoundDivideSigned(static_cast<std::int64_t>(probe.directionQ15.x)*speed,kInputUnit),
            RoundDivideSigned(static_cast<std::int64_t>(probe.directionQ15.y)*speed,kInputUnit),
        };
        probe.position.x+=velocity.x;
        probe.position.y+=velocity.y;
        if (!CoverHandleValid(state.combat,probe.physicalHandle)) return false;
        CoverSlot& physical=state.combat.covers[probe.physicalHandle.index];
        physical.bounds=RectPixels{probe.position.x/kQ8One-4,
            probe.position.y/kQ8One-4,8,8};
    }
    return true;
}

void ResolveIndexProbeImpacts(Act3Runtime& state) noexcept {
    for (IndexRuntime::Probe& probe:state.index.probes) {
        if (probe.active==0 || CoverHandleValid(state.combat,probe.physicalHandle)) continue;
        probe.active=0;
        probe.respawnTicks=120;
    }
}

void RemoveWitnessSegment(CombatState& combat,SlotHandle handle) noexcept {
    if (!CoverHandleValid(combat,handle)) return;
    combat.covers[handle.index].mass=0;
    combat.covers[handle.index].active=0;
}

bool StartWitnessSweeper(Act3Runtime& state,std::uint8_t targetIndex) noexcept {
    if (targetIndex>=kWitnessRecordCount || state.witness.sweeperActive!=0
        || state.witness.sweeperReturning!=0
        || state.movement.physicalLedger.mass[AccountIndex(
            PhysicalAccount::ProtocolActive)]!=32) return false;
    CombatState combat=state.combat;
    const std::int32_t targetY=state.witness.records[targetIndex].position.y/kQ8One;
    SlotHandle handles[kWitnessSweeperSegmentCount]{};
    for (std::uint8_t i=0;i<kWitnessSweeperSegmentCount;++i) {
        if (!AddBackedCover(combat,RectPixels{12,targetY-16+i*4,4,4},4,
                PhysicalAccount::ProtocolActive,CoverBehavior::Sweeper,handles[i])) return false;
    }
    if (!CombatStateValid(combat,state.movement.physicalLedger)) return false;
    state.combat=combat;
    for (std::uint8_t i=0;i<kWitnessSweeperSegmentCount;++i)
        state.witness.sweeperSegments[i]=handles[i];
    state.witness.sweeperTarget=targetIndex;
    state.witness.sweeperTick=0;
    state.witness.sweeperGap=0;
    state.witness.sweeperActive=1;
    return true;
}

bool FinishWitnessSweeper(Act3Runtime& state) noexcept {
    WitnessRuntime& witness=state.witness;
    if (witness.sweeperActive==0 || witness.sweeperTick<90) return true;
    witness.sweeperGap=CoverHandleValid(state.combat,witness.sweeperSegments[4])?0u:1u;
    if (witness.sweeperGap==0 && witness.records[witness.sweeperTarget].active!=0
        && !TransferWitnessRecord(state,witness.sweeperTarget,
            PhysicalAccount::Transit)) return false;
    for (SlotHandle handle:witness.sweeperSegments) RemoveWitnessSegment(state.combat,handle);
    witness.protocolEscrowMass=static_cast<std::uint16_t>(32
        -state.movement.physicalLedger.mass[AccountIndex(PhysicalAccount::ProtocolActive)]);
    witness.sweeperActive=0;
    witness.sweeperReturnTick=0;
    witness.sweeperReturning=witness.protocolEscrowMass!=0?1u:0u;
    return CombatStateValid(state.combat,state.movement.physicalLedger);
}

bool TickWitness(Act3Runtime& state, const GameplayIntent& intent) noexcept {
    WitnessRuntime& witness = state.witness;
    if (witness.sweeperReturning!=0) {
        ++witness.sweeperReturnTick;
        const std::uint16_t targetReturned=static_cast<std::uint16_t>(
            static_cast<std::uint32_t>(witness.protocolEscrowMass)
                *witness.sweeperReturnTick/36u);
        const std::uint16_t baseline=static_cast<std::uint16_t>(
            32-witness.protocolEscrowMass);
        const std::uint16_t active=state.movement.physicalLedger.mass[
            AccountIndex(PhysicalAccount::ProtocolActive)];
        const std::uint16_t returned=active>baseline?static_cast<std::uint16_t>(active-baseline):0;
        if (targetReturned>returned) {
            PhysicalLedger ledger=state.movement.physicalLedger;
            if (!PhysicalTransfer(ledger,PhysicalAccount::Transit,
                    PhysicalAccount::ProtocolActive,
                    static_cast<std::uint16_t>(targetReturned-returned))
                || !CombatStateValid(state.combat,ledger)) return false;
            state.movement.physicalLedger=ledger;
        }
        if (witness.sweeperReturnTick>=36) witness.sweeperReturning=0;
    }
    const std::uint32_t waveStarts[3] = {1,1201,2401};
    for (std::uint8_t wave=0; wave<3; ++wave) {
        if (state.encounterTick != waveStarts[wave]) continue;
        std::uint32_t stable = 0xFFFFFFFFu;
        std::uint8_t target = 0xFFu;
        for (std::uint8_t i=0; i<kWitnessRecordCount; ++i) {
            if (witness.records[i].active != 0 && witness.records[i].stableId < stable) {
                stable=witness.records[i].stableId; target=i;
            }
        }
        if (target != 0xFFu && !StartWitnessSweeper(state,target)) return false;
    }
    if (witness.sweeperActive != 0) {
        ++witness.sweeperTick;
        const WitnessRecord& target = witness.records[witness.sweeperTarget];
        const std::int32_t x=12+static_cast<std::int32_t>(
            (static_cast<std::int64_t>(target.position.x/kQ8One-12)
                *witness.sweeperTick)/90);
        for (SlotHandle handle:witness.sweeperSegments) {
            if (CoverHandleValid(state.combat,handle)) state.combat.covers[handle.index].bounds.x=x;
        }
    }
    std::uint8_t nearest=0xFFu;
    std::uint64_t nearestDistance=static_cast<std::uint64_t>(18*kQ8One)*(18*kQ8One)+1u;
    const Vec2Q8 player=state.combat.actors[0].position;
    for (std::uint8_t i=0; i<kWitnessRecordCount; ++i) {
        const WitnessRecord& record=witness.records[i];
        if (record.active==0) continue;
        const std::int64_t dx=record.position.x-player.x;
        const std::int64_t dy=record.position.y-player.y;
        const std::uint64_t distance=static_cast<std::uint64_t>(dx*dx+dy*dy);
        if (distance<nearestDistance || (distance==nearestDistance
                && nearest!=0xFFu && record.stableId<witness.records[nearest].stableId)) {
            nearest=i; nearestDistance=distance;
        }
    }
    for (WitnessRecord& record : witness.records) {
        if (&record != (nearest==0xFFu ? nullptr : &witness.records[nearest])) record.channelTicks=0;
    }
    if (nearest!=0xFFu && intent.action==GameplayAction::Reclaim) {
        WitnessRecord& record=witness.records[nearest];
        if (++record.channelTicks>=kWitnessDismantleTicks) {
            if (!TransferWitnessRecord(state,nearest,PhysicalAccount::PlayerShell)) {
                --record.channelTicks;
                return false;
            }
            witness.directRecordMass=static_cast<std::uint8_t>(
                witness.directRecordMass+16);
        }
    } else if (nearest!=0xFFu) witness.records[nearest].channelTicks=0;
    return true;
}

void UpdateIndex(Act3Runtime& state, const GameplayIntent& intent, Vec2Q8 before,
    std::uint16_t coverBefore, std::uint16_t shellBefore) noexcept {
    IndexRuntime& index=state.index;
    const Vec2Q8 after=state.combat.actors[0].position;
    const std::int64_t dx=after.x-before.x;
    const std::int64_t dy=after.y-before.y;
    index.distanceQ8 += IntegerSqrt(static_cast<std::uint64_t>(dx*dx+dy*dy));
    const PhysicalLedger& ledger=state.movement.physicalLedger;
    const std::uint16_t coverAfter=ledger.mass[AccountIndex(PhysicalAccount::Cover)];
    const std::uint16_t shellAfter=ledger.mass[AccountIndex(PhysicalAccount::OpponentShell)];
    if (coverAfter<coverBefore) index.damage=static_cast<std::uint16_t>(
        index.damage+coverBefore-coverAfter);
    if (shellAfter<shellBefore) index.damage=static_cast<std::uint16_t>(
        index.damage+shellBefore-shellAfter);
    if (intent.action==GameplayAction::None) ++index.idleTicks;
    std::uint8_t destroyed=0;
    for (SlotHandle handle:index.metricNodes) if (!CoverHandleValid(state.combat,handle)) ++destroyed;
    index.destroyedMetricNodes=destroyed;
}

void RenderBattle(PixelBuffer buffer, const Act3Runtime& state) noexcept {
    Clear(buffer,PaletteColor::Background);
    DrawLedgerGrid(buffer);
    for (const CoverSlot& cover:state.combat.covers) {
        if (cover.active==0) continue;
        FillRect(buffer,RectI{cover.bounds.x,cover.bounds.y,cover.bounds.width,
            cover.bounds.height},cover.behavior==CoverBehavior::Sweeper
                ?PaletteColor::Warning:PaletteColor::Cover);
    }
    for (const NeutralSlot& neutral:state.combat.neutrals) {
        if (neutral.active==0) continue;
        FillRect(buffer,RectI{neutral.position.x/kQ8One-1,neutral.position.y/kQ8One-1,3,3},
            PaletteColor::Neutral);
    }
    for (const ProjectileSlot& shot:state.combat.projectiles) {
        if (shot.active==0) continue;
        FillRect(buffer,RectI{shot.position.x/kQ8One-1,shot.position.y/kQ8One-1,3,3},
            shot.owner==CombatSide::Player?PaletteColor::Player:PaletteColor::Opponent);
    }
    for (std::uint8_t side=0;side<2;++side) {
        const ChargeSlot& charge=state.combat.charges[side];
        if (charge.active==0) continue;
        const std::int32_t radius=ProjectileRadiusPixels(charge.mass);
        StrokeRect(buffer,RectI{charge.position.x/kQ8One-radius,
            charge.position.y/kQ8One-radius,radius*2+1,radius*2+1},
            side==0?PaletteColor::Player:PaletteColor::Opponent);
    }
    FillRect(buffer,RectI{state.combat.actors[0].position.x/kQ8One-2,
        state.combat.actors[0].position.y/kQ8One-2,5,5},PaletteColor::Core);
    FillRect(buffer,RectI{state.combat.actors[1].position.x/kQ8One-2,
        state.combat.actors[1].position.y/kQ8One-2,5,5},PaletteColor::Opponent);
    DrawLatin(buffer,14,6,kDefinitions[state.encounterIndex].name,PaletteColor::Warning);
    if (state.encounterIndex==0) {
        char meters[]="DAMAGE 000  DIST 000  IDLE 0000";
        WriteUnsigned(meters,7,state.index.damage>999?999:state.index.damage,3);
        WriteUnsigned(meters,17,(state.index.distanceQ8/kQ8One)>999?999:
            state.index.distanceQ8/kQ8One,3);
        WriteUnsigned(meters,27,state.index.idleTicks>9999?9999:state.index.idleTicks,4);
        DrawLatin(buffer,60,158,meters,PaletteColor::Neutral);
        for (const IndexRuntime::Probe& probe:state.index.probes) {
            if (probe.active==0) continue;
            StrokeRect(buffer,RectI{probe.position.x/kQ8One-3,
                probe.position.y/kQ8One-3,7,7},PaletteColor::Warning);
        }
    } else if (state.encounterIndex==1) {
        for (const WitnessRecord& record:state.witness.records) {
            if (record.active==0) continue;
            StrokeRect(buffer,RectI{record.position.x/kQ8One-6,
                record.position.y/kQ8One-3,12,6},PaletteColor::Warning);
        }
    } else if (state.current.action==CurrentAction::Foreclose) {
        FillRect(buffer,RectI{250,7,48-state.current.actionTicks*48/75,2},
            PaletteColor::Opponent);
    }
}

}  // namespace

const Act3EncounterDef& GetAct3EncounterDef(std::uint8_t index) noexcept {
    return kDefinitions[index<kAct3EncounterCount?index:0];
}

bool ValidateAct3EncounterDef(const Act3EncounterDef& definition) noexcept {
    if (definition.id<EncounterId::Index || definition.id>EncounterId::Current
        || definition.fieldCount>kAct3MaximumFieldCount
        || definition.neutralCount>kAct3MaximumNeutralCount
        || definition.opponentMass<kCoreMass) return false;
    std::uint32_t fieldMass=0;
    for (std::uint8_t i=0;i<definition.fieldCount;++i) {
        const Act3RectMassDef& field=definition.field[i];
        const RectPixels bounds=Bounds(field);
        if (field.mass==0 || bounds.x<kPlayInterior.x || bounds.y<kPlayInterior.y
            || bounds.x+bounds.width>kPlayInterior.x+kPlayInterior.width
            || bounds.y+bounds.height>kPlayInterior.y+kPlayInterior.height) return false;
        fieldMass+=field.mass;
    }
    std::uint32_t neutralMass=0;
    for (std::uint8_t i=0;i<definition.neutralCount;++i) {
        const Act3NeutralDef& neutral=definition.neutral[i];
        if (neutral.mass==0 || neutral.x<kPlayInterior.x || neutral.y<kPlayInterior.y
            || neutral.x>=kPlayInterior.x+kPlayInterior.width
            || neutral.y>=kPlayInterior.y+kPlayInterior.height) return false;
        neutralMass+=neutral.mass;
    }
    if (fieldMass!=definition.fieldMass || neutralMass!=definition.neutralMass) return false;
    const auto circleOverlaps=[](std::int32_t x,std::int32_t y,std::int32_t radius,
            RectPixels bounds) noexcept {
        const std::int32_t closestX=x<bounds.x?bounds.x:
            (x>bounds.x+bounds.width?bounds.x+bounds.width:x);
        const std::int32_t closestY=y<bounds.y?bounds.y:
            (y>bounds.y+bounds.height?bounds.y+bounds.height:y);
        const std::int64_t dx=static_cast<std::int64_t>(x)-closestX;
        const std::int64_t dy=static_cast<std::int64_t>(y)-closestY;
        return dx*dx+dy*dy<static_cast<std::int64_t>(radius)*radius;
    };
    const auto spawnFree=[&](std::int32_t x,std::int32_t y,
            std::int32_t radius) noexcept {
        if (x-radius<kPlayInterior.x || y-radius<kPlayInterior.y
            || x+radius>kPlayInterior.x+kPlayInterior.width
            || y+radius>kPlayInterior.y+kPlayInterior.height) return false;
        for (std::uint8_t i=0;i<definition.fieldCount;++i)
            if (circleOverlaps(x,y,radius,Bounds(definition.field[i]))) return false;
        return true;
    };
    const std::int32_t playerRadius=ActorRadiusPixels(64);
    const std::int32_t opponentRadius=ActorRadiusPixels(definition.opponentMass);
    if (!spawnFree(definition.playerX,definition.playerY,playerRadius)
        || !spawnFree(definition.opponentX,definition.opponentY,opponentRadius)) return false;
    const std::int64_t spawnDx=definition.playerX-definition.opponentX;
    const std::int64_t spawnDy=definition.playerY-definition.opponentY;
    if (spawnDx*spawnDx+spawnDy*spawnDy
        <static_cast<std::int64_t>(playerRadius+opponentRadius)
            *(playerRadius+opponentRadius)) return false;
    constexpr std::int32_t grid=20;
    constexpr std::int32_t columns=(kPlayInterior.width+grid-1)/grid;
    constexpr std::int32_t rows=(kPlayInterior.height+grid-1)/grid;
    bool blocked[rows][columns]{};
    for (std::int32_t row=0;row<rows;++row) for (std::int32_t column=0;column<columns;++column) {
        const std::int32_t x=kPlayInterior.x
            +(column*grid+grid/2<kPlayInterior.width?column*grid+grid/2:kPlayInterior.width-1);
        const std::int32_t y=kPlayInterior.y
            +(row*grid+grid/2<kPlayInterior.height?row*grid+grid/2:kPlayInterior.height-1);
        for (std::uint8_t i=0;i<definition.fieldCount;++i) {
            const RectPixels bounds=Bounds(definition.field[i]);
            if (x>=bounds.x && y>=bounds.y && x<bounds.x+bounds.width
                && y<bounds.y+bounds.height) blocked[row][column]=true;
        }
    }
    const auto columnFor=[](std::int32_t x) noexcept {
        const std::int32_t value=(x-kPlayInterior.x)/grid;
        return value<0?0:(value>=columns?columns-1:value);
    };
    const auto rowFor=[](std::int32_t y) noexcept {
        const std::int32_t value=(y-kPlayInterior.y)/grid;
        return value<0?0:(value>=rows?rows-1:value);
    };
    const std::int32_t startColumn=columnFor(definition.playerX);
    const std::int32_t startRow=rowFor(definition.playerY);
    const std::int32_t goalColumn=columnFor(definition.opponentX);
    const std::int32_t goalRow=rowFor(definition.opponentY);
    blocked[startRow][startColumn]=false;
    blocked[goalRow][goalColumn]=false;
    bool reached[rows][columns]{};
    std::uint8_t queueRow[rows*columns]{};
    std::uint8_t queueColumn[rows*columns]{};
    std::uint16_t read=0;
    std::uint16_t write=1;
    queueRow[0]=static_cast<std::uint8_t>(startRow);
    queueColumn[0]=static_cast<std::uint8_t>(startColumn);
    reached[startRow][startColumn]=true;
    constexpr std::int8_t dx[4]={-1,1,0,0};
    constexpr std::int8_t dy[4]={0,0,-1,1};
    while (read<write) {
        const std::int32_t row=queueRow[read];
        const std::int32_t column=queueColumn[read++];
        for (std::uint8_t direction=0;direction<4;++direction) {
            const std::int32_t nextColumn=column+dx[direction];
            const std::int32_t nextRow=row+dy[direction];
            if (nextColumn<0 || nextColumn>=columns || nextRow<0 || nextRow>=rows
                || blocked[nextRow][nextColumn] || reached[nextRow][nextColumn]) continue;
            reached[nextRow][nextColumn]=true;
            queueRow[write]=static_cast<std::uint8_t>(nextRow);
            queueColumn[write++]=static_cast<std::uint8_t>(nextColumn);
        }
    }
    if (!reached[goalRow][goalColumn]) return false;
    const auto freeDirections=[&](std::int32_t x,std::int32_t y,std::int32_t radius) noexcept {
        std::uint8_t free=0;
        for (std::uint8_t direction=0;direction<4;++direction)
            if (spawnFree(x+dx[direction]*grid,y+dy[direction]*grid,radius)) ++free;
        return free>=2;
    };
    return freeDirections(definition.playerX,definition.playerY,playerRadius)
        && freeDirections(definition.opponentX,definition.opponentY,opponentRadius);
}

ChoiceId ResolveIndexResult(std::uint16_t damage, std::uint32_t distanceQ8,
    std::uint16_t idleTicks, std::uint16_t unscoredMass,
    std::uint8_t destroyedMetricNodes) noexcept {
    const bool optimize=damage>=32 && distanceQ8>=160u*kQ8One && idleTicks==0;
    if (destroyedMetricNodes>=2) return ChoiceId::Third;
    if (unscoredMass==16 && !optimize) return ChoiceId::Second;
    if (optimize) return ChoiceId::First;
    return ChoiceId::Second;
}

ChoiceId ResolveWitnessResult(std::uint8_t recordsPreserved,
    std::uint8_t directRecordMass) noexcept {
    if (directRecordMass>=48) return ChoiceId::Third;
    return recordsPreserved==3 ? ChoiceId::First : ChoiceId::Second;
}

CurrentAction CurrentActionAtCycleTick(std::uint16_t tick) noexcept {
    tick=static_cast<std::uint16_t>(tick%454u);
    if (tick<48) return CurrentAction::Orbit;
    if (tick<108) return CurrentAction::MoveA;
    if (tick<109) return CurrentAction::Wave;
    if (tick<199) return CurrentAction::ReturnA;
    if (tick<259) return CurrentAction::MoveB;
    if (tick<334) return CurrentAction::Foreclose;
    return CurrentAction::ReturnB;
}

Vec2Q8 RotateCurrentDirection(Vec2Q8 directionQ15, std::int8_t degrees) noexcept {
    directionQ15=NormalizeDirection(directionQ15);
    std::int32_t cosine=32767;
    std::int32_t sine=0;
    switch (degrees<0?-degrees:degrees) {
    case 8: cosine=32448; sine=4560; break;
    case 24: cosine=29934; sine=13327; break;
    default: break;
    }
    if (degrees<0) sine=-sine;
    return NormalizeDirection(Vec2Q8{
        RoundDivideSigned(static_cast<std::int64_t>(directionQ15.x)*cosine
            -static_cast<std::int64_t>(directionQ15.y)*sine,kInputUnit),
        RoundDivideSigned(static_cast<std::int64_t>(directionQ15.x)*sine
            +static_cast<std::int64_t>(directionQ15.y)*cosine,kInputUnit),
    });
}

bool PopulateAct3Encounter(Act3Runtime& state) noexcept {
    const Act3EncounterDef& definition=GetAct3EncounterDef(state.encounterIndex);
    if (!ValidateAct3EncounterDef(definition)) return false;
    if (!InitializePhysicalLedger(state.movement.physicalLedger,
            PlayerActiveMass(state.claimLedger),definition.opponentMass,
            definition.fieldMass,definition.neutralMass)) return false;
    InitializeCombatState(state.combat,
        Vec2Q8{definition.playerX*kQ8One,definition.playerY*kQ8One},
        Vec2Q8{definition.opponentX*kQ8One,definition.opponentY*kQ8One});
    state.movement.playerPosition=state.combat.actors[0].position;
    state.movement.playerVelocity=Vec2Q8{0,0};
    InitializeReclaimState(state.reclaim);
    state.index=IndexRuntime{};
    state.witness=WitnessRuntime{};
    state.current=CurrentRuntime{};
    std::uint8_t metric=0;
    for (std::uint8_t i=0;i<definition.fieldCount;++i) {
        SlotHandle handle{};
        if (!AddCover(state.combat,Bounds(definition.field[i]),definition.field[i].mass,
                handle)) return false;
        if (definition.field[i].metric!=0) state.index.metricNodes[metric++]=handle;
    }
    std::uint8_t unscored=0;
    for (std::uint8_t i=0;i<definition.neutralCount;++i) {
        const Act3NeutralDef& neutral=definition.neutral[i];
        if (!AddNeutral(state.combat,Vec2Q8{neutral.x*kQ8One,neutral.y*kQ8One},
                neutral.mass,0,0,state.neutrals[i])) return false;
        if (neutral.tag==Act3NeutralTag::Unscored) state.index.unscored[unscored++]=state.neutrals[i];
    }
    if (definition.id==EncounterId::Witness) {
        constexpr std::int16_t positions[3][2]={{132,78},{160,92},{188,106}};
        for (std::uint8_t i=0;i<3;++i) state.witness.records[i]=WitnessRecord{
            Vec2Q8{positions[i][0]*kQ8One,positions[i][1]*kQ8One},
            static_cast<std::uint32_t>(100+i),0,1};
    } else if (definition.id==EncounterId::Index) {
        constexpr std::int16_t spawnY[2]={56,124};
        for (std::uint8_t i=0;i<2;++i) {
            state.index.probes[i].position=Vec2Q8{160*kQ8One,spawnY[i]*kQ8One};
            state.index.probes[i].targetCore=i;
            const Vec2Q8 target=state.combat.actors[i].position;
            state.index.probes[i].directionQ15=NormalizeDirection(Vec2Q8{
                target.x-state.index.probes[i].position.x,
                target.y-state.index.probes[i].position.y});
            if (!AddBackedCover(state.combat,RectPixels{156,spawnY[i]-4,8,8},16,
                    PhysicalAccount::ProtocolActive,CoverBehavior::Checksum,
                    state.index.probes[i].physicalHandle)) return false;
            state.index.probes[i].active=1;
        }
    } else if (definition.id==EncounterId::Current && !PopulateCurrent(state)) return false;
    return CombatStateValid(state.combat,state.movement.physicalLedger);
}

bool InitializeAct3Runtime(Act3Runtime& state,const ClaimLedger& claims,
    const NarrativeState& narrative) noexcept {
    if (!ClaimLedgerValid(claims)) return false;
    state=Act3Runtime{};
    state.claimLedger=claims;
    state.narrative=narrative;
    if (!InitializeGameState(state.movement,0xA3C70001u,narrative.choiceBits)) return false;
    state.movement.claimLedger=claims;
    state.movement.narrative=narrative;
    InitializeHudLoanSystem(state.hudLoan);
    state.scene=Act3Scene::EncounterIntro;
    state.focusedChoice=ChoiceId::Second;
    return true;
}

bool TickAct3Runtime(Act3Runtime& state,const InputFrame& input) noexcept {
    if (state.scene==Act3Scene::Fatal || state.scene==Act3Scene::Complete)
        return state.scene!=Act3Scene::Fatal;
    if (state.scene!=Act3Scene::Playing) ++state.globalTick;
    if (state.scene==Act3Scene::EncounterIntro) {
        ++state.sceneTicks;
        const std::uint16_t minimum=state.introRevisit!=0?kEncounterRevisitIntroMinimumTicks:
            kEncounterIntroMinimumTicks;
        if (state.sceneTicks>=kEncounterIntroAutomaticTicks
            || (state.sceneTicks>=minimum && Pressed(input,PlatformAction::Confirm))) {
            state.scene=Act3Scene::ObjectiveCard; state.sceneTicks=0;
        }
        return true;
    }
    if (state.scene==Act3Scene::ObjectiveCard) {
        if (++state.sceneTicks>=kObjectiveCardTicks) return StartPlaying(state);
        return true;
    }
    if (state.scene==Act3Scene::HitFreeze) {
        if (++state.sceneTicks>=kHitFreezeTicks) {
            state.scene=state.retryPending!=0?Act3Scene::EncounterRetry:Act3Scene::SettlementWait;
            state.sceneTicks=0;
        }
        return true;
    }
    if (state.scene==Act3Scene::EncounterRetry) {
        state.introRevisit=1; state.scene=Act3Scene::EncounterIntro; state.sceneTicks=0;
        return true;
    }
    if (state.scene==Act3Scene::SettlementWait) {
        if (++state.sceneTicks>=kSettlementWaitTicks) {
            if (state.encounterIndex<2) { state.scene=Act3Scene::Preview; state.sceneTicks=0; }
            else { state.scene=Act3Scene::Settlement; state.sceneTicks=0; }
        }
        return true;
    }
    if (state.scene==Act3Scene::Settlement) {
        ChoiceId requested=state.focusedChoice;
        if (Pressed(input,PlatformAction::SelectLeft)) requested=ChoiceId::First;
        if (Pressed(input,PlatformAction::SelectUp)) requested=ChoiceId::Second;
        if (Pressed(input,PlatformAction::SelectRight)) requested=ChoiceId::Third;
        if (requested!=state.focusedChoice) {
            state.focusedChoice=requested; state.scene=Act3Scene::Preview; state.sceneTicks=0;
        } else if (Held(input,PlatformAction::Confirm)) {
            state.scene=Act3Scene::ConfirmHold; state.sceneTicks=0;
        }
        return true;
    }
    if (state.scene==Act3Scene::Preview) {
        if (++state.sceneTicks>=kAllocationPreviewTicks) {
            state.scene=state.encounterIndex<2?Act3Scene::CommitTransit:Act3Scene::Settlement;
            state.sceneTicks=0;
        }
        return true;
    }
    if (state.scene==Act3Scene::ConfirmHold) {
        if (!Held(input,PlatformAction::Confirm)) {
            state.scene=Act3Scene::Settlement; state.sceneTicks=0;
        } else if (++state.sceneTicks>=kConfirmHoldTicks) {
            state.committedChoice=state.focusedChoice;
            state.scene=Act3Scene::CommitTransit; state.sceneTicks=0;
        }
        return true;
    }
    if (state.scene==Act3Scene::CommitTransit) {
        if (++state.sceneTicks>=kCommitTransitTicks) {
            if (!ApplyAct3Choice(state)) { state.scene=Act3Scene::Fatal; return false; }
            state.scene=Act3Scene::EntityResponse; state.sceneTicks=0;
        }
        return true;
    }
    if (state.scene==Act3Scene::EntityResponse) {
        ++state.sceneTicks;
        if (state.sceneTicks>=kEntityResponseAutomaticTicks
            || (state.sceneTicks>=kEntityResponseMinimumTicks
                && Pressed(input,PlatformAction::Confirm))) {
            state.scene=Act3Scene::LedgerReturn; state.sceneTicks=0;
        }
        return true;
    }
    if (state.scene==Act3Scene::LedgerReturn) {
        if (++state.sceneTicks>=kLedgerReturnTicks) {
            ++state.encounterIndex;
            if (state.encounterIndex>=kAct3EncounterCount) state.scene=Act3Scene::Complete;
            else { state.scene=Act3Scene::EncounterIntro; state.sceneTicks=0; }
        }
        return true;
    }

    PhysicalLedger& ledger=state.movement.physicalLedger;
    if (state.movement.paused!=0) {
        if (input.focused!=0 && Pressed(input,PlatformAction::Pause)) {
            state.movement.paused=0; ClearInterruptedActions(state);
        }
        return true;
    }
    if (state.movement.dashCooldown>0) --state.movement.dashCooldown;
    if (state.movement.dashInvulnerableRemaining>0) --state.movement.dashInvulnerableRemaining;
    TickCombatCountdowns(state.combat);
    if (input.focused==0 || Pressed(input,PlatformAction::Pause)) {
        state.movement.paused=1; ClearInterruptedActions(state); return true;
    }
    InputFrame effective=input;
    if (state.movement.dashTicksRemaining>0) {
        ClearAction(effective,PlatformAction::HudReallocate);
        ClearAction(effective,PlatformAction::Dash);
        ClearAction(effective,PlatformAction::Fire);
        ClearAction(effective,PlatformAction::Reclaim);
    }
    state.movement.claimLedger=state.claimLedger;
    state.movement.narrative=state.narrative;
    state.movement.impulseInstalled=HudModuleFullyInstalled(state.hudLoan,HudModuleId::Impulse)?1u:0u;
    const GameplayIntent intent=ResolveGameplayInput(state.movement.inputArbiter,effective,
        CanStartDash(state),true);
    if (!AdvanceHudLoanEarly(state.hudLoan,ledger,
            intent.action==GameplayAction::HudReallocate,state.previousPlayerReclaim)) return false;
    state.previousPlayerReclaim=0;
    if (Pressed(input,PlatformAction::HudCycle)) {
        const std::uint8_t next=static_cast<std::uint8_t>(
            (static_cast<std::uint8_t>(state.hudLoan.selected)+1u)%kHudModuleCount);
        SelectHudModule(state.hudLoan,static_cast<HudModuleId>(next));
    }
    ++state.encounterTick;
    const Vec2Q8 playerBefore=state.combat.actors[0].position;
    const std::uint16_t coverBefore=ledger.mass[AccountIndex(PhysicalAccount::Cover)];
    const std::uint16_t shellBefore=ledger.mass[AccountIndex(PhysicalAccount::OpponentShell)];
    // Authority 32.9 step 07: every opponent/encounter decision observes the
    // pre-Charge, pre-acceleration player state for this tick.
    if (state.encounterIndex==0) {
        if (state.combat.actors[1].exposureGraceTicks==0)
            state.combat.actors[1].exposureGraceTicks=1;
        if (!TickIndexProbes(state)) return false;
    } else if (state.encounterIndex==1) {
        state.combat.actors[1].projectileImmune=1;
        if (!TickWitness(state,intent)) return false;
    } else if (!TickCurrent(state)) return false;
    const Vec2Q8 aim=AimDirection(state,input);
    state.combat.actors[0].aimDirectionQ15=aim;
    ChargeSlot& charge=state.combat.charges[0];
    if (intent.action==GameplayAction::HudReallocate && charge.active!=0) {
        if (!RollbackCharge(state.combat,ledger,CombatSide::Player)) return false;
    } else if (intent.action==GameplayAction::Fire && !HudFireLocked(state.hudLoan)) {
        if (charge.active==0) {
            if (state.combat.actors[0].shotCooldownTicks==0
                && ledger.mass[AccountIndex(PhysicalAccount::PlayerShell)]>=8
                && !BeginCharge(state.combat,ledger,CombatSide::Player,aim)) return false;
        } else if (!AdvanceCharge(state.combat,ledger,CombatSide::Player)) return false;
    } else if (charge.active!=0) {
        if (Held(input,PlatformAction::Fire)
            || input.actions[ActionIndex(PlatformAction::Fire)].released!=0) {
            if (!ReleaseCharge(state.combat,ledger,CombatSide::Player)) return false;
        } else if (!RollbackCharge(state.combat,ledger,CombatSide::Player)) return false;
    }
    state.movement.chargeActive=charge.active!=0?1u:0u;
    if (!AdvanceGameIntent(state.movement,intent)) return false;
    state.combat.actors[0].position=state.movement.playerPosition;
    state.combat.actors[0].velocity=state.movement.playerVelocity;
    const std::int32_t playerRadius=ActorRadiusPixels(
        ledger.mass[AccountIndex(PhysicalAccount::PlayerShell)]);
    if (ResolveActorCovers(state.combat.actors[0],playerRadius,state.combat)
        && state.movement.dashTicksRemaining>0) {
        state.movement.dashTicksRemaining=0;
        state.movement.dashInvulnerableRemaining=0;
    }
    state.movement.playerPosition=state.combat.actors[0].position;
    state.movement.playerVelocity=state.combat.actors[0].velocity;
    if (!StepCombatProjectiles(state.combat,ledger,state.claimLedger)) return false;
    if (state.encounterIndex==0) ResolveIndexProbeImpacts(state);
    if (state.combat.actors[0].coreHit!=0) {
        ++state.movement.globalTick; ++state.globalTick; BeginRetry(state); return true;
    }
    if (state.encounterIndex==2 && state.combat.actors[1].coreHit!=0) {
        ++state.movement.globalTick; ++state.globalTick; BeginSuccess(state,state.focusedChoice); return true;
    }
    if (state.encounterIndex<2 && state.combat.actors[1].coreHit!=0) {
        state.combat.actors[1].coreHit=0;
        state.combat.phase=CombatPhase::Active;
        state.combat.settlement.valid=0;
    }
    if (state.encounterIndex==1 && !FinishWitnessSweeper(state)) return false;
    if (state.encounterIndex==0) UpdateIndex(state,intent,playerBefore,coverBefore,shellBefore);
    if (!TickNeutralDynamics(state.combat,ledger)) return false;
    ReclaimTickResult reclaimed{};
    if (!TickReclaim(state.combat,ledger,state.reclaim,
            intent.action==GameplayAction::Reclaim,false,reclaimed)) return false;
    state.previousPlayerReclaim=reclaimed.playerMass;
    if (!MergeNeutralClusters(state.combat,ledger)) return false;
    const bool playerShellHit=state.combat.shellDamageThisTick[0]!=0;
    if (!ResolvePendingHudCommit(state.hudLoan,ledger,playerShellHit)) return false;
    if (state.encounterIndex==0 && state.encounterTick>=kIndexDurationTicks) {
        BeginSuccess(state,ResolveIndexResult(state.index.damage,state.index.distanceQ8,
            state.index.idleTicks,UnscoredMass(state),state.index.destroyedMetricNodes));
    } else if (state.encounterIndex==1 && state.encounterTick>=kWitnessDurationTicks) {
        BeginSuccess(state,ResolveWitnessResult(WitnessRecordsPreserved(state),
            state.witness.directRecordMass));
    }
    ++state.movement.globalTick;
    ++state.globalTick;
    return PhysicalLedgerValid(ledger) && ClaimLedgerValid(state.claimLedger)
        && CombatStateValid(state.combat,ledger) && HudLoanSystemValid(state.hudLoan,ledger);
}

void RenderAct3Runtime(PixelBuffer buffer,const Act3Runtime& state) noexcept {
    if (state.scene==Act3Scene::Playing || state.scene==Act3Scene::HitFreeze
        || state.scene==Act3Scene::EncounterRetry || state.scene==Act3Scene::SettlementWait) {
        RenderBattle(buffer,state); return;
    }
    Clear(buffer,PaletteColor::Background);
    const Act3EncounterDef& definition=GetAct3EncounterDef(state.encounterIndex);
    switch (state.scene) {
    case Act3Scene::EncounterIntro:
        DrawCentered(buffer,70,definition.introLine1,PaletteColor::Neutral);
        DrawCentered(buffer,84,definition.introLine2,PaletteColor::Neutral); break;
    case Act3Scene::ObjectiveCard:
        DrawCentered(buffer,76,definition.name,PaletteColor::Opponent);
        DrawCentered(buffer,92,definition.objective,PaletteColor::Warning); break;
    case Act3Scene::Settlement: case Act3Scene::Preview:
    case Act3Scene::ConfirmHold: case Act3Scene::CommitTransit:
        DrawCentered(buffer,50,definition.name,PaletteColor::Opponent);
        if (state.encounterIndex==0)
            DrawCentered(buffer,80,"OPTIMIZE   REFUSE THE SCORE   REWRITE THE METRIC",PaletteColor::Neutral);
        else if (state.encounterIndex==1)
            DrawCentered(buffer,80,"PRESERVE   COMPRESS   ERASE",PaletteColor::Neutral);
        else DrawCentered(buffer,80,"HOLD   RETURN   CIRCULATE",PaletteColor::Neutral);
        break;
    case Act3Scene::EntityResponse:
        DrawCentered(buffer,84,state.committedChoice==ChoiceId::First?"THE FIRST CLAIM STANDS":
            (state.committedChoice==ChoiceId::Second?"THE RECORD REMAINS":"THE RULE CHANGES"),
            PaletteColor::Warning); break;
    case Act3Scene::LedgerReturn:
        DrawCentered(buffer,84,"RETURN TO LEDGER",PaletteColor::Neutral); break;
    case Act3Scene::Complete:
        DrawCentered(buffer,84,"ACT 3 COMPLETE",PaletteColor::Warning); break;
    case Act3Scene::Fatal:
        DrawCentered(buffer,84,"LEDGER ERROR",PaletteColor::Warning); break;
    default: break;
    }
}

std::uint32_t Act3RuntimeHash(const Act3Runtime& state) noexcept {
    std::uint32_t hash=2166136261u;
    const auto add=[&hash](std::uint32_t value) noexcept {
        for (std::uint8_t i=0;i<4;++i) { hash^=static_cast<std::uint8_t>(value>>(i*8)); hash*=16777619u; }
    };
    const auto addVec=[&add](Vec2Q8 value) noexcept {
        add(static_cast<std::uint32_t>(value.x)); add(static_cast<std::uint32_t>(value.y));
    };
    const auto addHandle=[&add](SlotHandle value) noexcept { add(value.index); add(value.generation); };
    const auto addRect=[&add](RectPixels value) noexcept {
        add(static_cast<std::uint32_t>(value.x)); add(static_cast<std::uint32_t>(value.y));
        add(static_cast<std::uint32_t>(value.width)); add(static_cast<std::uint32_t>(value.height));
    };
    for (std::uint16_t value:state.claimLedger.mass) add(value);
    add(state.narrative.choiceBits); add(state.narrative.adjudicatedMass);
    add(state.narrative.encounterIndex); add(state.narrative.role);
    for (std::uint8_t value:state.narrative.entityStatus) add(value);
    add(state.narrative.endingsSeen); add(GameStateHash(state.movement));
    for (const ChargeSlot& charge:state.combat.charges) {
        addVec(charge.position); addVec(charge.directionQ15); add(charge.stableId);
        add(charge.generation); add(charge.mass); add(charge.stageMass);
        add(charge.ageTicks); add(charge.active);
    }
    for (const ProjectileSlot& shot:state.combat.projectiles) {
        addVec(shot.position); addVec(shot.velocity); add(shot.stableId); add(shot.generation);
        add(shot.mass); add(shot.widthPixels); add(shot.heightPixels);
        add(static_cast<std::uint32_t>(shot.owner)); add(shot.active);
    }
    for (const NeutralSlot& neutral:state.combat.neutrals) {
        addVec(neutral.position); addVec(neutral.velocity); add(neutral.stableId);
        add(neutral.generation); add(neutral.mass); add(neutral.reclaimLockTicks);
        add(neutral.immunityTicks); add(neutral.playerMissMass); add(neutral.opponentMissMass);
        add(neutral.protocolTagMass); add(neutral.patchFeedMass); add(neutral.adapterEscrowMass);
        add(neutral.structureMass); add(neutral.inertiaTicks); add(neutral.active);
    }
    for (const CoverSlot& cover:state.combat.covers) {
        addRect(cover.bounds); add(cover.stableId); add(cover.generation); add(cover.mass);
        add(static_cast<std::uint32_t>(cover.backingAccount));
        add(static_cast<std::uint32_t>(cover.behavior)); add(cover.active);
    }
    for (const CombatActor& actor:state.combat.actors) {
        addVec(actor.position); addVec(actor.velocity); addVec(actor.aimDirectionQ15);
        add(actor.exposureGraceTicks); add(actor.shotCooldownTicks);
        add(actor.projectileImmune); add(actor.coreHit);
    }
    for (std::uint16_t value:state.combat.settlement.physicalLedger.mass) add(value);
    for (std::uint16_t value:state.combat.settlement.claimLedger.mass) add(value);
    add(state.combat.settlement.tick); add(static_cast<std::uint32_t>(state.combat.settlement.hitSide));
    add(state.combat.settlement.valid); add(state.combat.shellDamageThisTick[0]);
    add(state.combat.shellDamageThisTick[1]); add(state.combat.nextStableId);
    add(state.combat.globalTick); add(static_cast<std::uint32_t>(state.combat.phase));
    add(static_cast<std::uint32_t>(state.combat.error));
    for (std::uint16_t value:state.reclaim.accumulator) add(value);
    for (std::uint16_t value:state.reclaim.targetIndex) add(value);
    for (const HudModuleLoan& module:state.hudLoan.modules) {
        add(module.mass); add(module.channelTicks); add(module.stateTicks);
        add(module.maturityRemaining); add(module.maturityAdvanceTicks);
        add(module.transitMass); add(module.arrivedMass);
        add(static_cast<std::uint32_t>(module.state)); add(module.warning);
    }
    add(static_cast<std::uint32_t>(state.hudLoan.selected));
    add(static_cast<std::uint32_t>(state.hudLoan.pendingModule));
    add(state.hudLoan.reallocationLockTicks); add(state.hudLoan.pendingCommit);
    for (SlotHandle handle:state.neutrals) addHandle(handle);
    for (const IndexRuntime::Probe& probe:state.index.probes) {
        addVec(probe.position); addVec(probe.directionQ15); addHandle(probe.physicalHandle);
        add(probe.respawnTicks);
        add(probe.targetCore); add(probe.active);
    }
    for (SlotHandle handle:state.index.metricNodes) addHandle(handle);
    for (SlotHandle handle:state.index.unscored) addHandle(handle);
    add(state.index.distanceQ8); add(state.index.damage); add(state.index.idleTicks);
    add(state.index.destroyedMetricNodes);
    for (const WitnessRecord& record:state.witness.records) {
        addVec(record.position); add(record.stableId); add(record.channelTicks); add(record.active);
    }
    for (SlotHandle handle:state.witness.sweeperSegments) addHandle(handle);
    add(state.witness.sweeperTick); add(state.witness.sweeperReturnTick);
    add(state.witness.protocolEscrowMass); add(state.witness.sweeperTarget);
    add(state.witness.sweeperActive); add(state.witness.sweeperGap);
    add(state.witness.sweeperReturning); add(state.witness.directRecordMass);
    for (SlotHandle handle:state.current.ownedNeutrals) addHandle(handle);
    add(static_cast<std::uint32_t>(state.current.action)); addVec(state.current.forecloseAim);
    add(state.current.actionTicks); add(state.current.returnAccumulator);
    add(state.current.initialProjectileFirstStableId); add(state.current.cycleIndex);
    add(state.current.orbitShots);
    add(static_cast<std::uint32_t>(state.scene));
    add(static_cast<std::uint32_t>(state.focusedChoice));
    add(static_cast<std::uint32_t>(state.committedChoice));
    add(state.globalTick); add(state.encounterTick); add(state.sceneTicks);
    add(state.previousPlayerReclaim); add(state.encounterIndex);
    add(state.retryPending); add(state.introRevisit);
    return hash;
}

#if defined(BP_TEST)
bool ForceAct3EncounterSuccessForTest(Act3Runtime& state) noexcept {
    if (state.scene!=Act3Scene::Playing) return false;
    ChoiceId result=state.focusedChoice;
    if (state.encounterIndex==0) result=ResolveIndexResult(state.index.damage,
        state.index.distanceQ8,state.index.idleTicks,UnscoredMass(state),
        state.index.destroyedMetricNodes);
    else if (state.encounterIndex==1) result=ResolveWitnessResult(
        WitnessRecordsPreserved(state),state.witness.directRecordMass);
    BeginSuccess(state,result);
    return true;
}

bool TickCurrentScriptForTest(Act3Runtime& state) noexcept {
    return TickCurrent(state);
}
#endif

}  // namespace bp
