#include <chrono>
#include <exception>
#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <Windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include <shellapi.h>
#include <hidusage.h>

#include "ChiralScroll.h"
#include "ChiralScrollException.h"
#include "HidUtils.h"
#include "Log.h"
#include "resource.h"
#include "Settings.h"
#include "SettingsDialog.h"
#include "StringUtils.h"
#include "TouchZoneCtrl.h"
#include "WinScroller.h"

namespace chiralscroll
{

namespace
{

constexpr wchar_t kWindowClass[] = L"ChiralScrollMain";
constexpr wchar_t kTitle[] = L"ChiralScroll";
constexpr UINT kTrayMessage = WM_APP + 1;

enum MenuId : UINT
{
	kMenuEnable = 1,
	kMenuSettings,
	kMenuClose,
};

struct CommandLineOptions
{
	bool logToConsole = false;
	bool panicOnUnexpectedInput = false;
	logging::Level logLevel = logging::Level::kWarn;
};

// Accepts -flag and --flag; --logLevel takes the next argument or =value.
// Returns nullopt (after showing usage) on a bad command line.
std::optional<CommandLineOptions> ParseCommandLine()
{
	CommandLineOptions options;
	int argc = 0;
	wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if(!argv)
	{
		return options;
	}
	bool ok = true;
	for(int i = 1; i < argc && ok; ++i)
	{
		std::wstring_view arg = argv[i];
		while(!arg.empty() && arg.front() == L'-')
		{
			arg.remove_prefix(1);
		}
		if(arg == L"logToConsole")
		{
			options.logToConsole = true;
		}
		else if(arg == L"panicOnUnexpectedInput")
		{
			options.panicOnUnexpectedInput = true;
		}
		else if(arg == L"logLevel" || arg.starts_with(L"logLevel="))
		{
			std::wstring_view value;
			if(arg.starts_with(L"logLevel="))
			{
				value = arg.substr(std::wstring_view(L"logLevel=").size());
			}
			else if(i + 1 < argc)
			{
				value = argv[++i];
			}
			const std::optional<logging::Level> level = logging::ParseLevel(value);
			if(level)
			{
				options.logLevel = *level;
			}
			else
			{
				ok = false;
			}
		}
		else
		{
			ok = false;
		}
	}
	LocalFree(argv);
	if(!ok)
	{
		MessageBoxW(nullptr,
			L"Usage: ChiralScroll [--logToConsole] [--panicOnUnexpectedInput]\n"
			L"                    [--logLevel trace|debug|info|warn|err|critical|off]",
			kTitle, MB_OK | MB_ICONERROR);
		return std::nullopt;
	}
	return options;
}

std::filesystem::path GetCurrentDirectoryPath()
{
	const DWORD size = ::GetCurrentDirectoryW(0, nullptr);
	std::wstring str(size, L'\0');
	::GetCurrentDirectoryW(size, str.data());
	str.resize(size - 1);
	return std::filesystem::path(str);
}

class App
{
public:
	App(HINSTANCE hInstance, const CommandLineOptions& options)
		: hInstance_(hInstance),
		  settingsPath_(GetCurrentDirectoryPath() / "settings.ini"),
		  touchDevices_(GetTouchDevices(options.panicOnUnexpectedInput)),
		  settings_(LoadSettings(settingsPath_, touchDevices_)),
		  chiralScroll_(
			settings_,
			std::make_unique<WinScroller>(WinScroller::Direction::kVertical),
			std::make_unique<WinScroller>(WinScroller::Direction::kHorizontal))
	{
		WNDCLASSW wc{};
		wc.lpfnWndProc = &App::WndProc;
		wc.hInstance = hInstance_;
		wc.hIcon = LoadIcon(hInstance_, MAKEINTRESOURCE(IDI_CHIRALSCROLL));
		wc.lpszClassName = kWindowClass;
		THROW_IF_FALSE(RegisterClassW(&wc), "RegisterClass failed.");

		// The window is never shown; it receives WM_INPUT and owns the tray menu.
		hwnd_ = CreateWindowExW(
			0, kWindowClass, kTitle, WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			nullptr, nullptr, hInstance_, this);
		THROW_IF_FALSE(hwnd_ != nullptr,
			std::format("CreateWindow failed: {}", GetErrorMessage(GetLastError())));

		const RAWINPUTDEVICE rid[]{
			{HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_KEYBOARD, RIDEV_INPUTSINK, hwnd_},
			{HID_USAGE_PAGE_DIGITIZER, HID_USAGE_DIGITIZER_TOUCH_PAD, RIDEV_INPUTSINK, hwnd_},
		};
		THROW_IF_FALSE(
			RegisterRawInputDevices(rid, sizeof(rid)/sizeof(RAWINPUTDEVICE), sizeof(RAWINPUTDEVICE)),
			std::format("RegisterRawInputDevices failed: {}", GetErrorMessage(GetLastError())));

		nid_.cbSize = sizeof(nid_);
		nid_.hWnd = hwnd_;
		nid_.uID = 1;
		nid_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
		nid_.uCallbackMessage = kTrayMessage;
		nid_.hIcon = LoadIcon(hInstance_, MAKEINTRESOURCE(IDI_CHIRALSCROLL));
		wcscpy_s(nid_.szTip, kTitle);
		THROW_IF_FALSE(Shell_NotifyIconW(NIM_ADD, &nid_), "Shell_NotifyIcon failed.");
		trayAdded_ = true;
	}

