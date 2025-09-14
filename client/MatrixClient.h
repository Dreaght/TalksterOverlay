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

#undef SendMessage

class MatrixClient {
public:
    MatrixClient(const std::wstring& homeserver);
    ~MatrixClient();

    // Async SSO login
    std::future<bool> LoginWithSSOAsync();

    // Async SSO login + auto-create random room
    std::future<bool> LoginWithSSOAndRandomRoomAsync();

    bool JoinRoom(const std::string& roomIdOrAlias);
    void SendMessageAsync(const std::string& roomId, const std::string& text);

    void Start();
    void Stop();

    void SetOnMessage(std::function<void(const std::string& roomId, const std::string& msg)> callback) {
        m_onMessage = callback;
    }

private:
    void SyncLoop();
    void SyncOnce();
    std::string HttpRequest(const std::wstring& method,
                            const std::wstring& path,
                            const std::string& body = "",
                            bool auth = false);
    std::optional<std::string> RunLocalSSOListener();
    std::string ExtractJsonValue(const std::string& json, const std::string& key);

private:
    std::wstring m_homeserver;
    std::string m_accessToken;
    std::string m_userId;

    std::function<void(const std::string&, const std::string&)> m_onMessage;

    std::thread m_thread;
    std::atomic<bool> m_running{ false };
};
