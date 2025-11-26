#pragma once
#include <iostream>
#include <Windows.h>
#include <winhttp.h>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <map>

#pragma comment(lib,"winhttp.lib")

// nlohmann/json 사용
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// common.h include (CHATGPT_RESULT 정의)
// 프로젝트에 common.h가 있다면 이것을 사용, 없다면 아래 정의 사용
#ifndef CHATGPT_RESULT_DEFINED
#define CHATGPT_RESULT_DEFINED
struct CHATGPT_RESULT
{
    json o;
    std::string t;
    std::vector<char> data;
};
#endif

// ========================
// 설정 파일 로더
// ========================
class ConfigLoader
{
private:
    std::map<std::string, std::string> config;
    std::string configPath;

public:
    ConfigLoader(const std::string& path = "config.ini") : configPath(path)
    {
        LoadConfig();
    }

    bool LoadConfig()
    {
        std::ifstream file(configPath);
        if (!file.is_open())
        {
            // 설정 파일이 없으면 기본값으로 생성
            CreateDefaultConfig();
            return false;
        }

        std::string line;
        std::string currentSection;

        while (std::getline(file, line))
        {
            // 공백 제거
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);

            // 빈 줄이나 주석 무시
            if (line.empty() || line[0] == ';' || line[0] == '#')
                continue;

            // 섹션 파싱 [Section]
            if (line[0] == '[' && line[line.length() - 1] == ']')
            {
                currentSection = line.substr(1, line.length() - 2);
                continue;
            }

            // Key=Value 파싱
            size_t pos = line.find('=');
            if (pos != std::string::npos)
            {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);

                // 앞뒤 공백 제거
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);

                std::string fullKey = currentSection.empty() ? key : currentSection + "." + key;
                config[fullKey] = value;
            }
        }

        file.close();
        return true;
    }

    void CreateDefaultConfig()
    {
        std::ofstream file(configPath);
        if (file.is_open())
        {
            file << "[Claude]\n";
            file << "; API Key를 여기에 입력하세요\n";
            file << "ApiKey=YOUR_API_KEY_HERE\n";
            file << "\n";
            file << "; 사용할 모델명 (claude-sonnet-4-5, claude-opus-4-1, claude-haiku-4-5 등)\n";
            file << "Model=claude-sonnet-4-5\n";
            file << "\n";
            file << "; API 버전\n";
            file << "ApiVersion=2023-06-01\n";
            file.close();
        }
    }

    std::string Get(const std::string& key, const std::string& defaultValue = "")
    {
        auto it = config.find(key);
        if (it != config.end())
            return it->second;
        return defaultValue;
    }

    bool Exists(const std::string& key)
    {
        return config.find(key) != config.end();
    }
};

// ========================
// 문자열 변환 함수들
// ========================
std::string UTF8ToANSI(const std::string& utf8Str)
{
    if (utf8Str.empty()) {
        return std::string();
    }

    // UTF-8을 UTF-16으로 변환
    int wideLength = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, NULL, 0);
    if (wideLength == 0) {
        return std::string();
    }

    std::vector<wchar_t> wideStr(wideLength);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &wideStr[0], wideLength);

    // UTF-16을 ANSI로 변환
    int ansiLength = WideCharToMultiByte(CP_ACP, 0, &wideStr[0], -1, NULL, 0, NULL, NULL);
    if (ansiLength == 0) {
        return std::string();
    }

    std::vector<char> ansiStr(ansiLength);
    WideCharToMultiByte(CP_ACP, 0, &wideStr[0], -1, &ansiStr[0], ansiLength, NULL, NULL);

    return std::string(&ansiStr[0]);
}

