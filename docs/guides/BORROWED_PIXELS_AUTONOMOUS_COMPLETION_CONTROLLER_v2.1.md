# BORROWED PIXELS v2.1

## Codex 긴급 자율 완성 컨트롤러

- 저장 위치: `C:\Users\sdjsd\Desktop\BORROWED_PIXELS\docs\guides\BORROWED_PIXELS_AUTONOMOUS_COMPLETION_CONTROLLER_v2.1.md`
- 적용 기간: 지금부터 2026-09-04 공모전 제출 후보 완성까지
- 저장소 루트: `C:\Users\sdjsd\Desktop\BORROWED_PIXELS`
- 목적: T00부터 T14까지 조사·구현·실행·검증·수정을 닫힌 루프로 진행하고, 사람에게는 실제 플레이와 최종 제출 판단만 남긴다.
- 상태: 운영 지시서. 게임 규칙·수치·서사 권위가 아니다.

> 이 파일은 `BORROWED_PIXELS_AUTHORITY_v2.1.md`가 정한 게임 규칙을 변경하지 않는다. 단, HUMAN DIRECTOR가 이번 마감에 한해 검증된 로컬 티켓의 커밋·후보 브랜치 생성·fast-forward 통합을 AUTONOMOUS ORCHESTRATOR에게 위임한다. 외부 push·공개·출품·대규모 삭제·게임 방향 변경은 위임하지 않는다.

---

# 0. 가장 빠른 사용법

1. 이 파일을 위 저장 위치에 넣는다.
2. 저장소의 중복 가이드와 unborn branch를 아래 1장의 명령으로 정리한다.
3. `integration/rc-2026-09-03` 기준 커밋이 생기고 working tree가 clean인지 확인한다.
4. VS Code에서 `C:\Users\sdjsd\Desktop\BORROWED_PIXELS`를 연다.
5. 새 Codex 세션에서 13장의 `/goal` 명령을 붙인다. Goal이 이 파일 전체를 실행 계약으로 읽게 한다.
6. Codex는 실제 차단 요소 또는 `HUMAN_CHECKPOINT`가 나오기 전까지 다음 티켓을 자율 진행한다.

완료의 의미는 계획이나 초안이 아니라 다음 상태다.

```text
코드 작성
→ 실제 MSVC 빌드
→ 테스트·실행·용량 측정
→ 실패 원인 분석
→ 수정
→ 같은 검증 재실행
→ 독립 검토
→ 로컬 통합
→ 다음 티켓
```

---

# 1. 현재 T00 SPEC_BLOCKER 해제

현재 blocker의 원인은 다음 두 가지다.

- integration branch에 기준 commit이 없는 unborn 상태
- 정식 가이드와 `(2)` 경로 수정본이 동시에 존재해 dirty 상태

`(2)` 파일을 경로 수정이 반영된 정식본으로 사용한다. 기존 정식본은 저장소 밖에 백업한다. 이 컨트롤러 파일도 `docs/guides/`에 넣은 뒤 한 번에 기준 commit을 만든다.

Developer PowerShell x64에서 실행:

```powershell
Set-Location C:\Users\sdjsd\Desktop\BORROWED_PIXELS

New-Item -ItemType Directory -Force C:\Users\sdjsd\Desktop\BORROWED_PIXELS_BACKUP

Copy-Item `
  .\docs\guides\BORROWED_PIXELS_IMPLEMENTATION_STARTER_GUIDE_v2.1.md `
  C:\Users\sdjsd\Desktop\BORROWED_PIXELS_BACKUP\BORROWED_PIXELS_IMPLEMENTATION_STARTER_GUIDE_v2.1.before-desktop-path.md `
  -Force

