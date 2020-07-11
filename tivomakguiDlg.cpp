
// tivomakguiDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "tivomakgui.h"
#include "tivomakguiDlg.h"
#include "afxdialogex.h"
#include "subprocess.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CTiVoMAKGUIDlg dialog

CTiVoMAKGUIDlg::CTiVoMAKGUIDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_TIVOMAKGUI_DIALOG, pParent)
	, m_sMAK(_T(""))
	, m_sStatus(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTiVoMAKGUIDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_MAK, m_cMAK);
	DDX_Text(pDX, IDC_EDIT_MAK, m_sMAK);
	DDX_Control(pDX, IDC_STATIC_STATUS, m_cStatus);
	DDX_Text(pDX, IDC_STATIC_STATUS, m_sStatus);
	DDX_Control(pDX, IDAPPLY, m_cSetButton);
}

BEGIN_MESSAGE_MAP(CTiVoMAKGUIDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDAPPLY, &CTiVoMAKGUIDlg::OnBnClickedApply)
END_MESSAGE_MAP()


// CTiVoMAKGUIDlg message handlers

BOOL CTiVoMAKGUIDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_cMAK.SetLimitText(10);
	LoadMAK();

	UpdateData(FALSE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CTiVoMAKGUIDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CTiVoMAKGUIDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CTiVoMAKGUIDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CTiVoMAKGUIDlg::OnBnClickedApply()
{
	UpdateData();

	CString sTiVoMAKExe = GetTiVoMAKPath();
	if (!::PathFileExists(sTiVoMAKExe))
	{
		CString sError;
		sError.LoadString(IDS_ERROR_TIVOMAK);
		SetStatus(sError);
		return;
	}

	CString sCLI;
	sCLI.Format(_T("-set %s"), m_sMAK);

	CSubProcess subProcess;
	HRESULT hr = subProcess.LoadSettings(sTiVoMAKExe, true, sCLI);
	if (SUCCEEDED(hr))
	{
		hr = subProcess.Start();
		if (SUCCEEDED(hr))
		{
			hr = subProcess.WaitForCompletion(60);
			if (SUCCEEDED(hr))
			{
				CString sSuccess;
				sSuccess.LoadString(IDS_SUCCESS);
				SetStatus(sSuccess);
			}
		}
	}

	if (FAILED(hr))
		SetStatus(subProcess.GetErrorMessage());
}

void CTiVoMAKGUIDlg::LoadMAK()
{
	CString sTiVoMAKExe = GetTiVoMAKPath();
	if (!::PathFileExists(sTiVoMAKExe))
	{
		CString sError;
		sError.LoadString(IDS_ERROR_TIVOMAK);
		SetStatus(sError);
		return;
	}

	CSubProcess subProcess;
	HRESULT hr = subProcess.LoadSettings(sTiVoMAKExe, true);
	if (SUCCEEDED(hr))
	{
		hr = subProcess.Start();
		if (SUCCEEDED(hr))
		{
			hr = subProcess.WaitForCompletion(60);
			if (SUCCEEDED(hr))
				m_sMAK = subProcess.GetStdOut();
		}
	}

	if (FAILED(hr))
		SetStatus(subProcess.GetErrorMessage());
}

CString CTiVoMAKGUIDlg::GetTiVoMAKPath()
{
	CString sModulePath;
	::GetModuleFileName(AfxGetApp()->m_hInstance, sModulePath.GetBuffer(_MAX_PATH), _MAX_PATH);
	sModulePath.ReleaseBuffer();

	CString sDrive, sFolder, sName, sExt;
	_tsplitpath_s(	sModulePath,
					sDrive.GetBuffer(_MAX_DRIVE), _MAX_DRIVE,
					sFolder.GetBuffer(_MAX_DIR), _MAX_DIR,
					sName.GetBuffer(_MAX_FNAME), _MAX_FNAME,
					sExt.GetBuffer(_MAX_EXT), _MAX_EXT 
	);
	sDrive.ReleaseBuffer();
	sFolder.ReleaseBuffer();
	sName.ReleaseBuffer();
	sExt.ReleaseBuffer();

	CString sTiVoMAKExe;
	_tmakepath_s(sTiVoMAKExe.GetBuffer(_MAX_PATH), _MAX_PATH, sDrive, sFolder, _T("tivomak"), _T("exe"));
	sTiVoMAKExe.ReleaseBuffer();

	return sTiVoMAKExe;
}

void CTiVoMAKGUIDlg::SetStatus(CString sStatus)
{
	if (sStatus.Trim().GetLength() == 0)
	{
		m_sStatus = _T("");
		UpdateData(FALSE);

		m_cStatus.ShowWindow(SW_HIDE);
	}
	else
	{
		m_sStatus = sStatus;
		UpdateData(FALSE);

		m_cStatus.ShowWindow(SW_SHOW);
	}
}
