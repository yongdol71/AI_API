// AI_API_TestDlg.cpp : 구현 파일
//

#include "stdafx.h"
#include "AI_API_Test.h"
#include "AI_API_TestDlg.h"
#include "afxdialogex.h"
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CAI_API_TestDlg 대화 상자

CAI_API_TestDlg::CAI_API_TestDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_AIAPI_TEST_DIALOG, pParent)
	, m_hIcon(nullptr)
	, m_hDll(nullptr)
	, m_pfnGetAIResponse(nullptr)
	, m_pfnGetAIResponseWithFiles(nullptr)
	, m_pfnGetCurrentModel(nullptr)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CAI_API_TestDlg::~CAI_API_TestDlg()
{
	UnloadDLL();
}

void CAI_API_TestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RICHEDIT_CHAT, m_ctrlChatHistory);
	DDX_Control(pDX, IDC_LIST_FILES, m_ctrlFileList);
	DDX_Control(pDX, IDC_EDIT_PROMPT, m_ctrlPrompt);
}

BEGIN_MESSAGE_MAP(CAI_API_TestDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_ADD_FILE, &CAI_API_TestDlg::OnBnClickedBtnAddFile)
	ON_BN_CLICKED(IDC_BTN_REMOVE_FILE, &CAI_API_TestDlg::OnBnClickedBtnRemoveFile)
	ON_BN_CLICKED(IDC_BTN_CLEAR_FILES, &CAI_API_TestDlg::OnBnClickedBtnClearFiles)
	ON_BN_CLICKED(IDC_BTN_SEND, &CAI_API_TestDlg::OnBnClickedBtnSend)
	ON_BN_CLICKED(IDC_BTN_CLEAR, &CAI_API_TestDlg::OnBnClickedBtnClear)
	ON_BN_CLICKED(IDC_BTN_LOAD_DEFAULT, &CAI_API_TestDlg::OnBnClickedBtnLoadDefault)
END_MESSAGE_MAP()

// CAI_API_TestDlg 메시지 처리기

BOOL CAI_API_TestDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// DLL 로드
	LoadDLL();

	// 기본 프롬프트 로드
	LoadDefaultPrompt();

	// RichEdit 설정
	m_ctrlChatHistory.SetReadOnly(TRUE);
	m_ctrlChatHistory.SetBackgroundColor(FALSE, RGB(255, 255, 255));

	return TRUE;
}

void CAI_API_TestDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR CAI_API_TestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// ========================================
// DLL 관리
// ========================================

void CAI_API_TestDlg::LoadDLL()
{
	// aiAPI.dll 로드
	m_hDll = LoadLibraryA("aiAPI.dll");
	if (m_hDll == nullptr)
	{
		MessageBox(_T("aiAPI.dll을 찾을 수 없습니다."), _T("오류"), MB_OK | MB_ICONERROR);
		return;
	}

	// 함수 포인터 가져오기
	m_pfnGetAIResponse = (PFN_GetAIResponse)GetProcAddress(m_hDll, "GetAIResponse");
	m_pfnGetAIResponseWithFiles = (PFN_GetAIResponseWithFiles)GetProcAddress(m_hDll, "GetAIResponseWithFiles");
	m_pfnGetCurrentModel = (PFN_GetCurrentModel)GetProcAddress(m_hDll, "GetCurrentModel");

	if (m_pfnGetAIResponse == nullptr || m_pfnGetAIResponseWithFiles == nullptr)
	{
		MessageBox(_T("DLL 함수를 찾을 수 없습니다."), _T("오류"), MB_OK | MB_ICONERROR);
		UnloadDLL();
		return;
	}

	// 모델 정보 표시
	if (m_pfnGetCurrentModel != nullptr)
	{
		const char* model = m_pfnGetCurrentModel();
		CString title;
		title.Format(_T("AI API Tester - %S"), model);
		SetWindowText(title);
	}
}

