#include "WebSocketClient.h"
#include <thread>
#include <vector>
#include <string>
#include <random>
#include <sstream>
#include <functional>
#include <iostream>
#include <windows.h>

// Link ws2_32.lib
#pragma comment(lib, "ws2_32.lib")

// ----------------- Minimal SHA-1 -----------------
struct SHA1Context { uint32_t state[5]; uint32_t count[2]; uint8_t buffer[64]; };
void SHA1Transform(uint32_t state[5], const uint8_t buffer[64]);
void SHA1Init(SHA1Context* context);
void SHA1Update(SHA1Context* context, const uint8_t* data, size_t len);
void SHA1Final(uint8_t digest[20], SHA1Context* context);

static const char b64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static inline uint32_t rol(uint32_t value, uint32_t bits) { return (value << bits) | (value >> (32 - bits)); }

static void SHA1Transform(uint32_t state[5], const uint8_t buffer[64]) {
    uint32_t a, b, c, d, e, t, W[80];

    for (int i = 0; i < 16; i++) {
        W[i] = (buffer[i*4] << 24) | (buffer[i*4+1] << 16) | (buffer[i*4+2] << 8) | (buffer[i*4+3]);
    }
    for (int i = 16; i < 80; i++) {
        W[i] = rol(W[i-3] ^ W[i-8] ^ W[i-14] ^ W[i-16], 1);
    }

    a = state[0]; b = state[1]; c = state[2]; d = state[3]; e = state[4];

    for (int i = 0; i < 80; i++) {
        if (i < 20)
            t = rol(a,5) + ((b & c) | ((~b) & d)) + e + W[i] + 0x5A827999;
        else if (i < 40)
            t = rol(a,5) + (b ^ c ^ d) + e + W[i] + 0x6ED9EBA1;
        else if (i < 60)
            t = rol(a,5) + ((b & c) | (b & d) | (c & d)) + e + W[i] + 0x8F1BBCDC;
        else
            t = rol(a,5) + (b ^ c ^ d) + e + W[i] + 0xCA62C1D6;

        e = d; d = c; c = rol(b,30); b = a; a = t;
    }

    state[0] += a; state[1] += b; state[2] += c; state[3] += d; state[4] += e;
}

void SHA1Init(SHA1Context* context) {
    context->count[0] = context->count[1] = 0;
    context->state[0] = 0x67452301;
    context->state[1] = 0xEFCDAB89;
    context->state[2] = 0x98BADCFE;
    context->state[3] = 0x10325476;
    context->state[4] = 0xC3D2E1F0;
}

void SHA1Update(SHA1Context* context, const uint8_t* data, size_t len) {
    size_t i, j;
    j = (context->count[0] >> 3) & 63;
    if ((context->count[0] += static_cast<uint32_t>(len << 3)) < (len << 3)) context->count[1]++;
    context->count[1] += static_cast<uint32_t>(len >> 29);
    for (i = 0; i < len; i++) {
        context->buffer[j++] = data[i];
        if (j == 64) {
            SHA1Transform(context->state, context->buffer);
            j = 0;
        }
    }
}

void SHA1Final(uint8_t digest[20], SHA1Context* context) {
    uint32_t i;
    uint8_t finalcount[8];
    for (i = 0; i < 8; i++) {
        finalcount[i] = static_cast<uint8_t>((context->count[(i >= 4 ? 0 : 1)]
                            >> ((3 - (i & 3)) * 8)) & 255);
    }
    uint8_t c = 0x80;
    SHA1Update(context, &c, 1);
    while ((context->count[0] & 504) != 448) {
        c = 0x00;
        SHA1Update(context, &c, 1);
    }
    SHA1Update(context, finalcount, 8);
    for (i = 0; i < 20; i++) {
        digest[i] = static_cast<uint8_t>((context->state[i>>2] >> ((3-(i & 3))*8)) & 255);
    }
}

std::string Base64Encode(const uint8_t* data, size_t len) {
    std::string result;
    int val = 0, valb = -6;
    for (size_t i = 0; i < len; ++i) {
        val = (val << 8) + data[i];
        valb += 8;
        while (valb >= 0) {
            result.push_back(b64chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) result.push_back(b64chars[((val << 8) >> (valb + 8)) & 0x3F]);
    while (result.size() % 4) result.push_back('=');
    return result;
}

std::string ComputeAcceptKey(const std::string& key) {
    SHA1Context ctx;
    uint8_t hash[20];
    SHA1Init(&ctx);
    std::string combined = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    SHA1Update(&ctx, reinterpret_cast<const uint8_t*>(combined.data()), combined.size());
    SHA1Final(hash, &ctx);
    return Base64Encode(hash, 20);
}

// ----------------- Helper Logging -----------------
static void LogError(const std::string& msg) {
    std::cerr << "[WebSocketClient] ERROR: " << msg << std::endl;
    MessageBoxA(nullptr, msg.c_str(), "WebSocketClient Error", MB_ICONERROR | MB_OK);
}

static void LogInfo(const std::string& msg) {
    std::cerr << "[WebSocketClient] INFO: " << msg << std::endl;
}

// ----------------- WebSocketClient -----------------
WebSocketClient::WebSocketClient(const std::string& host, uint16_t port, const std::string& path)
    : m_host(host), m_port(port), m_path(path) {}

WebSocketClient::~WebSocketClient() {
    Close();
}

void WebSocketClient::Connect() {
    LogInfo("Starting WSA...");
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        LogError("WSAStartup failed");
        return;
    }

    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) {
        LogError("Failed to create socket");
        WSACleanup();
        return;
    }

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(m_port);
    if (inet_pton(AF_INET, m_host.c_str(), &server.sin_addr) <= 0) {
        LogError("Invalid server IP address");
        closesocket(m_socket);
        WSACleanup();
        return;
    }

    LogInfo("Connecting to server...");
    if (connect(m_socket, (sockaddr*)&server, sizeof(server)) != 0) {
        LogError("Failed to connect to server");
        closesocket(m_socket);
        WSACleanup();
        return;
    }

    LogInfo("Performing handshake...");
    if (!PerformHandshake()) {
        LogError("WebSocket handshake failed");
        Close();
        return;
    }

    LogInfo("Handshake successful, starting receive thread...");
    m_running = true;
    m_recvThread = std::thread(&WebSocketClient::ReceiveLoop, this);
}

