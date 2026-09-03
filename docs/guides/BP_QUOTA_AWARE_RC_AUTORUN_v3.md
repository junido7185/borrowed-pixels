# BORROWED PIXELS — QUOTA-AWARE AUTONOMOUS RC CONTROLLER v3

Purpose: finish BORROWED PIXELS from the actual repository state to a locally verified Windows x64 contest RC with minimum Codex quota waste.

Repository:
C:\Users\sdjsd\Desktop\BORROWED_PIXELS

This file changes operating procedure only. It does not change authority, frozen game values, contest constraints, or truthful PASS requirements.

---

## 1. Current verified resume point

Expected snapshot at installation:
- ticket/T08B-act3 HEAD: 6e0388502118ec4cc1e9715d89481a723a00bfb0
- integration/rc-2026-09-03 HEAD: 98f298636195ecbce50c72a9592d93bcf5ff808f
- T00-T08A integrated
- T08B implementation + repair complete
- T08B final changed SHA requires one final read-only review
- worktree clean
- last measured contest size: 224,256B

Always verify actual Git state. If repository evidence is newer, newer verified repository state wins for progress. Authority remains the only game-rule authority.

Do not redo accepted work.

---

# 2. QUOTA-AWARE EXECUTION MODEL

Codex quota is treated as scarce reasoning budget.

Spend it on:
1. unresolved implementation decisions;
2. code changes;
3. first-cause failure diagnosis;
4. one final review of executable changes.

Do NOT spend it repeatedly on:
- restating authority;
- rereading unchanged authority documents in full;
- repeating successful logs;
- repeating historical ticket tests only for narration;
- preliminary reviews before implementation is final;
- explaining plans to the Human Director;
- rebuilding accepted tickets;
- size optimization while far below the internal cap;
- formatting evidence beyond what is needed for reproducibility.

The repository is persistent memory.

At every resume:
1. read `docs/STATUS.md`;
2. inspect branch, HEAD, `git status --short`, recent log, and current ticket evidence;
3. read this controller;
4. execute the first unfinished action immediately.

If the authority tree has not changed since the previous verified checkpoint, DO NOT reread every authority file from beginning to end. Reuse the previously verified authority hash state and load only the exact sections needed by the active ticket. If authority changed, invalidate the cache and fully reverify it.

---

# 3. EVIDENCE CACHE — THE MAIN QUOTA SAVER

A successful machine result may be reused only when all inputs that affect it are unchanged.

Cache identity:
- executable source/config tree;
- authority tree;
- compiler/toolchain identity;
- build configuration;
- exact command.

Examples:
- moving an already-tested candidate SHA by local `--ff-only` changes a branch ref, not its executable tree. Do not rerun the same expensive suite solely because the ref name changed.
- a docs-only review/evidence commit does not invalidate a review of the unchanged executable tree if the diff is restricted to approved evidence/status/handoff files.
- any source, CMake, compiler flag, frozen authority, or relevant generated-code change invalidates affected machine evidence.

Never reuse FAIL.
Never turn NOT_RUN into PASS.
T14 always performs one clean final RC validation regardless of cache.

Record concise evidence reuse as:
`EVIDENCE_REUSED: <command/result> because executable-tree=<hash> authority-tree=<hash> toolchain=<id> unchanged`

---

# 4. REVIEW ECONOMY

Perform exactly one independent read-only review after the executable implementation is final.

During repair:
- focused reproduction;
- focused fix;
- focused recheck.

Do not repeatedly invoke reviewer contexts between small edits.

If final review finds P0/P1:
- repair;
- rerun affected checks;
- perform one new final review of the changed executable tree.

A review report may then be committed as docs-only evidence without recursively requiring a review of the report commit itself, provided the executable/config/authority tree is unchanged.

---

# 5. OUTPUT ECONOMY

Full tool output goes to `docs/evidence/.../*.verbose.log`.

Chat/Goal output should contain only:
- action taken;
- command;
- exit code/PASS/FAIL;
- exact bytes/hash when relevant;
- first actionable error;
- next action.

On failure, inspect the first relevant error or at most the small tail/context needed to diagnose it. Do not inject megabytes of successful test output into model context.

A plan is never a stopping condition.

---

# 6. CONTINUOUS PIPELINE

