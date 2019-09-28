#pragma once


// CFontsLibraryDlg 对话框

class CFontsLibraryDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CFontsLibraryDlg)

public:
	CFontsLibraryDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CFontsLibraryDlg();

// 对话框数据
	enum { IDD = IDD_FONTS_LIBRARY_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	int m_iFontsFamily;
	CString m_sImageChar;
};
