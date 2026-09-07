# ChiralScroll

Chiral (circular) edge scrolling for Windows Precision touchpads. Touch the
right edge of the touchpad and drag to scroll vertically, or the bottom edge to
scroll horizontally — then keep dragging in circles to scroll continuously, and
reverse the circle to reverse the scroll. It recreates the Synaptics
chiral-scroll behavior from the Windows 7 era on modern Windows, working
directly from raw touchpad input, so it runs alongside the standard Windows
Precision drivers on any touchpad. No drivers, no services, no dependencies —
one small tray exe.

This is a continuation of
[ChiralScroll by Derek Brown](https://sourceforge.net/projects/chiralscroll/)
(MIT licensed). This fork rewrites the app as pure Win32 + the C++20 standard
library (the original used wxWidgets/abseil/spdlog via vcpkg), fixes several
bugs, and adds scroll-event batching and rotation-sense reversal detection.
The scroll engine's behavior is otherwise faithful to the original.

## Usage

Run `ChiralScroll.exe` and leave it in the system tray.

- **Vertical scroll:** touch the right edge of the touchpad, drag up or down.
- **Horizontal scroll:** touch the bottom edge, drag left or right.
- **Continuous (chiral) scroll:** once scrolling, keep dragging in circles;
  circle the opposite way to reverse direction.
- **Tray menu** (left- or right-click the icon): Enable toggle, Settings, Close.

The Settings dialog lists every touchpad device with per-device options:
enable, scroll speeds (negative reverses direction), keyboard lockout (ms of
scroll suppression after typing, for palm rejection), and draggable edge-zone
sizes on a visual touchpad map.

## Settings file

Settings are stored in `settings.ini` next to the exe. The dialog covers the
common options; the `[Global Settings]` section also accepts advanced keys
(edit the file and restart the app):

| Key | Default | Meaning |
|---|---|---|
| `scrollFlushMs` | `15` | Scroll events are batched and sent at most once per this interval. `0` sends one event per touchpad report (the original behavior). Larger values calm hover/hot-tracking in list views at the cost of chunkier steps. |
| `reverseRotationRad` | `0.9` | Radians (~52°) of counter-circling before chiral scrolling flips direction. Lower flips sooner; higher resists accidental flips. |
| `startDeadzone`, `startDeadzoneAngle`, `moveDeadzone`, `reverseDeadzone`, `reverseDeadzoneAngle`, `sensScalingFactor` | — | Fine-grained gesture tuning inherited from the original; defaults are sensible. |

## Building

Visual Studio 2022, no external dependencies:

```
msbuild ChiralScroll.sln -p:Configuration=Release -p:Platform=x64
```

Debug and Release both build warning-clean. The original project recommended
the Debug build because of an occasional Release-build `SendInput` stall; that
issue has not reproduced in this rewrite (event batching greatly reduces
`SendInput` pressure), but if scrolling ever freezes on Release, try Debug and
file an issue.

## Installing

Copy the exe to any writable folder (it writes `settings.ini` and
`chiralscroll.log` beside itself) and run it. For autostart, put a shortcut in
`shell:startup` with "Start in" set to that folder.

## Privacy note

The app registers a raw-input listener for the keyboard as well as the
touchpad — this powers the typing-lockout (palm rejection) feature. Keyboard
events are used only as a timestamp; no key data is ever read, stored, or
logged.

## History

- Original ChiralScroll (wxWidgets) by Derek Brown:
  https://sourceforge.net/projects/chiralscroll/
- 2026: rewritten as dependency-free Win32 (this repo). The pre-rewrite source
  is preserved at the `wx-baseline` git tag; design notes live in
  `docs/superpowers/`, and resolved/open work in `docs/BACKLOG.md`.

## License

MIT — see [`LICENSE`](LICENSE). Original work Copyright (c) 2021 Derek Brown;
Win32 rewrite and modifications Copyright (c) 2026 Jamison Wilde.
