# AI API Test Application

MFC Dialog-based 테스트 프로그램으로 aiAPI.dll의 멀티모달 기능을 테스트합니다.

## 📁 프로젝트 구조

```
AI_API_Test/
├── AI_API_Test.h               # 애플리케이션 클래스 헤더
├── AI_API_Test.cpp             # 애플리케이션 진입점
├── AI_API_TestDlg.h            # 메인 다이얼로그 헤더
├── AI_API_TestDlg.cpp          # 메인 다이얼로그 구현
├── Resource.h                  # 리소스 ID 정의
├── AI_API_Test.rc              # 리소스 스크립트
├── AI_API_Test.vcxproj         # Visual Studio 프로젝트
├── stdafx.h, stdafx.cpp        # 미리 컴파일된 헤더
├── targetver.h                 # Windows 버전 정의
├── default_prompt.txt          # 기본 프롬프트 파일
└── res/
    ├── AI_API_Test.ico         # 애플리케이션 아이콘
    └── AI_API_Test.rc2         # 추가 리소스
```

## 🎨 UI 레이아웃

```
┌─────────────────────────────────────────────┐
│  AI API Tester - claude-sonnet-4-5  [_][□][X]│
├─────────────────────────────────────────────┤
│  대화 내역: (최상단 - 가장 넓은 영역)         │
│  ┌─────────────────────────────────────────┐│
│  │ [User] 이 이미지를 분석해주세요          ││
│  │ 📎 photo.jpg                            ││
│  │                                         ││
│  │ [AI] 이 이미지는 귀여운 고양이가...      ││
│  └─────────────────────────────────────────┘│
│                                             │
│  첨부 파일:                                  │
│  ┌─────────────────────────────────────────┐│
│  │ C:\images\photo.jpg                     ││
│  │ C:\documents\report.pdf                 ││
│  └─────────────────────────────────────────┘│
│  [추가] [제거] [모두제거]                     │
│                                             │
│  프롬프트: (4줄 이상)                        │
│  ┌─────────────────────────────────────────┐│
│  │                                         ││
│  │                                         ││
│  │                                         ││
│  │                                         ││
│  └─────────────────────────────────────────┘│
│          [기본값 불러오기] [지우기] [전송]    │
└─────────────────────────────────────────────┘
```

## 🚀 빌드 방법

### 필수 요구사항
- Visual Studio 2019 이상
- MFC 라이브러리 설치
- Windows SDK

### 빌드 단계
1. Visual Studio에서 `AI_API_Test.vcxproj` 열기
2. 솔루션 구성: `Debug` 또는 `Release`
3. 플랫폼: `Win32` 또는 `x64`
4. 빌드 → 솔루션 빌드 (Ctrl+Shift+B)

## 💻 사용 방법

### 1. DLL 배치
빌드된 실행 파일과 동일한 디렉토리에 다음 파일들을 배치:
- `aiAPI.dll` - AI API DLL
- `config.ini` - Claude API 설정
- `default_prompt.txt` - 기본 프롬프트 (자동 생성됨)

### 2. 실행
1. `AI_API_Test.exe` 실행
2. 창 제목에 현재 모델 이름 표시 확인

### 3. 파일 첨부
- **추가** 버튼 클릭하여 파일 선택
- 여러 파일 동시 선택 가능 (Ctrl/Shift 사용)
- 지원 형식: 이미지(JPG, PNG, GIF, WebP), 문서(PDF, TXT, JSON)

### 4. 프롬프트 입력
- 프롬프트 입력창에 질문 입력
- **기본값 불러오기**: `default_prompt.txt`의 내용 로드
- **지우기**: 프롬프트 초기화

### 5. 전송
- **전송** 버튼 클릭 또는 Ctrl+Enter
- 대화 내역에 사용자 메시지와 AI 응답 표시

## 🎨 기능 상세

### 대화 내역
- RichEdit 컨트롤로 서식 있는 텍스트 표시
- 사용자 메시지: 파란색, 굵게
- AI 응답: 녹색, 굵게
- 첨부 파일: 회색, 이탤릭체
- 자동 스크롤 지원

### 파일 관리
- 드래그 앤 드롭 미지원 (향후 추가 가능)
- 파일 경로 전체 표시
- 선택한 파일만 제거 또는 전체 제거

### 프롬프트 관리
- 멀티라인 편집 (4줄 이상)
- Enter키로 줄바꿈
- 기본 프롬프트 저장/로드

## 📋 DLL 인터페이스

### 사용하는 함수
```cpp
// 텍스트만 전송
bool GetAIResponse(const char* input, CHATGPT_RESULT* result);

// 파일 첨부하여 전송
bool GetAIResponseWithFiles(const char* input, const char** filePaths, int fileCount, CHATGPT_RESULT* result);

// 현재 모델 조회
const char* GetCurrentModel();
```

### CHATGPT_RESULT 구조체
```cpp
struct CHATGPT_RESULT {
    json o;                  // JSON 응답 전체
    std::string t;           // 텍스트 응답
    std::vector<char> data;  // 바이너리 데이터
};
```

## ⚙️ 설정 파일

### config.ini
```ini
[Claude]
ApiKey=your_api_key_here
Model=claude-sonnet-4-5
ApiVersion=2023-06-01
```

### default_prompt.txt
GUI 컴포넌트 분석을 위한 기본 프롬프트가 저장됩니다.
프로그램 첫 실행 시 자동 생성됩니다.

## 🔧 문제 해결

### DLL을 찾을 수 없습니다
- `aiAPI.dll`이 실행 파일과 같은 폴더에 있는지 확인
- 시스템 PATH에 DLL 경로 추가

### API 오류 발생
- `config.ini`에 올바른 API 키 설정 확인
- 인터넷 연결 확인
- 파일 크기 제한 확인 (5MB 이하)

### RichEdit 컨트롤 오류
- MFC DLL이 올바르게 설치되었는지 확인
- 프로젝트 속성 → MFC 사용 → 공유 DLL에서 MFC 사용

## 📝 향후 개선 사항

- [ ] 드래그 앤 드롭 파일 첨부
- [ ] 대화 내역 저장/불러오기
- [ ] 이미지 미리보기
- [ ] 설정 다이얼로그 (API 키, 모델 선택)
- [ ] 멀티스레드 API 호출 (UI 블로킹 방지)
- [ ] 진행률 표시

## 📄 라이선스

이 프로젝트는 aiAPI.dll 테스트 목적으로 작성되었습니다.
