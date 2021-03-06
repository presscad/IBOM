
// MainFrm.cpp : CMainFrame 类的实现
//

#include "stdafx.h"
#include "IBOM.h"
#include "MainFrm.h"
#include "Tool.h"
#include "RunDBPDlg.h"
#include "ScannerLoader.h"
#include "XhLicAgent.h"
#include "XhLmd.h"
#include "RunDBPDlg.h"
#include "Splash.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWndEx)

const int  iMaxUserToolbars = 10;
const UINT uiFirstUserToolBarId = AFX_IDW_CONTROLBAR_FIRST + 40;
const UINT uiLastUserToolBarId = uiFirstUserToolBarId + iMaxUserToolbars - 1;

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
	ON_WM_CREATE()
	ON_COMMAND(ID_VIEW_CUSTOMIZE, &CMainFrame::OnViewCustomize)
	ON_REGISTERED_MESSAGE(AFX_WM_CREATETOOLBAR, &CMainFrame::OnToolbarCreateNew)
	ON_COMMAND_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_WINDOWS_7, &CMainFrame::OnApplicationLook)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_WINDOWS_7, &CMainFrame::OnUpdateApplicationLook)
	ON_WM_SETTINGCHANGE()
	ON_COMMAND(ID_SNAP, &CMainFrame::OnSnap)
	ON_UPDATE_COMMAND_UI(ID_SNAP, &CMainFrame::OnUpdateSnap)
	ON_COMMAND(ID_MOVE, &CMainFrame::OnMove)
	ON_UPDATE_COMMAND_UI(ID_MOVE, &CMainFrame::OnUpdateMove)
	ON_COMMAND(ID_ZOOM, &CMainFrame::OnZoom)
	ON_UPDATE_COMMAND_UI(ID_ZOOM, &CMainFrame::OnUpdateZoom)
	ON_COMMAND(ID_RUN_DBP, &CMainFrame::OnRunDBP)
	ON_UPDATE_COMMAND_UI(ID_RUN_DBP, &CMainFrame::OnUpdateRunDBP)
	ON_MESSAGE(WM_REFRESH_SCANNER_FILE_TO_TREE,OnRefreshScannerFileToTree)
	ON_COMMAND(ID_LICENSE_CENTER, &CMainFrame::OnLicenseCenter)
	ON_COMMAND(ID_SET_CAD_PATH, &CMainFrame::OnSetCadPath)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // 状态行指示器
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

// CMainFrame 构造/析构

