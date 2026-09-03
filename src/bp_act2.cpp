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
    const CombatActor& player = state.combat.actors[0];
    const CombatActor& opponent = state.combat.actors[1];
    FillRect(buffer, RectI{player.position.x / kQ8One - 2,
        player.position.y / kQ8One - 2, 5, 5}, PaletteColor::Core);
    FillRect(buffer, RectI{opponent.position.x / kQ8One - 2,
        opponent.position.y / kQ8One - 2, 5, 5}, PaletteColor::Opponent);
    DrawLatin(buffer, 14, 6, kDefinitions[state.encounterIndex].name, PaletteColor::Warning);
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
    return true;
}

void BeginSuccess(Act2Runtime& state) noexcept {
    state.scene = state.encounterIndex == 1 ? Act2Scene::EntityResponse : Act2Scene::Settlement;
    state.sceneTicks = 0;
    state.focusedChoice = ChoiceId::Second;
    if (state.encounterIndex == 1) {
        const ChoiceId choice = AdapterChoiceForEscrow(state.adapter.playerEscrow);
        if (!ApplyAdapterSettlementForEscrow(
                state.claimLedger, state.narrative, state.adapter.playerEscrow)) {
            state.scene = Act2Scene::Fatal;
        } else {
            state.narrative.encounterIndex = 5;
            state.narrative.entityStatus[static_cast<std::uint8_t>(EncounterId::Adapter)] =
                static_cast<std::uint8_t>(choice) + 1u;
        }
    }
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

bool ShiftRouteClear(const CombatState& combat, std::uint16_t panelIndex,
    std::int8_t dx, std::int8_t dy) noexcept {
    RectPixels destination = combat.covers[panelIndex].bounds;
    destination.x += dx;
    destination.y += dy;
    if (destination.x < kPlayInterior.x || destination.y < kPlayInterior.y
        || destination.x + destination.width > kPlayInterior.x + kPlayInterior.width
        || destination.y + destination.height > kPlayInterior.y + kPlayInterior.height) return false;
    for (std::uint16_t index = 0; index < kCoverCapacity; ++index) {
        if (index != panelIndex && combat.covers[index].active != 0
            && Overlap(destination, combat.covers[index].bounds)) return false;
    }
    return true;
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

bool AddGeneralNeutral(CombatState& combat, PhysicalLedger& ledger,
    Vec2Q8 position, std::uint16_t mass, PhysicalAccount from) noexcept {
    SlotHandle handle{};
    return AddNeutral(combat, position, mass, 0, 0, handle)
        && PhysicalTransfer(ledger, from, PhysicalAccount::Neutral, mass);
}

bool BeginFrameActionMutable(Act2Runtime& state, FrameAction action) noexcept {
    PhysicalLedger& ledger = state.movement.physicalLedger;
    const std::uint16_t panelIndex = ClosestPanel(state);
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
        if (panelIndex == kInvalidSlotIndex || state.frame.structureControlMass < 16) return false;
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
            || ledger.mass[AccountIndex(PhysicalAccount::OpponentShell)] < 32) return false;
        CoverSlot& panel = state.combat.covers[panelIndex];
        const Vec2Q8 center{(panel.bounds.x + panel.bounds.width / 2) * kQ8One,
            (panel.bounds.y + panel.bounds.height / 2) * kQ8One};
        if (!AddGeneralNeutral(state.combat, ledger, center, panel.mass, PhysicalAccount::Cover)) return false;
        panel.mass = 0;
        panel.active = 0;
        for (std::uint8_t index = 0; index < kAct2MaximumFieldCount; ++index) {
            if (state.frame.panels[index].index == panelIndex) {
                state.frame.panelAliveMask = static_cast<std::uint16_t>(
                    state.frame.panelAliveMask & ~(1u << index));
                break;
            }
        }
        const Vec2Q8 player = state.combat.actors[0].position;
        if (!SpawnScriptedProjectile(state.combat, ledger, CombatSide::Opponent, 32,
                state.combat.actors[1].position,
                Vec2Q8{player.x - state.combat.actors[1].position.x,
                    player.y - state.combat.actors[1].position.y},
                PixelsPerSecondToQ8PerTick(105))) return false;
        state.frame.structureControlMass = static_cast<std::uint16_t>(
            state.frame.structureControlMass - 32);
        state.frame.actionCooldown = 120;
    } else if (action == FrameAction::Claim) {
        if (ledger.mass[AccountIndex(PhysicalAccount::OpponentShell)] < 32) return false;
        const Vec2Q8 player = state.combat.actors[0].position;
        if (!SpawnScriptedProjectile(state.combat, ledger, CombatSide::Opponent, 32,
                state.combat.actors[1].position,
                Vec2Q8{player.x - state.combat.actors[1].position.x,
                    player.y - state.combat.actors[1].position.y},
                ProjectileSpeedQ8(32))) return false;
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
    return fieldMass == definition.fieldMass && neutralMass == definition.neutralMass;
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
            combat.covers[handle.index].bounds.x = 12
                + static_cast<std::int32_t>(296u * sweeper.waveTick / 90u);
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
        for (CombatActor& actor : combat.actors) {
            const std::int32_t x = actor.position.x / kQ8One;
            const std::int32_t y = actor.position.y / kQ8One;
            if (x >= wall.bounds.x && x < wall.bounds.x + wall.bounds.width
                && y >= wall.bounds.y - 3 && y < wall.bounds.y + wall.bounds.height + 3) {
                actor.position.y = (index == 0
                    ? wall.bounds.y + wall.bounds.height + 3
                    : wall.bounds.y - 3) * kQ8One;
                actor.velocity.y = 0;
            }
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
    if (state.scene == Act2Scene::Fatal || state.scene == Act2Scene::Complete) return state.scene != Act2Scene::Fatal;
    ++state.sceneTicks;
    ++state.globalTick;
    if (state.scene == Act2Scene::EncounterIntro) {
        if (state.sceneTicks >= 180 || (state.sceneTicks >= 90 && Pressed(input, PlatformAction::Confirm))) {
            state.scene = Act2Scene::ObjectiveCard;
            state.sceneTicks = 0;
        }
        return true;
    }
    if (state.scene == Act2Scene::ObjectiveCard) {
        if (state.sceneTicks >= 90) return StartEncounterPlaying(state);
        return true;
    }
    if (state.scene == Act2Scene::Settlement) {
        if (Pressed(input, PlatformAction::SelectLeft)) state.focusedChoice = ChoiceId::First;
        if (Pressed(input, PlatformAction::SelectUp)) state.focusedChoice = ChoiceId::Second;
        if (Pressed(input, PlatformAction::SelectRight)) state.focusedChoice = ChoiceId::Third;
        if (Held(input, PlatformAction::Confirm)) {
            state.scene = Act2Scene::ConfirmHold;
            state.sceneTicks = 0;
        }
        return true;
    }
    if (state.scene == Act2Scene::ConfirmHold) {
        if (!Held(input, PlatformAction::Confirm)) {
            state.scene = Act2Scene::Settlement;
            state.sceneTicks = 0;
        } else if (state.sceneTicks >= 30) {
            const EncounterId encounter = GetAct2EncounterDef(state.encounterIndex).id;
            if (!ApplySettlement(state.claimLedger, state.narrative, encounter, state.focusedChoice)) {
                state.scene = Act2Scene::Fatal;
                return false;
            }
            state.narrative.encounterIndex = static_cast<std::uint8_t>(encounter) + 1u;
            state.narrative.entityStatus[static_cast<std::uint8_t>(encounter)] =
                static_cast<std::uint8_t>(state.focusedChoice) + 1u;
            state.scene = Act2Scene::EntityResponse;
            state.sceneTicks = 0;
        }
        return true;
    }
    if (state.scene == Act2Scene::EntityResponse) {
        if (state.sceneTicks >= 180 || (state.sceneTicks >= 90 && Pressed(input, PlatformAction::Confirm))) {
            state.scene = Act2Scene::LedgerReturn;
            state.sceneTicks = 0;
        }
        return true;
    }
    if (state.scene == Act2Scene::LedgerReturn) {
        if (state.sceneTicks >= 36) {
            ++state.encounterIndex;
            if (state.encounterIndex >= kAct2EncounterCount) state.scene = Act2Scene::Complete;
            else {
                state.scene = Act2Scene::EncounterIntro;
                state.sceneTicks = 0;
            }
        }
        return true;
    }

    ++state.encounterTick;
    PhysicalLedger& ledger = state.movement.physicalLedger;
    TickCombatCountdowns(state.combat);
    state.movement.claimLedger = state.claimLedger;
    state.movement.narrative = state.narrative;
    const GameplayIntent intent = ResolveGameplayInput(state.movement.inputArbiter, input,
        state.movement.dashCooldown == 0, true);
    const Vec2Q8 aim = AimDirection(state, input);
    state.combat.actors[0].aimDirectionQ15 = aim;
    ChargeSlot& charge = state.combat.charges[0];
    if (intent.action == GameplayAction::Fire && !HudFireLocked(state.hudLoan)) {
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

    if (state.encounterIndex == 0) {
        const std::uint16_t waveStarts[3] = {0, 1800, 3600};
        if (state.keeper.wavesCompleted < 3
            && state.encounterTick == static_cast<std::uint32_t>(waveStarts[state.keeper.wavesCompleted] + 1)
            && !StartSweeperWave(state.combat, ledger, state.keeper.sweeper)) return false;
        if (!TickSweeperWave(state.combat, ledger, state.keeper.sweeper)) return false;
    } else if (state.encounterIndex == 1) {
        if (state.encounterTick == kCompactorStartTick
            && !StartCompactor(state.combat, ledger, state.adapter.compactor)) return false;
        if (!TickCompactor(state.combat, ledger, state.adapter.compactor)) return false;
        const std::int32_t speed = PixelsPerSecondToQ8PerTick(
            AdapterMaximumSpeedPixelsPerSecond(state.adapter.playerEscrow));
        if (state.combat.actors[1].position.x > 34 * kQ8One) {
            state.combat.actors[1].position.x -= speed;
            if (state.combat.actors[1].position.x <= 34 * kQ8One) state.adapter.adapterArrived = 1;
        }
    } else if (state.encounterTick % kFrameDecisionTicks == 0
        && state.frame.actionCooldown == 0) {
        FrameObservation observation{};
        observation.lineOfSight = 1;
        observation.rangeFit = 1;
        observation.neutralNear = ledger.mass[AccountIndex(PhysicalAccount::Neutral)] != 0;
        observation.selfLowStructure = state.frame.structureControlMass < 64;
        observation.playerChannel = charge.active != 0;
        std::uint8_t available = 1u << static_cast<std::uint8_t>(FrameAction::Move);
        if (ClosestPanel(state) != kInvalidSlotIndex) {
            available = static_cast<std::uint8_t>(available
                | (1u << static_cast<std::uint8_t>(FrameAction::Shift))
                | (state.frame.structureControlMass >= 16
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
        state.scene = Act2Scene::EncounterIntro;
        state.sceneTicks = 0;
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
        if (state.combat.actors[0].position.x >= 286 * kQ8One
            || state.encounterTick >= kAdapterDurationTicks) BeginSuccess(state);
    }
    return PhysicalLedgerValid(ledger) && ClaimLedgerValid(state.claimLedger)
        && CombatStateValid(state.combat, ledger);
}

void RenderAct2Runtime(PixelBuffer buffer, const Act2Runtime& state) noexcept {
    if (state.scene == Act2Scene::Playing) {
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
    case Act2Scene::ConfirmHold:
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
    const std::uint8_t* bytes = reinterpret_cast<const std::uint8_t*>(&state);
    std::uint32_t hash = 2166136261u;
    for (std::size_t index = 0; index < sizeof(state); ++index) {
        hash ^= bytes[index];
        hash *= 16777619u;
    }
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
#endif

}  // namespace bp
