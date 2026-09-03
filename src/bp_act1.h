#pragma once

#include "bp_claim_ledger.h"
#include "bp_combat.h"
#include "bp_physical_ledger.h"

#include <cstdint>

namespace bp {

inline constexpr std::uint8_t kAct1EncounterCount = 3;
inline constexpr std::uint8_t kAct1MaximumFieldCount = 10;
inline constexpr std::uint8_t kAct1NeutralCount = 8;

struct Act1FieldDef {
    const char* id;
    std::int16_t centerX;
    std::int16_t centerY;
    std::int16_t width;
    std::int16_t height;
    std::uint16_t mass;
};

enum class Act1NeutralTag : std::uint8_t {
    Free = 0,
    PatchFeed,
};

struct Act1NeutralDef {
    std::int16_t x;
    std::int16_t y;
    std::uint16_t mass;
    Act1NeutralTag tag;
};

struct Act1EncounterDef {
    EncounterId id;
    ClaimOwner owner;
    const char* name;
    const char* objective;
    const char* introLine1;
    const char* introLine2;
    std::uint16_t opponentMass;
    std::uint16_t fieldMass;
    std::uint16_t neutralMass;
    std::int16_t playerX;
    std::int16_t playerY;
    std::int16_t opponentX;
    std::int16_t opponentY;
    std::uint8_t fieldCount;
    Act1FieldDef field[kAct1MaximumFieldCount];
    Act1NeutralDef neutral[kAct1NeutralCount];
};

const Act1EncounterDef& GetAct1EncounterDef(std::uint8_t index) noexcept;
bool ValidateAct1EncounterDef(const Act1EncounterDef& definition) noexcept;
std::uint16_t PlayerActiveMassForClaim(const ClaimLedger& ledger) noexcept;

bool InitializeAct1Physical(
    PhysicalLedger& ledger,
    const ClaimLedger& claims,
    const Act1EncounterDef& definition) noexcept;

bool PopulateAct1Combat(
    CombatState& combat,
    const PhysicalLedger& ledger,
    const Act1EncounterDef& definition,
    SlotHandle (&neutralHandles)[kAct1NeutralCount]) noexcept;

bool ApplyVesselMinimumClaim(
    ClaimLedger& ledger,
    NarrativeState& narrative) noexcept;
bool ApplyVesselContinuationClaim(
    ClaimLedger& ledger,
    NarrativeState& narrative,
    ChoiceId choice) noexcept;

const char* Act1ChoiceText(EncounterId encounter, ChoiceId choice) noexcept;
const char* Act1ResponseText(EncounterId encounter, ChoiceId choice) noexcept;

}  // namespace bp
