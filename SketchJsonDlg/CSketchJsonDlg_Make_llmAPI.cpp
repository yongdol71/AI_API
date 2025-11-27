#include "pch.h"
#include "CSketchJsonDlg_Make_llmAPI.h"
#include <Windows.h>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <cstdlib>

// DLL과 동일한 구조체를 사용하기 위해 common.h include
// 프로젝트 설정에서 include 경로 추가 필요: $(SolutionDir)AI_API\include
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// ========================
// DLL 함수 원형 정의
// ========================

// CHATGPT_RESULT 구조체 (DLL과 동일해야 함)
struct CHATGPT_RESULT
{
    json o;              // nlohmann::json 객체
    std::string t;       // 응답 텍스트
    std::vector<char> data;
};

typedef bool (*PFN_GetAIResponse)(const char* input, CHATGPT_RESULT* result);
typedef bool (*PFN_GetAIResponseWithImage)(const char* input, const char* imagePath, CHATGPT_RESULT* result);
typedef bool (*PFN_GetAIResponseWithFile)(const char* input, const char* filePath, CHATGPT_RESULT* result);
typedef bool (*PFN_GetAIResponseWithFiles)(const char* input, const char** filePaths, int fileCount, CHATGPT_RESULT* result);
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
// 유틸리티: ANSI -> UTF-8 변환
// ========================
static std::string ANSIToUTF8(const std::string& ansiStr)
{
    if (ansiStr.empty())
        return "";

    // ANSI -> Wide char
    int wideLen = MultiByteToWideChar(CP_ACP, 0, ansiStr.c_str(), -1, nullptr, 0);
    if (wideLen == 0)
        return ansiStr;

    std::vector<wchar_t> wideBuffer(wideLen);
    MultiByteToWideChar(CP_ACP, 0, ansiStr.c_str(), -1, wideBuffer.data(), wideLen);

    // Wide char -> UTF-8
    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wideBuffer.data(), -1, nullptr, 0, nullptr, nullptr);
    if (utf8Len == 0)
        return ansiStr;

    std::vector<char> utf8Buffer(utf8Len);
    WideCharToMultiByte(CP_UTF8, 0, wideBuffer.data(), -1, utf8Buffer.data(), utf8Len, nullptr, nullptr);

    return std::string(utf8Buffer.data());
}

// ========================
// 유틸리티: 파일 읽기 (UTF-8 변환 포함)
// ========================
static std::string ReadTextFile(const std::string& filePath, bool convertToUtf8 = true)
{
    // ANSI 경로 -> Wide char 변환 (한글 경로 지원)
    int wideLen = MultiByteToWideChar(CP_ACP, 0, filePath.c_str(), -1, nullptr, 0);
    if (wideLen == 0)
        return "";

    std::vector<wchar_t> widePathBuffer(wideLen);
    MultiByteToWideChar(CP_ACP, 0, filePath.c_str(), -1, widePathBuffer.data(), wideLen);

    // Wide char 경로로 파일 열기
    std::ifstream file(widePathBuffer.data(), std::ios::binary);
    if (!file.is_open())
        return "";

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    if (!convertToUtf8 || content.empty())
        return content;

    // UTF-8 BOM 확인 (EF BB BF)
    if (content.size() >= 3 &&
        (unsigned char)content[0] == 0xEF &&
        (unsigned char)content[1] == 0xBB &&
        (unsigned char)content[2] == 0xBF)
    {
        // UTF-8 BOM 제거, 이미 UTF-8이므로 변환 불필요
        return content.substr(3);
    }

    // BOM이 없으면 ANSI로 간주하고 UTF-8로 변환
    return ANSIToUTF8(content);
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
// 유틸리티: 문자열을 double로 변환 (안전하게)
// ========================
static double SafeStod(const std::string& str, double defaultVal = 0.0)
{
    if (str.empty())
        return defaultVal;

    try
    {
        return std::stod(str);
    }
    catch (...)
    {
        return defaultVal;
    }
}

// ========================
// 유틸리티: CSV 컬럼을 구조체에 매핑
// CSV 형식: ID, Name, Type, X, Y, Width, Height, Extra...
// ========================
static CSV_ITEM_DATA ParseCSVColumns(const std::vector<std::string>& columns)
{
    CSV_ITEM_DATA item;
    item.rawColumns = columns;

    if (columns.size() >= 1)
        item.id = columns[0];
    if (columns.size() >= 2)
        item.name = columns[1];
    if (columns.size() >= 3)
        item.type = columns[2];
    if (columns.size() >= 4)
        item.x = SafeStod(columns[3]);
    if (columns.size() >= 5)
        item.y = SafeStod(columns[4]);
    if (columns.size() >= 6)
        item.width = SafeStod(columns[5]);
    if (columns.size() >= 7)
        item.height = SafeStod(columns[6]);
    if (columns.size() >= 8)
        item.extra = columns[7];

    return item;
}

// ========================
// 함수 2: AI API 호출 및 CSV 결과 파싱
// ========================
bool ProcessImageWithJsonAndGetCSV(
    const std::string& imagePath,
    const std::string& jsonFilePath,
    const std::string& promptFilePath,
    CSV_RESULT_MAP& outResultMap)
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
    CHATGPT_RESULT result;
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

    // 7. CSV 내용 파싱하여 구조체로 변환 후 Map에 저장
    //    형식: ID, Name, Type, X, Y, Width, Height, Extra
    std::istringstream csvStream(csvContent);
    std::string line;

    while (std::getline(csvStream, line))
    {
        line = Trim(line);
        if (line.empty())
            continue;

        // 헤더 라인 스킵 (선택적)
        if (line.find("ID") == 0 && line.find(",") != std::string::npos)
        {
            // 헤더인지 확인 (첫 번째 컬럼이 "ID"이고 다음 컬럼이 "Name" 등인 경우)
            std::vector<std::string> headerCheck = ParseCSVLine(line);
            if (headerCheck.size() >= 2 &&
                (headerCheck[1] == "Name" || headerCheck[1] == "name" ||
                 headerCheck[1] == "NAME"))
            {
                continue;  // 헤더 스킵
            }
        }

        std::vector<std::string> columns = ParseCSVLine(line);
        if (columns.size() >= 1 && !columns[0].empty())
        {
            // 구조체로 변환
            CSV_ITEM_DATA itemData = ParseCSVColumns(columns);

            // ID를 Key로 Map에 저장
            outResultMap[itemData.id] = itemData;
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

    CHATGPT_RESULT result;
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

    CHATGPT_RESULT result;
    bool bSuccess = g_pfnGetAIResponse(
        prompt.c_str(),
        &result);

    if (bSuccess)
    {
        outResponse = result.t;
    }

    return bSuccess;
}
