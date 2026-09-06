#pragma once

#include <filesystem>
#include <format>
#include <optional>
#include <string_view>
#include <utility>

namespace chiralscroll::logging
{

enum class Level
{
	kTrace = 0,
	kDebug,
	kInfo,
	kWarn,
	kError,
	kCritical,
	kOff,
};

// Parses trace|debug|info|warn|err|critical|off (spdlog-compatible names).
std::optional<Level> ParseLevel(std::wstring_view name);

void SetLevel(Level level);
Level GetLevel();

// Route output to a file (truncated on open) or to a new console's stderr.
// Last call wins. Before either is called, output is dropped.
void InitFile(const std::filesystem::path& path);
void InitConsole();

// Writes one formatted line. Prefer the LOG_* macros.
void Write(Level level, std::string_view message);

template<typename... Args>
void Log(Level level, std::format_string<Args...> fmt, Args&&... args)
{
	if(level >= GetLevel())
	{
		Write(level, std::format(fmt, std::forward<Args>(args)...));
	}
}

}  // namespace chiralscroll::logging

#define LOG_TRACE(...) ::chiralscroll::logging::Log(::chiralscroll::logging::Level::kTrace, __VA_ARGS__)
#define LOG_DEBUG(...) ::chiralscroll::logging::Log(::chiralscroll::logging::Level::kDebug, __VA_ARGS__)
#define LOG_INFO(...) ::chiralscroll::logging::Log(::chiralscroll::logging::Level::kInfo, __VA_ARGS__)
#define LOG_WARN(...) ::chiralscroll::logging::Log(::chiralscroll::logging::Level::kWarn, __VA_ARGS__)
#define LOG_ERROR(...) ::chiralscroll::logging::Log(::chiralscroll::logging::Level::kError, __VA_ARGS__)
