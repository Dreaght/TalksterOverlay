#include "MatrixClient.h"
#include <windows.h>
#include <shellapi.h>
#include <winhttp.h>
#include <string>
#include <sstream>
#include <thread>
#include <future>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winhttp.lib")

inline void ShowError(const std::wstring& title, const std::wstring& msg) {
    MessageBoxW(NULL, msg.c_str(), title.c_str(), MB_ICONERROR | MB_OK);
}

// ------------------ Constructor / Destructor ------------------
MatrixClient::MatrixClient(const std::wstring& homeserver)
    : m_homeserver(homeserver) {}

MatrixClient::~MatrixClient() { Stop(); }

// ------------------ JSON Helper ------------------
std::string MatrixClient::ExtractJsonValue(const std::string& json, const std::string& key) {
    std::string pattern = "\"" + key + "\":\"";
    auto pos = json.find(pattern);
    if (pos == std::string::npos) return {};
    pos += pattern.size();
    auto end = json.find('"', pos);
    if (end == std::string::npos) return {};
    return json.substr(pos, end - pos);
}

inline void ShowInfo(const std::wstring& title, const std::wstring& msg) {
    MessageBoxW(NULL, msg.c_str(), title.c_str(), MB_ICONINFORMATION | MB_OK);
}

void MatrixClient::SetOnLogin(LoginCallback cb) {
    onLogin_ = std::move(cb);
}

// ------------------ Helper: SSO login + callback ------------------
bool MatrixClient::PerformLogin() {
    ShowInfo(L"Login", L"Starting SSO login...");

    auto tokenOpt = RunLocalSSOListener();
    if (!tokenOpt) {
        ShowError(L"SSO Error", L"Failed to receive SSO login token.");
        if (onLogin_) onLogin_(false);
        return false;
    }

    ShowInfo(L"Login", L"Got login token, exchanging for access token...");

    std::string loginToken = *tokenOpt;
    std::string body = "{\"type\":\"m.login.token\",\"token\":\"" + loginToken + "\"}";
    auto resp = HttpRequest(L"POST", L"/_matrix/client/r0/login", body);

    if (resp.empty()) {
        ShowError(L"Login Error", L"Failed to exchange token for access token.");
        if (onLogin_) onLogin_(false);
        return false;
    }

    m_accessToken = ExtractJsonValue(resp, "access_token");
    m_userId = ExtractJsonValue(resp, "user_id");

    if (m_accessToken.empty()) {
        ShowError(L"Login Error", L"Access token not found in response.");
        if (onLogin_) onLogin_(false);
        return false;
    }

    ShowInfo(L"Login", L"Login successful. User ID: " + std::wstring(m_userId.begin(), m_userId.end()));

    // Fire callback for regular login (without room)
    if (onLogin_) onLogin_(true);

    return true;
}

// ------------------ Async SSO Login ------------------
std::future<bool> MatrixClient::LoginWithSSOAsync() {
    return std::async(std::launch::async, [this]() -> bool {
        return PerformLogin(); // only login + callback
    });
}

// ------------------ Async SSO Login + Room ------------------
std::future<bool> MatrixClient::LoginWithSSOAndRandomRoomAsync() {
    return std::async(std::launch::async, [this]() -> bool {
        ShowInfo(L"Login", L"Starting SSO login + room join...");

        if (!PerformLogin()) return false; // login + callback already done

        ShowInfo(L"Login", L"Creating or joining a random room...");

        std::string randomRoomAlias =
            "room_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());

        std::string createBody =
            "{\"room_alias_name\":\"" + randomRoomAlias + "\", \"visibility\":\"private\"}";
        auto createResp = HttpRequest(L"POST", L"/_matrix/client/r0/createRoom", createBody, true);

        std::string roomId = ExtractJsonValue(createResp, "room_id");
        bool joined = false;

        if (roomId.empty()) {
            ShowInfo(L"Login", L"Room creation failed, trying to join alias instead.");
            joined = JoinRoom("#" + randomRoomAlias + ":matrix.org");
            m_currentRoomId = "#" + randomRoomAlias + ":matrix.org";
        } else {
            ShowInfo(L"Login", L"Room created. Joining room ID: " + std::wstring(roomId.begin(), roomId.end()));
            joined = JoinRoom(roomId);
            m_currentRoomId = roomId;
        }

        // Fire callback again to notify the user about room join result
        if (onLogin_) onLogin_(joined);

        return joined;
    });
}