T08B FINAL REVIEW
→ local integration
→ T09
→ T10
→ TITLE-to-ending reachability
→ T11
→ T12
→ T13
→ T14
→ RC_CANDIDATE

One writer at a time.

Normal build/test/review failure enters automatic repair.

Stop only for:
- mutually exclusive higher-precedence authority requirements;
- unavailable required Windows tool/filesystem permission/external input;
- three materially different failed fixes to the same root cause where continuing would require authority change;
- external push/payment/publication/submission;
- completed RC candidate.

Gate 3/4 and unavailable subjective Gate 5 remain HUMAN_REVIEW_DEFERRED, never fake PASS, and do not stop machine-verifiable completion under the standing Human Director delegation.

---

# 7. T08B CLOSEOUT

Expected executable HEAD:
`6e0388502118ec4cc1e9715d89481a723a00bfb0`

This SHA is a repair after earlier review findings.

Therefore:
1. inspect the final diff;
2. run only checks needed to confirm repaired findings plus any invalidated T08B checks;
3. one final independent read-only review;
4. require P0/P1=0;
5. commit final evidence if required;
6. create/update `candidate/T08B-final`;
7. locally fast-forward integrate;
8. reuse unchanged executable evidence where cache identity proves equivalence;
9. immediately begin T09.

Do not recreate T08B.

---

# 8. T09 IMPLEMENTATION BLUEPRINT — PRECEDENT

Primary new files:
- `src/bp_precedent.h`
- `src/bp_precedent.cpp`
- `tests/bp_precedent_tests.cpp`

CMake already globs `src/*.cpp` and `tests/*.cpp`; do not edit CMake merely to register these files.

Use existing:
- ClaimLedger / PhysicalLedger / NarrativeState / PolicyProfile
- GameState
- CombatState
- ReclaimState
- HudLoanSystem
- RenderPolicyAudit
- existing combat projectile/reclaim/collision APIs

Minimal route glue:
`bp_win32.cpp` may only connect `Act3Scene::Complete` → `InitializePrecedentRuntime(...)` and add tick/render dispatch. Mark CROSS_TICKET_GLUE.

Do not modify Act3 to become PRECEDENT.

## 8.1 Recommended API

```cpp
enum class IdentityPolicy : uint8_t { Material=0, Continuity, Relation };
enum class DistributionPolicy : uint8_t { Equality=0, Need, Utility };
enum class GovernancePolicy : uint8_t { Rule=0, Judgment, Commons };
enum class MemoryPolicy : uint8_t { Full=0, Summary, None };

struct PolicyDecision {
    PolicyProfile profile;
    EncounterId decisiveEncounter[4];
    ChoiceId decisiveChoice[4];
};

enum class PrecedentScene : uint8_t {
    RoleAssign = 0,
    PolicyAudit,
    Playing,
    HitFreeze,
    Retry,
    CoreUnfold,
    Complete,
    Fatal,
};

enum class PrecedentAction : uint8_t {
    Evade = 0,
    Move,
    Reclaim,
    Tap,
    Cut,
    Claim,
    Foreclose,
};

struct PrecedentRuntime {
    ClaimLedger claimLedger;
    NarrativeState narrative;
    PolicyDecision policy;

    GameState movement;
    CombatState combat;
    ReclaimState reclaim;
    HudLoanSystem hudLoan;

    SlotHandle covers[10];
    SlotHandle neutrals[16];
    SlotHandle tetherNodes[3];
    SlotHandle voteNodes[3];

    PrecedentScene scene;
    PrecedentAction action;
    PrecedentAction pendingAction;

    uint32_t globalTick;
    uint32_t encounterTick;

    uint16_t sceneTicks;
    uint16_t phaseTicks;
    uint16_t outputDamage[2];
    uint16_t outputReclaim[2];
    uint32_t outputMovementQ8[2];

    uint16_t previousPlayerReclaim;
    uint16_t markedOriginMass;
    uint16_t markedOriginTicks;
    uint16_t reclaimPriorityTicks;

    uint8_t auditRow;
    uint8_t phase;
    uint8_t exceptionShown;
    uint8_t exceptionDeniedShown;
    uint8_t borrowedActive;
};
```

Keep runtime smaller if fields can be derived from `encounterTick`. Do not add timers for exact periodic rules when modulo arithmetic is sufficient.

Recommended functions:

