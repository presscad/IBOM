
// IBOMView.cpp : CIBOMView 类的实现
//
#include "stdafx.h"
#include "MainFrm.h"
#include "FileTreeDlg.h"
#include "SettingDlg.h"
#include "ScannerLoader.h"
#include "LicFuncDef.h"
#include "XhLicAgent.h"

#ifndef SHARED_HANDLERS
#include "IBOM.h"
#endif

#include "IBOMDoc.h"
#include "IBOMView.h"
#include "DataModel.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CIBOMView

IMPLEMENT_DYNCREATE(CIBOMView, CView)

BEGIN_MESSAGE_MAP(CIBOMView, CView)
	// 标准打印命令
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_COMMAND(ID_IMMERSE_MODE, &CIBOMView::OnImmerseMode)
	ON_UPDATE_COMMAND_UI(ID_IMMERSE_MODE, &CIBOMView::OnUpdateImmerseMode)
	ON_COMMAND(ID_TYPEWRITER_MODE, &CIBOMView::OnTypewriterMode)
	ON_UPDATE_COMMAND_UI(ID_TYPEWRITER_MODE, &CIBOMView::OnUpdateTypewriterMode)
	ON_COMMAND(ID_RETRIEVE_IMAGE_DATA,&CIBOMView::OnRetrieveData)
	ON_COMMAND(ID_TURN_CLOCKWISE90,&CIBOMView::OnTurnClockwise90)
	ON_COMMAND(ID_TURN_ANTICLOCKWISE90,&CIBOMView::OnTurnAntiClockwise90)
	ON_COMMAND(ID_START_CALC,&CIBOMView::OnStartCalc)
	ON_COMMAND(ID_PICK_POINT_1,&CIBOMView::OnPickPoint1)
	ON_COMMAND(ID_PICK_POINT_2,&CIBOMView::OnPickPoint2)
	ON_COMMAND(ID_PICK_POINT_3,&CIBOMView::OnPickPoint3)
	ON_COMMAND(ID_PICK_POINT_4,&CIBOMView::OnPickPoint4)
	ON_UPDATE_COMMAND_UI(ID_PICK_POINT_1, &CIBOMView::OnUpdatePickPoint1)
	ON_UPDATE_COMMAND_UI(ID_PICK_POINT_2, &CIBOMView::OnUpdatePickPoint2)
	ON_UPDATE_COMMAND_UI(ID_PICK_POINT_3, &CIBOMView::OnUpdatePickPoint3)
	ON_UPDATE_COMMAND_UI(ID_PICK_POINT_4, &CIBOMView::OnUpdatePickPoint4)
	ON_COMMAND(ID_SETTING, &CIBOMView::OnSetting)
	ON_COMMAND(ID_OUTPUT_TBOM, &CIBOMView::OnOutputTbom)
	ON_COMMAND(ID_FILE_PROP, &CIBOMView::OnFileProp)
END_MESSAGE_MAP()

// CIBOMView 构造/析构

CIBOMView::CIBOMView()
{
	// TODO: 在此处添加构造代码
	m_cPreType=-1;
	m_iCurPinpoint=-1;
}

CIBOMView::~CIBOMView()
{
}

BOOL CIBOMView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: 在此处通过修改
	//  CREATESTRUCT cs 来修改窗口类或样式

	return CView::PreCreateWindow(cs);
}
void CIBOMView::DoDataExchange(CDataExchange* pDX)
{
	CView::DoDataExchange(pDX);
}
void CIBOMView::OnInitialUpdate()
{
	CView::OnInitialUpdate();
}
void CIBOMView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CFileTreeDlg* pTreePage=((CMainFrame*)AfxGetMainWnd())->GetFileTreePage();
	if(pTreePage)
		pTreePage->RefreshTreeCtrl();
}
// CIBOMView 绘制
void CIBOMView::OnDraw(CDC* /*pDC*/)
{
	CIBOMDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;
}

// CIBOMView 诊断
#ifdef _DEBUG
void CIBOMView::AssertValid() const
{
	CView::AssertValid();
}

void CIBOMView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CIBOMDoc* CIBOMView::GetDocument() const // 非调试版本是内联的
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CIBOMDoc)));
	return (CIBOMDoc*)m_pDocument;
}
#endif //_DEBUG

void CIBOMView::ShiftView(BYTE cType,DWORD dwRefData/*=NULL*/)
{
	if(cType==CDataCmpModel::DATA_CMP_MODEL)
	{
		if(m_cPreType!=CDataCmpModel::DATA_CMP_MODEL)
		{
			m_xDisplayDataPage.ShowWindow(SW_HIDE);
			m_xOutPutInfoPage.ShowWindow(SW_SHOWNOACTIVATE);
		}
		m_xOutPutInfoPage.RefreshListCtrl();
	}
	else
	{
		if(m_cPreType==CDataCmpModel::DATA_CMP_MODEL)
		{
			m_xOutPutInfoPage.ShowWindow(SW_HIDE);
			m_xDisplayDataPage.ShowWindow(SW_SHOWNOACTIVATE);
		}
		if(dwRefData==NULL)
			dwRefData=m_xDisplayDataPage.m_pData;
		m_xDisplayDataPage.InitPage(cType,dwRefData);
	}
	m_cPreType=cType;
}
void CIBOMView::SetCurTask(int iCurTask)
{
	if(m_xDisplayDataPage.GetSafeHwnd())
		m_xDisplayDataPage.SetCurTask(iCurTask);
}
CDisplayDataPage* CIBOMView::GetDataPage()
{
	if(m_xDisplayDataPage.GetSafeHwnd())
		return &m_xDisplayDataPage;
	else
		return NULL;
}
// CIBOMView 消息处理程序
//
void CIBOMView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);
	CRect rect;
	GetClientRect(&rect);
	if(m_xDisplayDataPage.GetSafeHwnd())
		m_xDisplayDataPage.MoveWindow(&rect);
	if(m_xOutPutInfoPage.GetSafeHwnd())
		m_xOutPutInfoPage.MoveWindow(&rect);
}
int CIBOMView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	m_xDisplayDataPage.Create(IDD_DISP_DATA_PAGE,this);
	m_xOutPutInfoPage.Create(IDD_OUTPUT_INFO_DLG,this);
	ShiftView(CDataCmpModel::DATA_CMP_MODEL);
	return 0;
}