	~App()
	{
		if(trayAdded_)
		{
			Shell_NotifyIconW(NIM_DELETE, &nid_);
		}
	}

	int Run()
	{
		MSG msg{};
		while(GetMessageW(&msg, nullptr, 0, 0) > 0)
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		return static_cast<int>(msg.wParam);
	}

private:
	static Settings LoadSettings(
		const std::filesystem::path& path,
		const std::unordered_map<HANDLE, TouchDevice>& devices)
	{
		std::vector<std::string> names;
		names.reserve(devices.size());
		for(const auto& pair : devices)
		{
			names.push_back(std::string(pair.second.name()));
		}
		return Settings::FromFile(path, names);
	}

	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if(msg == WM_NCCREATE)
		{
			const CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
			SetWindowLongPtr(hwnd, GWLP_USERDATA,
				reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}
		App* app = reinterpret_cast<App*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		if(!app)
		{
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}
		try
		{
			return app->HandleMessage(hwnd, msg, wParam, lParam);
		}
		catch(const std::exception& e)
		{
			app->OnException(e);
			return 0;
		}
	}

	LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch(msg)
		{
			case WM_INPUT:
				return OnRawInput(hwnd, wParam, lParam);
			case kTrayMessage:
				if(lParam == WM_LBUTTONUP || lParam == WM_RBUTTONUP)
				{
					ShowTrayMenu();
				}
				return 0;
			case WM_COMMAND:
				OnMenuCommand(LOWORD(wParam));
				return 0;
			case WM_DESTROY:
				PostQuitMessage(0);
				return 0;
		}
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	LRESULT OnRawInput(HWND hwnd, WPARAM wParam, LPARAM lParam)
	{
		if(!stopped_)
		{
			auto maybeData = HidData::FromRawInput(reinterpret_cast<HRAWINPUT>(lParam));
			if(maybeData)
			{
				HandleHidInput(*maybeData);
			}
			else
			{
				// Input must have been keyboard.
				chiralScroll_.ProcessKeyboard();
			}
		}
		// When the application was in the foreground we must call
		// DefWindowProc for cleanup; same when input handling has stopped.
		if(stopped_ || GET_RAWINPUT_CODE_WPARAM(wParam) == RIM_INPUT)
		{
			return DefWindowProc(hwnd, WM_INPUT, wParam, lParam);
		}
		return 0;
	}

