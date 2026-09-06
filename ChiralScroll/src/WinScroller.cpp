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
	LOG_INFO("Start scrolling session.");
}

void WinScroller::Scroll(int amt)
{
	INPUT input{};

	input.type = INPUT_MOUSE;
	input.mi.dx = 0;
	input.mi.dy = 0;
	input.mi.mouseData = amt;
	input.mi.dwFlags = dir_ == Direction::kVertical ? MOUSEEVENTF_WHEEL : MOUSEEVENTF_HWHEEL;
	input.mi.time = 0;  //Windows will do the timestamp
	input.mi.dwExtraInfo = GetMessageExtraInfo();

	THROW_IF_FALSE(SendInput(1, &input, sizeof(INPUT)),
		GetErrorMessage(GetLastError()));
	LOG_DEBUG("Scroll by {} {}.",
		amt, dir_ == Direction::kVertical ? "vertical" : "horizontal");
}

void WinScroller::StopScrolling()
{
	ClipCursor(nullptr);
	LOG_INFO("Stop scrolling session.");
}

}  // namespace chiralscroll
