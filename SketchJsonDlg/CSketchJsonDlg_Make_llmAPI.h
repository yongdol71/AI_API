#pragma once

#include <string>
#include <vector>
#include <map>

// ========================
// CSV 결과 저장용 구조체
// ========================
struct CSV_ITEM_DATA
{
    std::string id;           // ID (Key)
    std::string name;         // 이름 또는 설명
    std::string type;         // 타입
    double x;                 // X 좌표
    double y;                 // Y 좌표
    double width;             // 너비
    double height;            // 높이
    std::string extra;        // 추가 정보

    // 기본 생성자
    CSV_ITEM_DATA()
        : x(0.0), y(0.0), width(0.0), height(0.0)
    {}

    // 원본 컬럼 데이터 (파싱 전 raw 데이터)
    std::vector<std::string> rawColumns;
};

// 결과 Map 타입 정의
typedef std::map<std::string, CSV_ITEM_DATA> CSV_RESULT_MAP;

// ========================
// DLL 로딩/해제
// ========================

// DLL 로드 및 함수 연결
// dllPath: aiAPI.dll 경로 (예: "aiAPI.dll" 또는 전체 경로)
// 반환: 성공 시 true
bool LoadAiApiDll(const char* dllPath);

// DLL 해제
void UnloadAiApiDll();

// ========================
// AI API 호출 함수
// ========================

// 이미지와 JSON 파일을 첨부하여 AI API 호출 후 CSV 결과 파싱
// imagePath: 이미지 파일 경로
// jsonFilePath: JSON 파일 경로
// promptFilePath: 프롬프트 템플릿 파일 경로 ({JSON_CONTENT} 플레이스홀더 지원)
// outResultMap: 결과 CSV를 파싱한 Map (Key: ID, Value: CSV_ITEM_DATA 구조체)
// 반환: 성공 시 true
bool ProcessImageWithJsonAndGetCSV(
    const std::string& imagePath,
    const std::string& jsonFilePath,
    const std::string& promptFilePath,
    CSV_RESULT_MAP& outResultMap);

// 이미지만 첨부하여 AI API 호출
// imagePath: 이미지 파일 경로
// prompt: 프롬프트 텍스트
// outResponse: AI 응답 텍스트
// 반환: 성공 시 true
bool ProcessImageOnly(
    const std::string& imagePath,
    const std::string& prompt,
    std::string& outResponse);

// 텍스트만으로 AI API 호출
// prompt: 프롬프트 텍스트
// outResponse: AI 응답 텍스트
// 반환: 성공 시 true
bool ProcessTextOnly(
    const std::string& prompt,
    std::string& outResponse);
