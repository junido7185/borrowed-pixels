#pragma once

#include "bp_input.h"
#include "bp_render.h"
#include "bp_types.h"

#include <cstdint>

namespace bp {

inline constexpr std::uint16_t kEnoughConfirmTicks=60;
inline constexpr std::uint16_t kFinalPreviewTicks=12;
inline constexpr std::uint16_t kFinalConfirmTicks=30;
inline constexpr std::uint16_t kFinalCommitTicks=36;
inline constexpr std::uint16_t kOneRevealTicks=48;
inline constexpr std::uint16_t kEndingMinimumTicks=360;
inline constexpr std::uint16_t kEndingAutomaticTicks=720;
inline constexpr std::uint16_t kCreditsTicks=720;
inline constexpr std::uint16_t kAuditSummaryTicks=180;

enum class EndingId : std::uint8_t {
    Enough=0,
    Rule,
    Hand,
    OpenLedger,
    One,
};

enum class FinalChoice : std::uint8_t {
    Ratify=0,
    Override,
    Open,
    TotalClaim,
    Count,
};

enum class NarrativeScene : std::uint8_t {
    EnoughConfirm=0,
    FinalAllocation,
    FinalPreview,
    FinalConfirm,
    FinalCommit,
    Ending,
    Credits,
    Title,
    AuditTrace,
    Fatal,
};

enum class NarrativeRouteRequest : std::uint8_t {
    None=0,
    NewRun,
    ContinueVessel,
};

struct NarrativeRuntime {
    ClaimLedger claimLedger;
    NarrativeState narrative;
    NarrativeScene scene;
    EndingId ending;
    FinalChoice focused;
    FinalChoice committed;
    std::uint32_t globalTick;
    std::uint16_t sceneTicks;
    std::uint16_t holdTicks;
    std::uint16_t oneRevealTicks;
    std::uint8_t commitOwner;
    std::uint8_t auditIndex;
    std::uint8_t oneUnlocked;
    std::uint8_t openUnlocked;
    std::uint8_t vesselContinueAvailable;
    std::uint8_t auditTraceUnlocked;
    std::uint8_t fullEndingReached;
    NarrativeRouteRequest routeRequest;
};

struct SaveBoundaryObserver {
    std::uint8_t lastRuntimeStage;
    std::uint8_t lastEncounterIndex;
    std::uint8_t lastScene;
    std::uint32_t lastOptions;
    std::uint8_t initialized;
};

bool InitializeFinalNarrative(NarrativeRuntime& state,const ClaimLedger& claims,
    const NarrativeState& narrative) noexcept;
bool InitializeEnoughNarrative(NarrativeRuntime& state,const ClaimLedger& claims,
    const NarrativeState& narrative) noexcept;
bool InitializeTitleNarrative(NarrativeRuntime& state,const ClaimLedger& claims,
    const NarrativeState& narrative,std::uint32_t saveFlags) noexcept;
bool OpenLedgerUnlocked(const ClaimLedger& claims,const NarrativeState& narrative) noexcept;
bool TotalClaimUnlocked(const NarrativeState& narrative) noexcept;
bool ApplyFinalChoice(NarrativeRuntime& state,FinalChoice choice) noexcept;
bool TickNarrativeRuntime(NarrativeRuntime& state,const InputFrame& input) noexcept;
void RenderNarrativeRuntime(PixelBuffer buffer,const NarrativeRuntime& state,
    std::uint32_t presentationOptions=0) noexcept;
std::uint32_t NarrativeRuntimeHash(const NarrativeRuntime& state) noexcept;

bool ObserveSaveBoundary(SaveBoundaryObserver& observer,std::uint8_t runtimeStage,
    std::uint8_t encounterIndex,std::uint8_t scene,std::uint32_t options,
    bool authoritativeBoundary,bool forbiddenState) noexcept;

}  // namespace bp