CMainFrame::CMainFrame()
{
	// TODO: 在此添加成员初始化代码
	theApp.m_nAppLook = ID_VIEW_APPLOOK_WINDOWS_7;//theApp.GetInt(_T("ApplicationLook"), ID_VIEW_APPLOOK_VS_2008);
	m_fileTreeView.Init(RUNTIME_CLASS(CFileTreeDlg), IDD_FILE_TREE_DLG);
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
		return -1;
	// 基于持久值设置视觉管理器和样式
	OnApplicationLook(theApp.m_nAppLook);
	//菜单栏的创建
	if (!m_wndMenuBar.Create(this,AFX_DEFAULT_TOOLBAR_STYLE,IDR_MAINFRAME))
	{
		TRACE0("未能创建菜单栏\n");
		return -1;      // 未能创建
	}
	m_wndMenuBar.SetPaneStyle(m_wndMenuBar.GetPaneStyle() | CBRS_SIZE_FIXED | CBRS_TOOLTIPS | CBRS_TOP);
	// 防止菜单栏在激活时获得焦点
	CMFCPopupMenu::SetForceMenuFocus(FALSE);
	//工具栏的创建
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC)||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("未能创建工具栏\n");
		return -1;      // 未能创建
	}
	if (!m_wndPictureToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC,CRect(1,1,1,1),IDR_PICTURE_TOOL) ||
		!m_wndPictureToolBar.LoadToolBar(IDR_PICTURE_TOOL))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}
	
	CString strToolBarName;
	//BOOL bNameValid = strToolBarName.LoadString(IDS_TOOLBAR_STANDARD);
	BOOL bNameValid = strToolBarName.LoadString(IDR_MAINFRAME);
	ASSERT(bNameValid);
	m_wndToolBar.SetWindowText(strToolBarName);
	m_wndToolBar.SetBorders ();
	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);
	//在此处可以创建用户定义的工具栏

	// 允许用户定义的工具栏操作:
	InitUserToolbars(NULL, uiFirstUserToolBarId, uiLastUserToolBarId);
	//状态栏的创建
	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("未能创建状态栏\n");
		return -1;      // 未能创建
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT));
	// TODO: 如果您不希望工具栏和菜单栏可停靠，请删除这五行
	AddPane(&m_wndMenuBar);	//停靠菜单
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndPictureToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	//EnableAutoHidePanes(CBRS_ALIGN_ANY);
	//SetMenuBarState(AFX_MBS_VISIBLE);

	DockPane(&m_wndToolBar);
	DockPane(&m_wndPictureToolBar);
	DockPaneLeftOf(&m_wndToolBar,&m_wndPictureToolBar);
	// 启用 Visual Studio 2005 样式停靠窗口行为
	CDockingManager::SetDockingMode(DT_SMART);
	// 启用 Visual Studio 2005 样式停靠窗口自动隐藏行为
	EnableAutoHidePanes(CBRS_ALIGN_ANY);

	// 加载菜单项图像(不在任何标准工具栏上):
	//CMFCToolBar::AddToolBarForImageCollection(IDR_MENU_IMAGES, theApp.m_bHiColorIcons ? IDB_MENU_IMAGES_24 : 0);

	// 创建停靠窗口并进行布局
	if (!CreateDockingWindows())
	{
		TRACE0("未能创建停靠窗口\n");
		return -1;
	}
	m_fileTreeView.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_fileTreeView);
	// 启用工具栏和停靠窗口菜单替换
	EnablePaneMenu(TRUE, ID_VIEW_CUSTOMIZE, strCustomize, ID_VIEW_TOOLBAR);

	// 启用快速(按住 Alt 拖动)工具栏自定义
	CMFCToolBar::EnableQuickCustomization();

	if (CMFCToolBar::GetUserImages() == NULL)
	{
		// 加载用户定义的工具栏图像
		/*if (m_UserImages.Load(_T(".\\res\\UserImages.bmp")))
		{
			CMFCToolBar::SetUserImages(&m_UserImages);
		}*/
	}

	// 启用菜单个性化(最近使用的命令)
	// TODO: 定义您自己的基本命令，确保每个下拉菜单至少有一个基本命令。
	CList<UINT, UINT> lstBasicCommands;

	lstBasicCommands.AddTail(ID_FILE_NEW);
	lstBasicCommands.AddTail(ID_FILE_OPEN);
	lstBasicCommands.AddTail(ID_FILE_SAVE);
	lstBasicCommands.AddTail(ID_FILE_PRINT);
	lstBasicCommands.AddTail(ID_APP_EXIT);
	lstBasicCommands.AddTail(ID_EDIT_CUT);
	lstBasicCommands.AddTail(ID_EDIT_PASTE);
	lstBasicCommands.AddTail(ID_EDIT_UNDO);
	lstBasicCommands.AddTail(ID_APP_ABOUT);
	lstBasicCommands.AddTail(ID_VIEW_STATUS_BAR);
	lstBasicCommands.AddTail(ID_VIEW_TOOLBAR);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2003);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_VS_2005);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_BLUE);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_SILVER);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_BLACK);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_AQUA);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_WINDOWS_7);
	lstBasicCommands.AddTail(ID_SORTING_SORTALPHABETIC);
	lstBasicCommands.AddTail(ID_SORTING_SORTBYTYPE);
	lstBasicCommands.AddTail(ID_SORTING_SORTBYACCESS);
	lstBasicCommands.AddTail(ID_SORTING_GROUPBYTYPE);
	//不隐藏不常用菜单，即显示所有菜单 wht 18-07-27
	//CMFCToolBar::SetBasicCommands(lstBasicCommands);
	CSplashWnd::ShowSplashScreen(this);

	CString szTitle=this->GetTitle();
	if(theApp.IsHasInternalTest())
	{
#ifdef _DEBUG
		szTitle.Append("(内测版Debug)");	//用于调试程序时区分Release进程还是Debug进程 wjh-2018.11.22
#else
		szTitle.Append("(内测版)");
#endif
	}
	SetTitle(szTitle);
	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWndEx::PreCreateWindow(cs) )
		return FALSE;
	// TODO: 在此处通过修改
	//  CREATESTRUCT cs 来修改窗口类或样式

	return TRUE;
}
static BOOL CreateDockingWindow(CWnd *pParentWnd,UINT nDlgID,UINT nViewNameID,CDialogPanel &dlgPanel,
	DWORD dwPosStyle,int nWidth=200,int nHeight=200)
{
	CString sViewName="";
	BOOL bNameValid = sViewName.LoadString(nViewNameID);
	ASSERT(bNameValid);
	if (!dlgPanel.Create(sViewName, pParentWnd, CRect(0, 0, nWidth, nHeight), TRUE, nDlgID,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | dwPosStyle | CBRS_FLOAT_MULTI))
	{
		TRACE0("未能创建“"+sViewName+"”窗口\n");
		return FALSE;
	}
	return TRUE;
}

