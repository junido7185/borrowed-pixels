# BORROWED PIXELS

## Codex 다중 세션 구현 운영서 v2.1

- 대상: Windows 10/11 x64 C++20/Win32 독립 실행형 게임
- 하드 상한: 압축 해제 제출 폴더 1,474,560B
- 내부 RC 상한: 1,350,000B
- 구현 권위: 같은 패키지의 `BORROWED_PIXELS_AUTHORITY_v2.1.md`
- 기본 원칙: 한 세션은 한 티켓·한 파일 집합만 소유하고 `READY_FOR_REVIEW`까지만 선언한다.

이 운영서는 게임을 새로 설계하는 문서가 아니다. 숫자·좌표·문구·규칙의 결정권은 production spec과 runtime contract에 있다.

---

# 1. 구현 세션에 제공할 파일

모든 새 세션은 다음 다섯 파일을 받는다.

1. `BORROWED_PIXELS_AUTHORITY_v2.1.md`
2. `BORROWED_PIXELS_FINAL_PRODUCTION_SPEC_v2.1.md`
3. `BORROWED_PIXELS_RUNTIME_CONTRACT_v2.1.json`
4. `BORROWED_PIXELS_FONT_GLYPHS_v2.1.json`
5. 현재 티켓 handoff와 STATUS

UI 작업에는 `BORROWED_PIXELS_UI_REFERENCE_ATLAS_v2.1.svg`를 추가한다. 구 v1.0·v2.0 문서는 전달하지 않는다.

세션은 시작 전에 AUTHORITY의 SHA-256과 자기 파일의 실제 SHA-256을 비교한다. 불일치는 작업 중단이다.

# 2. 역할

| 역할 | 쓰기 범위 | 책임 | 금지 |
|---|---|---|---|
| HUMAN DIRECTOR | 최종 merge·명세 amendment | 화면·손맛·권리·출시 승인 | 검증 없이 PASS 선언 |
| INTEGRATOR | integration branch, STATUS | 직렬 통합·전체 gate | 기능 구현·자의적 수정 |
| IMPLEMENTER Txx | 티켓 allowlist | bounded 구현·테스트·handoff | 다른 티켓 파일·명세 수정 |
| REVIEWER | 읽기 전용 | 재현·P0/P1·byte 회귀 | 직접 수정 |
| COMPLIANCE | evidence·dist | SHA·권리·크기·환경 기록 | 코드 변경 |
| UI REVIEW | docs/ui review만 | 기준 atlas 비교·가독성 제안 | 런타임 규칙·좌표 변경 |

동시 코드 writer는 최대 2개다. 공용 헤더를 쓰는 티켓 둘은 병렬 실행하지 않는다.

# 3. 저장소 구조

```text
BORROWED_PIXELS/
├─ CMakeLists.txt
├─ docs/
│  ├─ authority/
│  ├─ evidence/TXX/
│  ├─ handoffs/TXX/
│  ├─ ui/approved/
│  ├─ STATUS.md
│  ├─ OWNERSHIP.md
│  ├─ PROVENANCE.md
│  └─ RIGHTS_LOG.md
├─ src/
│  ├─ bp_types.h
│  ├─ bp_config.h
│  ├─ bp_platform.h
│  ├─ bp_win32.cpp
│  ├─ bp_math.h
│  ├─ bp_physical_ledger.h
│  ├─ bp_physical_ledger.cpp
│  ├─ bp_claim_ledger.h
│  ├─ bp_claim_ledger.cpp
│  ├─ bp_game.h
│  ├─ bp_game.cpp
│  ├─ bp_input.cpp
│  ├─ bp_combat.cpp
│  ├─ bp_reclaim.cpp
│  ├─ bp_hud_loan.cpp
│  ├─ bp_encounter.h
│  ├─ bp_encounter.cpp
│  ├─ bp_act1.cpp
│  ├─ bp_act2.cpp
│  ├─ bp_act3.cpp
│  ├─ bp_precedent.cpp
│  ├─ bp_narrative.cpp
│  ├─ bp_render.cpp
│  ├─ bp_font.cpp
│  ├─ bp_audio.cpp
│  ├─ bp_save.cpp
│  └─ bp_content.h
├─ tests/
├─ tools/
└─ dist/contest/
```

`dist/contest`에는 문서·테스트·JSON·SVG·디버그 심볼을 넣지 않는다. runtime JSON은 빌드 시 C++ 상수로 생성하며 게임이 파일을 파싱하지 않는다.

# 4. 브랜치·통합 규칙

- 기준 branch: `integration/rc-2026-09-03`.
- 티켓 branch: `ticket/TXX-short-name`.
- 검토 후보: `candidate/TXX-final`.
- 사람 승인 뒤에만 `integration`으로 fast-forward.
- 수정이 생기면 이전 review와 SHA 승인은 무효.
- 구현자는 merge·tag·main 갱신을 하지 않는다.
- dirty working tree, 권위 SHA 불일치, allowlist 밖 변경은 즉시 중단한다.

# 5. 티켓과 파일 소유권

