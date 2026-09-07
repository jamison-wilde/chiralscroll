#include "WinScroller.h"

#include <Windows.h>

#include "ChiralScrollException.h"
#include "Log.h"

namespace chiralscroll
{

void WinScroller::StartScrolling()
{
	POINT p;
	GetCursorPos(&p);
	RECT r = {p.x, p.y, p.x + 1, p.y + 1};
	ClipCursor(&r);
	pending_ = 0.0;
	lastFlush_ = std::chrono::steady_clock::now() - interval_;
	LOG_INFO("Start scrolling session.");
}

void WinScroller::Scroll(double amt)
{
	pending_ += amt;
	const auto now = std::chrono::steady_clock::now();
	if(now - lastFlush_ >= interval_)
	{
		Flush();
		lastFlush_ = now;
	}
}

void WinScroller::Flush()
{
	// Send whole units only; the sub-unit remainder stays in pending_ for the
	// next flush rather than being rounded away.
	const int amount = static_cast<int>(pending_);
	if(amount == 0)
	{
		return;
	}

	INPUT input{};

	input.type = INPUT_MOUSE;
	input.mi.dx = 0;
	input.mi.dy = 0;
	input.mi.mouseData = static_cast<DWORD>(amount);
	input.mi.dwFlags = dir_ == Direction::kVertical ? MOUSEEVENTF_WHEEL : MOUSEEVENTF_HWHEEL;
	input.mi.time = 0;  //Windows will do the timestamp
	input.mi.dwExtraInfo = GetMessageExtraInfo();

	if(!SendInput(1, &input, sizeof(INPUT)))
	{
		// Flush runs from destructors; never throw. Injection can fail
		// transiently (e.g. secure desktop), so log and drop the batch.
		LOG_ERROR("SendInput failed: {}", GetErrorMessage(GetLastError()));
	}
	else
	{
		LOG_DEBUG("Scroll by {} {}.",
			amount, dir_ == Direction::kVertical ? "vertical" : "horizontal");
	}
	pending_ -= amount;
}

void WinScroller::StopScrolling()
{
	Flush();
	ClipCursor(nullptr);
	LOG_INFO("Stop scrolling session.");
}

}  // namespace chiralscroll
