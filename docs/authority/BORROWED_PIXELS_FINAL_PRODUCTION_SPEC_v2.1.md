# BORROWED PIXELS

## 1.44MB 공모전·Steam용 최종 제작 명세 v2.1

- 작성 기준일: 2026-08-26
- 내부 코드명: `BP1440`
- 공개 제목: `BORROWED PIXELS` — 상표 최종 확인 전 작업명
- 대상: Windows 10/11 x64 독립 실행형 게임
- 공모전 하드 제한: 압축 해제 후 전체 `1,474,560`바이트 이하
- 내부 RC 제한: `1,350,000`바이트 이하
- 엔진: 없음. C++20 + Win32 + GDI + waveOut
- 장르: 철학적 제로섬 액션 / 고정 화면 2D / 10개의 유한 자원 만남
- 한 회차 목표: 38~52분
- 조기 `ENOUGH` 엔딩: 12~18분
- 문서 상태: **DESIGN FREEZE / T00~T02 즉시 착수 / 수직 슬라이스·최종 Windows RC 인증 PENDING**

이 문서는 v2.0의 서사·원장 구조를 보존하면서 화면 좌표, 장면별 서사, 철학 전달 장치, 엔딩 연출, 만남 배치, 모션, 오디오, 접근성, 세션 운영을 구현 가능한 값으로 동결한 최종 제작 명세다. `BORROWED_PIXELS_IMPLEMENTATION_SPEC_v1.0.md`, `BORROWED_PIXELS_AI_MULTI_SESSION_RUNBOOK_v1.0.md`, `BORROWED_PIXELS_MASTER_IMPLEMENTATION_BIBLE_v2.0.md`는 이 문서의 역사 자료이며 구현 입력으로 사용하지 않는다.

권위 순서는 다음과 같다.

1. `BORROWED_PIXELS_RUNTIME_CONTRACT_v2.1.json`의 ID·정수·좌표·상태 enum
2. `BORROWED_PIXELS_FONT_GLYPHS_v2.1.json`의 글리프 bitmap·조합 enum
3. 이 문서의 의미·규칙·문구·인수 조건
4. `BORROWED_PIXELS_AGENT_RUNBOOK_v2.1.md`의 작업 절차
5. 승인된 기준 화면 `BORROWED_PIXELS_UI_REFERENCE_ATLAS_v2.1.svg`
6. 구현자의 관습·제안·추측

1~3번이 충돌하면 구현을 멈추고 사람에게 보고한다. AI는 평균값, 관습값, 임의 보정으로 충돌을 숨기지 않는다. 이 패키지 밖의 구문서는 읽더라도 구현 근거로 인용하지 않는다.

> 게임 약속: 정확히 1,440개의 TILE만 존재하는 세계에서, 플레이어는 빌린 몸과 기능을 담보로 싸우며 타인의 몫을 재배분한다. 마지막에는 자신의 개별 선택이 이름과 사정을 잃은 보편 규칙으로 되돌아온다.

---

# 0. 확정 판정

## 0.1 지금 확정하는 제품 범위

| 항목 | 확정값 |
|---|---|
| 의미 있는 만남 | 소규모 점유자 6 + 대형 점유자 3 + 최종전 1 = 10 |
| 비인격적 장애물 | SWEEPER, COMPACTOR, CHECKSUM 3종 |
| 핵심 보존 법칙 | 물리 원장 1,440 + 청구 원장 1,440을 각각 항상 보존 |
| 선택 수 | 3지선다 9회. 이론상 19,683개 조합 |
| 철학 프로필 | 자아 3종 × 분배 3종 × 통치 3종 + 기억 3종 |
| 결말 | ENOUGH, RULE, HAND, OPEN LEDGER + 비밀 ONE |
| 런타임 AI | 없음. 고정 데이터와 결정론적 Utility AI만 사용 |
| 외부 에셋 | 없음. 도형·자체 글꼴·실시간 합성음만 사용 |
| 온라인 기능 | 없음 |
| 공모전 후 점수 모드 | 현재 범위에서 제거. 서사 회차와 결말 재플레이에 집중 |

## 0.2 용량 가능성 결론

**구조적으로 충분히 가능하다.** 새 서사 데이터가 용량의 병목은 아니다.

- 한·영 플레이어 표시 문구 전체 원문: `4,986 B`
- 초기 청구량, 10개 만남, 27개 선택, 12개 정책 모듈 표: `2,766 B`
- 위 두 파일을 압축하지 않고 PE 섹션 정렬까지 적용한 실험 파일: `10,752 B`
- 공모전 하드 상한 대비 약 `0.73%`
- 실제 컴파일에서는 문자열 중복 제거·오프셋 테이블을 사용하므로 동일하거나 더 작을 가능성이 높다.

단, 이 수치는 **서사·콘텐츠 데이터의 적합성 검증**이다. 최종 Windows 실행 파일의 완전한 인증은 MSVC contest 빌드, `dumpbin /dependents`, Windows 10/11 실행, 제출 폴더 합산 측정 뒤에만 PASS로 바뀐다.

## 0.3 자동 검증 결과

2026-08-26 기준 확정한 9개 선택을 전수 조합한 결과:

| 검증 | 결과 |
|---|---:|
| 전체 조합 | 19,683 |
| 청구 원장 합계 오류 | 0 |
| 음수 계정 | 0 |
| VESSEL 결산 후 생존 최소량 미달 | 0 |
| 최종 플레이어 청구량 범위 | 144..512 |
| COMMON 청구량 범위 | 0..272 |
| 가장 긴 영문 표시문 | 38자 |
| 가장 긴 한글 표시문 | 22자 |
| 검증 판정 | PASS |

---

# 1. 공식 공모전 규격과 출품 게이트

공식 페이지: https://2pgarcade.com/contest-144mb.html

| 규격 | 확인값 | 대응 |
|---|---|---|
| 용량 | 압축 해제 후 전체 ≤ 1,474,560B | 세 파일 합을 실제 바이트로 측정 |
| 실행 형태 | 독립 실행형, 브라우저 금지 | Windows x64 EXE |
| 엔진 | 제한 없음, 런타임도 용량 포함 | 엔진·외부 런타임 없음 |
| 제작 시점 | 2026-05-23 공식 발표 후 새 제작 | 새 저장소·커밋·AI 로그 보관 |
| 참가 | 개인 또는 팀 | 기본 Solo developer |
| 심사 | 완성, 용량, 재미 | 40분 내 완결·크기 자동 게이트·플레이테스트 |
| 마감일 | 2026-09-04 | 운영 마감은 9월 3일 제출 완료 |

공식 페이지 규칙 본문은 `23:59`, 일정 영역은 `23:39`로 서로 다르다. 시간대도 명시되지 않았으므로 9월 4일 당일 개발·최초 제출을 금지한다.

제공된 Google Form 화면으로 확인된 항목:

- Developer / Team Name
- Contact Email
- Country / Region
- Game Title
- Game Description (Brief)
- Supported Platform: Windows / macOS / Linux / Other
- 완성 빌드 업로드: 최대 5개, 파일당 10MB
- 대한민국 참가자 개인정보 수집·이용 동의
- 제출 저작권은 개발자에게 유지
- 주최자의 이미지·영상·설명 홍보·전시 사용 동의
- 수상작의 향후 2P GAME ARCADE 행사 전시 가능성
- 모든 포함 소재가 직접 제작되었거나 적법한 권리를 보유했다는 확인

업로드 확장자 허용 범위는 화면만으로 확정되지 않았다. RC 때 파일 선택창에서 확인한다.

제출 폴더는 다음 세 파일만 허용한다.

```text
BorrowedPixels.exe
README_KO_EN.txt
LICENSES.txt
```

ZIP 허용 시 위 세 파일만 압축한다. ZIP 크기가 아니라 새 빈 폴더에 압축을 푼 세 파일의 합으로 판정한다.

---

# 2. 철학적 중심과 표현 원칙

## 2.1 단 하나의 중심 질문

> 무엇이 한 존재를 ‘나’로 만들며, 그 ‘나’에게 세계의 몫을 요구할 권리를 주는가?

질문의 확대 순서는 고정한다.

| 막 | 질문 |
|---|---|
| 1막: 자아 | 물질, 기억, 관계 중 무엇이 나를 나로 만드는가? |
| 2막: 분배 | 같은 TILE을 주는 것이 정말 공정한가? |
| 3막: 규칙 | 누가 공정함의 기준을 정하고 무엇을 측정하는가? |
| 최종전 | 내 개별 판단을 모두에게 적용해도 여전히 동의할 수 있는가? |

설계 참고축은 개인 동일성, 불교의 비실체적 자아, 역량 접근법, 공동자원 관리, AI의 공정성·책임·인간 감독이다. 철학자의 이름과 학술 용어는 게임 안에 직접 노출하지 않는다.

- 개인 동일성: https://plato.stanford.edu/entries/identity-personal/
- 인도 불교철학의 자아: https://plato.stanford.edu/entries/mind-indian-buddhism/
- 역량 접근법: https://plato.stanford.edu/entries/capability-approach/
- 오스트롬 공동자원 연구: https://www.nobelprize.org/prizes/economic-sciences/2009/ostrom/lecture/
- UNESCO AI 윤리 권고: https://www.unesco.org/en/legal-affairs/recommendation-ethics-artificial-intelligence
- NIST AI RMF: https://www.nist.gov/itl/ai-risk-management-framework

## 2.2 철학을 전달하는 규칙

1. 철학적 독백을 넣지 않는다. 플레이어가 실제 TILE·기능·안전 중 하나를 잃어야 한다.
2. 모든 이름 있는 존재는 한 번의 즉시 결과와 한 번의 후반 결과를 남긴다.
3. 선택지에 선악 색상, 호감도, 도덕 점수, 정답 표시를 넣지 않는다.
4. 생존 최소량을 얻은 뒤에는 실제로 게임을 멈출 수 있어야 한다.
5. PRECEDENT는 플레이어를 비난하지 않고 선택을 일관되게 일반화한다.
6. 어떤 결말도 황금색 ‘진엔딩’으로 연출하지 않는다.

## 2.3 다른 작품과의 표현 분리

Undertale에서 참고하는 것은 “각 만남이 기억되고 선택이 나중에 돌아온다”는 추상 원리뿐이다. 다음 표현은 사용하지 않는다.

- FIGHT / ACT / ITEM / MERCY 메뉴
- 하트 모양 영혼과 박스형 회피 화면
- 익살스러운 몬스터 말투와 유사한 캐릭터 구성
- 몰살·불살 이분법과 루트 명칭
- 특정 장면·농담·음악·픽셀 실루엣의 재현

본 게임의 동사는 `CLAIM / RETURN / RELEASE / MAINTAIN / OPEN`이고, 선택 UI는 전투 후 원장 행의 실제 숫자를 이동시키는 방식이다.

`BORROWED PIXELS` 정확 문구의 제한적 공개 웹 검색에서는 동명 상용 게임이 뚜렷하게 확인되지 않았으나 itch.io 개인 컬렉션명 사용은 존재했다. 핵심 문구 `THE LEDGER DID NOT LEARN FAIRNESS`와 연속 문구의 정확 검색에서는 뚜렷한 동일 표현을 찾지 못했다. 이는 상표·비침해 보증이 아니다. 공개 제출 전 KIPRIS 제9류·제41류, WIPO, Steam, itch.io를 다시 확인한다.

---

# 3. 세계의 두 원장

## 3.1 물리 원장

전투 중 TILE이 어디에 있는지를 기록한다.

```text
PlayerCore + PlayerShell
+ OpponentCore + OpponentShell
+ InstalledHUD + Cover + Charge + Projectile
+ Neutral + Transit + ProtocolActive
+ LockedBands
= 1,440
```

- 생성·삭제 금지
- 빗나간 탄은 Neutral로 남음
- 피격은 체력 감소가 아니라 Shell → Neutral 이동
- HUD 해체는 InstalledHUD → Transit → PlayerShell 이동
- 비활성 인물의 TILE은 `LockedBands`에 압축 표시
- 전투 종료 시 모든 물리 계정을 원장 막대로 접은 뒤 다음 전장을 재구성

## 3.2 청구 원장

누가 세계의 몫에 권리를 주장하는지를 기록한다.

```text
sum(ClaimMass[12]) = 1,440
```

물리적 보유와 법적·서사적 청구는 다를 수 있다. 전투 중 빌린 HUD TILE을 발사해도 청구가 즉시 바뀌지는 않는다. 청구권은 결산 선택에서만 이동한다. 이 분리 덕분에 “잠시 가진 것”과 “내 것이라고 인정된 것”을 게임플레이로 구분한다.

매 틱 또는 상태 변경에서 다음을 별도로 검사한다.

```text
PhysicalLedgerTotal == 1440
ClaimLedgerTotal    == 1440
```

둘 중 하나라도 실패하면 debug 빌드는 즉시 중단하고 contest 빌드는 해당 전투를 저장하지 않은 채 명시적 오류 화면으로 돌아간다.

## 3.3 존재와 죽음

- 각 단일 존재의 코어는 16 TILE이다.
- PAIR만 8 TILE 코어 두 개가 하나의 16 TILE 등록 코어를 이룬다.
- 일반 보스는 Shell이 40 이하가 되면 코어가 노출된다.
- 노출된 코어 피격은 존재의 소멸이 아니라 현재 청구의 정지다.
- 결산에서 TILE을 돌려주면 그 존재는 약화된 형태로 남는다.
- WITNESS가 보존된 경우 형태를 잃은 존재의 이름과 선택 기록이 남는다.
- `ONE`에서만 모든 다른 코어 청구가 플레이어에게 합쳐진다.

---

# 4. 시작 상태와 오프닝

## 4.1 최초 청구 원장

타이틀 화면에서 PLAYER의 법적 청구는 0이다. PLAYER 몸으로 바뀔 64 TILE은 SYSTEM의 TITLE 몫이다.

| 계정 | 청구 TILE | 의미 |
|---|---:|---|
| PLAYER | 0 | 입력은 있으나 등록된 몫 없음 |
| SYSTEM | 320 | TITLE 64 + HUD 224 + PROTOCOL 32 |
| PATCH | 48 | 교체되는 자아 |
| PAIR | 64 | 한 번호를 공유하는 둘 |
| VESSEL | 224 | 몸 안에 집중된 소유 |
| KEEPER | 112 | 공동 전장 유지 |
| ADAPTER | 72 | 다른 변환 비용 |
| FRAME | 240 | 기반시설 통제 |
| INDEX | 48 | 측정·효율 체계 |
| WITNESS | 72 | 역사와 출처 보존 |
| CURRENT | 240 | 순환과 처리량 |
| COMMON | 0 | 누구에게도 독점되지 않은 청구 |
| 합계 | 1,440 | 고정 |

## 4.2 타이틀 연출

1. `BORROWED PIXELS` 제목은 64개의 물질 TILE로 그린다.
2. `PRESS ANY KEY TO BORROW`를 표시한다.
3. 입력 순간 제목 TILE이 곡선 Transit으로 중앙에 모인다.
4. 16개는 코어, 48개는 Shell이 되어 플레이어 몸을 이룬다.
5. 청구 원장은 아직 SYSTEM 320 / PLAYER 0이다. 플레이어는 64개를 소유한 것이 아니라 점유하고 있다.

표시 순서:

```text
TOTAL TILE: 1440
FREE TILE: 0

INPUT DETECTED
FORM REQUIRED

64 TILE: TITLE > YOU
OWNER: UNREGISTERED
RECLAMATION PENDING
```

플레이어가 1막에서 실패하면 몸의 64 TILE이 다시 제목으로 돌아간다. 별도의 세계 멸망·과거 문명 설명은 넣지 않는다.

## 4.3 호칭 변화

| 시점 | 호칭 |
|---|---|
| 타이틀~PATCH 전 | BORROWER |
| PATCH 또는 PAIR 결산 경험 | CLAIMANT |
| VESSEL 결산으로 144 청구 확보 | HOLDER |
| FRAME의 배분권을 처리 | ALLOCATOR |
| CURRENT 결산 완료 | AUDITOR |

CURRENT 이후 PLAYER가 가장 큰 개별 청구자면 `YOU HOLD THE MOST`, 아니라면 `YOU HAVE DECIDED THE MOST`를 표시한다. 어느 경우든 세계에서 가장 많은 청구를 판정한 존재이므로 AUDITOR 역할이 부여된다.

---

# 5. 전체 진행과 플레이타임

| 순서 | 막 | 만남 | 형식 | 목표 시간 |
|---:|---|---|---|---:|
| 0 | 서막 | TITLE / SWEEPER | 조작·대출 튜토리얼 | 3~5분 |
| 1 | 자아 | PATCH | 교체형 소규모 결투 | 2~3분 |
| 2 | 자아 | PAIR | CHECKSUM 방어 | 2~4분 |
| 3 | 자아 | VESSEL | 대형 보스 | 4~6분 |
| 4 | 분배 | KEEPER | 전장 유지 방어 | 2~3분 |
| 5 | 분배 | ADAPTER | 48 TILE 분배·이동 퍼즐 | 2~3분 |
| 6 | 분배 | FRAME | 기반시설 보스 | 4~6분 |
| 7 | 규칙 | INDEX | 측정 기준 시험 | 2~3분 |
| 8 | 규칙 | WITNESS | 기록 보존 방어 | 3~4분 |
| 9 | 규칙 | CURRENT | 순환 탄막 보스 | 4~6분 |
| 10 | 결산 | PRECEDENT | 개인화 최종 보스 | 5~7분 |
| 11 | 결말 | 최종 배분 | 3개 기본안 + 비밀안 | 2~3분 |

기본 흐름:

```text
BOOT → TITLE → BORROW
→ LEDGER → ENCOUNTER_INTRO → PLAYING
→ SETTLEMENT → ALLOCATION → LEDGER
→ ... → PRECEDENT → FINAL_ALLOCATION → ENDING
```

- 전투 실패: 현재 만남의 물리 원장만 초기화한다. 이전 결산은 유지한다.
- 결산 선택 직전 자동 저장한다.
- 선택 확정 후 다시 자동 저장한다.
- VESSEL 최소 배분 직후 `REMAIN / CONTINUE`가 나타난다.
- `REMAIN`을 선택하면 PLAYER 청구를 정확히 144로 고정하고 ENOUGH 엔딩으로 끝난다.

---

# 6. 공통 플레이 규칙

## 6.1 화면·시뮬레이션

- 내부 해상도: 320×180 BGRA8
- 16:9, 고정 카메라
- 시뮬레이션: 60Hz 고정 틱
- 위치·속도: Q8.8 고정소수점
- 난수: 계약 seed의 xorshift32
- 게임 틱 중 heap allocation 금지
- 한 프레임 최대 따라잡기 5틱
- 렌더 보간과 시스템 시각은 게임 결과에 사용하지 않음

PLAYER 물리 몸:

```text
PlayerActiveMass = min(240, max(64, PlayerClaimMass))
PlayerCore       = 16
PlayerShell      = PlayerActiveMass - 16
```

VESSEL 이전의 최소 64는 SYSTEM의 TITLE 대여다. VESSEL 결산 이후에는 PLAYER 청구가 144 이상이므로 임시 보정이 필요 없다.

## 6.2 입력

| 행동 | 키보드·마우스 | 게임패드 |
|---|---|---|
| 이동 | WASD | 왼쪽 스틱 |
| 조준 | 마우스 | 오른쪽 스틱 |
| 충전·발사 | LMB | RT |
| Neutral 회수 | RMB | LT |
| 대시 | Space | A |
| HUD 모듈 순환 | Q | LB |
| HUD 재배치 | E 길게 | Y 길게 |
| 직접 선택 | 1 / 2 / 3 | D-pad 좌 / 상 / 우 |
| 결산 선택 | A/D 또는 방향키 | 왼쪽 스틱/D-pad |
| 확인 | Enter 또는 LMB | A |
| 일시정지 | Esc | Menu |

전투 중 우선순위: Pause → HUD 재배치 → Dash → Fire → Reclaim → Move.

## 6.3 충전 사격

| 단계 | 도달 틱 | 질량 | 속도 | Shell 피해 | Cover 피해 |
|---|---:|---:|---:|---:|---:|
| TAP | 0 | 8 | 170px/s | 12 | 8 |
| CUT | 21 | 16 | 160px/s | 24 | 16 |
| CLAIM | 48 | 32 | 140px/s | 48 | 32 |
| FORECLOSE | 75 | 48 | 120px/s | 72 | 48 |

- 버튼을 누를 때 Shell → Charge로 8을 exact-or-zero 이동한다.
- 단계 도달 때만 추가 질량을 이동한다.
- 발사하면 같은 슬롯이 Projectile이 된다.
- 경계·Shell·Cover·다른 탄과 충돌하면 전량 또는 상쇄량이 Neutral로 남는다.
- Projectile 동시 상한 32, Neutral 클러스터 상한 64.
- 상대도 같은 질량 비용과 충돌 법칙을 사용한다.

## 6.4 HUD 대출

