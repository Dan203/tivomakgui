
// tivomakguiDlg.h : header file
//

#pragma once


// CTiVoMAKGUIDlg dialog
class CTiVoMAKGUIDlg : public CDialogEx
{
// Construction
public:
	CTiVoMAKGUIDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TIVOMAKGUI_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;
	CEdit m_cMAK;
	CString m_sMAK;
	CEdit m_cStatus;
	CString m_sStatus;

	void LoadMAK();
	CString GetTiVoMAKPath();
	void SetStatus(CString sStatus);

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnClickedApply();
	DECLARE_MESSAGE_MAP()
public:
	CButton m_cSetButton;
};
