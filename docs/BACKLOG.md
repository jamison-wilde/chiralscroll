# Backlog

Post-rewrite behavior improvements. NOT part of the 2026-09-05 Win32 rewrite
(that plan is behavior-parity only; scroll math is frozen there so regressions
stay attributable). Both confirmed by user testing of the wx baseline
2026-09-05, i.e. they are pre-existing behavior, not rewrite regressions.

## 1. Scroll event flood causes hover hot-tracking jitter

**File:** `ChiralScroll/src/WinScroller.cpp` (`Scroll`), call path
`ScrollSession::Scroll` → `Scroller::Scroll`.

`SendInput(MOUSEEVENTF_WHEEL)` fires once per touchpad HID report
(~125/s, small deltas). Hover hot-tracking UIs (Explorer file tree folder
selection under the pinned cursor) re-evaluate per discrete wheel event, so
the selection jumps around far more than with Precision two-finger scrolling,
which coalesces/smooths in the driver stack.

**Direction:** accumulate signed deltas in `WinScroller` and flush on a
fixed cadence (~60Hz timer or elapsed-time check in `Scroll`), dropping
sub-threshold flushes. Consider making the cadence a Global setting.

## 2. Circle-scroll reversal (chirality change) sometimes missed

**File:** `ChiralScroll/src/TouchSession.cpp` (`ScrollSession::ContinueScrolling`).

Reversal is detected only when the new movement vector falls within
`reverseDeadzoneAngle/2` of the exact opposite of the previous movement
direction (`direction_`). Rotation sense is never tracked. When the user
switches CW↔CCW mid-circle, the transition movement is tangential, never
enters the reverse cone, and `direction_` incrementally chases the new curve —
scrolling continues with the old sign.

**Direction:** track chirality explicitly: per update compute the
perpendicular dot product `direction_.x()*newDir.y() - direction_.y()*newDir.x()`
(z of the cross product; sign = rotation sense). Accumulate signed rotation and
flip `scrollDirection_` when the sense is consistently opposite for a few
frames / a threshold of accumulated radians. Keep the existing straight-line
reverse-cone test for non-circular back-drags. CW/CCW must be distinguishable
even when the reversal transition is geometrically ambiguous for a frame or
two.