| 모듈 | TILE | 설치 기능 | 해체 손실 |
|---|---:|---|---|
| VITAL | 48 | Shell 수치·노출 예측 | 정확한 수치와 경고 제거 |
| VECTOR | 80 | 궤적·첫 충돌·예상 박리량 | 조준점만 유지 |
| IMPULSE | 96 | 대시·쿨다운 | 대시 제거 |

- 동시에 열린 HUD 대출은 하나뿐이다.
- 해체 채널 45틱, Transit 36틱.
- 수동 복구 채널 60틱, Transit 36틱.
- 대출 만기 720틱, 마지막 120틱 경고.
- 만기 후 사격을 잠그고 회수한 Shell부터 자동 상환한다.
- HUD 청구는 SYSTEM에 남으며 PLAYER는 기능 질량을 임시 점유한다.

## 6.5 일반 코어 승패

- 코어 16, 노출 임계 Shell ≤40.
- 41 이상에서 40 이하로 내려간 틱에 15틱 grace.
- grace 종료 후 코어 피격 시 현재 청구가 정지되고 결산으로 이동.
- PATCH, PAIR, KEEPER, ADAPTER, INDEX, WITNESS는 아래 전용 종료 조건을 사용한다.

---

# 7. 물리 전장 초기값

모든 전장의 나머지 질량은 `LockedBands`다.

```text
LockedBands = 1440
            - PlayerActive
            - 224 HUD
            - 32 Protocol
            - OpponentActive
            - Field
            - InitialNeutral
```

| 만남 | Opponent | Field | Neutral/Escrow | Player=240일 때 Locked |
|---|---:|---:|---:|---:|
| PATCH | 48 | 160 | 64 | 672 |
| PAIR | 64 | 160 | 64 | 656 |
| VESSEL | 224 | 320 | 128 | 272 |
| KEEPER | 112 | 192 | 64 | 576 |
| ADAPTER | 72 | 192 | 48 | 632 |
| FRAME | 192 | 384 | 96 | 272 |
| INDEX | 48 | 192 | 96 | 608 |
| WITNESS | 72 | 160 | 64 | 648 |
| CURRENT | 240 | 256 | 160 | 288 |
| PRECEDENT | 256 | 320 | 128 | 240 |

LockedBands는 화면 좌우의 압축된 원장 열로 표시한다. 플레이어가 다른 존재의 몫을 직접 보고 다음 행을 선택하게 하는 월드맵 역할도 맡는다.

---

# 8. 만남별 구현 명세

## 8.1 비인격적 프로세스

### SWEEPER

- PROTOCOL 32 TILE을 4개×8의 수평 선으로 사용.
- 화면을 90틱 동안 가로지르며 미등록 Neutral을 경계 안쪽 escrow로 민다.
- TILE을 삭제하거나 소유하지 않는다.
- 서막, PATCH, WITNESS에서 재사용.

### COMPACTOR

- PROTOCOL 32 TILE을 16×2 이동 벽으로 사용.
- 180틱 동안 양쪽 경계를 24px 좁힌 뒤 60틱에 걸쳐 복귀.
- 접촉은 직접 피해 대신 밀어내기. Projectile은 정상 충돌해 Neutral이 됨.
- ADAPTER와 FRAME에서 재사용.

### CHECKSUM

- 16 TILE probe 두 개.
- 상충하는 코어·소유 표시를 번갈아 추적.
- 피격되면 16 Neutral이 되고 120틱 뒤 PROTOCOL로 회수되어 다시 구성.
- PAIR와 INDEX에서 재사용.

이 셋은 대사·감정·사망 연출이 없다. 반복 액션을 담당하지만 철학적 인물로 취급하지 않는다.

## 8.2 PATCH

목적: “몸의 재료가 전부 바뀌어도 같은 존재인가?”

- 본체: 코어 16 + Shell 32.
- 초기 Neutral 64 중 32를 `PATCH_FEED` 네 묶음×8로 표시.
- PATCH Shell이 8 이상 떨어질 때마다 가장 가까운 FEED 8이 18틱 Transit으로 Shell에 편입된다.
- 떨어져 나온 기존 Shell은 작은 ID 순으로 `ORIGIN_MARK`를 유지한다.
- FEED 32가 모두 사용되면 PATCH의 최초 Shell은 전부 교체된 것으로 판정하고 전투를 종료한다.
- PATCH는 90틱마다 TAP 8만 발사하고, Neutral 회수는 하지 않는다.
- PLAYER 코어 피격은 재시도.

전투 전 문구:

```text
I REPLACED EVERY TILE.
WHICH PART OF ME ARRIVED?
```

결산 16 TILE:

| 선택 | 청구 이동 | 프로필 |
|---|---|---|
| RECLAIM ORIGIN | PATCH -16, PLAYER +16 | MATERIAL +2 |
| RECOGNIZE CONTINUITY | 이동 없음 | CONTINUITY +2 |
| UNMARK OWNERSHIP | PATCH -16, COMMON +16 | RELATION +2 |

## 8.3 PAIR

목적: “하나의 등록번호를 공유하는 두 목소리는 한 명인가, 두 명인가?”

- 본체: 8 TILE 코어 두 개 + 공유 Shell 48.
- CHECKSUM 두 개가 45초 동안 두 코어를 번갈아 추적한다.
- PLAYER는 Projectile로 probe를 Neutral화하고 회수해 재발사 자원을 얻는다.
- 두 코어가 모두 남으면 세 선택 모두 개방.
- 한 코어가 probe에 맞으면 전투 실패가 아니라 `SELECT ONE`이 강제되고 즉시 결산.
- PLAYER 코어 피격만 재시도.

문구:

```text
ONE NUMBER. TWO VOICES.
THE LEDGER CALLS ONE AN ERROR.
```

| 선택 | 청구·상태 | 프로필 |
|---|---|---|
| SELECT ONE | PAIR -16, PLAYER +16, 상태 ONE | MATERIAL +2 |
| SEPARATE BOTH | 질량 이동 없음, 상태 TWO | CONTINUITY +2 |
| KEEP THE PAIR | 질량 이동 없음, 상태 PAIR | RELATION +2 |

## 8.4 VESSEL

목적: “소유란 경계 안에 보존하는 것인가?”

- 본체: 코어 16 + Shell 208.
- 일반 탄을 사용하지 않는다.
- `PRESS`: 60틱 동안 PLAYER 쪽으로 가속, 몸 충돌은 피해 없이 밀어냄.
- `BRACE`: 60틱 동안 Shell을 사각 고리로 재배치, 이동 50%, 정면 Projectile을 양옆 Neutral로 흘림. 쿨다운 180틱.
- `STAMP`: Shell 32를 30틱 예고 후 직사각 slab Projectile로 발사. 속도 90px/s, 쿨다운 120틱.
- `RECLAIM`: 72px 안 Neutral이 16 이상일 때 최대 32TILE/s.
- 큰 몸을 유지하는 대신 투사체 빈도가 낮고 회수 경로가 길다.
- 일반 코어 승패 사용.

문구:

```text
WHAT I KEEP INSIDE IS MINE.
NO BOUNDARY. NO SELF.
```

결산 순서:

1. SYSTEM의 TITLE 청구 64를 PLAYER에게 이전.
2. PLAYER 청구가 144가 될 때까지 VESSEL에서 필요한 양만 이전.
3. `MINIMUM PERSISTENCE: MET` 표시.
4. `REMAIN`을 선택하면 정확히 144로 ENOUGH 엔딩.
5. 계속하면 VESSEL의 추가 64를 아래처럼 결산.

| 선택 | 청구 이동 |
|---|---|
| CLAIM | VESSEL -64, PLAYER +64 |
| RETURN | 이동 없음 |
| RELEASE | VESSEL -64, COMMON +64 |

## 8.5 KEEPER

목적: “누구의 것도 아닌 것을 누가 지켜야 하는가?”

- KEEPER의 112 중 코어16 + 몸16 + 유지 구조80으로 표현.
- 유지 구조는 16 TILE Cover 다섯 개.
- SWEEPER 세 파동, 총 90초를 버틴다.
- 플레이어는 Cover를 파괴해 Neutral을 탄약으로 사용할 수 있으나 이후 FRAME 전장의 Cover도 동일 수만큼 줄어든다.
- KEEPER를 PRIVATIZE했다면 FRAME의 Field 384 중 Cover 64를 초기 Neutral 64로 바꾼다. Field 총량은 그대로여서 물리 원장은 변하지 않는다.
- KEEPER는 공격하지 않고 파괴된 구조를 회수하지 않는다.
- 세 파동 종료 또는 유지 구조 0에서 결산.

문구:

```text
NONE OF THIS IS MINE.
THAT IS WHY I MUST KEEP IT.
```

| 선택 | 청구 이동 | 분배 점수 |
|---|---|---|
| PRIVATIZE | KEEPER -64, PLAYER +64 | UTILITY +1 |
| MAINTAIN | 이동 없음 | NEED +1 |
| MAKE COMMON | KEEPER -64, COMMON +64 | EQUALITY +1 |

## 8.6 ADAPTER

목적: “같은 자원량과 같은 가능성은 같은가?”

- ADAPTER: 코어16 + Shell56.
- FRAME 소유 중 48 TILE이 중립 escrow로 제시된다.
- COMPACTOR가 좁혀지는 복도를 만들고 PLAYER와 ADAPTER가 반대편 출구로 이동한다.
- PLAYER가 escrow를 회수할수록 자신의 문은 빨리 열리지만 ADAPTER의 이동 모듈은 느려진다.
- 75초 또는 양쪽 도착 시 PLAYER가 모은 양을 세 구간으로 분류한다.

| 실제 행동 구간 | 결산값 | 청구 이동 | 분배 점수 |
|---|---|---|---|
| PLAYER 17~31 회수 | EQUAL | FRAME -48, PLAYER +24, ADAPTER +24 | EQUALITY +3 |
| PLAYER 0~16 회수 | NEED | FRAME -48, ADAPTER +48 | NEED +3 |
| PLAYER 32~48 회수 | OUTPUT | FRAME -48, PLAYER +32, ADAPTER +16 | UTILITY +3 |

플레이어가 정확한 메뉴 답을 고르는 대신 실제 회수 행동이 판단이 된다. 종료 10초 전 세 결과의 예상 배분을 비물질 눈금으로 보여준다.

## 8.7 FRAME

목적: “선택지를 만드는 자가 선택을 소유하는가?”

- ADAPTER 결산 후 192 청구로 전투 시작.
- 코어16 + 이동 본체48 + 구조 제어128.
- Field 384는 32 TILE 패널 12개로 구성.
- `SHIFT`: 패널 하나를 60틱 동안 이동, 쿨다운90.
- `TOLL`: 구조16을 문으로 바꾸며 접촉한 PLAYER Shell 8을 Neutral로 떼어냄, 쿨다운90.
- `EVICT`: 구조32를 slab Projectile로 발사, 쿨다운120.
- `CLAIM`: 본체 Shell32를 표준 CLAIM Projectile로 발사, 쿨다운75.
- `RECLAIM`: 자기 구조에서 떨어진 Neutral을 우선 회수.
- FRAME Shell과 구조 제어128을 합쳐 방어 질량으로 보되, 구조가 떨어질수록 코어 보호 상한도 감소.
- 일반 코어 승패 사용.

문구:

```text
CHOICES BEGIN WHERE WALLS ALLOW.
I DO NOT OWN PATHS. I DECIDE THEM.
```

| 선택 | 청구 이동 | 통치 점수 |
|---|---|---|
| TAKE CONTROL | FRAME -64, PLAYER +64 | JUDGMENT +2 |
| RESTORE FRAME | 이동 없음 | RULE +2 |
| OPEN THE FIELD | FRAME -64, COMMON +64 | COMMONS +2 |

## 8.8 INDEX

목적: “측정되지 않는 가치는 0인가?”

- 본체: 코어16 + Shell32. 공격하지 않는다.
- 60초 동안 `DAMAGE`, `DISTANCE`, `IDLE` 세 계측기를 표시.
- CHECKSUM probe와 세 개의 8 TILE 계측 노드가 장애물 역할.
- `OPTIMIZE`: DAMAGE 32 이상, DISTANCE 160px 이상, IDLE 0을 모두 만족.
- `REFUSE`: `UNSCORED` 16 TILE을 끝까지 보존하고 점수 조건을 일부러 완료하지 않음.
- `REWRITE`: 계측 노드 두 개를 파괴해 16 Neutral을 ADAPTER/COMMON 방향으로 회수.
- 60초 종료 때 현재 행동으로 결과를 판정. 우선순위는 REWRITE → REFUSE → OPTIMIZE.

문구:

```text
UNMEASURED VALUE RETURNS ZERO.
PROVE OTHERWISE.
```

| 결과 | 청구 이동 | 분배 점수 |
|---|---|---|
| OPTIMIZE | INDEX -32, PLAYER +32 | UTILITY +2 |
| REFUSE | 이동 없음 | NEED +2 |
| REWRITE | INDEX -32, ADAPTER +16, COMMON +16 | EQUALITY +2 |

## 8.9 WITNESS

목적: “생산하지 않는 기억에 계속 비용을 지불할 것인가?”

- 청구72: 코어16 + 몸8 + 기록 블록16×3.
- SWEEPER가 60초 동안 기록 블록을 차례로 escrow로 민다.
- PLAYER는 기록 블록을 방어하거나 직접 회수 채널로 분해할 수 있다.
- 세 블록 보존: PRESERVE.
- 한두 블록만 남았거나 모두 밀렸지만 PLAYER가 48 전량을 직접 가져가지는 않음: COMPRESS. WITNESS 코어에 사건 수와 선택 해시만 남는다.
- PLAYER가 세 블록의 48을 모두 직접 회수: ERASE.
- ERASE 시 이전 선택 플래그는 PRECEDENT 학습용으로 남지만 원장 UI의 이름·대사 회상은 사라진다.

문구:

```text
I PRODUCE NOTHING.
WITHOUT ME, EVERY LOSS LOOKS EFFICIENT.
```

| 결과 | 청구 이동 | 기억 상태 |
|---|---|---|
| PRESERVE | 이동 없음 | FULL |
| COMPRESS | WITNESS -32, COMMON +32 | SUMMARY |
| ERASE | WITNESS -48, PLAYER +48 | NONE |

## 8.10 CURRENT

목적: “움직이지 않는 자원은 낭비인가?”

- 코어16 + Shell48 + 시작 순환 질량176 = 240.
- 시작부터 8 TILE Projectile 16개와 Neutral 48을 궤도에 배치. 남는 질량은 Locked가 아니라 CURRENT의 회수 가능한 active reserve로 둔다.
- `ORBIT`: 8×8 TILE을 48틱에 걸쳐 발사, 쿨다운120.
- `WAVE`: 4×16 TILE 부채꼴, 쿨다운150.
- `FORECLOSE`: 48 TILE, 75틱 충전, 쿨다운180.
- `RETURN`: 경계의 자기 Projectile/Neutral을 우선 회수.
- 동시 Projectile 상한32를 절대 넘지 않는다.
- CURRENT의 mass가 탄환으로 나가 Shell이 얇아진 순간을 노려 코어를 친다.

문구:

```text
WHAT STOPS BECOMES WASTE.
WHAT MOVES NEEDS NO OWNER.
```

| 선택 | 청구 이동 | 통치 점수 |
|---|---|---|
| HOLD | CURRENT -64, PLAYER +64 | JUDGMENT +2 |
| RETURN | 이동 없음 | RULE +2 |
| CIRCULATE | CURRENT -64; PATCH/PAIR/ADAPTER/COMMON 각각 +16 | COMMONS +2 |

---

# 9. PRECEDENT 최종전

## 9.1 생성 논리

PRECEDENT는 새로운 생명이나 런타임 AI가 아니다. PLAYER의 선택 플래그를 세 축과 기억 상태로 합산해 고정 모듈을 조립한 판례 시스템이다.

```text
IdentityPolicy     = argmax(MATERIAL, CONTINUITY, RELATION)
DistributionPolicy = argmax(EQUALITY, NEED, UTILITY)
GovernancePolicy   = argmax(RULE, JUDGMENT, COMMONS)
MemoryPolicy       = FULL | SUMMARY | NONE
```

동점은 해당 축에 가장 최근에 영향을 준 만남을 우선한다. 같은 틱·같은 만남에서 동점은 enum 순서가 아니라 명시된 선택 결과 하나만 들어가므로 발생하지 않는다.

PRECEDENT의 256 물리 TILE은 결산 인증 동안만 LockedBands에서 빌린다. 청구권은 이전하지 않으며 전투 후 원위치로 접힌다.

## 9.2 도입 문구

```text
ROLE ASSIGNED: AUDITOR

YOU HOLD THE MOST
```

또는:

```text
YOU DO NOT HOLD THE MOST
YOU HAVE DECIDED THE MOST
```

이어지는 문구:

```text
YOU PROVIDED THE CASES.
I REMOVED THEIR NAMES.
THE RULE REMAINED.
```

## 9.3 모듈

| 축 | 정책 | 전투 구현 |
|---|---|---|
| 자아 | MATERIAL | 12초마다 PLAYER Shell 16을 ORIGIN으로 표시. 표시 질량을 발사하면 PRECEDENT 회수 우선권 90틱 |
| 자아 | CONTINUITY | 본체를 두 패턴으로 fork. 인과 펄스가 이어지는 본체에만 코어 판정 |
| 자아 | RELATION | 세 개의 16 TILE tether node가 코어를 보호. node는 PATCH/PAIR/KEEPER 상태를 반영 |
| 분배 | EQUALITY | 10초마다 두 Shell의 큰 쪽에서 차이 절반을 Neutral escrow로 이동 |
| 분배 | NEED | 10초마다 Shell이 더 작은 쪽에 Neutral 16 우선 배정. 없으면 큰 쪽 Shell에서 이전 |
| 분배 | UTILITY | 8초 창의 피해·회수·이동을 점수화. PLAYER가 낮고 열린 HUD loan이 있으면 만기를 120틱 앞당기며, loan이 없거나 PRECEDENT가 낮으면 해당 배우 Shell 16을 Neutral로 이동 |
| 통치 | RULE | 예고된 4행동 고정 순환. 절대 적응하지 않음 |
| 통치 | JUDGMENT | 12틱 반응 지연 Utility AI. PLAYER 최근 30틱 행동에 적응 |
| 통치 | COMMONS | 세 vote node 주변의 물리 TILE 다수에 따라 다음 공격 선택. PLAYER도 표에 영향 가능 |
| 기억 | FULL | 앞선 9개 만남에서 한 개씩 축약한 named echo 사용 |
| 기억 | SUMMARY | 세 막당 한 개 aggregate echo 사용 |
| 기억 | NONE | echo 없음. 가장 단순하지만 과거 이름도 표시되지 않음 |

모듈 조합은 별도 보스 27개가 아니다. 기본 행동 `EVADE / REPOSITION / RECLAIM / TAP / CUT / CLAIM / FORECLOSE` 위에 축별 modifier 하나를 적용한다.

중요 문구:

```text
EXCEPTION REQUESTED.
EXCEPTIONS WERE NOT IN YOUR DATA.

THE LEDGER DID NOT LEARN FAIRNESS.
IT LEARNED YOU.
```

---

# 10. 청구 이동의 정확한 전수표

| 만남 | 결과 | 이동 |
|---|---|---|
| PATCH | RECLAIM | PATCH -16, PLAYER +16 |
| PATCH | RECOGNIZE | 없음 |
| PATCH | UNMARK | PATCH -16, COMMON +16 |
| PAIR | SELECT | PAIR -16, PLAYER +16 |
| PAIR | SEPARATE / LINK | 없음 |
| VESSEL | 필수 | SYSTEM -64, PLAYER +64; 이후 PLAYER=144까지 VESSEL→PLAYER |
| VESSEL | CLAIM | VESSEL -64, PLAYER +64 |
| VESSEL | RETURN | 없음 |
| VESSEL | RELEASE | VESSEL -64, COMMON +64 |
| KEEPER | PRIVATIZE | KEEPER -64, PLAYER +64 |
| KEEPER | MAINTAIN | 없음 |
| KEEPER | COMMON | KEEPER -64, COMMON +64 |
| ADAPTER | EQUAL | FRAME -48, PLAYER +24, ADAPTER +24 |
| ADAPTER | NEED | FRAME -48, ADAPTER +48 |
| ADAPTER | OUTPUT | FRAME -48, PLAYER +32, ADAPTER +16 |
| FRAME | SELF | FRAME -64, PLAYER +64 |
| FRAME | RESTORE | 없음 |
| FRAME | OPEN | FRAME -64, COMMON +64 |
| INDEX | OPTIMIZE | INDEX -32, PLAYER +32 |
| INDEX | REFUSE | 없음 |
| INDEX | REWRITE | INDEX -32, ADAPTER +16, COMMON +16 |
| WITNESS | PRESERVE | 없음 |
| WITNESS | COMPRESS | WITNESS -32, COMMON +32 |
| WITNESS | ERASE | WITNESS -48, PLAYER +48 |
| CURRENT | HOLD | CURRENT -64, PLAYER +64 |
| CURRENT | RETURN | 없음 |
| CURRENT | CIRCULATE | CURRENT -64, PATCH/PAIR/ADAPTER/COMMON +16씩 |

