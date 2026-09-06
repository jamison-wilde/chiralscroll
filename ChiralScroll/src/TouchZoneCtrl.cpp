#include "TouchZoneCtrl.h"

#include <algorithm>
#include <windowsx.h>

#include "Log.h"

namespace chiralscroll
{

namespace
{

bool NearPoint(POINT pt, POINT target, int slop)
{
	return pt.x >= target.x - slop && pt.x <= target.x + slop &&
	       pt.y >= target.y - slop && pt.y <= target.y + slop;
}

}  // namespace

void TouchZoneCtrl::RegisterWindowClass(HINSTANCE hInstance)
{
	WNDCLASSW wc{};
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = &TouchZoneCtrl::WndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
	wc.lpszClassName = kTouchZoneCtrlClass;
	if(!RegisterClassW(&wc))
	{
		LOG_ERROR("TouchZoneCtrl RegisterClassW failed: {}", GetLastError());
	}
}

TouchZoneCtrl* TouchZoneCtrl::FromHandle(HWND hwnd)
{
	return reinterpret_cast<TouchZoneCtrl*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
}

void TouchZoneCtrl::SetZones(float vZone, float hZone)
{
	vZone_ = std::clamp(vZone, 0.0f, 1.0f);
	hZone_ = std::clamp(hZone, 0.0f, 1.0f);
	InvalidateRect(hwnd_, nullptr, FALSE);
}

LRESULT CALLBACK TouchZoneCtrl::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if(msg == WM_NCCREATE)
	{
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(new TouchZoneCtrl(hwnd)));
		return TRUE;
	}
	TouchZoneCtrl* ctrl = FromHandle(hwnd);
	if(!ctrl)
	{
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	if(msg == WM_NCDESTROY)
	{
		SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
		delete ctrl;
		return 0;
	}
	return ctrl->HandleMessage(msg, wParam, lParam);
}

int TouchZoneCtrl::Scaled(int value) const
{
	return MulDiv(value, static_cast<int>(GetDpiForWindow(hwnd_)), 96);
}

TouchZoneCtrl::Drag TouchZoneCtrl::HitTest(POINT pt, const RECT& client) const
{
	if(NearPoint(pt, VGrabberPos(client), Scaled(kGrabberSize)/2 + Scaled(kHitSlop)))
	{
		return Drag::kVertical;
	}
	if(NearPoint(pt, HGrabberPos(client), Scaled(kGrabberSize)/2 + Scaled(kHitSlop)))
	{
		return Drag::kHorizontal;
	}
	return Drag::kNone;
}

LRESULT TouchZoneCtrl::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
		case WM_PAINT:
			OnPaint();
			return 0;
		case WM_LBUTTONDOWN:
		{
			RECT client;
			GetClientRect(hwnd_, &client);
			const POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			drag_ = HitTest(pt, client);
			if(drag_ != Drag::kNone)
			{
				SetCapture(hwnd_);
			}
			return 0;
		}
		case WM_MOUSEMOVE:
		{
			if(drag_ == Drag::kNone)
			{
				return 0;
			}
			RECT client;
			GetClientRect(hwnd_, &client);
			const int width = client.right - client.left;
			const int height = client.bottom - client.top;
			if(drag_ == Drag::kVertical && width > 0)
			{
				vZone_ = std::clamp(1.0f - static_cast<float>(GET_X_LPARAM(lParam))/width, 0.0f, 1.0f);
			}
			else if(drag_ == Drag::kHorizontal && height > 0)
			{
				hZone_ = std::clamp(1.0f - static_cast<float>(GET_Y_LPARAM(lParam))/height, 0.0f, 1.0f);
			}
			InvalidateRect(hwnd_, nullptr, FALSE);
			NotifyParent();
			return 0;
		}
		case WM_LBUTTONUP:
			if(drag_ != Drag::kNone)
			{
				ReleaseCapture();
			}
			drag_ = Drag::kNone;
			return 0;
		case WM_CAPTURECHANGED:
			drag_ = Drag::kNone;
			return 0;
		case WM_SETCURSOR:
		{
			POINT pt;
			GetCursorPos(&pt);
			ScreenToClient(hwnd_, &pt);
			RECT client;
			GetClientRect(hwnd_, &client);
			const Drag hit = drag_ != Drag::kNone ? drag_ : HitTest(pt, client);
			if(hit == Drag::kVertical)
			{
				SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
			}
			else if(hit == Drag::kHorizontal)
			{
				SetCursor(LoadCursor(nullptr, IDC_SIZENS));
			}
			else
			{
				SetCursor(LoadCursor(nullptr, IDC_ARROW));
			}
			return TRUE;
		}
		case WM_ENABLE:
			InvalidateRect(hwnd_, nullptr, TRUE);
			return 0;
	}
	return DefWindowProc(hwnd_, msg, wParam, lParam);
}

