#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef SendMessage

#include <winhttp.h>
#include <string>
#include <iostream>
#pragma comment(lib, "winhttp.lib")
#include "MatrixClient.h"

MatrixClient::MatrixClient(const std::wstring& homeserver)
    : m_homeserver(homeserver) {}

// --- Minimal JSON helpers ---
static std::string ExtractJsonValue(const std::string& json, const std::string& key) {
    std::string pattern = "\"" + key + "\":\"";
    auto pos = json.find(pattern);
    if (pos == std::string::npos) return {};
    pos += pattern.size();
    auto end = json.find('"', pos);
    if (end == std::string::npos) return {};
    return json.substr(pos, end - pos);
}

// --- Matrix methods ---
bool MatrixClient::Login(const std::string& user, const std::string& password) {
    std::string body = "{\"type\":\"m.login.password\",\"user\":\"" + user +
                       "\",\"password\":\"" + password + "\"}";
    auto resp = HttpRequest(L"POST", L"/_matrix/client/r0/login", body);
    if (resp.empty()) return false;

    m_accessToken = ExtractJsonValue(resp, "access_token");
    m_userId = ExtractJsonValue(resp, "user_id");
    return !m_accessToken.empty();
}

bool MatrixClient::JoinRoom(const std::string& roomIdOrAlias) {
    auto path = L"/_matrix/client/r0/join/" +
        std::wstring(roomIdOrAlias.begin(), roomIdOrAlias.end());
    auto resp = HttpRequest(L"POST", path, "{}", true);
    return !resp.empty();
}

bool MatrixClient::SendMessage(const std::string& roomId, const std::string& text) {
    std::string txnId = std::to_string(GetTickCount64());
    auto path = L"/_matrix/client/r0/rooms/" +
        std::wstring(roomId.begin(), roomId.end()) +
        L"/send/m.room.message/" +
        std::wstring(txnId.begin(), txnId.end());

    std::string body = "{\"msgtype\":\"m.text\",\"body\":\"" + text + "\"}";
    auto resp = HttpRequest(L"PUT", path, body, true);
    return !resp.empty();
}

void MatrixClient::Sync() {
    auto resp = HttpRequest(L"GET", L"/_matrix/client/r0/sync?timeout=1000", "", true);
    if (resp.empty()) return;

    // Very minimal parsing: just print the raw response
    std::cout << resp << std::endl;
}

// --- Internal HTTP helper ---
std::string MatrixClient::HttpRequest(const std::wstring& method,
                                      const std::wstring& path,
                                      const std::string& body,
                                      bool auth) {
    HINTERNET hSession = WinHttpOpen(L"MatrixClient/1.0",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return {};

    HINTERNET hConnect = WinHttpConnect(hSession, m_homeserver.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return {}; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, method.c_str(), path.c_str(),
                                            NULL, WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            WINHTTP_FLAG_SECURE);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return {}; }

    std::wstring headers;
    if (auth && !m_accessToken.empty()) {
        std::string token = "Authorization: Bearer " + m_accessToken;
        headers = std::wstring(token.begin(), token.end());
    }

    BOOL bResults = WinHttpSendRequest(hRequest,
                                       headers.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : headers.c_str(),
                                       (DWORD)-1L,
                                       body.empty() ? WINHTTP_NO_REQUEST_DATA : (LPVOID)body.data(),
                                       (DWORD)body.size(),
                                       (DWORD)body.size(),
                                       0);

    std::string result;
    if (bResults && WinHttpReceiveResponse(hRequest, NULL)) {
        DWORD dwSize = 0;
        do {
            DWORD dwDownloaded = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;

            if (dwSize == 0) break;
            std::string buffer(dwSize, 0);

            if (!WinHttpReadData(hRequest, &buffer[0], dwSize, &dwDownloaded)) break;
            buffer.resize(dwDownloaded);
            result += buffer;
        } while (dwSize > 0);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return result;
}
