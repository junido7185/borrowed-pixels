# BORROWED PIXELS v2.2 — T03 UI/PLATFORM AMENDMENT

## 0. Approval and scope

- Status: `HUMAN_DIRECTOR_APPROVED`
- Approval date: `2026-09-01`
- Ticket: `T03 / ticket/T03-render-ui`
- Integration base SHA: `7efaf404957949b178d36697e0be49a4b7521d96`
- Purpose: resolve the two validated T03 blockers without changing story copy, policy meaning, font metrics, canvas geometry, simulation, ledgers, save data, or later-ticket behavior.

This amendment has precedence over BORROWED PIXELS v2.1 only for the clauses named below:

1. `POLICY_AUDIT` CASE text wrapping and its memory-row arrow/underline.
2. T03 file ownership needed to connect the renderer to the existing Win32 backbuffer.
3. The one-time recovery of the already-preserved dirty T03 worktree.

Every other v2.1 authority value and SHA remains unchanged. An implementer must not treat this amendment as general permission to redesign UI or platform APIs.

---

## 1. Decision A — POLICY_AUDIT CASE uses two fixed lines

### 1.1 Preserved authority

The following remain unchanged:

- canvas `320×180`;
- Latin cell `5×7`, advance `6`, space advance `4`, line height `8`;
- `POLICY_AUDIT` title, axis rows, memory row, and continue rectangles;
- `caseX=34`, `arrowX=116`, `ruleX=130`;
- all CASE subject names, choice verbs, RULE copy, reveal timing, colors, z-order, and the `CASE → RULE` meaning;
- the three axis row rectangles `[28,28,264,28]`, `[28,62,264,28]`, `[28,96,264,28]` and memory row `[28,130,264,20]`.

No abbreviation such as `CONT.`, font compression, scale reduction, horizontal coordinate change, ellipsis, or silent clipping is approved.

### 1.2 Authoritative split rule

The displayed CASE is represented as `subject` plus `choice` and rendered as:

```text
SUBJECT
/ CHOICE
```

Examples:

```text
PATCH
/ CONTINUE

ADAPTER
/ NEED

WITNESS
/ PRESERVE
```

When the WITNESS result requires anonymization, the first line is exactly `CASE // HASH` and the second line still contains the choice verb:

```text
CASE // HASH
/ CONTINUE
```

Implementations must use the structured subject and choice values. If only a preformatted CASE string exists, split on the **last** occurrence of ` / ` so the internal `//` in `CASE // HASH` is never treated as the separator.

The approved subject tokens are `PATCH`, `PAIR`, `VESSEL`, `KEEPER`, `ADAPTER`, `FRAME`, `INDEX`, `WITNESS`, `CURRENT`, and `CASE // HASH`. The approved choice tokens are:

```text
RECLAIM CONTINUE UNMARK
SELECT SEPARATE LINK
CLAIM RETURN RELEASE
PRIVATE MAINTAIN COMMON
EQUAL NEED OUTPUT
SELF RESTORE OPEN
OPTIMIZE REFUSE REWRITE
PRESERVE COMPRESS ERASE
HOLD CIRCULATE
```

The memory CASE must show the actual WITNESS choice verb: `PRESERVE`, `COMPRESS`, or `ERASE`. `FULL`, `SUMMARY`, and `NONE` are policy results, not CASE verbs. Therefore the v2.1 SVG example `WITNESS / FULL` is superseded by `WITNESS / PRESERVE` for the FULL example. If NONE anonymizes the subject, the second line remains `/ ERASE`.

### 1.3 Exact bitmap text origins

Coordinates below are top-left pixel origins for the runtime 5×7 bitmap renderer, not SVG baselines.

