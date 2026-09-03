# T07 OUTPUT

TICKET: T07

BASE SHA: `c4fef9de1663be43054b0cabad39b0c79813ff94`

CANDIDATE SHA: resolve `candidate/T07-final` after the evidence commit.

IMPLEMENTED:

- Exact Act 1 PATCH, PAIR, and VESSEL field/neutral/spawn/mass definitions.
- TITLE/BORROW/ledger/intro/objective/play/hit-freeze/retry/settlement/preview/
  hold/transit/response/return/ENOUGH scene framework with frozen timings.
- PATCH feed replacement and TAP; PAIR alternating CHECKSUM probes, tagged
  reclaimable protocol rebuild, forced SELECT, and 2,700-tick completion;
  VESSEL PRESS/BRACE/STAMP/RECLAIM policy and two-stage 144 minimum settlement.
- Authority 32.9 orchestration, deterministic complete-state hashing, palette
  rendering, live Win32 reachability, and focused/exhaustive regression tests.

AUTOMATED RESULTS:

- T07 2,066 checks PASS; T05 4,081 and all prior T02-T06 regressions PASS.
- T03 5,035,224 checks PASS, including 11,172 Hangul syllables and 81 policy
  profiles with zero bitmap overlap.
- Windows x64 MSVC configure, Test, CTest, Debug, debug smoke, and Contest PASS.
- Final contest EXE launch/capture/exit PASS; /OPT:REF MAP reachability PASS.
- dist/contest: one file, 183,808 bytes; both absolute caps PASS.
- Contest SHA-256: `59916441c7e2547d9a18bac2412241e67693e96ff051b2f318cec038c98d6492`.

GATES:

- Gate 3/4 subjective findings: HUMAN_REVIEW_DEFERRED, never claimed PASS.
- Independent final review: P0=0, P1=0, P2=0.
- Byte delta from T06: +54,784 bytes. Runbook section 12 requires explicit
  Human byte-budget reapproval before integration because the increase is >=25KB.

STATUS AT CANDIDATE: BYTE_BUDGET_APPROVAL_REQUIRED; not yet integrated at that point.

LOCAL INTEGRATION:

- Human Director explicitly reapproved the `+54,784` byte T07 increase under
  runbook section 12 and approved candidate `c04cab5551660446742a9a114f52d413fa6a3eb7`.
- `integration/rc-2026-09-03` fast-forwarded from
  `c4fef9de1663be43054b0cabad39b0c79813ff94` to that exact candidate.
- Post-integration configure, Test, direct suite, CTest, Debug, smoke, Contest,
  exact dist, and SHA checks all reproduced the recorded PASS results.
- No push, tag, publication, or submission was performed.

INTEGRATION STATUS: LOCALLY_INTEGRATED; not externally accepted or released.
