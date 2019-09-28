
// MainFrm.h : CMainFrame ��Ľӿ�
//

#pragma once
#include "DialogPanel.h"
#include "FileTreeDlg.h"
#include "IBOMView.h"
class CMainFrame : public CFrameWndEx
{
	
protected: // �������л�����
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

public:
	CFileTreeDlg* GetFileTreePage(){return (CFileTreeDlg*)m_fileTreeView.GetDlgPtr();}
// ��д
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL LoadFrame(UINT nIDResource, DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, CWnd* pParentWnd = NULL, CCreateContext* pContext = NULL);

// ʵ��
public:
	virtual ~CMainFrame();
	void ActivateDockpage(CRuntimeClass *pRuntimeClass);
	void ModifyDockpageStatus(CRuntimeClass *pRuntimeClass, BOOL bShow);
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // �ؼ���Ƕ���Ա
	CMFCMenuBar       m_wndMenuBar;
	CMFCToolBar		  m_wndToolBar;
	CMFCToolBar		  m_wndPictureToolBar;
	CMFCStatusBar     m_wndStatusBar;
	CMFCToolBarImages m_UserImages;
	CDialogPanel		m_fileTreeView;		
// ���ɵ���Ϣӳ�亯��
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


