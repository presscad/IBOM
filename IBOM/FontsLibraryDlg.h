#pragma once


// CFontsLibraryDlg �Ի���

class CFontsLibraryDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CFontsLibraryDlg)

public:
	CFontsLibraryDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CFontsLibraryDlg();

// �Ի�������
	enum { IDD = IDD_FONTS_LIBRARY_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
	int m_iFontsFamily;
	CString m_sImageChar;
};
