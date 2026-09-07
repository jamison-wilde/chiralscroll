# Backlog Fixes Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax.

**Goal:** Resolve all 7 items in docs/BACKLOG.md: 5 mechanical fixes (items 3-7) and 2 scroll-behavior improvements (items 1-2: scroll-event batching, chirality-based reversal detection).

**Architecture:** Batching lives inside `WinScroller` (accumulate + time-gated flush). Chirality lives inside `ScrollSession` (signed-rotation accumulator alongside the existing reverse-cone test). Both get tunable thresholds as hidden `[Global Settings]` ini keys (read at load, not shown in the dialog).

**Tech Stack:** existing pure Win32 + C++20 project; no new dependencies.

**Spec:** docs/BACKLOG.md (items 1-7) — the authority for what each fix must achieve.

## Global Constraints

- Build (from repo root): `"/c/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe" ChiralScroll.sln -p:Configuration=Debug -p:Platform=x64 -m -v:m` — must end 0 errors / 0 warnings (W4-as-errors).
- Smoke: exe alive 5s with `--logLevel debug`, `ContactInfo link=` lines in log; ALL kills path-scoped via `powershell -NoProfile -Command 'Get-Process ChiralScroll -ErrorAction SilentlyContinue | Where-Object { $_.Path -like "*x64*" } | Stop-Process -Force'` (single-quoted at bash level); the user's running instance in x64/Debug is killed only around rebuilds and a fresh instance relaunched and LEFT RUNNING after.
- Style: tabs, Allman braces, `namespace chiralscroll`, trailing `_` members. Size caps: files <900, functions <90 lines.
- `settings.ini` format: only ADDITIVE new keys in `[Global Settings]`; existing keys unchanged. `Settings::ToFile` continues writing only `enabled` for globals.
- LOG macros: runtime strings via `"{}"`; format strings literal.
- No test infra (standing ruling): gate = clean build + smoke + user feel-test (Task 2 checkpoint).
- One commit per backlog item (Conventional Commits), so each is independently revertible during feel-tuning.

## Execution policy

Economy. Task 1 = items 3-7 (one implementer, one batch review). Task 2 = items 1-2 (`review: gate`, then USER feel-test + tuning loop). Final combined review (sonnet) over the full range after Task 2.

## Touched files

| File | Now (approx) | Change |
|---|---|---|
| ChiralScroll/src/HidUtils.cpp | 565 | items 3,4: two `-1` checks, one trace guard |
| ChiralScroll/src/TouchZoneCtrl.cpp / .h | ~255 / ~75 | items 5,7: DPI scaling + double-buffered paint |
| ChiralScroll/src/Main.cpp | ~400 | item 6: gray/guard Enable; item 1: pass flush ms |
| ChiralScroll/src/WinScroller.cpp / .h | 46 / 24 | item 1: accumulate + flush |
| ChiralScroll/src/TouchSession.cpp / .h | 145 / 81 | item 2: rotation-sense tracking |
| ChiralScroll/src/Settings.cpp / .h | ~245 / ~72 | items 1,2: two new global fields |
| README.txt, docs/BACKLOG.md | — | Task 2: tuning note; mark items resolved |

---

### Task 1: Mechanical fixes (backlog items 3-7)

**Files:** Modify `ChiralScroll/src/HidUtils.cpp`, `ChiralScroll/src/TouchZoneCtrl.h`, `ChiralScroll/src/TouchZoneCtrl.cpp`, `ChiralScroll/src/Main.cpp`.

**Interfaces:** No signature changes consumed by Task 2. Read each file first; apply edits to its CURRENT state (post-rewrite, post-fix-waves).

- [ ] **Step 1 (item 3): fix dead error checks.** In `HidData::FromRawInput` (HidUtils.cpp), both `GetRawInputData(...) < 0` comparisons are always false (`UINT` return). Replace both with `== static_cast<UINT>(-1)`:

```cpp
	if(GetRawInputData(handle, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) == static_cast<UINT>(-1)) {
```
(and identically for the second call that fills `raw_input_bytes`).

