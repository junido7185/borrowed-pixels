#include "bp_claim_ledger.h"

#include <cstdint>

namespace bp {
namespace {

constexpr std::uint16_t kInitialClaimMass[kClaimOwnerCount] = {
    0,
    320,
    48,
    64,
    224,
    112,
    72,
    240,
    48,
    72,
    240,
    0,
};

std::uint8_t OwnerIndex(ClaimOwner owner) noexcept {
    return static_cast<std::uint8_t>(owner);
}

bool ValidOwner(ClaimOwner owner) noexcept {
    return OwnerIndex(owner) < kClaimOwnerCount;
}

bool TransferCandidate(
    ClaimLedger& ledger,
    ClaimOwner from,
    ClaimOwner to,
    std::uint16_t amount) noexcept {
    if (!ValidOwner(from) || !ValidOwner(to)) {
        return false;
    }
    const std::uint8_t fromIndex = OwnerIndex(from);
    const std::uint8_t toIndex = OwnerIndex(to);
    if (fromIndex == toIndex) {
        return amount == 0;
    }
    if (ledger.mass[fromIndex] < amount) {
        return false;
    }
    const std::uint32_t destination = static_cast<std::uint32_t>(ledger.mass[toIndex]) + amount;
    if (destination > UINT16_MAX) {
        return false;
    }
    ledger.mass[fromIndex] = static_cast<std::uint16_t>(ledger.mass[fromIndex] - amount);
    ledger.mass[toIndex] = static_cast<std::uint16_t>(destination);
    return true;
}

bool ApplyChoiceTransfers(ClaimLedger& ledger, EncounterId encounter, ChoiceId choice) noexcept {
    switch (encounter) {
    case EncounterId::Patch:
        if (choice == ChoiceId::First) {
            return TransferCandidate(ledger, ClaimOwner::Patch, ClaimOwner::Player, 16);
        }
        if (choice == ChoiceId::Third) {
            return TransferCandidate(ledger, ClaimOwner::Patch, ClaimOwner::Common, 16);
        }
        return true;
    case EncounterId::Pair:
        return choice != ChoiceId::First
            || TransferCandidate(ledger, ClaimOwner::Pair, ClaimOwner::Player, 16);
    case EncounterId::Vessel:
        if (choice == ChoiceId::First) {
            return TransferCandidate(ledger, ClaimOwner::Vessel, ClaimOwner::Player, 64);
        }
        if (choice == ChoiceId::Third) {
            return TransferCandidate(ledger, ClaimOwner::Vessel, ClaimOwner::Common, 64);
        }
        return true;
    case EncounterId::Keeper:
        if (choice == ChoiceId::First) {
            return TransferCandidate(ledger, ClaimOwner::Keeper, ClaimOwner::Player, 64);
        }
        if (choice == ChoiceId::Third) {
            return TransferCandidate(ledger, ClaimOwner::Keeper, ClaimOwner::Common, 64);
        }
        return true;
    case EncounterId::Adapter:
        if (!TransferCandidate(ledger, ClaimOwner::Frame, ClaimOwner::Adapter,
                choice == ChoiceId::First ? 24 : (choice == ChoiceId::Second ? 48 : 16))) {
            return false;
        }
        if (choice == ChoiceId::First) {
            return TransferCandidate(ledger, ClaimOwner::Frame, ClaimOwner::Player, 24);
        }
        if (choice == ChoiceId::Third) {
            return TransferCandidate(ledger, ClaimOwner::Frame, ClaimOwner::Player, 32);
        }
        return true;
    case EncounterId::Frame:
        if (choice == ChoiceId::First) {
            return TransferCandidate(ledger, ClaimOwner::Frame, ClaimOwner::Player, 64);
        }
        if (choice == ChoiceId::Third) {
            return TransferCandidate(ledger, ClaimOwner::Frame, ClaimOwner::Common, 64);
        }
        return true;
    case EncounterId::Index:
        if (choice == ChoiceId::First) {
            return TransferCandidate(ledger, ClaimOwner::Index, ClaimOwner::Player, 32);
        }
        if (choice == ChoiceId::Third) {
            return TransferCandidate(ledger, ClaimOwner::Index, ClaimOwner::Adapter, 16)
                && TransferCandidate(ledger, ClaimOwner::Index, ClaimOwner::Common, 16);
        }
        return true;
    case EncounterId::Witness:
        if (choice == ChoiceId::Second) {
            return TransferCandidate(ledger, ClaimOwner::Witness, ClaimOwner::Common, 32);
        }
        if (choice == ChoiceId::Third) {
            return TransferCandidate(ledger, ClaimOwner::Witness, ClaimOwner::Player, 48);
        }
        return true;
    case EncounterId::Current:
        if (choice == ChoiceId::First) {
            return TransferCandidate(ledger, ClaimOwner::Current, ClaimOwner::Player, 64);
        }
        if (choice == ChoiceId::Third) {
            return TransferCandidate(ledger, ClaimOwner::Current, ClaimOwner::Patch, 16)
                && TransferCandidate(ledger, ClaimOwner::Current, ClaimOwner::Pair, 16)
                && TransferCandidate(ledger, ClaimOwner::Current, ClaimOwner::Adapter, 16)
                && TransferCandidate(ledger, ClaimOwner::Current, ClaimOwner::Common, 16);
        }
        return true;
    default:
        return false;
    }
}

bool ApplyVesselMinimum(ClaimLedger& ledger) noexcept {
    if (!TransferCandidate(ledger, ClaimOwner::System, ClaimOwner::Player, 64)) {
        return false;
    }
    const std::uint16_t playerMass = ledger.mass[OwnerIndex(ClaimOwner::Player)];
    if (playerMass >= 144) {
        return true;
    }
    return TransferCandidate(
        ledger,
        ClaimOwner::Vessel,
        ClaimOwner::Player,
        static_cast<std::uint16_t>(144 - playerMass));
}

}  // namespace

ClaimLedger MakeInitialClaimLedger() noexcept {
    ClaimLedger ledger{};
    for (std::uint8_t index = 0; index < kClaimOwnerCount; ++index) {
        ledger.mass[index] = kInitialClaimMass[index];
    }
    return ledger;
}

std::uint32_t ClaimLedgerTotal(const ClaimLedger& ledger) noexcept {
    std::uint32_t total = 0;
    for (std::uint8_t index = 0; index < kClaimOwnerCount; ++index) {
        total += ledger.mass[index];
    }
    return total;
}

bool ClaimLedgerValid(const ClaimLedger& ledger) noexcept {
    return ClaimLedgerTotal(ledger) == kLedgerTotal;
}

bool ClaimTransfer(
    ClaimLedger& ledger,
    ClaimOwner from,
    ClaimOwner to,
    std::uint16_t amount) noexcept {
    if (!ClaimLedgerValid(ledger)) {
        return false;
    }
    ClaimLedger candidate = ledger;
    if (!TransferCandidate(candidate, from, to, amount) || !ClaimLedgerValid(candidate)) {
        return false;
    }
    ledger = candidate;
    return true;
}

bool ApplySettlement(
    ClaimLedger& ledger,
    NarrativeState& narrative,
    EncounterId encounter,
    ChoiceId choice) noexcept {
    const std::uint8_t encounterIndex = static_cast<std::uint8_t>(encounter);
    const std::uint8_t choiceIndex = static_cast<std::uint8_t>(choice);
    if (!ClaimLedgerValid(ledger)
        || encounterIndex >= kSettlementEncounterCount
        || choiceIndex >= static_cast<std::uint8_t>(ChoiceId::Count)) {
        return false;
    }

    ClaimLedger candidateLedger = ledger;
    NarrativeState candidateNarrative = narrative;
    if (encounter == EncounterId::Vessel && !ApplyVesselMinimum(candidateLedger)) {
        return false;
    }
    if (!ApplyChoiceTransfers(candidateLedger, encounter, choice)
        || !ClaimLedgerValid(candidateLedger)) {
        return false;
    }

    const std::uint32_t shift = static_cast<std::uint32_t>(encounterIndex) * 2u;
    const std::uint32_t mask = 3u << shift;
    candidateNarrative.choiceBits =
        (candidateNarrative.choiceBits & ~mask) | (static_cast<std::uint32_t>(choiceIndex) << shift);

    ledger = candidateLedger;
    narrative = candidateNarrative;
    return true;
}

}  // namespace bp
