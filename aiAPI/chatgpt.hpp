#pragma once
#include <iostream>
#include <Windows.h>
#include <winhttp.h>
#include <vector>
#include <optional>
#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <memory>

#pragma comment(lib,"winhttp.lib")

// common.h include (유틸리티 함수)
#include "../include/common.h"

// WinHTTP�� ����� ������ HTTP GET ��û
std::vector<char> Fetch(const char* urlStr)
{
    std::vector<char> result;
    
    // URL �Ľ�
    std::wstring wUrl;
    int len = MultiByteToWideChar(CP_UTF8, 0, urlStr, -1, NULL, 0);
    if (len > 0)
    {
        wUrl.resize(len);
        MultiByteToWideChar(CP_UTF8, 0, urlStr, -1, &wUrl[0], len);
    }

    URL_COMPONENTS urlComp;
    ZeroMemory(&urlComp, sizeof(urlComp));
    urlComp.dwStructSize = sizeof(urlComp);

    wchar_t hostName[256];
    wchar_t urlPath[1024];
    
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = sizeof(hostName) / sizeof(wchar_t);
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = sizeof(urlPath) / sizeof(wchar_t);

    if (!WinHttpCrackUrl(wUrl.c_str(), 0, 0, &urlComp))
        return result;

    HINTERNET hSession = WinHttpOpen(
        L"Fetch/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);

    if (!hSession)
        return result;

    HINTERNET hConnect = WinHttpConnect(
        hSession,
        urlComp.lpszHostName,
        urlComp.nPort,
        0);

    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        return result;
    }

    DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"GET",
        urlComp.lpszUrlPath,
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags);

    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return result;
    }

    BOOL bResults = WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        0,
        0);

    if (bResults)
        bResults = WinHttpReceiveResponse(hRequest, NULL);

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

            result.insert(result.end(), buffer.begin(), buffer.begin() + dwDownloaded);

        } while (dwSize > 0);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return result;
}

// WinHTTP ��û ���� �Լ�
std::vector<char> WinHttpPostRequest(
    const std::wstring& host,
    const std::wstring& path,
    const std::wstring& method,
    const std::vector<std::wstring>& headers,
    const char* postData,
    size_t postDataSize);

class CHATGPT_API
{
    std::string tok;
    std::string model = "gpt-4o";

public:
    CHATGPT_API(const char* api_key)
    {
        tok = api_key;
    }

    void SetModel(const char* m)
    {
        model = m;
    }

    std::wstring Bearer()
    {
        wchar_t auth[200] = {};
        swprintf_s(auth, 200, L"Authorization: Bearer %S", tok.c_str());
        return auth;
    }

    CHATGPT_RESULT Image(const char* prompt, int wi = 1024, int he = 1024, int max_tokens = 1000)
    {
        std::vector<char> data(10000);
        sprintf_s(data.data(), 10000, R"({
"model": "dall-e-3",
"prompt": "%s",
"n": 1,
"size": "%dx%d"
})", prompt, wi, he);

        data.resize(strlen(data.data()));

        std::wstring keyHeader = Bearer();
        std::wstring contentType = L"Content-Type: application/json";

        std::vector<std::wstring> headers;
        headers.push_back(keyHeader);
        headers.push_back(contentType);

        std::vector<char> out = WinHttpPostRequest(
            L"api.openai.com",
            L"/v1/images/generations",
            L"POST",
            headers,
            data.data(),
            data.size());

        out.push_back('\0');

