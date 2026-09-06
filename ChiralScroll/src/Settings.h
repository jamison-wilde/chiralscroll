#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace chiralscroll
{

class Settings
{
public:
	struct GlobalSettings
	{
		// Whether the application is enabled.
		bool enabled;
		// How far to move after initial touch before scrolling starts.
		float startDeadzone;
		// To start scrolling, the contact point must move within a cone of this
		// size from the canonical direction (or it's reverse).
		float startDeadzoneAngle;
		// How far to move between each scroll interval.
		float moveDeadzone;
		// How far to move backwards before reversing scroll direction.
		float reverseDeadzone;
		// To reverse scrolling direction, the contact point must move backwards
		// within a cone of this size from the previous movement direction.
		float reverseDeadzoneAngle;
		// A scaling factor applied to sensitivity to make the vSens and hSens settings more convenient.
		float sensScalingFactor;
		// Accumulated counter-rotation (radians) before circle scrolling
		// flips direction.
		float reverseRotationRad;
		// Minimum interval between synthesized scroll events; pending deltas
		// accumulate between flushes. 0 sends every event immediately.
		int scrollFlushMs;
	};

	struct DeviceSettings
	{
		// Whether scrolling on this device is enabled.
		bool enabled;
		// Time to disable scrolling after a keyboard event in milliseconds.
		int typingLockoutMs;
		// The width of the touch zone to start vertical scrolling, as a fraction of
		// the horizontal width, measured from right edge.
		float vScrollZone;
		// The width of the touch zone to start horizontal scrolling, as a fraction
		// of the vertical width, measured from bottom.
		float hScrollZone;
		// Vertical scrolling sensitivity.
		float vSens;
		// Horizontal scrolling sensitivity.
		float hSens;
	};

	Settings() = default;
	Settings(const Settings&) = default;
	Settings& operator=(const Settings&) = default;

	static Settings FromFile(const std::filesystem::path& path, const std::vector<std::string>& devices);
	void ToFile(const std::filesystem::path& path) const;

	GlobalSettings& GetGlobalSettings();
	std::unordered_map<std::string, DeviceSettings>& GetDeviceSettings();
	DeviceSettings& GetDeviceSettings(std::string_view deviceName);

private:
	GlobalSettings globalSettings_;
	// Per-device settings.
	std::unordered_map<std::string, DeviceSettings> deviceSettings_;
};

}  // namespace chiralscroll