- [ ] **Step 2 (item 4): stop paying for disabled trace.** In `GetContactsInReport` (HidUtils.cpp), the `LOG_TRACE("  button1={}, ...")` statement evaluates three `GetButton(...)` calls even when tracing is off. Wrap the statement:

```cpp
	if(logging::GetLevel() <= logging::Level::kTrace)
	{
		LOG_TRACE("  button1={}, button2={}, button3={}",
			GetButton(hidData, {0x09, 0x01}), GetButton(hidData, {0x09, 0x02}), GetButton(hidData, {0x09, 0x03}));
	}
```

- [ ] **Step 3 (item 5): DPI-scale TouchZoneCtrl metrics.** Add a private helper to TouchZoneCtrl (declare in .h, define in .cpp):

```cpp
	int Scaled(int value) const
	{
		return MulDiv(value, static_cast<int>(GetDpiForWindow(hwnd_)), 96);
	}
```

Replace every use of `kGrabberSize`, `kHitSlop`, `kCornerSize` in member functions with `Scaled(...)` equivalents: grabber rect construction in `Paint`, the two `kGrabberSize/2 + kHitSlop` slop expressions in `HitTest`, and both `kCornerSize` args to `CreateRoundRectRgn`. Query per call (no caching) so per-monitor moves stay correct.

- [ ] **Step 4 (item 7): double-buffer Paint and stop erasing.** Rework `WM_PAINT` handling so `Paint` draws into a memory DC and covers every pixel (corners included), then change both `InvalidateRect(hwnd_, nullptr, TRUE)` calls (SetZones and the drag path in HandleMessage/WM_MOUSEMOVE) to `FALSE`. In the `WM_PAINT` case:

```cpp
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC dc = BeginPaint(hwnd_, &ps);
			RECT client;
			GetClientRect(hwnd_, &client);
			HDC memDc = CreateCompatibleDC(dc);
			HBITMAP bitmap = CreateCompatibleBitmap(dc, client.right, client.bottom);
			HGDIOBJ oldBitmap = SelectObject(memDc, bitmap);
			FillRect(memDc, &client, GetSysColorBrush(COLOR_BTNFACE));
			Paint(memDc, client);
			BitBlt(dc, 0, 0, client.right, client.bottom, memDc, 0, 0, SRCCOPY);
			SelectObject(memDc, oldBitmap);
			DeleteObject(bitmap);
			DeleteDC(memDc);
			EndPaint(hwnd_, &ps);
			return 0;
		}
```

`Paint` itself is unchanged apart from the DPI scaling in Step 3 (it already clips to the round rect; the memDC pre-fill supplies the corners). Watch the function-size cap: if this pushes `HandleMessage` past 90 lines, extract the WM_PAINT block into a private `void OnPaint()`.

- [ ] **Step 5 (item 6): gray + guard the tray Enable item while the dialog is open.** In Main.cpp `ShowTrayMenu`, add `(dialogOpen_ ? MF_GRAYED : 0)` to the Enable item's flags (as already done for Settings/Close). In `OnMenuCommand`, case `kMenuEnable`: early-break if `dialogOpen_`.

- [ ] **Step 6: build + smoke** (Global Constraints; kill/relaunch dance around the rebuild, leave a fresh instance running).
- [ ] **Step 7: commit per item** (5 commits):
  - `fix: check GetRawInputData failure correctly (UINT return)`
  - `perf: skip trace-level button queries when tracing is disabled`
  - `fix: scale TouchZoneCtrl metrics for DPI`
  - `fix: double-buffer TouchZoneCtrl painting to stop drag flicker`
  - `fix: gray tray Enable item while settings dialog is open`

---

### Task 2: Scroll behavior (backlog items 1-2) — `review: gate`, then USER feel-test

**Files:** Modify `ChiralScroll/src/Settings.h`, `ChiralScroll/src/Settings.cpp`, `ChiralScroll/src/WinScroller.h`, `ChiralScroll/src/WinScroller.cpp`, `ChiralScroll/src/Main.cpp`, `ChiralScroll/src/TouchSession.h`, `ChiralScroll/src/TouchSession.cpp`, `README.txt`, `docs/BACKLOG.md`.