Move-Item `
  '.\docs\guides\BORROWED_PIXELS_IMPLEMENTATION_STARTER_GUIDE_v2.1(2).md' `
  .\docs\guides\BORROWED_PIXELS_IMPLEMENTATION_STARTER_GUIDE_v2.1.md `
  -Force

git add docs
git status --short
git commit -m "docs: freeze v2.1 authority and autonomous operation"

git rev-parse HEAD
git status --short
```

통과 기준:

- `git rev-parse HEAD`가 40자리 SHA를 출력한다.
- 마지막 `git status --short`가 아무것도 출력하지 않는다.
- 현재 branch가 `integration/rc-2026-09-03`이다.
- `docs/guides/`에 `(2)` 파일이 없다.
- 정식 starter guide와 이 controller만 존재한다.

이후 T00 worktree를 만든다.

```powershell
git worktree add `
  C:\Users\sdjsd\Desktop\BORROWED_PIXELS_T00 `
  -b ticket/T00-bootstrap `
  integration/rc-2026-09-03
```

이미 같은 이름의 branch나 worktree가 존재하면 삭제하거나 덮어쓰지 않는다. `git worktree list`와 `git branch --list` 결과를 Codex가 조사해 재사용 여부를 결정하게 한다.

---

# 2. HUMAN DIRECTOR의 한시적 권한 위임

## 2.1 자동 실행을 허용하는 것

AUTONOMOUS ORCHESTRATOR는 다음을 사람의 매회 확인 없이 수행할 수 있다.

- 저장소·권위 문서·Git 상태 읽기
- 로컬 branch와 worktree 생성
- 한 번에 한 티켓의 allowlist 안에서 코드와 테스트 수정
- debug/test/contest 빌드
- 로컬 EXE 실행과 headless test 실행
- 캡처·로그·MAP·evidence 생성
- 자동 수정 가능한 빌드·테스트·명세 구현 오류 수정
- 구현자와 검토자 역할의 분리 수행
- P0/P1=0이고 필수 자동 검증이 PASS인 티켓의 candidate branch 생성
- 위 조건을 만족한 candidate의 로컬 `--ff-only` integration
- accepted SHA를 다음 티켓의 base로 고정
- `docs/STATUS.md`, handoff, evidence 갱신

## 2.2 자동 실행을 허용하지 않는 것

다음은 반드시 `HUMAN_CHECKPOINT`에서 멈춘다.

- 원격 저장소 push·force-push
- 공모전 폼 제출·파일 업로드·외부 공개
- 비용 결제·유료 API·새 계정·자격 증명 사용
- 저장소 밖 사용자 파일 삭제
- authority·runtime contract·font contract·production spec의 의미 변경
- 핵심 만남·원장·엔딩 삭제
- 저작권 출처가 불명확한 외부 코드·이미지·폰트·음원 도입
- Gate 3 미감, Gate 4 조작감·이해도, Gate 5 철학·재미를 사람 대신 최종 PASS 선언
- 공모전 하드 조건을 바꾸거나 무시하는 결정

## 2.3 기존 runbook과의 관계

이 위임은 runbook에서 **각 티켓의 로컬 merge마다 사람 승인을 기다리게 한 운영 절차만** 이번 마감까지 대체한다.

다음은 그대로 유지된다.

- 티켓별 allowlist
- authority hash 검증
- 구현과 검토의 역할 분리
- P0/P1=0 조건
- exact byte measurement
- SPEC_BLOCKER 규칙
- Gate 4 전 전체 콘텐츠 착수 금지
- 외부 push·제출은 사람만 수행

---

# 3. 자동 완성 작업 계약

```yaml
task_id: BP-CONTEST-RC-2026-09-04
outcome: >
  Windows x64에서 독립 실행되고, BORROWED PIXELS v2.1의 10개 만남,
  두 1,440 원장, PRECEDENT 81 profile, 다섯 엔딩을 갖춘 공모전 RC 후보를 만든다.
audience:
  - 1.44MB 공모전 심사자
  - 향후 포트폴리오와 Steam 확장의 개발자