static void SetDockingWindowIcon(CDialogPanel &dlgPanel,UINT nIdHC,UINT nCommonId,BOOL bHiColorIcons)
{
	HICON hViewIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), 
		MAKEINTRESOURCE(bHiColorIcons ? nIdHC : nCommonId), 
		IMAGE_ICON,::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
	dlgPanel.SetIcon(hViewIcon, FALSE);
}
BOOL CMainFrame::CreateDockingWindows()
{
	CreateDockingWindow(this,IDD_FILE_TREE_DLG,IDS_FILE_CTRL_VIEW,m_fileTreeView,CBRS_LEFT);
	return TRUE;
}
//
void CMainFrame::ActivateDockpage(CRuntimeClass *pRuntimeClass)
{
	ModifyDockpageStatus(pRuntimeClass,TRUE);
}
void CMainFrame::ModifyDockpageStatus(CRuntimeClass *pRuntimeClass, BOOL bShow)
{
	if(m_fileTreeView.GetDlgPtr()->IsKindOf(pRuntimeClass))
		m_fileTreeView.ShowPane(bShow,FALSE,TRUE);
}
// CMainFrame 诊断

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWndEx::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWndEx::Dump(dc);
}
#endif //_DEBUG


// CMainFrame 消息处理程序

void CMainFrame::OnViewCustomize()
{
	CMFCToolBarsCustomizeDialog* pDlgCust = new CMFCToolBarsCustomizeDialog(this, TRUE /* 扫描菜单*/);
	pDlgCust->EnableUserDefinedToolbars();
	pDlgCust->Create();
}

LRESULT CMainFrame::OnToolbarCreateNew(WPARAM wp,LPARAM lp)
{
	LRESULT lres = CFrameWndEx::OnToolbarCreateNew(wp,lp);
	if (lres == 0)
	{
		return 0;
	}

	CMFCToolBar* pUserToolbar = (CMFCToolBar*)lres;
	ASSERT_VALID(pUserToolbar);

	BOOL bNameValid;
	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);

	pUserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
	return lres;
}

