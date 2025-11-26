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

// common.h include (CHATGPT_RESULT 정의 및 유틸리티 함수)
#include "../include/common.h"

// ========================
// ���� ���� �δ�
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
            // ���� ������ ������ �⺻������ ����
            CreateDefaultConfig();
            return false;
        }

        std::string line;
        std::string currentSection;

        while (std::getline(file, line))
        {
            // ���� ����
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);

            // �� ���̳� �ּ� ����
            if (line.empty() || line[0] == ';' || line[0] == '#')
                continue;

            // ���� �Ľ� [Section]
            if (line[0] == '[' && line[line.length() - 1] == ']')
            {
                currentSection = line.substr(1, line.length() - 2);
                continue;
            }

            // Key=Value �Ľ�
            size_t pos = line.find('=');
            if (pos != std::string::npos)
            {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);

                // �յ� ���� ����
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
            file << "; API Key�� ���⿡ �Է��ϼ���\n";
            file << "ApiKey=YOUR_API_KEY_HERE\n";
            file << "\n";
            file << "; ����� �𵨸� (claude-sonnet-4-5, claude-opus-4-1, claude-haiku-4-5 ��)\n";
            file << "Model=claude-sonnet-4-5\n";
            file << "\n";
            file << "; API ����\n";
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
// ���ڿ� ��ȯ �Լ���
// ========================
std::string UTF8ToANSI(const std::string& utf8Str)
{
    if (utf8Str.empty()) {
        return std::string();
    }

    // UTF-8�� UTF-16���� ��ȯ
    int wideLength = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, NULL, 0);
    if (wideLength == 0) {
        return std::string();
    }

    std::vector<wchar_t> wideStr(wideLength);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &wideStr[0], wideLength);

    // UTF-16�� ANSI�� ��ȯ
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

    // ANSI�� UTF-16���� ��ȯ
    int wideLength = MultiByteToWideChar(CP_ACP, 0, ansiStr.c_str(), -1, NULL, 0);
    if (wideLength == 0) {
        return std::string();
    }

    std::vector<wchar_t> wideStr(wideLength);
    MultiByteToWideChar(CP_ACP, 0, ansiStr.c_str(), -1, &wideStr[0], wideLength);

    // UTF-16�� UTF-8�� ��ȯ
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
    // ANSI�� UTF-8�� ��ȯ
    std::string utf8Str = ANSIToUTF8(inputStr);

    // JSON �̽������� ó��
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
// WinHTTP ��� HTTP ��û �Լ�
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

    // WinHTTP ���� �ʱ�ȭ
    hSession = WinHttpOpen(
        L"Claude-API-Client/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);

    if (!hSession)
        return response;

    // ���� ����
    hConnect = WinHttpConnect(hSession, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        return response;
    }

    // ��û ����
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

    // ��� �߰�
    for (size_t i = 0; i < headers.size(); ++i)
    {
        WinHttpAddRequestHeaders(
            hRequest,
            headers[i].c_str(),
            (DWORD)-1,
            WINHTTP_ADDREQ_FLAG_ADD);
    }

    // ��û ����
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

    // ���� �б�
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

    // ����
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);

    return response;
}