**Interfaces:**
- Produces: `Settings::GlobalSettings` gains `float reverseRotationRad` and `int scrollFlushMs` (appended, in that order); `WinScroller(Direction dir, int flushIntervalMs)`.
- Consumes: Task 1's edits (no interface overlap).

- [ ] **Step 1: new global settings.** Settings.h `GlobalSettings` — append after `sensScalingFactor`:

```cpp
		// Accumulated counter-rotation (radians) before circle scrolling
		// flips direction.
		float reverseRotationRad;
		// Minimum interval between synthesized scroll events; pending deltas
		// accumulate between flushes. 0 sends every event immediately.
		int scrollFlushMs;
```

Settings.cpp `kDefaultSettings` — append matching defaults after `sensScalingFactor = 0.1f;`:

```cpp
	float reverseRotationRad = 0.9f;
	int scrollFlushMs = 15;
```

Settings.cpp `FromFile` globalSettings_ init list — append after the sensScalingFactor line:

```cpp
		globalSection.READ_SETTING(reverseRotationRad),
		globalSection.READ_SETTING(scrollFlushMs),
```

- [ ] **Step 2 (item 1): batching WinScroller.** WinScroller.h becomes:

```cpp
#pragma once

#include <chrono>

#include "Scroller.h"

namespace chiralscroll
{

class WinScroller : public Scroller
{
public:
	enum class Direction { kVertical, kHorizontal };

	WinScroller(Direction dir, int flushIntervalMs)
		: dir_(dir), interval_(std::chrono::milliseconds(flushIntervalMs)) {}
	virtual ~WinScroller() { StopScrolling(); }

	void StartScrolling() override;
	void Scroll(int amt) override;
	void StopScrolling() override;

private:
	void Flush();

	const Direction dir_;
	const std::chrono::steady_clock::duration interval_;
	int pending_ = 0;
	std::chrono::steady_clock::time_point lastFlush_{};
};

}  // namespace chiralscroll
```

WinScroller.cpp: `StartScrolling` additionally sets `pending_ = 0;` and `lastFlush_ = std::chrono::steady_clock::now() - interval_;` (so the first movement flushes immediately — no start-of-scroll lag). `Scroll(int amt)` becomes:

```cpp
void WinScroller::Scroll(int amt)
{
	pending_ += amt;
	const auto now = std::chrono::steady_clock::now();
	if(now - lastFlush_ >= interval_)
	{
		Flush();
		lastFlush_ = now;
	}
}
```

`Flush()` contains the previous SendInput body, using `pending_` as `mouseData`, no-op when `pending_ == 0`, and resets `pending_ = 0` after sending (keep the existing SPDLOG-era LOG_DEBUG line, logging the flushed amount). `StopScrolling` calls `Flush()` BEFORE `ClipCursor(nullptr)` so the tail of the gesture lands.

