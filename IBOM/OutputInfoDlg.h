#pragma once


// COutputInfoDlg �Ի���
#include "supergridctrl.h"
class COutputInfoDlg : public CDialog
{
	DECLARE_DYNCREATE(COutputInfoDlg)

public:
	COutputInfoDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~COutputInfoDlg();

	void RelayoutWnd();
	// �Ի�������
	enum { IDD = IDD_OUTPUT_INFO_DLG };
	CSuperGridCtrl	m_listInfoCtrl;
	CTabCtrl	m_tabInfoGroup;
	CString  m_sRecord;
	double m_fLenTolerance,m_fWeightTolerance;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnOK();
	afx_msg void OnCancel();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSelchangeTabGroup(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnChangeCompareTolerance();
public:
	void RefreshListCtrl();
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
};