void CAI_API_TestDlg::UnloadDLL()
{
	if (m_hDll != nullptr)
	{
		FreeLibrary(m_hDll);
		m_hDll = nullptr;
	}

	m_pfnGetAIResponse = nullptr;
	m_pfnGetAIResponseWithFiles = nullptr;
	m_pfnGetCurrentModel = nullptr;
}

// ========================================
// 프롬프트 관리
// ========================================

void CAI_API_TestDlg::LoadDefaultPrompt()
{
	std::ifstream file("default_prompt.txt");
	if (!file.is_open())
	{
		// 파일이 없으면 기본값 생성
		SaveDefaultPrompt();
		return;
	}

	std::string line;
	std::string content;
	while (std::getline(file, line))
	{
		content += line + "\r\n";
	}
	file.close();

	m_strDefaultPrompt = CString(content.c_str());
}

void CAI_API_TestDlg::SaveDefaultPrompt()
{
	const char* defaultPrompt =
		"[Attached: 1 mobile view image + 1 ROI coordinate JSON file]\n"
		"Task Objective\n"
		"Analyze and categorize each ROI in the image to identify GUI components, then organize results in a table format\n"
		"Basic Information\n"
		"\n"
		"Image size: 540x1506 pt (same unit as JSON coordinates)\n"
		"JSON structure: {ID, absoluteBoundingBox{x, y, width, height}}\n"
		"\n"
		"GUI Component Classification Criteria\n"
		"1. Interactive Elements\n"
		"\n"
		"Button: Clickable single-action element\n"
		"Dropdown: Expandable menu for option selection\n"
		"Checkbox: Toggle for multiple selections\n"
		"Radio: Toggle for single selection\n"
		"Text Input: User input field\n"
		"Tab: Screen navigation element\n"
		"\n"
		"2. Visual Elements\n"
		"\n"
		"Icon: Small graphic element, nearly square-shaped (typically under 50pt)\n"
		"Image: Decorative/informational graphic element (not an icon)\n"
		"Label/Tag: Text badge for displaying information\n"
		"\n"
		"3. Structural Elements\n"
		"\n"
		"Container: Grouping element that holds other components\n"
		"\n"
		"Grid rows should also be classified as containers\n"
		"\n"
		"\n"
		"Card: Independent UI unit grouping related information\n"
		"Card Grid: Collection container specifically for cards\n"
		"Grid/Table: Structured data display\n"
		"Graph: Data visualization chart\n"
		"\n"
		"Analysis Guidelines\n"
		"\n"
		"Identify Hierarchy: Analyze from parent containers to child elements in order\n"
		"Handle Duplicates: When multiple IDs exist in the same area, specify each role separately\n"
		"Indicate States: Note activation/deactivation, selected/unselected states when clear\n"
		"Naming Consistency: Use identical naming for components with the same function\n"
		"Text Content: Include displayed text for buttons/labels/tags in \"Description\" column\n"
		"Language: Provide all descriptions in Korean (한글)\n"
		"\n"
		"Output Format\n"
		"Format the result as a CSV table with the following specifications:\n"
		"\n"
		"Columns: ID,GUI Component,Coordinates,Description\n"
		"Sort from top to bottom of the screen\n"
		"Express coordinates concisely as (x, y, w, h) format\n"
		"Write all descriptions in Korean\n"
		"Each line must end with a newline character\n"
		"Wrap the entire CSV output with:\n"
		"\n"
		"CSV_Start (followed by newline) at the beginning\n"
		"CSV_End (followed by newline) at the end\n"
		"\n"
		"\n"
		"This format allows for easy parsing of results\n"
		"\n"
		"Example format:\n"
		"CSV_Start\n"
		"ID,GUI Component,Coordinates,Description\n"
		"1,Button,\"(10, 20, 100, 40)\",로그인 버튼\\n\n"
		"2,Grid,\"(10, 210, 100, 400)\",상세 정보\\n\n"
		"CSV_End\n"
		"Important: Response Format\n"
		"Provide ONLY the CSV table in your response. Do not include any explanations, commentary, preamble, or postamble. Just the CSV table wrapped with CSV_Start and CSV_End markers.\n";

	std::ofstream file("default_prompt.txt");
	if (file.is_open())
	{
		file << defaultPrompt;
		file.close();
	}

	m_strDefaultPrompt = CString(defaultPrompt);
}

