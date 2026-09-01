# T03 OUTPUT

TICKET: T03

BASE SHA: `7efaf404957949b178d36697e0be49a4b7521d96`

AMENDMENT-ONLY COMMIT: `60d9e11204208f3fc43d348029167ac21caef61b`

AMENDMENT SHA-256: `9431ddb6e512ed7842f8c0c1d36ae7c33374cdb1824ab5037e4f87f0256c961f`

REVIEWED IMPLEMENTATION SHA: `1578d483b9bf6cae1d1e3e9109309829bbe5ad8d`

CANDIDATE SHA: resolve `candidate/T03-final`; the controller reports the immutable final SHA after the review-evidence commit so the document does not contain a self-referential commit ID.

CHANGED PATHS:

- `src/bp_render.h`, `src/bp_render.cpp`
- `src/bp_font.h`, `src/bp_font.cpp`
- `src/bp_win32.cpp` (amendment-limited bridge only)
- `tests/bp_render_font_ui_tests.cpp`
- `docs/STATUS.md`
- `docs/evidence/T03/**`
- `docs/handoffs/T03/INPUT.md`, `docs/handoffs/T03/OUTPUT.md`
- Authority is isolated in amendment-only commit `60d9e112...`; no v2.1 authority file changed.

IMPLEMENTED:

- Clipped BGRA8 pixel primitives and the frozen nine-color palette.
- Complete authority-derived Latin 5x7 table (59 glyphs) and Hangul 8x12 segment composition.
- Deterministic TITLE, GLOBAL_LEDGER, COMBAT, SETTLEMENT, POLICY_AUDIT, and OPEN LEDGER ENDING reference screens.
- Amendment two-line POLICY_AUDIT CASE rendering from structured subject/choice values at exact origins.
- Actual WITNESS choice verb in the memory CASE plus the memory arrow and measured rule underline.
- Exhaustive approved CASE fit and pixel-mask non-overlap checks; all 81 policy profiles rendered in T03's supported EN mode.
- Narrow Win32 bridge: each `PresentBackbuffer` invocation renders into T01's existing private 320x180 buffer before the existing blit. No new buffer, allocation, platform API, or game state was added.

AUTOMATED RESULTS:

- T03: `5,035,224` checks without evidence writes; `5,035,232` with eight BMP writes.
- Latin authority table: 59/59, FNV `E3AAAAB5`.
- Hangul: 11,172/11,172, aggregate `160B0D5B`.
- Policy: 81 profiles, one T03-supported language mode, overlap pixels `0`.
- T02 regression: `2,365,194` checks; 19,683 combinations; SaveV2 96B.
- CTest: 1/1 PASS.
- Debug smoke: exit 0.
- Contest clean build twice: identical SHA-256 `8469ccdbc283e3748181e0e797706b40baa69a4d3ee9197645df4128764f3c1f`.
- Contest dist: `129,024B`, delta `+2,560B`, internal/hard cap PASS.
- Dependencies: KERNEL32, USER32, GDI32, WINMM only.
- v2.1 authority: 6/6 PASS; amendment hash PASS.

LIVE WIN32 RESULT:

- Launched `dist/contest/BorrowedPixels.exe`, observed physical client `1280x720`, captured `contest-title-live.png`, and closed normally with exit 0.
- Live capture SHA-256: `7880c6341d3c4dd17d877ac0f29c94d126ac2714bb6348a2b168212d961138f4`.
- Full comparison against the headless 320x180 TITLE scaled nearest 4x: `0` mismatched pixels.
- TITLE strings remain in the LTCG contest EXE and the reachable call site is `PresentBackbuffer -> RenderUiReference`.

RIGHTS / ASSET RESULT:

- No external library, font, image, audio, runtime JSON, network, browser, or runtime AI was added.
- Evidence BMP/PNG files are generated from project code and the local contest executable; they are not runtime assets and are not copied into `dist/contest`.

REMAINING CHECK:

- Independent read-only review reported P0=0, P1=0, and P2=0 for implementation commit `1578d483...`.
- Gate 3 subjective visual PASS remains Human Director-only and is not claimed here.

STATUS: READY_FOR_REVIEW — not ACCEPTED and not integrated.

LOCAL INTEGRATION:

- Human Director approved exact candidate `1e49151c1f0d70d20f77f7bb8aa77f2972de0f0f` on base `7efaf404957949b178d36697e0be49a4b7521d96`.
- `integration/rc-2026-09-03` was advanced with `git merge --ff-only` to the exact candidate.
- Full post-integration authority/build/test/smoke/contest/dist regression passed; see `docs/evidence/T03/integration-regression.txt`.
- Subjective Gate 3 visual judgment remains `HUMAN_REVIEW_DEFERRED`, not PASS.

INTEGRATION STATUS: LOCALLY_INTEGRATED — not externally pushed, tagged, published, or submitted.
