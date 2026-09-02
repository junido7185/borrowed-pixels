# T05 INPUT

TICKET: T05 Charge, Projectile, hit, and Core

BASE SHA: `587f8fe9abfee78b5fbcbd160ece1408275458b7`

AUTHORITY:

- Verified BORROWED PIXELS v2.1 manifest and all six listed authority files.
- Approved T03 amendment remains present at SHA-256 `9431ddb6e512ed7842f8c0c1d36ae7c33374cdb1824ab5037e4f87f0256c961f`; it does not alter combat simulation.
- Relevant production clauses: 3.2–3.3, 6.3, 6.5, and 32.1, 32.4–32.6, 32.9–32.10.
- Runbook T05: `combat`; mass conservation and swept collision.

ALLOWLIST:

- `src/bp_combat.*`
- `tests/bp_combat_tests.cpp`
- `docs/STATUS.md`
- `docs/evidence/T05/**`
- `docs/handoffs/T05/**`

ACCEPTANCE:

- Actor-local Charge with exact-or-zero Shell transfer, exact stage ticks/masses, rollback, partial interception, release, recoil, and standard cooldowns.
- Fixed Projectile 32, Neutral 64, Cover 32 arrays; actor-local Charge one; monotonic nonzero stable IDs and index+generation handles.
- Integer swept TOI for Core, Charge, opposing Projectile, Shell, Cover, and boundary with frozen priority and stable-ID ties.
- Exact Shell/Cover/interception/boundary mass transactions with no creation or loss.
- Shell 41+ to 40-or-less grace 15; exposed Core HIT_FREEZE and immutable two-ledger snapshot.
- Neutral slot saturation first performs deterministic nearest-pair recovery; no mass deletion.
- Full Windows x64 MSVC Debug/Test/CTest/smoke/Contest/dist regression.

EXCLUSIONS:

- T06 owns Neutral damping, normal merge tick, reclaim force/intake, and HUD loans.
- T07 owns encounter placement and live gameplay connection.
- No authority, save layout, UI, copy, asset, dependency, network, browser, or runtime-AI change.
