// dllmain.cpp : DLL ���ø����̼��� �������� �����մϴ�.
#include "pch.h"
#include "claude_Sonnet3_5.hpp"

#pragma once

#define AIAPI_DLL_EXPORTS

#ifdef AIAPI_DLL_EXPORTS
#define AIAPI_DLL_API __declspec(dllexport)
#else
#define AIAPI_DLL_API __declspec(dllimport)
#endif

// ���� Claude API �ν��Ͻ� (���� ���Ͽ��� �ε�)
static CLAUDE_SONET_API* g_claudeAPI = nullptr;
static std::string g_lastError;
static HMODULE g_hModule = nullptr;  // DLL 모듈 핸들 저장

// DLL 경로 기준으로 config.ini 전체 경로 가져오기
static std::string GetConfigPath()
{
    if (g_hModule == nullptr)
        return "config.ini";

    char dllPath[MAX_PATH] = { 0 };
    GetModuleFileNameA(g_hModule, dllPath, MAX_PATH);

    // DLL 파일명 제거하고 디렉토리 경로만 남기기
    std::string fullPath(dllPath);
    size_t lastSlash = fullPath.find_last_of("\\/");
    if (lastSlash != std::string::npos)
    {
        fullPath = fullPath.substr(0, lastSlash + 1);
    }

    return fullPath + "config.ini";
}

// DLL �ʱ�ȭ �Լ�
bool InitializeClaudeAPI(const char* configPath)
{
    try
    {
        if (g_claudeAPI != nullptr)
        {
            delete g_claudeAPI;
            g_claudeAPI = nullptr;
        }

        g_claudeAPI = new CLAUDE_SONET_API(configPath);
        g_lastError.clear();
        return true;
    }
    catch (const std::exception& e)
    {
        g_lastError = e.what();
        return false;
    }
    catch (...)
    {
        g_lastError = "Unknown error during initialization";
        return false;
    }
}

// DLL ���� �Լ�
void CleanupClaudeAPI()
{
    if (g_claudeAPI != nullptr)
    {
        delete g_claudeAPI;
        g_claudeAPI = nullptr;
    }
}
/*
// ���� �޽��� ��������
extern "C" AIAPI_DLL_API char* GetLastError()
{
    return g_lastError.c_str();
}*/

// AI ���� ��������
extern "C" AIAPI_DLL_API bool GetAIResponse(const char* input, CHATGPT_RESULT* result)
{
    if (result == nullptr)
    {
        g_lastError = "Result pointer is null";
        return false;
    }

    // API가 초기화되지 않았으면 초기화 시도
    if (g_claudeAPI == nullptr)
    {
        if (!InitializeClaudeAPI(GetConfigPath().c_str()))
        {
            result->t = "Error: " + g_lastError;
            return false;
        }
    }

    try
    {
        BOOL bSucceed = g_claudeAPI->Text(input, result);
        return bSucceed ? true : false;
    }
    catch (const std::exception& e)
    {
        g_lastError = e.what();
        result->t = "Exception: " + g_lastError;
        return false;
    }
    catch (...)
    {
        g_lastError = "Unknown exception in GetAIResponse";
        result->t = "Exception: " + g_lastError;
        return false;
    }
}

// ���� ���� ��ε� (�ɼ�)
extern "C" AIAPI_DLL_API bool ReloadConfig(const char* configPath)
{
    if (configPath == nullptr)
    {
        configPath = "config.ini";
    }
    return InitializeClaudeAPI(configPath);
}

// 현재 모델 이름 가져오기 (옵션)
extern "C" AIAPI_DLL_API const char* GetCurrentModel()
{
    if (g_claudeAPI != nullptr)
    {
        static std::string modelName;
        modelName = g_claudeAPI->GetModel();
        return modelName.c_str();
    }
    return "Not initialized";
}

