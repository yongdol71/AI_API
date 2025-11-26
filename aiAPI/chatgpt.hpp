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

#pragma comment(lib,"winhttp.h")

#include "claude_Sonnet3_5.hpp"

// WinHTTP를 사용한 간단한 HTTP GET 요청
std::vector<char> Fetch(const char* urlStr)
{
    std::vector<char> result;
    
    // URL 파싱
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

// WinHTTP 요청 헬퍼 함수
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
            // nlohmann/json 사용
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
            // JSON 파싱 오류
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
            // nlohmann/json 사용
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
            // JSON 파싱 오류
        }
        return {};
    }
};

// WinHTTP 요청 헬퍼 함수 구현
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
