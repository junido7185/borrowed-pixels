# BORROWED PIXELS v2.1 소유권 경계

한 번에 한 명의 writer만 작업하며 각 티켓은 아래 경계를 넘지 않는다. 정확한 파일 허용 범위와 인수 조건은 v2.1 agent runbook이 우선한다.

| 티켓 | 소유 영역 |
|---|---|
| T00 | 루트 빌드 파일, `tools/`, `docs/` 운영·증거 파일 |
| T01 | `src/bp_win32.cpp`, `src/bp_platform.h` |
| T02 | core types, physical/claim ledger, save, 관련 tests |
| T03 | renderer, font, UI tests |
| T04 | game, input, math, 관련 tests |
| T05 | combat, 관련 tests |
| T06 | reclaim, HUD loan, 관련 tests |
| T07 | encounter framework, act1, 승인된 content 범위 |
| T08A | `src/bp_act2.cpp`, 해당 test |
| T08B | `src/bp_act3.cpp`, 해당 test |
| T09 | precedent, 해당 test |
| T10 | narrative, 승인된 save 범위 |
| T11 | audio, option UI layer |
| T12 | content/font/narrative의 승인된 KO/EN copy |
| T13 | 사람 승인 amendment가 지시한 값만 |
| T14 | dist, release tooling, evidence |

공용 API 변경이 현재 티켓 밖 파일을 요구하면 임의 수정하지 않고 API gate 또는 `SPEC_BLOCKER`로 돌린다. Authority 파일은 모든 티켓에서 읽기 전용이다.
