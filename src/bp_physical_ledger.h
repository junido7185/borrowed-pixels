#pragma once

#include "bp_types.h"

namespace bp {

std::uint32_t PhysicalLedgerTotal(const PhysicalLedger& ledger) noexcept;
bool PhysicalLedgerValid(const PhysicalLedger& ledger) noexcept;

bool InitializePhysicalLedger(
    PhysicalLedger& ledger,
    std::uint16_t playerActiveMass,
    std::uint16_t opponentActiveMass,
    std::uint16_t fieldMass,
    std::uint16_t initialNeutralMass) noexcept;

bool PhysicalTransfer(
    PhysicalLedger& ledger,
    PhysicalAccount from,
    PhysicalAccount to,
    std::uint16_t amount) noexcept;

}  // namespace bp