// ------------------ Join / Send ------------------
bool MatrixClient::JoinRoom(const std::string& roomIdOrAlias) {
    std::wstring path = L"/_matrix/client/r0/join/" +
        std::wstring(roomIdOrAlias.begin(), roomIdOrAlias.end());
    auto resp = HttpRequest(L"POST", path, "{}", true);
    if (resp.empty()) {
        ShowError(L"Join Room Error", L"Failed to join room: " + std::wstring(roomIdOrAlias.begin(), roomIdOrAlias.end()));
        return false;
    }
    return true;
}

void MatrixClient::SendMessageAsync(const std::string& roomId, const std::string& text) {
    std::thread([this, roomId, text]() {
        // ShowInfo(L"SendMessageAsync", L"Preparing to send message to room: " + std::wstring(roomId.begin(), roomId.end()));

        std::string txnId = std::to_string(::GetTickCount64());
        std::wstring path = L"/_matrix/client/r0/rooms/" +
            std::wstring(roomId.begin(), roomId.end()) +
            L"/send/m.room.message/" +
            std::wstring(txnId.begin(), txnId.end());

        // ShowInfo(L"SendMessageAsync", L"HTTP PUT path: " + path);

        std::string body = "{\"msgtype\":\"m.text\",\"body\":\"" + text + "\"}";
        auto resp = HttpRequest(L"PUT", path, body, true);

        if (resp.empty()) {
            ShowError(L"Send Message Error", L"Failed to send message to room: " +
                      std::wstring(roomId.begin(), roomId.end()));
        } else {
            // ShowInfo(L"SendMessageAsync", L"Message sent successfully: " + std::wstring(text.begin(), text.end()));
        }
    }).detach();
}

// ------------------ Sync ------------------
void MatrixClient::Start() {
    if (m_running) return;
    m_running = true;
    m_thread = std::thread(&MatrixClient::SyncLoop, this);
}

void MatrixClient::Stop() {
    if (!m_running) return;
    m_running = false;
    if (m_thread.joinable())
        m_thread.join();
}

