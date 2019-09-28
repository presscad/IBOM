#pragma once
#include "f_ent_list.h"
#include "resource.h"
#include "DataModel.h"
#include "XhTreeCtrl.h"

// CFileTreeDlg 对话框
struct TREEITEM_INFO{
	TREEITEM_INFO(){;}
	TREEITEM_INFO(long type,DWORD dw){itemType=type;dwRefData=dw;}
	long itemType;
	DWORD dwRefData;
};
class CFileTreeDlg : public CDialog
{
	DECLARE_DYNCREATE(CFileTreeDlg)
	CXhTreeCtrl m_xFileCtrl;
	//CTreeCtrl m_xFileCtrl;
	CImageList m_imageList;
	ATOM_LIST<TREEITEM_INFO>itemInfoList;
	TREEITEM_INFO* m_pSelItemInfo;
	HTREEITEM m_hImageGroup,m_hDwgBomGroup;
	HTREEITEM m_hLoftBomGroup,m_hLoftTmaGroup;
	HTREEITEM m_hSelect;
	BOOL bEditState;
	BOOL DeleteItem(HTREEITEM hItem);
	void AppendFile(ARRAY_LIST<CXhChar500> &filePathList,BOOL bDisplayDlg);
public:
	CFileTreeDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CFileTreeDlg();

// 对话框数据
	enum { IDD = IDD_FILE_TREE_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnOK();
	virtual void OnCancel();
	void MoveItem(int head0_up1_down2_tail3);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnNMClickTreeCtrl(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRClickTreeCtrl(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBeginLabelEditTree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEndLabelEditTree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnSelchangedTreeCtrl(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnCompareData();
	afx_msg void OnExportCompResult();
	afx_msg void OnImportImageFile();
	afx_msg void OnImportImageFolder();
	afx_msg void OnImportLoftBom();
	afx_msg void OnAddSegData();
	afx_msg void OnRenameItem();
	afx_msg void OnImportDwgBom();
	afx_msg void OnRetrieveData();
	afx_msg void OnExportImageFile();
	afx_msg void OnDeleteItem();
	afx_msg void OnExportDwgExcel();
	afx_msg void OnMoveToTop();
	afx_msg void OnMoveUp();
	afx_msg void OnMoveDown();
	afx_msg void OnMoveToBottom();
	afx_msg void OnEditItemProp();
public:
	void ContextMenu(CWnd *pWnd, CPoint point);
	void RefreshTreeCtrl();
	void RefreshImageTreeCtrl();
	void InsertImageGroup(HTREEITEM hParentItem);
	void InsertDwgBomGroup(HTREEITEM hParentItem);
	void InsertLoftBomGroup(HTREEITEM hParentItem);
	HTREEITEM InsertImageSegItem(SEGITEM *pSegItem);
	void RefreshView();
	BOOL GetSelItemInfo();
	HTREEITEM FindItem(const char* strText); 
	void RetrieveData();
};
