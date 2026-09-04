# BORROWED PIXELS v2.1 제작 출처

## 프로젝트 자료

- 게임 규칙·문구·수치·좌표의 출처는 `docs/authority/`의 v2.1 권위 패키지다.
- UI atlas PNG/SVG는 개발 검토 자료이며 런타임 또는 contest 배포물에 포함하지 않는다.
- 구 v1.0/v2.0 문서는 구현 입력으로 사용하지 않는다.

## T00 제작 기록

- CMake 구성, PowerShell 검증 도구, 운영 문서는 프로젝트 전용으로 새로 작성했다.
- 구현은 AI-assisted이며 Human Director가 범위와 권위를 지정했다.
- 외부 코드 조각, 엔진, third-party runtime, 다운로드 에셋, 외부 폰트·이미지·오디오는 사용하지 않았다.
- MSVC, Windows SDK, CMake, Git은 제작 도구이며 contest 폴더에 재배포하지 않는다.
- T00의 생성 C++ stub은 빌드 디렉터리에만 생성되며 게임 규칙이나 T01 Win32 동작을 구현하지 않는다.

후속 티켓은 새 코드·문구·자료의 출처와 사람 승인 여부를 이 문서에 이어 기록한다.

## T11-T12 제작 기록

- 오디오는 외부 음원 없이 프로젝트 코드의 triangle/pulse/noise 합성기로 실행 중 생성한다.
- 영문·한국어 문구는 v2.1 권위 문서 Appendix A와 승인된 UI 용어만 사용한다.
- Latin 5×7 bitmap과 한글 8×12 자모 조합은 저장소 원본 코드이며 외부 폰트 파일을 사용하지 않는다.
- 구현 과정은 AI-assisted였고 Human Director가 지시·검토했다. 크레딧에는 `AI-assisted design and implementation; human-directed and reviewed`를 사실대로 표시한다.
- 이미지·폰트·음원·엔진·third-party runtime 등 외부 런타임 에셋은 0개다.
