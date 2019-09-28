#pragma once


// CRunDBPDlg 对话框

class CRunDBPDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CRunDBPDlg)

public:
	CRunDBPDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CRunDBPDlg();
	BOOL GetCadPath(char* cad_path);
// 对话框数据
	enum { IDD = IDD_RUN_DBP_DLG };
	CComboBox m_xPathCmbBox;
	CString m_sCadPath;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnOK();
	afx_msg void OnCbnSelchangeCmbPath();
};
