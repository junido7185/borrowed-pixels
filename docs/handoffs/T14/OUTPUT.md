# T14 RC output

State: `RC_FINAL_REVIEW_PENDING`

Feature development is frozen. `tools/verify_rc.ps1` completed with
`RC_VALIDATION_STATUS=PASS` on Windows x64 MSVC.

- Executable: `dist/contest/BorrowedPixels.exe`
- Executable bytes: `284160`
- Executable SHA-256: `03d5feef3bb15c3f1433056909fbe3c6f50ec05b7df927e1d2a318dafd946551`
- Extracted dist: `286204` bytes in exactly three whitelisted files
- Dependencies: `KERNEL32.dll`, `USER32.dll`, `GDI32.dll`, `WINMM.dll`
- Debug/Test/CTest/Contest/live-window/MAP/authority/dist: PASS
- Independent final review: pending on the committed candidate

Human-only debt:

- Gate 3/4 subjective review: `HUMAN_REVIEW_DEFERRED`
- Gate 5 subjective playtest: `HUMAN_REVIEW_DEFERRED`
- Three-PC launch/Defender/SmartScreen: `HUMAN_REVIEW_DEFERRED`
- Upload/publication/submission: not authorized and not performed