| 티켓 | 범위 | 주 소유 파일 | 선행 | 핵심 인수 조건 |
|---|---|---|---|---|
| T00 | 저장소·CMake·size gate·권리대장 | 루트, tools, docs | 없음 | clean build skeleton, dist exact bytes |
| T01 | Win32 창·타이밍·입력·waveOut shell | bp_win32, platform | T00 | 320×180, integer scale, focus pause |
| T02 | 타입·두 원장·transaction·SaveV2 | types, ledgers, save, tests | T00 | 19,683 조합, 96B golden |
| T03 | primitive renderer·font·UI shell | render, font, UI tests | T01,T02 API | atlas 구조·glyph·팔레트 exact |
| T04 | fixed tick·이동·충돌·대시 | game,input,math | T01,T02 | replay hash, boundary tests |
| T05 | Charge·Projectile·피격·Core | combat | T04 | 질량 보존·swept collision |
| T06 | Neutral·회수·HUD loan | reclaim,hud_loan | T05,T03 | 7상태×3모듈, 포화 테스트 |
| T07 | 만남 framework·서막·PATCH·PAIR·VESSEL | encounter,act1 | T06 | Gate 4 수직 슬라이스 |
| T08A | KEEPER·ADAPTER·FRAME | act2 | Gate4,T07 interface | 전장 exact, 결과 전 구간 |
| T08B | INDEX·WITNESS·CURRENT | act3 | Gate4,T07 interface | 전장 exact, 결과 우선순위 |
| T09 | PRECEDENT 81 profile | precedent | T08A,T08B | 81 profile softlock0 |
| T10 | 진행·결산·5엔딩·AUDIT TRACE | narrative,save | T09 | 조건·연출·continue 정확 |
| T11 | 합성 오디오·옵션·접근성 | audio, UI option layer | T10 | voice overflow·hash 독립 |
| T12 | KO/EN copy·전체 glyph·credits | content,font,narrative copy | T11 | 잘림·누락0, provenance |
| T13 | 밸런스·플레이테스트 amendment | 명세 승인 범위만 | 전체 | Gate5 정량 기준 |
| T14 | RC·3PC·출품 | dist, evidence | 전체 | ≤1,350,000B 내부 gate |

T08A와 T08B만 동시 구현할 수 있다. 두 세션은 공용 header를 수정하지 않는다. 변경이 필요하면 signature 제안만 handoff에 남기고 중단한다.

# 6. 모든 구현 세션 시작 프롬프트

다음 블록에서 대괄호만 티켓 값으로 바꿔 새 세션 첫 메시지에 붙인다.

```text
You are BP-[TICKET], one bounded implementation session for BORROWED PIXELS v2.1.

Read in full before editing:
1. BORROWED_PIXELS_AUTHORITY_v2.1.md
2. BORROWED_PIXELS_FINAL_PRODUCTION_SPEC_v2.1.md
3. BORROWED_PIXELS_RUNTIME_CONTRACT_v2.1.json
4. BORROWED_PIXELS_FONT_GLYPHS_v2.1.json
5. docs/handoffs/[TICKET]/INPUT.md

Verify every SHA against AUTHORITY. Older v1.0/v2.0 files are non-authoritative.

Hard constraints:
- Windows x64 standalone C++20/Win32.
- Extracted contest folder <= 1,474,560 bytes; internal RC <= 1,350,000.
- PhysicalLedgerTotal()==1440 and ClaimLedgerTotal()==1440.
- No engine, third-party runtime, external runtime asset, network, browser, or runtime AI.
- Fixed arrays and deterministic integer/fixed-point simulation.
- Modify only this ticket allowlist: [FILES].
- Do not invent or tune coordinates, copy, rules, timing, weights, save fields, or presentation.

Before editing:
1. Report authority hashes, branch HEAD, working-tree state and allowlist.
2. Run the current build, tests and exact dist measurement.
3. Map every ticket acceptance criterion to a test or manual capture.
4. If a required value is missing or contradictory, stop with SPEC_BLOCKER.

After editing:
1. Build debug, test and contest targets.
2. Run ticket tests plus both ledger validators and replay hash.
3. Report changed paths, commands, exact results, dist bytes and byte delta.
4. Write docs/handoffs/[TICKET]/OUTPUT.md with remaining risks.
5. End at READY_FOR_REVIEW. Never claim ACCEPTED or merge.
```

# 7. 구현 세션의 출력 형식

```text
TICKET:
BASE SHA:
CANDIDATE SHA:
AUTHORITY HASHES:
CHANGED PATHS:
ACCEPTANCE MAP:
COMMANDS RUN:
TEST RESULTS:
DIST BYTES / DELTA:
DETERMINISM RESULT:
RIGHTS / ASSET RESULT:
KNOWN RISKS:
STATUS: READY_FOR_REVIEW | BLOCKED
```

명령과 로그 경로 없이 `테스트 통과`라고 쓰면 실패다. 실행하지 않은 Windows 검증을 추정해 PASS로 쓰지 않는다.

# 8. 읽기 전용 리뷰 프롬프트

