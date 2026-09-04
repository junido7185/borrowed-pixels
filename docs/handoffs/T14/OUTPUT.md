# T14 RC output

State: `RC_REPAIR_FINAL_REVIEW_PENDING`

Feature development is frozen. `tools/verify_rc.ps1` completed with
`RC_VALIDATION_STATUS=PASS` on Windows x64 MSVC.

- Executable: `dist/contest/BorrowedPixels.exe`
- Executable bytes: `289280`
- Executable SHA-256: `bcd5b1ba8beb509ab1814a786e350fcdadfbf10b9ffdba3c019f7fb6292150a0`
- Extracted dist: `291734` bytes in exactly three whitelisted files
- Dependencies: `KERNEL32.dll`, `USER32.dll`, `GDI32.dll`, `WINMM.dll`
- Debug/Test/CTest/Contest/live-window/MAP/authority/dist: PASS
- Review of `10f8ec6...`: P0=0/P1=3/P2=1; all findings repaired and not integrated
- Review of `15368d1...`: P0=0/P1=4/P2=0; all findings repaired and not integrated
- Review of `b0c4548...`: P0=0/P1=2/P2=1; all findings repaired and not integrated
- Independent final review: pending on the repaired committed candidate

Human-only debt:

- Gate 3/4 subjective review: `HUMAN_REVIEW_DEFERRED`
- Gate 5 subjective playtest: `HUMAN_REVIEW_DEFERRED`
- Three-PC launch/Defender/SmartScreen: `HUMAN_REVIEW_DEFERRED`
- Upload/publication/submission: not authorized and not performed
