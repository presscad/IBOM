#pragma once
#include "PropertyList.h"
#include "PropListItem.h"
// CSettingDlg �Ի���

class CSettingDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSettingDlg)

	void DisplaySystemSetting();
	void DisplayImageFileProp();
	void DisplayBomFileProp();
	void InitBomFilePropHashTbl();
	void InitImageFilePropHashTbl();
	void InitSystemSettingPropHashTbl();
	int GetImageFilePropValueStr(long id, char *valueStr,UINT nMaxStrBufLen=100,CPropTreeItem *pItem=NULL);	//��������ID�õ�����ֵ
	int GetBomFilePropValueStr(long id, char *valueStr,UINT nMaxStrBufLen=100,CPropTreeItem *pItem=NULL);	//��������ID�õ�����ֵ
public:
	static int m_iCurrentTabPage;
public:
	CSettingDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CSettingDlg();
	CImageFileHost *m_pImageFile;
	CDataCmpModel *m_pModel;
// �Ի�������
	enum { IDD = IDD_SETTING_DLG };
	CPropertyList	m_propList;
	CTabCtrl	m_ctrlPropGroup;
	DECLARE_PROP_FUNC(CSettingDlg)
	int GetPropValueStr(long id, char *valueStr,UINT nMaxStrBufLen=100,CPropTreeItem *pItem=NULL);	//��������ID�õ�����ֵ
	void DisplayPropSetting();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnDefault();
	afx_msg void OnSelchangeTabGroup(NMHDR* pNMHDR, LRESULT* pResult);
};
