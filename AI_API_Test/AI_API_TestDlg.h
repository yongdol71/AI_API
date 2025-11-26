// AI_API_TestDlg.h : 헤더 파일
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include <vector>
#include <string>

// DLL 인터페이스
#include "../include/common.h"

// DLL 함수 포인터 타입 정의
typedef bool (*PFN_GetAIResponse)(const char*, CHATGPT_RESULT*);
typedef bool (*PFN_GetAIResponseWithFiles)(const char*, const char**, int, CHATGPT_RESULT*);
typedef const char* (*PFN_GetCurrentModel)();

// CAI_API_TestDlg 대화 상자
class CAI_API_TestDlg : public CDialogEx
{
// 생성입니다.
public:
	CAI_API_TestDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.
	virtual ~CAI_API_TestDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_AIAPI_TEST_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.

// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

// 컨트롤 변수
public:
	CRichEditCtrl m_ctrlChatHistory;
	CListBox m_ctrlFileList;
	CEdit m_ctrlPrompt;

// 데이터 변수
private:
	CString m_strPrompt;
	std::vector<CString> m_arrFiles;
	CString m_strDefaultPrompt;

// DLL 관련
private:
	HMODULE m_hDll;
	PFN_GetAIResponse m_pfnGetAIResponse;
	PFN_GetAIResponseWithFiles m_pfnGetAIResponseWithFiles;
	PFN_GetCurrentModel m_pfnGetCurrentModel;

// 이벤트 핸들러
public:
	afx_msg void OnBnClickedBtnAddFile();
	afx_msg void OnBnClickedBtnRemoveFile();
	afx_msg void OnBnClickedBtnClearFiles();
	afx_msg void OnBnClickedBtnSend();
	afx_msg void OnBnClickedBtnClear();
	afx_msg void OnBnClickedBtnLoadDefault();

// 헬퍼 함수
private:
	void LoadDLL();
	void UnloadDLL();
	void LoadDefaultPrompt();
	void SaveDefaultPrompt();
	void AddChatMessage(const CString& role, const CString& text);
	void AppendFileAttachments();
	void SetChatColor(COLORREF color);
	CString Utf8ToAnsi(const char* utf8Str);
	std::string AnsiToUtf8(const char* ansiStr);
};