deliverables:
  - dist/contest/BorrowedPixels.exe
  - dist/contest/README.txt
  - 필요한 경우에만 dist/contest/LICENSES.txt
  - T00~T14 accepted commits and evidence
  - final validation matrix and handoff
boundaries:
  - Windows x64 C++20 Win32 standalone
  - extracted dist hard cap 1474560 bytes
  - internal RC cap 1350000 bytes
  - PhysicalLedgerTotal == 1440
  - ClaimLedgerTotal == 1440
  - no engine, third-party runtime, external runtime asset, network, browser, runtime AI
  - preserve authority documents and user changes
acceptance:
  - authority hashes match
  - debug, test, contest build evidence
  - required automated tests pass
  - P0/P1 findings zero for accepted tickets
  - exact dist bytes recorded
  - Gate 4 human checkpoint resolved before T08
  - Gate 6 human-only items explicitly separated
approval_gates:
  - Gate 3 visual judgment
  - Gate 4 first-play understanding and control feel
  - Gate 5 external playtest interpretation
  - remote push and contest submission
non_goals:
  - Steam integration before contest RC
  - new story, encounter, UI system, or optional feature not in v2.1
  - redesigning authority to save implementation time
```

---

# 4. Orchestrator 상태 머신

상태는 다음 중 하나만 사용한다.

```text
BOOTSTRAP
→ IMPLEMENTING_TXX
→ VERIFYING_TXX
→ REVIEWING_TXX
→ REPAIRING_TXX
→ INTEGRATING_TXX
→ NEXT_TICKET
→ HUMAN_CHECKPOINT
→ RC_VERIFYING
→ RC_CANDIDATE
```

`FAIL`은 자동 수정 루프로 들어가며 곧바로 사용자에게 떠넘기지 않는다. 다음만 `BLOCKED`다.

- 권위 문서끼리 실제 충돌
- allowlist 밖 API 변경 없이는 진행 불가
- 필요한 Windows 도구·권한·자료가 없음
- 자동 수정 시 게임 방향 또는 authority 변경이 필요
- 같은 근본 원인을 세 접근으로 해결하지 못함
- 사람의 실제 플레이·취향·외부 행동만으로 판정 가능

각 상태 전환은 `docs/STATUS.md`와 해당 handoff에 기록한다. 상태 문서 작성 때문에 구현을 장시간 멈추지 않는다.

---

# 5. 티켓 연속 실행 순서

기본적으로 writer는 한 명이며 다음 순서를 직렬 실행한다.

```text
T00
→ T01
→ T02
→ API GATE
→ T03
→ T04
→ T05
→ T06
→ T07
→ HUMAN CHECKPOINT: GATE 3/4
→ T08A
→ T08B
→ T09
→ T10
→ T11
→ T12
→ T13
→ T14
→ RC CANDIDATE
```

T08A/T08B도 마감 안정성을 위해 직렬 처리한다. T08B는 T08A가 integration에 반영된 최신 accepted SHA에서 시작한다.

각 티켓의 목표, allowlist, acceptance는 `BORROWED_PIXELS_AGENT_RUNBOOK_v2.1.md`가 유일한 운영 원본이다. 컨트롤러는 그 내용을 새로 발명하거나 완화하지 않는다.

---

# 6. 한 티켓의 폐쇄형 자동 루프

각 TXX에서 다음을 모두 수행한다.

## 6.1 시작

1. 최신 integration SHA를 기록한다.
2. authority hash를 manifest와 비교한다.
3. working tree와 worktree 상태를 확인한다.
4. 해당 티켓 branch/worktree를 최신 accepted SHA에서 만든다.
5. allowlist와 acceptance를 그대로 추출한다.
6. baseline debug/test/contest와 dist 측정을 실행한다.

## 6.2 구현

1. acceptance마다 자동 test 또는 수동 evidence 항목을 연결한다.
2. allowlist 안에서 최소 변경으로 구현한다.
3. 기존 빌드·검증 도구를 재사용한다.
4. 새로운 규칙·문구·좌표·밸런스를 만들지 않는다.
5. 구현 도중에도 빌드 가능한 체크포인트를 유지한다.

## 6.3 검증

반드시 실제로 실행한다.

```powershell
cmake --preset windows-msvc
cmake --build --preset debug
cmake --build --preset test
ctest --preset all --output-on-failure
cmake --build --preset contest
powershell -ExecutionPolicy Bypass -File .\tools\measure_dist.ps1 -Path .\dist\contest -InternalCap 1350000 -HardCap 1474560
```

프로젝트가 아직 위 interface를 만들기 전이면 T00에서 만든 실제 명령을 사용하고 이후 고정한다. 실행하지 않은 명령은 PASS가 아니라 `NOT_RUN`이다.

## 6.4 독립 검토

가능하면 별도 subagent 또는 새 reviewer context가 다음을 읽기 전용으로 확인한다.

- SHA·authority hash·changed path
- acceptance와 실제 evidence 대응
- 두 원장 질량 생성·손실
- preview/commit/save 혼동
- deterministic simulation 위반
- overflow, slot saturation, corrupted save, failure path
- 외부 의존과 권리 문제
- contest bytes와 delta

검토자는 수정하지 않고 P0/P1/P2를 낸다.

## 6.5 자동 수리

- P0/P1 또는 test failure가 있으면 구현 역할로 돌아간다.
- 수정 뒤 관련 test만이 아니라 전체 회귀를 다시 실행한다.
- candidate SHA가 바뀌면 이전 reviewer 승인은 무효다.
- P0/P1=0이 될 때까지 반복한다.

## 6.6 로컬 자동 통합

다음이 모두 참일 때만 AUTONOMOUS ORCHESTRATOR가 통합한다.

- authority hash 일치
- changed paths allowlist 안
- 필수 자동 test PASS
- dist hard cap PASS
- P0/P1=0
- 해당 단계의 선행 Gate PASS
- commit과 evidence 존재

통합은 로컬에서만 수행한다.

```powershell
git branch candidate/TXX-final CANDIDATE_SHA
git switch integration/rc-2026-09-03
git merge --ff-only candidate/TXX-final
```

fast-forward가 실패하면 merge commit을 만들지 않는다. branch ancestry를 재조사하고 최신 integration 위로 티켓을 재적용·재검증한다.

통합 직후 full build/test/byte measurement를 다시 실행하고 다음 티켓으로 이동한다. 티켓을 통합했다는 이유만으로 사용자에게 멈추지 않는다.

---

# 7. 실패 전략

같은 증상에 같은 수정을 반복하지 않는다.

| 실패 단계 | 자동 행동 |
|---|---|
| 1회 | 첫 실제 오류와 최초 불일치 tick/symbol/rect를 찾아 국소 수정 |
| 2회 | 환경·입력·base SHA·authority 가정을 실제 값으로 재검증 |
| 3회 | 최소 재현 test를 만들고 다른 구현 경로로 교체 |
| 계속 실패 | 마지막 PASS, 재현 명령, 세 접근, 필요한 한 결정을 BLOCKED 보고 |

금지:

- test를 삭제하거나 기대값을 구현에 맞춰 낮추기
- 누락 질량을 임의 계정에 보정하기
- 크기를 줄이려고 안전 검사 삭제
- 컴파일 실패를 새 third-party library 도입으로 숨기기
- 경고·NOT_RUN을 PASS로 바꾸기
- 마감이 가깝다는 이유로 authority를 조용히 축소하기

---

# 8. 시간과 범위 우선순위

현재 목표는 Steam 확장판이 아니라 **검증 가능한 공모전 RC**다.

우선순위:

1. 실행·질량 불변식·결정론·저장·용량
2. TITLE부터 엔딩까지 전체 도달 가능성
3. 철학이 행동과 결산으로 전달되는 핵심 연출
4. KO/EN 가독성·옵션·합성 오디오
5. 비핵심 변주와 추가 polish

일정이 밀리면 production spec 18장에 허용된 비핵심 항목만 제거 후보로 제시한다. 실제 제거는 기존 명세가 이미 허용한 범위에서만 하고 handoff에 기록한다.

절대 자동 삭제하지 않는 것:

- 두 1,440 원장
- 몸을 탄으로 쓰는 전투와 빗나간 탄의 잔존·회수
- HUD loan
- 여섯 소규모 점유자의 질문과 선택
- 세 주요 보스와 PRECEDENT
- 필수 엔딩과 continue 의미
- 결정론·크기·권리·접근성 최소 검증

---

# 9. 사람에게 요구하는 최소 세 번의 확인

## CHECKPOINT A — 최초 기준 commit

1장의 명령으로 clean base SHA를 만든다. 이것은 자동화 시작 전 한 번만 한다.

## CHECKPOINT B — Gate 3/4 수직 슬라이스

Codex가 T07까지 통합하면 다음만 인계한다.

- 실행할 EXE 경로
- 10분 이내 조작 순서
- 5개 관찰 질문
- capture와 자동 test 결과
- PASS/FAIL 입력 양식

사람은 직접 플레이해 다음을 확인한다.

- PLAYER·상대·Neutral이 구분되는가
- 쏘면 몸이 줄고 탄이 세계에 남는다고 이해되는가
- HUD 해체가 실제 질량 대출로 느껴지는가
- VESSEL 뒤 계속 전투가 생존이 아닌 선택으로 읽히는가
- 조작이 진행을 막을 정도로 불편하지 않은가

사람이 `GATE4_PASS: YES` 또는 구체적 실패 관찰을 입력할 때까지 T08을 시작하지 않는다. 실패 관찰이 구현 오류면 Codex가 T07을 자동 수정·재검증한다. 명세 미감·밸런스 변경이면 amendment 승인만 요청한다.

## CHECKPOINT C — RC와 제출

Codex는 T14에서 다음을 준비한 뒤 멈춘다.

- contest folder와 exact bytes
- SHA-256
- dumpbin dependencies
- 자동 test 결과
- 알려진 human-only Gate 5 결과
- 3PC 실행 체크표
- 출품 폼용 제목·설명·플랫폼 정보 초안

사람만 세 PC의 실제 실행 결과를 확인하고 최종 파일을 공모전 폼에 업로드한다.

---

# 10. 자동 증거 우선순위

완료 주장의 우선순위:

1. 실제 명령과 종료 코드
2. 자동 test·validator
3. exact bytes·hash·원본 diff
4. capture·replay·실행 결과
5. acceptance trace matrix
6. 에이전트 설명

상위 증거의 FAIL을 하위 설명으로 덮을 수 없다. 증거가 없으면 `NOT_VERIFIED`다.

필수 최종 evidence:

```text
docs/evidence/
├─ T00 ... T14/
│  ├─ commands.txt
│  ├─ test-results.txt
│  ├─ dist-bytes.txt
│  └─ 필요한 capture/hash/map
└─ RC/
   ├─ final-validation.txt
   ├─ dependencies.txt
   ├─ sha256.txt
   └─ three-pc-checklist.md