void MatrixClient::SyncLoop() {
    while (m_running) {
        SyncOnce();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void MatrixClient::SyncOnce() {
    auto resp = HttpRequest(L"GET", L"/_matrix/client/r0/sync?timeout=30000", "", true);
    if (resp.empty()) return;

    size_t pos = 0;
    while ((pos = resp.find("\"msgtype\":\"m.text\"", pos)) != std::string::npos) {
        try {
            size_t roomPos = resp.rfind("\"room_id\":\"", pos);
            if (roomPos == std::string::npos) { pos += 12; continue; }
            roomPos += 10;
            size_t roomEnd = resp.find('"', roomPos);
            if (roomEnd == std::string::npos) { pos += 12; continue; }
            std::string roomId = resp.substr(roomPos, roomEnd - roomPos);

            size_t bodyPos = resp.find("\"body\":\"", pos);
            if (bodyPos == std::string::npos) { pos = roomEnd; continue; }
            bodyPos += 8;
            size_t bodyEnd = resp.find('"', bodyPos);
            if (bodyEnd == std::string::npos) { pos = bodyPos; continue; }
            std::string body = resp.substr(bodyPos, bodyEnd - bodyPos);

            if (m_onMessage) m_onMessage(roomId, body);

            pos = bodyEnd;
        } catch (...) { pos++; }
    }
}

// ------------------ HTTP ------------------
std::string MatrixClient::HttpRequest(const std::wstring& method,
                                     const std::wstring& path,
                                     const std::string& body,
                                     bool auth) {
    HINTERNET hSession = ::WinHttpOpen(L"MatrixClient/1.0",
                                      WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                      WINHTTP_NO_PROXY_NAME,
                                      WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) { ShowError(L"HTTP Error", L"Failed to open WinHTTP session."); return {}; }

    HINTERNET hConnect = ::WinHttpConnect(hSession, m_homeserver.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { ::WinHttpCloseHandle(hSession); ShowError(L"HTTP Error", L"Failed to connect to server."); return {}; }

    HINTERNET hRequest = ::WinHttpOpenRequest(hConnect, method.c_str(), path.c_str(),
                                             NULL, WINHTTP_NO_REFERER,
                                             WINHTTP_DEFAULT_ACCEPT_TYPES,
                                             WINHTTP_FLAG_SECURE);
    if (!hRequest) { ::WinHttpCloseHandle(hConnect); ::WinHttpCloseHandle(hSession); ShowError(L"HTTP Error", L"Failed to create HTTP request."); return {}; }

    std::wstring headers;
    if (auth && !m_accessToken.empty()) {
        std::string token = "Authorization: Bearer " + m_accessToken;
        headers = std::wstring(token.begin(), token.end());
    }

    BOOL bResults = ::WinHttpSendRequest(hRequest,
                                         headers.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : headers.c_str(),
                                         (DWORD)-1L,
                                         body.empty() ? WINHTTP_NO_REQUEST_DATA : (LPVOID)body.data(),
                                         (DWORD)body.size(),
                                         (DWORD)body.size(),
                                         0);

    std::string result;
    if (bResults && ::WinHttpReceiveResponse(hRequest, NULL)) {
        DWORD dwSize = 0;
        do {
            DWORD dwDownloaded = 0;
            if (!::WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
            if (dwSize == 0) break;
            std::string buffer(dwSize, 0);
            if (!::WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded)) break;
            buffer.resize(dwDownloaded);
            result += buffer;
        } while (dwSize > 0);
    } else {
        ShowError(L"HTTP Error", L"Failed to send or receive HTTP request.");
    }

    ::WinHttpCloseHandle(hRequest);
    ::WinHttpCloseHandle(hConnect);
    ::WinHttpCloseHandle(hSession);
    return result;
}

// ------------------ Local SSO Listener ------------------
std::optional<std::string> MatrixClient::RunLocalSSOListener() {
    ShowInfo(L"SSO", L"Starting local listener at http://localhost:8000 ...");

    ::WSADATA wsaData;
    if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) { ShowError(L"WSA Error", L"Failed to initialize Winsock."); return {}; }

    ::SOCKET listenSock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) { ::WSACleanup(); ShowError(L"Socket Error", L"Failed to create listening socket."); return {}; }

    ::SOCKADDR_IN service{};
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = ::htonl(INADDR_LOOPBACK);
    service.sin_port = ::htons(8000);

    if (::bind(listenSock, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
        ::closesocket(listenSock); ::WSACleanup();
        ShowError(L"Bind Error", L"Failed to bind listening socket.");
        return {};
    }

    if (::listen(listenSock, 1) == SOCKET_ERROR) {
        ::closesocket(listenSock); ::WSACleanup();
        ShowError(L"Listen Error", L"Failed to listen on socket.");
        return {};
    }

    std::wstring url = L"https://" + m_homeserver +
    L"/_matrix/client/r0/login/sso/redirect?redirectUrl=http://localhost:8000";
    ShowInfo(L"SSO", L"Opening browser for login: " + url);
    ::ShellExecuteW(NULL, L"open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);

    ::SOCKET clientSock = ::accept(listenSock, nullptr, nullptr);
    if (clientSock == INVALID_SOCKET) { ::closesocket(listenSock); ::WSACleanup(); ShowError(L"Accept Error", L"Failed to accept incoming connection."); return {}; }

    ShowInfo(L"SSO", L"Received callback from browser. Extracting token...");

    char buffer[2048];
    int received = ::recv(clientSock, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) { ::closesocket(clientSock); ::closesocket(listenSock); ::WSACleanup(); ShowError(L"Recv Error", L"Failed to receive HTTP request."); return {}; }
    buffer[received] = 0;

    std::string request(buffer);
    auto tokenPos = request.find("loginToken=");
    if (tokenPos == std::string::npos) { ::closesocket(clientSock); ::closesocket(listenSock); ::WSACleanup(); ShowError(L"SSO Error", L"loginToken not found in request."); return {}; }
    tokenPos += 11;
    auto endPos = request.find(' ', tokenPos);
    std::string loginToken = request.substr(tokenPos, endPos - tokenPos);

    ShowInfo(L"SSO", L"Got loginToken: " + std::wstring(loginToken.begin(), loginToken.end()));

    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>Login successful! You can close this window.</h1>";
    ::send(clientSock, response.c_str(), response.size(), 0);

    ::closesocket(clientSock);
    ::closesocket(listenSock);
    ::WSACleanup();
    return loginToken;
}
