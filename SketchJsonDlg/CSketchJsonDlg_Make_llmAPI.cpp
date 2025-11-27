#include "pch.h"
#include "CSketchJsonDlg_Make_llmAPI.h"
#include <Windows.h>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>

// ========================
// DLL 함수 원형 정의
// ========================
typedef bool (*PFN_GetAIResponse)(const char* input, void* result);
typedef bool (*PFN_GetAIResponseWithImage)(const char* input, const char* imagePath, void* result);
typedef bool (*PFN_GetAIResponseWithFile)(const char* input, const char* filePath, void* result);
typedef bool (*PFN_GetAIResponseWithFiles)(const char* input, const char** filePaths, int fileCount, void* result);
typedef char* (*PFN_GetLastError)();
typedef const char* (*PFN_GetCurrentModel)();
typedef bool (*PFN_ReloadConfig)(const char* configPath);

// ========================
// 멤버 변수 (클래스에 추가하거나 전역으로 사용)
// ========================
static HMODULE g_hAiApiDll = nullptr;
static PFN_GetAIResponse g_pfnGetAIResponse = nullptr;
static PFN_GetAIResponseWithImage g_pfnGetAIResponseWithImage = nullptr;
static PFN_GetAIResponseWithFile g_pfnGetAIResponseWithFile = nullptr;
static PFN_GetAIResponseWithFiles g_pfnGetAIResponseWithFiles = nullptr;
static PFN_GetLastError g_pfnGetLastError = nullptr;
static PFN_GetCurrentModel g_pfnGetCurrentModel = nullptr;
static PFN_ReloadConfig g_pfnReloadConfig = nullptr;

// ========================
// CHATGPT_RESULT 구조체 (DLL과 동일)
// ========================
struct CHATGPT_RESULT_LOCAL
{
    void* o;           // JSON object (nlohmann::json)
    std::string t;     // 응답 텍스트
    std::vector<char> data;
};

// ========================
// 함수 1: DLL 로딩 및 함수 연결
// ========================
bool LoadAiApiDll(const char* dllPath)
{
    // 이미 로드되어 있으면 해제
    if (g_hAiApiDll != nullptr)
    {
        FreeLibrary(g_hAiApiDll);
        g_hAiApiDll = nullptr;
    }

    // DLL 로드
    g_hAiApiDll = LoadLibraryA(dllPath);
    if (g_hAiApiDll == nullptr)
    {
        DWORD error = GetLastError();
        // 오류 처리
        return false;
    }

    // 함수 원형 연결
    g_pfnGetAIResponse = (PFN_GetAIResponse)GetProcAddress(g_hAiApiDll, "GetAIResponse");
    g_pfnGetAIResponseWithImage = (PFN_GetAIResponseWithImage)GetProcAddress(g_hAiApiDll, "GetAIResponseWithImage");
    g_pfnGetAIResponseWithFile = (PFN_GetAIResponseWithFile)GetProcAddress(g_hAiApiDll, "GetAIResponseWithFile");
    g_pfnGetAIResponseWithFiles = (PFN_GetAIResponseWithFiles)GetProcAddress(g_hAiApiDll, "GetAIResponseWithFiles");
    g_pfnGetLastError = (PFN_GetLastError)GetProcAddress(g_hAiApiDll, "GetLastError");
    g_pfnGetCurrentModel = (PFN_GetCurrentModel)GetProcAddress(g_hAiApiDll, "GetCurrentModel");
    g_pfnReloadConfig = (PFN_ReloadConfig)GetProcAddress(g_hAiApiDll, "ReloadConfig");

    // 필수 함수 확인
    if (g_pfnGetAIResponseWithFiles == nullptr)
    {
        FreeLibrary(g_hAiApiDll);
        g_hAiApiDll = nullptr;
        return false;
    }

    return true;
}

// ========================
// DLL 해제
// ========================
void UnloadAiApiDll()
{
    if (g_hAiApiDll != nullptr)
    {
        FreeLibrary(g_hAiApiDll);
        g_hAiApiDll = nullptr;
    }

    g_pfnGetAIResponse = nullptr;
    g_pfnGetAIResponseWithImage = nullptr;
    g_pfnGetAIResponseWithFile = nullptr;
    g_pfnGetAIResponseWithFiles = nullptr;
    g_pfnGetLastError = nullptr;
    g_pfnGetCurrentModel = nullptr;
    g_pfnReloadConfig = nullptr;
}