Main.cpp: both `std::make_unique<WinScroller>(WinScroller::Direction::k...)` calls gain the second argument `settings_.GetGlobalSettings().scrollFlushMs` (settings_ is initialized before chiralScroll_ in the member-init order — verify, don't reorder).

- [ ] **Step 3 (item 2): chirality tracking in ScrollSession.** TouchSession.h ScrollSession — add private members and a method declaration:

```cpp
	// Detects sustained counter-rotation (chirality change) and flips the
	// scroll direction. Returns true if it flipped.
	bool UpdateRotation(Vector<float> newDir);
	void ResetRotation();

	// Rotation sense established for this stroke: +1 CCW, -1 CW, 0 unknown.
	float rotationSense_ = 0.0f;
	float senseAccum_ = 0.0f;
	float counterRotation_ = 0.0f;
```

TouchSession.cpp — add near AngleBetween:

```cpp
	// Radians of consistent rotation before a stroke's sense is established.
	constexpr float kSenseEstablishRad = 0.6f;
```

New methods:

```cpp
bool ScrollSession::UpdateRotation(Vector<float> newDir)
{
	// Signed angle from the previous movement direction to the new one:
	// positive is CCW in touchpad coordinates.
	const float cross = direction_.x()*newDir.y() - direction_.y()*newDir.x();
	const float dot = direction_*newDir;
	const float angle = atan2f(cross, dot);

	if(rotationSense_ == 0.0f)
	{
		senseAccum_ += angle;
		if(fabsf(senseAccum_) > kSenseEstablishRad)
		{
			rotationSense_ = senseAccum_ > 0.0f ? 1.0f : -1.0f;
			counterRotation_ = 0.0f;
		}
		return false;
	}
	if(angle*rotationSense_ < 0.0f)
	{
		counterRotation_ += fabsf(angle);
		if(counterRotation_ > settings_.reverseRotationRad)
		{
			scrollDirection_ *= -1.0f;
			rotationSense_ = -rotationSense_;
			senseAccum_ = 0.0f;
			counterRotation_ = 0.0f;
			return true;
		}
		return false;
	}
	// Rotation agrees with the established sense; decay accumulated noise.
	counterRotation_ = std::max(0.0f, counterRotation_ - fabsf(angle)*0.5f);
	return false;
}

void ScrollSession::ResetRotation()
{
	rotationSense_ = 0.0f;
	senseAccum_ = 0.0f;
	counterRotation_ = 0.0f;
}
```

`ContinueScrolling` changes: in the existing reverse-cone branch, after `scrollDirection_ *= -1.0f;` add `ResetRotation();` (a straight back-drag starts a fresh stroke). In the continue branch, call `UpdateRotation(newDir/static_cast<float>(newDir.Norm()))` — normalized, guarded by the branch's existing deadzone condition so degenerate zero-length vectors never reach it — immediately before `Scroll(newDir, newPos);`. (UpdateRotation's own flip already adjusted `scrollDirection_`; `Scroll` then applies it — no further handling needed on a `true` return, but add `LOG_DEBUG("Chirality flip: scrollDirection={}", scrollDirection_);` when it returns true; include Log.h.) Note: `atan2f`/`fabsf` need `<cmath>` (already included) and `std::max` needs `<algorithm>` (already included).

- [ ] **Step 4: docs.** README.txt Settings paragraph — append one sentence: `Advanced tuning keys in settings.ini's [Global Settings] section: scrollFlushMs (scroll event batching interval, 0 = off) and reverseRotationRad (how much counter-circling flips the scroll direction).` docs/BACKLOG.md — replace the header line `# Backlog` with `# Backlog` plus a `## Resolved (2026-09-06)` section at the bottom listing all 7 items as one-liners with their fix commits (fill hashes after committing), and delete the items' full sections.
- [ ] **Step 5: build + smoke** (usual dance, fresh instance left running).
- [ ] **Step 6: commit per item + docs:**
  - `feat: batch scroll events to reduce hover hot-tracking jitter (#1)` (Settings flushMs + WinScroller + Main)
  - `feat: detect circle-scroll direction change via rotation sense (#2)` (Settings reverseRotationRad + TouchSession)
  - `docs: note tuning keys and mark backlog items resolved`
- [ ] **Step 7: USER CHECKPOINT (controller-run):** user feel-tests jitter and reversal; tuning loop adjusts `scrollFlushMs` / `reverseRotationRad` in `settings.ini` (restart applies scrollFlushMs; reverseRotationRad also needs restart since sessions copy globals from the loaded settings) or code defaults until satisfied.

## Self-review notes

- Item interactions checked: batching sums the SAME ints ScrollSession already produces (sign flips flow through untouched); chirality changes only `scrollDirection_`, which `Scroll` already consumes; new GlobalSettings fields are appended so every aggregate init list updates in the same two places (kDefaultSettings, FromFile) — both specified. ScrollSession copies `GlobalSettings` by value at construction; new fields ride along automatically.
- Straight-line scrolling: angle ≈ 0 per update → sense never establishes → no behavior change; the reverse-cone path is untouched except the added reset.
- `scrollFlushMs = 0` reproduces the old per-event behavior exactly (interval 0 → every Scroll flushes).

## Unresolved questions

None — thresholds (0.9 rad flip, 0.6 rad establish, 15ms flush) are starting defaults; the user tunes by feel at Step 7.
