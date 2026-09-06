#pragma once

#include "Scroller.h"

namespace chiralscroll
{

class WinScroller : public Scroller
{
public:
	enum class Direction { kVertical, kHorizontal };

	WinScroller(Direction dir) : dir_(dir) {}
	virtual ~WinScroller() { StopScrolling(); }

	void StartScrolling() override;
	void Scroll(int amt) override;
	void StopScrolling() override;

private:
	const Direction dir_;
};

}  // namespace chiralscroll