void TouchZoneCtrl::OnPaint() const
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
}

void TouchZoneCtrl::Paint(HDC dc, const RECT& client) const
{
	const int width = client.right - client.left;
	const int height = client.bottom - client.top;

	HRGN clipRgn = CreateRoundRectRgn(0, 0, width + 1, height + 1, Scaled(kCornerSize), Scaled(kCornerSize));
	SelectClipRgn(dc, clipRgn);

	HBRUSH padBrush = CreateSolidBrush(RGB(128, 128, 128));
	FillRect(dc, &client, padBrush);

	if(IsWindowEnabled(hwnd_))
	{
		const int vEdge = static_cast<int>(width*(1.0f - vZone_));
		const int hEdge = static_cast<int>(height*(1.0f - hZone_));
		const int oldBkMode = SetBkMode(dc, TRANSPARENT);

		if(hZone_ > 0)
		{
			HBRUSH hatch = CreateHatchBrush(HS_BDIAGONAL, RGB(200, 0, 0));
			RECT zone{0, hEdge, vEdge, height};
			FillRect(dc, &zone, hatch);
			DeleteObject(hatch);
			HPEN pen = CreatePen(PS_SOLID, 1, RGB(200, 0, 0));
			HGDIOBJ oldPen = SelectObject(dc, pen);
			MoveToEx(dc, 0, hEdge, nullptr);
			LineTo(dc, vEdge, hEdge);
			SelectObject(dc, oldPen);
			DeleteObject(pen);
		}
		if(vZone_ > 0)
		{
			HBRUSH hatch = CreateHatchBrush(HS_FDIAGONAL, RGB(0, 160, 0));
			RECT zone{vEdge, 0, width, height};
			FillRect(dc, &zone, hatch);
			DeleteObject(hatch);
			HPEN pen = CreatePen(PS_SOLID, 1, RGB(0, 160, 0));
			HGDIOBJ oldPen = SelectObject(dc, pen);
			MoveToEx(dc, vEdge, 0, nullptr);
			LineTo(dc, vEdge, height);
			SelectObject(dc, oldPen);
			DeleteObject(pen);
		}
		SetBkMode(dc, oldBkMode);

		HBRUSH black = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
		const POINT v = VGrabberPos(client);
		const POINT h = HGrabberPos(client);
		const int grabberSize = Scaled(kGrabberSize);
		RECT vg{v.x - grabberSize/2, v.y - grabberSize/2, v.x + grabberSize/2 + 1, v.y + grabberSize/2 + 1};
		RECT hg{h.x - grabberSize/2, h.y - grabberSize/2, h.x + grabberSize/2 + 1, h.y + grabberSize/2 + 1};
		FillRect(dc, &vg, black);
		FillRect(dc, &hg, black);
	}

	DeleteObject(padBrush);
	SelectClipRgn(dc, nullptr);
	DeleteObject(clipRgn);
}

POINT TouchZoneCtrl::VGrabberPos(const RECT& client) const
{
	const int width = client.right - client.left;
	const int height = client.bottom - client.top;
	return {static_cast<LONG>(width*(1.0f - vZone_)), height/2};
}

POINT TouchZoneCtrl::HGrabberPos(const RECT& client) const
{
	const int width = client.right - client.left;
	const int height = client.bottom - client.top;
	return {width/2, static_cast<LONG>(height*(1.0f - hZone_))};
}

void TouchZoneCtrl::NotifyParent() const
{
	HWND parent = GetParent(hwnd_);
	if(parent)
	{
		SendMessage(parent, WM_COMMAND,
			MAKEWPARAM(GetDlgCtrlID(hwnd_), TZCN_CHANGED),
			reinterpret_cast<LPARAM>(hwnd_));
	}
}

}  // namespace chiralscroll
