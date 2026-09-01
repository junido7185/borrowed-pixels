# BORROWED PIXELS v2.1 status

- Integration base SHA: `2cb1502269000184311e6d1acb909864aa813e30`
- Current controller state: `REVIEWING_T01`
- Authority state: v2.1 manifest verification `PASS`
- Current ticket: T01 platform candidate
- Contest dist: `126,464B`

| Ticket | State | Evidence |
|---|---|---|
| T00 | LOCALLY_INTEGRATED | `docs/evidence/T00/`, `docs/handoffs/T00/OUTPUT.md` |
| T01 | READY_FOR_REVIEW | `docs/evidence/T01/`, `docs/handoffs/T01/OUTPUT.md` |
| T02-T14 | PENDING | Sequential controller execution after T01 review |

## Gate status

| Gate | State | Note |
|---|---|---|
| Gate 0 | PASS | Six authority hashes match; no v1.0/v2.0 source used |
| Gate 1 | CANDIDATE_PASS | T01 shell builds and runs; `126,464B <= 450,000B` |
| Gate 2 | PENDING | T02 ledgers/save |
| Gate 3/4 | PENDING | T07 integration then human checkpoint |
| Gate 5/6 | PENDING | Later tickets and human verification |

`READY_FOR_REVIEW` is not acceptance or merge approval. The controller may locally fast-forward only after the candidate has P0/P1=0 and all automated checks reproduce.