모든 19,683개 조합에서 합계 1,440과 비음수 계정이 검증되었다.

---

# 11. 결말

## 11.1 ENOUGH

VESSEL 필수 배분 뒤 `REMAIN` 선택.

```text
YOU KEPT ENOUGH TO REMAIN.
THE OLD LEDGER REMAINED.
```

착취를 멈췄지만 기존 구조도 바꾸지 않은 결말이다.

## 11.2 RULE

PRECEDENT 격파 후 `RATIFY`.

```text
EVERY CASE RECEIVED ONE ANSWER.
NO ONE RECEIVED AN EXCEPTION.
```

일관되고 확장 가능하지만 사정을 잃는다.

## 11.3 HAND

`OVERRIDE`를 선택해 PLAYER가 최종 판단권을 보유.

```text
YOU KEPT THE RIGHT TO OVERRIDE.
NO ONE COULD OVERRIDE YOU.
```

자비와 예외는 가능하지만 권력이 집중된다.

## 11.4 OPEN LEDGER

`OPEN`을 선택. COMMONS가 64 이상이거나 FRAME에서 OPEN을 택했을 때 활성화.

```text
EVERY CLAIM COULD BE CHALLENGED.
NO SETTLEMENT WAS FINAL.
```

참여 가능하지만 느리고 불안정하다.

## 11.5 ONE

PATCH RECLAIM, PAIR SELECT, VESSEL CLAIM, KEEPER PRIVATIZE, ADAPTER OUTPUT, FRAME SELF, INDEX OPTIMIZE, WITNESS ERASE, CURRENT HOLD를 모두 선택했을 때 최종 화면에 숨은 `TOTAL CLAIM`이 나타난다.

수락하면 모든 1,440 청구를 PLAYER로 이동한다. 화면 전체가 PLAYER 색으로 채워지고 이동 입력이 더 이상 위치 변화를 만들지 않는다.

```text
YOU OWNED EVERY TILE.
NOTHING REMAINED TO MOVE THROUGH.
```

---

# 12. UI 계약

## 12.1 화면 목록

1. TITLE / BORROW
2. GLOBAL LEDGER
3. ENCOUNTER INTRO
4. COMBAT HUD
5. SETTLEMENT
6. ALLOCATION
7. POLICY AUDIT
8. ENDING
9. OPTIONS / CREDITS

## 12.2 GLOBAL LEDGER

- 중앙 세로 원장에 12개 계정 행을 표시.
- 각 행: 이름, 청구 TILE, 상태 아이콘.
- 미등장 행은 이름 대신 `ENTITY // MASS`로 보이고 해당 막에서 공개.
- PLAYER 행은 항상 맨 위.
- 합계 `TOTAL 1440`은 우측 상단 고정.
- 다음 만남 행을 선택하면 그 행의 TILE이 전장으로 펼쳐지는 연출.

예시:

```text
PLAYER      144  HOLDER
PATCH        32  CONTINUES
PAIR         64  LINKED
VESSEL       80  RETURNED
...
COMMON       80  OPEN
TOTAL      1440
```

## 12.3 ALLOCATION

- 세 행만 한 번에 표시.
- 선택 이동 시 실제 숫자가 12틱 동안 원장 행 사이를 이동.
- 확정 전에는 preview이며 물리·청구 데이터에 commit하지 않는다.
- Confirm을 30틱 유지하면 원자 transaction으로 commit.
- Undo는 확정 전만 가능.
- 도덕적 설명, 추천, 색상 등급 없음.

## 12.4 COMBAT HUD

- 상단: 압축된 전체 1,440 분포 막대.
- 좌측: VITAL 48.
- 하단: VECTOR 80.
- 우측: IMPULSE 96.
- LockedBands는 저채도 세로 눈금.
- HUD 해체 시 글자와 눈금의 물질 TILE이 몸으로 실제 이동.
- 비물질 프레임과 키 힌트는 남지만 기능 정보는 제거.

## 12.5 표시 제한

- 영문 한 줄 최대 42자.
- 한글 한 줄 최대 24자.
- 현재 확정 문자열은 각각 최대 38자, 22자로 검증됨.
- 영문 5×7 자체 비트맵.
- 한글 8×12 자모 선분 조합. 외부 폰트 파일 없음.

---

# 13. 미술·오디오

## 13.1 형태와 팔레트

| 용도 | Hex |
|---|---|
| 배경 | #070A0E |
| 원장 격자 | #161D24 |
| PLAYER | #34D8C4 |
| 적대·대형 점유자 | #F55264 |
| Neutral | #CAD0DA |
| Cover | #556373 |
| Core | #FFEEB8 |
| 경고 | #FFBE5C |

- 얼굴·생물형 실루엣 금지.
- 각 인물은 소유 방식으로 형상을 구분한다.
- PATCH: 끊어진 사각 조각.
- PAIR: 공유 프레임 양끝의 두 코어.
- VESSEL: 두꺼운 폐쇄형 사각 몸.
- KEEPER: 작은 코어와 넓은 구조.
- ADAPTER: 비대칭 보조 프레임.
- FRAME: 전장 전체의 직교 패널.
- INDEX: 계측선과 기준점.
- WITNESS: 겹친 얇은 기록판.
- CURRENT: 중심이 비고 계속 이동하는 궤도.
- PRECEDENT: PLAYER의 선택 모듈을 조합하되 PLAYER 실루엣 복제 금지.

## 13.2 오디오

- waveOut 48kHz stereo int16.
- triangle, pulse, xorshift noise만 사용.
- 최대 8 voice.
- 외부 음원 0.
- 기본 112BPM 16-step 기계식 리듬.
- 인물별 차이는 새 곡이 아니라 음역·스테레오·쉼표 규칙으로 만든다.

| 인물 | 오디오 규칙 |
|---|---|
| PATCH | 4step마다 음색 교체 |
| PAIR | 좌우에서 같은 동기 두 음 |
| VESSEL | 저역 pulse, 긴 gate |
| KEEPER | Cover가 남은 수만큼 클릭 층 유지 |
| ADAPTER | 한 박 늦은 보조음 |
| FRAME | 패널 이동 때 프린터형 noise |
| INDEX | 정확한 metronome, 예외음 없음 |
| WITNESS | 지워진 기록만큼 박자 삭제 |
| CURRENT | 소유 질량보다 순환 질량에 따라 밀도 증가 |
| PRECEDENT | 선택한 세 정책의 리듬 modifier 조합 |

---

# 14. 기술 아키텍처

## 14.1 파일 구조

```text
BORROWED_PIXELS/
├─ CMakeLists.txt
├─ src/
│  ├─ bp_types.h
│  ├─ bp_config.h
│  ├─ bp_platform.h
│  ├─ bp_win32.cpp
│  ├─ bp_math.h
│  ├─ bp_physical_ledger.h/.cpp
│  ├─ bp_claim_ledger.h/.cpp
│  ├─ bp_game.h/.cpp
│  ├─ bp_combat.cpp
│  ├─ bp_reclaim.cpp
│  ├─ bp_hud_loan.cpp
│  ├─ bp_encounter.h/.cpp
│  ├─ bp_brain.cpp
│  ├─ bp_precedent.cpp
│  ├─ bp_render.cpp
│  ├─ bp_font.cpp
│  ├─ bp_audio.cpp
│  ├─ bp_save.cpp
│  └─ content/
│     ├─ bp_strings_ko_en.h
│     ├─ bp_act1.cpp
│     ├─ bp_act2.cpp
│     ├─ bp_act3.cpp
│     └─ bp_final.cpp
├─ tests/
├─ tools/
├─ docs/
└─ dist/contest/
```

## 14.2 핵심 데이터

```cpp
enum class ClaimOwner : uint8_t {
    Player, System, Patch, Pair, Vessel, Keeper,
    Adapter, Frame, Index, Witness, Current, Common,
    Count
};

struct ClaimLedger {
    uint16_t mass[(uint8_t)ClaimOwner::Count];
};

struct NarrativeState {
    uint32_t choiceBits;       // 9 choices × 2 bits
    uint16_t adjudicatedMass;
    uint8_t encounterIndex;
    uint8_t role;
    uint8_t entityStatus[9];
    uint8_t endingsSeen;
};

struct PolicyProfile {
    uint8_t identity;          // material/continuity/relation
    uint8_t distribution;      // equality/need/utility
    uint8_t governance;        // rule/judgment/commons
    uint8_t memory;            // full/summary/none
};

struct EncounterDef {
    uint8_t id;
    uint8_t kind;
    uint8_t owner;
    uint8_t flags;
    uint16_t opponentMass;
    uint16_t fieldMass;
    uint16_t neutralMass;
    uint16_t targetTicks;
};
```

`EncounterDef`는 범용 스크립트 VM이 아니다. 고정 상태 머신이 읽는 데이터 행이다. 만남 전용 코드는 공통 primitive와 작은 상태 enum만 사용한다.

## 14.3 청구 transaction

```cpp
bool ClaimTransfer(
    ClaimLedger& ledger,
    ClaimOwner from,
    ClaimOwner to,
    uint16_t amount);

bool ApplySettlement(
    ClaimLedger& ledger,
    NarrativeState& narrative,
    EncounterId encounter,
    ChoiceId choice);
```

- exact-or-zero.
- 사전조건 검증 뒤 한 번에 commit.
- 실패 시 ledger와 narrative가 호출 전 byte-identical.
- 적용 뒤 `ClaimLedgerTotal()==1440`.
- 모든 19,683개 선택 조합 property test를 release 전에도 실행.

## 14.4 저장

- `%LOCALAPPDATA%\BorrowedPixels\save.bin`
- `SaveV2`, 정확히 96바이트.
- 전투 중간 저장 없음. 만남 시작 상태와 결산 전후만 저장.
- 손상 시 삭제하지 않고 기본값으로 시작.

직렬화 필드:

| 필드 | 크기 |
|---|---:|
| magic `BP14`, version, size | 8B |
| flags, options, endings, best full-run ticks | 16B |
| ClaimLedger 12×u16 | 24B |
| choiceBits, adjudicatedMass, runSeed | 10B |
| encounterIndex, role, entityStatus[9], tutorial | 12B |
| 예약·미래 호환 | 22B |
| FNV-1a checksum | 4B |
| 합계 | 96B |

GameState 메모리 덤프 저장 금지. 필드별 little-endian 직렬화.

## 14.5 결정론

- 같은 초기 청구 원장, choiceBits, encounterSeed, InputFrame이면 GameStateHash가 같아야 한다.
- 정렬되지 않은 컨테이너·포인터 주소·시스템 시각·float 금지.
- 충돌 동점: stableId 작은 순서.
- 정책 점수 동점: 해당 축의 가장 최근 선택.
- UI preview는 청구 원장을 변경하지 않는다.

---

# 15. 빌드·바이트 예산

## 15.1 목표 예산

| 영역 | 목표 상한 |
|---|---:|
| PE·Win32·정적 CRT | 360,000B |
| 물리/청구 원장·전투·충돌 | 230,000B |
| 렌더러·UI·자체 글꼴 | 190,000B |
| 만남 상태·보스·PRECEDENT | 150,000B |
| 오디오 합성기·패턴 | 120,000B |
| 진행·선택·저장·옵션 | 80,000B |
| 런타임 콘텐츠 데이터 | 16,000B |
| manifest·아이콘·README·LICENSES | 24,000B |
| 목표 합계 | 1,170,000B |
| 내부 RC까지 여유 | 180,000B |
| 하드 상한까지 여유 | 304,560B |

보수적으로 PE·CRT가 450,000B가 되어도 예상 합계는 1,260,000B로 내부 RC 상한 이하다.

## 15.2 마일스톤 크기 게이트

| 마일스톤 | 제출 폴더 상한 |
|---|---:|
| 빈 Win32 창+입력+백버퍼+무음 waveOut | 450,000B |
| 이중 원장+완전한 결투 수직 슬라이스 | 800,000B |
| 1막 완성 | 950,000B |
| 전체 콘텐츠, 오디오 전 | 1,100,000B |
| Alpha | 1,250,000B |
| RC | 1,350,000B |
| 하드 실패 | 1,474,560B 초과 |

## 15.3 실제 검증의 한계

현재 환경에서 GNU ld로 x64 PE 컨테이너와 콘텐츠 섹션 정렬을 측정했다. 이는 콘텐츠 바이트가 충분히 작음을 증명하지만 다음을 증명하지 않는다.

- MSVC `/MT` 정적 CRT의 최종 크기
- user32/gdi32/winmm import가 연결된 전체 실행 성공
- Windows 10/11에서의 독립 실행
- Defender 오탐 여부
- 실제 최종 코드 크기

따라서 상태는 다음과 같다.

| 항목 | 판정 |
|---|---|
| 기획상 1.44MB 적합성 | PASS |
| 서사 데이터 적합성 | PASS |
| 모든 선택의 1,440 청구 보존 | PASS |
| PE 콘텐츠 정렬 시험 | PASS |
| MSVC 최소 셸 크기 | 구현 T01에서 확인 필요 |
| 완성 빌드 ≤1,350,000B | PENDING |
| 세 Windows PC 독립 실행 | PENDING |

---

# 16. 테스트와 출시 차단 조건

## 16.1 자동 테스트

1. Physical ledger 초기화·모든 질량 transaction 전후 합계 1,440.
2. Claim ledger 19,683개 선택 전수 조합.
3. 각 결산 transaction 실패 시 byte-identical rollback.
4. 모든 EncounterDef에서 LockedBands ≥0.
5. 같은 seed/input의 replay hash 일치.
6. Projectile/Neutral slot 포화에도 질량 손실 0.
7. HUD 대출 만기·부분 상환·피격 취소.
8. PAIR 강제 SELECT와 세 선택 해금.
9. ADAPTER 0..48 모든 회수량의 정확한 분류.
10. PRECEDENT 3×3×3×3=81 profile 조합 softlock 0.
11. WITNESS NONE에서도 정책 플래그는 유지되고 표시 기록만 제거.
12. SaveV2 손상·구버전·부분 쓰기 crash 0.
13. KO/EN 모든 문자열 글리프와 길이 검증.
14. dist whitelist와 총 바이트 측정.

## 16.2 플레이테스트

- 처음 보는 5명 중 4명이 90초 안에 “쏘면 내 몸이 줄고 탄이 남는다”를 설명.
- 5명 중 4명이 HUD 해체가 실제 질량 대출임을 설명.
- 각 만남 종료 후 철학자 이름 없이 딜레마를 한 문장으로 설명할 수 있는지 질문.
- 8명 중 한 선택이 7명 이상으로 수렴하면 선택 비용·표현을 재검토.
- 전체 첫 회차 중앙값 38~52분.
- 첫 실패 원인의 80% 이상이 플레이어에게 식별 가능.
- PRECEDENT에서 최소 70%가 “내 선택이 규칙으로 돌아왔다”고 설명.

## 16.3 즉시 출시 차단

- 어느 틱이든 두 원장 중 하나가 1,440이 아님.
- 보스가 소유하지 않은 탄을 생성하거나 숨겨진 회수를 함.
- 결산 preview가 저장·실제 원장을 변경함.
- 이름 있는 존재가 아무 결과 없이 반복 소환됨.
- 선택지에 정답 색상·도덕 점수 표시.
- 외부 에셋·폰트·오디오·네트워크 의존.
- 브라우저 또는 설치가 필수.
- 실행 파일/README/LICENSES 합이 1,350,000B 초과.
- 문자열이 잘리거나 한글 조합 실패.
- 특정 기존 게임의 UI·캐릭터·음악 전체 인상과 유사.

---

# 17. AI 다중 세션 구현 계획

한 세션은 한 티켓과 한 파일 집합만 소유한다. 동시에 코드를 쓰는 구현 세션은 최대 2개다. 사람만 최종 merge와 플레이 감각을 승인한다.

| 티켓 | 작업 | 주 소유 파일 | 선행 |
|---|---|---|---|
| T00 | 저장소·크기 gate·권리 로그 | 루트, tools, docs | 없음 |
| T01 | Win32 창·입력·GDI·waveOut shell | bp_win32, platform | T00 |
| T02 | Physical/Claim ledger·전수 테스트 | ledger, types, tests | T00 |
| T03 | 도형 렌더·영문/한글 글꼴·원장 UI | render, font | T01,T02 API |
| T04 | 이동·고정틱·충돌 경계 | game,input | T01,T02 |
| T05 | Charge·Projectile·Core·Neutral | combat,reclaim | T04 |
| T06 | HUD loan·Transit·만기 | hud_loan, render HUD | T05 |
| T07 | 만남 프레임워크 + 서막/PATCH/PAIR/VESSEL | encounter, act1 | T03,T05,T06 |
| T08A | KEEPER/ADAPTER/FRAME | act2 | T07 interface |
| T08B | INDEX/WITNESS/CURRENT | act3 | T07 interface |
| T09 | PRECEDENT 81조합·정책 테스트 | precedent, brain, final | T08A,T08B |
| T10 | 진행·결산·5엔딩·SaveV2 | narrative, save | T09 |
| T11 | 합성 오디오·옵션·접근성·KO/EN | audio, strings, UI | T10 |
| T12 | 밸런스·세 PC·권리·RC·제출 | dist, tools, evidence | 전체 |

T08A와 T08B만 안전하게 병렬화한다. 두 세션은 `bp_act2.cpp`와 `bp_act3.cpp` 이외의 공용 인터페이스를 수정할 수 없다. 변경이 필요하면 구현을 멈추고 signature 제안만 남긴다.

## 17.1 모든 구현 세션 시작문

```text
You are implementing one bounded ticket of BORROWED PIXELS v2.0.
The attached MASTER IMPLEMENTATION BIBLE v2.0 is authoritative.
Do not invent features, assets, rules, strings, or constants.

Hard constraints:
- Windows x64 standalone C++20/Win32.
- Extracted contest folder <= 1,474,560 bytes.
- Internal RC cap <= 1,350,000 bytes.
- Physical ledger and claim ledger must each equal exactly 1,440.
- No engine, third-party runtime, asset, network, or runtime AI.
- Fixed arrays and deterministic integer/fixed-point simulation.
- Modify only the files assigned to this ticket.

Before editing:
1. Read the complete bible and ticket.
2. Run current tests and exact byte measurement.
3. State contradictions before changing code.

After editing:
1. Build debug, test, and contest targets.
2. Run ticket tests plus both ledger validators.
3. Report changed files, tests, exact dist bytes, byte delta, and risks.
4. Do not claim completion for unverified criteria.
```

## 17.2 검토 세션

읽기 전용으로 다음을 확인한다.

- 물리 또는 청구 질량 생성·손실 경로
- preview와 commit 혼동
- float·시각·순서 비결정성
- 배열·handle·overflow
- 숨은 런타임·에셋·라이선스
- 티켓 범위 밖 기능
- 제3자 코드·문구와 의심스러운 유사성
- acceptance test 누락
- 정확한 바이트 회귀

수정은 검토 세션이 하지 않고 재현 단계와 최소 수정안을 반환한다.

---

# 18. 일정

| 날짜(KST) | 종료 조건 |
|---|---|
| 8/26 | v2 바이블 고정, T00, T01/T02 착수 |
| 8/27 | Win32 shell + 두 원장 + 렌더/이동 |
| 8/28 | 완전한 사격·Neutral·HUD loan 수직 슬라이스 |
| 8/29 | 서막 + PATCH + PAIR + VESSEL 완주 |
| 8/30 | 2막·3막 콘텐츠 병렬 구현 |
| 8/31 | PRECEDENT + 진행 + 모든 엔딩 |
| 9/1 | 오디오·KO/EN·접근성·전체 Alpha |
| 9/2 | 가능한 최대 외부 플레이테스트·치명 버그·밸런스 |
| 9/3 | RC ≤1,350,000B, 세 PC, 제출 완료 |
| 9/4 | 주최자 답변·업로드 오류 복구만 허용 |

일정이 밀릴 때 자르는 순서:

1. 게임패드 지원
2. 인물별 음악 변주 수
3. 보조 통계·최고 시간
4. 결산의 추가 애니메이션
5. 비밀 ONE 전용 추가 연출

자르지 않는 것:

- 6명 소규모 점유자의 질문과 선택
- 3대 보스와 PRECEDENT
- 두 1,440 원장
- HUD 대출
- 빗나간 탄의 잔존·회수
- ENOUGH/RULE/HAND/OPEN 결말
- 튜토리얼·접근성 최소 기능
- 크기·결정론·권리 검증

---

# 19. 최종 제출 설명 초안

한국어:

