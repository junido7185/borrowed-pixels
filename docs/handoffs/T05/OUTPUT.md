# T05 OUTPUT

TICKET: T05

BASE SHA: `587f8fe9abfee78b5fbcbd160ece1408275458b7`

CANDIDATE SHA: resolve `candidate/T05-final` after independent review and review-evidence commit.

CHANGED PATHS:

- `src/bp_combat.h`, `src/bp_combat.cpp`
- `tests/bp_combat_tests.cpp`
- `docs/STATUS.md`
- `docs/evidence/T05/**`
- `docs/handoffs/T05/INPUT.md`, `docs/handoffs/T05/OUTPUT.md`

IMPLEMENTED:

- Integer-only Charge lifecycle, exact standard stages, partial-mass projectile formulas, rollback, recoil, and cooldowns.
- Fixed-capacity Charge/Projectile/Neutral/Cover state with stable IDs, generations, explicit fatal codes, and aggregate-ledger validation.
- Swept circle/circle, relative projectile/projectile, circle/AABB, and boundary TOI with global time/priority/stable-ID processing.
- Shell, Cover, Projectile, Charge, and boundary conversions to Neutral with exact physical-ledger transactions and provenance.
- Exposure grace, Core hit HIT_FREEZE, and physical/claim ledger snapshot.
- Deterministic nearest Neutral pair merge to recover a saturated slot before a collision transaction.

AUTOMATED RESULTS:

- T05 direct test: `4,052` checks; capacities Projectile 32 and Neutral 64.
- 211 one-pixel Cover positions tested against a 260px/tick swept segment without tunneling.
- 56 consecutive boundary conversions and 32-step byte-identical dual replay preserve both 1,440 ledgers.
- T04 regression: `333,093` checks, replay `61646592`.
- T03 regression: `5,035,224` checks, Hangul 11,172, policy profiles 81, overlap pixels 0.
- T02 regression: `2,365,194` checks, 19,683 combinations, SaveV2 96 bytes.
- Debug/Test/CTest/debug smoke/Contest clean build: PASS on local Windows x64 MSVC.
- Contest dist: one file, `129,024B`; delta from T04 `0B`; internal/hard caps PASS.

REVIEW:

- Initial independent review found P1=1 in failed non-Core impact atomicity; repair `8d259afae6cf2b848b6c3b14b60e322eeb39621f` moved the impact position update inside the candidate transaction.
- Final read-only review of `20e06d870185111971231820b392e963d81f2b94` reports P0=0, P1=0, P2=1 and confirms the production repair is correct.
- The P2 is a non-blocking test-only gap: the byte-identity test covers preflight failure but does not inject a post-copy ProcessImpact failure. It is recorded in `docs/evidence/T05/independent-review.txt`.
- The final review-evidence-only tip is checked once more before standing local fast-forward approval is used.

STATUS: READY_FOR_REVIEW — not ACCEPTED and not integrated.

LOCAL INTEGRATION:

- Standing Human Director approval applied after candidate, ancestry, clean state, authority hashes, P0/P1=0, required tests, and exact dist were rechecked.
- `integration/rc-2026-09-03` fast-forwarded from `587f8fe9abfee78b5fbcbd160ece1408275458b7` to candidate `f8f1c042f00623cb5b0e71aad137b409f4c2c656`.
- Post-integration authority/configure/Debug/Test/direct/CTest/smoke/Contest/dist regression passed.
- The independent P2 test-only fault-injection debt remains recorded; no push, tag, publication, or submission was performed.

INTEGRATION STATUS: LOCALLY_INTEGRATED — not externally accepted or released.
