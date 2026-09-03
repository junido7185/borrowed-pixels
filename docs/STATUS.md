# BORROWED PIXELS v2.1 + approved T03 amendment status

- Latest integrated candidate SHA: `6febd613a5c5c9c20e6acb9b9ba86a81e8d9f1a1`
- Current controller state: `T08B_REVIEWING`
- Authority state: v2.1 manifest verification `PASS`; T03 amendment SHA verification `PASS`
- Current ticket: T08B Act 3 rule encounters
- Contest dist: `227,840B` (T08B delta `+21,504B`, internal cap PASS)

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
| T08A | LOCALLY_INTEGRATED | Candidate `6febd613...`; post-integration full regression PASS; final review P0/P1/P2=0 |
| T08B | REVIEWING | Candidate prepared; final independent review pending |
| T09-T14 | PENDING | Sequential controller execution |

## Gate status

| Gate | State | Note |
|---|---|---|
| Gate 0 | PASS | Six authority hashes match; no v1.0/v2.0 source used |
| Gate 1 | PASS | T01 shell builds and runs; `126,464B <= 450,000B` |
| Gate 2 | PASS | 19,683 combinations, atomic rollback, and 96B SaveV2 golden pass |
| Gate 3/4 | HUMAN_REVIEW_DEFERRED | T07 playable package prepared; subjective PASS not claimed |
| Gate 5/6 | PENDING | Later tickets and human verification |

T03 was locally fast-forward integrated after explicit Human Director approval. Amendment commit `60d9e11204208f3fc43d348029167ac21caef61b` contains authority only. Subjective Gate 3/4 findings remain `HUMAN_REVIEW_DEFERRED` under the standing continuation decision and are not claimed PASS.
