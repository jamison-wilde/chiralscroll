#pragma once

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/combobox.h>
#include <wx/dialog.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include "TouchpadCtrl.h"

namespace chiralscroll
{

// Settings dialog layout. Hand-written replacement for the code previously
// generated from formbuilder/ChiralScroll.fbp by wxFormBuilder; the .fbp is
// kept as the original design reference but is no longer part of the build.
// Subclass and override the On* handlers (see SettingsDialogImpl in Main.cpp).
class SettingsDialog : public wxDialog
{
public:
	SettingsDialog(
		wxWindow* parent,
		wxWindowID id = wxID_ANY,
		const wxString& title = "ChiralScroll Settings",
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxSize(620, 385),
		long style = wxDEFAULT_DIALOG_STYLE);

protected:
	enum
	{
		ID_DEVICE_SELECTOR = 1000,
		ID_ENABLE_DEVICE,
		ID_VERTICAL_SENS,
		ID_HORIZONTAL_SENS,
	};

	virtual void OnSelectDevice(wxCommandEvent& event) { event.Skip(); }
	virtual void OnEnable(wxCommandEvent& event) { event.Skip(); }
	virtual void OnSave(wxCommandEvent& event) { event.Skip(); }

	wxStaticText* deviceSelectorText_;
	wxComboBox* deviceSelector_;
	wxCheckBox* enableDevice_;
	wxStaticText* keyboardLockoutMsText_;
	wxTextCtrl* keyboardLockoutMs_;
	wxStaticText* verticalSensText_;
	wxTextCtrl* verticalSens_;
	wxStaticText* horizontalSensText_;
	wxTextCtrl* horizontalSens_;
	TouchpadCtrl* touchpadCtrl_;
	wxStdDialogButtonSizer* dialogOkCancel_;
	wxButton* dialogOkCancelSave_;
	wxButton* dialogOkCancelCancel_;
};

}  // namespace chiralscroll
