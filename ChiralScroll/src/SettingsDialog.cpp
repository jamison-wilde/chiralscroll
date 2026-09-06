#include "SettingsDialog.h"

#include <format>
#include <optional>
#include <string>

#include <windowsx.h>

#include "resource.h"
#include "StringUtils.h"
#include "TouchZoneCtrl.h"

namespace chiralscroll
{

namespace
{

std::wstring GetItemText(HWND dlg, int id)
{
	HWND ctrl = GetDlgItem(dlg, id);
	const int length = GetWindowTextLengthW(ctrl);
	std::wstring text(length, L'\0');
	GetWindowTextW(ctrl, text.data(), length + 1);
	return text;
}

std::optional<float> ParseFloat(const std::wstring& str)
{
	try
	{
		return std::stof(str);
	}
	catch(const std::exception&)
	{
		return std::nullopt;
	}
}

std::optional<int> ParseInt(const std::wstring& str)
{
	try
	{
		return std::stoi(str);
	}
	catch(const std::exception&)
	{
		return std::nullopt;
	}
}

class DialogState
{
public:
	explicit DialogState(const Settings& settings) : settings_(settings) {}

	Settings& settings() { return settings_; }

	void OnInit(HWND dlg)
	{
		dlg_ = dlg;
		HWND combo = GetDlgItem(dlg_, IDC_DEVICE_SELECTOR);
		for(const auto& pair : settings_.GetDeviceSettings())
		{
			ComboBox_AddString(combo, StringToWstring(pair.first).c_str());
		}
		ComboBox_SetCurSel(combo, 0);
		LoadDevice();
	}

	void OnCommand(WPARAM wParam, LPARAM lParam)
	{
		const int id = LOWORD(wParam);
		const int code = HIWORD(wParam);
		switch(id)
		{
			case IDC_DEVICE_SELECTOR:
				if(code == CBN_SELCHANGE)
				{
					StoreFields();
					LoadDevice();
				}
				break;
			case IDC_ENABLE_DEVICE:
				if(code == BN_CLICKED && current_)
				{
					current_->enabled =
						IsDlgButtonChecked(dlg_, IDC_ENABLE_DEVICE) == BST_CHECKED;
					EnableControls(current_->enabled);
				}
				break;
			case IDC_TOUCH_ZONES:
				if(code == TZCN_CHANGED && current_)
				{
					const TouchZoneCtrl* ctrl =
						TouchZoneCtrl::FromHandle(reinterpret_cast<HWND>(lParam));
					current_->vScrollZone = ctrl->vZone();
					current_->hScrollZone = ctrl->hZone();
				}
				break;
			case IDOK:
				StoreFields();
				EndDialog(dlg_, 1);
				break;
			case IDCANCEL:
				EndDialog(dlg_, 0);
				break;
		}
	}

private:
	void LoadDevice()
	{
		HWND combo = GetDlgItem(dlg_, IDC_DEVICE_SELECTOR);
		const int selection = ComboBox_GetCurSel(combo);
		if(selection < 0)
		{
			// No touch devices.
			current_ = nullptr;
			CheckDlgButton(dlg_, IDC_ENABLE_DEVICE, BST_UNCHECKED);
			EnableWindow(GetDlgItem(dlg_, IDC_ENABLE_DEVICE), FALSE);
			EnableControls(false);
			return;
		}
		const int length = ComboBox_GetLBTextLen(combo, selection);
		std::wstring name(length, L'\0');
		ComboBox_GetLBText(combo, selection, name.data());
		current_ = &settings_.GetDeviceSettings(WstringToString(name));

		CheckDlgButton(dlg_, IDC_ENABLE_DEVICE,
			current_->enabled ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemTextW(dlg_, IDC_KEYBOARD_LOCKOUT,
			std::to_wstring(current_->typingLockoutMs).c_str());
		SetDlgItemTextW(dlg_, IDC_VERTICAL_SENS,
			std::format(L"{:.2f}", current_->vSens).c_str());
		SetDlgItemTextW(dlg_, IDC_HORIZONTAL_SENS,
			std::format(L"{:.2f}", current_->hSens).c_str());
		TouchZoneCtrl::FromHandle(GetDlgItem(dlg_, IDC_TOUCH_ZONES))
			->SetZones(current_->vScrollZone, current_->hScrollZone);
		EnableControls(current_->enabled);
	}

	// Parses the edit fields into the current device settings. A field that
	// fails to parse keeps its previous value.
	void StoreFields()
	{
		if(!current_)
		{
			return;
		}
		if(const auto value = ParseInt(GetItemText(dlg_, IDC_KEYBOARD_LOCKOUT)))
		{
			current_->typingLockoutMs = *value;
		}
		if(const auto value = ParseFloat(GetItemText(dlg_, IDC_VERTICAL_SENS)))
		{
			current_->vSens = *value;
		}
		if(const auto value = ParseFloat(GetItemText(dlg_, IDC_HORIZONTAL_SENS)))
		{
			current_->hSens = *value;
		}
	}

	void EnableControls(bool enable)
	{
		EnableWindow(GetDlgItem(dlg_, IDC_KEYBOARD_LOCKOUT), enable);
		EnableWindow(GetDlgItem(dlg_, IDC_VERTICAL_SENS), enable);
		EnableWindow(GetDlgItem(dlg_, IDC_HORIZONTAL_SENS), enable);
		EnableWindow(GetDlgItem(dlg_, IDC_TOUCH_ZONES), enable);
	}

	Settings settings_;
	Settings::DeviceSettings* current_ = nullptr;
	HWND dlg_ = nullptr;
};

INT_PTR CALLBACK DlgProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if(msg == WM_INITDIALOG)
	{
		SetWindowLongPtr(dlg, DWLP_USER, lParam);
		reinterpret_cast<DialogState*>(lParam)->OnInit(dlg);
		return TRUE;
	}
	DialogState* state = reinterpret_cast<DialogState*>(GetWindowLongPtr(dlg, DWLP_USER));
	if(state && msg == WM_COMMAND)
	{
		state->OnCommand(wParam, lParam);
		return TRUE;
	}
	return FALSE;
}

}  // namespace

std::optional<Settings> ShowSettingsDialog(HINSTANCE hInstance, HWND owner, const Settings& settings)
{
	DialogState state(settings);
	const INT_PTR result = DialogBoxParamW(
		hInstance, MAKEINTRESOURCE(IDD_SETTINGS), owner, &DlgProc,
		reinterpret_cast<LPARAM>(&state));
	if(result == 1)
	{
		return state.settings();
	}
	return std::nullopt;
}

}  // namespace chiralscroll
