#pragma once
// CDisplayDdataPage 对话框
#include "supergridctrl.h"
#include "DataModel.h"
#include "ImageHost.h"
#include "InputPartInfoDlg.h"
#include "afxwin.h"
#include "Config.h"
#include "LinkLabel.h"

struct PIXEL_TRANS{
	POINT xOrg;				//局部像素坐标系原点在全局像素坐标系中的坐标
	double fScale2Global;	//局部像素坐标系与全局像素尺度之比
	PIXEL_TRANS(int xiOrgX=0,int yiOrgY=0,double scaleof_local2global=1.0)
	{
		xOrg.x=xiOrgX;
		xOrg.y=yiOrgY;
		fScale2Global=scaleof_local2global;
	}
	POINT TransToLocal(const POINT& xPixelGlobal);
	POINT TransToGlobal(const POINT& xPixelLocal);
};
class CDisplayDataPage : public CDialog
{
	BOOL m_bCanPasteItem;
	double m_fZoomCoefOfScrX;
	DECLARE_DYNCREATE(CDisplayDataPage)
	PIXEL_TRANS m_xCurrTrans;	//当前显示模式下的像素转换
	const static int HEAD_FIX_ROW_COUNT=10;
	const static int TAIL_FIX_ROW_COUNT=50;
public:
	static const int TASK_OTHER = 0;
	static const int TASK_SNAP  = 1;	//图片捕捉
	static const int TASK_MOVE  = 2;	//图片移动
	static const int TASK_ZOOM  = 3;	//图片缩放

