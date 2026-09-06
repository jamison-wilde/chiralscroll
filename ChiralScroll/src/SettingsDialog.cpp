#include "SettingsDialog.h"

namespace chiralscroll
{

SettingsDialog::SettingsDialog(
	wxWindow* parent,
	wxWindowID id,
	const wxString& title,
	const wxPoint& pos,
	const wxSize& size,
	long style)
	: wxDialog(parent, id, title, pos, size, style)
{
	SetSizeHints(wxDefaultSize, wxDefaultSize);

	wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);

	// Device selector row, right-aligned.
	wxBoxSizer* deviceSizer = new wxBoxSizer(wxHORIZONTAL);
	deviceSizer->Add(0, 0, 1, wxEXPAND, 5);
	deviceSelectorText_ = new wxStaticText(this, wxID_ANY, "Touch Device:");
	deviceSizer->Add(deviceSelectorText_, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
	deviceSelector_ = new wxComboBox(
		this, ID_DEVICE_SELECTOR, wxEmptyString, wxDefaultPosition, wxDefaultSize,
		0, nullptr, wxCB_DROPDOWN | wxCB_READONLY | wxCB_SORT);
	deviceSelector_->SetMinSize(wxSize(400, -1));
	deviceSelector_->SetMaxSize(wxSize(400, -1));
	deviceSizer->Add(deviceSelector_, 0, wxALL, 5);
	deviceSizer->Add(5, 0, 0, 0, 0);
	topSizer->Add(deviceSizer, 0, wxEXPAND, 5);

	// Device settings box: controls on the left, touchpad zone control on the right.
	wxStaticBoxSizer* settingsSizer = new wxStaticBoxSizer(
		new wxStaticBox(this, wxID_ANY, "Device Settings"), wxHORIZONTAL);
	wxWindow* box = settingsSizer->GetStaticBox();

	wxBoxSizer* controlsSizer = new wxBoxSizer(wxVERTICAL);
	enableDevice_ = new wxCheckBox(box, ID_ENABLE_DEVICE, "Enable");
	controlsSizer->Add(enableDevice_, 0, wxALL, 5);
	controlsSizer->Add(0, 0, 1, wxEXPAND, 5);
	keyboardLockoutMsText_ = new wxStaticText(box, wxID_ANY, "Keyboard Lockout (ms)");
	controlsSizer->Add(keyboardLockoutMsText_, 0, wxTOP | wxRIGHT | wxLEFT, 5);
	keyboardLockoutMs_ = new wxTextCtrl(box, wxID_ANY);
	controlsSizer->Add(keyboardLockoutMs_, 0, wxALL, 5);
	controlsSizer->Add(0, 0, 1, wxEXPAND, 5);
	verticalSensText_ = new wxStaticText(box, wxID_ANY, "Vertical Scrolling Speed");
	controlsSizer->Add(verticalSensText_, 0, wxTOP | wxRIGHT | wxLEFT, 5);
	verticalSens_ = new wxTextCtrl(box, ID_VERTICAL_SENS);
	controlsSizer->Add(verticalSens_, 0, wxALL, 5);
	controlsSizer->Add(0, 0, 1, wxEXPAND, 5);
	horizontalSensText_ = new wxStaticText(box, wxID_ANY, "Horizontal Scrolling Speed");
	controlsSizer->Add(horizontalSensText_, 0, wxTOP | wxRIGHT | wxLEFT, 5);
	horizontalSens_ = new wxTextCtrl(box, ID_HORIZONTAL_SENS);
	controlsSizer->Add(horizontalSens_, 0, wxALL, 5);
	settingsSizer->Add(controlsSizer, 0, wxEXPAND, 5);

	touchpadCtrl_ = new TouchpadCtrl(box);
	settingsSizer->Add(touchpadCtrl_, 1, wxALL | wxEXPAND, 5);
	topSizer->Add(settingsSizer, 1, wxEXPAND, 5);

	dialogOkCancel_ = new wxStdDialogButtonSizer();
	dialogOkCancelSave_ = new wxButton(this, wxID_SAVE);
	dialogOkCancel_->AddButton(dialogOkCancelSave_);
	dialogOkCancelCancel_ = new wxButton(this, wxID_CANCEL);
	dialogOkCancel_->AddButton(dialogOkCancelCancel_);
	dialogOkCancel_->Realize();
	topSizer->Add(dialogOkCancel_, 0, wxALL | wxEXPAND, 5);

	SetSizer(topSizer);
	Layout();
	Centre(wxBOTH);

	deviceSelector_->Bind(wxEVT_COMBOBOX, &SettingsDialog::OnSelectDevice, this);
	enableDevice_->Bind(wxEVT_CHECKBOX, &SettingsDialog::OnEnable, this);
	dialogOkCancelSave_->Bind(wxEVT_BUTTON, &SettingsDialog::OnSave, this);
}

}  // namespace chiralscroll
