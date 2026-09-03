#pragma once

#include "bp_encounter.h"

#include <cstdint>

namespace bp {

inline constexpr std::uint8_t kAct3EncounterCount = 3;
inline constexpr std::uint8_t kAct3MaximumFieldCount = 9;
inline constexpr std::uint8_t kAct3MaximumNeutralCount = 20;
inline constexpr std::uint8_t kIndexMetricNodeCount = 3;
inline constexpr std::uint8_t kIndexUnscoredCount = 2;
inline constexpr std::uint8_t kWitnessRecordCount = 3;
inline constexpr std::uint8_t kWitnessSweeperSegmentCount = 8;
inline constexpr std::uint8_t kCurrentInitialProjectileCount = 16;
inline constexpr std::uint8_t kCurrentOwnedNeutralCount = 6;
inline constexpr std::uint16_t kIndexDurationTicks = 3600;
inline constexpr std::uint16_t kWitnessDurationTicks = 3600;
inline constexpr std::uint16_t kWitnessDismantleTicks = 60;

enum class Act3NeutralTag : std::uint8_t {
    Free = 0,
    Unscored,
};

struct Act3RectMassDef {
    std::int16_t centerX;
    std::int16_t centerY;
    std::int16_t width;
    std::int16_t height;
    std::uint16_t mass;
    std::uint8_t metric;
};

struct Act3NeutralDef {
    std::int16_t x;
    std::int16_t y;
    std::uint16_t mass;
    Act3NeutralTag tag;
};

struct Act3EncounterDef {
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
    Act3RectMassDef field[kAct3MaximumFieldCount];
    Act3NeutralDef neutral[kAct3MaximumNeutralCount];
};

struct IndexRuntime {
    struct Probe {
        Vec2Q8 position;
        Vec2Q8 directionQ15;
        SlotHandle physicalHandle;
        std::uint16_t respawnTicks;
        std::uint8_t targetCore;
        std::uint8_t active;
    } probes[2];
    SlotHandle metricNodes[kIndexMetricNodeCount];
    SlotHandle unscored[kIndexUnscoredCount];
    std::uint32_t distanceQ8;
    std::uint16_t damage;
    std::uint16_t idleTicks;
    std::uint8_t destroyedMetricNodes;
};

struct WitnessRecord {
    Vec2Q8 position;
    std::uint32_t stableId;
    std::uint16_t channelTicks;
    std::uint8_t active;
};

struct WitnessRuntime {
    WitnessRecord records[kWitnessRecordCount];
    SlotHandle sweeperSegments[kWitnessSweeperSegmentCount];
    std::uint16_t sweeperTick;
    std::uint16_t sweeperReturnTick;
    std::uint16_t protocolEscrowMass;
    std::uint8_t sweeperTarget;
    std::uint8_t sweeperActive;
    std::uint8_t sweeperGap;
    std::uint8_t sweeperReturning;
    std::uint8_t directRecordMass;
};

enum class CurrentAction : std::uint8_t {
    Orbit = 0,
    MoveA,
    Wave,
    ReturnA,
    MoveB,
    Foreclose,
    ReturnB,
};

struct CurrentRuntime {
    SlotHandle ownedNeutrals[kCurrentOwnedNeutralCount];
    CurrentAction action;
    Vec2Q8 forecloseAim;
    std::uint16_t actionTicks;
    std::uint16_t returnAccumulator;
    std::uint32_t initialProjectileFirstStableId;
    std::uint8_t cycleIndex;
    std::uint8_t orbitShots;
};

enum class Act3Scene : std::uint8_t {
    EncounterIntro = 0,
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
    Complete,
    Fatal,
};

struct Act3Runtime {
    ClaimLedger claimLedger;
    NarrativeState narrative;
    GameState movement;
    CombatState combat;
    ReclaimState reclaim;
    HudLoanSystem hudLoan;
    SlotHandle neutrals[kAct3MaximumNeutralCount];
    IndexRuntime index;
    WitnessRuntime witness;
    CurrentRuntime current;
    Act3Scene scene;
    ChoiceId focusedChoice;
    ChoiceId committedChoice;
    std::uint32_t globalTick;
    std::uint32_t encounterTick;
    std::uint16_t sceneTicks;
    std::uint16_t previousPlayerReclaim;
    std::uint8_t encounterIndex;
    std::uint8_t retryPending;
    std::uint8_t introRevisit;
};

const Act3EncounterDef& GetAct3EncounterDef(std::uint8_t index) noexcept;
bool ValidateAct3EncounterDef(const Act3EncounterDef& definition) noexcept;
ChoiceId ResolveIndexResult(std::uint16_t damage, std::uint32_t distanceQ8,
    std::uint16_t idleTicks, std::uint16_t unscoredMass,
    std::uint8_t destroyedMetricNodes) noexcept;
ChoiceId ResolveWitnessResult(std::uint8_t recordsPreserved,
    std::uint8_t directRecordMass) noexcept;
CurrentAction CurrentActionAtCycleTick(std::uint16_t tick) noexcept;
Vec2Q8 RotateCurrentDirection(Vec2Q8 directionQ15, std::int8_t degrees) noexcept;

bool PopulateAct3Encounter(Act3Runtime& state) noexcept;
bool InitializeAct3Runtime(Act3Runtime& state, const ClaimLedger& claims,
    const NarrativeState& narrative) noexcept;
bool TickAct3Runtime(Act3Runtime& state, const InputFrame& input) noexcept;
void RenderAct3Runtime(PixelBuffer buffer, const Act3Runtime& state) noexcept;
std::uint32_t Act3RuntimeHash(const Act3Runtime& state) noexcept;

#if defined(BP_TEST)
bool ForceAct3EncounterSuccessForTest(Act3Runtime& state) noexcept;
bool TickCurrentScriptForTest(Act3Runtime& state) noexcept;
#endif

}  // namespace bp
