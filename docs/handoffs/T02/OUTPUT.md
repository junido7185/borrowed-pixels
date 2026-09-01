# T02 OUTPUT

TICKET: T02

BASE SHA: `0e0bfcab64f4eb61ce7ee0d504b735df3fa0c776`

CANDIDATE SHA: resolve the committed tip of `ticket/T02-ledgers-save`; the controller reports the exact immutable SHA after commit.

AUTHORITY HASHES:

- Runtime contract: `9221d45d7f0bdead2cc211cc38329c0cdad12799c02f99238eb5c1a0ce56861e`
- Font glyphs: `6adee18ff38166820f414f4748e152cc70823edf80252d117818cab9837a5fcf`
- Production spec: `99c2c285fbe15dd3861c51c99c5fb71df74cbf6bda4c20eba99667f69c2fb343`
- Agent runbook: `4e77e88e660a5f4861d246132df0fa0a9f9a2a71d1831edae8d3d39d05c77995`
- UI atlas SVG: `4e15fd4a8fc1ecd36066f4f7b57962935a44dce46ce3ef6ad39b634694d18638`
- UI atlas PNG: `4e5ef2c6840c2b328d82a098baeb03afae0da2061cc8d206bb25ee329e98bfd2`

CHANGED PATHS:

- `src/bp_types.h`
- `src/bp_physical_ledger.h`, `src/bp_physical_ledger.cpp`
- `src/bp_claim_ledger.h`, `src/bp_claim_ledger.cpp`
- `src/bp_save.h`, `src/bp_save.cpp`
- `tests/bp_ledger_save_tests.cpp`
- `docs/STATUS.md`
- `docs/evidence/T02/**`
- `docs/handoffs/T02/OUTPUT.md`

IMPLEMENTED:

- Frozen ClaimOwner and PhysicalAccount order plus core narrative/policy/encounter types.
- Exact 1,440 initial ClaimLedger and encounter PhysicalLedger initialization.
- Exact-or-zero single ledger transfers with byte-identical failure behavior.
- Atomic ApplySettlement for all nine three-way decisions, including VESSEL's mandatory title/minimum-persistence transfers.
- Field-by-field SaveV2 little-endian 96-byte serialization/deserialization, reserved-zero enforcement, FNV-1a32, and non-mutating rejection.
- Exhaustive ledger, settlement, rollback, corruption, partial-input, and golden tests.

RESULTS:

- `T02_PASS checks=2365194 combinations=19683 save_bytes=96`
- CTest: 1/1 PASS
- Contest bytes: `126,464B`; delta from T01: `0B`
- Contest SHA-256: `fb2ae29fde83e564aaee4ff214da4750ef8e401381f31d139f6ffcce5b15e93e`; two clean builds matched
- Dependencies: KERNEL32, USER32, GDI32, WINMM only
- Authority: 6/6 PASS

EVIDENCE:

- `docs/evidence/T02/commands.txt`
- `docs/evidence/T02/test-results.txt`
- `docs/evidence/T02/save-golden.hex`
- `docs/evidence/T02/settlement-table.txt`
- `docs/evidence/T02/dist-bytes.txt`
- `docs/evidence/T02/dependencies.txt`

KNOWN RISKS / DEFERRED OWNERSHIP:

- T02 serializes fields only. T10 owns `%LOCALAPPDATA%`, save.tmp write+flush+replace, save.invalid rename, save timing, and UI failure notices.
- `entityStatus`, `role`, `adjudicatedMass`, and progression semantics are preserved as frozen fields; their encounter/progression meanings remain with T07-T10 because the authority does not assign a T02 byte encoding beyond storage.
- Replay/game-state hashing is T04-owned and was not claimed here.

STATUS: READY_FOR_REVIEW
