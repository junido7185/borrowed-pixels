# T00 OUTPUT

TICKET: T00

BASE SHA: `152b0177896b83bf0d16457fb7f20e98d0b9b374`

CANDIDATE SHA: resolve the committed tip of `ticket/T00-bootstrap`; the exact immutable SHA is reported by the orchestrator after commit.

AUTHORITY HASHES:

- Runtime contract: `9221d45d7f0bdead2cc211cc38329c0cdad12799c02f99238eb5c1a0ce56861e`
- Font glyphs: `6adee18ff38166820f414f4748e152cc70823edf80252d117818cab9837a5fcf`
- Production spec: `99c2c285fbe15dd3861c51c99c5fb71df74cbf6bda4c20eba99667f69c2fb343`
- Agent runbook: `4e77e88e660a5f4861d246132df0fa0a9f9a2a71d1831edae8d3d39d05c77995`
- UI atlas SVG: `4e15fd4a8fc1ecd36066f4f7b57962935a44dce46ce3ef6ad39b634694d18638`
- UI atlas PNG: `4e5ef2c6840c2b328d82a098baeb03afae0da2061cc8d206bb25ee329e98bfd2`

CHANGED PATHS:

- `.gitattributes`, `.gitignore`
- `CMakeLists.txt`, `CMakePresets.json`
- `tools/measure_dist.ps1`, `tools/verify_authority.ps1`
- `docs/STATUS.md`, `docs/OWNERSHIP.md`, `docs/PROVENANCE.md`, `docs/RIGHTS_LOG.md`
- `docs/evidence/T00/**`
- `docs/handoffs/T00/OUTPUT.md`

ACCEPTANCE MAP:

| 기준 | 증거 |
|---|---|
| v2.1 저장소 skeleton | root build files와 `src/`, `tests/`, `tools/`, `docs/evidence/T00`, `docs/handoffs/T00`, `docs/ui/approved`, `dist/contest` 디렉터리 |
| 고정 preset interface | `docs/evidence/T00/commands.txt`의 configure/build/ctest 명령 |
| 세 target skeleton | `bp_debug`, `bp_test`, `bp_contest` 실제 MSVC x64 build 및 exit 0 실행 |
| production spec 33.2 flags | `docs/evidence/T00/contest-flags.txt` |
| exact dist gate | `docs/evidence/T00/dist-bytes.txt`, 정상/RC/hard 세 exit 경로 |
| contest output 격리 | `dist/contest/BorrowedPixels.exe` 한 파일, 107,008B |
| 권위 무변경 | `tools/verify_authority.ps1` 6/6 PASS |
| 외부 runtime/asset 없음 | `docs/PROVENANCE.md`, `docs/RIGHTS_LOG.md`, dependency evidence |

COMMANDS RUN: `docs/evidence/T00/commands.txt`

TEST RESULTS: `docs/evidence/T00/test-results.txt`

DIST BYTES / DELTA: `107,008B`; baseline dist가 없어서 delta는 `NOT_AVAILABLE`.

DETERMINISM RESULT: `NOT_APPLICABLE` — T00에는 simulation이나 game rule이 없다.

RIGHTS / ASSET RESULT: 외부 runtime·library·asset·network·browser·runtime AI 0. 권위 atlas는 배포하지 않았다.

KNOWN RISKS:

- T00 executable은 의도적으로 즉시 종료하는 target skeleton이다. 창·입력·게임 동작은 T01 이후 범위다.
- 세 원장·replay·save 검증은 해당 기능이 아직 없어 `NOT_APPLICABLE`이며 T02/T04에서 실제 검사해야 한다.
- 최종 인간 권리·상표·3PC·Defender/SmartScreen 검증은 후속 gate에 남아 있다.

STATUS: READY_FOR_REVIEW
