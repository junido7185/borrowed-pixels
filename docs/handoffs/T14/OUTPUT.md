# T14 RC output

State: `RC_CANDIDATE`

Candidate `9f19a71cb632affa97bdc102f489cc404211a2a8` was locally fast-forward
integrated into `integration/rc-2026-09-03`.

Feature development is frozen. `tools/verify_rc.ps1` completed with
`RC_VALIDATION_STATUS=PASS` on Windows x64 MSVC.

- Executable: `dist/contest/BorrowedPixels.exe`
- Executable bytes: `290304`
- Executable SHA-256: `cf5b2380e13a12c2f970b311503389773c361bd91f4952fd28c79cbed1807699`
- Extracted dist: `292758` bytes in exactly three whitelisted files
- Dependencies: `KERNEL32.dll`, `USER32.dll`, `GDI32.dll`, `WINMM.dll`
- Debug/Test/CTest/Contest/live-window/MAP/authority/dist: PASS
- Review of `10f8ec6...`: P0=0/P1=3/P2=1; all findings repaired and not integrated
- Review of `15368d1...`: P0=0/P1=4/P2=0; all findings repaired and not integrated
- Review of `b0c4548...`: P0=0/P1=2/P2=1; all findings repaired and not integrated
- Review of `b143f8f...`: P0=0/P1=2/P2=2; both P1 and the T11 P2 were repaired
- Final Windows validation: PASS; open P0/P1=0, remaining P2 is the historical T05 test-only fault-injection debt

Human-only debt:

- Gate 3/4 subjective review: `HUMAN_REVIEW_DEFERRED`
- Gate 5 subjective playtest: `HUMAN_REVIEW_DEFERRED`
- Three-PC launch/Defender/SmartScreen: `HUMAN_REVIEW_DEFERRED`
- Upload/publication/submission: not authorized and not performed
