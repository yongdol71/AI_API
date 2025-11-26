#pragma once

#include <string>
#include <vector>
#include <Windows.h>
#include <fstream>
#include <algorithm>
#include <wincrypt.h>

#pragma comment(lib, "Crypt32.lib")

// nlohmann/json 사용
#include <nlohmann/json.hpp>
using json = nlohmann::json;

struct CHATGPT_RESULT
{
    json o;  // jsonxx::Object 대신 json 사용
    std::string t;
    std::vector<char> data;
};

// ========================
// Base64 인코딩 함수
// ========================
inline std::string Base64Encode(const unsigned char* data, size_t length)
{
    if (length == 0 || data == nullptr)
        return "";

    DWORD base64Length = 0;

    // 필요한 버퍼 크기 계산
    if (!CryptBinaryToStringA(data, (DWORD)length,
        CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
        NULL, &base64Length))
    {
        return "";
    }

    // Base64 인코딩
    std::vector<char> base64Buffer(base64Length);
    if (!CryptBinaryToStringA(data, (DWORD)length,
        CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
        base64Buffer.data(), &base64Length))
    {
        return "";
    }

    return std::string(base64Buffer.data(), base64Length - 1); // null terminator 제외
}

// ========================
// 파일 읽기 함수
// ========================
inline bool ReadBinaryFile(const char* filepath, std::vector<unsigned char>& outData)
{
    // ANSI -> Wide char 변환 (한글 경로 지원)
    int wideLen = MultiByteToWideChar(CP_ACP, 0, filepath, -1, nullptr, 0);
    if (wideLen == 0)
        return false;

    std::vector<wchar_t> wideBuffer(wideLen);
    MultiByteToWideChar(CP_ACP, 0, filepath, -1, wideBuffer.data(), wideLen);

    // Wide char로 파일 열기
    std::ifstream file(wideBuffer.data(), std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return false;

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    outData.resize(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(outData.data()), size))
    {
        outData.clear();
        return false;
    }

    return true;
}

// ========================
// MIME 타입 감지 함수
// ========================
inline std::string GetMimeType(const char* filepath)
{
    std::string path(filepath);

    // 소문자로 변환
    std::transform(path.begin(), path.end(), path.begin(), ::tolower);

    // 확장자 추출
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos)
        return "application/octet-stream";

    std::string ext = path.substr(dotPos);

    // MIME 타입 매핑
    if (ext == ".jpg" || ext == ".jpeg")
        return "image/jpeg";
    else if (ext == ".png")
        return "image/png";
    else if (ext == ".gif")
        return "image/gif";
    else if (ext == ".webp")
        return "image/webp";
    else if (ext == ".pdf")
        return "application/pdf";
    else if (ext == ".txt")
        return "text/plain";
    else if (ext == ".json")
        return "application/json";
    else if (ext == ".xml")
        return "application/xml";
    else if (ext == ".csv")
        return "text/csv";
    else if (ext == ".md")
        return "text/markdown";
    else
        return "application/octet-stream";
}

// ========================
// 파일 타입 체크 함수
// ========================
inline bool IsImageFile(const char* filepath)
{
    std::string mime = GetMimeType(filepath);
    return mime.find("image/") == 0;
}

inline bool IsPdfFile(const char* filepath)
{
    std::string mime = GetMimeType(filepath);
    return mime == "application/pdf";
}

inline bool IsTextFile(const char* filepath)
{
    std::string mime = GetMimeType(filepath);
    return mime.find("text/") == 0 ||
           mime == "application/json" ||
           mime == "application/xml" ||
           mime == "text/csv";
}

inline bool IsDocumentFile(const char* filepath)
{
    // Claude API는 document 타입으로 PDF만 지원
    return IsPdfFile(filepath);
}
