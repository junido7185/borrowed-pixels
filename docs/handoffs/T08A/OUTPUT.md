# T08A OUTPUT

TICKET: T08A

BASE SHA: `dd1791d501a49328d70a59f366f84100ae99a79f`

CANDIDATE SHA: resolve `candidate/T08A-final` after final evidence commit.

IMPLEMENTED:

- Exact KEEPER, ADAPTER, and FRAME battlefield masses, spawns, rectangles,
  neutral placements, encounter durations, and authoritative copy.
- SWEEPER cancellation/ProtocolEscrow/36-tick return and COMPACTOR
  indestructible conversion plus 180/60/60 movement.
- KEEPER structure persistence into FRAME, ADAPTER capability speed and
  behavior-derived 0..48 settlement, and FRAME scored action policy.
- FRAME state-derived observation, SHIFT, proximity-gated TOLL, material
  30-tick EVICT/CLAIM telegraphs, MOVE, structure-priority RECLAIM,
  structure-linked defense cap, and circle/AABB collision.
- Battlefield spawn, boundary, AABB, 20x20 flood-fill and adjacent-direction
  validation; common pause/HUD/hit-freeze/settlement scene flow; padding-free
  field-wise hashing; palette rendering and Win32 reachability.
- Minimal tested cross-ticket glue for account-aware Cover behavior and tagged
  Neutral reclaim results.

AUTOMATED RESULTS:

- T08A 72,445 checks PASS, including 49 ADAPTER ranges and 1,024 FRAME
  observations across all nonempty availability masks.
- All T02-T07 regressions PASS, including 11,172 Hangul syllables and the
  recorded 61,646,592 replay hash.
- Windows x64 MSVC configure, clean Test, direct suite, CTest, clean Debug,
  debug smoke, clean Contest, live contest launch, and MAP reachability PASS.
- dist/contest: one file, 206,336 bytes; internal and hard caps PASS.
- Contest SHA-256: `ce910ed6afda19a285de275511a5c70f3e54cb2747bfa85c5ddd07b18a33e831`.

GATES:

- Gate 3/4 subjective findings remain HUMAN_REVIEW_DEFERRED.
- Independent reviews reported P1 counts 6, 3, then 1; every finding was
  repaired and regression-tested. Final exact-SHA review remains pending.
- No push, tag, publication, submission, or merge was performed on the ticket branch.

STATUS: candidate preparation; not externally accepted or released.
