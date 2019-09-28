
// IBOMView.h : CIBOMView 类的接口
//

#pragma once
#include "IBOMDoc.h"
#include "DisplayDataPage.h"
#include "OutputInfoDlg.h"

class CIBOMView : public CView
{
	CDisplayDataPage m_xDisplayDataPage;
	COutputInfoDlg m_xOutPutInfoPage;
	BYTE m_cPreType;
	int m_iCurPinpoint;
protected: // 仅从序列化创建
	CIBOMView();
	DECLARE_DYNCREATE(CIBOMView)

// 特性
public:
	CIBOMDoc* GetDocument() const;
	void ShiftView(BYTE cType,DWORD dwRefData=NULL);
	void SetCurTask(int iCurTask);
	BYTE GetDisType(){return m_xDisplayDataPage.m_cDisType;}
	CDisplayDataPage* GetDataPage();
	double GetCompareLenTolerance(){return m_xOutPutInfoPage.m_fLenTolerance;}
	double GetCompareWeightTolerance(){return m_xOutPutInfoPage.m_fWeightTolerance;}
// 操作
public:

// 重写
public:
	virtual void OnDraw(CDC* pDC);  // 重写以绘制该视图
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	int GetPinpointIndex(){return m_iCurPinpoint;}
	int SetPinpointIndex(int index){return (m_iCurPinpoint=index);}
	BOOL IsCanPinpoint();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual void OnInitialUpdate(); // 构造后第一次调用
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	void PickPoint(int index);
// 实现
public:
	virtual ~CIBOMView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// 生成的消息映射函数
protected:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnImmerseMode();
	afx_msg void OnUpdateImmerseMode(CCmdUI *pCmdUI);
	afx_msg void OnTypewriterMode();
	afx_msg void OnUpdateTypewriterMode(CCmdUI *pCmdUI);
	afx_msg void OnRetrieveData();
	afx_msg void OnPickPoint1();
	afx_msg void OnPickPoint2();
	afx_msg void OnPickPoint3();
	afx_msg void OnPickPoint4();
	afx_msg void OnUpdatePickPoint1(CCmdUI *pCmdUI);
	afx_msg void OnUpdatePickPoint2(CCmdUI *pCmdUI);
	afx_msg void OnUpdatePickPoint3(CCmdUI *pCmdUI);
	afx_msg void OnUpdatePickPoint4(CCmdUI *pCmdUI);
	afx_msg void OnTurnClockwise90();
	afx_msg void OnTurnAntiClockwise90();
	afx_msg void OnSetting();
	afx_msg void OnStartCalc();
	afx_msg void OnConnectScanner();
	afx_msg void OnOutputTbom();
#ifdef __TEST_
	afx_msg void OnTest();
#endif
	afx_msg void OnFileProp();
};

#ifndef _DEBUG  // IBOMView.cpp 中的调试版本
inline CIBOMDoc* CIBOMView::GetDocument() const
   { return reinterpret_cast<CIBOMDoc*>(m_pDocument); }
#endif