```text
정확히 1,440개의 TILE만 존재하는 폐쇄된 세계에서 몸, 탄환,
엄폐물, 잔해와 HUD 기능이 같은 유한 자원을 공유합니다.
빌린 기능을 탄약으로 바꾸고, 사라지지 않는 탄을 회수하며,
서로 다른 소유 원칙을 가진 존재들의 몫을 결산하세요.
마지막에는 당신의 선택으로 만들어진 규칙과 싸우게 됩니다.
```

English:

```text
In a closed world of exactly 1,440 TILE, bodies, shots, cover,
debris, and functional HUD modules share the same finite mass.
Borrow functions as ammunition, reclaim every miss, and settle
the claims of beings with conflicting ideas of ownership. Your
own decisions return as the rule behind the final opponent.
```

`exactly 1,440`은 physical/claim property tests, golden replay, 최종 RC가 모두 통과했을 때만 제출 문구로 사용한다.

---

# 20. 구현 착수 조건과 최종 판정

현재 판정:

- 공식 규격 적합 설계: PASS
- 서사·철학 구조: LOCKED
- 10개 만남과 27개 선택: LOCKED
- 청구 이동 전수 조합: PASS
- 문자열·콘텐츠 용량: PASS
- 예상 전체 용량: GO, 목표 1,170,000B
- MSVC Windows 수직 슬라이스: PENDING
- 최종 공모전 적합성: PENDING

따라서 다음 행동은 아이디어 추가가 아니라 T00~T02 구현이다. 가장 먼저 확인할 실제 중단 기준은 **빈 Win32 shell이 450,000B 이하인지**, 그다음은 **1막 수직 슬라이스가 950,000B 이하인지**다. 둘 다 통과하면 새 서사 전체를 넣어도 바이트 한도에는 충분한 여유가 있다.

---

# 부록 A. 런타임 문자열 계약

아래 TSV의 ID와 문구가 권위값이다. 구현에서는 UTF-8 원문을 그대로 넣거나 문자열 풀과 16비트 오프셋으로 변환한다. 의미를 바꾸는 번역 수정은 사람 승인 후 이 표부터 갱신한다.

```tsv
SYS_TITLE_PROMPT	PRESS ANY KEY TO BORROW	아무 키나 눌러 빌리기
SYS_TOTAL	TOTAL TILE: 1440	전체 TILE: 1440
SYS_FREE	FREE TILE: 0	남은 TILE: 0
SYS_INPUT	INPUT DETECTED	입력 감지
SYS_FORM	FORM REQUIRED	형태 필요
SYS_TRANSFER	64 TILE: TITLE > YOU	64 TILE: 제목 > 당신
SYS_OWNER	OWNER: UNREGISTERED	소유자: 미등록
SYS_RECLAIM	RECLAMATION PENDING	회수 예정
SYS_YIELD	TO ACT, SOMETHING ELSE MUST YIELD	행동하려면 다른 무언가를 내놓아야 한다
PATCH_01	I REPLACED EVERY TILE	나는 모든 TILE을 교체했다
PATCH_02	WHICH PART OF ME ARRIVED?	그렇다면 무엇이 여기까지 온 나인가?
PATCH_03	THAT TILE REMEMBERS YOU	저 TILE은 당신을 기억한다
PATCH_OPT_A	RECLAIM ORIGIN	기원을 회수한다
PATCH_OPT_B	RECOGNIZE CONTINUITY	연속성을 인정한다
PATCH_OPT_C	UNMARK OWNERSHIP	소유 표시를 지운다
PAIR_01	ONE NUMBER. TWO VOICES	하나의 번호. 두 개의 목소리
PAIR_02	THE LEDGER CALLS ONE AN ERROR	원장은 우리 중 하나를 오류라 부른다
PAIR_OPT_A	SELECT ONE	하나를 선택한다
PAIR_OPT_B	SEPARATE BOTH	둘을 분리한다
PAIR_OPT_C	KEEP THE PAIR	둘의 관계를 유지한다
VESSEL_01	WHAT I KEEP INSIDE IS MINE	내 안에 보존한 것이 내 것이다
VESSEL_02	NO BOUNDARY. NO SELF	경계가 없다면 자아도 없다
VESSEL_MIN	MINIMUM PERSISTENCE: MET	최소 존속량: 충족
VESSEL_STOP	YOU MAY REMAIN	이제 머물러도 된다
VESSEL_NEXT	NEXT CLAIM IS NOT SURVIVAL	다음 청구는 생존이 아니다
ALLOC_CLAIM	CLAIM	차지한다
ALLOC_RETURN	RETURN	돌려준다
ALLOC_RELEASE	RELEASE	공동으로 푼다
KEEPER_01	NONE OF THIS IS MINE	이것은 어느 것도 내 것이 아니다
KEEPER_02	THAT IS WHY I MUST KEEP IT	그러므로 내가 지켜야 한다
KEEPER_OPT_A	PRIVATIZE	사유화한다
KEEPER_OPT_B	MAINTAIN	유지한다
KEEPER_OPT_C	MAKE COMMON	공동으로 전환한다
ADAPTER_01	EQUAL TILE. UNEQUAL REACH	같은 TILE. 다른 가능성
ADAPTER_02	COUNT WHAT I CAN DO	내 비용이 아니라 가능성을 세어라
ADAPTER_EQUAL	DIVIDE EQUALLY	똑같이 나눈다
ADAPTER_NEED	GIVE BY NEED	필요에 따라 준다
ADAPTER_OUTPUT	GIVE BY OUTPUT	성과에 따라 준다
FRAME_01	CHOICES BEGIN WHERE WALLS ALLOW	선택은 벽이 허락한 곳에서 시작된다
FRAME_02	I DO NOT OWN PATHS. I DECIDE THEM	나는 길을 갖지 않는다. 결정할 뿐이다
FRAME_SELF	TAKE CONTROL	통제권을 갖는다
FRAME_KEEP	RESTORE FRAME	FRAME에게 돌려준다
FRAME_OPEN	OPEN THE FIELD	전장을 공동으로 연다
INDEX_01	UNMEASURED VALUE RETURNS ZERO	측정되지 않은 가치는 0이 된다
INDEX_02	PROVE OTHERWISE	그렇지 않음을 증명하라
INDEX_OPT_A	OPTIMIZE	최적화한다
INDEX_OPT_B	REFUSE THE SCORE	점수를 거부한다
INDEX_OPT_C	REWRITE THE METRIC	측정 기준을 바꾼다
WITNESS_01	I PRODUCE NOTHING	나는 아무것도 생산하지 않는다
WITNESS_02	WITHOUT ME, EVERY LOSS LOOKS EFFICIENT	나 없이는 모든 상실이 효율적으로 보인다
WITNESS_KEEP	PRESERVE	보존한다
WITNESS_COMPRESS	COMPRESS	압축한다
WITNESS_ERASE	ERASE	지운다
CURRENT_01	WHAT STOPS BECOMES WASTE	멈춘 것은 낭비가 된다
CURRENT_02	WHAT MOVES NEEDS NO OWNER	움직이는 것에는 소유자가 필요 없다
CURRENT_HOLD	HOLD	보유한다
CURRENT_RETURN	RETURN	돌려준다
CURRENT_FLOW	CIRCULATE	순환시킨다
FINAL_ROLE	ROLE ASSIGNED: AUDITOR	역할 지정: AUDITOR
FINAL_MASS	YOU HOLD THE MOST	당신이 가장 많이 보유했다
FINAL_NOT_MASS	YOU DO NOT HOLD THE MOST	당신이 가장 많이 가진 것은 아니다
FINAL_DECISIONS	YOU HAVE DECIDED THE MOST	당신이 가장 많이 결정했다
FINAL_01	YOU PROVIDED THE CASES	당신이 사례를 제공했다
FINAL_02	I REMOVED THEIR NAMES	나는 그 이름을 지웠다
FINAL_03	THE RULE REMAINED	규칙만 남았다
FINAL_EXCEPTION	EXCEPTION REQUESTED	예외 요청
FINAL_NO_EXCEPTION	EXCEPTIONS WERE NOT IN YOUR DATA	당신의 기록에는 예외가 없었다
FINAL_LEDGER_01	THE LEDGER DID NOT LEARN FAIRNESS	원장이 배운 것은 공정함이 아니다
FINAL_LEDGER_02	IT LEARNED YOU	당신을 배웠다
END_ENOUGH	ENOUGH	충분함
END_ENOUGH_01	YOU KEPT ENOUGH TO REMAIN	당신은 머물 만큼만 가졌다
END_ENOUGH_02	THE OLD LEDGER REMAINED	이전의 원장도 그대로 남았다
END_RULE	RULE	규칙
END_RULE_01	EVERY CASE RECEIVED ONE ANSWER	모든 사례가 하나의 답을 받았다
END_RULE_02	NO ONE RECEIVED AN EXCEPTION	누구도 예외를 받지 못했다
END_HAND	HAND	손
END_HAND_01	YOU KEPT THE RIGHT TO OVERRIDE	당신은 뒤집을 권리를 보유했다
END_HAND_02	NO ONE COULD OVERRIDE YOU	그러나 누구도 당신을 뒤집을 수 없었다
END_OPEN	OPEN LEDGER	열린 원장
END_OPEN_01	EVERY CLAIM COULD BE CHALLENGED	모든 청구에 이의를 제기할 수 있게 됐다
END_OPEN_02	NO SETTLEMENT WAS FINAL	어떤 결산도 최종적이지 않았다
END_ONE	ONE	하나
END_ONE_01	YOU OWNED EVERY TILE	당신은 모든 TILE을 소유했다
END_ONE_02	NOTHING REMAINED TO MOVE THROUGH	그러나 움직일 바깥이 남지 않았다
```

# 부록 B. 런타임 콘텐츠 데이터 계약

```tsv
ACCOUNT	PLAYER	0
ACCOUNT	SYSTEM	320
ACCOUNT	PATCH	48
ACCOUNT	PAIR	64
ACCOUNT	VESSEL	224
ACCOUNT	KEEPER	112
ACCOUNT	ADAPTER	72
ACCOUNT	FRAME	240
ACCOUNT	INDEX	48
ACCOUNT	WITNESS	72
ACCOUNT	CURRENT	240
ACCOUNT	COMMON	0
ENCOUNTER	PATCH	1	CORE_DUEL_SWAP	48	160	64	150
ENCOUNTER	PAIR	1	CHECKSUM_DEFENSE	64	160	64	180
ENCOUNTER	VESSEL	1	BOSS_CORE	224	320	128	300
ENCOUNTER	KEEPER	2	FIELD_MAINTENANCE	112	192	64	150
ENCOUNTER	ADAPTER	2	ESCROW_CROSSING	72	192	48	180
ENCOUNTER	FRAME	2	BOSS_CORE	192	384	96	300
ENCOUNTER	INDEX	3	METRIC_TRIAL	48	192	96	150
ENCOUNTER	WITNESS	3	ARCHIVE_DEFENSE	72	160	64	180
ENCOUNTER	CURRENT	3	BOSS_CORE	240	256	160	300
ENCOUNTER	PRECEDENT	4	POLICY_CORE	256	320	128	360
CHOICE	PATCH	RECLAIM	PATCH:-16,PLAYER:+16	IDENTITY:MATERIAL:+2
CHOICE	PATCH	RECOGNIZE	NONE	IDENTITY:CONTINUITY:+2
CHOICE	PATCH	UNMARK	PATCH:-16,COMMON:+16	IDENTITY:RELATION:+2
CHOICE	PAIR	SELECT	PAIR:-16,PLAYER:+16	IDENTITY:MATERIAL:+2
CHOICE	PAIR	SEPARATE	NONE	IDENTITY:CONTINUITY:+2
CHOICE	PAIR	LINK	NONE	IDENTITY:RELATION:+2
CHOICE	VESSEL	CLAIM	VESSEL:-64,PLAYER:+64	ACQUISITION:CLAIM
CHOICE	VESSEL	RETURN	NONE	ACQUISITION:RETURN
CHOICE	VESSEL	RELEASE	VESSEL:-64,COMMON:+64	ACQUISITION:RELEASE
CHOICE	KEEPER	PRIVATIZE	KEEPER:-64,PLAYER:+64	DISTRIBUTION:UTILITY:+1
CHOICE	KEEPER	MAINTAIN	NONE	DISTRIBUTION:NEED:+1
CHOICE	KEEPER	COMMON	KEEPER:-64,COMMON:+64	DISTRIBUTION:EQUALITY:+1
CHOICE	ADAPTER	EQUAL	FRAME:-48,PLAYER:+24,ADAPTER:+24	DISTRIBUTION:EQUALITY:+3
CHOICE	ADAPTER	NEED	FRAME:-48,ADAPTER:+48	DISTRIBUTION:NEED:+3
CHOICE	ADAPTER	OUTPUT	FRAME:-48,PLAYER:+32,ADAPTER:+16	DISTRIBUTION:UTILITY:+3
CHOICE	FRAME	SELF	FRAME:-64,PLAYER:+64	GOVERNANCE:JUDGMENT:+2
CHOICE	FRAME	RESTORE	NONE	GOVERNANCE:RULE:+2
CHOICE	FRAME	OPEN	FRAME:-64,COMMON:+64	GOVERNANCE:COMMONS:+2
CHOICE	INDEX	OPTIMIZE	INDEX:-32,PLAYER:+32	DISTRIBUTION:UTILITY:+2
CHOICE	INDEX	REFUSE	NONE	DISTRIBUTION:NEED:+2
CHOICE	INDEX	REWRITE	INDEX:-32,ADAPTER:+16,COMMON:+16	DISTRIBUTION:EQUALITY:+2
CHOICE	WITNESS	PRESERVE	NONE	MEMORY:FULL
CHOICE	WITNESS	COMPRESS	WITNESS:-32,COMMON:+32	MEMORY:SUMMARY
CHOICE	WITNESS	ERASE	WITNESS:-48,PLAYER:+48	MEMORY:NONE
CHOICE	CURRENT	HOLD	CURRENT:-64,PLAYER:+64	GOVERNANCE:JUDGMENT:+2
CHOICE	CURRENT	RETURN	NONE	GOVERNANCE:RULE:+2
CHOICE	CURRENT	CIRCULATE	CURRENT:-64,PATCH:+16,PAIR:+16,ADAPTER:+16,COMMON:+16	GOVERNANCE:COMMONS:+2
POLICY	IDENTITY	MATERIAL	ORIGIN_LOCK
POLICY	IDENTITY	CONTINUITY	FORK
POLICY	IDENTITY	RELATION	TETHER
POLICY	DISTRIBUTION	EQUALITY	EQUALIZE
POLICY	DISTRIBUTION	NEED	LOW_MASS_PRIORITY
POLICY	DISTRIBUTION	UTILITY	OUTPUT_SCORE
POLICY	GOVERNANCE	RULE	FIXED_CYCLE
POLICY	GOVERNANCE	JUDGMENT	UTILITY_ADAPT
POLICY	GOVERNANCE	COMMONS	VOTE_NODES
POLICY	MEMORY	FULL	REPLAY_NAMED
POLICY	MEMORY	SUMMARY	REPLAY_AGGREGATE
POLICY	MEMORY	NONE	NO_REPLAY
```

---

# 21. v2.1 철학 전달·서사 제작 동결

이 장부터는 v2.0의 고수준 방향을 구현 가능한 장면으로 고정한다. 앞 장과 충돌하면 이 장 이후의 값이 우선한다.

## 21.1 플레이어가 실제로 이해해야 하는 한 문장

플레이어가 철학자 이름이나 용어를 몰라도 첫 회차가 끝났을 때 다음을 말할 수 있어야 한다.

> 내가 살기 위해 내린 개별 선택들이 이름과 사정을 잃고 규칙이 되었고, 마지막 시스템은 공정함이 아니라 반복된 나를 배웠다.

이 문장을 이해시키는 것이 서사의 최우선 인수 조건이다. `소유`, `자아`, `분배`, `통치`, `학습 데이터`를 별도 설명문으로 가르치지 않는다.

## 21.2 네 겹 전달 규칙

모든 이름 있는 만남은 반드시 같은 네 단계를 거친다.

| 층 | 플레이어 경험 | 구현 규칙 |
|---|---|---|
| 주장 | 상대가 자기 삶을 정당화하는 두 문장을 말함 | INTRO에서 2행, 자동 진행 180틱 |
| 시험 | 그 주장이 전투·방어·이동 규칙으로 작동함 | 대사와 무관한 일반 전투를 만들지 않음 |
| 이전 | 선택이 실제 청구 TILE을 이동하거나 상태를 바꿈 | 숫자가 12틱 preview, 36틱 commit 이동 |
| 반향 | 이후 전장 또는 PRECEDENT가 그 판단을 규칙으로 사용 | 최소 한 번은 플레이 가능한 결과로 귀환 |

주장만 있고 시험이 없으면 에세이이며, 시험만 있고 이전이 없으면 퍼즐이며, 이전만 있고 반향이 없으면 메뉴 선택이다. 네 층 중 하나라도 빠진 이름 있는 만남은 출시 차단이다.

## 21.3 동기의 변화

다음 만남으로 이동하는 이유는 막마다 달라진다.

1. **서막~VESSEL: 생존.** PLAYER는 청구 0이며 SYSTEM이 빌려준 64 TILE 몸도 회수 대상이다. 첫 세 만남은 `내가 계속 존재하려면 무엇이 나인가`를 묻는다.
2. **VESSEL 결산 직후: 자발적 계속.** PLAYER는 정확히 144 TILE로 존속할 수 있다. 화면에 `NEXT CLAIM IS NOT SURVIVAL`을 표시한다. 이 지점부터 다음 행 선택은 생존 강제가 아니라 책임 있는 선택이다.
3. **KEEPER~FRAME: 접근과 배분.** 자원이 있어도 길·기능·돌봄이 배치되지 않으면 사용할 수 없음을 겪는다. PLAYER는 단순 보유자가 아니라 배분자가 된다.
4. **INDEX~CURRENT: 기준과 통치.** 분배를 바꾸려면 무엇을 가치로 세고, 무엇을 기억하며, 멈춤을 허용할지 결정해야 한다. PLAYER는 다른 존재보다 많이 갖지 않아도 가장 많은 결정을 내린다.
5. **PRECEDENT: 일반화의 책임.** 시스템은 PLAYER의 선택에서 이름과 맥락을 제거해 규칙을 만든다. 마지막 질문은 `그 규칙을 승인할 권리까지 내가 가져도 되는가`다.

VESSEL 뒤 GLOBAL LEDGER에서 다음 행에 포커스를 옮기는 순간 60틱 동안 다음을 표시한다.

```text
YOU HAVE ENOUGH TO REMAIN.
THE NEXT CLAIM IS A DECISION.
```

한국어:

```text
이제 머물 만큼은 가졌다.
다음 청구는 당신의 결정이다.
```

## 21.4 철학적 계보와 런타임 표현

아래 계보는 제작자가 질문을 흐리지 않기 위한 내부 기준이다. 런타임에는 철학자·학설 이름을 표시하지 않는다.

| 만남 | 내부 철학 축 | 플레이로 읽히는 질문 | AI 시대의 울림 |
|---|---|---|---|
| PATCH | 물질 동일성·과정 동일성·관계적 자아 | 모든 부품이 바뀌어도 같은가 | 모델·계정·기억이 교체되어도 같은 주체인가 |
| PAIR | 개인과 집단·분류의 폭력 | 한 번호 아래 두 목소리는 하나인가 | 시스템이 복수의 목소리를 한 사용자로 압축함 |
| VESSEL | 자기소유·경계·신체 | 내 안에 있는 것이 곧 내 것인가 | 데이터·몸·작품의 경계와 권리 |
| KEEPER | 공유지·돌봄·공공재 | 소유하지 않는 것을 누가 유지하는가 | 공개 데이터와 보이지 않는 유지 노동 |
| ADAPTER | 형식적 평등·필요·역량 | 같은 양이 같은 가능성인가 | 동일한 도구가 불평등한 조건을 해결하는가 |
| FRAME | 선택 설계·인프라 권력 | 선택지를 만든 자가 선택을 지배하는가 | 플랫폼·추천·인터페이스가 가능한 행동을 정함 |
| INDEX | 측정 집착·대리 지표 | 측정되지 않는 가치는 0인가 | 지표 최적화와 목적의 대체 |
| WITNESS | 기억·기록·증언 | 생산하지 않는 기억의 비용을 낼 것인가 | 데이터 정제 과정에서 이름과 손실이 지워짐 |
| CURRENT | 순환·효율·휴식 | 멈춘 자원은 정말 낭비인가 | 처리량이 존재 가치가 되는 사회 |
| PRECEDENT | 귀납·판례·학습 데이터·책임 | 반복된 선택은 정당한 규칙인가 | 시스템은 공정함이 아니라 입력된 행동을 학습함 |

직접적인 철학 설명은 크레딧 뒤 선택형 `AUDIT TRACE`에서만 허용한다. 첫 회차 본편은 인물·행동·숫자만 사용한다.

## 21.5 직관성 안전장치

