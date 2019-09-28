#pragma once


// CInputPerfixAndSuffixDlg 对话框

class CInputPerfixAndSuffixDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CInputPerfixAndSuffixDlg)

public:
	CInputPerfixAndSuffixDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CInputPerfixAndSuffixDlg();

// 对话框数据
	enum { IDD = IDD_INPUT_PERFIX_AND_SUFFIX_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CString m_sPrefix;
	CString m_sSuffix;
	int m_iRotateAngle;
	virtual BOOL OnInitDialog();
};
