#pragma once

#include "bp_act1.h"
#include "bp_game.h"
#include "bp_hud_loan.h"
#include "bp_reclaim.h"
#include "bp_render.h"

#include <cstdint>

namespace bp {

inline constexpr std::uint16_t kTitleBorrowTicks = 72;
inline constexpr std::uint16_t kLedgerRowUnfoldTicks = 36;
inline constexpr std::uint16_t kEncounterIntroMinimumTicks = 90;
inline constexpr std::uint16_t kEncounterRevisitIntroMinimumTicks = 18;
inline constexpr std::uint16_t kEncounterIntroAutomaticTicks = 180;
inline constexpr std::uint16_t kObjectiveCardTicks = 90;
inline constexpr std::uint16_t kHitFreezeTicks = 5;
inline constexpr std::uint16_t kSettlementWaitTicks = 42;
inline constexpr std::uint16_t kAllocationPreviewTicks = 12;
inline constexpr std::uint16_t kConfirmHoldTicks = 30;
inline constexpr std::uint16_t kCommitTransitTicks = 36;
inline constexpr std::uint16_t kEntityResponseMinimumTicks = 90;
inline constexpr std::uint16_t kEntityResponseAutomaticTicks = 180;
inline constexpr std::uint16_t kLedgerReturnTicks = 36;

inline constexpr std::uint16_t kPatchTapCooldownTicks = 90;
inline constexpr std::uint16_t kPatchFeedTransitTicks = 18;
inline constexpr std::uint16_t kPatchFeedTriggerLoss = 8;
inline constexpr std::uint8_t kPatchSuccessFeedCount = 4;

inline constexpr std::uint16_t kPairDurationTicks = 2700;
inline constexpr std::uint16_t kPairProbeRetargetTicks = 30;
inline constexpr std::int32_t kPairProbeSpeedPixelsPerSecond = 84;

inline constexpr std::uint16_t kVesselPressTicks = 60;
inline constexpr std::uint16_t kVesselPressCooldownTicks = 90;
inline constexpr std::uint16_t kVesselBraceTicks = 60;
inline constexpr std::uint16_t kVesselBraceCooldownTicks = 180;
inline constexpr std::uint8_t kVesselBraceSpeedPercent = 50;
inline constexpr std::uint8_t kVesselDeflectAngleDegrees = 70;
inline constexpr std::uint16_t kVesselStampMass = 32;
inline constexpr std::uint16_t kVesselStampTelegraphTicks = 30;
inline constexpr std::int32_t kVesselStampSpeedPixelsPerSecond = 90;
inline constexpr std::uint16_t kVesselStampCooldownTicks = 120;
inline constexpr std::int16_t kVesselStampWidth = 24;
inline constexpr std::int16_t kVesselStampHeight = 10;
inline constexpr std::int32_t kVesselReclaimRangePixels = 72;
inline constexpr std::uint16_t kVesselReclaimMassPerSecond = 32;

enum class Act1Scene : std::uint8_t {
    Title = 0,
    BorrowAnimation,
    LedgerFocus,
    RowUnfold,
    EncounterIntro,
    ObjectiveCard,
    Playing,
    HitFreeze,
    EncounterRetry,
    SettlementWait,
    Settlement,
    Preview,
    ConfirmHold,
    CommitTransit,
    EntityResponse,
    LedgerReturn,
    EnoughEnding,
    ContinueReady,
    Fatal,
};

struct PatchRuntime {
    SlotHandle feed[kPatchSuccessFeedCount];
    std::uint16_t shellBaseline;
    std::uint16_t transitTicks;
    std::uint16_t transitNeutralIndex;
    std::uint8_t feedsUsed;
    std::uint8_t transitActive;
    std::uint8_t pendingReservation;
};

struct PairProbe {
    Vec2Q8 position;
    Vec2Q8 directionQ15;
    SlotHandle protocolNeutral;
    std::uint16_t respawnTicks;
    std::uint8_t targetCore;
    std::uint8_t active;
};

struct PairRuntime {
    PairProbe probes[2];
    std::uint16_t coreX[2];
    std::uint16_t coreY[2];
    std::uint8_t coreAliveMask;
    std::uint8_t forcedSelect;
};

enum class VesselAction : std::uint8_t {
    Press = 0,
    Brace,
    Stamp,
    Reclaim,
};

struct VesselRuntime {
    VesselAction action;
    Vec2Q8 pressDirectionQ15;
    std::uint16_t actionTicks;
    std::uint16_t pressCooldown;
    std::uint16_t braceCooldown;
    std::uint16_t stampCooldown;
    std::uint16_t stampTelegraph;
    std::uint16_t reclaimAccumulator;
};

struct Act1Runtime {
    ClaimLedger claimLedger;
    NarrativeState narrative;
    GameState movement;
    CombatState combat;
    ReclaimState reclaim;
    HudLoanSystem hudLoan;
    SlotHandle neutralHandles[kAct1NeutralCount];
    PatchRuntime patch;
    PairRuntime pair;
    VesselRuntime vessel;
    Act1Scene scene;
    ChoiceId focusedChoice;
    ChoiceId committedChoice;
    std::uint32_t globalTick;
    std::uint32_t encounterTick;
    // CROSS_TICKET_GLUE: monotonic observer serial for platform audio only;
    // excluded from gameplay decisions and Act1RuntimeHash.
    std::uint32_t audioCoreHitSerial;
    std::uint16_t sceneTicks;
    std::uint16_t previousPlayerReclaim;
    std::uint8_t encounterIndex;
    std::uint8_t settlementStage;
    std::uint8_t initializedEncounter;
    std::uint8_t failure;
    std::uint8_t retryPending;
    std::uint8_t introRevisit;
};

bool InitializeAct1Runtime(Act1Runtime& state) noexcept;
bool TickAct1Runtime(Act1Runtime& state, const InputFrame& input) noexcept;
void RenderAct1Runtime(PixelBuffer buffer, const Act1Runtime& state) noexcept;
std::uint32_t Act1RuntimeHash(const Act1Runtime& state) noexcept;

#if defined(BP_TEST)
bool ForceAct1EncounterSuccessForTest(Act1Runtime& state) noexcept;
#endif

}  // namespace bp
