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
	void Scroll(double amt) override;
	void StopScrolling() override;

private:
	void Flush();

	const Direction dir_;
	const std::chrono::steady_clock::duration interval_;
	// Accumulated wheel units awaiting a flush. Kept fractional: only whole
	// units are sent, and the remainder carries to the next flush so slow
	// movement is not rounded away.
	double pending_ = 0.0;
	std::chrono::steady_clock::time_point lastFlush_{};
};

}  // namespace chiralscroll