// ========================
// Claude API Ŭ����
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
        // ���� ���Ͽ��� �ε�
        apiKey = config.Get("Claude.ApiKey", "");
        model = config.Get("Claude.Model", "claude-sonnet-4-5");
        apiVersion = config.Get("Claude.ApiVersion", "2023-06-01");

        // API Key ����
        if (apiKey.empty() || apiKey == "YOUR_API_KEY_HERE")
        {
            throw std::runtime_error(
                "API Key�� �������� �ʾҽ��ϴ�. config.ini ������ Ȯ���ϼ���.");
        }
    }

    // ���Ž� ������ (���� �ڵ� ȣȯ��)
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

        // JSON ��û ����
        std::vector<char> data(20000);
        sprintf_s(data.data(), 20000, u8R"({
    "model": "%s",
    "max_tokens": %d,
    "messages": [
        {"role": "user", "content": "%s"}
    ]
})", model.c_str(), max_tokens, CreateJSONString(prompt).c_str());

        data.resize(strlen(data.data()));

        // ��� �غ�
        std::wstring keyHeader = L"x-api-key: " + StringToWString(apiKey);
        std::wstring versionHeader = L"anthropic-version: " + StringToWString(apiVersion);

        std::vector<std::wstring> headers;
        headers.push_back(keyHeader);
        headers.push_back(versionHeader);
        headers.push_back(L"Content-Type: application/json");

        // WinHTTP ��û
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

        // JSON �Ľ� (nlohmann/json ���)
        try
        {
            std::string utf8String = response.data();
//            std::string ansiString = UTF8ToANSI(utf8String);
            
            // nlohmann/json���� �Ľ�
            json j = json::parse(utf8String);
            pResult->o = j;

            // ���� üũ
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

            // ���� ���� ó��
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

    // ========================
    // 멀티모달 지원: 파일 첨부 함수
    // ========================
    bool TextWithFiles(const char* prompt, const char** filePaths, int fileCount,
                       CHATGPT_RESULT* pResult, int max_tokens = 4096)
    {
        if (apiKey.empty())
        {
            pResult->t = "Error: API Key not configured";
            return false;
        }

        if (filePaths == nullptr || fileCount <= 0)
        {
            // 파일이 없으면 일반 Text 호출
            return Text(prompt, pResult, 0, max_tokens);
        }

        try
        {
            // JSON content 배열 구성
            json contentArray = json::array();

            // 1. 텍스트 프롬프트 추가
            if (prompt != nullptr && strlen(prompt) > 0)
            {
                contentArray.push_back({
                    {"type", "text"},
                    {"text", prompt}  // nlohmann/json이 UTF-8과 JSON 이스케이프를 자동 처리
                });
            }

            // 2. 파일들 추가
            for (int i = 0; i < fileCount; i++)
            {
                const char* filepath = filePaths[i];

                // 파일 읽기
                std::vector<unsigned char> fileData;
                if (!ReadBinaryFile(filepath, fileData))
                {
                    pResult->t = std::string("Error: Failed to read file: ") + ANSIToUTF8(filepath);
                    return false;
                }

                // 파일 크기 체크 (5MB 제한)
                const size_t MAX_FILE_SIZE = 5 * 1024 * 1024;
                if (fileData.size() > MAX_FILE_SIZE)
                {
                    pResult->t = std::string("Error: File too large (max 5MB): ") + ANSIToUTF8(filepath);
                    return false;
                }

                // Base64 인코딩
                std::string base64Data = Base64Encode(fileData.data(), fileData.size());
                if (base64Data.empty())
                {
                    pResult->t = std::string("Error: Failed to encode file: ") + ANSIToUTF8(filepath);
                    return false;
                }

                // MIME 타입 감지
                std::string mimeType = GetMimeType(filepath);

                // 이미지 파일 처리
                if (IsImageFile(filepath))
                {
                    contentArray.push_back({
                        {"type", "image"},
                        {"source", {
                            {"type", "base64"},
                            {"media_type", mimeType},
                            {"data", base64Data}
                        }}
                    });
                }
                // PDF 문서 파일 처리
                else if (IsPdfFile(filepath))
                {
                    contentArray.push_back({
                        {"type", "document"},
                        {"source", {
                            {"type", "base64"},
                            {"media_type", mimeType},
                            {"data", base64Data}
                        }}
                    });
                }
                // 텍스트 파일 처리 (JSON, TXT, XML 등)
                else if (IsTextFile(filepath))
                {
                    std::string textContent;

                    // UTF-8 BOM 확인 (EF BB BF)
                    bool hasUtf8Bom = (fileData.size() >= 3 &&
                                      fileData[0] == 0xEF &&
                                      fileData[1] == 0xBB &&
                                      fileData[2] == 0xBF);

                    if (hasUtf8Bom)
                    {
                        // UTF-8 BOM이 있으면 BOM을 제거하고 UTF-8 그대로 사용
                        textContent = std::string(fileData.begin() + 3, fileData.end());
                    }
                    else
                    {
                        // BOM이 없으면 ANSI(CP949)로 간주하고 UTF-8로 변환
                        // Windows에서 대부분의 텍스트 파일은 ANSI로 저장됨
                        std::string ansiText(fileData.begin(), fileData.end());

                        // 디버그: 원본 ANSI 저장
                        {
                            std::ofstream debugFileAnsi("debug_file_ansi.txt", std::ios::binary);
                            debugFileAnsi.write(ansiText.c_str(), ansiText.length());
                            debugFileAnsi.close();
                        }

                        // ANSI (CP949) -> Wide char
                        int wideLen = MultiByteToWideChar(CP_ACP, 0, ansiText.c_str(), (int)ansiText.length(), nullptr, 0);
                        if (wideLen > 0)
                        {
                            std::vector<wchar_t> wideBuffer(wideLen + 1);
                            MultiByteToWideChar(CP_ACP, 0, ansiText.c_str(), (int)ansiText.length(), wideBuffer.data(), wideLen);
                            wideBuffer[wideLen] = 0;

                            // Wide char -> UTF-8
                            int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wideBuffer.data(), wideLen, nullptr, 0, nullptr, nullptr);
                            if (utf8Len > 0)
                            {
                                std::vector<char> utf8Buffer(utf8Len + 1);
                                WideCharToMultiByte(CP_UTF8, 0, wideBuffer.data(), wideLen, utf8Buffer.data(), utf8Len, nullptr, nullptr);
                                utf8Buffer[utf8Len] = 0;
                                textContent = std::string(utf8Buffer.data());

                                // 디버그: 변환된 UTF-8 저장
                                {
                                    std::ofstream debugFileUtf8("debug_file_utf8.txt", std::ios::binary);
                                    debugFileUtf8.write(textContent.c_str(), textContent.length());
                                    debugFileUtf8.close();
                                }
                            }
                        }
                    }

                    // 파일명 추출
                    std::string filename = filepath;
                    size_t lastSlash = filename.find_last_of("\\/");
                    if (lastSlash != std::string::npos)
                        filename = filename.substr(lastSlash + 1);

                    // 파일명을 ANSI에서 UTF-8로 변환
                    std::string filenameUtf8 = ANSIToUTF8(filename);

                    // 텍스트 블록으로 추가 (filename과 textContent 모두 UTF-8)
                    std::string textBlock = std::string("File: ") + filenameUtf8 + "\n\n" + textContent;
                    contentArray.push_back({
                        {"type", "text"},
                        {"text", textBlock}  // nlohmann/json이 UTF-8과 JSON 이스케이프를 자동 처리
                    });
                }
                else
                {
                    pResult->t = std::string("Error: Unsupported file type: ") + ANSIToUTF8(filepath);
                    return false;
                }
            }

            // JSON 요청 생성
            json requestJson = {
                {"model", model},
                {"max_tokens", max_tokens},
                {"messages", json::array({
                    {
                        {"role", "user"},
                        {"content", contentArray}
                    }
                })}
            };

            std::string jsonString = requestJson.dump();

            // 디버그: 최종 JSON 요청 저장
            {
                std::ofstream debugJson("debug_request.json", std::ios::binary);
                debugJson << requestJson.dump(2);  // pretty print
                debugJson.close();
            }

            // 요청 크기 확인 (디버깅)
            if (jsonString.size() > 10 * 1024 * 1024)  // 10MB 초과
            {
                pResult->t = "Error: Request too large (" + std::to_string(jsonString.size()) + " bytes)";
                return false;
            }

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
                jsonString.c_str(),
                jsonString.size());

            if (response.empty())
            {
                pResult->t = "Error: No response from API";
                return false;
            }

            response.push_back('\0');

            // JSON 파싱
            std::string utf8String = response.data();
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

            // 응답 결과 처리
            if (j.contains("content") && j["content"].is_array())
            {
                auto& content = j["content"];
                if (content.size() > 0)
                {
                    auto& firstContent = content[0];
                    if (firstContent.contains("text"))
                    {
                        std::string responseUtf8 = firstContent["text"].get<std::string>();

                        // 디버그: API 응답 UTF-8 원본 저장
                        {
                            std::ofstream debugResponseUtf8("debug_response_utf8.txt", std::ios::binary);
                            debugResponseUtf8 << responseUtf8;
                            debugResponseUtf8.close();
                        }

                        pResult->t = UTF8ToANSI(responseUtf8);
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
            pResult->t = "Error: Unknown exception";
            return false;
        }
    }

    // 편의 함수: 단일 이미지 첨부
    bool TextWithImage(const char* prompt, const char* imagePath,
                       CHATGPT_RESULT* pResult, int max_tokens = 4096)
    {
        const char* files[] = { imagePath };
        return TextWithFiles(prompt, files, 1, pResult, max_tokens);
    }

    // 편의 함수: 단일 파일 첨부
    bool TextWithFile(const char* prompt, const char* filePath,
                      CHATGPT_RESULT* pResult, int max_tokens = 4096)
    {
        const char* files[] = { filePath };
        return TextWithFiles(prompt, files, 1, pResult, max_tokens);
    }
};
