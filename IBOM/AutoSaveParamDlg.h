#pragma once
#include "afxwin.h"


// CAutoSaveFileParameterDlg 对话框

class CAutoSaveParamDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CAutoSaveParamDlg)

public:
	CAutoSaveParamDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CAutoSaveParamDlg();

// 对话框数据
	enum { IDD = IDD_AUTO_SAVE_PARAM_DLG };
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
public:
	BOOL m_bAnymorePrompt;
	CString m_sAutoSaveTime;
	CComboBox m_cmbSvaeFileType;
	int m_iAutoSaveType;
	afx_msg void OnBnClickedOk();
};