        try
        {
            // nlohmann/json ���
            json j = json::parse(out.data());
            
            CHATGPT_RESULT r;
            r.o = j;

            if (j.contains("data") && j["data"].is_array())
            {
                auto& dataArray = j["data"];
                if (dataArray.size() > 0)
                {
                    auto& data0 = dataArray[0];
                    if (data0.contains("url"))
                    {
                        r.t = data0["url"].get<std::string>();
                        r.data = Fetch(r.t.c_str());
                    }
                }
            }
            return r;
        }
        catch (const json::exception& e)
        {
            // JSON �Ľ� ����
        }
        return {};
    }

    CHATGPT_RESULT Text(const char* prompt, int Temperature = 0.8, int max_tokens = 1000)
    {
        std::vector<char> data(10000);
        sprintf_s(data.data(), 10000, R"({
"model": "%s",
"messages": [
    {"role": "system", "content": "You are a helpful assistant."},
    {"role": "user", "content": "%s"}
],
"temperature": %.1f,
"max_tokens": %d
})", model.c_str(), prompt, (float)Temperature / 10.0f, max_tokens);

        data.resize(strlen(data.data()));

        std::wstring keyHeader = Bearer();
        std::wstring contentType = L"Content-Type: application/json";

        std::vector<std::wstring> headers;
        headers.push_back(keyHeader);
        headers.push_back(contentType);

        std::vector<char> out = WinHttpPostRequest(
            L"api.openai.com",
            L"/v1/chat/completions",
            L"POST",
            headers,
            data.data(),
            data.size());

        out.push_back('\0');

        try
        {
            // nlohmann/json ���
            json j = json::parse(out.data());

            CHATGPT_RESULT result;
            result.o = j;

            if (j.contains("error") && j["error"].is_object())
            {
                auto& error = j["error"];
                result.t = error["message"].get<std::string>();
            }
            else if (j.contains("choices") && j["choices"].is_array())
            {
                auto& choices = j["choices"];
                if (choices.size() > 0)
                {
                    auto& firstChoice = choices[0];
                    auto& message = firstChoice["message"];
                    result.t = message["content"].get<std::string>();
                }
            }
            else
            {
                result.t = "Unexpected response format.";
            }

            return result;
        }
        catch (const json::exception& e)
        {
            // JSON 파싱 실패
        }
        return {};
    }

    // ========================
    // Vision API: 이미지와 함께 질문
    // ========================
    CHATGPT_RESULT TextWithImage(const char* prompt, const char* imagePath, int max_tokens = 1000)
    {
        CHATGPT_RESULT result;

        try
        {
            // 파일 읽기
            std::vector<unsigned char> fileData;
            if (!ReadBinaryFile(imagePath, fileData))
            {
                result.t = std::string("Error: Failed to read image file: ") + imagePath;
                return result;
            }

            // 파일 크기 체크 (20MB 제한)
            const size_t MAX_FILE_SIZE = 20 * 1024 * 1024;
            if (fileData.size() > MAX_FILE_SIZE)
            {
                result.t = std::string("Error: Image file too large (max 20MB): ") + imagePath;
                return result;
            }

            // Base64 인코딩
            std::string base64Data = Base64Encode(fileData.data(), fileData.size());
            if (base64Data.empty())
            {
                result.t = "Error: Failed to encode image";
                return result;
            }

            // MIME 타입 감지
            std::string mimeType = GetMimeType(imagePath);
            if (!IsImageFile(imagePath))
            {
                result.t = "Error: File is not a supported image format";
                return result;
            }

            // JSON 요청 생성 (GPT-4 Vision)
            json contentArray = json::array();

            // 텍스트 추가
            if (prompt != nullptr && strlen(prompt) > 0)
            {
                contentArray.push_back({
                    {"type", "text"},
                    {"text", prompt}
                });
            }

            // 이미지 추가
            contentArray.push_back({
                {"type", "image_url"},
                {"image_url", {
                    {"url", "data:" + mimeType + ";base64," + base64Data}
                }}
            });

            json requestJson = {
                {"model", "gpt-4o"},  // Vision 지원 모델
                {"messages", json::array({
                    {
                        {"role", "user"},
                        {"content", contentArray}
                    }
                })},
                {"max_tokens", max_tokens}
            };

            std::string jsonString = requestJson.dump();

            // 헤더 준비
            std::wstring keyHeader = Bearer();
            std::wstring contentType = L"Content-Type: application/json";

            std::vector<std::wstring> headers;
            headers.push_back(keyHeader);
            headers.push_back(contentType);

            // WinHTTP 요청
            std::vector<char> out = WinHttpPostRequest(
                L"api.openai.com",
                L"/v1/chat/completions",
                L"POST",
                headers,
                jsonString.c_str(),
                jsonString.size());

            out.push_back('\0');

            // JSON 파싱
            json j = json::parse(out.data());
            result.o = j;

            if (j.contains("error") && j["error"].is_object())
            {
                auto& error = j["error"];
                result.t = error["message"].get<std::string>();
            }
            else if (j.contains("choices") && j["choices"].is_array())
            {
                auto& choices = j["choices"];
                if (choices.size() > 0)
                {
                    auto& firstChoice = choices[0];
                    auto& message = firstChoice["message"];
                    result.t = message["content"].get<std::string>();
                }
            }
            else
            {
                result.t = "Unexpected response format.";
            }

            return result;
        }
        catch (const json::exception& e)
        {
            result.t = std::string("JSON Error: ") + e.what();
            return result;
        }
        catch (const std::exception& e)
        {
            result.t = std::string("Error: ") + e.what();
            return result;
        }
        catch (...)
        {
            result.t = "Unknown error";
            return result;
        }
    }

    // ========================
    // Vision API: 여러 이미지와 함께 질문
    // ========================
    CHATGPT_RESULT TextWithImages(const char* prompt, const char** imagePaths, int imageCount, int max_tokens = 1000)
    {
        CHATGPT_RESULT result;

        try
        {
            // JSON content 배열 구성
            json contentArray = json::array();

            // 텍스트 추가
            if (prompt != nullptr && strlen(prompt) > 0)
            {
                contentArray.push_back({
                    {"type", "text"},
                    {"text", prompt}
                });
            }

            // 이미지들 추가
            for (int i = 0; i < imageCount; i++)
            {
                const char* imagePath = imagePaths[i];

                // 파일 읽기
                std::vector<unsigned char> fileData;
                if (!ReadBinaryFile(imagePath, fileData))
                {
                    result.t = std::string("Error: Failed to read image file: ") + imagePath;
                    return result;
                }

                // 파일 크기 체크
                const size_t MAX_FILE_SIZE = 20 * 1024 * 1024;
                if (fileData.size() > MAX_FILE_SIZE)
                {
                    result.t = std::string("Error: Image file too large (max 20MB): ") + imagePath;
                    return result;
                }

                // Base64 인코딩
                std::string base64Data = Base64Encode(fileData.data(), fileData.size());
                if (base64Data.empty())
                {
                    result.t = std::string("Error: Failed to encode image: ") + imagePath;
                    return result;
                }

                // MIME 타입 감지
                std::string mimeType = GetMimeType(imagePath);
                if (!IsImageFile(imagePath))
                {
                    result.t = std::string("Error: File is not a supported image: ") + imagePath;
                    return result;
                }

                // 이미지 추가
                contentArray.push_back({
                    {"type", "image_url"},
                    {"image_url", {
                        {"url", "data:" + mimeType + ";base64," + base64Data}
                    }}
                });
            }

            json requestJson = {
                {"model", "gpt-4o"},
                {"messages", json::array({
                    {
                        {"role", "user"},
                        {"content", contentArray}
                    }
                })},
                {"max_tokens", max_tokens}
            };

            std::string jsonString = requestJson.dump();

            // 헤더 준비
            std::wstring keyHeader = Bearer();
            std::wstring contentType = L"Content-Type: application/json";

            std::vector<std::wstring> headers;
            headers.push_back(keyHeader);
            headers.push_back(contentType);

            // WinHTTP 요청
            std::vector<char> out = WinHttpPostRequest(
                L"api.openai.com",
                L"/v1/chat/completions",
                L"POST",
                headers,
                jsonString.c_str(),
                jsonString.size());

            out.push_back('\0');

            // JSON 파싱
            json j = json::parse(out.data());
            result.o = j;

            if (j.contains("error") && j["error"].is_object())
            {
                auto& error = j["error"];
                result.t = error["message"].get<std::string>();
            }
            else if (j.contains("choices") && j["choices"].is_array())
            {
                auto& choices = j["choices"];
                if (choices.size() > 0)
                {
                    auto& firstChoice = choices[0];
                    auto& message = firstChoice["message"];
                    result.t = message["content"].get<std::string>();
                }
            }
            else
            {
                result.t = "Unexpected response format.";
            }

            return result;
        }
        catch (const json::exception& e)
        {
            result.t = std::string("JSON Error: ") + e.what();
            return result;
        }
        catch (const std::exception& e)
        {
            result.t = std::string("Error: ") + e.what();
            return result;
        }
        catch (...)
        {
            result.t = "Unknown error";
            return result;
        }
    }
};

// WinHTTP ��û ���� �Լ� ����
std::vector<char> WinHttpPostRequest(
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

    hSession = WinHttpOpen(
        L"ChatGPT-API/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);

    if (!hSession)
        return response;

    hConnect = WinHttpConnect(hSession, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        return response;
    }

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

    for (size_t i = 0; i < headers.size(); ++i)
    {
        WinHttpAddRequestHeaders(
            hRequest,
            headers[i].c_str(),
            (DWORD)-1,
            WINHTTP_ADDREQ_FLAG_ADD);
    }

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

    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);

    return response;
}
