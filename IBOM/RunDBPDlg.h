#pragma once


// CRunDBPDlg �Ի���

class CRunDBPDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CRunDBPDlg)

public:
	CRunDBPDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CRunDBPDlg();
	BOOL GetCadPath(char* cad_path);
// �Ի�������
	enum { IDD = IDD_RUN_DBP_DLG };
	CComboBox m_xPathCmbBox;
	CString m_sCadPath;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnOK();
	afx_msg void OnCbnSelchangeCmbPath();
};
