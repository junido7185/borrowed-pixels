# T01 OUTPUT

TICKET: T01

BASE SHA: `2cb1502269000184311e6d1acb909864aa813e30`

CANDIDATE SHA: resolve the committed tip of `ticket/T01-platform`; the controller reports the exact immutable SHA after commit.

AUTHORITY HASHES:

- Runtime contract: `9221d45d7f0bdead2cc211cc38329c0cdad12799c02f99238eb5c1a0ce56861e`
- Font glyphs: `6adee18ff38166820f414f4748e152cc70823edf80252d117818cab9837a5fcf`
- Production spec: `99c2c285fbe15dd3861c51c99c5fb71df74cbf6bda4c20eba99667f69c2fb343`
- Agent runbook: `4e77e88e660a5f4861d246132df0fa0a9f9a2a71d1831edae8d3d39d05c77995`
- UI atlas SVG: `4e15fd4a8fc1ecd36066f4f7b57962935a44dce46ce3ef6ad39b634694d18638`
- UI atlas PNG: `4e5ef2c6840c2b328d82a098baeb03afae0da2061cc8d206bb25ee329e98bfd2`

CHANGED PATHS:

- `src/bp_platform.h`
- `src/bp_win32.cpp`
- `docs/STATUS.md`
- `docs/evidence/T01/**`
- `docs/handoffs/T01/OUTPUT.md`

IMPLEMENTED:

- Per-Monitor-V2 Win32 window with system-aware fallback, 1280x720 default client, 640x360 minimum, DPI change handling, and Alt+Enter borderless mode.
- Fixed 320x180 32-bit GDI backbuffer with nearest-neighbor integer scaling and BG letterbox.
- QPC 60Hz fixed accumulator with five-tick cap, high-resolution waitable timer, and fallback-only timer-period adjustment.
- Keyboard/mouse edge collection, repeat suppression, canvas coordinate conversion, and dynamically loaded XInput with keyboard/mouse fallback.
- Focus-loss input clear, simulation pause, and 15fps background rendering.
- Non-fatal 48kHz stereo int16 waveOut probe/silent fallback shell.
- Debug-only platform self-tests and deterministic smoke auto-close; no game behavior or rules.
- Saturating signed-axis inversion so the valid XInput extreme `-32768` maps deterministically to `+32767` for movement and aim.

ACCEPTANCE EVIDENCE:

- Commands: `docs/evidence/T01/commands.txt`
- Contract mapping: `docs/evidence/T01/platform-contract.txt`
- Tests: `docs/evidence/T01/test-results.txt`
- Dist: `docs/evidence/T01/dist-bytes.txt`
- Dependencies/PE: `docs/evidence/T01/dependencies.txt`
- Contest flags: `docs/evidence/T01/contest-flags.txt`
- Visual: `docs/evidence/T01/platform-shell.png`

DIST BYTES / DELTA: `126,464B`; T00 baseline `107,008B`; delta `+19,456B`.

CONTEST SHA-256: `d9a833d92eaa76a7ed012f6934c6adb4030f63706ecc13f7b8d06f1ad1c67acf`; two clean repaired builds matched.

RIGHTS / ASSET RESULT: no external library, runtime asset, network, browser, or runtime AI was introduced. Only authority-derived constants and OS APIs are used.

KNOWN RISKS / NOT_RUN:

- Physical gamepad interaction was not run because no controller was available; dynamic loading and fallback were automated/code-reviewed.
- Forced waveOut and timer-unavailability paths were not injected; both paths are non-fatal in source.
- A controlled cross-monitor WM_DPICHANGED drag was not available; the 150% primary-display client size was verified exactly.
- T01 exposes platform/input types for the API gate but intentionally does not connect an engine, simulation, renderer, or game rule.

STATUS: READY_FOR_REVIEW
