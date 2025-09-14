#pragma once
#include <string>

inline std::wstring ToWString(const std::string& s) {
    return std::wstring(s.begin(), s.end());
}

inline std::string ToString(const std::wstring& ws) {
    return std::string(ws.begin(), ws.end());
}