void CIBOMView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if((m_xDisplayDataPage.biLayoutType==3||m_xDisplayDataPage.biLayoutType==4)&&nChar!=VK_DELETE)
	{
		m_xDisplayDataPage.PostMessage(WM_KEYDOWN,nChar,nFlags);
	}
}

void CIBOMView::OnImmerseMode()
{
	CConfig::m_bImmerseMode = !CConfig::m_bImmerseMode;
	ShiftView(m_xDisplayDataPage.m_cDisType,m_xDisplayDataPage.m_pData);
}


void CIBOMView::OnUpdateImmerseMode(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(CConfig::m_bImmerseMode);
}


void CIBOMView::OnTypewriterMode()
{
	CConfig::m_bTypewriterMode = !CConfig::m_bTypewriterMode;
	ShiftView(m_xDisplayDataPage.m_cDisType,m_xDisplayDataPage.m_pData);
}


void CIBOMView::OnUpdateTypewriterMode(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(CConfig::m_bTypewriterMode);
}

void CIBOMView::OnRetrieveData()
{
	CFileTreeDlg *pFilePage=((CMainFrame*)AfxGetMainWnd())->GetFileTreePage();
	pFilePage->RetrieveData();
}

void CIBOMView::OnTurnClockwise90()
{
	if(m_xDisplayDataPage.biLayoutType!=1)
		return;
	CImageFileHost *pImageFile=(CImageFileHost*)m_xDisplayDataPage.m_pData;
	if(pImageFile==NULL)
		return;
	pImageFile->TurnClockwise90();
	m_xDisplayDataPage.ClearImage();
	m_xDisplayDataPage.Invalidate();
}

void CIBOMView::OnTurnAntiClockwise90()
{
	if(m_xDisplayDataPage.biLayoutType!=1)
		return;
	CImageFileHost *pImageFile=(CImageFileHost*)m_xDisplayDataPage.m_pData;
	if(pImageFile==NULL)
		return;
	pImageFile->TurnAntiClockwise90();
	m_xDisplayDataPage.ClearImage();
	m_xDisplayDataPage.Invalidate();
}

void CIBOMView::PickPoint(int index)
{
	if(m_xDisplayDataPage.biLayoutType!=1)
		return;
	CImageFileHost *pImageFile=(CImageFileHost*)m_xDisplayDataPage.m_pData;
	if(pImageFile==NULL||m_xDisplayDataPage.cPolyLineState!=POLYLINE_4PT::STATE_VALID)
		return;
	if(index<0||index>=4)
		return;
	m_iCurPinpoint=index;
	m_xDisplayDataPage.ZoomInPolyPointAt(index);

}
void CIBOMView::OnPickPoint1()
{
	PickPoint(0);
}

void CIBOMView::OnPickPoint2()
{
	PickPoint(1);
}
void CIBOMView::OnPickPoint3()
{
	PickPoint(2);
}
void CIBOMView::OnPickPoint4()
{
	PickPoint(3);
}
void CIBOMView::OnUpdatePickPoint1(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_iCurPinpoint==0);
}
void CIBOMView::OnUpdatePickPoint2(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_iCurPinpoint==1);
}
void CIBOMView::OnUpdatePickPoint3(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_iCurPinpoint==2);
}
void CIBOMView::OnUpdatePickPoint4(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_iCurPinpoint==3);
}

BOOL CIBOMView::IsCanPinpoint()
{
	int iCurPt=GetPinpointIndex();
	return (iCurPt>=0&&iCurPt<4);
}

void CIBOMView::OnSetting()
{
	CSettingDlg dlg;
	dlg.DoModal();
}

void CIBOMView::OnStartCalc()
{
#ifdef __TEST_
	OnTest();
#else
	WinExec("calc.exe",SW_SHOW);
#endif
}

void CIBOMView::OnOutputTbom()
{
	CString filter="Tower(*.tbom)|*.tbom||";
	CFileDialog savedlg(FALSE,"tbom","tower.tbom",OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,filter);
	savedlg.m_ofn.lpstrTitle="导出LTMA放样软件接口文件";
	if(savedlg.DoModal()!=IDOK)
		return;
	dataModel.ExportTmaBomFile(savedlg.GetPathName());
}

void CIBOMView::OnFileProp()
{
	CSettingDlg dlg;
	dlg.m_pModel=&dataModel;
	dlg.DoModal();
}