// ========================================
// 대화 내역 관리
// ========================================

void CAI_API_TestDlg::AddChatMessage(const CString& role, const CString& text)
{
	// 현재 선택 위치 저장
	CHARRANGE cr;
	m_ctrlChatHistory.GetSel(cr);

	// 텍스트 끝으로 이동
	int nLen = m_ctrlChatHistory.GetTextLength();
	m_ctrlChatHistory.SetSel(nLen, nLen);

	// 역할 표시
	CHARFORMAT2 cfRole;
	ZeroMemory(&cfRole, sizeof(CHARFORMAT2));
	cfRole.cbSize = sizeof(CHARFORMAT2);
	cfRole.dwMask = CFM_BOLD | CFM_COLOR | CFM_SIZE;
	cfRole.dwEffects = CFE_BOLD;
	cfRole.yHeight = 220; // 11pt
	cfRole.crTextColor = (role == _T("User")) ? RGB(0, 102, 204) : RGB(0, 153, 76);

	m_ctrlChatHistory.SetSelectionCharFormat(cfRole);
	m_ctrlChatHistory.ReplaceSel(_T("[") + role + _T("]\r\n"));

	// 메시지 내용
	CHARFORMAT2 cfText;
	ZeroMemory(&cfText, sizeof(CHARFORMAT2));
	cfText.cbSize = sizeof(CHARFORMAT2);
	cfText.dwMask = CFM_BOLD | CFM_COLOR | CFM_SIZE;
	cfText.dwEffects = 0; // Bold 해제
	cfText.yHeight = 200; // 10pt
	cfText.crTextColor = RGB(0, 0, 0);

	m_ctrlChatHistory.SetSelectionCharFormat(cfText);
	m_ctrlChatHistory.ReplaceSel(text + _T("\r\n\r\n"));

	// 자동 스크롤
	m_ctrlChatHistory.LineScroll(m_ctrlChatHistory.GetLineCount());
}

void CAI_API_TestDlg::AppendFileAttachments()
{
	if (m_arrFiles.empty())
		return;

	int nLen = m_ctrlChatHistory.GetTextLength();
	m_ctrlChatHistory.SetSel(nLen, nLen);

	CHARFORMAT2 cfFile;
	ZeroMemory(&cfFile, sizeof(CHARFORMAT2));
	cfFile.cbSize = sizeof(CHARFORMAT2);
	cfFile.dwMask = CFM_ITALIC | CFM_COLOR | CFM_SIZE;
	cfFile.dwEffects = CFE_ITALIC;
	cfFile.yHeight = 180; // 9pt
	cfFile.crTextColor = RGB(128, 128, 128);

	m_ctrlChatHistory.SetSelectionCharFormat(cfFile);

	for (size_t i = 0; i < m_arrFiles.size(); i++)
	{
		CString fileName = m_arrFiles[i];
		int pos = fileName.ReverseFind(_T('\\'));
		if (pos != -1)
			fileName = fileName.Mid(pos + 1);

		CString attachment;
		attachment.Format(_T("📎 %s\r\n"), fileName);
		m_ctrlChatHistory.ReplaceSel(attachment);
	}

	m_ctrlChatHistory.ReplaceSel(_T("\r\n"));
}

// ========================================
// 이벤트 핸들러
// ========================================