	//
	static const BYTE COL_LABEL		= 0;
	static const BYTE COL_MATERIAL	= 1;
	static const BYTE COL_SPEC		= 2;
	static const BYTE COL_LEN		= 3;
	static const BYTE COL_COUNT		= 4;
	static const BYTE COL_WEIGHT	= 5;
	static const BYTE COL_SUM_WEIGHT = 6;
	static const BYTE COL_MAP_SUM_WEIGHT = 7;
	//
	static const BYTE LAYOUT_BROWSE = 2;	//数据区域信息(查看模式)
	static const BYTE LAYOUT_COLREV = 3;	//数据区域按列查看校审模式
private:
	BYTE m_cCurTask;
	BYTE m_biLayoutType;	//0.构件信息 1.图片信息 2.数据区域信息(查看模式) 3.按列查看 4.按行查看
	BYTE m_biCurCol;		//按列校审是当前校审列
	BOOL m_bStartDrag;
	BOOL m_bHorzTracing;
	CPoint m_ptLBDownPos;
	RECT m_rcClient;
	HCURSOR m_hCursor;	//当前使用的光标类型
	HCURSOR m_hArrowCursor;
	HCURSOR m_hPin;
	HCURSOR m_hHorzSize;
	HCURSOR m_hMove;
	HCURSOR m_hSnap;
	HCURSOR m_hCross;
	BOOL m_bTraced;
	int m_nOldHorzY,m_nInputDlgH;
	double m_fDividerYScale;
	POLYLINE_4PT m_xPolyline;
	IMAGE_DATA_EX m_xCurrImageData;
	IMAGE_DATA_EX m_xCurrImageData2;
	CImageFileHost* GetActiveImageFile();
	__declspec(property(get=GetActiveImageFile)) CImageFileHost* pActiveImgFile;
public:
	CInputPartInfoDlg m_xInputPartInfoPage;
	CSuperGridCtrl	m_listDataCtrl,m_listDataCtrl1,m_listFileCtrl;
	CSliderCtrl m_sliderBalanceCoef;
	CSliderCtrl m_sliderZoomScale;
	int m_nRecordNum;
	//
	BYTE m_cDisType;
	DWORD m_pData;
	//
	const static int REV_COL_COUNT=5;
	const static int COL_SUM_COUNT=8;
	static long m_arrColWidth[8];
	static int GetHeadFixRowCount(){return CConfig::m_bTypewriterMode?HEAD_FIX_ROW_COUNT:0;}
	static int GetTailFixRowCount(){return CConfig::m_bTypewriterMode?TAIL_FIX_ROW_COUNT:0;}
	const static int LISTCTRL_HEAD_ITEM=-1;
	const static int LISTCTRL_TAIL_ITEM=0xEFFFFFFF;
	int GetHeadOrTailValidItem(CListCtrl *pListCtrl,int iCurSel=-1);
	void InitPolyLine();
	BYTE PolyLineState(){return m_xPolyline.m_cState;}
	__declspec(property(get=PolyLineState)) BYTE cPolyLineState;
	CPoint* GetPolyLinePtArr(){return m_xPolyline.m_ptArr;}
	bool ZoomInPolyPointAt(int index);
	bool ZoomAll();
	BOOL MarkDrawingError(BYTE flag);
	BOOL IsCanPasteItem(){return m_bCanPasteItem;}
public:
	CDisplayDataPage(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CDisplayDataPage();
	void ClearImage();
	void RelayoutWnd();
	void ShiftCheckColumn(int iCol);
	void InitPage(BYTE cType,DWORD dwRefData);
	void HideInputCtrl();
	bool Focus(CSuperGridCtrl *pListCtrl,CSuperGridCtrl::CTreeItem* pItem);
	CSuperGridCtrl::CTreeItem* RefreshListCtrl(long idSelProp=0);
	int StretchBlt(HDC hDC,CImageFileHost* pImageFile);
	int StretchBlt(CDC* pDC,CImageRegionHost *pRegionHost,RECT& rcPanel,PIXEL_TRANS* pPixelTranslator=NULL,BOOL bSubWorkPanel=FALSE);
	CWnd* GetWorkPanelRect(CRect& rect);
	void InvertLine(CDC* pDC,CPoint ptFrom,CPoint ptTo,RECT* pClipRect=NULL);
	void InvertEllipse(CDC* pDc,CPoint center);
	static void DrawClipLine(CDC* pDC,CPoint from,CPoint to,RECT* pClipRect=NULL);
	POINT TransPoint(CPoint pt,BOOL bTransFromCanvasPanel,BOOL bPanelDC=FALSE);
	POINT IntelliAnchorCrossHairPoint(POINT point,BYTE ciLT0RT1RB2LB3,double *pfTurnGradientToOrthoImg=NULL);
	void SetCurTask(int iCurTask);
	void OperOther();
	BYTE GetCurTask(){return m_cCurTask;}
	BYTE get_biLayoutType(){return m_biLayoutType;}
	__declspec(property(get=get_biLayoutType)) BYTE biLayoutType;
	BYTE get_biCurCol(){return m_biCurCol;}
	__declspec(property(get=get_biCurCol)) BYTE biCurCol;
	void InvalidateImageWorkPanel();
// 对话框数据
	enum { IDD = IDD_DISP_DATA_PAGE };
	
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	DECLARE_MESSAGE_MAP()
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPaint();
	afx_msg void OnOK();
	afx_msg void OnCancel();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
public:
	afx_msg void OnNMCustomdrawSliderBalanceCoef(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMCustomdrawSliderZoomScale(NMHDR *pNMHDR, LRESULT *pResult);
	CStatic m_balanceCoef;
	CComboBox m_cmdModel;
	afx_msg void OnCbnSelchangeCmbModel();
	int m_iModel;
	afx_msg void OnMouseLeave();
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRetrieveData();
	afx_msg void OnSelectFolder();
	afx_msg void OnMarkNormal();
	afx_msg void OnMarkError();
	afx_msg void OnAddPart();
	afx_msg void OnDeletePart();
	afx_msg void OnCopyItem();
	afx_msg void OnPasteItem();
	afx_msg void OnNMReleasedcaptureSliderBalanceCoef(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMReleasedcaptureSliderZoomScale(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposSliderSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSelchangeTabGroup(NMHDR* pNMHDR, LRESULT* pResult);
	CString m_sSumWeight;
	CColorLabel m_ctrlWeightLabel;
	CTabCtrl m_tabCtrl;
};