```cpp
ChoiceId ChoiceForEncounter(const NarrativeState&, EncounterId) noexcept;

bool DerivePolicyDecision(
    const NarrativeState& narrative,
    PolicyDecision& out) noexcept;

PolicyAuditView BuildPolicyAuditView(
    const PolicyDecision& decision) noexcept;

bool InitializePrecedentRuntime(
    PrecedentRuntime& state,
    const ClaimLedger& claims,
    const NarrativeState& narrative) noexcept;

bool TickPrecedentRuntime(
    PrecedentRuntime& state,
    const InputFrame& input) noexcept;

void RenderPrecedentRuntime(
    PixelBuffer buffer,
    const PrecedentRuntime& state) noexcept;

uint32_t PrecedentRuntimeHash(
    const PrecedentRuntime& state) noexcept;
```

## 8.2 Policy derivation

Read each encounter from:
`(choiceBits >> (encounterId*2)) & 3`.

IDENTITY:
PATCH:
- RECLAIM / First => MATERIAL +2
- RECOGNIZE / Second => CONTINUITY +2
- UNMARK / Third => RELATION +2
PAIR:
- SELECT / First => MATERIAL +2
- SEPARATE / Second => CONTINUITY +2
- LINK / Third => RELATION +2

Tie: later influencing encounter wins, therefore PAIR beats PATCH on equal maximum.

DISTRIBUTION:
KEEPER:
- First => UTILITY +1
- Second => NEED +1
- Third => EQUALITY +1
ADAPTER:
- First => EQUALITY +3
- Second => NEED +3
- Third => UTILITY +3
INDEX:
- First => UTILITY +2
- Second => NEED +2
- Third => EQUALITY +2

Tie order:
INDEX > ADAPTER > KEEPER.

GOVERNANCE:
FRAME:
- First => JUDGMENT +2
- Second => RULE +2
- Third => COMMONS +2
CURRENT:
- First => JUDGMENT +2
- Second => RULE +2
- Third => COMMONS +2

Tie:
CURRENT > FRAME.

MEMORY:
WITNESS First=PRESERVE=>FULL
Second=COMPRESS=>SUMMARY
Third=ERASE=>NONE.

Implement a generic three-score accumulator:
`score`, `latestEncounter`, `latestChoice`.
Winner comparison:
1. larger score;
2. if equal, larger chronological EncounterId.

Do not use enum order as a hidden tie-breaker.

## 8.3 POLICY AUDIT

Do not implement a new renderer.

Call existing `RenderPolicyAudit`.

State flow:
- RoleAssign: 60 ticks.
- PolicyAudit row 0..3.
- each row reveal cannot advance before 36 ticks.
- row remains/auto advances at 90 ticks.
- confirm may advance during [36,90).
- after fourth row => populate PRECEDENT battlefield and Playing.

Build four rows from the same `PolicyDecision` used by combat.

For WITNESS/ERASE memory NONE:
subject=`CASE // HASH`
choice=`ERASE`
rule=`NO CASE MAY SPEAK`.

This prevents renderer/combat policy divergence.

## 8.4 Battlefield initialization

Runtime contract:
- player spawn (48,92)
- opponent spawn (264,90)
- PRECEDENT 256
- cover 320 = 10×32
- neutral 128 = 16×8

Use `InitializePhysicalLedger(...)`.
Use 10 exact covers and 16 exact neutrals from runtime contract.
Use existing `InitializeCombatState`, `AddCover`, `AddNeutral`.

PRECEDENT 256 is temporary borrowed physical mass and must return to LockedBands after battle.

Track PRECEDENT provenance using existing slot ownership/provenance where possible:
- OpponentCore/OpponentShell;
- opponent-owned Projectile;
- neutral `opponentMissMass`;
- explicit policy-node handles funded from a known PRECEDENT account.

Implement one `FoldPrecedentBorrow(...)` transaction at core defeat that removes PRECEDENT-owned active entities and returns exactly 256 to LockedBands. Verify exact 256 before committing. If not exact, fail atomically rather than compensating.

## 8.5 Phases

Determine from initial opponent shell budget:
- phase 0: 100..67%
- phase 1: 66..34%
- phase 2: 33..0%

Phase 0:
identity only.

Phase 1:
identity + distribution.
First 120 ticks display distribution policy + CASE→RULE.

