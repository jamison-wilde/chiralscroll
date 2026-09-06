#pragma once

#include <string>
#include <string_view>

namespace chiralscroll
{

std::wstring StringToWstring(std::string_view str);
std::string WstringToString(std::wstring_view wstr);

}  // namespace chiralscroll