std::string ANSIToUTF8(const std::string& ansiStr)
{
    if (ansiStr.empty()) {
        return std::string();
    }

    // ANSI를 UTF-16으로 변환
    int wideLength = MultiByteToWideChar(CP_ACP, 0, ansiStr.c_str(), -1, NULL, 0);
    if (wideLength == 0) {
        return std::string();
    }

    std::vector<wchar_t> wideStr(wideLength);
    MultiByteToWideChar(CP_ACP, 0, ansiStr.c_str(), -1, &wideStr[0], wideLength);

    // UTF-16을 UTF-8로 변환
    int utf8Length = WideCharToMultiByte(CP_UTF8, 0, &wideStr[0], -1, NULL, 0, NULL, NULL);
    if (utf8Length == 0) {
        return std::string();
    }

    std::vector<char> utf8Str(utf8Length);
    WideCharToMultiByte(CP_UTF8, 0, &wideStr[0], -1, &utf8Str[0], utf8Length, NULL, NULL);

    return std::string(&utf8Str[0]);
}

std::wstring StringToWString(const std::string& str)
{
    if (str.empty()) return std::wstring();
    
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    std::vector<wchar_t> buffer(size);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &buffer[0], size);
    
    return std::wstring(&buffer[0]);
}

std::string CreateJSONString(const std::string& inputStr)
{
    // ANSI를 UTF-8로 변환
    std::string utf8Str = ANSIToUTF8(inputStr);

    // JSON 이스케이프 처리
    std::string jsonStr;
    jsonStr.reserve(utf8Str.length() * 2);

    for (size_t i = 0; i < utf8Str.length(); ++i)
    {
        char c = utf8Str[i];
        switch (c) {
        case '\"': jsonStr += "\\\""; break;
        case '\\': jsonStr += "\\\\"; break;
        case '\b': jsonStr += "\\b"; break;
        case '\f': jsonStr += "\\f"; break;
        case '\n': jsonStr += "\\n"; break;
        case '\r': jsonStr += "\\r"; break;
        case '\t': jsonStr += "\\t"; break;
        default:
            if (static_cast<unsigned char>(c) < 0x20) {
                char buf[7];
                sprintf_s(buf, "\\u%04x", static_cast<unsigned char>(c));
                jsonStr += buf;
            }
            else {
                jsonStr += c;
            }
            break;
        }
    }

    return jsonStr;
}

// ========================
// WinHTTP 기반 HTTP 요청 함수
// ========================
std::vector<char> WinHttpRequest(
    const std::wstring& host,
    const std::wstring& path,
    const std::wstring& method,
    const std::vector<std::wstring>& headers,
    const char* postData,
    size_t postDataSize)
{
    std::vector<char> response;
    HINTERNET hSession = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;

    // WinHTTP 세션 초기화
    hSession = WinHttpOpen(
        L"Claude-API-Client/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);

    if (!hSession)
        return response;

    // 서버 연결
    hConnect = WinHttpConnect(hSession, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        return response;
    }

    // 요청 생성
    hRequest = WinHttpOpenRequest(
        hConnect,
        method.c_str(),
        path.c_str(),
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);

    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return response;
    }

    // 헤더 추가
    for (size_t i = 0; i < headers.size(); ++i)
    {
        WinHttpAddRequestHeaders(
            hRequest,
            headers[i].c_str(),
            (DWORD)-1,
            WINHTTP_ADDREQ_FLAG_ADD);
    }

    // 요청 전송
    BOOL bResults = WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        (LPVOID)postData,
        (DWORD)postDataSize,
        (DWORD)postDataSize,
        0);

    if (bResults)
        bResults = WinHttpReceiveResponse(hRequest, NULL);

    // 응답 읽기
    if (bResults)
    {
        DWORD dwSize = 0;
        DWORD dwDownloaded = 0;
        std::vector<char> buffer(4096);

        do
        {
            dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize))
                break;

            if (dwSize == 0)
                break;

            if (dwSize > buffer.size())
                buffer.resize(dwSize);

            if (!WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded))
                break;

            response.insert(response.end(), buffer.begin(), buffer.begin() + dwDownloaded);

        } while (dwSize > 0);
    }

    // 정리
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);

    return response;
}