void CMainFrame::OnApplicationLook(UINT id)
{
	CWaitCursor wait;

	theApp.m_nAppLook = id;

	switch (theApp.m_nAppLook)
	{
	case ID_VIEW_APPLOOK_WIN_2000:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManager));
		break;

	case ID_VIEW_APPLOOK_OFF_XP:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOfficeXP));
		break;

	case ID_VIEW_APPLOOK_WIN_XP:
		CMFCVisualManagerWindows::m_b3DTabsXPTheme = TRUE;
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
		break;

	case ID_VIEW_APPLOOK_OFF_2003:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2003));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	case ID_VIEW_APPLOOK_VS_2005:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2005));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	case ID_VIEW_APPLOOK_VS_2008:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2008));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	case ID_VIEW_APPLOOK_WINDOWS_7:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows7));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	default:
		switch (theApp.m_nAppLook)
		{
		case ID_VIEW_APPLOOK_OFF_2007_BLUE:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_LunaBlue);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_BLACK:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_ObsidianBlack);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_SILVER:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Silver);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_AQUA:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Aqua);
			break;
		}

		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2007));
		CDockingManager::SetDockingMode(DT_SMART);
	}

	RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE);

	theApp.WriteInt(_T("ApplicationLook"), theApp.m_nAppLook);
}

void CMainFrame::OnUpdateApplicationLook(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(theApp.m_nAppLook == pCmdUI->m_nID);
}

BOOL CMainFrame::LoadFrame(UINT nIDResource, DWORD dwDefaultStyle, CWnd* pParentWnd, CCreateContext* pContext) 
{
	// 基类将执行真正的工作

	if (!CFrameWndEx::LoadFrame(nIDResource, dwDefaultStyle, pParentWnd, pContext))
	{
		return FALSE;
	}


	// 为所有用户工具栏启用自定义按钮
	BOOL bNameValid;
	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);

	for (int i = 0; i < iMaxUserToolbars; i ++)
	{
		CMFCToolBar* pUserToolbar = GetUserToolBarByIndex(i);
		if (pUserToolbar != NULL)
		{
			pUserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
		}
	}

	return TRUE;
}


void CMainFrame::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CFrameWndEx::OnSettingChange(uFlags, lpszSection);
}
//
void CMainFrame::OnSnap()
{
	CIBOMView* pView=theApp.GetIBomView();
	pView->SetCurTask(CDisplayDataPage::TASK_SNAP);
}
void CMainFrame::OnUpdateSnap(CCmdUI *pCmdUI)
{
	CIBOMView* pView=theApp.GetIBomView();
	bool bOn=pView->GetDisType()==CDataCmpModel::DWG_IMAGE_DATA;
	pCmdUI->Enable(bOn);
}
void CMainFrame::OnMove()
{
	CIBOMView* pView=theApp.GetIBomView();
	pView->SetCurTask(CDisplayDataPage::TASK_MOVE);
}
void CMainFrame::OnUpdateMove(CCmdUI *pCmdUI)
{
	CIBOMView* pView=theApp.GetIBomView();
	bool bOn=(pView->GetDisType()==CDataCmpModel::DWG_IMAGE_DATA||
				pView->GetDisType()==CDataCmpModel::IMAGE_REGION_DATA);
	pCmdUI->Enable(bOn);
}
void CMainFrame::OnZoom()
{
	CIBOMView* pView=theApp.GetIBomView();
	pView->SetCurTask(CDisplayDataPage::TASK_ZOOM);
}
void CMainFrame::OnUpdateZoom(CCmdUI *pCmdUI)
{
	CIBOMView* pView=theApp.GetIBomView();
	bool bOn=(pView->GetDisType()==CDataCmpModel::DWG_IMAGE_DATA||
				pView->GetDisType()==CDataCmpModel::IMAGE_REGION_DATA);
	pCmdUI->Enable(bOn);
}

