#pragma once
#include "afxwin.h"
#include "Recog.h"

// CInputPartInfoDlg �Ի���

class CInputPartInfoDlg : public CDialog
{
	DECLARE_DYNAMIC(CInputPartInfoDlg)
public:
	CInputPartInfoDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CInputPartInfoDlg();
	void RelayoutWnd();
	void RefreshDlg();
	int GetRectHeight();
	void SetEditFocus();
	CImageRegionHost* GetRegionHost();
	// �Ի�������
	enum { IDD = IDD_INPUT_PARTINFO_DLG };
	IRecoginizer::BOMPART* m_pBomPart;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
	afx_msg void OnOK();
	afx_msg void OnCancel();
	afx_msg void OnEnChangeSumWeight();
public:
	CString m_sPartNo;
	long m_nNum;
	double m_fLength;
	CString m_sWeight;
	double m_fSumWeight;
	CString m_sMaterial;
	CString	m_sJgGuiGe;
};