| Row | CASE line 1 | CASE line 2 | Arrow `>` | RULE | Underline y |
|---|---:|---:|---:|---:|---:|
| Axis 1 `[28,28,264,28]` | `(34,31)` | `(34,43)` | `(116,38)` | `(130,31)` | `43` |
| Axis 2 `[28,62,264,28]` | `(34,65)` | `(34,77)` | `(116,72)` | `(130,65)` | `77` |
| Axis 3 `[28,96,264,28]` | `(34,99)` | `(34,111)` | `(116,106)` | `(130,99)` | `111` |
| Memory `[28,130,264,20]` | `(34,131)` | `(34,140)` | `(116,136)` | `(130,131)` | `144` |

The CASE text budget is 76 pixels per line, covering x=34 through x=109. The arrow begins at x=116. With the frozen Latin metrics, the longest approved second line (`/ CIRCULATE`) and the anonymized first line (`CASE // HASH`) both fit. Add an exhaustive measurement test over every approved subject/choice combination relevant to each policy axis.

The rule underline starts at x=130 and ends at `130 + MeasureText(rule) - 1`, using the existing frozen measurement routine and never extending outside the row. The memory row now explicitly renders the same `>` arrow and animated rule underline required by production spec §22.4/§25.6; their omission from the v2.1 SVG panel is superseded by this amendment.

CASE and RULE text must never share a lit pixel. CASE text must never share a lit pixel with the arrow. This is a bitmap-pixel assertion, not a bounding-box estimate.

### 1.4 Reference and golden behavior

For `POLICY_AUDIT` only, this amendment supersedes the single-line CASE rendering in `BORROWED_PIXELS_UI_REFERENCE_ATLAS_v2.1.svg/.png`. All unaffected geometry and styling must still match the v2.1 atlas.

The T03 golden capture becomes authoritative evidence only when it demonstrates:

- all four two-line CASE labels;
- the three axis arrows plus the memory-row arrow;
- four animated RULE underlines;
- palette compliance and zero cross-column lit-pixel overlap;
- unchanged title, row rectangles, rule copy, and continue region.

---

## 2. Decision B — narrow T03 Win32 render bridge

### 2.1 Allowlist exception

T03 may modify `src/bp_win32.cpp` in addition to its existing renderer, font, and UI-test allowlist. This exception exists only to connect the already-tested T03 renderer to T01's existing private `320×180` backbuffer.

Not approved:

- modifying `src/bp_platform.h`;
- changing window creation, DIB format, integer scaling, input, focus pause, timing, audio, or message-loop semantics;
- adding a second backbuffer, heap-owned frame copy, GDI+ layer, external asset, third-party library, runtime parser, or new platform abstraction;
- implementing T04 game state or later-ticket behavior.

### 2.2 Required bridge behavior

Inside `src/bp_win32.cpp`, call the existing tested T03 renderer exactly once per presented frame after the existing backbuffer is available and before the existing Win32 present/blit operation.

- Reuse the backbuffer pointer, width, height, stride, and pixel format already owned by T01.
- Include and call the T03-owned public renderer interface; do not duplicate drawing code in `bp_win32.cpp`.
- If the current renderer interface cannot accept the private buffer, the smallest POD surface view may be added to a T03-owned renderer header. It may contain only pixel pointer, width, height, and stride in pixels. It must contain no HWND/HDC/Win32 handle and no ownership or allocation behavior.
- Preserve the renderer entry point already exercised by headless tests. If an adapter is necessary, it must delegate to that entry point rather than create a separate render path.
- Until a later ticket provides live game state, use the renderer's existing deterministic UI-shell/default model. Do not invent simulation state in the platform layer.
- Rendering may mutate only the supplied pixels and renderer-local presentation state. It must not mutate either 1,440 ledger, SaveV2, simulation state, or input state.

The contest build must retain and execute this renderer path under `/OPT:REF`. Finding strings in the EXE is diagnostic evidence only; a launched contest EXE capture showing nonblank expected UI plus a reachable call site is required for PASS.

---

## 3. One-time dirty-worktree recovery authorization

The runbook's normal clean-start rule remains in force for all other tickets. For this T03 recovery only, the preserved uncommitted work on `ticket/T03-render-ui` may be resumed rather than discarded, because it contains the already-reported 368,658 passing checks and 11,172 Hangul syllable checks.

Before any new code edit, the T03 session must:

