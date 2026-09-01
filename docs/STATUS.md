# BORROWED PIXELS v2.1 상태

- 통합 기준 SHA: `152b0177896b83bf0d16457fb7f20e98d0b9b374`
- 현재 상태: `REVIEWING_T00`
- 권위 상태: v2.1 manifest SHA 검증 PASS
- 현재 티켓: T00 bootstrap candidate
- contest dist: `107,008B`

| 티켓 | 상태 | 근거 |
|---|---|---|
| T00 | READY_FOR_REVIEW | `docs/evidence/T00/`, `docs/handoffs/T00/OUTPUT.md` |
| T01~T14 | PENDING | T00 로컬 통합 뒤 순차 진행 |

## Gate 상태

| Gate | 상태 | 비고 |
|---|---|---|
| Gate 0 | PASS | 권위 해시 6개 일치, 구 v1.0/v2.0 문서 미사용 |
| Gate 1 | PENDING | T01 Win32 shell에서 수행 |
| Gate 2 | PENDING | T02 원장·저장에서 수행 |
| Gate 3/4 | PENDING | T07 통합 뒤 HUMAN_CHECKPOINT |
| Gate 5/6 | PENDING | 후속 티켓과 사람 검증 필요 |

`READY_FOR_REVIEW`는 수락이나 통합을 뜻하지 않는다. 독립 검토에서 P0/P1=0이고 자동 검증이 재현된 뒤에만 컨트롤러가 로컬 fast-forward 통합한다.