// ========================
// Claude API 클래스
// ========================
class CLAUDE_SONET_API
{
private:
    std::string apiKey;
    std::string model;
    std::string apiVersion;
    ConfigLoader config;

public:
    CLAUDE_SONET_API(const char* configPath = "config.ini") : config(configPath)
    {
        // 설정 파일에서 로드
        apiKey = config.Get("Claude.ApiKey", "");
        model = config.Get("Claude.Model", "claude-sonnet-4-5");
        apiVersion = config.Get("Claude.ApiVersion", "2023-06-01");

        // API Key 검증
        if (apiKey.empty() || apiKey == "YOUR_API_KEY_HERE")
        {
            throw std::runtime_error(
                "API Key가 설정되지 않았습니다. config.ini 파일을 확인하세요.");
        }
    }

    // 레거시 생성자 (기존 코드 호환성)
    CLAUDE_SONET_API(const char* api_key, bool use_as_key) : config("config.ini")
    {
        if (use_as_key)
        {
            apiKey = api_key;
            model = "claude-sonnet-4-5";
            apiVersion = "2023-06-01";
        }
        else
        {
            config = ConfigLoader(api_key);
            apiKey = config.Get("Claude.ApiKey", "");
            model = config.Get("Claude.Model", "claude-sonnet-4-5");
            apiVersion = config.Get("Claude.ApiVersion", "2023-06-01");
        }
    }

    void SetModel(const char* m)
    {
        model = m;
    }

    std::string GetModel() const
    {
        return model;
    }

    std::string GetApiKey() const
    {
        return apiKey;
    }

    bool Text(const char* prompt, CHATGPT_RESULT* pResult, int Temperature = 0, int max_tokens = 4096)
    {
        if (apiKey.empty())
        {
            pResult->t = "Error: API Key not configured";
            return false;
        }

        // JSON 요청 생성
        std::vector<char> data(20000);
        sprintf_s(data.data(), 20000, u8R"({
    "model": "%s",
    "max_tokens": %d,
    "messages": [
        {"role": "user", "content": "%s"}
    ]
})", model.c_str(), max_tokens, CreateJSONString(prompt).c_str());

        data.resize(strlen(data.data()));

        // 헤더 준비
        std::wstring keyHeader = L"x-api-key: " + StringToWString(apiKey);
        std::wstring versionHeader = L"anthropic-version: " + StringToWString(apiVersion);

        std::vector<std::wstring> headers;
        headers.push_back(keyHeader);
        headers.push_back(versionHeader);
        headers.push_back(L"Content-Type: application/json");

        // WinHTTP 요청
        std::vector<char> response = WinHttpRequest(
            L"api.anthropic.com",
            L"/v1/messages",
            L"POST",
            headers,
            data.data(),
            data.size());

        if (response.empty())
        {
            pResult->t = "Error: No response from API";
            return false;
        }

        response.push_back('\0');

        // JSON 파싱 (nlohmann/json 사용)
        try
        {
            std::string utf8String = response.data();
//            std::string ansiString = UTF8ToANSI(utf8String);
            
            // nlohmann/json으로 파싱
            json j = json::parse(utf8String);
            pResult->o = j;

            // 에러 체크
            if (j.contains("error") && j["error"].is_object())
            {
                auto& error = j["error"];
                if (error.contains("message"))
                {
                    pResult->t = "API Error: " + error["message"].get<std::string>();
                }
                else
                {
                    pResult->t = "API Error: Unknown error";
                }
                return false;
            }

            // 정상 응답 처리
            if (j.contains("content") && j["content"].is_array())
            {
                auto& content = j["content"];
                if (content.size() > 0)
                {
                    auto& firstContent = content[0];
                    if (firstContent.contains("text"))
                    {
//                        pResult->t = firstContent["text"].get<std::string>();
                        pResult->t = UTF8ToANSI(firstContent["text"].get<std::string>());
                        return true;
                    }
                }
            }

            pResult->t = "Error: Unexpected response format";
            return false;
        }
        catch (const json::parse_error& e)
        {
            pResult->t = std::string("Error: JSON parse failed - ") + e.what();
            return false;
        }
        catch (const json::exception& e)
        {
            pResult->t = std::string("Error: JSON error - ") + e.what();
            return false;
        }
        catch (const std::exception& e)
        {
            pResult->t = std::string("Error: ") + e.what();
            return false;
        }
        catch (...)
        {
            pResult->t = "Error: Unknown exception during parsing";
            return false;
        }
    }
};
