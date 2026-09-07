# BORROWED PIXELS

정확히 1,440개의 TILE만 존재하는 세계에서 빌린 몸과 기능으로 싸우고, 타인의 몫을 재배분하는 고정 화면 2D 액션 게임입니다. 플레이어의 선택은 마지막에 세계의 규칙으로 돌아옵니다.

C++20과 Windows 기본 API로 제작하며, 외부 엔진·이미지·폰트·음원 없이 도형, 자체 글리프, 실시간 합성 오디오를 사용합니다.

## 주요 구성

- 3개 Act, 10개의 만남, 9회의 3지선다와 5개 엔딩
- 각각 1,440을 보존하는 물리 원장과 청구 원장
- 회수, HUD 대여·재할당, 선택에 따라 달라지는 PRECEDENT 규칙
- 한국어·영어, 키보드·마우스·게임패드 지원
- 볼륨, 화면 흔들림, 섬광 감소, 색상 보조, 전체 화면 옵션
- 로컬 저장과 오프라인 실행

## 개발 환경

- Windows 10/11 x64
- Visual Studio 2022 또는 Build Tools 2022: C++ 데스크톱 개발 도구와 Windows SDK
- CMake 3.25 이상
- PowerShell

빌드 프리셋은 `Visual Studio 17 2022`의 x64 구성을 사용합니다. Windows 및 MSVC 이외의 환경은 현재 지원하지 않습니다.

## 빌드와 테스트

저장소 루트에서 실행합니다.

```powershell
# 프로젝트 구성
cmake --preset windows-msvc

# 테스트 빌드 및 실행
cmake --build --preset test
ctest --preset all --output-on-failure

# 개발용 실행 파일
cmake --build --preset debug
.\build\windows-msvc\bin\DEBUG\bp_debug.exe

# 공모전용 실행 파일
cmake --build --preset contest
.\dist\contest\BorrowedPixels.exe
```

빌드 결과와 실행 파일은 Git 추적에서 제외됩니다. 저장소를 새로 복제한 경우 위 명령으로 실행 파일을 생성하세요.

기준 문서와 배포 폴더는 다음 명령으로 검증할 수 있습니다. 배포 검증은 공모전용 빌드 후 실행합니다.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/verify_authority.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools/verify_dist_contents.ps1 -Path dist/contest
powershell -NoProfile -ExecutionPolicy Bypass -File tools/measure_dist.ps1 -Path dist/contest -InternalCap 1350000 -HardCap 1474560
```

전체 RC 검증 스크립트는 `tools/verify_rc.ps1`입니다. 빌드·테스트·실행 화면 확인과 증거 파일 생성을 수행합니다. 기존 검증 기록을 보존하려면 `-EvidenceDir build/rc-validation`처럼 별도 출력 경로를 지정하세요.

## 조작법

| 동작 | 키보드·마우스 | 게임패드 |
|---|---|---|
| 이동 / 조준 | W/A/S/D / 마우스 | 왼쪽 / 오른쪽 스틱 |
| 발사 / 회수 | 왼쪽 / 오른쪽 클릭 | 오른쪽 / 왼쪽 트리거 |
| 대시 | Space | A |
| HUD 모듈 / 재할당 | Q / E | LB / Y |
| 선택 | 1/2/3 또는 방향키 | D-pad |
| 확정 | Enter 또는 왼쪽 클릭 | A |
| 옵션 / 뒤로 | Esc | Start |
| 전체 화면 전환 | Alt+Enter | 옵션 메뉴 |

저장 파일 위치: `%LOCALAPPDATA%\BorrowedPixels\save.bin`

실행 환경과 배포 안내는 [배포용 README](dist/contest/README.txt)를 참고하세요.

## 저장소 구조

| 경로 | 내용 |
|---|---|
| `src/` | 게임 로직, Win32 플랫폼, 렌더링, 오디오, 저장, 현지화 |
| `tests/` | 원장·전투·진행·입력·렌더링·오디오 등의 자동 검증 |
| `tools/` | 기준 문서 해시, 배포 구성·용량, RC 검증 스크립트 |
| `docs/authority/` | 제작 명세, 런타임 계약, 승인된 변경 및 기준 화면 |
| `docs/guides/` | 구현 및 완료 관리 가이드 |
| `docs/handoffs/` | 작업별 구현·인계 기록 |
| `docs/evidence/` | 검증 로그와 검토 증거 |
| `dist/contest/` | 공모전용 배포 안내 및 빌드 출력 |

## 현재 상태

프로젝트 기록상 T14까지 통합된 **RC_CANDIDATE**입니다. 로컬 Windows x64 빌드·자동 테스트·배포 검증은 통과했으며, 주관적 플레이 평가와 여러 PC에서의 검증은 보류되어 있습니다. 최종 출시 인증 완료를 뜻하지 않습니다.

동결된 RC 배포 기록은 실행 파일 `290,304`바이트, 배포 파일 3개 합계 `292,810`바이트입니다. 새 빌드의 실제 용량은 검증 스크립트로 확인하세요.

- [개발 및 검증 상태](docs/STATUS.md)
- [T14 최종 인계](docs/handoffs/T14/OUTPUT.md)
- [사람이 확인할 항목](docs/handoffs/T14/HUMAN_CHECKLIST.md)
- [제작 명세](docs/authority/BORROWED_PIXELS_FINAL_PRODUCTION_SPEC_v2.1.md)
- [기준 문서 및 우선순위](docs/authority/BORROWED_PIXELS_AUTHORITY_v2.1.md)

## 제작 및 권리

제작자: **DEV_SLIME**. Human Director의 지휘·검토 아래 AI 보조 구현을 사용합니다.

별도의 오픈소스 라이선스는 지정되어 있지 않습니다. 권리와 출처는 [권리 대장](docs/RIGHTS_LOG.md), [제작 출처 기록](docs/PROVENANCE.md), [배포 고지](dist/contest/LICENSES.txt)를 참고하세요.