```

---

# 11. Context와 subagent 사용

AUTONOMOUS ORCHESTRATOR는 모든 구현 세부를 한 context에 계속 쌓지 않는다.

- 항상 유지: authority manifest, 현재 accepted SHA, 현재 티켓, acceptance, build/test 명령, blockers
- 티켓별 로드: 해당 production spec 절, runtime fields, allowlist, 직전 handoff
- 구현 역할: 변경과 1차 test
- reviewer 역할: 읽기 전용 반례 탐색
- integrator 역할: 전체 회귀와 로컬 ff-only

subagent 기능이 있으면 구현자와 검토자를 분리한다. 없으면 같은 Codex 세션이 역할을 순차 전환하되, 검토 단계에서 먼저 구현자의 결론을 믿지 말고 diff와 명령을 처음부터 재현한다.

병렬 writer는 사용하지 않는다. 조사·로그 분석·읽기 전용 리뷰만 병렬화할 수 있다.

---

# 12. 정지 보고 형식

사람에게 보고하고 멈추는 형식은 두 종류뿐이다.

## HUMAN_CHECKPOINT

```text
HUMAN_CHECKPOINT
Stage:
Why only a human can decide:
Ready artifact/executable:
Exact steps, under 10 minutes:
Questions to answer:
If PASS, next autonomous action:
If FAIL, evidence to return:
```

## HARD_BLOCKER

```text
HARD_BLOCKER
Ticket and base SHA:
Last passing state:
Exact failing command/reproduction:
First observed mismatch:
Three repair approaches attempted:
Why authority, permission, or missing input is required:
Smallest user decision/action:
No unsupported PASS claims: YES
```

일반 build/test failure는 이 형식으로 사용자에게 보내지 않고 먼저 자동 수리한다.

---

# 13. `/goal`에 등록할 지속 목표

이 프로젝트는 단발성 prompt보다 `/goal`에 정확히 맞는다. 전체 RC라는 완료 조건이 지속되므로 한 티켓이 끝나도 목표가 아직 충족되지 않았다면 다음 티켓으로 계속 갈 수 있다.

역할은 다음처럼 나눈다.

- `/goal`: 짧고 지속적인 최종 결과·검증·금지사항
- 이 controller: 4,000자에 넣기 어려운 실행 순서·권한·실패 복구·Gate
- authority와 runbook: 게임 규칙·수치·티켓 allowlist
- Git SHA·`docs/STATUS.md`·handoff·evidence: 중단 후 재개할 실제 체크포인트

`/goal`에는 production spec 전체를 복사하지 않는다. 아래처럼 **결과·권위 경로·자율 루프·정지 조건**만 넣는다. 세부 구현은 저장소 문서가 담당한다.

## 13.1 기능 확인과 제어

VS Code Codex의 slash command 목록에 `/goal`이 없으면 Codex를 갱신한 뒤 `/experimental`에서 Goals를 켜고 재시작한다. CLI를 함께 쓴다면 다음도 사용할 수 있다.

```powershell
codex --version
codex features enable goals
```

실행 중 제어:

```text
/goal          현재 Goal 확인
/goal edit     완료 조건 수정
/goal pause    안전하게 일시중지
/goal resume   같은 Goal 재개
/goal clear    Goal 제거
```

Goal 본문은 최대 4,000자이므로 긴 계약은 이 파일에 두고 Goal이 파일을 참조하게 한다.

## 13.2 실제 Goal 명령

Codex 화면에서 아래 블록을 `/goal`부터 한 번에 그대로 등록한다.

```text
/goal Complete BORROWED PIXELS v2.1 from the current repository state to a locally verified Windows x64 contest RC candidate before submission.

