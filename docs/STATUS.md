# BORROWED PIXELS v2.1 + approved T03 amendment status

- Latest integrated candidate SHA: `c04cab5551660446742a9a114f52d413fa6a3eb7`
- Current controller state: `T08A_READY_FOR_INTEGRATION`
- Authority state: v2.1 manifest verification `PASS`; T03 amendment SHA verification `PASS`
- Current ticket: T08A Act 2 process encounters
- Contest dist: `206,336B` (T08A delta `+22,528B`, internal cap PASS)

| Ticket | State | Evidence |
|---|---|---|
| T00 | LOCALLY_INTEGRATED | `docs/evidence/T00/`, `docs/handoffs/T00/OUTPUT.md` |
| T01 | LOCALLY_INTEGRATED | `docs/evidence/T01/`, `docs/handoffs/T01/OUTPUT.md` |
| T02 | LOCALLY_INTEGRATED | Independent review P0/P1/P2=0; `docs/evidence/T02/` |
| T03 | LOCALLY_INTEGRATED | Candidate `1e49151c...`; post-integration full regression PASS; independent review P0/P1/P2=0 |
| T04 | LOCALLY_INTEGRATED | Candidate `b8739064...`; post-integration full regression PASS; independent review P0/P1/P2=0 |
| T05 | LOCALLY_INTEGRATED | Candidate `f8f1c042...`; post-integration full regression PASS; independent review P0/P1=0, P2=1 test-only fault-injection debt |
| T06 | LOCALLY_INTEGRATED | Candidate `70a2ae0b...`; post-integration full regression PASS; final independent review P0/P1/P2=0 |
| T07 | LOCALLY_INTEGRATED | Candidate `c04cab55...`; Human Director byte-budget reapproval; post-integration full regression PASS |
| T08A | READY_FOR_INTEGRATION | 72,445 checks; final independent review P0/P1/P2=0 |
| T08B-T14 | PENDING | Sequential controller execution |

## Gate status

| Gate | State | Note |
|---|---|---|
| Gate 0 | PASS | Six authority hashes match; no v1.0/v2.0 source used |
| Gate 1 | PASS | T01 shell builds and runs; `126,464B <= 450,000B` |
| Gate 2 | PASS | 19,683 combinations, atomic rollback, and 96B SaveV2 golden pass |
| Gate 3/4 | HUMAN_REVIEW_DEFERRED | T07 playable package prepared; subjective PASS not claimed |
| Gate 5/6 | PENDING | Later tickets and human verification |

T03 was locally fast-forward integrated after explicit Human Director approval. Amendment commit `60d9e11204208f3fc43d348029167ac21caef61b` contains authority only. Subjective Gate 3/4 findings remain `HUMAN_REVIEW_DEFERRED` under the standing continuation decision and are not claimed PASS.
