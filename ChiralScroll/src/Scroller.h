#pragma once

namespace chiralscroll
{

class Scroller
{
public:
	virtual ~Scroller() = default;

	virtual void StartScrolling() = 0;
	// Scrolls by amt wheel units. Fractional amounts are the caller's real
	// computed value; implementations accumulate them rather than truncating,
	// so slow movement is not systematically lost.
	virtual void Scroll(double amt) = 0;
	virtual void StopScrolling() = 0;
};

}  // namespace chiralscroll