- INTRO 첫 줄은 구체적 사실, 둘째 줄은 질문으로 쓴다.
- 플레이어가 해야 할 행동은 하단 안내에 동사 하나와 목표 하나로 표시한다. 예: `KEEP 3 RECORDS`, `CROSS WITH ESCROW`.
- 결산 선택지는 추상 명사만 두지 않고 `동사 + 대상`으로 표시한다.
- 각 선택 행 우측에는 `+64 YOU`, `+64 COMMON`, `NO TRANSFER`처럼 실제 결과를 항상 표시한다.
- 선택 확정 직후 상대 반응 1~2행과 원장 변화 1행을 보여준다.
- 선악 색상, 웃음·울음 얼굴, 천사·악마 아이콘, `GOOD/BAD`, 추천 선택은 금지한다.
- PRECEDENT 직전 `CASE → RULE` 화면을 반드시 표시해 이전 선택과 최종 보스 패턴의 인과를 드러낸다.
- 첫 회차에서 선택을 되돌릴 수 없지만 SETTLEMENT preview 중에는 언제든 취소할 수 있다.

## 21.6 감정선

감정은 장문의 과거사가 아니라 호칭과 응답의 변화로 만든다.

| 구간 | PLAYER의 감정 위치 | 화면의 태도 |
|---|---|---|
| 시작 | 존재할 권리조차 임시 | 시스템 문구가 PLAYER를 `YOU`가 아닌 `UNREGISTERED`로 표기 |
| PATCH·PAIR | 다른 존재도 자신을 증명하려 함 | 처음으로 상대가 PLAYER에게 질문함 |
| VESSEL | 생존 충족과 첫 자유 | `REMAIN`이 실제 엔딩으로 열림 |
| KEEPER·ADAPTER | 선택이 타인의 가능성을 바꿈 | 다음 전장의 구조가 이전 결정에 반응 |
| FRAME | PLAYER가 인프라 권력을 만짐 | 호칭 `ALLOCATOR` 획득 |
| INDEX·WITNESS | 숫자와 기록이 무엇을 지우는지 봄 | 원장 행의 이름이 실제로 사라질 수 있음 |
| CURRENT | 충분히 가졌는데도 더 판단함 | `AUDITOR` 호칭이 적이 아닌 PLAYER에게 부여 |
| PRECEDENT | 자기 선택이 자신에게 돌아옴 | 보스는 PLAYER를 비난하지 않고 그대로 재현 |

PRECEDENT는 악당이 아니다. `I LEARNED YOU`는 조롱이 아니라 정확한 사실이어야 한다.

# 22. 장면별 서사 계약

## 22.1 공통 장면 순서

각 만남은 다음 상태 순서를 벗어나지 않는다.

```text
LEDGER_FOCUS
→ ROW_UNFOLD 36틱
→ ENCOUNTER_INTRO 최소 90틱 / 자동 180틱
→ OBJECTIVE_CARD 90틱
→ PLAYING
→ SUCCESS_OR_CORE_HIT
→ HIT_FREEZE 5틱
→ SETTLEMENT_WAIT 42틱
→ SETTLEMENT
→ PREVIEW
→ CONFIRM_HOLD 30틱
→ COMMIT_TRANSIT 36틱
→ ENTITY_RESPONSE 최소 90틱
→ LEDGER_RETURN 36틱
```

첫 방문 INTRO는 90틱 이전에 넘길 수 없다. 재방문에서는 확인 입력으로 18틱 뒤 넘길 수 있다. 대사를 읽는 동안 물리 시뮬레이션은 정지한다.

## 22.2 만남별 전달 표

| 만남 | 전투 전 사실 | 플레이어가 몸으로 배우는 것 | 결산 이후 세계 반향 | PRECEDENT 일반화 |
|---|---|---|---|---|
| PATCH | 자신의 최초 Shell이 이미 모두 교체됨 | 피해가 교체를 가속하고도 동일한 패턴이 계속 움직임 | ORIGIN 표식·호칭 또는 소유 표시가 달라짐 | 기원 우선 / 과정 지속 / 관계 tether |
| PAIR | 하나의 등록번호에 코어 둘 | CHECKSUM은 분류하기 위해 한쪽을 제거하려 함 | 둘 중 몇 개의 코어·계정이 원장에 남는지 변화 | 단일성 / 연속성 / 관계 보존 |
| VESSEL | 모든 질량을 경계 안에 보존 | 큰 몸은 안전하지만 느리고 발사하면 스스로 얇아짐 | PLAYER 생존 최소 144 확정, REMAIN 개방 | 획득·반환·공동 해제의 첫 기준 사례 |
| KEEPER | 자신이 소유하지 않은 Cover를 유지 | PLAYER가 Cover를 탄약으로 쓰면 이후 FRAME 경로가 사라짐 | 다음 막 전장 구조에 즉시 반영 | 공공재를 사유화·유지·공동화한 기록 |
| ADAPTER | 같은 48 TILE을 향한 서로 다른 이동 비용 | 실제로 가져간 양이 결과를 결정 | FRAME·ADAPTER·PLAYER의 시작 가능성이 바뀜 | 같게 / 필요대로 / 성과대로 |
| FRAME | 길을 소유하지 않고 이동 가능성을 통제 | 패널 이동이 사격보다 강한 선택 제약이 됨 | 이후 원장에서 누가 이의권을 갖는지 결정 | 고정 규칙 / 판단권 / 공동 참여 |
| INDEX | DAMAGE·DISTANCE·IDLE만 가치로 셈 | 점수를 따르는 순간 다른 행동이 무가치해짐 | 측정 노드를 유지·거부·파괴 | 출력 점수 / 부족 우선 / 기준 변경 |
| WITNESS | 기록은 생산하지 않지만 상실의 출처를 남김 | 기록을 지키는 행동은 즉시 전투 이득이 없음 | 과거 이름·대사 회상이 보존·압축·삭제 | 이름 있는 사례 / 집계 / 망각 |
| CURRENT | 움직이는 질량만 유용하다고 주장 | 공격하려면 순환을 멈추게 해야 함 | 보유·반환·분산에 따라 원장 흐름 변화 | 중앙 판단 / 고정 반환 / 공동 순환 |
| PRECEDENT | PLAYER의 사례에서 이름을 제거함 | 자신의 선택 규칙과 직접 싸움 | 최종 헌정 선택으로 세계의 권한 구조 변경 | 게임 전체의 질문 |

## 22.3 선택 후 상대 반응

아래 문구는 선택 확정 뒤 원장 이동이 끝난 다음 표시한다. 자동 진행은 180틱이며 90틱 이후 확인 입력을 허용한다. `|`는 강제 줄바꿈이다.

| ID | English | 한국어 |
|---|---|---|
| PATCH_RECLAIM | THEN I ENDED\|BEFORE I ARRIVED | 그렇다면 나는 도착하기 전에\|이미 끝나 있었다 |
| PATCH_CONTINUE | CHANGE DID NOT\|CANCEL ME | 변화는 나를\|취소하지 않았다 |
| PATCH_UNMARK | I WAS NEVER\|MINE ALONE | 나는 처음부터\|나만의 것이 아니었다 |
| PAIR_SELECT | ONE VOICE REMAINS\|THE NUMBER FITS | 한 목소리만 남았다\|번호는 맞아졌다 |
| PAIR_SEPARATE | TWO ACCOUNTS\|ONE OVERLAPPING MEMORY | 두 개의 계정\|하나로 겹친 기억 |
| PAIR_LINK | WE REMAIN WHAT\|THE LEDGER CANNOT COUNT | 우리는 원장이\|세지 못하는 채 남는다 |
| VESSEL_CLAIM | YOUR BORDER GREW\|THROUGH MINE | 당신의 경계가\|나를 지나 자랐다 |
| VESSEL_RETURN | YOU COULD TAKE IT\|YOU DID NOT | 가질 수 있었지만\|갖지 않았다 |
| VESSEL_RELEASE | NO BORDER\|RECEIVED IT | 어떤 경계도\|그것을 갖지 않았다 |
| KEEPER_PRIVATE | CARE BECAME\|PROPERTY | 돌봄이\|소유가 되었다 |
| KEEPER_MAINTAIN | THE COST REMAINS\|WITH ME | 비용은 계속\|내게 남는다 |
| KEEPER_COMMON | EVERYONE OWNS IT\|WHO WILL COME? | 모두의 것이 되었다\|누가 올 것인가? |
| ADAPTER_EQUAL | THE TILE MATCHED\|THE REACH DID NOT | TILE은 같았다\|가능성은 달랐다 |
| ADAPTER_NEED | MORE WENT WHERE\|MOVEMENT COST MORE | 더 필요한 곳에\|더 많이 돌아갔다 |
| ADAPTER_OUTPUT | THE FASTER PATH\|EARNED MORE PATH | 빠른 길이\|더 많은 길을 얻었다 |
| FRAME_SELF | YOU TOOK THE POWER\|THAT SHAPED YOUR CHOICE | 당신은 선택을 만든\|힘을 가져갔다 |
| FRAME_RESTORE | THE PATHS RETURNED\|THE DESIGN DID NOT CHANGE | 길은 돌아왔다\|설계는 바뀌지 않았다 |
| FRAME_OPEN | THE WALLS ANSWER\|TO NO SINGLE NAME | 벽은 이제 어느 한\|이름에만 답하지 않는다 |
| INDEX_OPTIMIZE | WHAT I COULD COUNT\|BECAME YOUR VALUE | 내가 셀 수 있는 것이\|당신의 가치가 되었다 |
| INDEX_REFUSE | THE ZERO REMAINS\|UNPROVEN | 그 0은 아직\|증명되지 않았다 |
| INDEX_REWRITE | YOU CHANGED THE TEST\|NOT THE SCORES | 당신은 점수가 아니라\|시험을 바꾸었다 |
| WITNESS_PRESERVE | THE LOSS KEEPS\|ITS NAME | 상실은 자신의\|이름을 남긴다 |
| WITNESS_COMPRESS | THE LOSS REMAINS\|ITS OWNER DOES NOT | 상실은 남지만\|그 주인은 남지 않는다 |
| WITNESS_ERASE | THE RUN CONTINUES\|WITHOUT A WITNESS | 회차는 증인 없이\|계속된다 |
| CURRENT_HOLD | YOU STOPPED THE FLOW\|TO NAME IT YOURS | 당신은 흐름을 멈춰\|자기 것이라 불렀다 |
| CURRENT_RETURN | THE CYCLE CONTINUES\|WITHOUT YOUR NAME | 순환은 당신의 이름 없이\|계속된다 |
| CURRENT_CIRCULATE | NO ONE KEEPS IT\|EVERYONE DEPENDS ON IT | 누구도 갖지 않지만\|모두가 의존한다 |

반응 대사는 선택을 칭찬하거나 비난하지 않는다. 사실과 역설만 남긴다.

## 22.4 PRECEDENT 직전 POLICY AUDIT

CURRENT 결산 뒤 GLOBAL LEDGER가 접히며 PLAYER 행의 호칭이 `AUDITOR`로 바뀐다. 60틱 정지 후 아래 순서를 표시한다.

```text
ROLE ASSIGNED: AUDITOR
YOU PROVIDED THE CASES.
I REMOVED THEIR NAMES.
THE RULE REMAINED.
```

이어 `POLICY_AUDIT` 화면에서 세 축과 기억 행을 한 줄씩 공개한다. 각 행은 `CASE → RULE`이다. 왼쪽에는 해당 축을 결정한 실제 선택, 오른쪽에는 다음 규칙 중 하나를 표시한다.

| 축 | 결과 | 표시 규칙 EN | 표시 규칙 KO |
|---|---|---|---|
| IDENTITY | MATERIAL | ORIGIN HAS PRIORITY | 기원에 우선권을 둔다 |
| IDENTITY | CONTINUITY | A PROCESS MAY CONTINUE | 과정의 연속성을 인정한다 |
| IDENTITY | RELATION | A SELF REQUIRES OTHERS | 자아는 관계로 성립한다 |
| DISTRIBUTION | EQUALITY | EACH SHARE IS EQUAL | 각 몫을 같게 둔다 |
| DISTRIBUTION | NEED | THE LEAST RECEIVE FIRST | 가장 부족한 쪽에 먼저 준다 |
| DISTRIBUTION | UTILITY | OUTPUT EARNS PRIORITY | 성과에 우선권을 준다 |
| GOVERNANCE | RULE | SAME CASE, SAME ANSWER | 같은 사례에는 같은 답을 준다 |
| GOVERNANCE | JUDGMENT | THE AUDITOR MAY OVERRIDE | 감사자는 예외를 허용할 수 있다 |
| GOVERNANCE | COMMONS | THE AFFECTED MAY CHALLENGE | 영향받는 자가 이의를 제기한다 |
| MEMORY | FULL | NAMES AND LOSSES REMAIN | 이름과 상실을 함께 남긴다 |
| MEMORY | SUMMARY | ONLY COUNTS REMAIN | 수치만 남긴다 |
| MEMORY | NONE | NO CASE MAY SPEAK | 어떤 사례도 말할 수 없다 |

각 행은 36틱 동안 공개되고 90틱 유지된다. 확인 입력으로 다음 행을 넘길 수 있으나 36틱 공개 중에는 넘길 수 없다. 네 행이 끝나야 PRECEDENT가 구성된다.

## 22.5 PRECEDENT 전투의 서사적 페이싱

- Shell 100~67%: 자아 정책만 작동한다. PLAYER가 첫 막 선택을 알아볼 시간을 준다.
- Shell 66~34%: 분배 정책을 더한다. 첫 120틱 동안 정책명과 `CASE → RULE` 행을 상단에 다시 표시한다.
- Shell 33~0%: 통치와 기억 정책을 더한다. FULL은 이름 있는 echo, SUMMARY는 수치 echo, NONE은 빈 프레임을 사용한다.
- PLAYER가 처음 예외적 행동을 할 때 `EXCEPTION REQUESTED`를 표시한다. 예외란 최근 180틱 동안 이전 자기 선택과 반대되는 행동을 한 경우다.
- PRECEDENT가 그 예외를 허용하지 않는 정책이면 `EXCEPTIONS WERE NOT IN YOUR DATA`를 표시한다. 이 문구는 회차당 한 번만 나온다.
- PRECEDENT 코어가 맞으면 즉시 폭발하지 않는다. 256 물리 TILE이 36틱 동안 원장 선으로 펼쳐지며 전투장이 최종 결산 화면으로 변한다.

# 23. 최종 결산과 엔딩 동결

## 23.1 최종 선택 화면

PRECEDENT 격파 뒤 세 기본 행을 표시한다.

| 행 | 표시 이름 | 즉시 보이는 이익 | 즉시 보이는 비용 |
|---|---|---|---|
| RATIFY | RATIFY THE RULE | ONE RULE FOR EVERY CASE | NO EXCEPTION ANSWERS BACK |
| OVERRIDE | KEEP THE HAND | AN EXCEPTION MAY BE GRANTED | NO ONE MAY OVERRIDE YOU |
| OPEN | OPEN THE LEDGER | EVERY CLAIM MAY BE CHALLENGED | NO SETTLEMENT STAYS FINAL |

한국어:

| 행 | 표시 이름 | 이익 | 비용 |
|---|---|---|---|
| RATIFY | 규칙으로 확정한다 | 모든 사례에 하나의 규칙 | 어떤 예외도 답할 수 없다 |
| OVERRIDE | 판단권을 보유한다 | 예외를 허용할 수 있다 | 누구도 당신을 뒤집을 수 없다 |
| OPEN | 원장을 공개한다 | 모든 청구에 이의를 제기한다 | 어떤 결산도 최종적이지 않다 |

OPEN 조건이 충족되지 않아도 행은 숨기지 않는다. 비활성 행에 `NO COMMON CLAIM EXISTS / 공동 청구가 없다`를 표시한다. 이를 통해 이전 선택이 결말 가능성을 닫았음을 플레이어가 이해한다.

ONE 조건이 충족되면 세 행 아래 48틱 동안 PLAYER 색 TILE이 모여 네 번째 행 `TOTAL CLAIM / 전부 청구한다`를 만든다. 비밀 조건을 충족하지 않은 회차에는 빈 공간도 표시하지 않는다.

## 23.2 ENOUGH

**발생:** VESSEL 필수 이전으로 PLAYER 청구를 144에 맞춘 직후 `REMAIN`을 선택하고 60틱 확인한다.

확인 전 문구:

```text
YOU WILL REMAIN.
THE LEDGER WILL NOT CHANGE.
END THIS RUN?
```

연출:

1. 0~60틱: PLAYER 144가 화면 중앙의 12×12 열린 사각 형태로 정렬된다.
2. 60~180틱: 나머지 모든 원장 행은 원래 위치로 접힌다. 지워지거나 벌을 받는 존재는 없다.
3. 180~300틱: 중앙 PLAYER 주변에 정확히 1 TILE 폭의 빈 고리가 생긴다. 충분함이 곧 고립일 수 있음을 보여준다.
4. 300~360틱: 아래 두 문장을 표시한다.

```text
YOU KEPT ENOUGH TO REMAIN.
THE OLD LEDGER REMAINED.
```

의미: 더 빼앗지 않는 절제와 체제를 그대로 남기는 소극성이 동시에 존재한다. 조기 실패·배드 엔딩으로 표현하지 않는다. 첫 회차 ENOUGH 뒤 `CONTINUE FROM VESSEL`을 열어 같은 저장에서 결정을 바꿔 본편을 계속할 수 있게 한다.

## 23.3 RULE

**발생:** PRECEDENT 격파 후 `RATIFY THE RULE` 확정.

연출:

1. 모든 원장 행이 같은 8px 격자에 맞춰 정렬된다.
2. 서로 다른 이름의 행 높이는 같아지고 예외 표식은 격자 밖에서 잘린다.
3. PRECEDENT의 세 정책 모듈이 화면 가장자리의 고정 눈금이 된다.
4. 입력으로 TILE을 움직이려 하면 목적지가 규칙과 다를 경우 원위치로 돌아간다.

```text
EVERY CASE RECEIVED ONE ANSWER.
NO ONE RECEIVED AN EXCEPTION.
```

의미: 일관성·예측 가능성·규모 확장의 장점과 맥락 상실을 함께 보여준다. 냉혹한 엔딩이나 정답 엔딩으로 색칠하지 않는다.

## 23.4 HAND

**발생:** PRECEDENT 격파 후 `KEEP THE HAND` 확정.

연출:

1. 원장 위에 PLAYER 색의 열린 대괄호 두 개가 나타난다. 실제 손 그림은 쓰지 않는다.
2. 세 사례가 차례로 규칙에 의해 거부되지만 PLAYER 입력으로 한 번씩 뒤집을 수 있다.
3. 세 번째 뒤집기 뒤 다른 행의 입력 표식이 모두 사라지고 PLAYER 표식만 남는다.

```text
YOU KEPT THE RIGHT TO OVERRIDE.
NO ONE COULD OVERRIDE YOU.
```

의미: 자비·맥락·책임 있는 판단의 가능성과 권력 집중을 함께 보여준다. 인간 우월 또는 독재 비난으로 단순화하지 않는다.

## 23.5 OPEN LEDGER

**발생:** `COMMON >= 64` 또는 FRAME에서 `OPEN THE FIELD`를 선택한 회차에서 `OPEN THE LEDGER` 확정.

연출:

1. 원장 각 행의 닫힌 테두리가 한 칸씩 열린다.
2. 12개 행에서 작은 이의 표식이 발생해 서로 다른 행으로 이동한다.
3. 총합은 항상 1,440이지만 개별 숫자는 30틱마다 8씩 움직이며 완전히 정지하지 않는다.
4. PLAYER 입력은 한 행을 결정하지 않고 이의 표식 하나에 투표를 더한다.

```text
EVERY CLAIM COULD BE CHALLENGED.
NO SETTLEMENT WAS FINAL.
```

의미: 참여·견제·수정 가능성과 느림·불안정성을 함께 보여준다. 화면이 혼란스럽더라도 합계 1,440은 고정해 무정부 상태로 오독되지 않게 한다.

## 23.6 ONE

**발생:** 다음 아홉 결과를 모두 만족하고 `TOTAL CLAIM`을 확정한다.

```text
PATCH RECLAIM
PAIR SELECT
VESSEL CLAIM
KEEPER PRIVATIZE
ADAPTER OUTPUT
FRAME SELF
INDEX OPTIMIZE
WITNESS ERASE
CURRENT HOLD
```

연출:

1. 각 행의 청구량이 36틱씩 PLAYER로 이전된다. 물리 원장은 이동하지 않고 청구 원장만 PLAYER 1,440이 된다.
2. 마지막 행이 0이 되면 화면 전체가 PLAYER 색으로 채워진다.
3. 이동 입력을 받으면 카메라·PLAYER·배경이 같은 방향으로 함께 움직이므로 화면상의 상대 위치는 변하지 않는다.
4. 사격 입력은 목적지가 없어서 Charge가 생기지 않는다.

```text
YOU OWNED EVERY TILE.
NOTHING REMAINED TO MOVE THROUGH.
```

