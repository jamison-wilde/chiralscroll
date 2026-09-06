#pragma once

#include <Windows.h>

namespace chiralscroll
{

// Window class name, usable in dialog templates. RegisterWindowClass must be
// called before any window using it is created.
inline constexpr wchar_t kTouchZoneCtrlClass[] = L"ChiralTouchZoneCtrl";

// WM_COMMAND notification code sent to the parent when a grabber drag changes
// a zone value.
inline constexpr WORD TZCN_CHANGED = 1;

// Draws the touchpad as a grey rounded rectangle with the vertical scroll
// zone (green hatch, from the right edge) and horizontal scroll zone (red
// hatch, from the bottom edge) plus two draggable grabber squares. Zones are
// fractions in [0, 1].
class TouchZoneCtrl
{
public:
	static void RegisterWindowClass(HINSTANCE hInstance);
	static TouchZoneCtrl* FromHandle(HWND hwnd);

	void SetZones(float vZone, float hZone);
	float vZone() const { return vZone_; }
	float hZone() const { return hZone_; }

private:
	explicit TouchZoneCtrl(HWND hwnd) : hwnd_(hwnd) {}

	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

	void Paint(HDC dc, const RECT& client) const;
	POINT VGrabberPos(const RECT& client) const;
	POINT HGrabberPos(const RECT& client) const;
	void NotifyParent() const;

	enum class Drag { kNone, kVertical, kHorizontal };

	Drag HitTest(POINT pt, const RECT& client) const;

	// Scales a design-time (96 DPI) metric to the control's current DPI.
	int Scaled(int value) const;

	// Grabber square size matches the old wx control.
	static constexpr int kGrabberSize = 7;
	static constexpr int kHitSlop = 5;
	static constexpr int kCornerSize = 13;

	HWND hwnd_;
	float vZone_ = 0.0f;
	float hZone_ = 0.0f;
	Drag drag_ = Drag::kNone;
};

}  // namespace chiralscroll
