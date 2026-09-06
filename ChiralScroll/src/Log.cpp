#include "Log.h"

#include <cstdio>
#include <Windows.h>

namespace chiralscroll::logging
{

namespace
{

// All logging happens on the UI thread; no synchronization needed.
Level g_level = Level::kWarn;
FILE* g_out = nullptr;

constexpr const char* LevelName(Level level)
{
	switch(level)
	{
		case Level::kTrace: return "trace";
		case Level::kDebug: return "debug";
		case Level::kInfo: return "info";
		case Level::kWarn: return "warn";
		case Level::kError: return "error";
		case Level::kCritical: return "critical";
		default: return "?";
	}
}

}  // namespace

std::optional<Level> ParseLevel(std::wstring_view name)
{
	if(name == L"trace") return Level::kTrace;
	if(name == L"debug") return Level::kDebug;
	if(name == L"info") return Level::kInfo;
	if(name == L"warn") return Level::kWarn;
	if(name == L"err") return Level::kError;
	if(name == L"critical") return Level::kCritical;
	if(name == L"off") return Level::kOff;
	return std::nullopt;
}

void SetLevel(Level level)
{
	g_level = level;
}

Level GetLevel()
{
	return g_level;
}

void InitFile(const std::filesystem::path& path)
{
	if(g_out && g_out != stderr)
	{
		fclose(g_out);
	}
	_wfopen_s(&g_out, path.c_str(), L"w");
}

void InitConsole()
{
	if(g_out && g_out != stderr)
	{
		fclose(g_out);
	}
	AllocConsole();
	FILE* reopened = nullptr;
	freopen_s(&reopened, "CONOUT$", "w", stderr);
	g_out = stderr;
}

void Write(Level level, std::string_view message)
{
	if(!g_out)
	{
		return;
	}
	SYSTEMTIME st;
	GetLocalTime(&st);
	fprintf(g_out, "[%02u:%02u:%02u.%03u] [%s] %.*s\n",
		st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
		LevelName(level), static_cast<int>(message.size()), message.data());
	fflush(g_out);
}

}  // namespace chiralscroll::logging