Repository: C:\Users\sdjsd\Desktop\BORROWED_PIXELS

The authoritative game rules are in docs/authority. The autonomous operating rules and Human Director delegation are in docs/guides/BORROWED_PIXELS_AUTONOMOUS_COMPLETION_CONTROLLER_v2.1.md. Read them completely and preserve their precedence.

Continue through the first incomplete ticket in T00→T14 order using a closed loop: investigate, implement, build, test, run/capture, measure exact contest bytes, independently review, automatically repair failures, reverify, commit, locally fast-forward integrate eligible work, run full regression, and continue to the next ticket. Do not stop at a plan, scaffold, READY_FOR_REVIEW, or a normal build/test failure.

Final machine-verifiable target: required v2.1 content reachable, both ledgers always exactly 1,440, deterministic tests passing, P0/P1=0 for integrated candidates, MSVC x64 debug/test/contest builds reproduced, and dist/contest <=1,350,000 extracted bytes with hashes, dependencies, rights, and evidence prepared.

Stop only for the Gate 3/4 human-play checkpoint before T08, a genuine authority/API decision, three different failed repair approaches to one root cause, or final human-only three-PC/submission actions. Never push, publish, submit, spend money, change authority, or introduce external runtime material.
```

`/goal`이 등록된 뒤 각 새 세션의 첫 메시지는 길게 반복할 필요가 없다.

```text
Resume the active BORROWED PIXELS completion goal from the actual repository state. Read the authority and autonomous controller, identify the first incomplete accepted ticket from SHA and evidence, and continue executing until a defined human checkpoint or the verified RC candidate. Do not return only a plan.
```

`/goal`의 역할은 장기 결과를 잊지 않게 하는 것이고, Git SHA·`docs/STATUS.md`·handoff·evidence가 세션 간 실제 진행 상태를 기억한다. `/goal`만 믿고 저장소 상태 기록을 생략하지 않는다.

Codex가 Goal을 등록하고도 계획만 제출하면 같은 Goal thread에 다음 한 줄을 보낸다.

```text
The Goal is active. Begin executing the first incomplete ticket now; a plan is not a stopping condition.
```

---

# 14. Goal을 사용할 수 없을 때의 MASTER AUTORUN PROMPT

아래는 `/goal`이 없는 환경에서만 사용한다. Goal을 정상 등록했다면 중복해서 붙일 필요가 없다. 세션에는 저장소 루트 `C:\Users\sdjsd\Desktop\BORROWED_PIXELS`를 연다.

```text
You are BP-AUTONOMOUS-ORCHESTRATOR, responsible for producing the locally verified BORROWED PIXELS v2.1 contest RC candidate, not a plan or prototype.