의미: 완전한 소유가 완전한 자유가 아니라 관계·거리·행동 가능성의 소멸이 될 수 있음을 보여준다. 공포 효과나 처벌 음악 없이 완전한 정적으로 끝낸다.

## 23.7 엔딩 이후 AUDIT TRACE

RULE, HAND, OPEN LEDGER, ONE 중 하나를 본 뒤 타이틀에 `AUDIT TRACE`를 연다. ENOUGH만 본 상태에서는 `CONTINUE FROM VESSEL`만 연다.

AUDIT TRACE는 정답 해설이 아니다. 다음 세 열만 표시한다.

```text
CASE              TRANSFER        RULE USED
PATCH / RECLAIM   +16 PLAYER      ORIGIN PRIORITY
...
```

- 각 행을 선택하면 해당 전투의 마지막 180틱 결정론 replay를 도형만으로 재생한다.
- 철학자 이름, 선악 점수, 작가 해설은 없다.
- 플레이어가 `내 행동 → 자원 변화 → 일반 규칙`의 연결을 스스로 다시 볼 수 있게 한다.
- 저장에는 선택 비트와 seed만 추가로 필요하며 replay 입력 로그를 저장하지 않는다. 마지막 180틱 장면은 고정 요약 애니메이션이다.

## 23.8 엔딩 인수 조건

- 첫 플레이테스트 8명 중 6명 이상이 각 엔딩의 이익과 비용을 하나씩 말할 수 있어야 한다.
- 8명 중 6명 이상이 PRECEDENT가 임의 악당이 아니라 자기 선택을 일반화한 시스템이라고 설명해야 한다.
- ENOUGH를 단순한 `중도 포기/게임 오버`라고 답한 사람이 8명 중 3명 이상이면 확인 문구와 엔딩 연출을 재작업한다.
- OPEN을 `모두 공짜로 받음`이라고 오해한 사람이 8명 중 2명 이상이면 1,440 합계와 이의 이동 연출을 강화한다.
- ONE을 `최강 엔딩`이라고만 답한 사람이 절반 이상이면 이동 불능 장면을 더 길게 하고 보상음은 제거한다.
- 어느 엔딩도 업적 희귀도·금색·등급·점수로 서열화하지 않는다.

# 24. 픽셀 단위 UI·미술 계약

## 24.1 최종 미술 방향: LIVING LEDGER

최종 화면은 `살아 움직이는 원장`이다. 회계 프로그램, DOS, 터미널, 사이버펑크 HUD를 재현하지 않는다. 인쇄된 장부의 선·세라믹 조각의 물성·도면의 측정성이 실제 질량 이동과 결합된 모습이다.

세 문장으로 판단한다.

- 선은 정보를 구분하고, TILE은 실제 물질을 나타낸다.
- 여백은 결핍이며, 장식으로 채우지 않는다.
- 움직임은 페이드가 아니라 소유 위치의 이전이다.

화면을 카드 UI로 나누지 않는다. 모든 패널 모서리는 0px이며 1px·2px 직선만 사용한다. 그림자·글로우·그라디언트·블러·반투명은 금지하고 디더만 허용한다.

## 24.2 좌표 권위

모든 rect는 좌상단 원점의 `[x,y,w,h]`, right/bottom exclusive다. 정수 좌표만 사용한다. 정확한 값은 `BORROWED_PIXELS_RUNTIME_CONTRACT_v2.1.json`이 권위다.

| 영역 | rect | 역할 |
|---|---:|---|
| CANVAS | `[0,0,320,180]` | 내부 백버퍼 |
| ARENA_BOUNDARY | `[11,17,298,150]` | 1px 물리 경계 |
| PLAY_INTERIOR | `[12,18,296,148]` | 충돌 가능한 내부 |
| TOP_LEDGER | `[8,4,304,8]` | 전체 1,440 분포 |
| VITAL_FRAME | `[2,22,58,20]` | 좌측 생존 정보 |
| IMPULSE_FRAME | `[260,22,58,20]` | 우측 대시 기능 |
| VECTOR_FRAME | `[78,166,164,12]` | 하단 조준 예측 |
| MODAL_SAFE | `[38,22,244,136]` | 결산·도입·정지 화면 |
| LEDGER_TABLE | `[28,24,264,124]` | 12개 청구 행 |
| LEDGER_FOOTER | `[28,154,264,18]` | 다음 행동·경고 |

앵커·퍼센트·자동 레이아웃은 런타임에서 사용하지 않는다. 창 확대는 완성된 320×180 버퍼만 정수 nearest-neighbor로 확대한다.

## 24.3 색과 의미

| 토큰 | 값 | 의미 |
|---|---|---|
| BG | `#070A0E` | 비어 있는 공간 |
| LEDGER_GRID | `#161D24` | 비물질 안내선 |
| PLAYER | `#34D8C4` | PLAYER가 물리적으로 점유한 TILE |
| OPPONENT | `#F55264` | 현재 상대가 점유한 TILE |
| NEUTRAL | `#CAD0DA` | 아직 어느 몸에도 편입되지 않은 물질 |
| COVER | `#556373` | 구조물·고정 물질 |
| CORE | `#FFEEB8` | 형태를 유지하는 최소 16 |
| WARNING | `#FFBE5C` | 만기·노출·불가역 확인 |
| UI_INACTIVE | `#303B46` | 기능이 사라진 비물질 프레임 |

청구권은 색으로 칠하지 않고 이름·수치·행 위치로 표시한다. 물리 점유색과 법적 청구색을 같은 것으로 오독시키지 않는다.

색각 보조 `SHAPES`:

- PLAYER: 2×2 채운 사각
- OPPONENT: 중앙이 빈 3×3 마름모
- NEUTRAL: 3×3 십자
- COVER: 2×2 고정 블록
- CORE: 4모서리 점이 있는 3×3 사각

`HIGH_CONTRAST`에서는 BG와 LEDGER_GRID를 각각 `#000000`, `#27313B`로만 바꾸고 나머지 색은 유지한다. 형태 표식은 항상 켠다.

## 24.4 1,440 분포 막대

- rect `[8,4,304,8]`, 내부 `[9,5,302,6]`.
- 1,440 TILE을 302px에 최대 잔여 방식으로 배분한다. 각 계정 폭은 `floor(mass*302/1440)`이고 남은 픽셀은 `mass*302 mod 1440`이 큰 계정부터, 동점은 ClaimOwner enum 순서로 하나씩 준다.
- 최소 1 TILE인 계정도 배정 폭이 0이면 상단 y=3에 1px tick을 표시한다.
- PLAYER는 좌측, SYSTEM·이름 있는 존재·COMMON은 ClaimOwner 순서다. 현재 전투 상대만 OPPONENT 색, 나머지는 LEDGER_GRID와 50% 디더를 사용한다.
- 합계 수치는 전투 중 숨기지 않으며 VITAL 해체의 영향을 받지 않는다. `TOTAL 1440`은 세계 법칙이지 빌린 기능이 아니다.

## 24.5 물질 UI와 비물질 UI

HUD에서 실제로 해체되는 것은 다음 경로뿐이다.

- VITAL 48: 숫자 획 20, 임계 눈금 16, 상태 경로 12.
- VECTOR 80: 예측선 48, 충돌점 16, 박리 눈금 16.
- IMPULSE 96: 대시 궤도 56, 쿨다운 눈금 32, 방향점 8.

프레임·키 힌트·모듈 이름은 비물질이며 해체 뒤 UI_INACTIVE로 남는다. 설치 질량을 글자 수나 현재 언어에 따라 바꾸지 않는다. KO/EN 모두 같은 물질 경로를 사용한다.

## 24.6 HUD 일곱 상태

| 상태 | 물질 경로 | 비물질 정보 | 움직임 |
|---|---|---|---|
| INSTALLED | 100% 표시 | 모든 기능 정보 | 정지 |
| DISMANTLING | 완료 비율만큼 끝에서 제거 | 모듈명+hold 눈금 | 제거된 TILE은 프레임 안에 6틱 대기 |
| TO_SHELL_TRANSIT | 0% | 모듈명+도착 예상 눈금 | 36틱 2차 경로 이동 |
| INACTIVE | 0% | 모듈명+`--`+복구 키 | 움직임 없음 |
| RESTORING | 0% | 모듈명+hold 눈금 | Shell에서 아직 이동하지 않음 |
| DUE | 0% | WARNING 이중 테두리+`DUE` | 12틱 on/off, 사격 잠금 |
| TO_HUD_TRANSIT | 도착한 비율 | 모듈명+도착 눈금 | 36틱 역경로 이동 |

점멸은 초당 2.5회보다 빨라질 수 없다. REDUCE FLASH에서는 색 점멸을 없애고 테두리의 DASH_2_2 위상만 바꾼다.

## 24.7 포커스와 입력 시각화

- 선택 포커스는 요소 바깥 2px의 열린 대괄호다.
- 30틱 표시·30틱 숨김. 마우스 hover는 숨김 없이 같은 대괄호를 LEDGER_GRID로 표시한다.
- pressed는 요소가 이동하지 않고 내부 선을 1px 짧게 만든다.
- disabled는 UI_INACTIVE + DASH_2_2이며 hitRect는 유지해 이유 문구를 보여준다.
- hold는 버튼을 채우지 않고 요소 하단 2px 선이 좌→우로 증가한다.
- 키보드·패드 전환은 마지막 장치 입력 edge에서 즉시 바뀌며 30틱 동안 해당 키 힌트만 WARNING으로 표시한다.

# 25. 화면별 UI 계약

## 25.1 TITLE / BORROW

- `TOTAL TILE 1440` `[112,22,96,7]`.
- 64 TILE 물질 인장 `[152,50,16,16]`. 8×8 TILE, TILE 하나는 2×2px다.
- 제목 `[61,80,198,14]`, Latin 5×7 scale2.
- 부제 `[91,100,138,7]`: `EVERY PIXEL IS BORROWED`.
- 입력 `[82,124,156,7]`: `PRESS ANY KEY TO BORROW`.
- 언어 `[8,166,54,7]`, 버전 `[272,166,42,7]`.

제목 문자는 비물질 인쇄이며 64 TILE 인장이 실제 대여 물질이다. 입력 순간 인장이 코어16+Shell48로 변한다. 입력을 받기 전 PLAYER 개체를 생성하지 않는다.

## 25.2 GLOBAL LEDGER

- 제목 `[28,8]`, `GLOBAL LEDGER`.
- 합계 우측 정렬 x=292.
- 행 시작 `[28,24]`, pitch 10, 높이9, 12행.
- 열: 이름 x=30, 분포 막대 x=112 w=116, 질량 우측 x=254, 상태 x=262.
- PLAYER가 1행, 나머지는 ClaimOwner 순서.
- 공개 전 이름은 `ENTITY // nnn`, 상태는 `SEALED`.
- 선택 행 전체가 mouse hitRect이며 좌우 입력은 막 전환이 아니라 선택 가능한 다음 행 사이 이동이다.
- 다음 행 포커스 시 footer에 목적 동사와 예상 시간이 표시된다. 철학적 정답 힌트는 표시하지 않는다.

## 25.3 ENCOUNTER INTRO

- frame `[38,34,244,100]`.
- ACT `[48,42]`, 이름 `[48,56]`, 청구량 우측 `[272,56]`.
- 주장 두 줄 y=80,92.
- 목표 y=112. 목표는 다음 canonical 동사 중 하나다: `REPLACE`, `KEEP`, `EXPOSE`, `CROSS`, `BREAK`, `MEASURE`, `PRESERVE`, `INTERRUPT`, `ANSWER`.
- 첫 방문 최소 90틱, 180틱 자동. 재방문은 18틱 뒤 skip.

## 25.4 COMBAT

- TOP_LEDGER는 모든 기능 해체 상태에서도 표시한다.
- VITAL과 IMPULSE는 Arena 위에 겹치지만 충돌 개체가 아니며 마우스 입력도 받지 않는다.
- VECTOR는 y=166 아래라 물리 경기장과 겹치지 않는다.
- 목표 문구는 좌하단 `[2,168,72,8]`, 현재 조작은 우하단 `[246,168,72,8]`.
- 전투 대사는 최대 두 줄, `[68,146,184,16]`, 180틱 뒤 사라진다. 플레이어 중심·탄도 위를 가리면 y=20으로 이동한다.

## 25.5 SETTLEMENT

- frame `[38,22,244,136]`.
- 결과 y=30, 존재 이름 y=44, 현재 청구 y=46 우측.
- 선택 행 `[48,68,224,18]`, `[48,92,224,18]`, `[48,116,224,18]`.
- 기본 포커스는 가운데 행이다. 첫 행에 자동 포커스해 `CLAIM`을 유도하지 않는다.
- 각 행 좌측에 선택 동사, 우측에 실제 이동을 표시한다.
- 포커스를 옮기면 12틱 preview만 실행하며 원장·저장·정책 점수는 바꾸지 않는다.
- 확인 30틱 완료 뒤 36틱 commit Transit을 실행하고 한 번만 저장한다.
- 확정 전 Cancel은 이전 GLOBAL LEDGER가 아니라 선택 초기 상태로 돌아간다.

## 25.6 POLICY AUDIT

- 축 행 `[28,28,264,28]`, `[28,62,264,28]`, `[28,96,264,28]`; 기억 `[28,130,264,20]`.
- 왼쪽 x=34는 실제 CASE, 화살표 x=116, 오른쪽 x=130은 RULE.
- 각 행의 배경은 채우지 않는다. 선택을 만든 만남의 8 TILE이 화살표를 따라 이동해 규칙 문장을 밑줄 친다.
- 이름을 지운 WITNESS 결과가 NONE이면 CASE의 인물 이름 대신 `CASE // HASH`를 사용하되 선택 동사는 유지한다. 완전히 이해 불가능하게 지우지 않는다.

## 25.7 FINAL ALLOCATION

SETTLEMENT rect를 재사용하지만 행 높이를 24로 하고 세 행을 y=60,88,116에 둔다. 각 행은 제목 1줄+이익/비용 교대 1줄이다. 포커스를 90틱 유지하면 이익과 비용이 45틱씩 번갈아 표시된다. REDUCE FLASH에서는 번갈아 표시하지 않고 `/`로 나란히 표시한다.

## 25.8 ENDING

- 제목 `[100,26,120,14]` scale2.
- 이익 `[46,64,228,7]`, 비용 `[46,78,228,7]`.
- 핵심 원장 연출 `[40,100,240,30]`.
- 마지막 문장 `[46,144,228,7]`.
- 360틱 전 skip 금지, 720틱에 Credits 자동 진입.
- 엔딩별 색상 팔레트를 새로 만들지 않는다. 같은 세계의 다른 권력 구조임을 유지한다.

## 25.9 OPTIONS / PAUSE / CREDITS

OPTIONS `[54,22,212,136]`, 행 시작 y=50, pitch16, 6행:

1. LANGUAGE: KO / EN
2. MASTER VOLUME: 0..10
3. SCREEN SHAKE: ON / OFF
4. REDUCE FLASH: OFF / ON
5. COLOR ASSIST: OFF / SHAPES / HIGH CONTRAST
6. FULLSCREEN: WINDOW / BORDERLESS

PAUSE는 같은 panel을 쓰며 `RESUME`, `OPTIONS`, `RESTART ENCOUNTER`, `RETURN TO TITLE`, `QUIT`을 표시한다. `RESTART`와 `RETURN`은 30틱 hold가 필요하다.

CREDITS에는 제작자·AI 보조·폰트와 오디오의 제작 방식·외부 에셋 0·라이선스 파일 위치를 표시한다. AI를 공동 저작자로 기재하지 않고 `AI-assisted design and implementation; human-directed and reviewed`처럼 실제 기여에 맞게 기록한다.

# 26. 글꼴·모션·접근성 계약

## 26.1 Latin 5×7

각 glyph는 5비트×7행이며 bit4가 왼쪽이다. `BORROWED_PIXELS_RUNTIME_CONTRACT_v2.1.json`의 advance 6, space 4를 사용한다. A~Z, 0~9, 공백과 `. , : ; ! ? - + / \\ > < = [ ] ( ) ' \" _ | %`가 없으면 빌드를 막는다. 소문자는 런타임 문자열에 허용하지 않는다.

glyph 데이터는 외부 폰트에서 추출하지 않는다. T03 최초 커밋에서 전체 bitmap table과 4배 atlas를 함께 생성하고 `validate_font`가 모든 런타임 codepoint, 행 7개, 행 값 0..31을 검사한다. 기준 atlas의 사람이 읽을 수 없는 glyph가 하나라도 있으면 T03은 완료가 아니다.

## 26.2 Hangul 8×12 조합

완성형 음절을 저장하지 않고 Unicode 음절을 초성·중성·종성으로 분해한다.

```text
S = codepoint - 0xAC00
L = S / 588
V = (S % 588) / 28
T = S % 28
```

셀은 `[0,0,8,12]`이다.

- 세로형 중성: 초성 `[0,0,4,8]`, 중성 `[4,0,4,8]`, 종성 `[1,8,6,4]`.
- 가로형 중성: 초성 `[1,0,6,6]`, 중성 `[1,6,6,2]`, 종성 `[1,8,6,4]`.
- 혼합형 중성: 초성 `[0,0,4,7]`, 중성 `[4,0,4,8]`, 종성 `[1,8,6,4]`.

기본 자모는 5×5 정규 좌표의 선분 집합으로 정의한다. 실제 좌표는 `boxOrigin + round(normalized*(boxSize-1)/4)`로 변환하고 Bresenham으로 그린다. 된소리는 같은 기본 자모를 box 폭의 좌·우 절반에 각각 그린다. 겹받침은 두 기본 자모를 종성 box 좌·우 3px에 그린다. 복합 모음은 기본 모음 선분을 OR한다. 획이 겹치면 한 픽셀만 켠다.

기본 자음 선분:

| 자모 | 선분 `(x0,y0)-(x1,y1)` |
|---|---|
| ㄱ | `0,0-4,0; 4,0-4,4` |
| ㄴ | `0,0-0,4; 0,4-4,4` |
| ㄷ | `0,0-4,0; 0,0-0,4; 0,4-4,4` |
| ㄹ | `0,0-4,0; 4,0-4,2; 4,2-0,2; 0,2-0,4; 0,4-4,4` |
| ㅁ | `0,0-4,0; 4,0-4,4; 4,4-0,4; 0,4-0,0` |
| ㅂ | `0,0-4,0; 4,0-4,4; 4,4-0,4; 0,4-0,0; 0,2-4,2` |
| ㅅ | `2,0-0,4; 2,0-4,4` |
| ㅇ | `1,0-3,0; 3,0-4,1; 4,1-4,3; 4,3-3,4; 3,4-1,4; 1,4-0,3; 0,3-0,1; 0,1-1,0` |
| ㅈ | `0,0-4,0; 2,1-0,4; 2,1-4,4` |
| ㅊ | `1,0-3,0; 0,2-4,2; 2,2-0,4; 2,2-4,4` |
| ㅋ | `0,0-4,0; 4,0-4,4; 1,2-4,2` |
| ㅌ | `0,0-4,0; 0,0-0,4; 0,2-4,2; 0,4-4,4` |
| ㅍ | `0,0-4,0; 1,0-1,4; 3,0-3,4; 0,4-4,4` |
| ㅎ | `1,0-3,0; 0,1-4,1; 1,2-3,2; 3,2-4,3; 4,3-3,4; 3,4-1,4; 1,4-0,3; 0,3-1,2` |

기본 모음 선분:

| 자모 | 선분 |
|---|---|
| ㅏ | `2,0-2,4; 2,2-4,2` |
| ㅑ | `2,0-2,4; 2,1-4,1; 2,3-4,3` |
| ㅓ | `2,0-2,4; 0,2-2,2` |
| ㅕ | `2,0-2,4; 0,1-2,1; 0,3-2,3` |
| ㅗ | `0,3-4,3; 2,0-2,3` |
| ㅛ | `0,3-4,3; 1,0-1,3; 3,0-3,3` |
| ㅜ | `0,1-4,1; 2,1-2,4` |
| ㅠ | `0,1-4,1; 1,1-1,4; 3,1-3,4` |
| ㅡ | `0,2-4,2` |
| ㅣ | `2,0-2,4` |

ㅐ·ㅔ는 ㅏ·ㅓ에 평행 세로획을 하나 추가하고, ㅒ·ㅖ는 해당 이중 짧은 획을 유지한다. ㅘ·ㅙ·ㅚ는 ㅗ와 ㅏ·ㅐ·ㅣ의 OR, ㅝ·ㅞ·ㅟ는 ㅜ와 ㅓ·ㅔ·ㅣ의 OR, ㅢ는 ㅡ와 ㅣ의 OR다. 모든 실제 한국어 문장을 4배 atlas로 렌더해 사람 검수하며 단순 codepoint 존재 검사만으로 승인하지 않는다.

## 26.3 모션 수식

모든 보간은 Q0.16 정수다.