void CAI_API_TestDlg::OnBnClickedBtnAddFile()
{
	CFileDialog dlg(TRUE, NULL, NULL,
		OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER,
		_T("모든 파일 (*.*)|*.*|이미지 (*.jpg;*.png;*.gif;*.webp)|*.jpg;*.png;*.gif;*.webp|문서 (*.pdf;*.txt;*.json)|*.pdf;*.txt;*.json||"));

	const int MAX_FILES = 100;
	const int BUFFER_SIZE = MAX_FILES * MAX_PATH;
	TCHAR* buffer = new TCHAR[BUFFER_SIZE];
	dlg.m_ofn.lpstrFile = buffer;
	dlg.m_ofn.nMaxFile = BUFFER_SIZE;
	buffer[0] = NULL;

	if (dlg.DoModal() == IDOK)
	{
		POSITION pos = dlg.GetStartPosition();
		while (pos != NULL)
		{
			CString filePath = dlg.GetNextPathName(pos);
			m_ctrlFileList.AddString(filePath);
			m_arrFiles.push_back(filePath);
		}
	}

	delete[] buffer;
}

void CAI_API_TestDlg::OnBnClickedBtnRemoveFile()
{
	int nSel = m_ctrlFileList.GetCurSel();
	if (nSel != LB_ERR)
	{
		m_ctrlFileList.DeleteString(nSel);
		m_arrFiles.erase(m_arrFiles.begin() + nSel);
	}
}

void CAI_API_TestDlg::OnBnClickedBtnClearFiles()
{
	m_ctrlFileList.ResetContent();
	m_arrFiles.clear();
}

void CAI_API_TestDlg::OnBnClickedBtnSend()
{
	if (m_pfnGetAIResponse == nullptr && m_pfnGetAIResponseWithFiles == nullptr)
	{
		MessageBox(_T("DLL이 로드되지 않았습니다."), _T("오류"), MB_OK | MB_ICONERROR);
		return;
	}

	// 프롬프트 가져오기
	m_ctrlPrompt.GetWindowText(m_strPrompt);
	if (m_strPrompt.IsEmpty())
	{
		MessageBox(_T("프롬프트를 입력하세요."), _T("알림"), MB_OK | MB_ICONINFORMATION);
		return;
	}

	// 사용자 메시지 표시
	AddChatMessage(_T("User"), m_strPrompt);
	AppendFileAttachments();

	// 파일 경로 배열 준비
	int fileCount = (int)m_arrFiles.size();
	const char** filePaths = nullptr;
	std::vector<CStringA> ansiPaths;

	if (fileCount > 0)
	{
		ansiPaths.resize(fileCount);
		filePaths = new const char* [fileCount];

		for (int i = 0; i < fileCount; i++)
		{
			ansiPaths[i] = CT2A(m_arrFiles[i]);
			filePaths[i] = ansiPaths[i].GetString();
		}
	}

	// DLL 호출
	CHATGPT_RESULT result;
	CStringA promptAnsi = CT2A(m_strPrompt);

	BOOL bSuccess = FALSE;
	if (fileCount > 0)
	{
		bSuccess = m_pfnGetAIResponseWithFiles(promptAnsi, filePaths, fileCount, &result);
	}
	else
	{
		bSuccess = m_pfnGetAIResponse(promptAnsi, &result);
	}

	// 결과 표시
	if (bSuccess)
	{
		CString response = CA2T(result.t.c_str());
		AddChatMessage(_T("AI"), response);
	}
	else
	{
		CString error = CA2T(result.t.c_str());
		MessageBox(error, _T("API 오류"), MB_OK | MB_ICONERROR);
	}

	// 정리
	if (filePaths != nullptr)
		delete[] filePaths;

	// 프롬프트 초기화
	m_ctrlPrompt.SetWindowText(_T(""));
}

void CAI_API_TestDlg::OnBnClickedBtnClear()
{
	m_ctrlPrompt.SetWindowText(_T(""));
}

void CAI_API_TestDlg::OnBnClickedBtnLoadDefault()
{
	m_ctrlPrompt.SetWindowText(m_strDefaultPrompt);
}
