#pragma once
// CDisplayDdataPage �Ի���
#include "supergridctrl.h"
#include "DataModel.h"
#include "ImageHost.h"
#include "InputPartInfoDlg.h"
#include "afxwin.h"

struct POLYLINE_4PT{
	CPoint m_ptArr[4];	//���ϡ����ϡ����¡�����
	int m_iActivePt;	//���������
	//
	static const BYTE STATE_NONE		= 0;
	static const BYTE STATE_WAITNEXTPT	= 1;
	static const BYTE STATE_VALID		= 2;
	BYTE m_cState;		//0.δ��ʼ�� 1.�ȴ�ѡ���2�� 2.��Ч
	void Init(){m_iActivePt=-1;m_cState=STATE_NONE;}
	POLYLINE_4PT(){Init();}
};
class CDisplayDataPage : public CDialog
{
	DECLARE_DYNCREATE(CDisplayDataPage)
public:
	static const int TASK_OTHER = 0;
	static const int TASK_SNAP  = 1;	//ͼƬ��׽
	static const int TASK_MOVE  = 2;	//ͼƬ�ƶ�
	static const int TASK_ZOOM  = 3;	//ͼƬ����

	//
	static const BYTE COL_LABEL		= 0;
	static const BYTE COL_MATERIAL	= 1;
	static const BYTE COL_SPEC		= 2;
	static const BYTE COL_LEN		= 3;
	static const BYTE COL_COUNT		= 4;
	static const BYTE COL_WEIGHT	= 5;
private:
	BYTE m_cCurTask;
	BYTE m_biLayoutType;	//0.������Ϣ 1.ͼƬ��Ϣ 2.����������Ϣ(�鿴ģʽ) 3.���в鿴 4.���в鿴
	BYTE m_biCurCol;		//����У���ǵ�ǰУ����
	BOOL m_bStartDrag;
	BOOL m_bHorzTracing;
	CPoint m_ptLBDownPos;
	RECT m_rcClient;
	HCURSOR m_hCursor;	//��ǰʹ�õĹ������
	HCURSOR m_hArrowCursor;
	HCURSOR m_hPin;
	HCURSOR m_hHorzSize;
	HCURSOR m_hMove;
	HCURSOR m_hSnap;
	HCURSOR m_hCross;
	BOOL m_bTraced;
	int m_nOldHorzY,m_nInputDlgH;
	double m_fDividerYScale;
	int m_arrColWidth[7];
public:
	CInputPartInfoDlg m_xInputPartInfoPage;
	CSuperGridCtrl	m_listDataCtrl,m_listDataCtrl1;
	CSliderCtrl m_sliderBalanceCoef;
	CString m_sContext;
	int m_nRecordNum;
	//
	BYTE m_cDisType;
	DWORD m_pData;
	POLYLINE_4PT m_xPolyline;

	const static int HEAD_FIX_ROW_COUNT=10;
	const static int TAIL_FIX_ROW_COUNT=50;
public:
	CDisplayDataPage(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CDisplayDataPage();
	void RelayoutWnd();
	void ShiftCheckColumn(int iCol);
	void InitPage(BYTE cType,DWORD dwRefData);
	bool Focus(RECT& rc,CSuperGridCtrl *pListCtrl);
	void RefreshListCtrl(long idSelProp=0);
	int StretchBlt(HDC hDC,CImageFileHost* pImageFile);
	int StretchBlt(HDC hDC,CImageRegionHost *pRegionHost);
	CWnd* GetWorkPanelRect(CRect& rect);
	void InvertLine(CDC* pDC,CPoint ptFrom,CPoint ptTo,RECT* pClipRect=NULL);
	void InvertEllipse(CDC* pDc,CPoint center);
	void DrawClipLine(CDC* pDC,CPoint from,CPoint to,RECT* pClipRect=NULL);
	POINT TransPoint(CPoint pt,BOOL bDrawToWork,BOOL bPanelDC=FALSE);
	POINT IntelliAnchorCrossHairPoint(POINT point,BYTE ciLT0RT1RB2LB3);
	void SetCurTask(int iCurTask);
	void OperOther();
	BYTE GetCurTask(){return m_cCurTask;}
	BYTE get_biLayoutType(){return m_biLayoutType;}
	__declspec(property(get=get_biLayoutType)) BYTE biLayoutType;
	BYTE get_biCurCol(){return m_biCurCol;}
	__declspec(property(get=get_biCurCol)) BYTE biCurCol;
// �Ի�������
	enum { IDD = IDD_DISP_DATA_PAGE };
	
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
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
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
public:
	afx_msg void OnNMCustomdrawSliderBalanceCoef(NMHDR *pNMHDR, LRESULT *pResult);
	CStatic m_balanceCoef;
	CComboBox m_cmdModel;
	afx_msg void OnCbnSelchangeCmbModel();
	int m_iModel;
	afx_msg void OnMouseLeave();
};
