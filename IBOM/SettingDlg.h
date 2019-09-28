#pragma once
#include "PropertyList.h"
#include "PropListItem.h"
// CSettingDlg 对话框

class CSettingDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSettingDlg)

	void DisplaySystemSetting();
	void DisplayImageFileProp();
	void DisplayBomFileProp();
	void InitBomFilePropHashTbl();
	void InitImageFilePropHashTbl();
	void InitSystemSettingPropHashTbl();
	int GetImageFilePropValueStr(long id, char *valueStr,UINT nMaxStrBufLen=100,CPropTreeItem *pItem=NULL);	//根据属性ID得到属性值
	int GetBomFilePropValueStr(long id, char *valueStr,UINT nMaxStrBufLen=100,CPropTreeItem *pItem=NULL);	//根据属性ID得到属性值
public:
	static int m_iCurrentTabPage;
public:
	CSettingDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSettingDlg();
	CImageFileHost *m_pImageFile;
	CDataCmpModel *m_pModel;
// 对话框数据
	enum { IDD = IDD_SETTING_DLG };
	CPropertyList	m_propList;
	CTabCtrl	m_ctrlPropGroup;
	DECLARE_PROP_FUNC(CSettingDlg)
	int GetPropValueStr(long id, char *valueStr,UINT nMaxStrBufLen=100,CPropTreeItem *pItem=NULL);	//根据属性ID得到属性值
	void DisplayPropSetting();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnDefault();
	afx_msg void OnSelchangeTabGroup(NMHDR* pNMHDR, LRESULT* pResult);
};
