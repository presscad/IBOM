
// MainFrm.h : CMainFrame 类的接口
//

#pragma once
#include "DialogPanel.h"
#include "FileTreeDlg.h"
#include "IBOMView.h"
class CMainFrame : public CFrameWndEx
{
	
protected: // 仅从序列化创建
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

public:
	CFileTreeDlg* GetFileTreePage(){return (CFileTreeDlg*)m_fileTreeView.GetDlgPtr();}
// 重写
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL LoadFrame(UINT nIDResource, DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, CWnd* pParentWnd = NULL, CCreateContext* pContext = NULL);

// 实现
public:
	virtual ~CMainFrame();
	void ActivateDockpage(CRuntimeClass *pRuntimeClass);
	void ModifyDockpageStatus(CRuntimeClass *pRuntimeClass, BOOL bShow);
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // 控件条嵌入成员
	CMFCMenuBar       m_wndMenuBar;
	CMFCToolBar		  m_wndToolBar;
	CMFCToolBar		  m_wndPictureToolBar;
	CMFCStatusBar     m_wndStatusBar;
	CMFCToolBarImages m_UserImages;
	CDialogPanel		m_fileTreeView;		
// 生成的消息映射函数
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnViewCustomize();
	afx_msg LRESULT OnToolbarCreateNew(WPARAM wp, LPARAM lp);
	afx_msg void OnApplicationLook(UINT id);
	afx_msg void OnUpdateApplicationLook(CCmdUI* pCmdUI);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg void OnSnap();
	afx_msg void OnUpdateSnap(CCmdUI *pCmdUI);
	afx_msg void OnMove();
	afx_msg void OnUpdateMove(CCmdUI *pCmdUI);
	afx_msg void OnZoom();
	afx_msg void OnUpdateZoom(CCmdUI *pCmdUI);
	afx_msg void OnRunDBP();
	afx_msg void OnUpdateRunDBP(CCmdUI *pCmdUI);
	afx_msg void OnLicenseCenter();
	afx_msg LRESULT OnRefreshScannerFileToTree(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

	BOOL CreateDockingWindows();
public:
	afx_msg void OnSetCadPath();
};


