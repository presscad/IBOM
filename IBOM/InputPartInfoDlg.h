#pragma once
#include "afxwin.h"
#include "Recog.h"

// CInputPartInfoDlg 对话框

class CInputPartInfoDlg : public CDialog
{
	DECLARE_DYNAMIC(CInputPartInfoDlg)
public:
	CInputPartInfoDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CInputPartInfoDlg();
	void RelayoutWnd();
	void RefreshDlg();
	int GetRectHeight();
	void SetEditFocus();
	CImageRegionHost* GetRegionHost();
	// 对话框数据
	enum { IDD = IDD_INPUT_PARTINFO_DLG };
	IRecoginizer::BOMPART* m_pBomPart;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
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
