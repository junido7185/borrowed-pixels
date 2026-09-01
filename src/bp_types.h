#pragma once

#include <cstdint>

namespace bp {

inline constexpr std::uint16_t kLedgerTotal = 1440;
inline constexpr std::uint16_t kCoreMass = 16;
inline constexpr std::uint16_t kInstalledHudMass = 224;
inline constexpr std::uint16_t kProtocolMass = 32;

enum class ClaimOwner : std::uint8_t {
    Player = 0,
    System,
    Patch,
    Pair,
    Vessel,
    Keeper,
    Adapter,
    Frame,
    Index,
    Witness,
    Current,
    Common,
    Count,
};

inline constexpr std::uint8_t kClaimOwnerCount =
    static_cast<std::uint8_t>(ClaimOwner::Count);

enum class PhysicalAccount : std::uint8_t {
    PlayerCore = 0,
    PlayerShell,
    OpponentCore,
    OpponentShell,
    InstalledHud,
    Cover,
    Charge,
    Projectile,
    Neutral,
    Transit,
    ProtocolActive,
    LockedBands,
    Count,
};

inline constexpr std::uint8_t kPhysicalAccountCount =
    static_cast<std::uint8_t>(PhysicalAccount::Count);

enum class EncounterId : std::uint8_t {
    Patch = 0,
    Pair,
    Vessel,
    Keeper,
    Adapter,
    Frame,
    Index,
    Witness,
    Current,
    Precedent,
    Count,
};

inline constexpr std::uint8_t kSettlementEncounterCount = 9;

enum class ChoiceId : std::uint8_t {
    First = 0,
    Second,
    Third,
    Count,
};

struct ClaimLedger {
    std::uint16_t mass[kClaimOwnerCount];
};

struct PhysicalLedger {
    std::uint16_t mass[kPhysicalAccountCount];
};

struct NarrativeState {
    std::uint32_t choiceBits;
    std::uint16_t adjudicatedMass;
    std::uint8_t encounterIndex;
    std::uint8_t role;
    std::uint8_t entityStatus[kSettlementEncounterCount];
    std::uint8_t endingsSeen;
};

struct PolicyProfile {
    std::uint8_t identity;
    std::uint8_t distribution;
    std::uint8_t governance;
    std::uint8_t memory;
};

struct EncounterDef {
    std::uint8_t id;
    std::uint8_t kind;
    std::uint8_t owner;
    std::uint8_t flags;
    std::uint16_t opponentMass;
    std::uint16_t fieldMass;
    std::uint16_t neutralMass;
    std::uint16_t targetTicks;
};

static_assert(kClaimOwnerCount == 12);
static_assert(kPhysicalAccountCount == 12);
static_assert(kSettlementEncounterCount == 9);
static_assert(sizeof(ClaimLedger) == 24);
static_assert(sizeof(PhysicalLedger) == 24);
static_assert(sizeof(NarrativeState) == 20);
static_assert(sizeof(PolicyProfile) == 4);
static_assert(sizeof(EncounterDef) == 12);

}  // namespace bp
