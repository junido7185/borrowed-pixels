#pragma once

#include "bp_encounter.h"

#include <cstdint>

namespace bp {

inline constexpr std::uint8_t kAct2EncounterCount = 3;
inline constexpr std::uint8_t kAct2MaximumFieldCount = 12;
inline constexpr std::uint8_t kAct2MaximumNeutralCount = 16;
inline constexpr std::uint8_t kKeeperStructureCount = 5;
inline constexpr std::uint8_t kSweeperSegmentCount = 8;
inline constexpr std::uint16_t kKeeperDurationTicks = 5400;
inline constexpr std::uint16_t kAdapterDurationTicks = 4500;
inline constexpr std::uint16_t kAdapterPreviewTick = 3900;
inline constexpr std::uint16_t kCompactorStartTick = 900;
inline constexpr std::uint16_t kFrameDecisionTicks = 30;

struct Act2RectMassDef {
    std::int16_t centerX;
    std::int16_t centerY;
    std::int16_t width;
    std::int16_t height;
    std::uint16_t mass;
};

struct Act2NeutralDef {
    std::int16_t x;
    std::int16_t y;
    std::uint16_t mass;
    std::uint8_t escrow;
};

struct Act2EncounterDef {
    EncounterId id;
    ClaimOwner owner;
    const char* name;
    const char* objective;
    const char* introLine1;
    const char* introLine2;
    std::uint16_t opponentMass;
    std::uint16_t fieldMass;
    std::uint16_t neutralMass;
    std::uint16_t durationTicks;
    std::int16_t playerX;
    std::int16_t playerY;
    std::int16_t opponentX;
    std::int16_t opponentY;
    std::uint8_t fieldCount;
    std::uint8_t neutralCount;
    Act2RectMassDef field[kAct2MaximumFieldCount];
    Act2NeutralDef neutral[kAct2MaximumNeutralCount];
};

enum class AdapterResult : std::uint8_t {
    Need = 0,
    Equal,
    Output,
};

enum class FrameAction : std::uint8_t {
    Shift = 0,
    Toll,
    Evict,
    Claim,
    Reclaim,
    Move,
    Count,
};

struct FrameObservation {
    std::uint8_t incomingThreat;
    std::uint8_t blockedChoice;
    std::uint8_t playerNearPanel;
    std::uint8_t playerLowShell;
    std::uint8_t coverBlocks;
    std::uint8_t playerChannel;
    std::uint8_t selfLowStructure;
    std::uint8_t lineOfSight;
    std::uint8_t rangeFit;
    std::uint8_t neutralNear;
};

struct SweeperRuntime {
    SlotHandle segments[kSweeperSegmentCount];
    std::uint16_t waveTick;
    std::uint16_t returnTick;
    std::uint16_t escrowInitial;
    std::uint8_t active;
    std::uint8_t returning;
};

struct CompactorRuntime {
    SlotHandle walls[2];
    std::uint16_t tick;
    std::uint8_t active;
};

struct KeeperRuntime {
    SlotHandle structures[kKeeperStructureCount];
    SweeperRuntime sweeper;
    std::uint8_t destroyedStructures;
    std::uint8_t wavesCompleted;
};

struct AdapterRuntime {
    CompactorRuntime compactor;
    std::uint16_t playerEscrow;
    std::uint8_t adapterArrived;
};

struct FrameRuntime {
    SlotHandle panels[kAct2MaximumFieldCount];
    RectPixels shiftStart;
    SlotHandle shiftPanel;
    RectPixels door;
    std::uint16_t actionCooldown;
    std::uint16_t actionTicks;
    std::uint16_t structureControlMass;
    std::uint16_t doorCooldown;
    std::uint16_t panelAliveMask;
    FrameAction action;
    std::int8_t shiftX;
    std::int8_t shiftY;
    std::uint8_t doorArmed;
};

enum class Act2Scene : std::uint8_t {
    EncounterIntro = 0,
    ObjectiveCard,
    Playing,
    Settlement,
    ConfirmHold,
    EntityResponse,
    LedgerReturn,
    Complete,
    Fatal,
};

struct Act2Runtime {
    ClaimLedger claimLedger;
    NarrativeState narrative;
    GameState movement;
    CombatState combat;
    ReclaimState reclaim;
    HudLoanSystem hudLoan;
    SlotHandle neutrals[kAct2MaximumNeutralCount];
    KeeperRuntime keeper;
    AdapterRuntime adapter;
    FrameRuntime frame;
    Act2Scene scene;
    ChoiceId focusedChoice;
    std::uint32_t globalTick;
    std::uint32_t encounterTick;
    std::uint16_t sceneTicks;
    std::uint16_t previousPlayerReclaim;
    std::uint8_t encounterIndex;
    std::uint8_t failure;
};

const Act2EncounterDef& GetAct2EncounterDef(std::uint8_t index) noexcept;
bool ValidateAct2EncounterDef(const Act2EncounterDef& definition) noexcept;
AdapterResult ClassifyAdapterResult(std::uint16_t playerEscrow) noexcept;
ChoiceId AdapterChoiceForEscrow(std::uint16_t playerEscrow) noexcept;
bool ApplyAdapterSettlementForEscrow(ClaimLedger& ledger, NarrativeState& narrative,
    std::uint16_t playerEscrow) noexcept;
std::int32_t AdapterMaximumSpeedPixelsPerSecond(std::uint16_t playerEscrow) noexcept;
std::int32_t FrameActionScore(FrameAction action, const FrameObservation& observation) noexcept;
FrameAction ChooseFrameAction(const FrameObservation& observation, std::uint8_t availableMask) noexcept;

bool StartSweeperWave(CombatState& combat, const PhysicalLedger& ledger,
    SweeperRuntime& sweeper) noexcept;
bool TickSweeperWave(CombatState& combat, PhysicalLedger& ledger,
    SweeperRuntime& sweeper) noexcept;
bool StartCompactor(CombatState& combat, const PhysicalLedger& ledger,
    CompactorRuntime& compactor) noexcept;
bool TickCompactor(CombatState& combat, const PhysicalLedger& ledger,
    CompactorRuntime& compactor) noexcept;

bool PopulateAct2Encounter(Act2Runtime& state) noexcept;
bool InitializeAct2Runtime(Act2Runtime& state, const ClaimLedger& claims,
    const NarrativeState& narrative) noexcept;
bool TickAct2Runtime(Act2Runtime& state, const InputFrame& input) noexcept;
void RenderAct2Runtime(PixelBuffer buffer, const Act2Runtime& state) noexcept;
std::uint32_t Act2RuntimeHash(const Act2Runtime& state) noexcept;

#if defined(BP_TEST)
bool ForceAct2EncounterSuccessForTest(Act2Runtime& state) noexcept;
bool ForceFrameActionForTest(Act2Runtime& state, FrameAction action) noexcept;
bool TickFrameActionForTest(Act2Runtime& state) noexcept;
#endif

}  // namespace bp