static CString GetProductVersion(const char* exe_path)
{
	CString product_version;
	DWORD dwLen=GetFileVersionInfoSize(exe_path,0);
	BYTE *data=new BYTE[dwLen];
	WORD product_ver[4];
	if(GetFileVersionInfo(exe_path,0,dwLen,data))
	{
		VS_FIXEDFILEINFO *info;
		UINT uLen;
		VerQueryValue(data,"\\",(void**)&info,&uLen);
		memcpy(product_ver,&info->dwProductVersionMS,4);
		memcpy(&product_ver[2],&info->dwProductVersionLS,4);
		product_version.Format("%d.%d.%d.%d",product_ver[1],product_ver[0],product_ver[3],product_ver[2]);
	}
	delete data;
	return product_version;
}
void CMainFrame::OnRunDBP()
{
	char cad_path[MAX_PATH]="",cad_name[MAX_PATH]="";
	BOOL bSetCadPath=FALSE;
	FILE* fp=NULL;
	CString m_sRxFile;
	if(ReadCadPath(cad_path))
	{
		sprintf(cad_name,"%sacad.exe",cad_path);
		m_sRxFile.Format("%s",cad_path);
		m_sRxFile+="Acad.rx";
		if((fp=fopen(m_sRxFile,"wt"))==NULL)
			bSetCadPath=TRUE;
	}
	else
		bSetCadPath=TRUE;
	if(bSetCadPath)
	{
		CRunDBPDlg dlg;
		if(dlg.DoModal()!=IDOK)
			return;
		dlg.GetCadPath(cad_path);
		sprintf(cad_name,"%s",dlg.m_sCadPath);
		m_sRxFile.Format("%s",cad_path);
		m_sRxFile+="Acad.rx";

		fp=fopen(m_sRxFile,"wt");
	}
	//启动CAD，加载DBP.arx
	if(fp==NULL)
	{
		AfxMessageBox(CXhChar500("绘图启动文件创建失败,不能启动绘图模块!CAD路径:%s",cad_name));
		return;
	}
	else
	{
		CString sProductVersion=GetProductVersion(cad_name);
		if(sProductVersion.Find("16.")==0)
			fprintf(fp,"%sDBP05.arx\n",APP_PATH);
		else if(sProductVersion.Find("17.")==0)
			fprintf(fp,"%sDBP07.arx\n",APP_PATH);
		else if(sProductVersion.Find("18.")==0)
			fprintf(fp,"%sDBP10.arx\n",APP_PATH);
		fclose(fp);
	}
	//创建进程，启动CAD
	STARTUPINFO startInfo;
	memset( &startInfo, 0 , sizeof( STARTUPINFO ) );
	startInfo.cb= sizeof( STARTUPINFO );
	startInfo.dwFlags=STARTF_USESHOWWINDOW;
	startInfo.wShowWindow=SW_SHOWMAXIMIZED;

	PROCESS_INFORMATION processInfo;
	memset( &processInfo,0,sizeof(PROCESS_INFORMATION) );
	BOOL bRet=CreateProcess(cad_name,"",NULL,NULL,FALSE,0,NULL,NULL,&startInfo,&processInfo);
	if(bRet)
	{
		WaitForInputIdle(processInfo.hProcess,INFINITE);
		Sleep(20000);
	}
	else
		MessageBox(_T("启动cad失败"));
}
void CMainFrame::OnUpdateRunDBP(CCmdUI *pCmdUI)
{
	pCmdUI->Enable();
}

LRESULT CMainFrame::OnRefreshScannerFileToTree(WPARAM wParam, LPARAM lParam)
{
#ifdef __SCANNER_LOADER_
	ARRAY_LIST<CXhChar500> filePathList;
	for(IMAGE_DOWNLOAD *pImageDownload=g_scannerLoader.EnumFirst(FALSE);pImageDownload;pImageDownload=g_scannerLoader.EnumNext(FALSE))
	{
		if(strlen(pImageDownload->sLocalPath)<=0)
			continue;
		CImageFileHost* pImageFile=dataModel.GetImageFile(pImageDownload->sName);
		if(pImageFile)
			continue;	//已存在
		filePathList.append(pImageDownload->sLocalPath);
	}
	CFileTreeDlg *pFilePage=GetFileTreePage();
	if(pFilePage&&filePathList.GetSize()>0)
		pFilePage->AppendFile(filePathList,FALSE);
#endif
	return 0;
}

void CMainFrame::OnLicenseCenter()
{
	ProcessLicenseAuthorize(PRODUCT_IBOM,theApp.m_version,theApp.execute_path,true);
}

void CMainFrame::OnSetCadPath()
{
	CRunDBPDlg dlg;
	dlg.DoModal();
}