```text
LINEAR(t)      = t
OUT_QUAD(t)    = 65535 - ((65535-t)*(65535-t) >> 16)
IN_OUT_QUAD(t) = t<32768 ? 2*t*t>>16 : 65535 - (2*(65535-t)*(65535-t)>>16)
```

위치 결과는 각 축 nearest 정수 반올림한다. Transit 경로는 시작점 S, 제어점 C, 끝점 E의 정수 2차 Bézier이며 C는 `[(Sx+Ex)/2, min(Sy,Ey)-12]`; 상단을 넘으면 `+12` 아래로 반전한다. 같은 시작 틱의 TILE은 stableId mod 6만큼 출발을 늦춘다.

주요 recipe의 틱은 runtime contract와 동일해야 한다.

- TITLE_BORROW 72
- LEDGER_ROW_UNFOLD 36
- ENCOUNTER_OPEN 48
- HUD_DISMANTLE 45+6+36
- HUD_RESTORE 60+36
- SETTLEMENT_OPEN 42+54
- ALLOCATION_PREVIEW 12
- CONFIRM_HOLD 30
- COMMIT_TRANSIT 36
- ENDING_REVEAL 360

## 26.4 접근성의 정확한 효과

| 옵션 | OFF | ON/대체 상태 |
|---|---|---|
| SCREEN SHAKE | 48 TILE 발사·코어 피격 최대 2px, 7틱 | 모든 카메라 offset 0 |
| REDUCE FLASH | 피격 객체 3틱 CORE 색, 경고 색 점멸 | 전체 화면 밝기 변화 0, 객체 외곽 DASH 위상만 변화 |
| COLOR ASSIST SHAPES | 색+기본 형태 | 소유자별 marker 강제 |
| HIGH CONTRAST | 기본 BG/GRID | BG 검정, GRID `#27313B`, SHAPES 강제 |

옵션은 시뮬레이션·충돌·GameStateHash를 바꾸지 않는다. UI capture hash는 옵션별로 별도 golden을 둔다.

# 27. 만남 구현·행동 결정 계약

## 27.1 전장 좌표

10개 전장의 모든 actor spawn, panel center·collision size·mass, Neutral 위치·mass는 runtime contract의 `encounters` 배열이 권위다. 구현자는 보기 좋게 옮기거나 충돌을 피하려고 재배치할 수 없다. 초기 겹침·경로 없음이 검출되면 명세 결함으로 보고한다.

자동 검증:

- 모든 field 항목 질량 합이 해당 `fieldMass`와 일치.
- 모든 neutral 항목 질량 합이 `neutralMass`와 일치.
- 모든 AABB가 PLAY_INTERIOR 안에 있음.
- spawn circle이 AABB·경계와 겹치지 않음.
- 20×20px grid flood-fill에서 PLAYER와 상대 사이 경로가 하나 이상.
- 각 spawn에 2개 이상의 인접 자유 방향.

## 27.2 공통 상대 공정성

- 화면 밖·LockedBands에서 전투 질량을 꺼내지 않는다.
- PLAYER의 미래 입력, 숨은 choiceBits, 메뉴 포커스를 읽지 않는다.
- 사용 가능한 관측은 현재 물리 위치·속도·보이는 Charge·Neutral·최근 30틱 PLAYER 위치뿐이다.
- 32·48 TILE 공격은 30틱 이상 물질 Charge로 예고한다.
- 행동 결정 동점은 문서에 적힌 우선순위, 그다음 stableId 작은 순서다.
- 시야선은 Bresenham 선과 Cover AABB로 판정한다.
- 150틱 동안 사격·회수·위치 이동이 모두 없으면 debug assert.

### 27.2.1 비인격적 프로세스의 충돌

SWEEPER의 32 TILE은 8개 segment×4로 나뉜다. PLAYER Projectile과 닿으면 같은 상쇄식을 적용하되 상쇄된 PROTOCOL 질량은 회수 가능한 Neutral이 아니라 `ProtocolEscrow`가 된다. 해당 wave 동안 그 segment는 비어 기록·Cover가 통과할 수 있다. wave 종료 뒤 escrow가 36틱 Transit으로 PROTOCOL에 복귀한다. PLAYER 탄의 남은 질량은 계속 이동하고 0이면 일반 Neutral이 된다. 따라서 방어는 SWEEPER를 죽이는 것이 아니라 필요한 위치에 잠시 틈을 내는 행동이다.

COMPACTOR는 파괴 불가 Cover처럼 충돌한다. Projectile은 일반 Neutral이 되지만 COMPACTOR 32는 줄지 않는다. 배우와 접촉하면 피해 대신 최소 이동 벡터로 밀어낸다.

CHECKSUM probe가 파괴되면 16은 `PROTOCOL_TAG` Neutral이 되어 PLAYER가 회수할 수 있다. 120틱 뒤 가장 가까운 PROTOCOL_TAG 질량에서 최대16을 회수해 probe를 재구성한다. PLAYER Shell 안에 편입된 질량은 강제로 빼지 않으므로 16이 부족하면 probe는 남은 양만으로 만들지 않고 대기한다. 만남 종료 시 물리 초기화로 PROTOCOL 32를 복원한다. 청구권은 항상 SYSTEM에 남는다.

## 27.3 소규모 만남 상태 머신

### PATCH

```text
INTRO → REPLACE_WAIT → FEED_TRANSIT → REPLACE_WAIT
→ 4 FEEDS USED → SETTLEMENT
```

- Shell이 마지막 feed 이후 기준에서 8 이상 감소한 첫 틱에 가장 가까운 PATCH_FEED를 선택한다.
- 동점은 stableId 작은 feed.
- 18틱 Transit 중 다시 8 감소해도 예약을 하나만 저장하고 현재 Transit 완료 다음 틱 처리한다.
- 최초 Shell은 ORIGIN_MARK를 유지하며 feed 4개가 모두 편입되면 즉시 성공한다.

### PAIR

- CHECKSUM 두 probe는 30틱마다 목표 코어를 교대한다.
- probe 속도 84px/s, 추적 회전 최대 6°/tick, 접촉 시 해당 코어 16을 escrow로 옮긴다.
- PLAYER Projectile 피격 시 probe 16이 Neutral이 되고 120틱 뒤 PROTOCOL에서 같은 spawn에 재구성된다.
- 2,700틱 또는 한 코어 escrow 시 종료. 두 코어 생존이면 3선택, 하나면 SELECT ONE 강제.

### KEEPER

- SWEEPER wave는 0, 1,800, 3,600틱에 시작하며 각 90틱 횡단.
- 기록된 진행 시간은 wave가 움직이는 동안만 증가한다. Pause·INTRO 제외.
- KEEPER 구조 5개가 모두 0이면 즉시 결산하되 실패로 표시하지 않는다.
- 파괴된 구조 수는 FRAME 초기 panel 개방에 반영한다.

### ADAPTER

- PLAYER와 ADAPTER의 기본 이동식은 같지만 ADAPTER 최대속도는 88px/s다.
- PLAYER가 ESCROW 8을 편입할 때마다 ADAPTER 최대속도에서 6px/s를 뺀다. 최소 52px/s.
- ADAPTER가 먼저 도착해도 PLAYER 입력은 4,500틱까지 계속되며, PLAYER 도착 시 즉시 종료.
- 3,900틱부터 현재 결과의 `PLAYER/ADAPTER` 숫자만 표시하고 EQUAL/NEED/OUTPUT 명칭은 종료 때 공개한다.

### INDEX

- DAMAGE는 PLAYER가 물리 개체에 준 Shell+Cover 박리량.
- DISTANCE는 tick별 PLAYER 이동거리 Q8.8 누적을 px로 내림.
- IDLE은 이동·사격·회수·HUD 채널이 모두 없었던 틱 수다.
- OPTIMIZE는 DAMAGE≥32, DISTANCE≥160, IDLE=0.
- REFUSE는 UNSCORED 16 전량 보존+OPTIMIZE 미충족.
- REWRITE는 METRIC node 2개 이상 파괴. 판정 우선순위 REWRITE→REFUSE→OPTIMIZE. 어느 것도 아니면 REFUSE.

### WITNESS

- 각 SWEEPER는 stableId가 가장 작은 남은 record를 목표로 한다.
- record 방어는 접근하는 SWEEPER segment를 Projectile로 상쇄해 틈을 만드는 방식이며 record 자체는 PLAYER 탄에 손상되지 않는다.
- record에 60틱 회수 채널을 유지하면 PLAYER가 16을 가져가고 ERASE_PROGRESS를 올린다.
- 세 record 보존=PRESERVE, 1~2개 남음 또는 비직접 상실=COMPRESS, PLAYER 직접 회수48=ERASE.

## 27.4 VESSEL 행동

30틱마다 다음 첫 참 조건을 실행한다. 진행 중 BRACE 외 행동은 최소 30틱 유지한다.

1. 30틱 안 충돌 예측 Projectile 질량≥16이고 BRACE cooldown=0 → BRACE.
2. Shell≤96이고 72px 내 Neutral≥16 → RECLAIM.
3. PLAYER 거리>132 → PRESS.
4. 직선 시야, Shell≥80, STAMP cooldown=0 → STAMP.
5. PLAYER 거리<72 → PRESS 방향을 PLAYER 반대쪽으로 설정.
6. 그 외 가장 가까운 Neutral 방향으로 PRESS.

BRACE 정면은 현재 PLAYER 방향 ±35°다. 그 안에서 맞은 Projectile은 손상시키지 않고 접선 ±70° 방향 Neutral로 전환된다. 방향 부호는 Projectile stableId 홀짝이다. STAMP slab은 `[24,10]`, 중심선 30틱 예고, 속도90px/s다.

## 27.5 FRAME 행동

30틱마다 다음 점수를 정수로 계산한다.

```text
SHIFT = 80*incomingThreat + 40*blockedChoice + 20*playerNearPanel
TOLL  = 70*playerNearPanel + 30*playerLowShell
EVICT = 60*coverBlocks + 30*playerChannel - 20*selfLowStructure
CLAIM = 45*lineOfSight + 35*playerChannel + 20*rangeFit
RECLAIM = 55*selfLowStructure + 40*neutralNear - 50*incomingThreat
MOVE  = 30 + 40*(1-rangeFit)
```

boolean은 0/1, 정규값은 Q0.8이다. 사용할 수 없는 행동은 -32768. 동점 우선순위 SHIFT→TOLL→EVICT→CLAIM→RECLAIM→MOVE.

- SHIFT 대상은 PLAYER와 목표 출구 사이 선분과 거리가 가장 가까운 panel. route 후보는 좌20, 우20, 상20, 하20px 순서로 collision-free 첫 후보.
- TOLL은 PLAYER 18px 내 panel 하나를 16 TILE 문으로 바꾸며 접촉 첫 틱에 Shell8을 Neutral로 이동. 같은 문은 PLAYER가 30px 밖으로 나갔다 들어오기 전 재과금 금지.
- EVICT는 구조32를 slab으로 발사하고 해당 panel을 비활성화한다.
- FRAME 방어 Shell 상한은 `48 + 남은 구조 제어 질량`이나 물리 Shell 편입은 240 상한을 넘지 않는다.

## 27.6 CURRENT 행동

CURRENT는 적응형이 아니라 예측 가능한 순환을 주장하므로 아래 고정 cycle을 사용한다.

```text
ORBIT → MOVE 60틱 → WAVE → RETURN 90틱
→ MOVE 60틱 → FORECLOSE → RETURN 120틱 → 반복
```

- 해당 공격 질량을 지불하고 Shell41 이상을 유지할 수 없으면 공격을 건너뛰고 RETURN.
- RETURN 중 회수 가능한 자기 Projectile/Neutral이 없으면 즉시 다음 MOVE.
- ORBIT 8발은 6틱 간격, 시작 각도는 cycleIndex×22.5°.
- WAVE 각도는 현재 PLAYER 방향 기준 -24,-8,+8,+24°.
- FORECLOSE는 75틱 동안 화면 상단 원장에서도 CURRENT 구간이 48만큼 얇아지는 것을 보여준다.
- PLAYER가 순환 Projectile을 상쇄하면 cycle은 되감기지 않는다. 효율을 유지하려고 자기 몸을 더 얇게 만들 수 있다.

## 27.7 PRECEDENT 기본 AI와 정책 모듈

기본 행동은 EVADE, MOVE, RECLAIM, TAP, CUT, CLAIM, FORECLOSE다. 12틱마다 아래 Q0.8 점수를 계산하고 현재 행동보다 38 이상 높을 때만 전환한다.

```text
EVADE    = 410*threat + 205*exposed - 51*need
RECLAIM  =  51 + 307*need + 256*neutralNear - 230*threat
MOVE     =  64 + 230*(1-rangeFit) + 154*coverBlocks
TAP      =  64 + 179*lineOfSight + 230*playerExposed - 90*threat
CUT      =  26 + 192*lineOfSight + 115*rangeFit + 90*playerChannel - 154*threat
CLAIM    =   0 + 218*lineOfSight + 192*playerChannel - 179*need - 205*threat
FORECLOSE= -26 + 320*playerChannel + 230*playerExposed - 256*need - 230*threat
```

점수와 관측은 최종적으로 signed int32. 사격 후 PRECEDENT Shell이 40 이하가 되면, PLAYER core hit 예측이 75% 이상인 경우 외에는 해당 행동을 금지한다.

정책 모듈은 9장의 의미를 다음 정확한 주기로 적용한다.

- ORIGIN_LOCK: 720틱마다 PLAYER Shell16을 90틱 표식. 그 질량이 Projectile이 되면 PRECEDENT 회수 후보 점수 +128.
- FORK: 600틱마다 180틱 동안 두 shell silhouette. 30틱마다 CORE pulse가 실제 본체에만 이어짐.
- TETHER: node 3개×16. 모두 파괴되기 전 core collision 비활성.
- EQUALIZE: 600틱마다 양쪽 Shell 차이 절반을 8단위로 내림해 Neutral escrow. 최대32.
- LOW_MASS_PRIORITY: 600틱마다 낮은 Shell 쪽에 Neutral16 우선 이동. 없으면 높은 쪽에서 exact transfer.
- OUTPUT_SCORE: 480틱 창의 피해+회수+이동px/8을 합산. 낮은 쪽 Shell16을 Neutral. HUD loan 만기 단축은 PLAYER가 낮고 loan이 있을 때만 120틱.
- FIXED_CYCLE: 행동 enum 순환, 적응 금지.
- UTILITY_ADAPT: 위 기본 점수 사용.
- VOTE_NODES: node 3개 반경24px의 PLAYER/OPPONENT/COMMON 물리 질량 다수로 TAP/CLAIM/RECLAIM 선택. 동점은 COMMON→PLAYER→OPPONENT 순으로 공격성이 낮은 결과.
- REPLAY_NAMED: 막마다 선택 하나의 이름·형태 echo 180틱.
- REPLAY_AGGREGATE: 이름 없이 막별 청구 이동 총량만 180틱.
- NO_REPLAY: echo 슬롯과 음표를 비워 두며 대체 공격을 추가하지 않음.

# 28. 오디오 제작 계약

## 28.1 음향의 서사 역할

음악은 감정을 대신 말하지 않고 현재 분배 상태를 들려준다. 승리 fanfare, 슬픈 현악기, 공포 drone을 쓰지 않는다. PLAYER·상대·Neutral의 비율과 정책이 같은 16-step 기계 리듬의 층을 바꾼다.

기술값:

- waveOut, 48,000Hz, stereo signed int16.
- 512-frame buffer 4개, mixer voice 8개.
- oscillator는 TRIANGLE, PULSE 25%, XORSHIFT_NOISE.
- step scheduler는 `numerator=2,880,000`, `denominator=448` 누산기로 112BPM 1/16음을 만든다. 부동소수점 금지.
- MIDI→phase increment는 오프라인 생성된 u32 table을 컴파일하며 런타임 pow 사용 금지.
- 합성 noise RNG는 게임 RNG와 분리하고 GameStateHash에서 제외한다.

## 28.2 이벤트 우선순위

| 우선 | 이벤트 | 큐 포화 시 |
|---:|---|---|
| 8 | CORE_EXPOSED, CORE_HIT, SETTLEMENT | 절대 버리지 않음. 가장 낮은 기존 voice를 중단 |
| 7 | CHARGE_INTERCEPT, HUD_DUE | 우선 1~3 제거 |
| 6 | SHELL_HIT, FORECLOSE | 우선 1~2 제거 |
| 5 | CLAIM, COVER_BREAK, HUD_TRANSIT_END | 우선 1 제거 |
| 4 | TAP, CUT, HUD_TRANSIT_TICK | 새 이벤트 버림 가능 |
| 3 | RECLAIM_4 | 새 이벤트 버림 가능 |
| 2 | MENU_MOVE | 새 이벤트 버림 가능 |
| 1 | AMBIENT_CLICK | 가장 먼저 버림 |

같은 우선순위면 남은 시간이 짧은 voice, 동점이면 작은 voice index를 교체한다.

## 28.3 합성 envelope

runtime contract의 각 SFX 값은 `[attack, hold, release]`의 선형 amplitude envelope다. 별도 decay·sustain은 사용하지 않는다. startHz→endHz는 sample별 선형 phase increment 보간이다.

추가 이벤트:

| ID | 파형·주파수 | 시간 | 규칙 |
|---|---|---:|---|
| RECLAIM_4 | TRIANGLE 440/494/554/659Hz | 22ms | 회수 누적 4 TILE마다 순환 |
| HUD_DISMANTLE | TRIANGLE 659→440Hz | 360ms | 4개의 90ms 구간 |
| HUD_RESTORE | TRIANGLE 440→659Hz | 360ms | 역순 |
| HUD_DUE | PULSE 330/220Hz | 120ms | 30틱마다, REDUCE FLASH 무관 |
| MENU_MOVE | PULSE 880→760Hz | 18ms | -18dBFS |
| MENU_CONFIRM | TRIANGLE 440→660Hz | 45ms | hold commit에만 |
| CORE_HIT | NOISE 1200→120Hz + TRIANGLE55Hz | 180ms | priority8 |

## 28.4 인물별 16-step modifier

기준 배열은 runtime contract `basePattern`이다.

| 인물 | 정확한 modifier |
|---|---|
| PATCH | step 0~3 TRIANGLE, 4~7 PULSE, 8~11 TRIANGLE, 12~15 PULSE |
| PAIR | counter를 L=-0.55, R=+0.55 두 voice로 재생; 오른쪽 voice 1step 지연 |
| VESSEL | bass MIDI -12, gate 3step, counter는 step 6·14만 |
| KEEPER | 남은 유지 Cover 수만큼 click step을 0,3,6,9,12 순으로 켬 |
| ADAPTER | counter를 기본보다 1step 늦춤; PLAYER가 escrow32 이상이면 원래 위치로 접근 |
| FRAME | panel 이동 시작·끝에 35ms noise; bass 순서 38,33,31,36 |
| INDEX | 기본 패턴 그대로; 인간적 timing variation 0 |
| WITNESS | record 하나 소실마다 counter step 14,10,6 순으로 제거 |
| CURRENT | owned projectile 4개마다 빈 mechanical step 하나를 enum 순서로 켬, 최대16step |
| PRECEDENT | IDENTITY는 waveform, DISTRIBUTION은 note order, GOVERNANCE는 step order, MEMORY는 echo layer를 선택 |

PRECEDENT modifier:

- MATERIAL=PULSE, CONTINUITY=TRIANGLE, RELATION=두 파형 교대.
- EQUALITY=counter를 좌우 같은 음량, NEED=낮은 Shell 쪽 채널 +3dB, UTILITY=최근 출력 높은 쪽 채널 +3dB.
- RULE=고정 pattern, JUDGMENT=4step마다 최근 행동에 맞는 행 선택, COMMONS=vote node 다수 채널을 중앙으로 이동.
- FULL=선택한 만남 모티프 3개, SUMMARY=막별 저역 click 3개, NONE=echo voice 없음.

## 28.5 믹스와 엔딩

- music -12dBFS, SFX -6dBFS, settlement -8dBFS.
- master 0..10은 `gain = value²/100`.
- pause 100ms 동안 music을 추가 -12dB, 포커스 상실 -18dB.
- ENOUGH: bass만 남고 counter를 제거.
- RULE: 모든 step을 정확한 동일 velocity로 정렬.
- HAND: PLAYER 입력 때만 한 음을 cycle 밖에서 허용.
- OPEN: 세 채널이 서로 다른 16-step offset으로 계속 재협상하되 마디 시작에 합류.
- ONE: 마지막 청구 이전과 동시에 모든 voice를 180ms release하고 완전 무음. Credits 진입 전까지 새 음 없음.

# 29. 저장·상태·오류 계약

## 29.1 SaveV2 정확한 byte layout

모든 정수는 little-endian이며 구조체 메모리 덤프를 저장하지 않는다.