// ========================
// 멀티모달 API: 이미지 첨부
// ========================
extern "C" AIAPI_DLL_API bool GetAIResponseWithImage(const char* input, const char* imagePath, CHATGPT_RESULT* result)
{
    if (result == nullptr)
    {
        g_lastError = "Result pointer is null";
        return false;
    }

    if (imagePath == nullptr)
    {
        g_lastError = "Image path is null";
        result->t = "Error: " + g_lastError;
        return false;
    }

    // API가 초기화되지 않았으면 초기화 시도
    if (g_claudeAPI == nullptr)
    {
        if (!InitializeClaudeAPI(GetConfigPath().c_str()))
        {
            result->t = "Error: " + g_lastError;
            return false;
        }
    }

    try
    {
        BOOL bSucceed = g_claudeAPI->TextWithImage(input, imagePath, result);
        return bSucceed ? true : false;
    }
    catch (const std::exception& e)
    {
        g_lastError = e.what();
        result->t = "Exception: " + g_lastError;
        return false;
    }
    catch (...)
    {
        g_lastError = "Unknown exception in GetAIResponseWithImage";
        result->t = "Exception: " + g_lastError;
        return false;
    }
}

// ========================
// 멀티모달 API: 단일 파일 첨부
// ========================
extern "C" AIAPI_DLL_API bool GetAIResponseWithFile(const char* input, const char* filePath, CHATGPT_RESULT* result)
{
    if (result == nullptr)
    {
        g_lastError = "Result pointer is null";
        return false;
    }

    if (filePath == nullptr)
    {
        g_lastError = "File path is null";
        result->t = "Error: " + g_lastError;
        return false;
    }

    // API가 초기화되지 않았으면 초기화 시도
    if (g_claudeAPI == nullptr)
    {
        if (!InitializeClaudeAPI(GetConfigPath().c_str()))
        {
            result->t = "Error: " + g_lastError;
            return false;
        }
    }

    try
    {
        BOOL bSucceed = g_claudeAPI->TextWithFile(input, filePath, result);
        return bSucceed ? true : false;
    }
    catch (const std::exception& e)
    {
        g_lastError = e.what();
        result->t = "Exception: " + g_lastError;
        return false;
    }
    catch (...)
    {
        g_lastError = "Unknown exception in GetAIResponseWithFile";
        result->t = "Exception: " + g_lastError;
        return false;
    }
}

// ========================
// 멀티모달 API: 여러 파일 첨부
// ========================
extern "C" AIAPI_DLL_API bool GetAIResponseWithFiles(const char* input, const char** filePaths, int fileCount, CHATGPT_RESULT* result)
{
    if (result == nullptr)
    {
        g_lastError = "Result pointer is null";
        return false;
    }

    if (filePaths == nullptr && fileCount > 0)
    {
        g_lastError = "File paths pointer is null";
        result->t = "Error: " + g_lastError;
        return false;
    }

    // API가 초기화되지 않았으면 초기화 시도
    if (g_claudeAPI == nullptr)
    {
        if (!InitializeClaudeAPI(GetConfigPath().c_str()))
        {
            result->t = "Error: " + g_lastError;
            return false;
        }
    }

    try
    {
        BOOL bSucceed = g_claudeAPI->TextWithFiles(input, filePaths, fileCount, result);
        return bSucceed ? true : false;
    }
    catch (const std::exception& e)
    {
        g_lastError = e.what();
        result->t = "Exception: " + g_lastError;
        return false;
    }
    catch (...)
    {
        g_lastError = "Unknown exception in GetAIResponseWithFiles";
        result->t = "Exception: " + g_lastError;
        return false;
    }
}

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        // DLL 모듈 핸들 저장
        g_hModule = hModule;
        // DLL 경로 기준으로 config.ini 찾아서 초기화
        InitializeClaudeAPI(GetConfigPath().c_str());
        break;
        
    case DLL_THREAD_ATTACH:
        break;
        
    case DLL_THREAD_DETACH:
        break;
        
    case DLL_PROCESS_DETACH:
        // DLL�� ��ε�� �� ����
        CleanupClaudeAPI();
        break;
    }
    return TRUE;
}
