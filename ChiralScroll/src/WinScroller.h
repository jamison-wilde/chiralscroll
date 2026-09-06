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
