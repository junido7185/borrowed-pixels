#include "bp_physical_ledger.h"

#include <cstdint>

namespace bp {
namespace {

std::uint8_t AccountIndex(PhysicalAccount account) noexcept {
    return static_cast<std::uint8_t>(account);
}

bool ValidAccount(PhysicalAccount account) noexcept {
    return AccountIndex(account) < kPhysicalAccountCount;
}

}  // namespace

std::uint32_t PhysicalLedgerTotal(const PhysicalLedger& ledger) noexcept {
    std::uint32_t total = 0;
    for (std::uint8_t index = 0; index < kPhysicalAccountCount; ++index) {
        total += ledger.mass[index];
    }
    return total;
}

bool PhysicalLedgerValid(const PhysicalLedger& ledger) noexcept {
    return PhysicalLedgerTotal(ledger) == kLedgerTotal;
}

bool InitializePhysicalLedger(
    PhysicalLedger& ledger,
    std::uint16_t playerActiveMass,
    std::uint16_t opponentActiveMass,
    std::uint16_t fieldMass,
    std::uint16_t initialNeutralMass) noexcept {
    if (playerActiveMass < 64 || playerActiveMass > 240 || opponentActiveMass < kCoreMass) {
        return false;
    }

    const std::uint32_t activeTotal =
        static_cast<std::uint32_t>(playerActiveMass)
        + opponentActiveMass
        + kInstalledHudMass
        + fieldMass
        + initialNeutralMass
        + kProtocolMass;
    if (activeTotal > kLedgerTotal) {
        return false;
    }

    PhysicalLedger candidate{};
    candidate.mass[AccountIndex(PhysicalAccount::PlayerCore)] = kCoreMass;
    candidate.mass[AccountIndex(PhysicalAccount::PlayerShell)] =
        static_cast<std::uint16_t>(playerActiveMass - kCoreMass);
    candidate.mass[AccountIndex(PhysicalAccount::OpponentCore)] = kCoreMass;
    candidate.mass[AccountIndex(PhysicalAccount::OpponentShell)] =
        static_cast<std::uint16_t>(opponentActiveMass - kCoreMass);
    candidate.mass[AccountIndex(PhysicalAccount::InstalledHud)] = kInstalledHudMass;
    candidate.mass[AccountIndex(PhysicalAccount::Cover)] = fieldMass;
    candidate.mass[AccountIndex(PhysicalAccount::Neutral)] = initialNeutralMass;
    candidate.mass[AccountIndex(PhysicalAccount::ProtocolActive)] = kProtocolMass;
    candidate.mass[AccountIndex(PhysicalAccount::LockedBands)] =
        static_cast<std::uint16_t>(kLedgerTotal - activeTotal);

    if (!PhysicalLedgerValid(candidate)) {
        return false;
    }
    ledger = candidate;
    return true;
}

bool PhysicalTransfer(
    PhysicalLedger& ledger,
    PhysicalAccount from,
    PhysicalAccount to,
    std::uint16_t amount) noexcept {
    if (!PhysicalLedgerValid(ledger) || !ValidAccount(from) || !ValidAccount(to)) {
        return false;
    }

    const std::uint8_t fromIndex = AccountIndex(from);
    const std::uint8_t toIndex = AccountIndex(to);
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

    PhysicalLedger candidate = ledger;
    candidate.mass[fromIndex] = static_cast<std::uint16_t>(candidate.mass[fromIndex] - amount);
    candidate.mass[toIndex] = static_cast<std::uint16_t>(destination);
    if (!PhysicalLedgerValid(candidate)) {
        return false;
    }
    ledger = candidate;
    return true;
}

}  // namespace bp