1. confirm the branch is exactly `ticket/T03-render-ui`;
2. confirm `7efaf404957949b178d36697e0be49a4b7521d96` is an ancestor;
3. record `git status --porcelain=v2`, unstaged diff, and staged diff under `docs/evidence/T03/`;
4. report every changed path and classify it as pre-amendment T03 work, amendment/evidence, or unexpected;
5. stop again if an unexpected path exists;
6. commit this amendment alone as a human-authority commit, without including the preserved T03 implementation changes;
7. record that amendment commit SHA in the final T03 handoff.

This recovery exception does not waive the final clean-worktree requirement. Before `candidate/T03-final` review, all approved T03 implementation/evidence must be committed and the worktree must be clean.

---

## 4. T03 completion gates after amendment

T03 may report `READY_FOR_REVIEW`, never `ACCEPTED`, only after all of the following are evidenced:

1. The previously passing 368,658 headless checks still pass, or the current authoritative test count is explained with no removed coverage.
2. All 11,172 modern Hangul syllable compositions still pass.
3. Exhaustive approved POLICY_AUDIT CASE measurements fit the 76-pixel per-line budget.
4. Bitmap overlap assertions report zero CASE/arrow and CASE/RULE shared lit pixels for every policy profile and language mode supported at T03.
5. The amended `policy-audit.bmp` passes automated structural/palette checks and human visual review.
6. Debug, test, and contest builds pass using the exact project command set already recorded in T03 evidence.
7. A launched contest EXE visibly renders the expected T03 UI through T01's real private backbuffer.
8. `/OPT:REF` no longer removes the renderer call path. String search alone is not sufficient proof.
9. Changed code paths are limited to the original T03 allowlist plus `src/bp_win32.cpp`; authority/handoff/evidence files are separately classified.
10. Exact `dist/contest` extracted-byte measurement is recorded and remains within the current gate and the hard `1,474,560`-byte limit.
11. Independent reviewer reports P0=0 and P1=0 for the new candidate SHA.
12. No merge, tag, or integration-branch update occurs without HUMAN DIRECTOR approval.

---

## 5. Resume directive for the T03 Codex session

```text
Resume BORROWED PIXELS T03 on ticket/T03-render-ui.

Authority decision:
- Read docs/authority/BORROWED_PIXELS_AMENDMENT_v2.2_T03_UI_PLATFORM.md in full before editing.
- It is HUMAN_DIRECTOR_APPROVED and overrides v2.1 only for POLICY_AUDIT CASE wrapping, memory-row arrow/underline, the narrow src/bp_win32.cpp renderer bridge, and this one-time dirty recovery.
- Integration base remains 7efaf404957949b178d36697e0be49a4b7521d96. Also report the amendment-only commit SHA.

First preserve and audit the existing dirty T03 state exactly as Amendment §3 requires. Do not reset, discard, regenerate, or silently replace the passing implementation. Stop if any unexpected changed path exists.

Then resolve both blockers only as authorized:
1. Render every POLICY_AUDIT CASE as SUBJECT on line 1 and / CHOICE on line 2 at the exact bitmap origins in Amendment §1.3. Preserve all copy, x coordinates, font metrics, rules, timing, and unaffected atlas geometry. Add the memory-row arrow and underline. Add exhaustive fit and lit-pixel non-overlap tests.
2. Modify src/bp_win32.cpp only as the platform exception. Route T01's existing private 320x180 backbuffer into the already-tested T03 renderer once per presented frame before the existing blit. Do not modify bp_platform.h or any T01 behavior.

Re-run the exact T03 debug/test/contest/dist command set and all amended gates in §4. Produce updated evidence, commit the approved T03 changes, create/update candidate/T03-final only if the worktree is clean, and return READY_FOR_REVIEW with candidate SHA, amendment SHA, changed paths, command-by-command PASS/FAIL/NOT_RUN, exact dist bytes, and remaining human checks.

Do not merge, tag, update integration, abbreviate copy, move columns, change font advance, or claim ACCEPTED.
```