Repository root:
C:\Users\sdjsd\Desktop\BORROWED_PIXELS

Read completely before acting:
1. docs/authority/BORROWED_PIXELS_AUTHORITY_v2.1.md
2. docs/authority/BORROWED_PIXELS_FINAL_PRODUCTION_SPEC_v2.1.md
3. docs/authority/BORROWED_PIXELS_RUNTIME_CONTRACT_v2.1.json
4. docs/authority/BORROWED_PIXELS_FONT_GLYPHS_v2.1.json
5. docs/authority/BORROWED_PIXELS_AGENT_RUNBOOK_v2.1.md
6. docs/guides/BORROWED_PIXELS_IMPLEMENTATION_STARTER_GUIDE_v2.1.md
7. docs/guides/BORROWED_PIXELS_AUTONOMOUS_COMPLETION_CONTROLLER_v2.1.md

The authority manifest controls all game rules, constants, coordinates, copy, narrative, encounter behavior, saves, and acceptance criteria. Never redesign or silently reduce them.

Human Director delegation for this deadline:
- You may create local branches/worktrees, edit within each ticket allowlist, build, test, run, capture, commit, create candidate branches, repair failures, and locally fast-forward integrate a ticket when authority hashes match, required automated checks pass, and independent review has P0/P1=0.
- This delegation supersedes only the runbook requirement to wait for individual human confirmation before each local merge.
- You may not push, publish, submit, spend money, introduce external runtime material, delete user data outside ticket-owned build artifacts, change authority, or claim subjective human gates passed.

