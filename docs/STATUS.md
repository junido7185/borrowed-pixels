# BORROWED PIXELS v2.1 + approved T03 amendment status

- Latest integrated ticket SHA: `1e49151c1f0d70d20f77f7bb8aa77f2972de0f0f`
- Current controller state: `NEXT_TICKET`
- Authority state: v2.1 manifest verification `PASS`; T03 amendment SHA verification `PASS`
- Current ticket: T04 fixed tick/movement/collision/dash
- Contest dist: `129,024B`

| Ticket | State | Evidence |
|---|---|---|
| T00 | LOCALLY_INTEGRATED | `docs/evidence/T00/`, `docs/handoffs/T00/OUTPUT.md` |
| T01 | LOCALLY_INTEGRATED | `docs/evidence/T01/`, `docs/handoffs/T01/OUTPUT.md` |
| T02 | LOCALLY_INTEGRATED | Independent review P0/P1/P2=0; `docs/evidence/T02/` |
| T03 | LOCALLY_INTEGRATED | Candidate `1e49151c...`; post-integration full regression PASS; independent review P0/P1/P2=0 |
| T04 | NEXT_TICKET | Begin from the clean post-T03 integration base |
| T05-T14 | PENDING | Sequential controller execution |

## Gate status

| Gate | State | Note |
|---|---|---|
| Gate 0 | PASS | Six authority hashes match; no v1.0/v2.0 source used |
| Gate 1 | PASS | T01 shell builds and runs; `126,464B <= 450,000B` |
| Gate 2 | PASS | 19,683 combinations, atomic rollback, and 96B SaveV2 golden pass |
| Gate 3/4 | PENDING | T07 integration then human checkpoint |
| Gate 5/6 | PENDING | Later tickets and human verification |

T03 was locally fast-forward integrated after explicit Human Director approval. Amendment commit `60d9e11204208f3fc43d348029167ac21caef61b` contains authority only. Subjective Gate 3/4 findings remain `HUMAN_REVIEW_DEFERRED` under the standing continuation decision and are not claimed PASS.
