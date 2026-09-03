#include "bp_act1.h"

#include "bp_game.h"

#include <cstdint>

namespace bp {
namespace {

constexpr Act1FieldDef Field(
    const char* id,
    std::int16_t x,
    std::int16_t y,
    std::int16_t width,
    std::int16_t height,
    std::uint16_t mass) noexcept {
    return Act1FieldDef{id, x, y, width, height, mass};
}

constexpr Act1NeutralDef Neutral(
    std::int16_t x,
    std::int16_t y,
    std::uint16_t mass,
    Act1NeutralTag tag = Act1NeutralTag::Free) noexcept {
    return Act1NeutralDef{x, y, mass, tag};
}

constexpr Act1EncounterDef kDefinitions[kAct1EncounterCount] = {
    {
        EncounterId::Patch, ClaimOwner::Patch, "PATCH", "REPLACE",
        "I REPLACED EVERY TILE.", "WHICH PART OF ME ARRIVED?",
        48, 160, 64, 48, 92, 270, 90, 5,
        {
            Field("P0", 160, 90, 14, 14, 32),
            Field("P1", 116, 52, 28, 8, 32),
            Field("P2", 116, 128, 28, 8, 32),
            Field("P3", 204, 52, 28, 8, 32),
            Field("P4", 204, 128, 28, 8, 32),
        },
        {
            Neutral(234, 54, 8, Act1NeutralTag::PatchFeed),
            Neutral(250, 70, 8, Act1NeutralTag::PatchFeed),
            Neutral(250, 110, 8, Act1NeutralTag::PatchFeed),
            Neutral(234, 126, 8, Act1NeutralTag::PatchFeed),
            Neutral(144, 70, 8), Neutral(176, 70, 8),
            Neutral(144, 110, 8), Neutral(176, 110, 8),
        },
    },
    {
        EncounterId::Pair, ClaimOwner::Pair, "PAIR", "KEEP",
        "ONE NUMBER. TWO VOICES.", "THE LEDGER CALLS ONE AN ERROR.",
        64, 160, 64, 48, 92, 248, 90, 5,
        {
            Field("P0", 160, 46, 40, 8, 32),
            Field("P1", 160, 134, 40, 8, 32),
            Field("P2", 118, 90, 10, 42, 32),
            Field("P3", 202, 90, 10, 42, 32),
            Field("P4", 160, 90, 20, 12, 32),
        },
        {
            Neutral(136, 62, 8), Neutral(160, 62, 8),
            Neutral(184, 62, 8), Neutral(136, 118, 8),
            Neutral(160, 118, 8), Neutral(184, 118, 8),
            Neutral(148, 90, 8), Neutral(172, 90, 8),
        },
    },
    {
        EncounterId::Vessel, ClaimOwner::Vessel, "VESSEL", "EXPOSE",
        "WHAT I KEEP INSIDE IS MINE.", "NO BOUNDARY. NO SELF.",
        224, 320, 128, 42, 92, 264, 90, 10,
        {
            Field("P0", 96, 46, 28, 8, 32),
            Field("P1", 160, 46, 28, 8, 32),
            Field("P2", 224, 46, 28, 8, 32),
            Field("P3", 96, 134, 28, 8, 32),
            Field("P4", 160, 134, 28, 8, 32),
            Field("P5", 224, 134, 28, 8, 32),
            Field("P6", 126, 74, 10, 22, 32),
            Field("P7", 194, 74, 10, 22, 32),
            Field("P8", 126, 108, 10, 22, 32),
            Field("P9", 194, 108, 10, 22, 32),
        },
        {
            Neutral(76, 70, 16), Neutral(76, 112, 16),
            Neutral(110, 90, 16), Neutral(148, 68, 16),
            Neutral(148, 112, 16), Neutral(182, 90, 16),
            Neutral(222, 68, 16), Neutral(222, 112, 16),
        },
    },
};

constexpr std::uint8_t OwnerIndex(ClaimOwner owner) noexcept {
    return static_cast<std::uint8_t>(owner);
}

constexpr std::uint8_t EncounterIndex(EncounterId encounter) noexcept {
    return static_cast<std::uint8_t>(encounter);
}

bool RectInsidePlay(const Act1FieldDef& field) noexcept {
    const std::int32_t left = field.centerX - field.width / 2;
    const std::int32_t top = field.centerY - field.height / 2;
    return field.width > 0 && field.height > 0
        && left >= kPlayInterior.x && top >= kPlayInterior.y
        && left + field.width <= kPlayInterior.x + kPlayInterior.width
        && top + field.height <= kPlayInterior.y + kPlayInterior.height;
}

}  // namespace

const Act1EncounterDef& GetAct1EncounterDef(std::uint8_t index) noexcept {
    return kDefinitions[index < kAct1EncounterCount ? index : 0];
}

bool ValidateAct1EncounterDef(const Act1EncounterDef& definition) noexcept {
    if (EncounterIndex(definition.id) >= kAct1EncounterCount
        || definition.fieldCount == 0
        || definition.fieldCount > kAct1MaximumFieldCount
        || definition.opponentMass < kCoreMass) {
        return false;
    }
    std::uint32_t fieldMass = 0;
    for (std::uint8_t index = 0; index < definition.fieldCount; ++index) {
        if (!RectInsidePlay(definition.field[index]) || definition.field[index].mass == 0) {
            return false;
        }
        fieldMass += definition.field[index].mass;
    }
    std::uint32_t neutralMass = 0;
    for (const Act1NeutralDef& neutral : definition.neutral) {
        if (neutral.mass == 0
            || neutral.x < kPlayInterior.x
            || neutral.y < kPlayInterior.y
            || neutral.x >= kPlayInterior.x + kPlayInterior.width
            || neutral.y >= kPlayInterior.y + kPlayInterior.height) {
            return false;
        }
        neutralMass += neutral.mass;
    }
    return fieldMass == definition.fieldMass
        && neutralMass == definition.neutralMass;
}

std::uint16_t PlayerActiveMassForClaim(const ClaimLedger& ledger) noexcept {
    const std::uint16_t claim = ledger.mass[OwnerIndex(ClaimOwner::Player)];
    return claim < 64 ? 64 : (claim > 240 ? 240 : claim);
}

bool InitializeAct1Physical(
    PhysicalLedger& ledger,
    const ClaimLedger& claims,
    const Act1EncounterDef& definition) noexcept {
    return ClaimLedgerValid(claims)
        && ValidateAct1EncounterDef(definition)
        && InitializePhysicalLedger(
            ledger,
            PlayerActiveMassForClaim(claims),
            definition.opponentMass,
            definition.fieldMass,
            definition.neutralMass);
}

bool PopulateAct1Combat(
    CombatState& combat,
    const PhysicalLedger& ledger,
    const Act1EncounterDef& definition,
    SlotHandle (&neutralHandles)[kAct1NeutralCount]) noexcept {
    if (!ValidateAct1EncounterDef(definition) || !PhysicalLedgerValid(ledger)) {
        return false;
    }
    InitializeCombatState(
        combat,
        Vec2Q8{definition.playerX * kQ8One, definition.playerY * kQ8One},
        Vec2Q8{definition.opponentX * kQ8One, definition.opponentY * kQ8One});
    for (std::uint8_t index = 0; index < definition.fieldCount; ++index) {
        const Act1FieldDef& field = definition.field[index];
        SlotHandle unused{};
        if (!AddCover(
                combat,
                RectPixels{
                    field.centerX - field.width / 2,
                    field.centerY - field.height / 2,
                    field.width,
                    field.height,
                },
                field.mass,
                unused)) {
            return false;
        }
    }
    for (std::uint8_t index = 0; index < kAct1NeutralCount; ++index) {
        const Act1NeutralDef& neutral = definition.neutral[index];
        if (!AddNeutral(
                combat,
                Vec2Q8{neutral.x * kQ8One, neutral.y * kQ8One},
                neutral.mass,
                0,
                0,
                neutralHandles[index])) {
            return false;
        }
        if (neutral.tag == Act1NeutralTag::PatchFeed) {
            combat.neutrals[neutralHandles[index].index].patchFeedMass = neutral.mass;
        }
    }
    if (definition.id == EncounterId::Pair) {
        combat.actors[static_cast<std::uint8_t>(CombatSide::Opponent)].projectileImmune = 1;
    }
    return CombatStateValid(combat, ledger);
}

bool ApplyVesselMinimumClaim(
    ClaimLedger& ledger,
    NarrativeState& narrative) noexcept {
    if (!ClaimLedgerValid(ledger)) {
        return false;
    }
    ClaimLedger candidate = ledger;
    if (!ClaimTransfer(candidate, ClaimOwner::System, ClaimOwner::Player, 64)) {
        return false;
    }
    const std::uint16_t player = candidate.mass[OwnerIndex(ClaimOwner::Player)];
    if (player < 144
        && !ClaimTransfer(
            candidate,
            ClaimOwner::Vessel,
            ClaimOwner::Player,
            static_cast<std::uint16_t>(144 - player))) {
        return false;
    }
    if (!ClaimLedgerValid(candidate)
        || candidate.mass[OwnerIndex(ClaimOwner::Player)] != 144) {
        return false;
    }
    ledger = candidate;
    narrative.role = 2;
    return true;
}

bool ApplyVesselContinuationClaim(
    ClaimLedger& ledger,
    NarrativeState& narrative,
    ChoiceId choice) noexcept {
    if (!ClaimLedgerValid(ledger)
        || ledger.mass[OwnerIndex(ClaimOwner::Player)] != 144
        || static_cast<std::uint8_t>(choice) >= static_cast<std::uint8_t>(ChoiceId::Count)) {
        return false;
    }
    ClaimLedger candidate = ledger;
    if (choice == ChoiceId::First
        && !ClaimTransfer(candidate, ClaimOwner::Vessel, ClaimOwner::Player, 64)) {
        return false;
    }
    if (choice == ChoiceId::Third
        && !ClaimTransfer(candidate, ClaimOwner::Vessel, ClaimOwner::Common, 64)) {
        return false;
    }
    NarrativeState narrativeCandidate = narrative;
    constexpr std::uint32_t shift = static_cast<std::uint32_t>(EncounterId::Vessel) * 2u;
    narrativeCandidate.choiceBits =
        (narrativeCandidate.choiceBits & ~(3u << shift))
        | (static_cast<std::uint32_t>(choice) << shift);
    ledger = candidate;
    narrative = narrativeCandidate;
    return true;
}

const char* Act1ChoiceText(EncounterId encounter, ChoiceId choice) noexcept {
    constexpr const char* choices[kAct1EncounterCount][3] = {
        {"RECLAIM ORIGIN", "RECOGNIZE CONTINUITY", "UNMARK OWNERSHIP"},
        {"SELECT ONE", "SEPARATE BOTH", "KEEP THE PAIR"},
        {"CLAIM", "RETURN", "RELEASE"},
    };
    const std::uint8_t encounterIndex = EncounterIndex(encounter);
    const std::uint8_t choiceIndex = static_cast<std::uint8_t>(choice);
    return encounterIndex < kAct1EncounterCount && choiceIndex < 3
        ? choices[encounterIndex][choiceIndex]
        : "";
}

const char* Act1ResponseText(EncounterId encounter, ChoiceId choice) noexcept {
    constexpr const char* responses[kAct1EncounterCount][3] = {
        {"THEN I ENDED|BEFORE I ARRIVED", "CHANGE DID NOT|CANCEL ME", "I WAS NEVER|MINE ALONE"},
        {"ONE VOICE REMAINS|THE NUMBER FITS", "TWO ACCOUNTS|ONE OVERLAPPING MEMORY", "WE REMAIN WHAT|THE LEDGER CANNOT COUNT"},
        {"YOUR BORDER GREW|THROUGH MINE", "YOU COULD TAKE IT|YOU DID NOT", "NO BORDER|RECEIVED IT"},
    };
    const std::uint8_t encounterIndex = EncounterIndex(encounter);
    const std::uint8_t choiceIndex = static_cast<std::uint8_t>(choice);
    return encounterIndex < kAct1EncounterCount && choiceIndex < 3
        ? responses[encounterIndex][choiceIndex]
        : "";
}

}  // namespace bp
