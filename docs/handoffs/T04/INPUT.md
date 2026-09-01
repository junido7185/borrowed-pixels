# T04 INPUT

TICKET: T04 fixed tick, movement, collision, and dash

BASE SHA: `53a3cac34427f98ec281754a4a19e6980686987d`

AUTHORITY:

- BORROWED PIXELS v2.1 manifest and all six listed files.
- Approved T03 amendment remains present at SHA-256 `9431ddb6e512ed7842f8c0c1d36ae7c33374cdb1824ab5037e4f87f0256c961f`; it does not change T04 simulation rules.
- Relevant production clauses: 14.5 and 32.1–32.3, 32.8–32.9.
- Runbook row T04: `game,input,math`; replay hash and boundary tests.

ALLOWLIST:

- `src/bp_game.*`
- `src/bp_input.*`
- `src/bp_math.h`
- `tests/bp_game_input_math_tests.cpp`
- `docs/STATUS.md`
- `docs/evidence/T04/**`
- `docs/handoffs/T04/**`

ACCEPTANCE:

- Integer-only Q8.8 conversion, fixed 60 Hz tick, stable deterministic hash.
- Pause → HUD reallocation → Dash → Fire → Reclaim → Move priority and nine-tick Fire/Reclaim switch lock.
- Player acceleration, axis friction, shell-dependent speed, circle/boundary and circle/AABB collision resolution.
- IMPULSE dash: eight ticks, 20 pixels, first five movement ticks projectile immunity, cooldown 144, no mass use, boundary stop.
- Claim and physical ledgers remain valid; rejected input leaves state byte-identical.
- Full Debug/Test/CTest/smoke/Contest/dist regression with exact bytes.

EXCLUSIONS:

- No Charge/Projectile/Core/Neutral implementation (T05).
- No encounter/gameplay presentation connector.
- No game-rule, authority, save-layout, asset, dependency, network, browser, or runtime-AI change.
