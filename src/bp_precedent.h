#pragma once

#include "bp_game.h"
#include "bp_hud_loan.h"
#include "bp_reclaim.h"
#include "bp_render.h"

#include <cstdint>

namespace bp {

inline constexpr std::uint16_t kPrecedentBorrowMass=256;
inline constexpr std::uint16_t kPrecedentInitialShell=240;
inline constexpr std::uint8_t kPrecedentCoverCount=10;
inline constexpr std::uint8_t kPrecedentNeutralCount=16;

enum class IdentityPolicy : std::uint8_t { Material=0, Continuity, Relation, Count };
enum class DistributionPolicy : std::uint8_t { Equality=0, Need, Utility, Count };
enum class GovernancePolicy : std::uint8_t { Rule=0, Judgment, Commons, Count };
enum class MemoryPolicy : std::uint8_t { Full=0, Summary, None, Count };

struct PolicyDecision {
    PolicyProfile profile;
    EncounterId decisiveEncounter[4];
    ChoiceId decisiveChoice[4];
};

enum class PrecedentScene : std::uint8_t {
    RoleAssign=0,
    PolicyAudit,
    Playing,
    HitFreeze,
    Retry,
    CoreUnfold,
    Complete,
    Fatal,
};

enum class PrecedentAction : std::uint8_t {
    Evade=0,
    Move,
    Reclaim,
    Tap,
    Cut,
    Claim,
    Foreclose,
    Count,
};

struct PrecedentActionScores {
    std::int32_t value[static_cast<std::uint8_t>(PrecedentAction::Count)];
};

struct PrecedentRuntime {
    ClaimLedger claimLedger;
    NarrativeState narrative;
    PolicyDecision policy;
    GameState movement;
    CombatState combat;
    ReclaimState reclaim;
    HudLoanSystem hudLoan;
    SlotHandle covers[kPrecedentCoverCount];
    SlotHandle neutrals[kPrecedentNeutralCount];
    SlotHandle tetherNodes[3];
    SlotHandle voteNodes[3];
    PrecedentScene scene;
    PrecedentAction action;
    PrecedentAction pendingAction;
    std::uint32_t globalTick;
    std::uint32_t encounterTick;
    std::uint16_t sceneTicks;
    std::uint16_t phaseTicks;
    std::uint16_t phaseDisplayTicks;
    std::uint16_t exceptionTicks;
    std::uint16_t outputDamage[2];
    std::uint16_t outputReclaim[2];
    std::uint32_t outputMovementQ8[2];
    std::uint16_t previousPlayerReclaim;
    std::uint16_t markedOriginMass;
    std::uint16_t markedOriginTicks;
    std::uint16_t originChargeMass;
    std::uint16_t reclaimPriorityTicks[2];
    std::uint16_t forkTicks;
    std::uint16_t echoTicks;
    std::uint16_t echoClaimTotal;
    std::uint16_t borrowedFoldedMass;
    std::uint8_t auditRow;
    std::uint8_t phase;
    std::uint8_t echoIndex;
    std::uint8_t exceptionShown;
    std::uint8_t exceptionDeniedShown;
    std::uint8_t borrowedActive;
    std::uint8_t pendingValid;
    std::uint8_t retryPending;
    std::uint8_t lastPlayerPolicyAction;
    std::uint32_t lastPlayerPolicyTick;
    std::uint32_t originPriorityStableId[2];
};

ChoiceId ChoiceForEncounter(const NarrativeState& narrative,EncounterId encounter) noexcept;
bool DerivePolicyDecision(const NarrativeState& narrative,PolicyDecision& out) noexcept;
PolicyAuditView BuildPolicyAuditView(const PolicyDecision& decision) noexcept;
std::uint8_t PrecedentPhaseForShell(std::uint16_t shell) noexcept;
PrecedentActionScores EvaluatePrecedentActions(const PrecedentRuntime& state) noexcept;
bool PopulatePrecedentBattlefield(PrecedentRuntime& state) noexcept;
bool FoldPrecedentBorrow(PrecedentRuntime& state) noexcept;
bool InitializePrecedentRuntime(PrecedentRuntime& state,const ClaimLedger& claims,
    const NarrativeState& narrative) noexcept;
bool TickPrecedentRuntime(PrecedentRuntime& state,const InputFrame& input) noexcept;
void RenderPrecedentRuntime(PixelBuffer buffer,const PrecedentRuntime& state) noexcept;
std::uint32_t PrecedentRuntimeHash(const PrecedentRuntime& state) noexcept;
std::uint16_t PrecedentActClaimMoved(const NarrativeState& narrative,
    std::uint8_t actIndex) noexcept;

#if defined(BP_TEST)
bool ApplyPrecedentNeutralTransferForTest(PrecedentRuntime& state,
    PhysicalAccount from,std::uint16_t mass) noexcept;
bool ApplyPrecedentOpponentAttackForTest(PrecedentRuntime& state,
    std::uint16_t mass) noexcept;
#endif

}  // namespace bp