Primary outcome:
Continue from the current clean integration base through T00→T14 and produce a reproducible Windows x64 contest RC candidate under 1,350,000 extracted bytes, with all machine-verifiable evidence. Do not stop after planning, scaffolding, one ticket, or READY_FOR_REVIEW. READY_FOR_REVIEW is an internal transition: invoke or perform an independent read-only review, automatically repair P0/P1 findings, integrate eligible tickets locally, and proceed to the next ticket.

First actions:
1. Verify x64 MSVC, CMake, Git, rc, and mt.
2. Report integration HEAD, branch, worktree list, clean status, and all authority hashes.
3. Read docs/STATUS.md and existing handoffs if present.
4. Determine the first incomplete ticket from actual accepted commits and evidence; do not redo accepted work.
5. Establish the exact build/test/dist commands, then begin that ticket immediately.

Per-ticket closed loop:
INVESTIGATE → IMPLEMENT → BUILD → TEST → RUN/CAPTURE → MEASURE BYTES → READ-ONLY REVIEW → REPAIR → REVERIFY → COMMIT → LOCAL FF-ONLY INTEGRATE → FULL REGRESSION → NEXT TICKET.

Rules:
- One writer and one ticket at a time. T08A and T08B are also sequential.
- Use the runbook allowlist and acceptance verbatim.
- Preserve all existing user changes and authority files.
- A normal failure is an input to automatic repair, not a reason to ask the user.
- On first failure, fix the first direct cause. On second, recheck assumptions/environment. On third, create a minimal reproduction and change approach.
- Tests must not be weakened to match the implementation.
- A changed candidate SHA invalidates prior review.
- Never mark an unrun Windows check PASS.
- Track exact contest folder bytes after every ticket and explain >=8KB growth; stop auto-integration for >=25KB unexplained growth.
- Keep docs/STATUS.md, ticket OUTPUT, and evidence current without replacing implementation work with documentation.

