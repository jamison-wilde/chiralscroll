# ChiralScroll Win32 Rewrite — Spec

## Goal

Remove all third-party dependencies (wxWidgets, abseil, spdlog) and the vcpkg
toolchain. The app becomes a single dependency-free Win32 exe built by MSVC
alone, compiling in seconds.

## What must NOT change (behavior parity)

The scroll engine is untouched: `TouchSession.*`, `Scroller.h`, `WinScroller.*`
(logic), `Vector.h`, HID parsing in `HidUtils.*` (logic), settings file format
(`settings.ini` via `GetPrivateProfileString`, same section/key names), log file
name (`chiralscroll.log` in CWD, truncated on start).

Preserved user-visible behavior:
- Tray icon (IDI_CHIRALSCROLL), left- OR right-click opens menu: Enable
  (checkable), Settings, separator, Close.
- Settings dialog: device combo (sorted, read-only), Enable checkbox,
  Keyboard Lockout (ms), Vertical/Horizontal Scrolling Speed edits, touchpad
  zone widget (grey rounded rect, green vertical-zone hatch from right, red
  horizontal-zone hatch from bottom, two draggable grabbers), Save/Cancel.
  Dialog edits a COPY of settings; only Save commits (assigns, applies via
  `ChiralScroll::SetSettings`, writes `settings.ini`).
- Command line: `--logToConsole` (also `-logToConsole`), `--logLevel <lvl>`
  (trace|debug|info|warn|err|critical|off, default warn),
  `--panicOnUnexpectedInput`.
- Raw input: hidden top-level window registers RIDEV_INPUTSINK for keyboard
  (typing lockout timestamp ONLY — no key data parsed) and touchpad digitizer.
  `RIM_INPUT` wparam → DefWindowProc passthrough.
- Unhandled exception in message handling: stop input processing, log error,
  MessageBox "ChiralScroll Error".

## Allowed behavior deltas (accepted)

- Settings dialog becomes MODAL (was modeless; modal loop still dispatches
  WM_INPUT so scrolling works while it is open). Multiple simultaneous dialogs
  no longer possible (was a bug).
- Edit-field validation happens on save/device-switch (parse failure keeps the
  previous value) instead of wx keystroke validators.
- Log line format may differ cosmetically; levels and destinations identical.
- Fix latent bug: `Settings.cpp` `absl::Substitute("Error parsing $0 ... $2", key, str)`
  references `$2` with only 2 args — becomes a correct 2-arg format.
- The app is now DPI-aware (PerMonitorV2) and uses comctl32 v6 visual styles;
  the wx build shipped DPI-unaware with classic-themed controls. TouchZoneCtrl's
  grabber/corner metrics are unscaled pixels (visually smaller at high DPI —
  backlogged).
- A failed log-file open now degrades to no logging instead of a startup
  error box (spdlog threw; the minimal logger stays silent).

## Replacements

| Removed | Replaced by |
|---|---|
| wxWidgets app/frame/taskbar | `wWinMain`, hidden window, `Shell_NotifyIcon`, `TrackPopupMenu` |
| wx SettingsDialog | `IDD_SETTINGS` DIALOGEX template + DialogBoxParam |
| wx TouchpadCtrl | `TouchZoneCtrl` registered child window class (GDI) |
| spdlog | `src/Log.h/.cpp` (~100 lines, std::format, file/stderr sinks) |
| absl::flat_hash_map | std::unordered_map |
| absl::StrCat/StrFormat/Substitute | std::format |
| absl::Time | std::chrono::steady_clock |
| vcpkg manifest + integration | nothing (no external deps remain) |

## End state

- Builds with VS2022 alone: `msbuild ChiralScroll.sln -p:Configuration=Debug -p:Platform=x64`.
- Release config retargeted v142 → v143.
- `C:\vcpkg` deleted, `vcpkg integrate remove` run, `vcpkg.json`,
  `vcpkg_installed/`, `formbuilder/` deleted (all preserved in git history via
  baseline commit).
- Project under git with baseline (wx) commit preceding the rewrite.
