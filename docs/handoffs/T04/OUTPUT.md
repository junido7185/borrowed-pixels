# T04 OUTPUT

TICKET: T04

BASE SHA: `53a3cac34427f98ec281754a4a19e6980686987d`

CANDIDATE SHA: resolve `candidate/T04-final` after independent review and review-evidence commit.

CHANGED PATHS:

- `src/bp_game.h`, `src/bp_game.cpp`
- `src/bp_input.h`, `src/bp_input.cpp`
- `src/bp_math.h`
- `tests/bp_game_input_math_tests.cpp`
- `docs/STATUS.md`
- `docs/evidence/T04/**`
- `docs/handoffs/T04/INPUT.md`, `docs/handoffs/T04/OUTPUT.md`

IMPLEMENTED:

- Deterministic 60 Hz Q8.8 player state and explicit little-endian FNV-1a state hash.
- Exact input priority, focus/pause handling, and Fire/Reclaim nine-tick switch lock.
- Shell-dependent movement speed, acceleration, per-axis friction, vector cap, actor radius, boundary clamp, and circle/AABB minimum separation.
- Eight-tick/20-pixel IMPULSE dash, five movement-tick projectile immunity, 144-tick cooldown, boundary cancellation, and active-dash action exclusion.
- Atomic invalid-tick rejection and both-ledger validation before/after simulation.

AUTOMATED RESULTS:

- T04 direct test: `333,093` checks, 4,096-tick golden replay `61646592`.
- Collision grid: 7,992 circle/AABB separation cases plus 20,000 randomized boundary replay ticks.
- T03 regression: `5,035,224` checks, 11,172 Hangul syllables, 81 policy profiles, overlap pixels 0.
- T02 regression: `2,365,194` checks, 19,683 combinations, SaveV2 96 bytes.
- Debug/Test/CTest/debug smoke/Contest clean build: PASS on local Windows x64 MSVC.
- Contest dist: one file, `129,024B`; internal and hard caps PASS; delta from T03 `0B`.
- Contest dependencies: KERNEL32, USER32, GDI32, WINMM only.

RIGHTS / DEPENDENCIES:

- No external library, font, image, audio, downloaded asset, runtime JSON parser, network, browser, or runtime AI added.
- T04 has no new runtime asset and writes nothing into `dist/contest` except the existing contest executable build output.

REVIEW:

- Independent read-only review result is recorded in `docs/evidence/T04/independent-review.txt` after candidate review.
- P0/P1 must be zero before standing local fast-forward approval is used.

STATUS: REVIEW_PENDING — not ACCEPTED and not yet integrated.
