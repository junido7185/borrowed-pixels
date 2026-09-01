# T03 INPUT

- Base branch: `integration/rc-2026-09-03`
- Base / API-freeze SHA: `7efaf404957949b178d36697e0be49a4b7521d96`
- Authority manifest SHA-256: `db043feb0abeeaff6136c420e99fc6bd45393886846fed8efb329f90575dc937`
- T03 amendment SHA-256: `9431ddb6e512ed7842f8c0c1d36ae7c33374cdb1824ab5037e4f87f0256c961f`
- Amendment-only commit: `60d9e11204208f3fc43d348029167ac21caef61b`
- Scope: primitive renderer, frozen palette/rect UI shell, Latin 5x7 bitmap font, Hangul 8x12 composition, and headless capture tests.
- Runtime API freeze: do not modify `bp_platform.h`, `bp_types.h`, ledger headers, or save headers. Amendment v2.2 permits only the narrow `bp_win32.cpp` renderer bridge.

## Changed-path allowlist

- `src/bp_render.h`, `src/bp_render.cpp`
- `src/bp_font.h`, `src/bp_font.cpp`
- `tests/bp_render_font_ui_tests.cpp`
- `src/bp_win32.cpp` only for the amendment's private-backbuffer renderer call
- `docs/evidence/T03/**`
- `docs/handoffs/T03/INPUT.md`, `docs/handoffs/T03/OUTPUT.md`
- `docs/STATUS.md`

## Acceptance map

- Primitive clipping and exact rect placement: headless pixel tests.
- Frozen nine-color palette: exhaustive capture pixel membership test.
- Latin font: required-codepoint, seven-row, row-range, metrics, and 4x atlas tests.
- Hangul font: all 11,172 modern syllables composed inside 8x12 plus representative glyph tests.
- UI shell: deterministic 320x180 TITLE, GLOBAL_LEDGER, COMBAT, SETTLEMENT, POLICY_AUDIT, and ENDING captures with frozen hashes and structural probes.
- POLICY_AUDIT amendment: two-line structured CASE labels, actual memory verb, four arrows/underlines, 76px exhaustive fit, 81-profile palette and lit-pixel overlap tests.
- Win32 bridge: launched contest EXE capture and exact 4x nearest comparison against the headless TITLE golden.
- Full regression: MSVC debug/test/CTest/direct/contest, smoke test, authority verification, exact dist measurement.