Phase 2:
identity + distribution + governance + memory.

Core hit:
- do not explode;
- enter CoreUnfold;
- 36 ticks;
- fold 256 borrowed PRECEDENT mass;
- then `Complete`.

T10 owns final allocation/endings.

## 8.6 Base AI

Every 12 ticks compute signed int32 Q0.8 scores.

Exact formulas from authority:
EVADE
RECLAIM
MOVE
TAP
CUT
CLAIM
FORECLOSE.

JUDGMENT:
- observe current state;
- reaction delay exactly 12 ticks;
- switch only if new score >= current score +38.

RULE:
- fixed deterministic action cycle;
- no utility adaptation.

COMMONS:
- 3 vote nodes radius 24;
- majority physical mass selects TAP / CLAIM / RECLAIM;
- tie preference COMMON → PLAYER → OPPONENT, least aggressive result.

Do not create new attacks. Map actions to existing combat primitives:
- MOVE/EVADE => movement target/velocity;
- RECLAIM => TickReclaim opponent-active;
- TAP/CUT/CLAIM/FORECLOSE => SpawnScriptedProjectile with frozen masses/timing from authority.

## 8.7 Policy modules

Apply only according to phase.

IDENTITY:
- ORIGIN_LOCK 720 ticks, mark Shell16 for 90
- FORK 600 ticks, 180 duration, real-core pulse each30
- TETHER 3×16; core collision disabled until all gone

DISTRIBUTION:
- EQUALIZE each600, half difference floor8 max32 → Neutral
- LOW_MASS_PRIORITY each600, Neutral16 to lower side or exact transfer
- OUTPUT_SCORE each480, damage+reclaim+movementPx/8; lower side Shell16→Neutral; if PLAYER lower and HUD loan open then maturity -120

GOVERNANCE:
- FIXED_CYCLE
- UTILITY_ADAPT
- VOTE_NODES

MEMORY:
- REPLAY_NAMED
- REPLAY_AGGREGATE
- NO_REPLAY; no replacement attack

## 8.8 T09 tests

Must include:
- all 3^9=19,683 choice combinations derive valid profile;
- exactly 81 unique PolicyProfiles;
- latest-encounter tie cases explicitly;
- all 81 initialize successfully;
- same seed/profile/input => same hash;
- 81 profiles no deterministic softlock;
- exact phase and module boundary ticks;
- physical total 1440;
- claim total 1440;
- PRECEDENT exact borrowed 256 returns;
- failure/slot saturation exact-or-zero;
- PolicyAudit CASE and combat policy originate from same decision object.

---

# 9. T10 IMPLEMENTATION BLUEPRINT — FINAL FLOW

Primary new files:
- `src/bp_narrative.h`
- `src/bp_narrative.cpp`
- `tests/bp_narrative_tests.cpp`

Existing `bp_save.h/.cpp` remains the 96-byte serializer authority. Extend it only for frozen T10 semantics/helpers; do not redesign format.

Minimal `bp_win32.cpp` glue:
- render/tick dispatch for NarrativeRuntime;
- PRECEDENT Complete → Narrative final allocation;
- Act1 EnoughEnding → Narrative ENOUGH flow;
- OS file adapter for safe SaveV2 read/write;
- new-run/continue route dispatch.

Do not rewrite Act1/Act2/Act3 into one new runtime.

## 9.1 Minimal top-level route

Current:
Act1 → Act2 → Act3

Extend:
Act1
  → Act2
  → Act3
  → PrecedentRuntime
  → NarrativeRuntime final allocation/ending/credits/title

Special:
Act1 `EnoughEnding`
  → NarrativeRuntime ENOUGH confirmation/ending
  → title with Continue From Vessel.

PlatformState minimal additions:
```cpp
PrecedentRuntime precedent;
NarrativeRuntime narrativeFlow;
bool precedentActive;
bool narrativeActive;
```

Render priority:
Narrative > Precedent > Act3 > Act2 > Act1.

Tick priority same.

This is smaller and safer than replacing current Act routers.

## 9.2 Narrative API

