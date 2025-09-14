#pragma once
#include <string>

class MatrixClient {
public:
    MatrixClient(const std::wstring& homeserver);

    bool Login(const std::string& user, const std::string& password);
    bool JoinRoom(const std::string& roomIdOrAlias);
    bool SendMessage(const std::string& roomId, const std::string& text);
    void Sync();

private:
    std::wstring m_homeserver;
    std::string m_accessToken;
    std::string m_userId;

    std::string HttpRequest(const std::wstring& method, const std::wstring& path,
                            const std::string& body = "", bool auth = false);
};
