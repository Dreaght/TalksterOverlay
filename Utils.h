#pragma once
#include <string>

inline std::wstring ToWString(const std::string& s) {
    return std::wstring(s.begin(), s.end());
}

inline std::string ToString(const std::wstring& ws) {
    return std::string(ws.begin(), ws.end());
}

inline bool EncryptData(const std::string& plaintext, std::vector<BYTE>& out) {
    DATA_BLOB inBlob;
    inBlob.pbData = (BYTE*)plaintext.data();
    inBlob.cbData = (DWORD)plaintext.size();

    DATA_BLOB outBlob;
    if (!CryptProtectData(&inBlob, L"MatrixToken", nullptr, nullptr, nullptr, 0, &outBlob))
        return false;

    out.assign(outBlob.pbData, outBlob.pbData + outBlob.cbData);
    LocalFree(outBlob.pbData);
    return true;
}

inline bool DecryptData(const std::vector<BYTE>& in, std::string& outPlaintext) {
    DATA_BLOB inBlob;
    inBlob.pbData = (BYTE*)in.data();
    inBlob.cbData = (DWORD)in.size();

    DATA_BLOB outBlob;
    if (!CryptUnprotectData(&inBlob, nullptr, nullptr, nullptr, nullptr, 0, &outBlob))
        return false;

    outPlaintext.assign((char*)outBlob.pbData, outBlob.cbData);
    LocalFree(outBlob.pbData);
    return true;
}