```cpp
enum class EndingId : uint8_t {
    Enough=0,
    Rule,
    Hand,
    OpenLedger,
    One,
};

enum class FinalChoice : uint8_t {
    Ratify=0,
    Override,
    Open,
    TotalClaim,
};

enum class NarrativeScene : uint8_t {
    EnoughConfirm = 0,
    FinalAllocation,
    FinalPreview,
    FinalConfirm,
    FinalCommit,
    Ending,
    Credits,
    Title,
    AuditTrace,
    Fatal,
};

struct NarrativeRuntime {
    ClaimLedger claimLedger;
    NarrativeState narrative;

    NarrativeScene scene;
    EndingId ending;
    FinalChoice focused;
    FinalChoice committed;

    uint32_t globalTick;
    uint16_t sceneTicks;
    uint16_t holdTicks;

    uint8_t oneUnlocked;
    uint8_t openUnlocked;
    uint8_t vesselContinueAvailable;
    uint8_t auditTraceUnlocked;
    uint8_t fullEndingReached;
};
```

Recommended functions:

```cpp
bool InitializeFinalNarrative(
    NarrativeRuntime&,
    const ClaimLedger&,
    const NarrativeState&) noexcept;

bool InitializeEnoughNarrative(
    NarrativeRuntime&,
    const ClaimLedger&,
    const NarrativeState&) noexcept;

bool OpenLedgerUnlocked(
    const ClaimLedger&,
    const NarrativeState&) noexcept;

bool TotalClaimUnlocked(
    const NarrativeState&) noexcept;

bool ApplyFinalChoice(
    NarrativeRuntime&,
    FinalChoice) noexcept;

bool TickNarrativeRuntime(
    NarrativeRuntime&,
    const InputFrame&) noexcept;

void RenderNarrativeRuntime(
    PixelBuffer,
    const NarrativeRuntime&) noexcept;

uint32_t NarrativeRuntimeHash(
    const NarrativeRuntime&) noexcept;
```

## 9.3 OPEN predicate

True when:
- COMMON claim >=64
OR
- FRAME choice == Third / OPEN.

Row always visible.
When false, disabled row says:
`NO COMMON CLAIM EXISTS`.

## 9.4 ONE predicate

All nine exact acquisitive choices:
PATCH First
PAIR First
VESSEL First
KEEPER First
ADAPTER Third
FRAME First
INDEX First
WITNESS Third
CURRENT First

If true:
- reveal TOTAL CLAIM for 48 ticks;
- final commit transfers ClaimLedger only until PLAYER=1440;
- other claim accounts =0;
- ClaimLedgerTotal remains 1440;
- physical ledger is not collapsed into claim ownership;
- ending runtime ignores movement as relative displacement;
- Fire creates no Charge.

## 9.5 ENOUGH

Existing Act1 currently enters `EnoughEnding`; T10 should own the authoritative final flow rather than rewriting all Act1.

On detection:
- initialize NarrativeRuntime from Act1 claim/narrative;
- require 60-tick ENOUGH confirmation;
- set ENOUGH ending bit only at authoritative ending fixation;
- unlock vesselContinueAvailable;
- do not unlock Audit Trace;
- run 360-tick ending;
- Credits at 720.

If T07 prematurely set the ENOUGH bit as a placeholder, normalize the copied T10 runtime so the authoritative T10 ending fixation is the only persistent save boundary. Avoid broad Act1 rewrite.

Continue From Vessel:
use the saved claim/narrative state at the Vessel minimum checkpoint and the smallest cross-ticket helper needed to reopen Vessel continuation settlement. Do not replay PATCH/PAIR.

## 9.6 Final choices/endings

RATIFY:
Ending Rule.

OVERRIDE:
Ending Hand.

OPEN:
Ending OpenLedger only when unlocked.

TOTAL CLAIM:
Ending One only when exact secret predicate true.

All ending minimum ticks: 360.
Auto Credits: 720.

At ending ID fixation:
- set corresponding endingSeen bit;
- RULE/HAND/OPEN/ONE set fullEndingReached and auditTraceUnlocked;
- ENOUGH sets vesselContinueAvailable only.

## 9.7 AUDIT TRACE

Available at Title only after Rule/Hand/Open/One.

Exactly:
CASE | TRANSFER | RULE USED.

No raw replay log in save.
Use choiceBits + runSeed/fixed geometric 180-tick summaries.

This can be deterministic presentation data derived on demand; do not build a replay recording subsystem.

## 9.8 SaveV2