void WebSocketClient::Close() {
    m_running = false;
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        LogInfo("Socket closed");
    }
    if (m_recvThread.joinable()) m_recvThread.join();
    WSACleanup();
}

void WebSocketClient::Send(const std::wstring& message) {
    if (m_socket == INVALID_SOCKET) {
        LogError("Cannot send: socket is invalid");
        return;
    }

    std::string utf8;
    int len = WideCharToMultiByte(CP_UTF8, 0, message.c_str(), -1, nullptr, 0, nullptr, nullptr);
    utf8.resize(len);
    WideCharToMultiByte(CP_UTF8, 0, message.c_str(), -1, utf8.data(), len, nullptr, nullptr);

    std::vector<uint8_t> frame;
    frame.push_back(0x81); // FIN + text
    size_t payload_len = utf8.size() - 1;
    if (payload_len <= 125) frame.push_back(0x80 | (uint8_t)payload_len);
    else if (payload_len <= 65535) { frame.push_back(0x80 | 126); frame.push_back((payload_len>>8)&0xFF); frame.push_back(payload_len&0xFF); }
    else { frame.push_back(0x80 | 127); for (int i=7;i>=0;--i) frame.push_back((payload_len>>(8*i))&0xFF); }

    uint8_t mask[4];
    std::random_device rd; for (int i=0;i<4;i++) mask[i] = static_cast<uint8_t>(rd());
    frame.insert(frame.end(), mask, mask+4);
    for (size_t i=0;i<payload_len;i++) frame.push_back(utf8[i] ^ mask[i%4]);

    if (send(m_socket, reinterpret_cast<char*>(frame.data()), frame.size(), 0) == SOCKET_ERROR)
        LogError("Failed to send message");
    else
        LogInfo("Message sent successfully");
}

void WebSocketClient::ReceiveLoop() {
    std::vector<uint8_t> buffer(2048);
    while (m_running) {
        int n = recv(m_socket, reinterpret_cast<char*>(buffer.data()), (int)buffer.size(), 0);
        if (n <= 0) {
            LogError("Connection closed or receive failed");
            break;
        }

        size_t i = 0;
        while (i < n) {
            uint8_t b1 = buffer[i++], b2 = buffer[i++];
            uint64_t payload_len = b2 & 0x7F;

            if (payload_len == 126) payload_len = (buffer[i++]<<8)|buffer[i++];
            else if (payload_len == 127) { payload_len=0; for(int j=0;j<8;j++) payload_len=(payload_len<<8)|buffer[i++]; }

            uint8_t mask[4] = {0,0,0,0};
            if (b2 & 0x80) for(int j=0;j<4;j++) mask[j]=buffer[i++];

            std::string payload;
            for(uint64_t j=0;j<payload_len;j++){ uint8_t byte=buffer[i++]; if(b2&0x80) byte^=mask[j%4]; payload.push_back(byte); }

            int wlen = MultiByteToWideChar(CP_UTF8,0,payload.c_str(),-1,nullptr,0);
            std::wstring wmsg(wlen,L'\0');
            MultiByteToWideChar(CP_UTF8,0,payload.c_str(),-1,wmsg.data(),wlen);

            if(m_onMessage) m_onMessage(wmsg);
        }
    }
}

bool WebSocketClient::PerformHandshake() {
    std::string key_raw(16,'\0');
    std::random_device rd; for(auto &c:key_raw) c=static_cast<char>(rd());
    std::string key = Base64Encode(reinterpret_cast<const uint8_t*>(key_raw.data()),16);

    std::ostringstream request;
    request << "GET " << m_path << " HTTP/1.1\r\n"
            << "Host: " << m_host << ":" << m_port << "\r\n"
            << "Upgrade: websocket\r\n"
            << "Connection: Upgrade\r\n"
            << "Sec-WebSocket-Key: " << key << "\r\n"
            << "Sec-WebSocket-Version: 13\r\n\r\n";

    if (send(m_socket, request.str().c_str(), (int)request.str().size(), 0) == SOCKET_ERROR) {
        LogError("Failed to send handshake request");
        return false;
    }

    char response[1024] = {};
    int n = recv(m_socket, response, sizeof(response)-1, 0);
    if (n <= 0) { LogError("Failed to receive handshake response"); return false; }
    response[n]=0;
    std::string respStr(response);

    std::string expected = ComputeAcceptKey(key);
    if(respStr.find("101 Switching Protocols")==std::string::npos || respStr.find(expected)==std::string::npos){
        LogError("Handshake validation failed");
        return false;
    }

    LogInfo("Handshake OK");
    return true;
}