// ========================
// 유틸리티: 파일 읽기
// ========================
static std::string ReadTextFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
        return "";

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// ========================
// 유틸리티: 문자열 트림
// ========================
static std::string Trim(const std::string& str)
{
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

// ========================
// 유틸리티: CSV 라인 파싱
// ========================
static std::vector<std::string> ParseCSVLine(const std::string& line)
{
    std::vector<std::string> result;
    std::stringstream ss(line);
    std::string item;

    while (std::getline(ss, item, ','))
    {
        result.push_back(Trim(item));
    }

    return result;
}

// ========================
// 함수 2: AI API 호출 및 CSV 결과 파싱
// ========================
bool ProcessImageWithJsonAndGetCSV(
    const std::string& imagePath,
    const std::string& jsonFilePath,
    const std::string& promptFilePath,
    std::map<std::string, std::vector<std::string>>& outResultMap)
{
    // 결과 맵 초기화
    outResultMap.clear();

    // DLL 로드 확인
    if (g_pfnGetAIResponseWithFiles == nullptr)
    {
        return false;
    }

    // 1. 프롬프트 파일 읽기
    std::string promptTemplate = ReadTextFile(promptFilePath);
    if (promptTemplate.empty())
    {
        return false;
    }

    // 2. JSON 파일 읽기 (사전 프로세스용)
    std::string jsonContent = ReadTextFile(jsonFilePath);
    if (jsonContent.empty())
    {
        return false;
    }

    // 3. 사전 프로세스: 프롬프트에 JSON 내용 삽입
    //    예: {JSON_CONTENT} 플레이스홀더를 실제 JSON으로 대체
    std::string finalPrompt = promptTemplate;
    size_t pos = finalPrompt.find("{JSON_CONTENT}");
    if (pos != std::string::npos)
    {
        finalPrompt.replace(pos, 14, jsonContent);
    }

    // 4. 파일 경로 배열 준비 (이미지 + JSON)
    std::vector<const char*> filePaths;
    filePaths.push_back(imagePath.c_str());
    filePaths.push_back(jsonFilePath.c_str());

    // 5. API 호출
    CHATGPT_RESULT_LOCAL result;
    bool bSuccess = g_pfnGetAIResponseWithFiles(
        finalPrompt.c_str(),
        filePaths.data(),
        (int)filePaths.size(),
        &result);

    if (!bSuccess)
    {
        return false;
    }

    // 6. 결과에서 CSV_Start ~ CSV_End 사이 내용 추출
    std::string responseText = result.t;

    size_t csvStartPos = responseText.find("CSV_Start");
    size_t csvEndPos = responseText.find("CSV_End");

    if (csvStartPos == std::string::npos || csvEndPos == std::string::npos)
    {
        // CSV 마커를 찾지 못함
        return false;
    }

    // CSV_Start 다음부터 CSV_End 이전까지 추출
    csvStartPos += strlen("CSV_Start");
    std::string csvContent = responseText.substr(csvStartPos, csvEndPos - csvStartPos);
    csvContent = Trim(csvContent);

    // 7. CSV 내용 파싱하여 Map에 저장
    //    형식: ID, Value1, Value2, ...
    std::istringstream csvStream(csvContent);
    std::string line;

    while (std::getline(csvStream, line))
    {
        line = Trim(line);
        if (line.empty())
            continue;

        std::vector<std::string> columns = ParseCSVLine(line);
        if (columns.size() >= 2)
        {
            // 첫 번째 컬럼이 ID (Key)
            std::string id = columns[0];

            // 나머지 컬럼들을 값으로 저장
            std::vector<std::string> values(columns.begin() + 1, columns.end());
            outResultMap[id] = values;
        }
    }

    return true;
}

// ========================
// 함수 3: 단순 API 호출 (이미지만)
// ========================
bool ProcessImageOnly(
    const std::string& imagePath,
    const std::string& prompt,
    std::string& outResponse)
{
    if (g_pfnGetAIResponseWithImage == nullptr)
    {
        return false;
    }

    CHATGPT_RESULT_LOCAL result;
    bool bSuccess = g_pfnGetAIResponseWithImage(
        prompt.c_str(),
        imagePath.c_str(),
        &result);

    if (bSuccess)
    {
        outResponse = result.t;
    }

    return bSuccess;
}

// ========================
// 함수 4: 단순 텍스트 API 호출
// ========================
bool ProcessTextOnly(
    const std::string& prompt,
    std::string& outResponse)
{
    if (g_pfnGetAIResponse == nullptr)
    {
        return false;
    }

    CHATGPT_RESULT_LOCAL result;
    bool bSuccess = g_pfnGetAIResponse(
        prompt.c_str(),
        &result);

    if (bSuccess)
    {
        outResponse = result.t;
    }

    return bSuccess;
}