Existing 96B serializer stays unchanged in layout.

Flags:
bit0 valid
bit1 vesselContinue
bit2 fullEndingReached
bit3 auditTraceUnlocked
bit4 previewPending MUST be 0 on disk.

Save moments:
- TITLE_BORROW complete
- immediately before encounter
- immediately before settlement selection
- after 36-tick commit completes
- immediately after ending fixed
- immediately after option change

Do not save preview/mid-combat/hit-freeze.

Quota-saving architecture:
do NOT inject file-writing code into every Act implementation.

Instead, top-level platform router observes authoritative state transitions and invokes one save coordinator. This avoids modifying Act1/2/3 solely for I/O.

Add pure helper:
```cpp
SaveData MakeSaveData(
    const ClaimLedger&,
    const NarrativeState&,
    uint32_t flags,
    uint32_t options,
    uint32_t runSeed,
    uint32_t bestFullRunTicks,
    uint8_t tutorial) noexcept;
```

Win32 safe file adapter uses only Kernel32-style APIs/environment retrieval already allowed:
- resolve `%LOCALAPPDATA%\BorrowedPixels`
- `save.tmp`
- write exactly96
- FlushFileBuffers
- replace `save.bin`
- preserve old save on failure
- invalid/version mismatch rename once to `save.invalid`
- no new external DLL dependency.

## 9.9 Save-boundary observer

PlatformState keeps tiny previous-flow snapshot:
```cpp
uint8_t lastRuntimeStage;
uint8_t lastEncounterIndex;
uint8_t lastScene;
uint32_t lastOptions;
```

After each successful simulation tick:
- compare old/new state;
- if one of the frozen save boundaries occurred, write once.

This avoids scattering save calls through all existing ticket code.

Tests must prove:
- each boundary exactly once;
- preview never writes;
- hit-freeze never writes;
- failed write leaves old save;
- previewPending disk bit zero.

## 9.10 T10 tests

Highest priority:
- 5 ending predicates;
- OPEN locked row still visible;
- ONE exact nine-choice predicate;
- ONE ends Claim PLAYER=1440, total1440;
- ENOUGH continue semantics;
- Audit Trace unlock semantics;
- exact ending bits;
- SaveV2 remains96B/golden;
- save boundary exactly once;
- preview/hit-freeze never save;
- old save survives write/replace failure;
- title return preserves claim ledger;
- deterministic TITLE→each ending route;
- no claim/physical invariant loss.

---

# 10. T11/T12/T13/T14 BUDGET POLICY

T11 and T12:
implement authority minimum, not optional polish.

T11:
- exact six Options rows;
- synthesized audio only;
- game hash independent from audio;
- silent fallback;
- no external assets.

T12:
- frozen KO/EN copy only;
- existing font/composition;
- glyph/rect validation;
- credits/provenance;
- no new font/assets.

T13:
if no Human-Director-approved balance amendment exists:
- do not tune;
- run deterministic/reachability/softlock/balance-bound checks;
- mark subjective Gate5 HUMAN_REVIEW_DEFERRED;
- continue.

T14:
feature freeze.
One clean final complete verification:
- authority
- Debug
- Test
- CTest
- Contest
- live EXE
- exact dist
- SHA256
- dependencies
- whitelist
- rights
- README
- final handoff.

---

# 11. QUOTA INTERRUPTION RECOVERY

If Codex usage stops unexpectedly, user should not reconstruct context.

After quota reset, send only:
`/goal resume`

On resume Codex must:
1. inspect actual repo;
2. preserve dirty work;
3. identify active branch/HEAD and first incomplete operation;
4. read only current diff/evidence plus exact needed authority sections;
5. continue the interrupted compile/fix/review/integration action.

Never start over because the model context is fresh.

If dirty work exists:
- do not reset/clean/checkout;
- inspect and continue it.

Repository state is the checkpoint.

---

# 12. FINAL DEFINITION

RC_CANDIDATE only when:
- required flow reachable;
- both ledgers always1440;
- deterministic checks pass;
- final MSVC Debug/Test/Contest pass;
- final reviewed executable P0/P1=0;
- dist <=1,350,000;
- hashes/dependencies/rights/README/evidence ready.

Human-only:
- deferred subjective play;
- three-PC if not actually run;
- upload/submission.

Never push, publish or submit automatically.
