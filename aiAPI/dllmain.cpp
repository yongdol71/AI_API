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

    // API�� �ʱ�ȭ���� �ʾ����� �ʱ�ȭ �õ�
    if (g_claudeAPI == nullptr)
    {
        if (!InitializeClaudeAPI("config.ini"))
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
        if (!InitializeClaudeAPI("config.ini"))
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
        if (!InitializeClaudeAPI("config.ini"))
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
        if (!InitializeClaudeAPI("config.ini"))
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
        // DLL�� �ε�� �� �ڵ����� �ʱ�ȭ
        InitializeClaudeAPI("config.ini");
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