```text
Review candidate [SHA] for BORROWED PIXELS ticket [TICKET]. Do not edit.

Verify:
- candidate SHA and authority hashes;
- changed paths are inside the ticket allowlist;
- every acceptance criterion has reproducible evidence;
- no physical/claim mass creation or loss;
- preview never commits and save never serializes preview;
- no float, system time, pointer order or unstable container affects simulation;
- no hidden asset/runtime/network dependency;
- coordinates, copy, timing and weights match the v2.1 contract;
- exact contest byte delta;
- failures, overflows, corrupted save and slot saturation.

Return findings as P0/P1/P2 with file, symbol, reproduction and smallest compliant fix.
P0/P1 must be zero for READY_FOR_HUMAN_GATE.
Do not make the fix yourself.
```

# 9. SPEC_BLOCKER 형식

다음 중 하나면 구현하지 않고 보고한다.

- 두 권위 파일의 값이 다름.
- 필요한 좌표·틱·문구·enum이 없음.
- 인수 조건이 현재 API로 테스트 불가능.
- allowlist 밖 수정 없이는 진행 불가.
- 공모전 크기·독립 실행·권리 조건을 깨야 진행 가능.

```text
SPEC_BLOCKER
Ticket:
Conflicting or missing clauses:
Why implementation requires a design decision:
Smallest proposed amendment:
Affected tests/files/bytes:
No code changed: YES/NO
```

# 10. 웹 GPT UI 사용법

v2.1에서 웹 GPT는 새 디자인을 정하는 주체가 아니라 **계약 검토자와 목업 변환기**다.

제공할 것:

- production spec 24~26장
- runtime contract의 palette, regions, screens, motion
- 승인 atlas
- 현재 320×180 capture

요청할 것:

1. rect·z-order·문자폭 위반 탐지.
2. KO/EN 잘림·정보 위계·색 비의존성 검토.
3. 동일 좌표를 사용한 4배 검토용 mockup.
4. 개선안은 `PROPOSAL`로만 제출하고 좌표를 자동 변경하지 않기.

금지할 것:

- 특정 게임·회사·작가 스타일 모사.
- PNG/SVG/폰트/아이콘을 런타임 에셋으로 넣기.
- 자동 layout, CSS percentage, 웹폰트, rounded card, glow.
- 새 버튼·상태·대사·게임 규칙 만들기.

웹 출력은 사람이 승인한 amendment가 되기 전까지 구현 근거가 아니다.

# 11. Gate별 사람 승인 질문

## Gate 3 UI

- 1초 안에 PLAYER·상대·Neutral을 구분하는가?
- VITAL/VECTOR/IMPULSE 중 무엇을 잃었는지 색 없이 아는가?
- TITLE의 64 TILE이 몸으로 바뀐다는 인과가 보이는가?
- 한국어가 작은 글씨가 아니라 의도된 조형으로 보이는가?
- 화면이 터미널·OS 오류창·기존 인디게임 모사로 보이지 않는가?

## Gate 4 수직 슬라이스

- 발사 비용이 체력 감소가 아니라 몸의 이동으로 느껴지는가?
- 빗나간 탄을 상대가 회수하는 순간이 억울하지 않고 이해되는가?
- HUD 해체가 단순 버프 선택이 아니라 기능 담보로 느껴지는가?
- VESSEL 뒤 계속하는 선택이 생존 이후의 욕망/책임으로 읽히는가?

## Gate 5 전체

- 각 만남의 질문이 대사 없이 플레이로 다시 설명되는가?
- PRECEDENT 패턴이 이전 선택과 연결되어 보이는가?
- 엔딩마다 이익과 비용이 함께 남는가?
- 어느 선택도 명백한 개발자 정답으로 연출되지 않는가?

# 12. 용량 회귀 규칙

- 모든 티켓은 contest dist 전체 합과 이전 accepted 대비 delta를 보고한다.
- 8KB 이상 증가 시 symbol/map 기반 사유를 기록한다.
- 25KB 이상 증가하면 사람의 byte budget 재승인 전 merge 금지.
- 문자열·테이블은 u16 offset pool, 반복 enum table을 사용한다.
- 디버그 문구·JSON parser·RTTI·exceptions·iostream·filesystem을 contest build에 넣지 않는다.
- 크기 감소를 위해 안전 검사·원장 assert의 release 오류 경로를 삭제하지 않는다. 메시지는 error code로 압축한다.

# 13. 최종 인수

다음이 모두 참일 때만 `IMPLEMENTATION COMPLETE`다.

- T00~T14가 accepted SHA로 integration에 직렬 반영.
- P0/P1 0.
- Gate1~6 증거 존재.
- 3PC Windows 실행.
- dist≤1,350,000B.
- 권리 대장에 외부 asset0, AI-assisted 항목, 인간 승인 기록.
- 10개 만남·81 PRECEDENT profile·5엔딩 접근 가능.
- 첫 회차 철학 이해 목표와 플레이 시간 목표 통과.

이전에는 `프로토타입`, `수직 슬라이스`, `RC 후보`처럼 실제 상태만 사용한다.
