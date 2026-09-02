# T06 INPUT

TICKET: T06 Neutral dynamics, reclaim, and HUD loan

BASE SHA: `e2f548b36caa2d5c36601991de8b4eae304f7216`

AUTHORITY:

- Production spec sections 3.1, 6.2, 6.4, 24.5-24.6, 32.6-32.9.
- Runtime contract combat.reclaim, combat.hud, motion HUD recipes, and fixed capacities.
- Runbook T06 ownership: `reclaim`, `hud_loan`, and related tests.

ALLOWLIST:

- `src/bp_reclaim.h`, `src/bp_reclaim.cpp`
- `src/bp_hud_loan.h`, `src/bp_hud_loan.cpp`
- `tests/bp_reclaim_hud_loan_tests.cpp`
- `docs/STATUS.md`
- `docs/evidence/T06/**`
- `docs/handoffs/T06/**`

ACCEPTANCE:

- Neutral inertia, damping, boundary reflection, deterministic ordinary merge, provenance, and capacity behavior.
- Player/opponent reclaim range, cone, score, Cover line of sight, force, accumulator, final-TILE arbitration, and exact ledger transfer.
- VITAL/VECTOR/IMPULSE exact mass and all seven HUD states.
- Exact 45/60/36/720/120/90 tick contracts, one open loan, Due fire lock, partial repayment, manual restore rollback, and Shell-hit pending-commit cancellation.
- Full regression, independent review P0/P1=0, and exact contest distribution measurement.
