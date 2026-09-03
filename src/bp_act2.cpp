#include "bp_act2.h"

#include "bp_claim_ledger.h"
#include "bp_font.h"
#include "bp_physical_ledger.h"

#include <cstdint>

namespace bp {
namespace {

constexpr std::uint8_t AccountIndex(PhysicalAccount account) noexcept {
    return static_cast<std::uint8_t>(account);
}

constexpr std::uint8_t SideIndex(CombatSide side) noexcept {
    return static_cast<std::uint8_t>(side);
}

constexpr std::uint8_t ActionIndex(PlatformAction action) noexcept {
    return static_cast<std::uint8_t>(action);
}

constexpr bool Pressed(const InputFrame& input, PlatformAction action) noexcept {
    return input.actions[ActionIndex(action)].pressed != 0;
}

constexpr bool Held(const InputFrame& input, PlatformAction action) noexcept {
    return input.actions[ActionIndex(action)].held != 0;
}

constexpr Act2RectMassDef Rect(
    std::int16_t x, std::int16_t y, std::int16_t w, std::int16_t h,
    std::uint16_t mass = 32) noexcept {
    return Act2RectMassDef{x, y, w, h, mass};
}

constexpr Act2NeutralDef Neutral(
    std::int16_t x, std::int16_t y, std::uint16_t mass = 8,
    bool escrow = false) noexcept {
    return Act2NeutralDef{x, y, mass, escrow ? 1u : 0u};
}

constexpr Act2EncounterDef kDefinitions[kAct2EncounterCount] = {
    {
        EncounterId::Keeper, ClaimOwner::Keeper, "KEEPER", "MAINTAIN THE FIELD",
        "NONE OF THIS IS MINE", "THAT IS WHY I MUST KEEP IT",
        112, 192, 64, 5400, 58, 92, 252, 90, 6, 8,
        {
            Rect(92,50,26,8), Rect(160,50,26,8), Rect(228,50,26,8),
            Rect(92,130,26,8), Rect(160,130,26,8), Rect(228,130,26,8),
        },
        {
            Neutral(116,92), Neutral(132,92), Neutral(148,92), Neutral(164,92),
            Neutral(180,92), Neutral(196,92), Neutral(212,92), Neutral(228,92),
        },
    },
    {
        EncounterId::Adapter, ClaimOwner::Adapter, "ADAPTER", "CROSS BY CAPABILITY",
        "EQUAL TILE. UNEQUAL REACH", "COUNT WHAT I CAN DO",
        72, 192, 48, 4500, 46, 92, 274, 92, 6, 6,
        {
            Rect(104,50,34,8), Rect(160,50,34,8), Rect(216,50,34,8),
            Rect(104,130,34,8), Rect(160,130,34,8), Rect(216,130,34,8),
        },
        {
            Neutral(136,76,8,true), Neutral(160,76,8,true), Neutral(184,76,8,true),
            Neutral(136,108,8,true), Neutral(160,108,8,true), Neutral(184,108,8,true),
        },
    },
    {
        EncounterId::Frame, ClaimOwner::Frame, "FRAME", "OPEN A PATH",
        "CHOICES BEGIN WHERE WALLS ALLOW", "I DO NOT OWN PATHS. I DECIDE THEM",
        192, 384, 96, 18000, 44, 92, 268, 90, 12, 12,
        {
            Rect(96,48,28,8), Rect(160,48,28,8), Rect(224,48,28,8),
            Rect(96,132,28,8), Rect(160,132,28,8), Rect(224,132,28,8),
            Rect(112,72,8,24), Rect(160,72,8,24), Rect(208,72,8,24),
            Rect(112,108,8,24), Rect(160,108,8,24), Rect(208,108,8,24),
        },
        {
            Neutral(76,90), Neutral(92,90), Neutral(128,90), Neutral(144,90),
            Neutral(176,90), Neutral(192,90), Neutral(228,90), Neutral(244,90),
            Neutral(136,60), Neutral(184,60), Neutral(136,120), Neutral(184,120),
        },
    },
};

constexpr Act2RectMassDef kKeeperStructures[kKeeperStructureCount] = {
    Rect(120,74,16,6,16), Rect(160,74,16,6,16), Rect(200,74,16,6,16),
    Rect(140,108,16,6,16), Rect(180,108,16,6,16),
};

RectPixels Bounds(const Act2RectMassDef& value) noexcept {
    return RectPixels{
        value.centerX - value.width / 2,
        value.centerY - value.height / 2,
        value.width,
        value.height,
    };
}

std::uint16_t PlayerActiveMass(const ClaimLedger& claims) noexcept {
    const std::uint16_t value = claims.mass[static_cast<std::uint8_t>(ClaimOwner::Player)];
    return value < 64 ? 64 : (value > 240 ? 240 : value);
}

Vec2Q8 AimDirection(const Act2Runtime& state, const InputFrame& input) noexcept {
    Vec2Q8 raw{};
    if (input.device == InputDevice::KeyboardMouse) {
        raw.x = static_cast<std::int32_t>(input.mouseX) * kQ8One
            - state.combat.actors[0].position.x;
        raw.y = static_cast<std::int32_t>(input.mouseY) * kQ8One
            - state.combat.actors[0].position.y;
    } else {
        raw.x = input.aimX;
        raw.y = input.aimY;
    }
    const std::uint64_t square = static_cast<std::uint64_t>(
        static_cast<std::int64_t>(raw.x) * raw.x)
        + static_cast<std::uint64_t>(static_cast<std::int64_t>(raw.y) * raw.y);
    const std::uint32_t length = IntegerSqrt(square);
    if (length == 0) return state.combat.actors[0].aimDirectionQ15;
    return Vec2Q8{
        RoundDivideSigned(static_cast<std::int64_t>(raw.x) * kInputUnit,
            static_cast<std::int32_t>(length)),
        RoundDivideSigned(static_cast<std::int64_t>(raw.y) * kInputUnit,
            static_cast<std::int32_t>(length)),
    };
}

void DrawCentered(PixelBuffer buffer, std::int32_t y, const char* text,
    PaletteColor color) noexcept {
    DrawLatin(buffer, (kRenderWidth - MeasureLatin(text)) / 2, y, text, color);
}

void RenderBattle(PixelBuffer buffer, const Act2Runtime& state) noexcept {
    Clear(buffer, PaletteColor::Background);
    DrawLedgerGrid(buffer);
    for (const CoverSlot& cover : state.combat.covers) {
        if (cover.active == 0) continue;
        const PaletteColor color = cover.behavior == CoverBehavior::Standard
            ? PaletteColor::Cover : (cover.behavior == CoverBehavior::OwnedStructure
                ? PaletteColor::Opponent : PaletteColor::Warning);
        FillRect(buffer, RectI{cover.bounds.x, cover.bounds.y,
            cover.bounds.width, cover.bounds.height}, color);
    }
    for (const NeutralSlot& neutral : state.combat.neutrals) {
        if (neutral.active == 0) continue;
        const std::int32_t x = neutral.position.x / kQ8One;
        const std::int32_t y = neutral.position.y / kQ8One;
        FillRect(buffer, RectI{x - 1, y - 1, 3, 3}, PaletteColor::Neutral);
    }
    for (const ProjectileSlot& shot : state.combat.projectiles) {
        if (shot.active == 0) continue;
        FillRect(buffer, RectI{shot.position.x / kQ8One - 1,
            shot.position.y / kQ8One - 1, 3, 3},
            shot.owner == CombatSide::Player ? PaletteColor::Player : PaletteColor::Opponent);
    }
    for (std::uint8_t side = 0; side < 2; ++side) {
        const ChargeSlot& charge = state.combat.charges[side];
        if (charge.active == 0) continue;
        const std::int32_t radius = ProjectileRadiusPixels(charge.mass);
        StrokeRect(buffer, RectI{charge.position.x/kQ8One-radius,
            charge.position.y/kQ8One-radius, radius*2+1, radius*2+1},
            side == 0 ? PaletteColor::Player : PaletteColor::Opponent);
    }
    const CombatActor& player = state.combat.actors[0];
    const CombatActor& opponent = state.combat.actors[1];
    FillRect(buffer, RectI{player.position.x / kQ8One - 2,
        player.position.y / kQ8One - 2, 5, 5}, PaletteColor::Core);
    FillRect(buffer, RectI{opponent.position.x / kQ8One - 2,
        opponent.position.y / kQ8One - 2, 5, 5}, PaletteColor::Opponent);
    DrawLatin(buffer, 14, 6, kDefinitions[state.encounterIndex].name, PaletteColor::Warning);
    if (state.encounterIndex == 1 && state.adapter.playerDoorOpen == 0) {
        FillRect(buffer, RectI{284, 80, 2, 24}, PaletteColor::Warning);
    }
    if (state.encounterIndex == 1 && state.encounterTick >= kAdapterPreviewTick) {
        char playerText[] = "PLAYER 00";
        char adapterText[] = "ADAPTER 00";
        const std::uint16_t value = state.adapter.playerEscrow;
        playerText[7] = static_cast<char>('0' + value / 10);
        playerText[8] = static_cast<char>('0' + value % 10);
        const std::uint16_t other = static_cast<std::uint16_t>(48 - value);
        adapterText[8] = static_cast<char>('0' + other / 10);
        adapterText[9] = static_cast<char>('0' + other % 10);
        DrawLatin(buffer, 112, 158, playerText, PaletteColor::Player);
        DrawLatin(buffer, 190, 158, adapterText, PaletteColor::Opponent);
    }
}

bool StartEncounterPlaying(Act2Runtime& state) noexcept {
    if (!PopulateAct2Encounter(state)) return false;
    state.scene = Act2Scene::Playing;
    state.sceneTicks = 0;
    state.encounterTick = 0;
    state.introRevisit = 0;
    return true;
}

void BeginSuccess(Act2Runtime& state) noexcept {
    state.retryPending = 0;
    state.combat.phase = CombatPhase::HitFreeze;
    state.scene = Act2Scene::HitFreeze;
    state.sceneTicks = 0;
    state.focusedChoice = ChoiceId::Second;
}

void BeginRetry(Act2Runtime& state) noexcept {
    state.retryPending = 1;
    state.combat.phase = CombatPhase::HitFreeze;
    state.scene = Act2Scene::HitFreeze;
    state.sceneTicks = 0;
}

void ClearAction(InputFrame& input, PlatformAction action) noexcept {
    input.actions[ActionIndex(action)] = ButtonState{};
}

void ClearInterruptedActions(Act2Runtime& state) noexcept {
    state.movement.chargeActive = 0;
    state.movement.reclaimActive = 0;
    state.movement.hudChannelActive = 0;
    if (state.combat.charges[SideIndex(CombatSide::Player)].active != 0) {
        RollbackCharge(state.combat, state.movement.physicalLedger, CombatSide::Player);
    }
}

bool CanStartDash(const Act2Runtime& state) noexcept {
    return state.movement.impulseInstalled != 0
        && state.movement.dashCooldown == 0
        && state.movement.dashTicksRemaining == 0
        && state.movement.chargeActive == 0
        && state.movement.reclaimActive == 0
        && state.movement.hudChannelActive == 0;
}

bool ApplyAct2Choice(Act2Runtime& state) noexcept {
    const EncounterId encounter = GetAct2EncounterDef(state.encounterIndex).id;
    const ChoiceId choice = encounter == EncounterId::Adapter
        ? AdapterChoiceForEscrow(state.adapter.playerEscrow) : state.committedChoice;
    const bool applied = encounter == EncounterId::Adapter
        ? ApplyAdapterSettlementForEscrow(
            state.claimLedger, state.narrative, state.adapter.playerEscrow)
        : ApplySettlement(state.claimLedger, state.narrative, encounter, choice);
    if (!applied) return false;
    state.narrative.encounterIndex = static_cast<std::uint8_t>(encounter) + 1u;
    state.narrative.entityStatus[static_cast<std::uint8_t>(encounter)] =
        static_cast<std::uint8_t>(choice) + 1u;
    state.movement.claimLedger = state.claimLedger;
    state.movement.narrative = state.narrative;
    return true;
}

void RemoveCoverHandle(CombatState& combat, SlotHandle handle) noexcept {
    if (!CoverHandleValid(combat, handle)) return;
    CoverSlot& cover = combat.covers[handle.index];
    cover.mass = 0;
    cover.active = 0;
}

bool Overlap(RectPixels a, RectPixels b) noexcept {
    return a.x < b.x + b.width && a.x + a.width > b.x
        && a.y < b.y + b.height && a.y + a.height > b.y;
}

std::uint64_t DistanceSquared(Vec2Q8 a, Vec2Q8 b) noexcept {
    const std::int64_t dx = static_cast<std::int64_t>(a.x) - b.x;
    const std::int64_t dy = static_cast<std::int64_t>(a.y) - b.y;
    return static_cast<std::uint64_t>(dx*dx + dy*dy);
}

bool PointInside(std::int32_t x, std::int32_t y, RectPixels bounds) noexcept {
    return x >= bounds.x && x < bounds.x + bounds.width
        && y >= bounds.y && y < bounds.y + bounds.height;
}

bool CoverBlocksLine(const CombatState& combat, Vec2Q8 from, Vec2Q8 to) noexcept {
    std::int32_t x = from.x / kQ8One;
    std::int32_t y = from.y / kQ8One;
    const std::int32_t endX = to.x / kQ8One;
    const std::int32_t endY = to.y / kQ8One;
    const std::int32_t dx = endX > x ? endX - x : x - endX;
    const std::int32_t sx = x < endX ? 1 : -1;
    const std::int32_t magnitudeY = endY > y ? endY - y : y - endY;
    const std::int32_t dy = -magnitudeY;
    const std::int32_t sy = y < endY ? 1 : -1;
    std::int32_t error = dx + dy;
    for (;;) {
        for (const CoverSlot& cover : combat.covers) {
            if (cover.active != 0 && PointInside(x, y, cover.bounds)) return true;
        }
        if (x == endX && y == endY) return false;
        const std::int32_t doubled = error * 2;
        if (doubled >= dy) { error += dy; x += sx; }
        if (doubled <= dx) { error += dx; y += sy; }
    }
}

bool ResolveActorCoverCollisions(CombatActor& actor, std::int32_t radiusPixels,
    const CombatState& combat) noexcept {
    bool collided = false;
    for (const CoverSlot& cover : combat.covers) {
        if (cover.active == 0) continue;
        const CollisionResolution collision = ResolveCircleAgainstAabb(
            actor.position, radiusPixels, cover.bounds);
        if (!collision.collided) continue;
        actor.position.x += collision.shiftXQ8;
        actor.position.y += collision.shiftYQ8;
        if (collision.shiftXQ8 != 0) actor.velocity.x = 0;
        if (collision.shiftYQ8 != 0) actor.velocity.y = 0;
        collided = true;
    }
    return collided;
}

bool ShiftRouteClear(const CombatState& combat, std::uint16_t panelIndex,
    std::int8_t dx, std::int8_t dy) noexcept {
    RectPixels destination = combat.covers[panelIndex].bounds;
    destination.x += dx;
    destination.y += dy;
    if (destination.x < kPlayInterior.x || destination.y < kPlayInterior.y
        || destination.x + destination.width > kPlayInterior.x + kPlayInterior.width
        || destination.y + destination.height > kPlayInterior.y + kPlayInterior.height) return false;
    RectPixels sweep = destination;
    const RectPixels start = combat.covers[panelIndex].bounds;
    if (start.x < sweep.x) { sweep.width += sweep.x - start.x; sweep.x = start.x; }
    else sweep.width = start.x + start.width - sweep.x;
    if (start.y < sweep.y) { sweep.height += sweep.y - start.y; sweep.y = start.y; }
    else sweep.height = start.y + start.height - sweep.y;
    for (std::uint16_t index = 0; index < kCoverCapacity; ++index) {
        if (index != panelIndex && combat.covers[index].active != 0
            && Overlap(sweep, combat.covers[index].bounds)) return false;
    }
    return true;
}

std::uint64_t PointSegmentDistanceSquared(Vec2Q8 point, Vec2Q8 a, Vec2Q8 b) noexcept {
    const std::int64_t vx = static_cast<std::int64_t>(b.x) - a.x;
    const std::int64_t vy = static_cast<std::int64_t>(b.y) - a.y;
    const std::int64_t wx = static_cast<std::int64_t>(point.x) - a.x;
    const std::int64_t wy = static_cast<std::int64_t>(point.y) - a.y;
    const std::int64_t lengthSquared = vx*vx + vy*vy;
    if (lengthSquared == 0) return DistanceSquared(point, a);
    std::int64_t projectionQ16 = ((wx*vx + wy*vy) << 16) / lengthSquared;
    if (projectionQ16 < 0) projectionQ16 = 0;
    if (projectionQ16 > 65536) projectionQ16 = 65536;
    const Vec2Q8 closest{
        a.x + static_cast<std::int32_t>(vx * projectionQ16 / 65536),
        a.y + static_cast<std::int32_t>(vy * projectionQ16 / 65536),
    };
    return DistanceSquared(point, closest);
}

std::uint16_t ClosestShiftPanel(const Act2Runtime& state) noexcept {
    const Vec2Q8 player = state.combat.actors[0].position;
    const Vec2Q8 exit{308 * kQ8One, 90 * kQ8One};
    std::uint16_t best = kInvalidSlotIndex;
    std::uint64_t bestDistance = UINT64_MAX;
    for (SlotHandle handle : state.frame.panels) {
        if (!CoverHandleValid(state.combat, handle)) continue;
        const CoverSlot& panel = state.combat.covers[handle.index];
        const Vec2Q8 center{(panel.bounds.x + panel.bounds.width / 2) * kQ8One,
            (panel.bounds.y + panel.bounds.height / 2) * kQ8One};
        const std::uint64_t distance = PointSegmentDistanceSquared(center, player, exit);
        if (best == kInvalidSlotIndex || distance < bestDistance
            || (distance == bestDistance && panel.stableId < state.combat.covers[best].stableId)) {
            best = handle.index;
            bestDistance = distance;
        }
    }
    return best;
}

std::uint16_t ClosestPanel(const Act2Runtime& state) noexcept {
    std::uint16_t best = kInvalidSlotIndex;
    std::uint64_t bestDistance = UINT64_MAX;
    const Vec2Q8 player = state.combat.actors[0].position;
    for (std::uint8_t index = 0; index < kAct2MaximumFieldCount; ++index) {
        const SlotHandle handle = state.frame.panels[index];
        if (!CoverHandleValid(state.combat, handle)) continue;
        const CoverSlot& panel = state.combat.covers[handle.index];
        const std::int64_t x = (panel.bounds.x + panel.bounds.width / 2) * kQ8One - player.x;
        const std::int64_t y = (panel.bounds.y + panel.bounds.height / 2) * kQ8One - player.y;
        const std::uint64_t distance = static_cast<std::uint64_t>(x*x + y*y);
        if (best == kInvalidSlotIndex || distance < bestDistance
            || (distance == bestDistance && panel.stableId < state.combat.covers[best].stableId)) {
            best = handle.index;
            bestDistance = distance;
        }
    }
    return best;
}

bool PlayerNearPanel(const Act2Runtime& state, std::uint16_t panelIndex,
    std::int32_t rangePixels) noexcept {
    if (panelIndex == kInvalidSlotIndex) return false;
    const CoverSlot& panel = state.combat.covers[panelIndex];
    const Vec2Q8 center{(panel.bounds.x + panel.bounds.width / 2) * kQ8One,
        (panel.bounds.y + panel.bounds.height / 2) * kQ8One};
    const std::int64_t range = static_cast<std::int64_t>(rangePixels) * kQ8One;
    return DistanceSquared(state.combat.actors[0].position, center)
        <= static_cast<std::uint64_t>(range*range);
}

FrameObservation ObserveFrame(const Act2Runtime& state) noexcept {
    FrameObservation observation{};
    const Vec2Q8 player = state.combat.actors[0].position;
    const Vec2Q8 frame = state.combat.actors[1].position;
    const std::uint16_t closest = ClosestPanel(state);
    observation.playerNearPanel = PlayerNearPanel(state, closest, 18) ? 1u : 0u;
    observation.playerLowShell = state.movement.physicalLedger.mass[
        AccountIndex(PhysicalAccount::PlayerShell)] <= 40 ? 1u : 0u;
    observation.coverBlocks = CoverBlocksLine(state.combat, frame, player) ? 1u : 0u;
    observation.lineOfSight = observation.coverBlocks == 0 ? 1u : 0u;
    observation.blockedChoice = CoverBlocksLine(
        state.combat, player, Vec2Q8{308*kQ8One,90*kQ8One}) ? 1u : 0u;
    observation.playerChannel = (state.combat.charges[0].active != 0
        || state.movement.reclaimActive != 0 || state.movement.hudChannelActive != 0) ? 1u : 0u;
    observation.selfLowStructure = state.frame.structureControlMass < 64 ? 1u : 0u;
    const std::uint64_t range = DistanceSquared(frame, player);
    observation.rangeFit = range >= static_cast<std::uint64_t>(72*kQ8One)*(72*kQ8One)
        && range <= static_cast<std::uint64_t>(132*kQ8One)*(132*kQ8One) ? 1u : 0u;
    for (const NeutralSlot& neutral : state.combat.neutrals) {
        if (neutral.active != 0
            && DistanceSquared(frame, neutral.position)
                <= static_cast<std::uint64_t>(72*kQ8One)*(72*kQ8One)) {
            observation.neutralNear = 1;
            break;
        }
    }
    for (const ProjectileSlot& shot : state.combat.projectiles) {
        if (shot.active != 0 && shot.owner == CombatSide::Player
            && DistanceSquared(frame, shot.position)
                <= static_cast<std::uint64_t>(80*kQ8One)*(80*kQ8One)) {
            observation.incomingThreat = 1;
            break;
        }
    }
    return observation;
}

bool AddGeneralNeutral(CombatState& combat, PhysicalLedger& ledger,
    Vec2Q8 position, std::uint16_t mass, PhysicalAccount from) noexcept {
    SlotHandle handle{};
    return AddNeutral(combat, position, mass, 0, 0, handle)
        && PhysicalTransfer(ledger, from, PhysicalAccount::Neutral, mass);
}

Vec2Q8 NormalizeDirection(Vec2Q8 value) noexcept {
    const std::uint32_t length = IntegerSqrt(
        static_cast<std::uint64_t>(static_cast<std::int64_t>(value.x)*value.x)
        + static_cast<std::uint64_t>(static_cast<std::int64_t>(value.y)*value.y));
    if (length == 0) return Vec2Q8{-kInputUnit, 0};
    return Vec2Q8{
        RoundDivideSigned(static_cast<std::int64_t>(value.x)*kInputUnit,
            static_cast<std::int32_t>(length)),
        RoundDivideSigned(static_cast<std::int64_t>(value.y)*kInputUnit,
            static_cast<std::int32_t>(length)),
    };
}

bool BeginOpponentMaterialCharge(Act2Runtime& state, PhysicalAccount sourceAccount) noexcept {
    CombatState& combat = state.combat;
    PhysicalLedger& ledger = state.movement.physicalLedger;
    ChargeSlot& charge = combat.charges[SideIndex(CombatSide::Opponent)];
    if (charge.active != 0 || combat.nextStableId == 0
        || combat.nextStableId == UINT32_MAX
        || !PhysicalTransfer(ledger, sourceAccount, PhysicalAccount::Charge, 32)) return false;
    const Vec2Q8 from = combat.actors[1].position;
    const Vec2Q8 player = combat.actors[0].position;
    charge.position = from;
    charge.directionQ15 = NormalizeDirection(Vec2Q8{player.x-from.x, player.y-from.y});
    charge.stableId = combat.nextStableId++;
    charge.generation = static_cast<std::uint16_t>(charge.generation + 1u);
    if (charge.generation == 0) charge.generation = 1;
    charge.mass = 32;
    charge.stageMass = 32;
    charge.ageTicks = 0;
    charge.active = 1;
    return CombatStateValid(combat, ledger);
}

bool BeginFrameActionMutable(Act2Runtime& state, FrameAction action) noexcept {
    PhysicalLedger& ledger = state.movement.physicalLedger;
    const std::uint16_t panelIndex = action == FrameAction::Shift
        ? ClosestShiftPanel(state) : ClosestPanel(state);
    state.frame.action = action;
    state.frame.actionTicks = 0;
    if (action == FrameAction::Shift) {
        if (panelIndex == kInvalidSlotIndex) return false;
        constexpr std::int8_t routeX[4] = {-20, 20, 0, 0};
        constexpr std::int8_t routeY[4] = {0, 0, -20, 20};
        std::uint8_t route = 0;
        while (route < 4 && !ShiftRouteClear(state.combat, panelIndex, routeX[route], routeY[route])) ++route;
        if (route == 4) return false;
        state.frame.shiftPanel = SlotHandle{panelIndex, state.combat.covers[panelIndex].generation};
        state.frame.shiftStart = state.combat.covers[panelIndex].bounds;
        state.frame.shiftX = routeX[route];
        state.frame.shiftY = routeY[route];
        state.frame.actionCooldown = 90;
    } else if (action == FrameAction::Toll) {
        if (panelIndex == kInvalidSlotIndex || state.frame.structureControlMass < 16
            || !PlayerNearPanel(state, panelIndex, 18)) return false;
        CoverSlot& panel = state.combat.covers[panelIndex];
        if (panel.mass < 16) return false;
        const Vec2Q8 center{(panel.bounds.x + panel.bounds.width / 2) * kQ8One,
            (panel.bounds.y + panel.bounds.height / 2) * kQ8One};
        if (!AddGeneralNeutral(state.combat, ledger, center, 16, PhysicalAccount::Cover)) return false;
        panel.mass = static_cast<std::uint16_t>(panel.mass - 16);
        if (panel.mass == 0) panel.active = 0;
        SlotHandle doorHandle{};
        state.frame.door = RectPixels{center.x / kQ8One - 2, center.y / kQ8One - 6, 4, 12};
        if (!AddBackedCover(state.combat, state.frame.door, 16,
                PhysicalAccount::OpponentShell, CoverBehavior::OwnedStructure, doorHandle)) return false;
        state.frame.doorArmed = 1;
        state.frame.doorCooldown = 0;
        state.frame.actionCooldown = 90;
    } else if (action == FrameAction::Evict) {
        if (panelIndex == kInvalidSlotIndex || state.frame.structureControlMass < 32
            || ledger.mass[AccountIndex(PhysicalAccount::Cover)] < 32) return false;
        state.frame.shiftPanel = SlotHandle{panelIndex, state.combat.covers[panelIndex].generation};
        CoverSlot& panel = state.combat.covers[panelIndex];
        if (panel.mass != 32) return false;
        panel.mass = 0;
        panel.active = 0;
        for (std::uint8_t index = 0; index < kAct2MaximumFieldCount; ++index) {
            if (state.frame.panels[index].index == panelIndex) {
                state.frame.panelAliveMask = static_cast<std::uint16_t>(
                    state.frame.panelAliveMask & ~(1u << index));
                break;
            }
        }
        state.frame.structureControlMass = static_cast<std::uint16_t>(
            state.frame.structureControlMass - 32);
        if (!BeginOpponentMaterialCharge(state, PhysicalAccount::Cover)) return false;
        state.frame.actionCooldown = 120;
    } else if (action == FrameAction::Claim) {
        if (ledger.mass[AccountIndex(PhysicalAccount::OpponentShell)] < 32) return false;
        if (!BeginOpponentMaterialCharge(state, PhysicalAccount::OpponentShell)) return false;
        state.frame.actionCooldown = 75;
    } else if (action == FrameAction::Reclaim) {
        state.frame.actionCooldown = 1;
    } else {
        state.frame.actionCooldown = 30;
    }
    return CombatStateValid(state.combat, ledger);
}

bool BeginFrameAction(Act2Runtime& state, FrameAction action) noexcept {
    Act2Runtime candidate = state;
    if (!BeginFrameActionMutable(candidate, action)) return false;
    state = candidate;
    return true;
}

bool TickFrameActionMutable(Act2Runtime& state) noexcept {
    PhysicalLedger& ledger = state.movement.physicalLedger;
    if (state.frame.actionCooldown > 0) --state.frame.actionCooldown;
    if (state.frame.action == FrameAction::Shift
        && CoverHandleValid(state.combat, state.frame.shiftPanel)
        && state.frame.actionTicks < 60) {
        ++state.frame.actionTicks;
        CoverSlot& panel = state.combat.covers[state.frame.shiftPanel.index];
        panel.bounds.x = state.frame.shiftStart.x + state.frame.shiftX * state.frame.actionTicks / 60;
        panel.bounds.y = state.frame.shiftStart.y + state.frame.shiftY * state.frame.actionTicks / 60;
    }
    if ((state.frame.action == FrameAction::Claim || state.frame.action == FrameAction::Evict)
        && state.combat.charges[1].active != 0) {
        ++state.frame.actionTicks;
        ChargeSlot& charge = state.combat.charges[1];
        charge.position = state.combat.actors[1].position;
        const Vec2Q8 player = state.combat.actors[0].position;
        charge.directionQ15 = NormalizeDirection(Vec2Q8{
            player.x-charge.position.x, player.y-charge.position.y});
        if (state.frame.actionTicks >= 30) {
            const std::uint32_t stableId = charge.stableId;
            if (!ReleaseCharge(state.combat, ledger, CombatSide::Opponent)) return false;
            if (state.frame.action == FrameAction::Evict) {
                for (ProjectileSlot& shot : state.combat.projectiles) {
                    if (shot.active != 0 && shot.stableId == stableId) {
                        shot.velocity = Vec2Q8{
                            RoundDivideSigned(static_cast<std::int64_t>(charge.directionQ15.x)
                                * PixelsPerSecondToQ8PerTick(105), kInputUnit),
                            RoundDivideSigned(static_cast<std::int64_t>(charge.directionQ15.y)
                                * PixelsPerSecondToQ8PerTick(105), kInputUnit),
                        };
                        shot.widthPixels = 16;
                        shot.heightPixels = 2;
                        break;
                    }
                }
            }
        }
    } else if (state.frame.action == FrameAction::Move) {
        CombatActor& frame = state.combat.actors[1];
        const CombatActor& player = state.combat.actors[0];
        constexpr std::int32_t acceleration = PixelsPerSecondSquaredToQ8PerTickSquared(500);
        const bool tooFar = DistanceSquared(frame.position, player.position)
            > static_cast<std::uint64_t>(112*kQ8One)*(112*kQ8One);
        const std::int32_t sign = (player.position.x < frame.position.x) == tooFar ? -1 : 1;
        frame.velocity.x += sign * acceleration;
        frame.velocity.y = TruncMultiplyDivide(frame.velocity.y, 86, 100);
        ClampVectorLength(frame.velocity, LargeOpponentMaximumSpeedQ8(
            ledger.mass[AccountIndex(PhysicalAccount::OpponentShell)]));
        frame.position.x += frame.velocity.x;
        frame.position.y += frame.velocity.y;
        ClampCircleToRect(frame.position, ActorRadiusPixels(
            ledger.mass[AccountIndex(PhysicalAccount::OpponentShell)]), kPlayInterior);
        ResolveActorCoverCollisions(frame, ActorRadiusPixels(
            ledger.mass[AccountIndex(PhysicalAccount::OpponentShell)]), state.combat);
    }
    if (state.frame.doorArmed != 0) {
        const Vec2Q8 player = state.combat.actors[0].position;
        const std::int32_t centerX = state.frame.door.x + state.frame.door.width / 2;
        const std::int32_t centerY = state.frame.door.y + state.frame.door.height / 2;
        const std::int64_t dx = player.x / kQ8One - centerX;
        const std::int64_t dy = player.y / kQ8One - centerY;
        const std::uint64_t distance = static_cast<std::uint64_t>(dx*dx + dy*dy);
        if (state.frame.doorCooldown == 0 && distance <= 18u*18u
            && ledger.mass[AccountIndex(PhysicalAccount::PlayerShell)] >= 8) {
            if (!AddGeneralNeutral(state.combat, ledger, player, 8,
                    PhysicalAccount::PlayerShell)) return false;
            state.frame.doorCooldown = 1;
        } else if (state.frame.doorCooldown != 0 && distance > 30u*30u) {
            state.frame.doorCooldown = 0;
        }
    }
    return CombatStateValid(state.combat, ledger);
}

bool TickFrameAction(Act2Runtime& state) noexcept {
    Act2Runtime candidate = state;
    if (!TickFrameActionMutable(candidate)) return false;
    state = candidate;
    return true;
}

}  // namespace

const Act2EncounterDef& GetAct2EncounterDef(std::uint8_t index) noexcept {
    return kDefinitions[index < kAct2EncounterCount ? index : 0];
}

bool ValidateAct2Battlefield(const Act2EncounterDef& definition) noexcept {
    const auto circleOverlaps = [](std::int32_t x, std::int32_t y,
            std::int32_t radius, RectPixels bounds) noexcept {
        const std::int32_t closestX = x < bounds.x ? bounds.x
            : (x > bounds.x + bounds.width ? bounds.x + bounds.width : x);
        const std::int32_t closestY = y < bounds.y ? bounds.y
            : (y > bounds.y + bounds.height ? bounds.y + bounds.height : y);
        const std::int64_t dx = static_cast<std::int64_t>(x) - closestX;
        const std::int64_t dy = static_cast<std::int64_t>(y) - closestY;
        return dx*dx + dy*dy < static_cast<std::int64_t>(radius)*radius;
    };
    const auto spawnFree = [&](std::int32_t x, std::int32_t y,
            std::int32_t radius) noexcept {
        if (x-radius < kPlayInterior.x || y-radius < kPlayInterior.y
            || x+radius > kPlayInterior.x+kPlayInterior.width
            || y+radius > kPlayInterior.y+kPlayInterior.height) return false;
        for (std::uint8_t i = 0; i < definition.fieldCount; ++i) {
            if (circleOverlaps(x, y, radius, Bounds(definition.field[i]))) return false;
        }
        if (definition.id == EncounterId::Keeper) {
            for (const Act2RectMassDef& structure : kKeeperStructures) {
                if (circleOverlaps(x, y, radius, Bounds(structure))) return false;
            }
        }
        return true;
    };
    const std::int32_t playerRadius = ActorRadiusPixels(64);
    const std::int32_t opponentRadius = ActorRadiusPixels(definition.opponentMass);
    if (!spawnFree(definition.playerX, definition.playerY, playerRadius)
        || !spawnFree(definition.opponentX, definition.opponentY, opponentRadius)) return false;
    const std::int64_t spawnDx = definition.playerX-definition.opponentX;
    const std::int64_t spawnDy = definition.playerY-definition.opponentY;
    if (spawnDx*spawnDx + spawnDy*spawnDy
        < static_cast<std::int64_t>(playerRadius+opponentRadius)
            * (playerRadius+opponentRadius)) return false;

    constexpr std::int32_t kGrid = 20;
    constexpr std::int32_t kColumns = (kPlayInterior.width+kGrid-1)/kGrid;
    constexpr std::int32_t kRows = (kPlayInterior.height+kGrid-1)/kGrid;
    bool blocked[kRows][kColumns]{};
    for (std::int32_t row = 0; row < kRows; ++row) {
        for (std::int32_t column = 0; column < kColumns; ++column) {
            const std::int32_t x = kPlayInterior.x
                + (column*kGrid+kGrid/2 < kPlayInterior.width
                    ? column*kGrid+kGrid/2 : kPlayInterior.width-1);
            const std::int32_t y = kPlayInterior.y
                + (row*kGrid+kGrid/2 < kPlayInterior.height
                    ? row*kGrid+kGrid/2 : kPlayInterior.height-1);
            for (std::uint8_t i = 0; i < definition.fieldCount; ++i) {
                if (PointInside(x, y, Bounds(definition.field[i]))) blocked[row][column] = true;
            }
            if (definition.id == EncounterId::Keeper) {
                for (const Act2RectMassDef& structure : kKeeperStructures) {
                    if (PointInside(x, y, Bounds(structure))) blocked[row][column] = true;
                }
            }
        }
    }
    const auto columnFor = [](std::int32_t x) noexcept {
        const std::int32_t value = (x-kPlayInterior.x)/kGrid;
        return value < 0 ? 0 : (value >= kColumns ? kColumns-1 : value);
    };
    const auto rowFor = [](std::int32_t y) noexcept {
        const std::int32_t value = (y-kPlayInterior.y)/kGrid;
        return value < 0 ? 0 : (value >= kRows ? kRows-1 : value);
    };
    const std::int32_t startColumn = columnFor(definition.playerX);
    const std::int32_t startRow = rowFor(definition.playerY);
    const std::int32_t goalColumn = columnFor(definition.opponentX);
    const std::int32_t goalRow = rowFor(definition.opponentY);
    blocked[startRow][startColumn] = false;
    blocked[goalRow][goalColumn] = false;
    bool reached[kRows][kColumns]{};
    std::uint8_t queueRow[kRows*kColumns]{};
    std::uint8_t queueColumn[kRows*kColumns]{};
    std::uint16_t read = 0;
    std::uint16_t write = 1;
    queueRow[0] = static_cast<std::uint8_t>(startRow);
    queueColumn[0] = static_cast<std::uint8_t>(startColumn);
    reached[startRow][startColumn] = true;
    constexpr std::int8_t dx[4] = {-1,1,0,0};
    constexpr std::int8_t dy[4] = {0,0,-1,1};
    while (read < write) {
        const std::int32_t row = queueRow[read];
        const std::int32_t column = queueColumn[read++];
        for (std::uint8_t direction = 0; direction < 4; ++direction) {
            const std::int32_t nextColumn = column+dx[direction];
            const std::int32_t nextRow = row+dy[direction];
            if (nextColumn < 0 || nextColumn >= kColumns || nextRow < 0 || nextRow >= kRows
                || blocked[nextRow][nextColumn] || reached[nextRow][nextColumn]) continue;
            reached[nextRow][nextColumn] = true;
            queueRow[write] = static_cast<std::uint8_t>(nextRow);
            queueColumn[write++] = static_cast<std::uint8_t>(nextColumn);
        }
    }
    if (!reached[goalRow][goalColumn]) return false;
    const auto freeDirections = [&](std::int32_t x, std::int32_t y,
            std::int32_t radius) noexcept {
        std::uint8_t free = 0;
        for (std::uint8_t direction = 0; direction < 4; ++direction) {
            if (spawnFree(x+dx[direction]*kGrid, y+dy[direction]*kGrid, radius)) ++free;
        }
        return free >= 2;
    };
    return freeDirections(definition.playerX, definition.playerY, playerRadius)
        && freeDirections(definition.opponentX, definition.opponentY, opponentRadius);
}

bool ValidateAct2EncounterDef(const Act2EncounterDef& definition) noexcept {
    if (definition.id < EncounterId::Keeper || definition.id > EncounterId::Frame
        || definition.fieldCount == 0 || definition.fieldCount > kAct2MaximumFieldCount
        || definition.neutralCount > kAct2MaximumNeutralCount
        || definition.opponentMass < kCoreMass) return false;
    std::uint32_t fieldMass = 0;
    for (std::uint8_t index = 0; index < definition.fieldCount; ++index) {
        const Act2RectMassDef& field = definition.field[index];
        const RectPixels bounds = Bounds(field);
        if (field.mass == 0 || bounds.x < kPlayInterior.x || bounds.y < kPlayInterior.y
            || bounds.x + bounds.width > kPlayInterior.x + kPlayInterior.width
            || bounds.y + bounds.height > kPlayInterior.y + kPlayInterior.height) return false;
        fieldMass += field.mass;
    }
    std::uint32_t neutralMass = 0;
    for (std::uint8_t index = 0; index < definition.neutralCount; ++index) {
        const Act2NeutralDef& neutral = definition.neutral[index];
        if (neutral.mass == 0 || neutral.x < kPlayInterior.x || neutral.y < kPlayInterior.y
            || neutral.x >= kPlayInterior.x + kPlayInterior.width
            || neutral.y >= kPlayInterior.y + kPlayInterior.height) return false;
        neutralMass += neutral.mass;
    }
    return fieldMass == definition.fieldMass && neutralMass == definition.neutralMass
        && ValidateAct2Battlefield(definition);
}

AdapterResult ClassifyAdapterResult(std::uint16_t playerEscrow) noexcept {
    return playerEscrow <= 16 ? AdapterResult::Need
        : (playerEscrow <= 31 ? AdapterResult::Equal : AdapterResult::Output);
}

ChoiceId AdapterChoiceForEscrow(std::uint16_t playerEscrow) noexcept {
    const AdapterResult result = ClassifyAdapterResult(playerEscrow);
    return result == AdapterResult::Equal ? ChoiceId::First
        : (result == AdapterResult::Need ? ChoiceId::Second : ChoiceId::Third);
}

bool ApplyAdapterSettlementForEscrow(ClaimLedger& ledger, NarrativeState& narrative,
    std::uint16_t playerEscrow) noexcept {
    return playerEscrow <= 48
        && ApplySettlement(ledger, narrative, EncounterId::Adapter,
            AdapterChoiceForEscrow(playerEscrow));
}

std::int32_t AdapterMaximumSpeedPixelsPerSecond(std::uint16_t playerEscrow) noexcept {
    const std::int32_t groups = playerEscrow > 48 ? 6 : playerEscrow / 8;
    const std::int32_t speed = 88 - groups * 6;
    return speed < 52 ? 52 : speed;
}

std::int32_t FrameActionScore(FrameAction action, const FrameObservation& o) noexcept {
    switch (action) {
    case FrameAction::Shift: return 80 * o.incomingThreat + 40 * o.blockedChoice + 20 * o.playerNearPanel;
    case FrameAction::Toll: return 70 * o.playerNearPanel + 30 * o.playerLowShell;
    case FrameAction::Evict: return 60 * o.coverBlocks + 30 * o.playerChannel - 20 * o.selfLowStructure;
    case FrameAction::Claim: return 45 * o.lineOfSight + 35 * o.playerChannel + 20 * o.rangeFit;
    case FrameAction::Reclaim: return 55 * o.selfLowStructure + 40 * o.neutralNear - 50 * o.incomingThreat;
    case FrameAction::Move: return 30 + 40 * (1 - o.rangeFit);
    default: return -32768;
    }
}

FrameAction ChooseFrameAction(const FrameObservation& observation, std::uint8_t availableMask) noexcept {
    FrameAction best = FrameAction::Shift;
    std::int32_t bestScore = -32768;
    for (std::uint8_t index = 0; index < static_cast<std::uint8_t>(FrameAction::Count); ++index) {
        if ((availableMask & (1u << index)) == 0) continue;
        const FrameAction action = static_cast<FrameAction>(index);
        const std::int32_t score = FrameActionScore(action, observation);
        if (score > bestScore) {
            best = action;
            bestScore = score;
        }
    }
    return best;
}

bool StartSweeperWave(CombatState& combat, const PhysicalLedger& ledger,
    SweeperRuntime& sweeper) noexcept {
    if (sweeper.active != 0 || sweeper.returning != 0
        || ledger.mass[AccountIndex(PhysicalAccount::ProtocolActive)] != 32) return false;
    CombatState combatCandidate = combat;
    SweeperRuntime candidate{};
    for (std::uint8_t index = 0; index < kSweeperSegmentCount; ++index) {
        if (!AddBackedCover(combatCandidate, RectPixels{12, 34 + index * 16, 4, 8}, 4,
                PhysicalAccount::ProtocolActive, CoverBehavior::Sweeper,
                candidate.segments[index])) return false;
    }
    candidate.active = 1;
    if (!CombatStateValid(combatCandidate, ledger)) return false;
    combat = combatCandidate;
    sweeper = candidate;
    return true;
}

bool TickSweeperWave(CombatState& combat, PhysicalLedger& ledger,
    SweeperRuntime& sweeper) noexcept {
    if (sweeper.active != 0) {
        ++sweeper.waveTick;
        for (SlotHandle handle : sweeper.segments) {
            if (!CoverHandleValid(combat, handle)) continue;
            CoverSlot& segment = combat.covers[handle.index];
            segment.bounds.x = 12
                + static_cast<std::int32_t>(296u * sweeper.waveTick / 90u);
            for (NeutralSlot& neutral : combat.neutrals) {
                if (neutral.active == 0
                    || neutral.playerMissMass != 0 || neutral.opponentMissMass != 0
                    || neutral.protocolTagMass != 0 || neutral.patchFeedMass != 0
                    || neutral.adapterEscrowMass != 0 || neutral.structureMass != 0) continue;
                const std::int32_t x = neutral.position.x / kQ8One;
                const std::int32_t y = neutral.position.y / kQ8One;
                if (x >= segment.bounds.x - 2
                    && x <= segment.bounds.x + segment.bounds.width + 2
                    && y >= segment.bounds.y - 2
                    && y <= segment.bounds.y + segment.bounds.height + 2) {
                    neutral.position.x = (segment.bounds.x + segment.bounds.width + 2) * kQ8One;
                    const std::int32_t sweepVelocity = static_cast<std::int32_t>(296 * kQ8One / 90);
                    if (neutral.velocity.x < sweepVelocity) neutral.velocity.x = sweepVelocity;
                }
            }
        }
        if (sweeper.waveTick >= 90) {
            for (SlotHandle handle : sweeper.segments) RemoveCoverHandle(combat, handle);
            sweeper.active = 0;
            sweeper.returning = 1;
            sweeper.escrowInitial = ledger.mass[AccountIndex(PhysicalAccount::Transit)];
            sweeper.returnTick = 0;
        }
    } else if (sweeper.returning != 0) {
        ++sweeper.returnTick;
        const std::uint16_t targetReturned = static_cast<std::uint16_t>(
            static_cast<std::uint32_t>(sweeper.escrowInitial) * sweeper.returnTick / 36u);
        const std::uint16_t alreadyReturned = static_cast<std::uint16_t>(
            sweeper.escrowInitial - ledger.mass[AccountIndex(PhysicalAccount::Transit)]);
        if (targetReturned > alreadyReturned
            && !PhysicalTransfer(ledger, PhysicalAccount::Transit, PhysicalAccount::ProtocolActive,
                static_cast<std::uint16_t>(targetReturned - alreadyReturned))) return false;
        if (sweeper.returnTick >= 36) sweeper.returning = 0;
    }
    return CombatStateValid(combat, ledger);
}

bool StartCompactor(CombatState& combat, const PhysicalLedger& ledger,
    CompactorRuntime& compactor) noexcept {
    if (compactor.active != 0
        || ledger.mass[AccountIndex(PhysicalAccount::ProtocolActive)] != 32) return false;
    CombatState combatCandidate = combat;
    CompactorRuntime candidate{};
    if (!AddBackedCover(combatCandidate, RectPixels{152, 18, 16, 2}, 16,
            PhysicalAccount::ProtocolActive, CoverBehavior::Compactor, candidate.walls[0])
        || !AddBackedCover(combatCandidate, RectPixels{152, 164, 16, 2}, 16,
            PhysicalAccount::ProtocolActive, CoverBehavior::Compactor, candidate.walls[1])) return false;
    candidate.active = 1;
    if (!CombatStateValid(combatCandidate, ledger)) return false;
    combat = combatCandidate;
    compactor = candidate;
    return true;
}

bool TickCompactor(CombatState& combat, const PhysicalLedger& ledger,
    CompactorRuntime& compactor) noexcept {
    if (compactor.active == 0) return CombatStateValid(combat, ledger);
    ++compactor.tick;
    std::int32_t inset = 0;
    if (compactor.tick <= 180) inset = 24 * compactor.tick / 180;
    else if (compactor.tick <= 240) inset = 24;
    else if (compactor.tick <= 300) inset = 24 * (300 - compactor.tick) / 60;
    for (std::uint8_t index = 0; index < 2; ++index) {
        if (!CoverHandleValid(combat, compactor.walls[index])) continue;
        CoverSlot& wall = combat.covers[compactor.walls[index].index];
        wall.bounds.y = index == 0
            ? 18 + inset : 164 - inset;
        for (std::uint8_t side = 0; side < 2; ++side) {
            CombatActor& actor = combat.actors[side];
            const PhysicalAccount shellAccount = side == 0
                ? PhysicalAccount::PlayerShell : PhysicalAccount::OpponentShell;
            const CollisionResolution collision = ResolveCircleAgainstAabb(
                actor.position,
                ActorRadiusPixels(ledger.mass[AccountIndex(shellAccount)]),
                wall.bounds);
            if (!collision.collided) continue;
            actor.position.x += collision.shiftXQ8;
            actor.position.y += collision.shiftYQ8;
            if (collision.shiftXQ8 != 0) actor.velocity.x = 0;
            if (collision.shiftYQ8 != 0) actor.velocity.y = 0;
        }
    }
    if (compactor.tick >= 300) {
        for (SlotHandle handle : compactor.walls) RemoveCoverHandle(combat, handle);
        compactor.active = 0;
    }
    return CombatStateValid(combat, ledger);
}

bool PopulateAct2Encounter(Act2Runtime& state) noexcept {
    const Act2EncounterDef& definition = GetAct2EncounterDef(state.encounterIndex);
    if (!ValidateAct2EncounterDef(definition)) return false;
    std::uint8_t disabledPanels = 0;
    if (definition.id == EncounterId::Frame) {
        disabledPanels = state.keeper.destroyedStructures;
        const std::uint32_t keeperShift = static_cast<std::uint32_t>(EncounterId::Keeper) * 2u;
        if (state.narrative.entityStatus[static_cast<std::uint8_t>(EncounterId::Keeper)] != 0
            && ((state.narrative.choiceBits >> keeperShift) & 3u)
                == static_cast<std::uint32_t>(ChoiceId::First)) {
            disabledPanels = static_cast<std::uint8_t>(disabledPanels + 2u);
        }
        if (disabledPanels > definition.fieldCount) disabledPanels = definition.fieldCount;
    }
    const std::uint16_t convertedField = static_cast<std::uint16_t>(disabledPanels * 32u);
    if (!InitializePhysicalLedger(state.movement.physicalLedger,
            PlayerActiveMass(state.claimLedger), definition.opponentMass,
            static_cast<std::uint16_t>(definition.fieldMass - convertedField),
            static_cast<std::uint16_t>(definition.neutralMass + convertedField))) return false;
    InitializeCombatState(state.combat,
        Vec2Q8{definition.playerX * kQ8One, definition.playerY * kQ8One},
        Vec2Q8{definition.opponentX * kQ8One, definition.opponentY * kQ8One});
    state.movement.playerPosition = state.combat.actors[0].position;
    state.movement.playerVelocity = Vec2Q8{0,0};
    InitializeReclaimState(state.reclaim);
    if (definition.id == EncounterId::Frame) state.frame = FrameRuntime{};
    for (std::uint8_t index = disabledPanels; index < definition.fieldCount; ++index) {
        SlotHandle handle{};
        if (!AddBackedCover(state.combat, Bounds(definition.field[index]),
                definition.field[index].mass, PhysicalAccount::Cover,
                definition.id == EncounterId::Frame ? CoverBehavior::OwnedStructure
                    : CoverBehavior::Standard,
                handle)) return false;
        if (definition.id == EncounterId::Frame) state.frame.panels[index] = handle;
    }
    for (std::uint8_t index = 0; index < definition.neutralCount; ++index) {
        const Act2NeutralDef& neutral = definition.neutral[index];
        if (!AddNeutral(state.combat, Vec2Q8{neutral.x * kQ8One, neutral.y * kQ8One},
                neutral.mass, 0, 0, state.neutrals[index])) return false;
        if (neutral.escrow != 0) {
            state.combat.neutrals[state.neutrals[index].index].adapterEscrowMass = neutral.mass;
        }
    }
    for (std::uint8_t index = 0; index < disabledPanels; ++index) {
        SlotHandle handle{};
        const Act2RectMassDef& panel = definition.field[index];
        if (!AddNeutral(state.combat,
                Vec2Q8{panel.centerX * kQ8One, panel.centerY * kQ8One},
                panel.mass, 0, 0, handle)) return false;
    }
    if (definition.id == EncounterId::Keeper) {
        state.keeper = KeeperRuntime{};
        for (std::uint8_t index = 0; index < kKeeperStructureCount; ++index) {
            if (!AddBackedCover(state.combat, Bounds(kKeeperStructures[index]), 16,
                    PhysicalAccount::OpponentShell, CoverBehavior::OwnedStructure,
                    state.keeper.structures[index])) return false;
        }
    } else if (definition.id == EncounterId::Adapter) {
        state.adapter = AdapterRuntime{};
    } else {
        state.frame.action = FrameAction::Move;
        state.frame.actionCooldown = 0;
        state.frame.structureControlMass = 128;
        for (std::uint8_t index = disabledPanels; index < definition.fieldCount; ++index) {
            if (CoverHandleValid(state.combat, state.frame.panels[index])) {
                state.frame.panelAliveMask = static_cast<std::uint16_t>(
                    state.frame.panelAliveMask | (1u << index));
            }
        }
    }
    return CombatStateValid(state.combat, state.movement.physicalLedger);
}

bool InitializeAct2Runtime(Act2Runtime& state, const ClaimLedger& claims,
    const NarrativeState& narrative) noexcept {
    if (!ClaimLedgerValid(claims)) return false;
    state = Act2Runtime{};
    state.claimLedger = claims;
    state.narrative = narrative;
    if (!InitializeGameState(state.movement, 0xA2C70001u, narrative.choiceBits)) return false;
    state.movement.claimLedger = claims;
    state.movement.narrative = narrative;
    InitializeHudLoanSystem(state.hudLoan);
    state.scene = Act2Scene::EncounterIntro;
    state.focusedChoice = ChoiceId::Second;
    return true;
}

bool TickAct2Runtime(Act2Runtime& state, const InputFrame& input) noexcept {
    if (state.scene == Act2Scene::Fatal || state.scene == Act2Scene::Complete) {
        return state.scene != Act2Scene::Fatal;
    }
    ++state.globalTick;
    if (state.scene == Act2Scene::EncounterIntro) {
        ++state.sceneTicks;
        const std::uint16_t minimum = state.introRevisit != 0
            ? kEncounterRevisitIntroMinimumTicks : kEncounterIntroMinimumTicks;
        if (state.sceneTicks >= kEncounterIntroAutomaticTicks
            || (state.sceneTicks >= minimum && Pressed(input, PlatformAction::Confirm))) {
            state.scene = Act2Scene::ObjectiveCard;
            state.sceneTicks = 0;
        }
        return true;
    }
    if (state.scene == Act2Scene::ObjectiveCard) {
        if (++state.sceneTicks >= kObjectiveCardTicks) return StartEncounterPlaying(state);
        return true;
    }
    if (state.scene == Act2Scene::HitFreeze) {
        if (++state.sceneTicks >= kHitFreezeTicks) {
            state.scene = state.retryPending != 0
                ? Act2Scene::EncounterRetry : Act2Scene::SettlementWait;
            state.sceneTicks = 0;
        }
        return true;
    }
    if (state.scene == Act2Scene::EncounterRetry) {
        state.introRevisit = 1;
        state.scene = Act2Scene::EncounterIntro;
        state.sceneTicks = 0;
        return true;
    }
    if (state.scene == Act2Scene::SettlementWait) {
        if (++state.sceneTicks >= kSettlementWaitTicks) {
            state.scene = Act2Scene::Settlement;
            state.sceneTicks = 0;
        }
        return true;
    }
    if (state.scene == Act2Scene::Settlement) {
        ChoiceId requested = state.focusedChoice;
        if (Pressed(input, PlatformAction::SelectLeft)) requested = ChoiceId::First;
        if (Pressed(input, PlatformAction::SelectUp)) requested = ChoiceId::Second;
        if (Pressed(input, PlatformAction::SelectRight)) requested = ChoiceId::Third;
        if (requested != state.focusedChoice) {
            state.focusedChoice = requested;
            state.scene = Act2Scene::Preview;
            state.sceneTicks = 0;
        } else if (Held(input, PlatformAction::Confirm)) {
            state.scene = Act2Scene::ConfirmHold;
            state.sceneTicks = 0;
        }
        return true;
    }
    if (state.scene == Act2Scene::Preview) {
        if (++state.sceneTicks >= kAllocationPreviewTicks) {
            state.scene = Act2Scene::Settlement;
            state.sceneTicks = 0;
        }
        return true;
    }
    if (state.scene == Act2Scene::ConfirmHold) {
        if (!Held(input, PlatformAction::Confirm)) {
            state.scene = Act2Scene::Settlement;
            state.sceneTicks = 0;
        } else if (++state.sceneTicks >= kConfirmHoldTicks) {
            state.committedChoice = state.focusedChoice;
            state.scene = Act2Scene::CommitTransit;
            state.sceneTicks = 0;
        }
        return true;
    }
    if (state.scene == Act2Scene::CommitTransit) {
        if (++state.sceneTicks >= kCommitTransitTicks) {
            if (!ApplyAct2Choice(state)) {
                state.scene = Act2Scene::Fatal;
                return false;
            }
            state.scene = Act2Scene::EntityResponse;
            state.sceneTicks = 0;
        }
        return true;
    }
    if (state.scene == Act2Scene::EntityResponse) {
        ++state.sceneTicks;
        if (state.sceneTicks >= kEntityResponseAutomaticTicks
            || (state.sceneTicks >= kEntityResponseMinimumTicks
                && Pressed(input, PlatformAction::Confirm))) {
            state.scene = Act2Scene::LedgerReturn;
            state.sceneTicks = 0;
        }
        return true;
    }
    if (state.scene == Act2Scene::LedgerReturn) {
        if (++state.sceneTicks >= kLedgerReturnTicks) {
            ++state.encounterIndex;
            if (state.encounterIndex >= kAct2EncounterCount) state.scene = Act2Scene::Complete;
            else {
                state.scene = Act2Scene::EncounterIntro;
                state.sceneTicks = 0;
            }
        }
        return true;
    }

    PhysicalLedger& ledger = state.movement.physicalLedger;
    if (state.movement.paused != 0) {
        if (input.focused != 0 && Pressed(input, PlatformAction::Pause)) {
            state.movement.paused = 0;
            ClearInterruptedActions(state);
        }
        return true;
    }
    if (state.movement.dashCooldown > 0) --state.movement.dashCooldown;
    if (state.movement.dashInvulnerableRemaining > 0) --state.movement.dashInvulnerableRemaining;
    TickCombatCountdowns(state.combat);
    if (input.focused == 0 || Pressed(input, PlatformAction::Pause)) {
        state.movement.paused = 1;
        ClearInterruptedActions(state);
        return true;
    }

    InputFrame effectiveInput = input;
    if (state.movement.dashTicksRemaining > 0) {
        ClearAction(effectiveInput, PlatformAction::HudReallocate);
        ClearAction(effectiveInput, PlatformAction::Dash);
        ClearAction(effectiveInput, PlatformAction::Fire);
        ClearAction(effectiveInput, PlatformAction::Reclaim);
    }
    state.movement.claimLedger = state.claimLedger;
    state.movement.narrative = state.narrative;
    state.movement.impulseInstalled = HudModuleFullyInstalled(
        state.hudLoan, HudModuleId::Impulse) ? 1u : 0u;
    const GameplayIntent intent = ResolveGameplayInput(state.movement.inputArbiter,
        effectiveInput, CanStartDash(state), true);
    if (!AdvanceHudLoanEarly(state.hudLoan, ledger,
            intent.action == GameplayAction::HudReallocate,
            state.previousPlayerReclaim)) return false;
    state.previousPlayerReclaim = 0;
    if (Pressed(input, PlatformAction::HudCycle)) {
        const std::uint8_t next = static_cast<std::uint8_t>(
            (static_cast<std::uint8_t>(state.hudLoan.selected) + 1u) % kHudModuleCount);
        SelectHudModule(state.hudLoan, static_cast<HudModuleId>(next));
    }
    ++state.encounterTick;
    const Vec2Q8 aim = AimDirection(state, input);
    state.combat.actors[0].aimDirectionQ15 = aim;
    ChargeSlot& charge = state.combat.charges[0];
    if (intent.action == GameplayAction::HudReallocate && charge.active != 0) {
        if (!RollbackCharge(state.combat, ledger, CombatSide::Player)) return false;
    } else if (intent.action == GameplayAction::Fire && !HudFireLocked(state.hudLoan)) {
        if (charge.active == 0) {
            if (state.combat.actors[0].shotCooldownTicks == 0
                && ledger.mass[AccountIndex(PhysicalAccount::PlayerShell)] >= 8
                && !BeginCharge(state.combat, ledger, CombatSide::Player, aim)) return false;
        } else if (!AdvanceCharge(state.combat, ledger, CombatSide::Player)) return false;
    } else if (charge.active != 0) {
        if (Held(input, PlatformAction::Fire)
            || input.actions[ActionIndex(PlatformAction::Fire)].released != 0) {
            if (!ReleaseCharge(state.combat, ledger, CombatSide::Player)) return false;
        } else if (!RollbackCharge(state.combat, ledger, CombatSide::Player)) return false;
    }
    state.movement.chargeActive = charge.active != 0 ? 1u : 0u;
    if (!AdvanceGameIntent(state.movement, intent)) return false;
    state.combat.actors[0].position = state.movement.playerPosition;
    state.combat.actors[0].velocity = state.movement.playerVelocity;
    const std::int32_t playerRadius = ActorRadiusPixels(
        ledger.mass[AccountIndex(PhysicalAccount::PlayerShell)]);
    const bool playerHitCover = ResolveActorCoverCollisions(
        state.combat.actors[0], playerRadius, state.combat);
    if (playerHitCover && state.movement.dashTicksRemaining > 0) {
        state.movement.dashTicksRemaining = 0;
        state.movement.dashInvulnerableRemaining = 0;
        state.combat.actors[0].velocity = Vec2Q8{0,0};
    }
    if (state.encounterIndex == 1 && state.adapter.playerDoorOpen == 0
        && state.combat.actors[0].position.x > (284-playerRadius)*kQ8One) {
        state.combat.actors[0].position.x = (284-playerRadius)*kQ8One;
        state.combat.actors[0].velocity.x = 0;
    }
    state.movement.playerPosition = state.combat.actors[0].position;
    state.movement.playerVelocity = state.combat.actors[0].velocity;

    if (state.encounterIndex == 0) {
        const std::uint16_t waveStarts[3] = {0, 1800, 3600};
        if (state.keeper.wavesCompleted < 3
            && state.encounterTick == static_cast<std::uint32_t>(waveStarts[state.keeper.wavesCompleted] + 1)
            && !StartSweeperWave(state.combat, ledger, state.keeper.sweeper)) return false;
        if (!TickSweeperWave(state.combat, ledger, state.keeper.sweeper)) return false;
    } else if (state.encounterIndex == 1) {
        const std::uint32_t doorRate = 1u + state.adapter.playerEscrow / 8u;
        state.adapter.playerDoorProgress += doorRate;
        if (state.adapter.playerDoorProgress >= kAdapterDurationTicks) {
            state.adapter.playerDoorProgress = kAdapterDurationTicks;
            state.adapter.playerDoorOpen = 1;
        }
        if (state.encounterTick == kCompactorStartTick
            && !StartCompactor(state.combat, ledger, state.adapter.compactor)) return false;
        if (!TickCompactor(state.combat, ledger, state.adapter.compactor)) return false;
        const std::int32_t speed = PixelsPerSecondToQ8PerTick(
            AdapterMaximumSpeedPixelsPerSecond(state.adapter.playerEscrow));
        if (state.combat.actors[1].position.x > 34 * kQ8One) {
            CombatActor& adapter = state.combat.actors[1];
            constexpr std::int32_t acceleration = PixelsPerSecondSquaredToQ8PerTickSquared(500);
            adapter.velocity.x -= acceleration;
            adapter.velocity.y = TruncMultiplyDivide(adapter.velocity.y, 86, 100);
            ClampVectorLength(adapter.velocity, speed);
            adapter.position.x += adapter.velocity.x;
            adapter.position.y += adapter.velocity.y;
            const CollisionResolution boundary = ClampCircleToRect(adapter.position,
                ActorRadiusPixels(ledger.mass[AccountIndex(PhysicalAccount::OpponentShell)]),
                kPlayInterior);
            if (boundary.shiftXQ8 != 0) adapter.velocity.x = 0;
            if (boundary.shiftYQ8 != 0) adapter.velocity.y = 0;
            ResolveActorCoverCollisions(adapter,
                ActorRadiusPixels(ledger.mass[AccountIndex(PhysicalAccount::OpponentShell)]),
                state.combat);
            if (adapter.position.x <= 34 * kQ8One) state.adapter.adapterArrived = 1;
        }
    } else if (state.encounterTick % kFrameDecisionTicks == 0
        && state.frame.actionCooldown == 0) {
        const FrameObservation observation = ObserveFrame(state);
        std::uint8_t available = 1u << static_cast<std::uint8_t>(FrameAction::Move);
        const std::uint16_t closestPanel = ClosestPanel(state);
        if (closestPanel != kInvalidSlotIndex) {
            available = static_cast<std::uint8_t>(available
                | (1u << static_cast<std::uint8_t>(FrameAction::Shift))
                | (state.frame.structureControlMass >= 16
                    && PlayerNearPanel(state, closestPanel, 18)
                    ? (1u << static_cast<std::uint8_t>(FrameAction::Toll)) : 0)
                | (state.frame.structureControlMass >= 32
                    ? (1u << static_cast<std::uint8_t>(FrameAction::Evict)) : 0));
        }
        if (ledger.mass[AccountIndex(PhysicalAccount::OpponentShell)] >= 32) {
            available = static_cast<std::uint8_t>(available
                | (1u << static_cast<std::uint8_t>(FrameAction::Claim)));
        }
        const std::uint16_t frameShellCap = static_cast<std::uint16_t>(48 + state.frame.structureControlMass);
        if (ledger.mass[AccountIndex(PhysicalAccount::Neutral)] != 0
            && ledger.mass[AccountIndex(PhysicalAccount::OpponentShell)] < frameShellCap) {
            available = static_cast<std::uint8_t>(available
                | (1u << static_cast<std::uint8_t>(FrameAction::Reclaim)));
        }
        const FrameAction selected = ChooseFrameAction(observation, available);
        if (!BeginFrameAction(state, selected)) {
            state.frame.action = FrameAction::Move;
            state.frame.actionCooldown = 30;
        }
    }

    if (state.encounterIndex == 2 && !TickFrameAction(state)) return false;

    if (!StepCombatProjectiles(state.combat, ledger, state.claimLedger)) return false;
    if (state.encounterIndex == 2) {
        for (std::uint8_t index = 0; index < kAct2MaximumFieldCount; ++index) {
            const std::uint16_t bit = static_cast<std::uint16_t>(1u << index);
            if ((state.frame.panelAliveMask & bit) != 0
                && !CoverHandleValid(state.combat, state.frame.panels[index])) {
                state.frame.panelAliveMask = static_cast<std::uint16_t>(state.frame.panelAliveMask & ~bit);
                if (state.frame.structureControlMass >= 32) {
                    state.frame.structureControlMass = static_cast<std::uint16_t>(
                        state.frame.structureControlMass - 32);
                }
            }
        }
        const std::uint16_t shellCap = static_cast<std::uint16_t>(48 + state.frame.structureControlMass);
        const std::uint16_t shell = ledger.mass[AccountIndex(PhysicalAccount::OpponentShell)];
        if (shell > shellCap
            && !AddGeneralNeutral(state.combat, ledger, state.combat.actors[1].position,
                static_cast<std::uint16_t>(shell - shellCap), PhysicalAccount::OpponentShell)) return false;
    }
    if (state.combat.actors[0].coreHit != 0) {
        ++state.failure;
        ++state.movement.globalTick;
        BeginRetry(state);
        return true;
    }
    if (state.combat.actors[1].coreHit != 0 && state.encounterIndex == 2) {
        BeginSuccess(state);
        return state.scene != Act2Scene::Fatal;
    }
    if (!TickNeutralDynamics(state.combat, ledger)) return false;
    ReclaimTickResult reclaimed{};
    const bool frameReclaim = state.encounterIndex == 2
        && state.frame.action == FrameAction::Reclaim
        && ledger.mass[AccountIndex(PhysicalAccount::OpponentShell)]
            < static_cast<std::uint16_t>(48 + state.frame.structureControlMass);
    if (!TickReclaim(state.combat, ledger, state.reclaim,
            intent.action == GameplayAction::Reclaim,
            frameReclaim,
            reclaimed, state.encounterIndex == 2)) return false;
    state.previousPlayerReclaim = reclaimed.playerMass;
    state.adapter.playerEscrow = static_cast<std::uint16_t>(
        state.adapter.playerEscrow + reclaimed.playerAdapterEscrowMass);
    if (!MergeNeutralClusters(state.combat, ledger)) return false;
    const bool playerShellHit = state.combat.shellDamageThisTick[
        SideIndex(CombatSide::Player)] != 0;
    if (!ResolvePendingHudCommit(state.hudLoan, ledger, playerShellHit)) return false;

    if (state.encounterIndex == 0) {
        std::uint8_t destroyed = 0;
        for (SlotHandle handle : state.keeper.structures) {
            if (!CoverHandleValid(state.combat, handle)) ++destroyed;
        }
        state.keeper.destroyedStructures = destroyed;
        std::uint8_t completed = 0;
        if (state.encounterTick >= 90) completed = 1;
        if (state.encounterTick >= 1890) completed = 2;
        if (state.encounterTick >= 3690) completed = 3;
        state.keeper.wavesCompleted = completed;
        if (destroyed == kKeeperStructureCount || state.encounterTick >= kKeeperDurationTicks) BeginSuccess(state);
    } else if (state.encounterIndex == 1) {
        if ((state.adapter.playerDoorOpen != 0
                && state.combat.actors[0].position.x >= 286 * kQ8One)
            || state.encounterTick >= kAdapterDurationTicks) BeginSuccess(state);
    }
    ++state.movement.globalTick;
    return PhysicalLedgerValid(ledger) && ClaimLedgerValid(state.claimLedger)
        && CombatStateValid(state.combat, ledger)
        && HudLoanSystemValid(state.hudLoan, ledger);
}

void RenderAct2Runtime(PixelBuffer buffer, const Act2Runtime& state) noexcept {
    if (state.scene == Act2Scene::Playing || state.scene == Act2Scene::HitFreeze
        || state.scene == Act2Scene::EncounterRetry
        || state.scene == Act2Scene::SettlementWait) {
        RenderBattle(buffer, state);
        return;
    }
    Clear(buffer, PaletteColor::Background);
    const Act2EncounterDef& definition = GetAct2EncounterDef(state.encounterIndex);
    switch (state.scene) {
    case Act2Scene::EncounterIntro:
        DrawCentered(buffer, 70, definition.introLine1, PaletteColor::Neutral);
        DrawCentered(buffer, 84, definition.introLine2, PaletteColor::Neutral);
        break;
    case Act2Scene::ObjectiveCard:
        DrawCentered(buffer, 76, definition.name, PaletteColor::Opponent);
        DrawCentered(buffer, 92, definition.objective, PaletteColor::Warning);
        break;
    case Act2Scene::Settlement:
    case Act2Scene::Preview:
    case Act2Scene::ConfirmHold:
    case Act2Scene::CommitTransit:
        DrawCentered(buffer, 50, definition.name, PaletteColor::Opponent);
        if (state.encounterIndex == 0) {
            DrawCentered(buffer, 80, "PRIVATIZE   MAINTAIN   MAKE COMMON", PaletteColor::Neutral);
        } else {
            DrawCentered(buffer, 80, "TAKE CONTROL   RESTORE FRAME   OPEN THE FIELD", PaletteColor::Neutral);
        }
        break;
    case Act2Scene::EntityResponse:
        if (state.encounterIndex == 1) {
            const AdapterResult result = ClassifyAdapterResult(state.adapter.playerEscrow);
            DrawCentered(buffer, 84, result == AdapterResult::Need ? "GIVE BY NEED"
                : (result == AdapterResult::Equal ? "DIVIDE EQUALLY" : "GIVE BY OUTPUT"),
                PaletteColor::Warning);
        } else DrawCentered(buffer, 84, "THE LEDGER REMEMBERS", PaletteColor::Warning);
        break;
    case Act2Scene::LedgerReturn: DrawCentered(buffer, 84, "RETURN TO LEDGER", PaletteColor::Neutral); break;
    case Act2Scene::Complete: DrawCentered(buffer, 84, "ACT 2 COMPLETE", PaletteColor::Warning); break;
    case Act2Scene::Fatal: DrawCentered(buffer, 84, "LEDGER ERROR", PaletteColor::Warning); break;
    default: break;
    }
}

std::uint32_t Act2RuntimeHash(const Act2Runtime& state) noexcept {
    std::uint32_t hash = 2166136261u;
    const auto add = [&hash](std::uint32_t value) noexcept {
        for (std::uint8_t i = 0; i < 4; ++i) {
            hash ^= static_cast<std::uint8_t>(value >> (i*8));
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
    const auto addRect = [&add](RectPixels rect) noexcept {
        add(static_cast<std::uint32_t>(rect.x)); add(static_cast<std::uint32_t>(rect.y));
        add(static_cast<std::uint32_t>(rect.width)); add(static_cast<std::uint32_t>(rect.height));
    };

    for (std::uint16_t value : state.claimLedger.mass) add(value);
    add(state.narrative.choiceBits); add(state.narrative.adjudicatedMass);
    add(state.narrative.encounterIndex); add(state.narrative.role);
    for (std::uint8_t value : state.narrative.entityStatus) add(value);
    add(state.narrative.endingsSeen); add(GameStateHash(state.movement));
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
        add(neutral.adapterEscrowMass); add(neutral.structureMass);
        add(neutral.inertiaTicks); add(neutral.active);
    }
    for (const CoverSlot& cover : state.combat.covers) {
        addRect(cover.bounds); add(cover.stableId); add(cover.generation); add(cover.mass);
        add(static_cast<std::uint32_t>(cover.backingAccount));
        add(static_cast<std::uint32_t>(cover.behavior)); add(cover.active);
    }
    for (const CombatActor& actor : state.combat.actors) {
        addVec(actor.position); addVec(actor.velocity); addVec(actor.aimDirectionQ15);
        add(actor.exposureGraceTicks); add(actor.shotCooldownTicks);
        add(actor.projectileImmune); add(actor.coreHit);
    }
    for (std::uint16_t value : state.combat.settlement.physicalLedger.mass) add(value);
    for (std::uint16_t value : state.combat.settlement.claimLedger.mass) add(value);
    add(state.combat.settlement.tick);
    add(static_cast<std::uint32_t>(state.combat.settlement.hitSide));
    add(state.combat.settlement.valid);
    add(state.combat.shellDamageThisTick[0]); add(state.combat.shellDamageThisTick[1]);
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
    for (SlotHandle handle : state.neutrals) addHandle(handle);
    for (SlotHandle handle : state.keeper.structures) addHandle(handle);
    for (SlotHandle handle : state.keeper.sweeper.segments) addHandle(handle);
    add(state.keeper.sweeper.waveTick); add(state.keeper.sweeper.returnTick);
    add(state.keeper.sweeper.escrowInitial); add(state.keeper.sweeper.active);
    add(state.keeper.sweeper.returning); add(state.keeper.destroyedStructures);
    add(state.keeper.wavesCompleted);
    for (SlotHandle handle : state.adapter.compactor.walls) addHandle(handle);
    add(state.adapter.compactor.tick); add(state.adapter.compactor.active);
    add(state.adapter.playerDoorProgress); add(state.adapter.playerEscrow);
    add(state.adapter.adapterArrived); add(state.adapter.playerDoorOpen);
    for (SlotHandle handle : state.frame.panels) addHandle(handle);
    addRect(state.frame.shiftStart); addHandle(state.frame.shiftPanel); addRect(state.frame.door);
    add(state.frame.actionCooldown); add(state.frame.actionTicks);
    add(state.frame.structureControlMass); add(state.frame.doorCooldown);
    add(state.frame.panelAliveMask); add(static_cast<std::uint32_t>(state.frame.action));
    add(static_cast<std::uint32_t>(state.frame.shiftX));
    add(static_cast<std::uint32_t>(state.frame.shiftY)); add(state.frame.doorArmed);
    add(static_cast<std::uint32_t>(state.scene));
    add(static_cast<std::uint32_t>(state.focusedChoice));
    add(static_cast<std::uint32_t>(state.committedChoice));
    add(state.globalTick); add(state.encounterTick); add(state.sceneTicks);
    add(state.previousPlayerReclaim); add(state.encounterIndex); add(state.failure);
    add(state.retryPending); add(state.introRevisit);
    return hash;
}

#if defined(BP_TEST)
bool ForceAct2EncounterSuccessForTest(Act2Runtime& state) noexcept {
    if (state.scene != Act2Scene::Playing || state.encounterIndex >= kAct2EncounterCount) return false;
    if (state.encounterIndex == 0) state.encounterTick = kKeeperDurationTicks;
    else if (state.encounterIndex == 1) state.encounterTick = kAdapterDurationTicks;
    else state.combat.actors[1].coreHit = 1;
    BeginSuccess(state);
    return state.scene != Act2Scene::Fatal;
}

bool ForceFrameActionForTest(Act2Runtime& state, FrameAction action) noexcept {
    return state.encounterIndex == 2 && state.scene == Act2Scene::Playing
        && BeginFrameAction(state, action);
}

bool TickFrameActionForTest(Act2Runtime& state) noexcept {
    return state.encounterIndex == 2 && TickFrameAction(state);
}

FrameObservation ObserveFrameForTest(const Act2Runtime& state) noexcept {
    return ObserveFrame(state);
}
#endif

}  // namespace bp
