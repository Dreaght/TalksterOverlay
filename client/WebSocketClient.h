#pragma once
#define WIN32_LEAN_AND_MEAN
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "Ws2_32.lib")

class WebSocketClient {
public:
    WebSocketClient(const std::string& host, uint16_t port, const std::string& path = "/");
    ~WebSocketClient();

    void Connect();
    void Close();
    void Send(const std::wstring& message);

    void SetOnMessage(std::function<void(const std::wstring&)> cb) {
        m_onMessage = std::move(cb);
    }

private:
    void ReceiveLoop();
    bool PerformHandshake();

    std::string m_host;
    std::string m_path;
    uint16_t m_port;
    SOCKET m_socket{ INVALID_SOCKET };
    std::thread m_recvThread;
    std::atomic<bool> m_running{ false };
    std::function<void(const std::wstring&)> m_onMessage;
};
