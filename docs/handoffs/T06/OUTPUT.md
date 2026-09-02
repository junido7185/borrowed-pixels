# T06 OUTPUT

TICKET: T06

BASE SHA: `e2f548b36caa2d5c36601991de8b4eae304f7216`

CANDIDATE SHA: resolve `candidate/T06-final` after independent review and review-evidence commit.

CHANGED PATHS:

- `src/bp_reclaim.h`, `src/bp_reclaim.cpp`
- `src/bp_hud_loan.h`, `src/bp_hud_loan.cpp`
- `tests/bp_reclaim_hud_loan_tests.cpp`
- `docs/STATUS.md`
- `docs/evidence/T06/**`
- `docs/handoffs/T06/**`

IMPLEMENTED:

- Fixed-capacity Neutral motion with 15-tick inertia handoff, exact 65067/65536 per-axis damping, 20% normal boundary reflection, and deterministic eligible-pair merging.
- Player/opponent reclaim targeting by exact range, integer 18-degree cone, authoritative score and stable-ID tie, Bresenham Cover occlusion, attraction, 32/60 accumulator, provenance order, and final-TILE arbitration.
- VITAL 48, VECTOR 80, and IMPULSE 96 loan state machines covering all seven authoritative HUD states.
- Atomic InstalledHUD/Transit/PlayerShell transactions, staged pre-collision reservation/tick-17 resolution, 45/60-tick channels, 36-tick transits, 720-tick maturity, 120-tick warning, Due fire lock, carried newly reclaimed Shell repayment, and 90-tick reallocation lock.
- One-open-loan enforcement, insufficient manual restore rollback, hold cancellation, and pending commit cancellation on a PLAYER Shell-hit tick.

AUTOMATED RESULTS:

- T06 direct test: `6,512` checks; 21 module-state cases and Neutral capacity 64.
- T05 regression: `4,052`; T04 `333,093`/replay `61646592`; T03 `5,035,224`/Hangul 11,172; T02 `2,365,194`.
- Debug/Test/CTest/debug smoke/Contest clean build: PASS on local Windows x64 MSVC.
- Contest dist: one file, `129,024B`; internal/hard caps PASS.
- Contest SHA-256: `f406cffff791b01cc0ad041245230c5d3bc8918d9faeb92ed3f043e19bb9f0bc`.

REVIEW:

- Initial independent review found three P1 defects in channel selection, frozen tick staging, and pre-INACTIVE maturity shortening.
- First repair review found one additional P1 at the pending-restore/DUE boundary and one P2 public wrapper risk.
- Final read-only review of `779581479eb84d285121a33ced8615d38c78336c` reports P0=0, P1=0, P2=0.
- The final review-evidence-only tip is checked once more before standing local fast-forward approval is used.

STATUS: READY_FOR_REVIEW — not ACCEPTED and not integrated.

LOCAL INTEGRATION:

- Standing Human Director approval applied after candidate, ancestry, clean state, authority hashes, P0/P1/P2=0, required tests, and exact dist were rechecked.
- `integration/rc-2026-09-03` fast-forwarded from `e2f548b36caa2d5c36601991de8b4eae304f7216` to candidate `70a2ae0b44d4b80a2beeba2d391ffe5d5633d207`.
- Post-integration authority/configure/Debug/Test/direct/CTest/smoke/Contest/dist regression passed.
- No push, tag, publication, or submission was performed.

INTEGRATION STATUS: LOCALLY_INTEGRATED — not externally accepted or released.
