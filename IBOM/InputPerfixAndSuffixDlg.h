#pragma once


// CInputPerfixAndSuffixDlg �Ի���

class CInputPerfixAndSuffixDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CInputPerfixAndSuffixDlg)

public:
	CInputPerfixAndSuffixDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CInputPerfixAndSuffixDlg();

// �Ի�������
	enum { IDD = IDD_INPUT_PERFIX_AND_SUFFIX_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
	CString m_sPrefix;
	CString m_sSuffix;
	int m_iRotateAngle;
	virtual BOOL OnInitDialog();
};
