#include "bp_hud_loan.h"

#include "bp_physical_ledger.h"

#include <cstdint>

namespace bp {
namespace {

constexpr std::uint8_t ModuleIndex(HudModuleId module) noexcept {
    return static_cast<std::uint8_t>(module);
}

constexpr std::uint8_t AccountIndex(PhysicalAccount account) noexcept {
    return static_cast<std::uint8_t>(account);
}

bool ValidModule(HudModuleId module) noexcept {
    return ModuleIndex(module) < kHudModuleCount;
}

bool IsOpen(const HudModuleLoan& module) noexcept {
    return module.state != HudLoanState::Installed;
}

std::uint16_t ExpectedInstalledMass(const HudLoanSystem& system) noexcept {
    std::uint16_t total = 0;
    for (const HudModuleLoan& module : system.modules) {
        if (module.state == HudLoanState::Installed
            || module.state == HudLoanState::Dismantling) {
            total = static_cast<std::uint16_t>(total + module.mass);
        } else if (module.state == HudLoanState::ToHudTransit) {
            total = static_cast<std::uint16_t>(total + module.arrivedMass);
        }
    }
    return total;
}

std::uint16_t ExpectedTransitMass(const HudLoanSystem& system) noexcept {
    std::uint16_t total = 0;
    for (const HudModuleLoan& module : system.modules) {
        total = static_cast<std::uint16_t>(total + module.transitMass);
    }
    return total;
}

void SetInstalled(HudModuleLoan& module) noexcept {
    module.channelTicks = 0;
    module.stateTicks = 0;
    module.maturityRemaining = 0;
    module.maturityAdvanceTicks = 0;
    module.transitMass = 0;
    module.arrivedMass = module.mass;
    module.state = HudLoanState::Installed;
    module.warning = 0;
}

void SetInactive(HudModuleLoan& module) noexcept {
    module.channelTicks = 0;
    module.stateTicks = 0;
    module.maturityRemaining = static_cast<std::uint16_t>(
        kHudMaturityTicks - module.maturityAdvanceTicks);
    module.transitMass = 0;
    module.arrivedMass = 0;
    module.state = HudLoanState::Inactive;
    module.warning = 0;
}

bool AdvanceTransit(HudModuleLoan& module, PhysicalLedger& ledger) noexcept {
    if (module.state != HudLoanState::ToShellTransit
        && module.state != HudLoanState::ToHudTransit) return true;
    if (module.stateTicks < kHudTransitTicks) ++module.stateTicks;
    const std::uint16_t desiredMoved = static_cast<std::uint16_t>(
        static_cast<std::uint32_t>(module.mass) * module.stateTicks / kHudTransitTicks);
    if (module.state == HudLoanState::ToShellTransit) {
        const std::uint16_t alreadyMoved = static_cast<std::uint16_t>(
            module.mass - module.transitMass);
        const std::uint16_t amount = static_cast<std::uint16_t>(desiredMoved - alreadyMoved);
        if (amount > 0 && !PhysicalTransfer(
                ledger, PhysicalAccount::Transit, PhysicalAccount::PlayerShell, amount)) return false;
        module.transitMass = static_cast<std::uint16_t>(module.transitMass - amount);
        if (module.stateTicks == kHudTransitTicks) SetInactive(module);
    } else {
        const std::uint16_t amount = static_cast<std::uint16_t>(desiredMoved - module.arrivedMass);
        if (amount > 0 && !PhysicalTransfer(
                ledger, PhysicalAccount::Transit, PhysicalAccount::InstalledHud, amount)) return false;
        module.transitMass = static_cast<std::uint16_t>(module.transitMass - amount);
        module.arrivedMass = static_cast<std::uint16_t>(module.arrivedMass + amount);
        if (module.stateTicks == kHudTransitTicks) SetInstalled(module);
    }
    return true;
}

void AdvanceMaturity(HudModuleLoan& module) noexcept {
    if (module.state != HudLoanState::Inactive && module.state != HudLoanState::Restoring) return;
    if (module.maturityRemaining > 0) --module.maturityRemaining;
    module.warning = module.maturityRemaining > 0
        && module.maturityRemaining <= kHudWarningTicks ? 1u : 0u;
    if (module.maturityRemaining == 0) {
        module.state = HudLoanState::Due;
        module.channelTicks = 0;
        module.warning = 1;
    }
}

bool ApplyDueRepayment(HudModuleLoan& module, PhysicalLedger& ledger,
    std::uint16_t reclaimedMass) noexcept {
    if (module.state != HudLoanState::Due || reclaimedMass == 0) return true;
    const std::uint16_t remaining = static_cast<std::uint16_t>(module.mass - module.transitMass);
    std::uint16_t amount = reclaimedMass < remaining ? reclaimedMass : remaining;
    const std::uint16_t shell = ledger.mass[AccountIndex(PhysicalAccount::PlayerShell)];
    if (amount > shell) amount = shell;
    if (amount > 0 && !PhysicalTransfer(
            ledger, PhysicalAccount::PlayerShell, PhysicalAccount::Transit, amount)) return false;
    module.transitMass = static_cast<std::uint16_t>(module.transitMass + amount);
    if (module.transitMass == module.mass) {
        module.state = HudLoanState::ToHudTransit;
        module.stateTicks = 0;
        module.arrivedMass = 0;
        module.warning = 0;
    }
    return true;
}

bool CommitDismantle(HudModuleLoan& module, PhysicalLedger& ledger) noexcept {
    if (!PhysicalTransfer(
            ledger, PhysicalAccount::InstalledHud, PhysicalAccount::Transit, module.mass)) return false;
    module.state = HudLoanState::ToShellTransit;
    module.stateTicks = 0;
    module.channelTicks = 0;
    module.transitMass = module.mass;
    module.arrivedMass = 0;
    module.warning = 0;
    return true;
}

bool CommitRestore(HudModuleLoan& module, PhysicalLedger& ledger) noexcept {
    if (ledger.mass[AccountIndex(PhysicalAccount::PlayerShell)] < module.mass) return false;
    if (!PhysicalTransfer(
            ledger, PhysicalAccount::PlayerShell, PhysicalAccount::Transit, module.mass)) return false;
    module.state = HudLoanState::ToHudTransit;
    module.stateTicks = 0;
    module.channelTicks = 0;
    module.transitMass = module.mass;
    module.arrivedMass = 0;
    module.warning = 0;
    return true;
}

}  // namespace

std::uint16_t HudModuleMass(HudModuleId module) noexcept {
    switch (module) {
        case HudModuleId::Vital: return 48;
        case HudModuleId::Vector: return 80;
        case HudModuleId::Impulse: return 96;
        default: return 0;
    }
}

std::uint8_t HudModuleSelectNumber(HudModuleId module) noexcept {
    return ValidModule(module) ? static_cast<std::uint8_t>(ModuleIndex(module) + 1u) : 0u;
}

void InitializeHudLoanSystem(HudLoanSystem& system) noexcept {
    system = HudLoanSystem{};
    system.selected = HudModuleId::Vital;
    system.pendingModule = HudModuleId::Count;
    for (std::uint8_t index = 0; index < kHudModuleCount; ++index) {
        HudModuleLoan& module = system.modules[index];
        module.mass = HudModuleMass(static_cast<HudModuleId>(index));
        SetInstalled(module);
    }
}

bool HudLoanSystemValid(const HudLoanSystem& system, const PhysicalLedger& ledger) noexcept {
    if (!PhysicalLedgerValid(ledger) || !ValidModule(system.selected)
        || system.pendingCommit > 1) return false;
    std::uint8_t open = 0;
    for (std::uint8_t index = 0; index < kHudModuleCount; ++index) {
        const HudModuleLoan& module = system.modules[index];
        if (module.mass != HudModuleMass(static_cast<HudModuleId>(index))
            || static_cast<std::uint8_t>(module.state) >= static_cast<std::uint8_t>(HudLoanState::Count)
            || module.channelTicks > kHudRestoreTicks
            || module.stateTicks > kHudTransitTicks
            || module.maturityRemaining > kHudMaturityTicks
            || module.maturityAdvanceTicks > kHudMaturityTicks
            || module.transitMass > module.mass
            || module.arrivedMass > module.mass) return false;
        if (IsOpen(module)) ++open;
        if (module.state == HudLoanState::Installed
            && (module.transitMass != 0 || module.arrivedMass != module.mass
                || module.maturityRemaining != 0 || module.maturityAdvanceTicks != 0)) return false;
        if (module.state == HudLoanState::ToShellTransit
            && static_cast<std::uint32_t>(module.transitMass) + module.arrivedMass > module.mass) return false;
        if (module.state == HudLoanState::ToHudTransit
            && static_cast<std::uint32_t>(module.transitMass) + module.arrivedMass != module.mass) return false;
        if (module.state == HudLoanState::Due && module.maturityRemaining != 0) return false;
    }
    if (open > 1) return false;
    if (system.pendingCommit != 0) {
        if (!ValidModule(system.pendingModule) || system.pendingModule != system.selected) return false;
        const HudModuleLoan& pending = system.modules[ModuleIndex(system.pendingModule)];
        const bool dismantlePending = pending.state == HudLoanState::Dismantling
            && pending.channelTicks == kHudDismantleTicks;
        const bool restorePending = pending.state == HudLoanState::Restoring
            && pending.channelTicks == kHudRestoreTicks;
        if (!dismantlePending && !restorePending) return false;
    } else if (system.pendingModule != HudModuleId::Count) return false;
    if (ledger.mass[AccountIndex(PhysicalAccount::InstalledHud)] != ExpectedInstalledMass(system)) return false;
    return ledger.mass[AccountIndex(PhysicalAccount::Transit)] >= ExpectedTransitMass(system);
}

bool SelectHudModule(HudLoanSystem& system, HudModuleId module) noexcept {
    if (!ValidModule(module)) return false;
    if (system.pendingCommit != 0) return false;
    for (const HudModuleLoan& loan : system.modules) {
        if (loan.state == HudLoanState::Dismantling
            || loan.state == HudLoanState::Restoring) return false;
    }
    system.selected = module;
    return true;
}

bool AdvanceHudLoanEarly(
    HudLoanSystem& system,
    PhysicalLedger& ledger,
    bool reallocateHeld,
    std::uint16_t playerReclaimedMassFromPreviousTick) noexcept {
    if (!HudLoanSystemValid(system, ledger) || system.pendingCommit != 0) return false;
    HudLoanSystem candidate = system;
    PhysicalLedger ledgerCandidate = ledger;
    if (candidate.reallocationLockTicks > 0) --candidate.reallocationLockTicks;

    for (HudModuleLoan& module : candidate.modules) {
        const HudLoanState before = module.state;
        if (!AdvanceTransit(module, ledgerCandidate)) return false;
        if (before == HudLoanState::ToHudTransit && module.state == HudLoanState::Installed) {
            candidate.reallocationLockTicks = kHudReallocationLockTicks;
        }
    }
    for (HudModuleLoan& module : candidate.modules) AdvanceMaturity(module);
    for (HudModuleLoan& module : candidate.modules) {
        if (!ApplyDueRepayment(
                module, ledgerCandidate, playerReclaimedMassFromPreviousTick)) return false;
    }

    HudModuleLoan& selected = candidate.modules[ModuleIndex(candidate.selected)];
    if (selected.state == HudLoanState::Installed) {
        if (reallocateHeld && candidate.reallocationLockTicks == 0
            && OpenHudLoanCount(candidate) == 0) {
            selected.state = HudLoanState::Dismantling;
            selected.channelTicks = 1;
            selected.arrivedMass = selected.mass;
        }
    } else if (selected.state == HudLoanState::Dismantling) {
        if (!reallocateHeld) {
            SetInstalled(selected);
        } else {
            if (selected.channelTicks < kHudDismantleTicks) ++selected.channelTicks;
            if (selected.channelTicks == kHudDismantleTicks) {
                candidate.pendingCommit = 1;
                candidate.pendingModule = candidate.selected;
            }
        }
    } else if (selected.state == HudLoanState::Inactive) {
        if (reallocateHeld && candidate.reallocationLockTicks == 0) {
            selected.state = HudLoanState::Restoring;
            selected.channelTicks = 1;
        }
    } else if (selected.state == HudLoanState::Restoring) {
        if (!reallocateHeld) {
            selected.state = HudLoanState::Inactive;
            selected.channelTicks = 0;
        } else {
            if (selected.channelTicks < kHudRestoreTicks) ++selected.channelTicks;
            if (selected.channelTicks == kHudRestoreTicks) {
                candidate.pendingCommit = 1;
                candidate.pendingModule = candidate.selected;
            }
        }
    }

    if (!HudLoanSystemValid(candidate, ledgerCandidate)) return false;
    system = candidate;
    ledger = ledgerCandidate;
    return true;
}

bool ResolvePendingHudCommit(
    HudLoanSystem& system,
    PhysicalLedger& ledger,
    bool playerShellHitThisTick) noexcept {
    if (!HudLoanSystemValid(system, ledger)) return false;
    if (system.pendingCommit == 0) return true;
    HudLoanSystem candidate = system;
    PhysicalLedger ledgerCandidate = ledger;
    HudModuleLoan& module = candidate.modules[ModuleIndex(candidate.pendingModule)];
    if (module.state == HudLoanState::Dismantling) {
        if (playerShellHitThisTick) SetInstalled(module);
        else if (!CommitDismantle(module, ledgerCandidate)) return false;
    } else if (module.state == HudLoanState::Restoring) {
        if (playerShellHitThisTick || !CommitRestore(module, ledgerCandidate)) {
            module.state = HudLoanState::Inactive;
            module.channelTicks = 0;
        }
    } else {
        return false;
    }
    candidate.pendingCommit = 0;
    candidate.pendingModule = HudModuleId::Count;
    if (!HudLoanSystemValid(candidate, ledgerCandidate)) return false;
    system = candidate;
    ledger = ledgerCandidate;
    return true;
}

bool TickHudLoan(
    HudLoanSystem& system,
    PhysicalLedger& ledger,
    bool reallocateHeld,
    bool playerShellHitThisTick,
    std::uint16_t playerReclaimedMassFromPreviousTick) noexcept {
    if (!AdvanceHudLoanEarly(
            system, ledger, reallocateHeld, playerReclaimedMassFromPreviousTick)) return false;
    return ResolvePendingHudCommit(system, ledger, playerShellHitThisTick);
}

bool ShortenOpenHudMaturity(HudLoanSystem& system, std::uint16_t ticks) noexcept {
    for (HudModuleLoan& module : system.modules) {
        if (!IsOpen(module)) continue;
        if (module.state == HudLoanState::Dismantling
            || module.state == HudLoanState::ToShellTransit) {
            const std::uint32_t advanced = static_cast<std::uint32_t>(
                module.maturityAdvanceTicks) + ticks;
            module.maturityAdvanceTicks = static_cast<std::uint16_t>(
                advanced < kHudMaturityTicks ? advanced : kHudMaturityTicks);
        } else if (module.state == HudLoanState::Inactive
            || module.state == HudLoanState::Restoring) {
            module.maturityRemaining = ticks < module.maturityRemaining
                ? static_cast<std::uint16_t>(module.maturityRemaining - ticks)
                : 0;
            module.warning = module.maturityRemaining > 0
                && module.maturityRemaining <= kHudWarningTicks ? 1u : 0u;
            if (module.maturityRemaining == 0) {
                module.state = HudLoanState::Due;
                module.channelTicks = 0;
                module.warning = 1;
            }
        }
        return true;
    }
    return false;
}

bool HudModuleFullyInstalled(const HudLoanSystem& system, HudModuleId module) noexcept {
    return ValidModule(module)
        && system.modules[ModuleIndex(module)].state == HudLoanState::Installed;
}

bool HudFireLocked(const HudLoanSystem& system) noexcept {
    for (const HudModuleLoan& module : system.modules) {
        if (module.state == HudLoanState::Dismantling
            || module.state == HudLoanState::Restoring
            || module.state == HudLoanState::Due) return true;
    }
    return false;
}

bool HudLoanWarning(const HudLoanSystem& system) noexcept {
    for (const HudModuleLoan& module : system.modules) {
        if (module.warning != 0) return true;
    }
    return false;
}

bool HudHasPendingCommit(const HudLoanSystem& system) noexcept {
    return system.pendingCommit != 0;
}

std::uint16_t HudVisibleMaterialMass(const HudLoanSystem& system, HudModuleId module) noexcept {
    if (!ValidModule(module)) return 0;
    const HudModuleLoan& loan = system.modules[ModuleIndex(module)];
    if (loan.state == HudLoanState::Installed) return loan.mass;
    if (loan.state == HudLoanState::Dismantling) {
        const std::uint16_t removed = static_cast<std::uint16_t>(
            static_cast<std::uint32_t>(loan.mass) * loan.channelTicks / kHudDismantleTicks);
        return static_cast<std::uint16_t>(loan.mass - removed);
    }
    if (loan.state == HudLoanState::ToHudTransit) return loan.arrivedMass;
    return 0;
}

std::uint8_t OpenHudLoanCount(const HudLoanSystem& system) noexcept {
    std::uint8_t count = 0;
    for (const HudModuleLoan& module : system.modules) {
        if (IsOpen(module)) ++count;
    }
    return count;
}

}  // namespace bp
