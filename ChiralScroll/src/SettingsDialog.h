#pragma once

#include <optional>
#include <Windows.h>

#include "Settings.h"

namespace chiralscroll
{

// Shows the modal settings dialog editing a copy of `settings`. Returns the
// edited copy if the user clicked Save, nullopt otherwise.
// TouchZoneCtrl::RegisterWindowClass must have been called first.
std::optional<Settings> ShowSettingsDialog(HINSTANCE hInstance, HWND owner, const Settings& settings);

}  // namespace chiralscroll
