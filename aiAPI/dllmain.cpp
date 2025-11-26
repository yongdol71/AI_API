// dllmain.cpp : DLL 애플리케이션의 진입점을 정의합니다.
#include "pch.h"
#include "claude_Sonnet3_5.hpp"

#pragma once

#define AIAPI_DLL_EXPORTS

#ifdef AIAPI_DLL_EXPORTS
#define AIAPI_DLL_API __declspec(dllexport)
#else
#define AIAPI_DLL_API __declspec(dllimport)
#endif    

// 전역 Claude API 인스턴스 (설정 파일에서 로드)
static CLAUDE_SONET_API* g_claudeAPI = nullptr;
static std::string g_lastError;

// DLL 초기화 함수
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

// DLL 정리 함수
void CleanupClaudeAPI()
{
    if (g_claudeAPI != nullptr)
    {
        delete g_claudeAPI;
        g_claudeAPI = nullptr;
    }
}
/*
// 에러 메시지 가져오기
extern "C" AIAPI_DLL_API char* GetLastError()
{
    return g_lastError.c_str();
}*/

// AI 응답 가져오기
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

// 설정 파일 재로드 (옵션)
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

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        // DLL이 로드될 때 자동으로 초기화
        InitializeClaudeAPI("config.ini");
        break;
        
    case DLL_THREAD_ATTACH:
        break;
        
    case DLL_THREAD_DETACH:
        break;
        
    case DLL_PROCESS_DETACH:
        // DLL이 언로드될 때 정리
        CleanupClaudeAPI();
        break;
    }
    return TRUE;
}
