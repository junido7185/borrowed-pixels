#pragma once

#include "bp_types.h"

namespace bp {

ClaimLedger MakeInitialClaimLedger() noexcept;
std::uint32_t ClaimLedgerTotal(const ClaimLedger& ledger) noexcept;
bool ClaimLedgerValid(const ClaimLedger& ledger) noexcept;

bool ClaimTransfer(
    ClaimLedger& ledger,
    ClaimOwner from,
    ClaimOwner to,
    std::uint16_t amount) noexcept;

bool ApplySettlement(
    ClaimLedger& ledger,
    NarrativeState& narrative,
    EncounterId encounter,
    ChoiceId choice) noexcept;

}  // namespace bp
