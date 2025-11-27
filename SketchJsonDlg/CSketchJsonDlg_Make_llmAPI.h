#pragma once

#include <string>
#include <vector>
#include <map>

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
// outResultMap: 결과 CSV를 파싱한 Map (Key: ID, Value: 나머지 컬럼들)
// 반환: 성공 시 true
bool ProcessImageWithJsonAndGetCSV(
    const std::string& imagePath,
    const std::string& jsonFilePath,
    const std::string& promptFilePath,
    std::map<std::string, std::vector<std::string>>& outResultMap);

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
