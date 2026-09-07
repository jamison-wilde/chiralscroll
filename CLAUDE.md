# CLAUDE.md — ChiralScroll

Pure Win32 + C++20 tray app for touchpad chiral edge scrolling. **Zero external
dependencies is a hard project rule** — no vcpkg, no third-party libraries; the
user chose a full rewrite specifically to eliminate them. Justify any exception
to the user before adding it.

## Build & verify

- Build (repo root): `"/c/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe" ChiralScroll.sln -p:Configuration=Debug -p:Platform=x64 -m -v:m`
- Gate: **0 errors AND 0 warnings** (W4 + warnings-as-errors). Both Debug and Release must stay clean.
- Smoke: run `ChiralScroll/x64/Debug/ChiralScroll.exe --logLevel debug`, alive 5s, `chiralscroll.log` shows `ContactInfo link=` lines. The log is exclusively locked while running — read it after the kill.
- **The user often runs the exe from `ChiralScroll/x64/Debug` as their live scroller.** Never `taskkill //IM ChiralScroll.exe`. Kill path-scoped:
  `powershell -NoProfile -Command 'Get-Process ChiralScroll -ErrorAction SilentlyContinue | Where-Object { $_.Path -like "*x64*" } | Stop-Process -Force'`
  (single quotes at the bash level — double quotes let bash eat `$_`). Relaunch a fresh instance after rebuilds and leave it running.
- No test suite (deliberate: GUI + hardware input). Gate = clean build + smoke + user feel-test for anything touching scroll behavior.

## Architecture map (src/)

- `Main.cpp` — wWinMain, App class: hidden window, WM_INPUT routing, tray icon/menu (incl. TaskbarCreated re-add, dialogOpen_ reentrancy guard), command line.
- `ChiralScroll.cpp/.h` — session orchestration: which touch starts/continues/ends a scroll, typing lockout.
- `TouchSession.cpp/.h` — scroll math: deadzones, reverse cone, rotation-sense (chirality) tracking. **Feel-sensitive: changes here need a user touchpad test.**
- `WinScroller.cpp/.h` — SendInput wheel synthesis with time-batched flushing. `Flush()` is destructor-reachable and must never throw.
- `HidUtils.cpp/.h` — raw input / HID report parsing, device enumeration, frame assembly.
- `Settings.cpp/.h` — settings.ini via GetPrivateProfileString. **Format is user data: additive keys only; `ToFile` writes only `enabled` for globals (hidden keys must survive round-trips).** GlobalSettings fields exist in THREE ordered places: struct, kDefaultSettings, FromFile init list — keep all three in sync.
- `SettingsDialog.cpp/.h` — modal native dialog (IDD_SETTINGS in resources/), edits a Settings copy; only Save commits.
- `TouchZoneCtrl.cpp/.h` — custom control "ChiralTouchZoneCtrl": DPI-scaled, double-buffered GDI zone editor.
- `Log.cpp/.h` — minimal std::format logger. Runtime strings via `LOG_x("{}", s)`; format strings must be literals.
- `resources/ChiralScroll.rc` — keep ASCII/UTF-8 (it was UTF-16 once; the Write tool preserves existing UTF-16 — verify bytes if rewriting).

## Style & limits

- Tabs, Allman braces, `namespace chiralscroll`, trailing `_` members, THROW_IF_* macros for Win32 failures (but never from destructor-reachable paths).
- Size caps ratcheted: file < 900 lines, function < 90 (see parent CLAUDE.md decomposition discipline).

## Behavior invariants

- Keyboard raw input is timestamp-only — never parse/store/log key data (privacy stance documented in README).
- `scrollFlushMs=0` must reproduce exact per-report SendInput behavior.
- Straight-line scrolling must be unaffected by chirality code (sense only establishes under sustained rotation).
- WM_INPUT path is ~125Hz: no per-event allocations/handles beyond the existing ones.

## Docs

- `docs/BACKLOG.md` — open/resolved work items (check before planning).
- `docs/superpowers/plans|specs/` — rewrite and backlog plan history; `wx-baseline` git tag = original wxWidgets code.