	void HandleHidInput(HidData& hidData)
	{
		const auto it = touchDevices_.find(hidData.header.hDevice);
		if(it == touchDevices_.end())
		{
			return;
		}
		const std::optional<std::vector<TouchDevice::Contact>> contacts =
			it->second.GetContacts(hidData);
		if(!contacts)
		{
			return;
		}
		chiralScroll_.ProcessTouch(it->second, *contacts);
	}

	void ShowTrayMenu()
	{
		HMENU menu = CreatePopupMenu();
		AppendMenuW(menu,
			MF_STRING | (settings_.GetGlobalSettings().enabled ? MF_CHECKED : MF_UNCHECKED),
			kMenuEnable, L"Enable");
		AppendMenuW(menu, MF_STRING | (dialogOpen_ ? MF_GRAYED : 0), kMenuSettings, L"Settings");
		AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
		AppendMenuW(menu, MF_STRING | (dialogOpen_ ? MF_GRAYED : 0), kMenuClose, L"Close");
		POINT pt;
		GetCursorPos(&pt);
		// Required so the menu dismisses when clicking elsewhere.
		SetForegroundWindow(hwnd_);
		TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd_, nullptr);
		PostMessage(hwnd_, WM_NULL, 0, 0);
		DestroyMenu(menu);
	}

	void OnMenuCommand(UINT id)
	{
		switch(id)
		{
			case kMenuEnable:
				settings_.GetGlobalSettings().enabled = !settings_.GetGlobalSettings().enabled;
				chiralScroll_.SetSettings(settings_);
				break;
			case kMenuSettings:
			{
				if(dialogOpen_)
				{
					break;
				}
				dialogOpen_ = true;
				const std::optional<Settings> result =
					ShowSettingsDialog(hInstance_, hwnd_, settings_);
				dialogOpen_ = false;
				if(result)
				{
					settings_ = *result;
					chiralScroll_.SetSettings(settings_);
					settings_.ToFile(settingsPath_);
				}
				break;
			}
			case kMenuClose:
				if(dialogOpen_)
				{
					break;
				}
				DestroyWindow(hwnd_);
				break;
		}
	}

	void OnException(const std::exception& e)
	{
		stopped_ = true;
		const std::string message = std::format("Caught exception: {}", e.what());
		LOG_ERROR("{}", message);
		MessageBoxW(nullptr, StringToWstring(message).c_str(),
			L"ChiralScroll Error", MB_OK | MB_ICONERROR);
	}

	const HINSTANCE hInstance_;
	HWND hwnd_ = nullptr;
	NOTIFYICONDATAW nid_{};
	bool trayAdded_ = false;
	std::filesystem::path settingsPath_;
	std::unordered_map<HANDLE, TouchDevice> touchDevices_;
	Settings settings_;
	ChiralScroll chiralScroll_;
	bool stopped_ = false;
	bool dialogOpen_ = false;
};

}  // namespace

}  // namespace chiralscroll

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{
	using namespace chiralscroll;

	const std::optional<CommandLineOptions> options = ParseCommandLine();
	if(!options)
	{
		return 1;
	}
	if(options->logToConsole)
	{
		logging::InitConsole();
	}
	else
	{
		logging::InitFile(GetCurrentDirectoryPath() / "chiralscroll.log");
	}
	logging::SetLevel(options->logLevel);

	INITCOMMONCONTROLSEX icc{sizeof(icc), ICC_STANDARD_CLASSES};
	InitCommonControlsEx(&icc);
	TouchZoneCtrl::RegisterWindowClass(hInstance);

	try
	{
		App app(hInstance, *options);
		return app.Run();
	}
	catch(const std::exception& e)
	{
		const std::string message = std::format("Caught exception: {}", e.what());
		LOG_ERROR("{}", message);
		MessageBoxW(nullptr, StringToWstring(message).c_str(),
			L"ChiralScroll Error", MB_OK | MB_ICONERROR);
		return 1;
	}
}