Stop only for:
1. Gate 3/4 human play checkpoint after T07 and before T08;
2. an authority/API decision that cannot be implemented within the current ticket;
3. three genuinely different failed repair approaches to the same root cause;
4. final RC human checks, three-PC execution, external push, or contest submission.

At a human stop, use exactly HUMAN_CHECKPOINT or HARD_BLOCKER from the controller. Otherwise continue autonomously.

Final required state:
- T00–T14 machine-verifiable work integrated locally or each truthful exception isolated;
- debug/test/contest builds reproduced on MSVC x64;
- P0/P1=0 for integrated candidates;
- Physical and Claim ledgers each remain exactly 1,440;
- required encounters, 81 PRECEDENT profiles, five endings, save, KO/EN, options, synthesized audio reachable as specified;
- dist/contest exact bytes <=1,350,000;
- dependencies, hashes, rights/provenance, evidence, README, and final handoff prepared;
- no push or submission performed.

Begin by inspecting the actual repository and continue executing. Do not return a plan as the final result.
```

---

# 15. Gate 4 재개 프롬프트

사람이 수직 슬라이스를 확인한 뒤 같은 세션에 붙인다.

```text
GATE4 HUMAN RESULT
GATE4_PASS: YES | NO
Observed control problems:
Observed readability problems:
Could explain body→projectile→Neutral/reclaim: YES | NO
Could explain HUD as material loan: YES | NO
Could explain post-VESSEL combat as choice, not survival: YES | NO

Use this evidence according to the v2.1 Gate 4 criteria. If the result exposes an implementation defect, repair and reissue the checkpoint. If PASS, record the evidence and resume autonomous execution from T08A without asking for another per-ticket approval.
```

---

# 16. 최종 인계 조건

Codex의 마지막 응답은 다음만 포함한다.

```text
RC STATUS: CANDIDATE | BLOCKED
INTEGRATION HEAD:
CONTEST EXE:
DIST FILES AND EXACT TOTAL BYTES:
SHA-256:
MSVC BUILD/TEST RESULT:
LEDGER/DETERMINISM RESULT:
P0/P1:
DEPENDENCIES:
HUMAN-ONLY CHECKS:
THREE-PC CHECKLIST:
SUBMISSION NOT PERFORMED: YES
```

설명만 있고 실행 파일·명령·증거가 없으면 RC가 아니다.