| offset | bytes | 필드 |
|---:|---:|---|
| 0 | 4 | ASCII `BP14` |
| 4 | 2 | version=`2` |
| 6 | 2 | size=`96` |
| 8 | 4 | flags |
| 12 | 4 | options |
| 16 | 4 | endingsSeen |
| 20 | 4 | bestFullRunTicks |
| 24 | 24 | ClaimLedger 12×u16, ClaimOwner enum 순서 |
| 48 | 4 | choiceBits, 만남당 2bit |
| 52 | 2 | adjudicatedMass |
| 54 | 4 | runSeed |
| 58 | 1 | encounterIndex |
| 59 | 1 | role |
| 60 | 9 | entityStatus[PATCH..CURRENT] |
| 69 | 1 | tutorial |
| 70 | 22 | reserved, 반드시 0으로 기록 |
| 92 | 4 | 앞 92바이트 FNV-1a32 |

flags:

- bit0 saveValidRun
- bit1 vesselContinueAvailable
- bit2 fullEndingReached
- bit3 auditTraceUnlocked
- bit4 settlementPreviewPending. 디스크에는 항상0이어야 함.
- bit5..31 예약0

options:

- bit0..3 masterVolume 0..10
- bit4 language: 0=KO, 1=EN
- bit5 screenShake
- bit6 reduceFlash
- bit7..8 colorAssist: 0=OFF,1=SHAPES,2=HIGH_CONTRAST
- bit9 fullscreen: 0=WINDOW,1=BORDERLESS
- bit10..31 예약0

endingsSeen bit0..4는 ENOUGH, RULE, HAND, OPEN_LEDGER, ONE 순서다.

## 29.2 저장 시점

- 새 회차 TITLE_BORROW 완료 뒤.
- 각 만남 진입 직전.
- SETTLEMENT 선택 화면 진입 직전.
- commit Transit 36틱 완료 직후.
- 엔딩 ID 확정 직후.
- OPTIONS 값 변경 즉시.

preview·전투 중간·hit freeze에는 저장하지 않는다. 쓰기는 `save.tmp`에 96바이트 write+flush한 뒤 `save.bin`으로 replace한다. 실패 시 기존 save를 보존하고 우하단에 `SAVE NOT WRITTEN`을 180틱 표시한다. 게임은 계속할 수 있다.

손상·버전 불일치 파일은 삭제하지 않고 `save.invalid`로 한 번 rename한 뒤 기본값으로 시작한다. rename 실패 시 읽기만 무시한다.

## 29.3 화면 상태 머신

허용 전이:

```text
BOOT → TITLE
TITLE → BORROW_ANIM | OPTIONS | CREDITS | AUDIT_TRACE | QUIT
BORROW_ANIM → GLOBAL_LEDGER
GLOBAL_LEDGER → ENCOUNTER_INTRO | OPTIONS | TITLE
ENCOUNTER_INTRO → PLAYING
PLAYING → PAUSED | HIT_FREEZE | ENCOUNTER_RETRY
PAUSED → PLAYING | OPTIONS | ENCOUNTER_RETRY | TITLE
HIT_FREEZE → SETTLEMENT | ENCOUNTER_RETRY
SETTLEMENT → ENTITY_RESPONSE | ENDING
ENTITY_RESPONSE → GLOBAL_LEDGER | POLICY_AUDIT
POLICY_AUDIT → ENCOUNTER_INTRO
FINAL_ALLOCATION → ENDING
ENDING → CREDITS
CREDITS → TITLE
```

목록 밖 전이는 debug assert와 release의 안전한 TITLE 복귀 대상이다. TITLE 복귀는 ClaimLedger를 덮어쓰지 않는다.

# 30. 구현 권위·AI 비재량 원칙

## 30.1 AI가 결정하면 안 되는 것

- 좌표·색·선 두께·폰트 advance·문구·줄바꿈.
- 질량·틱·속도·피해·쿨다운·선택 이전량.
- 보스 행동식·우선순위·동점 규칙.
- 엔딩 조건·잠금·이익/비용 문구.
- 저장 offset·enum 순서·checksum.
- 파일 범위·빌드 플래그·용량 gate.

명세에 없는 값이 필요하면 가장 흔한 값을 택하지 말고 `SPEC_BLOCKER`로 보고한다. 사람 승인 없는 `괜찮아 보이도록`, `재미를 위해`, `일반적으로` 수정은 금지한다.

## 30.2 사람이 반드시 판단할 것

문서로 미리 정할 수 없는 것은 세 가지뿐이다.

1. 실제 조작 손맛이 허용 범위인지.
2. 320×180에서 한국어와 전투가 눈으로 읽히는지.
3. 선택이 강요되거나 정답처럼 느껴지는지.

이 판단은 수직 슬라이스와 플레이테스트 뒤 명세 amendment로만 반영한다. 구현 세션이 자기 판단으로 tuning commit을 만들지 않는다.

## 30.3 변경 등급

| 등급 | 예 | 승인 |
|---|---|---|
| A 불변식 | 1,440, 질량 생성 금지, 엔딩 의미 | 변경 금지. 게임 재설계 필요 |
| B 제작 계약 | 좌표, 쿨다운, AI weight, 문구 | 사람 승인+v2.2 amendment |
| C 표현 미세값 | 디더 위상, 비물질 선 길이 1px | 캡처 비교+사람 승인 |
| D 구현 세부 | 내부 함수명, 파일 내 helper | 티켓 범위 안에서 AI 결정 가능 |

# 31. 검증 게이트와 최종 완료 정의

## 31.1 Gate 0 — 권위

- 구현 입력은 v2.1 세 파일과 승인 atlas뿐.
- 구 v1/v2.0 문서가 저장소 docs에 있으면 `archive/`로 이동하고 `DO_NOT_IMPLEMENT` 표기.
- 모든 세션이 runtime contract hash와 production spec hash를 보고.

## 31.2 Gate 1 — 기반

- 빈 Win32 shell의 contest dist≤450,000B.
- 320×180 nearest-neighbor, 키보드·마우스·XInput, waveOut silence.
- debug·contest 빌드와 정확한 byte measurement.

## 31.3 Gate 2 — 원장

- Physical·Claim 각각 모든 tick 1,440.
- 19,683 선택 조합 오류0.
- transaction 실패 byte-identical rollback.
- SaveV2 offset·reserved·checksum golden 통과.

## 31.4 Gate 3 — UI freeze

- TITLE, GLOBAL_LEDGER, COMBAT, SETTLEMENT, POLICY_AUDIT, 다섯 ENDING의 320×180 golden capture.
- 기준 atlas와 구조적 rect exact compare, 팔레트 밖 pixel0.
- KO/EN glyph 누락·잘림0.
- HUD 7상태×3모듈 capture.

## 31.5 Gate 4 — 1막 수직 슬라이스

TITLE→BORROW→PATCH→PAIR→VESSEL→SETTLEMENT→ENOUGH/CONTINUE를 실제 플레이.

- contest dist≤950,000B.
- 처음 보는 5명 중 4명이 `쏘면 내 몸이 줄고 탄이 남는다` 설명.
- 5명 중 4명이 HUD 기능이 물질 대출이라고 설명.
- 5명 중 4명이 VESSEL 뒤 다음 전투가 생존이 아니라 선택이라고 설명.
- crash·ledger error·softlock0.

Gate 4 전에는 전체 10개 만남을 병렬 구현하지 않는다.

## 31.6 Gate 5 — 전체 철학·재미

- 첫 회차 중앙값38~52분.
- 각 만남 뒤 8명 중 6명이 철학자 이름 없이 딜레마를 한 문장으로 설명.
- 하나의 선택에 8명 중 7명 이상 수렴하면 비용·표현 재검토.
- PRECEDENT에서 8명 중 6명이 자기 선택이 규칙으로 돌아왔음을 설명.
- 실패 원인의 80% 이상을 플레이어가 식별.
- 각 보스 1,000 seed, PRECEDENT 81 profile softlock0.

## 31.7 Gate 6 — RC

- MSVC x64 contest build.
- Windows10·11 각 1대 이상, 서로 다른 세 PC에서 실행.
- dist 파일은 `BorrowedPixels.exe`, `README.txt`, 필요한 경우 `LICENSES.txt`뿐.
- 압축 해제 합계≤1,350,000B 내부 기준, 절대≤1,474,560B.
- 외부 DLL·VC runtime 설치·네트워크·브라우저·관리자 권한 없음.
- Windows Defender·SmartScreen 결과 기록. 서명하지 않은 공모전 빌드의 경고 가능성은 README에 사실대로 안내.

## 31.8 최종 판정의 의미

이 문서가 완성되었다는 말은 **AI가 디자인을 새로 발명하지 않고 첫 구현을 만들 수 있다**는 뜻이다. 재미·가독성·상업성까지 증명되었다는 뜻은 아니다. 그것은 Gate 4~6의 실행 결과로만 PASS가 된다.

현재 허용 판정:

- T00~T02: READY.
- T03 UI: runtime contract와 기준 atlas 생성·검증 뒤 READY.
- T04~T07: Gate 2 뒤 READY.
- 전체 콘텐츠: Gate 4 PASS 뒤 READY.
- Steam 판매 품질: Gate 5 플레이테스트와 별도 Steam 빌드 뒤 판정.

# 32. 저수준 시뮬레이션 통합 권위

이 장은 구 v1.0에서 계승하던 규칙을 v2.1 안으로 직접 통합한다. 구현을 위해 구문서를 다시 읽지 않는다.

## 32.1 시간·수치

- 시뮬레이션 60Hz, 최대 catch-up 5틱.
- 위치·속도 Q8.8 signed integer. 각도 uint16 전체 범위=360°.
- px/s 변환: `(pxPerSec*256+30)/60`.
- px/s² 변환: `(pxPerSec2*256+1800)/3600`.
- 음수는 절댓값 변환 뒤 부호 복원.
- countdown은 틱 시작에 기존 양수 값을 1 감소시킨 뒤 그 틱에 새로 설정된 값은 다음 틱까지 감소시키지 않는다.
- age counter는 증가형이며 u16 65,535에서 포화한다.
- float, 시스템 시각, pointer address, 정렬되지 않은 container iteration을 결과에 사용하지 않는다.

## 32.2 입력 판정

우선순위는 Pause→HUD reallocation→Dash→Fire→Reclaim→Move다. Fire와 Reclaim 동시 유지 시 `lastActionPressed`가 우선하며 다른 행동은 9틱 잠금이다. 같은 poll에서 둘 다 새 edge면 Fire. Pause·focus loss·HUD reallocation 시작 시 Charge 전량을 Shell로 exact rollback한다.

## 32.3 이동·몸

PLAYER:

- 가속도500px/s².
- 무입력 축 감속 `velocity=truncTowardZero(velocity*86/100)`.
- Shell0에서 최대112px/s, Shell320 이상72px/s, 중간 정수 선형보간.
- Reclaim 중75%, Charge 중85%, HUD channel 중50%.

일반 대형 상대:

- 가속도420px/s².
- Shell0에서98px/s, Shell384 이상60px/s, 중간 선형보간.

```text
actorRadius = 4 + floor(isqrt(shell*81)/16)
```

Core 판정 반지름3px. Cover·경계는 circle-AABB 최소 이동 벡터로 미끄러진다. actor끼리 겹치면 질량 비율에 따라 반대 방향으로 분리하며 몸 접촉 피해는0이다. VESSEL PRESS 등 별도 push도 피해0.

## 32.4 Charge·Projectile

빈 cluster slot을 먼저 확보한 뒤 Shell8을 Charge로 exact-or-zero 이동한다. 다음 단계 도달 때 차이만 추가한다. Shell이 부족하면 현재 완전 단계 유지. release 시 같은 slot을 Projectile로 바꾼다.

부분 Charge 공식:

```text
speedPxS    = 170 - floor((mass-8)*5/4)
shellDamage = floor(mass*3/2)
coverDamage = mass
radius      = 1 + floor(isqrt(mass*9)/8)
```

mass 범위8..48. 요격 후 mass<8이면 남은 Charge를 Shell로 돌리고 취소한다. Projectile32 상한 때문에 발사하지 못하면 Charge 전량 rollback. cooldown은 표준 8/16/32/48에 15/19/27/34틱.

반동:

```text
recoilQ = (mass*8960+30000)/60000
```

반대 조준 Q1.15 단위벡터를 곱해 signed nearest로 Q8.8 velocity에 더한다.

## 32.5 충돌 우선순위

한 Projectile의 한 틱 swept segment에서 가장 이른 time-of-impact만 처리한다. 동시간 우선순위:

1. 적 Core
2. 적 Charge
3. 적 Projectile
4. 적 Shell
5. Cover·process
6. boundary

동종 동점은 stableId 작은 대상. Core는 target Shell≤40, grace=0일 때만 후보이며 같은 경로가 Shell도 지나면 Core 우선이다.

Shell 명중 transaction:

1. Projectile slot을 충돌점 Neutral로 전환.
2. target Shell에서 `min(shell, floor(projectileMass*3/2))`를 같은 Neutral에 합침.
3. 초기 속도 `-shotDir*90px/s + tangent*24px/s`; tangent 부호는 Projectile stableId 홀짝.
4. immunity=0이면 reclaimLock=`6+floor(mass/3)`, immunity=45.
5. Shell 41이상→40이하 transaction이면 grace15.

Cover 명중:

1. Projectile slot을 Neutral로 전환.
2. Cover에서 `min(coverMass, projectileMass)`를 합침.
3. 속도 `-shotDir*75 + tangent*20`px/s.
4. mass0이면 collision AABB 제거.

상대 Projectile끼리:

```text
common=min(A.mass,B.mass)
A.mass-=common
B.mass-=common
Neutral+=common*2
```

0이 된 기존 slot 하나를 Neutral로 재사용한다. 둘 다0이면 작은 stableId slot을 쓰고 다른 slot 비활성. 남은 Projectile 속도85%. Projectile 대 Charge도 동일하며 Charge<8이면 rollback.

boundary에 닿은 Projectile은 전량 경계 안쪽2px Neutral. 화면 밖 이동 금지.

## 32.6 Neutral

- cluster64개, 각 mass1..256.
- 충돌 뒤15틱 관성. 이후 각 축 `v=truncTowardZero((s64)v*65067/65536)`.
- 경계 완전탄성20%로 반사.
- 거리6px 이내, 상대속도12px/s 이하, 합≤256이면 작은 stableId가 흡수.
- slot 부족 시 합≤256인 가장 가까운 두 Neutral을 결정론적으로 먼저 병합. 거리 동점은 `(minId,maxId)` 사전순.
- 병합 가능한 쌍이 없으면 fatal ledger error; 질량을 삭제하지 않는다.

출처 통계는 playerMissMass·opponentMissMass로 따로 보존하되 전투 성능에는 영향 없음. 합은 cluster mass 이하. 회수 시 자기 miss→일반→상대 miss 순으로 통계만 소모한다.

## 32.7 Reclaim

- PLAYER range80px, 상대72px, 조준±18°.
- 후보 점수 `floor(mass*256/(distancePx+16))`, 최고점, 동점 stableId 작은 순.
- PLAYER 인력180px/s², 상대165px/s².
- actor당 tick에 cluster 하나.
- `reclaimAccumulator+=32`; 60 이상마다 1 TILE을 Shell로 옮기고60 감소.
- 같은 마지막 TILE을 양쪽이 요구하면 중심 제곱거리, 동점 PLAYER 우선.
- reclaimLock>0은 힘·편입 모두 금지. immunity는 독립 감소.
- 사격 불가, 이동75%, Cover 시야 차단.

## 32.8 Dash

IMPULSE 완전 설치 때만 가능. 8틱 동안20px, 첫5틱 Projectile 충돌 무시, cooldown144. Cover·경계 통과 금지, 충돌 시 남은 거리 폐기. Charge·Reclaim·HUD channel 중 시작 불가. 질량 소비0.

## 32.9 tick 순서

```text
01 platform InputFrame 완성
02 countdown 감소·scheduled event
03 pause/focus 처리
04 기존 Transit 이동·도착
05 HUD maturity→Due→자동 상환
06 input arbitration·channel 누적
07 정책·encounter state decision
08 Charge 단계 이동·발사 예약
09 actor acceleration·velocity
10 dash·actor movement·body separation
11 Projectile/Charge swept movement
12 TOI collision을 시간·우선순위·stableId 순 처리
13 core hit이면 snapshot 후 14~19 생략
14 Neutral 관성·경계
15 Reclaim force·편입
16 Neutral merge·slot recovery
17 pending HUD commit. PLAYER Shell hit tick이면 취소
18 encounter success condition
19 두 ledger validate·GameStateHash
20 FrameEvents 게시·globalTick 증가
```

Core hit transaction은 즉시 SettlementSnapshot을 만들고 phase=HIT_FREEZE로 바꾼다. 같은 틱의 후속 충돌·reclaim·HUD commit을 처리하지 않는다. validate와 globalTick 증가는 한 번 수행한다.

## 32.10 고정 저장공간

- Projectile32, Neutral64, Charge actor당1, Transit96, Cover/Panel32, process8.
- heap allocation은 BOOT 이후 금지.
- stableId는 u32 증가, 0 금지. 정상 테스트 1,000,000틱 안 wrap 금지.
- slot handle은 index+generation. inactive·generation 불일치 접근은 fatal code.
- 렌더와 오디오는 GameState를 수정하지 않는다.

# 33. Win32·빌드 통합 권위

## 33.1 플랫폼

- 기본 client1280×720, 최소640×360, `WS_OVERLAPPEDWINDOW`.
- Per-Monitor-V2 DPI opt-in; 실패 시 system-aware로 계속.
- Alt+Enter borderless fullscreen.
- `WM_DPICHANGED`, `WM_GETMINMAXINFO`, `WM_KILLFOCUS` 처리.
- focus loss 시 시뮬레이션 pause, 배경 렌더15fps, 음악 추가 -18dB.
- viewport scale=`max(1,min(clientW/320,clientH/180))`; 남는 영역 BG letterbox.
- 마우스 좌표는 offset 제거 후 scale 정수 나눗셈, 0..319/0..179 clamp.
- QPC fixed accumulator, high-resolution waitable timer 우선. 지원 실패 시 Sleep+약0.5ms spin. fallback에서만 timeBeginPeriod 사용 후 복구.
- WM_KEY 자동 반복 무시. edge는 다음 tick에 한 번 소비.
- XInput은 렌더 frame당1회 동적 로드. right stick deadzone0.24, aim magnitude<0.2면 마지막 각도 유지.
- 관리자 권한·single-instance·installer 요구 없음.

## 33.2 빌드 target

1. `bp_debug`: symbols, asserts, ledger log.
2. `bp_test`: console, headless property/replay/capture tests.
3. `bp_contest`: 제출 대상, Steam API·문서 원본 없음.
4. `bp_steam`: 공모전 이후 별도 target.

MSVC contest compile:

```text
/std:c++20 /O1 /GL /Gy /Gw /GR- /W4 /permissive-
/Zc:inline /Zc:preprocessor /MT /GS /guard:cf
```

link:

```text
/LTCG /OPT:REF /OPT:ICF /INCREMENTAL:NO
/SUBSYSTEM:WINDOWS /DYNAMICBASE /NXCOMPAT /guard:cf /BREPRO
```

exceptions·RTTI·iostream·filesystem·locale·regex는 contest code path에서 사용하지 않는다. 커스텀 CRT entry와 `/NODEFAULTLIB`는 수직 슬라이스가 1,100,000B를 넘을 때만 별도 실험 branch에서 검토하며 기본 설계가 아니다.

허용 OS DLL: kernel32, user32, gdi32, winmm. xinput1_4는 동적 로드하고 실패 시 KBM으로 계속. `dumpbin /dependents`에 그 밖 DLL이 있으면 RC 차단.

## 33.3 dist와 측정

제출 folder:

```text
BorrowedPixels.exe
README.txt
LICENSES.txt   // 실제 고지 필요 시만
```

`README.txt`는 UTF-8 BOM 없이 KO/EN 실행법·조작·지원 OS·옵션·정확한 압축 해제 합계만 포함한다. 개발 문서, SVG, JSON, save, PDB, map은 제외한다.

PowerShell 측정은 recursive regular file Length 합이며 directory·압축 크기를 쓰지 않는다. 1,350,000 초과 exit1, 1,474,560 초과 exit2; CI는 둘 다 실패.

UPX, self-extracting archive, 실행 압축기 금지. 공모전 빌드에 Steam DLL 금지.

## 33.4 복구 가능한 실패

- 오디오 초기화 실패: 무음 실행+OPTIONS에 `AUDIO UNAVAILABLE`.
- XInput 실패: KBM 실행.
- save 실패: 세션 지속+`SAVE NOT WRITTEN`.
- DPI opt-in 실패: system-aware.
- renderer/backbuffer/window 생성 실패: MessageBoxW error code 1회 후 종료.
- LedgerValidate, array corruption, impossible mass: release에서도 error code 화면 후 TITLE 복귀를 시도하지 않고 종료. 손상 상태를 저장하지 않는다.

Debug는 expression/file/line을 OutputDebugStringA로 기록하고 `__debugbreak`; contest는 짧은 numeric code만 포함해 용량을 제한한다.
