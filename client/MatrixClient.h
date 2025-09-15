#pragma once
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <optional>
#include <chrono>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <future>
#include <winhttp.h>
#include <shlobj.h> // SHGetFolderPath
#include <filesystem>
#include <fstream>
#include <../Utils.h>
#undef SendMessage




class MatrixClient {

    std::string m_nextBatch;

    std::mutex m_tasksMutex;
    std::vector<std::future<void>> m_tasks;

    static std::filesystem::path GetMatrixCredsPath() {
        wchar_t appData[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appData))) {
            std::filesystem::path p(appData);
            p /= L"Talkster";
            std::filesystem::create_directories(p); // make sure folder exists
            p /= L"matrix_credentials.dat";
            return p;
        }
        return L"matrix_credentials.dat"; // fallback
    }

    static void SaveCredentialsEncrypted(const std::string& accessToken, const std::string& userId) {
        std::string combined = accessToken + "\n" + userId;
        std::vector<BYTE> encrypted;
        if (!EncryptData(combined, encrypted)) return;

        auto path = GetMatrixCredsPath();
        std::ofstream f(path, std::ios::binary | std::ios::trunc);
        if (f.is_open()) {
            f.write((char*)encrypted.data(), encrypted.size());
        }
    }

    static std::optional<std::pair<std::string, std::string>> LoadCredentialsEncrypted() {
        auto path = GetMatrixCredsPath();
        if (!std::filesystem::exists(path)) return {};

        std::ifstream f(path, std::ios::binary);
        std::vector<BYTE> encrypted((std::istreambuf_iterator<char>(f)),
                                    std::istreambuf_iterator<char>());

        std::string decrypted;
        if (!DecryptData(encrypted, decrypted)) return {};

        size_t pos = decrypted.find('\n');
        if (pos == std::string::npos) return {};
        std::string token = decrypted.substr(0, pos);
        std::string userId = decrypted.substr(pos + 1);

        return std::make_pair(token, userId);
    }





public:
    MatrixClient(const std::wstring& homeserver);
    ~MatrixClient();

    using LoginCallback = std::function<void(bool)>;

    void SetOnLogin(LoginCallback cb);

    bool PerformLogin();

    // Async SSO login
    std::future<bool> LoginWithSSOAsync();

    // Async SSO login + auto-create random room
    std::future<bool> LoginWithSSOAndRandomRoomAsync();

    bool JoinRoom(const std::string& roomIdOrAlias);
    void SendMessageAsync(const std::string& roomId, const std::string& text);

    void Start();
    void Stop();

    void SendReadReceipt(const std::string &roomId, const std::string &eventId);

    void SetOnMessage(std::function<void(const std::string& roomId, const std::string& msg)> callback) {
        m_onMessage = callback;
    }

    std::string m_currentRoomId;

    std::string ExtractJsonValue(const std::string& json, const std::string& key);

    std::string HttpRequest(const std::wstring& method,
                            const std::wstring& path,
                            const std::string& body = "",
                            bool auth = false);

    void SetChatWindowHandle(HWND hwnd) {
        m_chatWindowHandle = hwnd;
    }

    static std::optional<std::string> LoadLastRoomLink() {
        auto path = GetLastRoomPath();
        if (!std::filesystem::exists(path)) return {};
        std::ifstream f(path);
        if (!f.is_open()) return {};
        std::string link;
        std::getline(f, link);
        if (link.empty()) return {};
        return link;
    }

    static std::filesystem::path GetLastRoomPath() {
        wchar_t appData[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appData))) {
            std::filesystem::path p(appData);
            p /= L"Talkster";
            std::filesystem::create_directories(p);
            p /= L"last_room.dat";
            return p;
        }
        return L"last_room.dat"; // fallback
    }

    static void SaveLastRoomLink(const std::string& roomLink) {
        auto path = GetLastRoomPath();
        std::ofstream f(path, std::ios::trunc);
        if (f.is_open()) f << roomLink;
    }

private:
    void SyncLoop();
    void SyncOnce();

    HINTERNET m_hRequest = nullptr;
    std::mutex m_httpMutex;

    std::optional<std::string> RunLocalSSOListener();

    LoginCallback onLogin_;

    std::wstring m_homeserver;
    std::string m_accessToken;
    std::string m_userId;

    HWND m_chatWindowHandle = nullptr;

    std::function<void(const std::string&, const std::string&)> m_onMessage;

    std::thread m_thread;
    std::atomic<bool> m_running{ false };

    template <typename F>
    void LaunchTask(F&& func) {
        std::lock_guard<std::mutex> lock(m_tasksMutex);
        m_tasks.emplace_back(std::async(std::launch::async, std::forward<F>(func)));
    }




};
