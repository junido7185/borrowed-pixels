# BORROWED PIXELS v2.1 — AUTHORITY MANIFEST

## 상태

- 설계 상태: `DESIGN_FREEZE`
- 착수 순서 확정: T00 즉시 → T01·T02 → API gate → T03·T04 → T05~T07
- 전체 콘텐츠 착수: 1막 수직 슬라이스 Gate 4 통과 뒤
- 최종 공모전·Steam 품질: 실제 Windows 빌드와 플레이테스트 전 `PENDING`

## 권위 파일과 SHA-256

| 순위 | 파일 | SHA-256 | 역할 |
|---:|---|---|---|
| 1 | `BORROWED_PIXELS_RUNTIME_CONTRACT_v2.1.json` | `9221d45d7f0bdead2cc211cc38329c0cdad12799c02f99238eb5c1a0ce56861e` | 정수·좌표·상태·질량·전장·오디오 |
| 2 | `BORROWED_PIXELS_FONT_GLYPHS_v2.1.json` | `6adee18ff38166820f414f4748e152cc70823edf80252d117818cab9837a5fcf` | Latin bitmap·한글 조합 계약 |
| 3 | `BORROWED_PIXELS_FINAL_PRODUCTION_SPEC_v2.1.md` | `99c2c285fbe15dd3861c51c99c5fb71df74cbf6bda4c20eba99667f69c2fb343` | 의미·서사·엔딩·규칙·인수 조건 |
| 4 | `BORROWED_PIXELS_AGENT_RUNBOOK_v2.1.md` | `4e77e88e660a5f4861d246132df0fa0a9f9a2a71d1831edae8d3d39d05c77995` | 다중 세션 운영·티켓·검토 |
| 5 | `BORROWED_PIXELS_UI_REFERENCE_ATLAS_v2.1.svg` | `4e15fd4a8fc1ecd36066f4f7b57962935a44dce46ce3ef6ad39b634694d18638` | 개발용 기준 화면·구조 비교 |

검토 편의 PNG:

- `BORROWED_PIXELS_UI_REFERENCE_ATLAS_v2.1.png`
- SHA-256 `4e5ef2c6840c2b328d82a098baeb03afae0da2061cc8d206bb25ee329e98bfd2`
- 런타임 에셋이 아니며 `dist/contest`에 포함하지 않는다.

## 충돌 처리

1. 숫자·좌표·enum은 runtime/font JSON.
2. 의미·문구·조건은 production spec.
3. 작업 방법은 runbook.
4. 화면 구조는 SVG atlas.
5. 구현자의 관습과 제안은 권위가 아니다.

상위 파일끼리 충돌하면 구현을 멈추고 `SPEC_BLOCKER`를 제출한다. 평균값·관습값·임의 tuning으로 해결하지 않는다.

## 폐기 문서

다음 파일은 역사 자료이며 구현 입력이 아니다.

- `BORROWED_PIXELS_IMPLEMENTATION_SPEC_v1.0.md`
- `BORROWED_PIXELS_AI_MULTI_SESSION_RUNBOOK_v1.0.md`
- `BORROWED_PIXELS_MASTER_IMPLEMENTATION_BIBLE_v2.0.md`

구문서의 단일 AUDITOR, 세 계약, SaveV1, 점수형 OPEN LEDGER, 구형 UI 좌표를 가져오지 않는다. v2.1 production spec 32~33장에 계승할 저수준 규칙을 이미 통합했다.

## 자동 검증 기록 — 2026-08-26

- 총 검사: 189
- 통과: 189
- 실패: 0
- ClaimLedger 초기 합계: 1,440
- 이름 있는 만남: 10
- 엔딩: 5
- 각 전장 field 질량 합계: 10/10 일치
- 각 전장 neutral 질량 합계: 10/10 일치
- 모든 field AABB: 320×180 arena 내부
- 모든 UI region: canvas 내부
- TITLE 물질 인장: 64 TILE
- Latin 5×7 glyph: 59개, 행·비트 오류 0
- SVG XML parse: PASS
- PNG 기준 화면: 1,920×720 render PASS

## 아직 자동 PASS라고 부를 수 없는 것

- MSVC x64 실제 EXE 크기
- Windows 10/11 및 서로 다른 세 PC 실행
- 1막 수직 슬라이스의 조작감과 가독성
- 38~52분 플레이 시간
- 철학 이해 플레이테스트 기준
- 최종 폴더 1,350,000B 내부 gate

위 항목은 문서 누락이 아니라 구현·플레이 검증 대상이다. 결과 없이 추정 PASS로 바꾸지 않는다.

## 새 구현 세션 첫 행동

1. 이 manifest와 네 권위 원본의 실제 SHA를 비교한다.
2. 구문서를 구현 context에서 제거한다.
3. runbook의 해당 TXX 프롬프트와 allowlist를 사용한다.
4. 누락값을 발견하면 코드를 쓰기 전에 `SPEC_BLOCKER`를 제출한다.
5. 세션은 `READY_FOR_REVIEW`까지만 선언한다.
