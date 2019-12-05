
// IBOM.cpp : 定义应用程序的类行为。
//

#include "stdafx.h"
#include <direct.h>
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "IBOM.h"
#include "MainFrm.h"
#include "IBOMDoc.h"
#include "IBOMView.h"
#include "Tool.h"
#include "XhLdsLm.h"
#include "LicFuncDef.h"
#include "XhLmd.h"
#include "Config.h"
#include "SettingDlg.h"
#include "ScannerLoader.h"
#include "Splash.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CIBOMApp

BEGIN_MESSAGE_MAP(CIBOMApp, CWinAppEx)
	ON_COMMAND(ID_APP_ABOUT, &CIBOMApp::OnAppAbout)
	// 基于文件的标准文档命令
	ON_COMMAND(ID_FILE_NEW, &CWinAppEx::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinAppEx::OnFileOpen)
	// 标准打印设置命令
	ON_COMMAND(ID_FILE_PRINT_SETUP, &CWinAppEx::OnFilePrintSetup)
END_MESSAGE_MAP()


// CIBOMApp 构造
DefGetSupportBOMType CIBOMApp::GetSupportBOMType=NULL;
DefGetSupportDataBufferVersion CIBOMApp::GetSupportDataBufferVersion=NULL;
DefCreateExcelBomFile CIBOMApp::CreateExcelBomFile=NULL;
DefGetExcelFormat CIBOMApp::GetBomExcelFormat=NULL;
CIBOMApp::CIBOMApp()
{
	m_bHiColorIcons = TRUE;

	// 支持重新启动管理器
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_ALL_ASPECTS;
#ifdef _MANAGED
	// 如果应用程序是利用公共语言运行时支持(/clr)构建的，则:
	//     1) 必须有此附加设置，“重新启动管理器”支持才能正常工作。
	//     2) 在您的项目中，您必须按照生成顺序向 System.Windows.Forms 添加引用。
	System::Windows::Forms::Application::SetUnhandledExceptionMode(System::Windows::Forms::UnhandledExceptionMode::ThrowException);
#endif

	// TODO: 将以下应用程序 ID 字符串替换为唯一的 ID 字符串；建议的字符串格式
	//为 CompanyName.ProductName.SubProduct.VersionInformation
	SetAppID(_T("IBOM.AppID.NoVersion"));

	// TODO: 在此处添加构造代码，
	// 将所有重要的初始化放置在 InitInstance 中
	m_hBomExport=NULL;
	
}

void ClearTemporaryFile(const char* dir_path)
{
	CXhChar500 workpath;
	if(dir_path!=NULL)
		workpath.Copy(dir_path);
	else
	{
		char APP_PATH[MAX_PATH]={};
		GetAppPath(APP_PATH);
		workpath.Printf("%sTemp\\",APP_PATH);
	}
	CFileFind finderFile; 
	BOOL bFind=finderFile.FindFile(CString(workpath)+_T("\\*.*"));//总文件夹，开始遍历 
	while(bFind) 
	{
		bFind = finderFile.FindNextFile();//查找文件 
		if (finderFile.IsDots()) 
			continue;
		CString sFileName=finderFile.GetFileName();
		if(!finderFile.IsDirectory())
		{
			CString sFileName=workpath;
			sFileName+="\\";
			sFileName+=finderFile.GetFileName();
			DeleteFile(sFileName);
		}
	}
}

// 唯一的一个 CIBOMApp 对象

CIBOMApp theApp;

#include <DbgHelp.h>
#pragma comment(lib,"DbgHelp.Lib")  //MiniDumpWriteDump链接时用到
LONG WINAPI  IBomExceptionFilter(EXCEPTION_POINTERS *pExptInfo)
{
	// 程序崩溃时，将写入程序目录下的ExceptionDump.dmp文件  
	GetAppPath(APP_PATH);
	CTime time = CTime::GetCurrentTime();
	CString sTime = time.Format("%Y%m%d%H%M%S");
	CXhChar500 sExceptionFilePath("%s\\IBomException(%s).dmp", APP_PATH,sTime);
	size_t size = sExceptionFilePath.Length;
	if (size > 0)
	{
		HANDLE hFile = ::CreateFile(sExceptionFilePath, GENERIC_WRITE,
			FILE_SHARE_WRITE, NULL, CREATE_NEW,
			FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			MINIDUMP_EXCEPTION_INFORMATION exptInfo;
			exptInfo.ThreadId = ::GetCurrentThreadId();
			exptInfo.ExceptionPointers = pExptInfo;
			//将内存堆栈dump到文件中
			//MiniDumpWriteDump需引入dbghelp头文件
			BOOL bOK = ::MiniDumpWriteDump(::GetCurrentProcess(),
				::GetCurrentProcessId(), hFile, MiniDumpNormal,
				&exptInfo, NULL, NULL);
		}
		return EXCEPTION_EXECUTE_HANDLER;
	}
	else
		return 0;
}
// CIBOMApp 初始化
BOOL CIBOMApp::IsHasInternalTest()
{
	return VerifyValidFunction(LICFUNC::FUNC_IDENTITY_INTERNAL_TEST);
}
BOOL CIBOMApp::InitInstance()
{
\
	{
\
		CCommandLineInfo cmdInfo;
\
		ParseCommandLine(cmdInfo);
\

\
		CSplashWnd::EnableSplashScreen(cmdInfo.m_bShowSplash);
\
	}
	::SetUnhandledExceptionFilter(IBomExceptionFilter);	//设置未处理异常过滤器生成dmp文件 wht 19-04-09

	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// 将它设置为包括所有要在应用程序中使用的
	// 公共控件类。
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();
	// 初始化 OLE 库
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();
	EnableTaskbarInteraction(FALSE);

	SetRegistryKey(_T("Xerofox"));
	LoadStdProfileSettings();  // 加载标准 INI 文件选项(包括 MRU)
	//
	GetAppPath(APP_PATH);
	CXhChar500 workpath("%sTemp\\",APP_PATH);
	int retcode=_mkdir(workpath);
	ClearTemporaryFile(workpath);
	IMindRobot::SetWorkDirectory(workpath);
	CConfig::InitKeyText();
	CConfig::ReadSysParaFromReg();
	dataModel.InitFileProp();
	char cfg_file[MAX_PATH];
	sprintf(cfg_file,"%sIBOM.cfg",APP_PATH);
	CConfig::LoadFromFile(cfg_file);
	CPartLibrary::LoadPartLibrary(APP_PATH);
	IMindRobot::SetMaxWaringMatchCoef(CConfig::m_ciWaringMatchCoef);
	//进行加密处理
	DWORD version[2]={0,20150318};
	BYTE* pByteVer=(BYTE*)version;
	pByteVer[0]=1;
	pByteVer[1]=0;
	pByteVer[2]=0;
	pByteVer[3]=1;
	char lic_file[MAX_PATH];
	WriteProfileString("SETUP","APP_PATH", APP_PATH);	//应用程序路径
	sprintf(lic_file,"%sIBOM.lic",APP_PATH);
	strcpy(execute_path,APP_PATH);
	m_version[0]=version[0];
	m_version[1]=version[1];
	DWORD retCode=0;
	bool passed_liccheck=true;
	for(int i=0;true;i++){
		retCode=ImportLicFile(lic_file,PRODUCT_IBOM,version);
		CXhChar50 errormsgstr("未指证书读取错误");
		if(retCode==0)
		{
			passed_liccheck=false;
			if(GetLicVersion()<1000004)
#ifdef AFX_TARG_ENU_ENGLISH
				errormsgstr.Copy("The certificate file has been out of date, the current software's version must use the new certificate file！");
#else 
				errormsgstr.Copy("该证书文件已过时，当前软件版本必须使用新证书！");
#endif
			else if(!VerifyValidFunction(LICFUNC::FUNC_IDENTITY_BASIC))
#ifdef AFX_TARG_ENU_ENGLISH
				errormsgstr.Copy("Software Lacks of legitimate use authorized!");
#else 
				errormsgstr.Copy("软件缺少合法使用授权!");
#endif
			else
			{
				passed_liccheck=true;
				WriteProfileString("Settings","lic_file",lic_file);
			}
			if(!passed_liccheck)
			{
#ifndef _LEGACY_LICENSE
				ExitCurrentDogSession();
#elif defined(_NET_DOG)
				ExitNetLicEnv(m_wDogModule);
#endif
			}
			else
				break;
		}
		else
		{
#ifdef AFX_TARG_ENU_ENGLISH
			if(retCode==-1)
				errormsgstr.Copy("0# Hard dog failed to initialize!");
			else if(retCode==1)
				errormsgstr.Copy("1# Unable to open the license file!");
			else if(retCode==2)
				errormsgstr.Copy("2# License file was damaged!");
			else if(retCode==3||retCode==4)
				errormsgstr.Copy("3# License file not found or does'nt match the hard lock!");
			else if(retCode==5)
				errormsgstr.Copy("5# License file doesn't match the authorized products in hard lock!");
			else if(retCode==6)
				errormsgstr.Copy("6# Beyond the scope of authorized version !");
			else if(retCode==7)
				errormsgstr.Copy("7# Beyond the scope of free authorize version !");
			else if(retCode==8)
				errormsgstr.Copy("8# The serial number of license file does'nt match the hard lock!");
#else 
			if(retCode==-1)
				errormsgstr.Copy("0#加密狗初始化失败!");
			else if(retCode==1)
				errormsgstr.Copy("1#无法打开证书文件");
			else if(retCode==2)
				errormsgstr.Copy("2#证书文件遭到破坏");
			else if(retCode==3||retCode==4)
				errormsgstr.Copy("3#执行目录下不存在证书文件或证书文件与加密狗不匹配");
			else if(retCode==5)
				errormsgstr.Copy("5#证书与加密狗产品授权版本不匹配");
			else if(retCode==6)
				errormsgstr.Copy("6#超出版本使用授权范围");
			else if(retCode==7)
				errormsgstr.Copy("7#超出免费版本升级授权范围");
			else if(retCode==8)
				errormsgstr.Copy("8#证书序号与当前加密狗不一致");
			else if(retCode==9)
				errormsgstr.Copy("9#授权过期，请续借授权");
			else if(retCode==10)
				errormsgstr.Copy("10#程序缺少相应执行权限，请以管理员权限运行程序");
			else if (retCode == 11)
				errormsgstr.Copy("11#授权异常，请使用管理员权限重新申请证书");
			else
				errormsgstr.Printf("未知错误，错误代码%d#", retCode);
#endif
#ifndef _LEGACY_LICENSE
			ExitCurrentDogSession();
#elif defined(_NET_DOG)
			ExitNetLicEnv(m_wDogModule);
#endif
			passed_liccheck=false;
			//return FALSE;
		}
		if(!passed_liccheck)
		{
			DWORD dwRet=ProcessLicenseAuthorize(PRODUCT_IBOM,m_version,theApp.execute_path,false,errormsgstr);
			if(dwRet==0)
				return FALSE;
			if(passed_liccheck=(dwRet==1))
				break;	//内部已成功导入证书文件
		}
	}
	if(!passed_liccheck)
	{
#ifndef _LEGACY_LICENSE
		ExitCurrentDogSession();
#elif defined(_NET_DOG)
		ExitNetLicEnv(m_wDogModule);
#endif
		return FALSE;
	}
	if(!VerifyValidFunction(IBOM_LICFUNC::FUNC_IDENTITY_BASIC))
	{
		AfxMessageBox("证书中缺少IBOM基本功能授权!");
		return FALSE;
	}
	IAlphabets* pAlphabets=IMindRobot::GetAlphabetKnowledge();
	//读入笔画特征
	CXhChar500 szStrokeFileFolder("%sfonts\\Strokes\\",theApp.execute_path);
	pAlphabets->InitStrokeFeatureByFiles(szStrokeFileFolder);
	//初始化字体
	CXhChar200 fontsfile("%sibom.fonts",theApp.execute_path);
	//pAlphabets->SetActiveFontSerial(CConfig::m_iFontsLibSerial);
	if(!pAlphabets->ImportFontsFile(fontsfile))
	{	//读入字体
		CXhChar500 sFontsFileRootPath("%sfonts\\",theApp.execute_path);
		pAlphabets->SetFontLibRootPath(sFontsFileRootPath);
		int i=0;
		bool bAppendMode=true;
		for(i=0;i<100;i++)
		{
			CXhChar500 folder=GetFontsLibPath(i,CIBOMApp::FONTS_LIB_FOLDER);
			if(pAlphabets->InitFromFolder(folder,i,bAppendMode)==false)
			{
				//AfxMessageBox("识别字体种类失败！");
				break;
			}
			//if(i==0)
			//	pAlphabets->SetRecogMode(i,1);
			//else 
				if(i==2)
				pAlphabets->SetPreSplitChar(i,TRUE);
			if(!bAppendMode)
			{
				CXhChar500 fontsFilePath=GetFontsLibPath(i,CIBOMApp::FONTS_LIB_FILE);
				pAlphabets->ExportFontsFile(fontsFilePath);
			}
		}
		if(i==0)
		{
			AfxMessageBox("识别字体种类失败！");
			return FALSE;
		}
		else if(bAppendMode)
			pAlphabets->ExportFontsFile(fontsfile);
	}

	InitContextMenuManager();
	InitKeyboardManager();
	InitTooltipManager();
	CMFCToolTipInfo ttParams;
	ttParams.m_bVislManagerTheme = TRUE;
	theApp.GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,
		RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

	if(!AfxInitRichEdit())
		return FALSE;
	// 注册应用程序的文档模板。文档模板
	// 将用作文档、框架窗口和视图之间的连接
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CIBOMDoc),
		RUNTIME_CLASS(CMainFrame),       // 主 SDI 框架窗口
		RUNTIME_CLASS(CIBOMView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);
	// 分析标准 shell 命令、DDE、打开文件操作的命令行
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	// 启用“DDE 执行”
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);
	// 调度在命令行中指定的命令。如果
	// 用 /RegServer、/Register、/Unregserver 或 /Unregister 启动应用程序，则返回 FALSE。
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;
	// 唯一的一个窗口已初始化，因此显示它并对其进行更新
	m_pMainWnd->ShowWindow(SW_SHOWMAXIMIZED);
	m_pMainWnd->UpdateWindow();
	m_pMainWnd->DragAcceptFiles();

	StartScannerListener();
	//
	TCHAR lib_path[MAX_PATH]="";
	sprintf(lib_path,"%sBomExport.dll",APP_PATH);
	m_hBomExport=LoadLibrary(lib_path);
	if(m_hBomExport)
	{
		CIBOMApp::GetSupportBOMType=(DefGetSupportBOMType)GetProcAddress(m_hBomExport,"GetSupportBOMType");
		CIBOMApp::GetSupportDataBufferVersion=(DefGetSupportDataBufferVersion)GetProcAddress(m_hBomExport,"GetSupportDataBufferVersion");
		CIBOMApp::CreateExcelBomFile=(DefCreateExcelBomFile)GetProcAddress(m_hBomExport,"CreateExcelBomFile");
		CIBOMApp::GetBomExcelFormat=(DefGetExcelFormat)GetProcAddress(m_hBomExport,"GetExcelFormat");
	}
	dataModel.DisplayProcess=DisplayProcess;
	return TRUE;
}

CXhChar500 CIBOMApp::GetFontsLibPath(int iFontSerial,BYTE biPathType)
{
	CXhChar500 fontsFilePath;
	if(biPathType==FONTS_LIB_FOLDER)
	{
		fontsFilePath.Printf("%sfonts\\hzfs\\",theApp.execute_path);
		if(iFontSerial>0)
			fontsFilePath.Printf("%sfonts\\hzfs-%d\\",theApp.execute_path,iFontSerial);
	}
	else if(biPathType==FONTS_LIB_FILE)
	{
		fontsFilePath.Printf("%sfonts\\ibom.fonts",theApp.execute_path);
		if(iFontSerial>0)
			fontsFilePath.Printf("%sfonts\\ibom.fonts-%d",theApp.execute_path,iFontSerial);
	}
	return fontsFilePath;
}

int CIBOMApp::ExitInstance()
{
	//TODO: 处理可能已添加的附加资源
	AfxOleTerm(FALSE);
	CConfig::WriteSysParaToReg();
	ClearTemporaryFile(NULL);
	return CWinAppEx::ExitInstance();
}
CIBOMDoc* CIBOMApp::GetDocument()
{
	POSITION pos,docpos;
	pos=AfxGetApp()->m_pDocManager->GetFirstDocTemplatePosition();
	for(CDocTemplate *pDocTemp=AfxGetApp()->m_pDocManager->GetNextDocTemplate(pos);
		pDocTemp;pDocTemp=AfxGetApp()->m_pDocManager->GetNextDocTemplate(pos))
	{
		docpos=pDocTemp->GetFirstDocPosition();
		for(CDocument *pDoc=pDocTemp->GetNextDoc(docpos);pDoc;pDoc=pDocTemp->GetNextDoc(docpos))
		{
			if(pDoc->IsKindOf(RUNTIME_CLASS(CIBOMDoc)))
				return (CIBOMDoc*)pDoc;
		}
	}
	return NULL;
}
CIBOMView* CIBOMApp::GetIBomView()
{
	CIBOMDoc *pDoc=GetDocument();
	if(pDoc==NULL)
		return NULL;
	CIBOMView *pView = (CIBOMView*)pDoc->GetView(RUNTIME_CLASS(CIBOMView));
	return pView;
}
// CIBOMApp 消息处理程序


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();
	CString	m_sVersion;
// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
	m_sVersion = _T("");
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_S_VERSION, m_sVersion);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// 用于运行对话框的应用程序命令
void CIBOMApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.m_sVersion.Format("IBOM %s 版 授权号：%d",GetProductVersion(),DogSerialNo());
	aboutDlg.DoModal();
}

// CIBOMApp 自定义加载/保存方法
void CIBOMApp::PreLoadState()
{
	BOOL bNameValid;
	CString strName;
	bNameValid = strName.LoadString(IDS_EDIT_MENU);
	ASSERT(bNameValid);
	GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EDIT);
	bNameValid = strName.LoadString(IDS_EXPLORER);
	ASSERT(bNameValid);
	GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EXPLORER);
}

void CIBOMApp::LoadCustomState()
{
}

void CIBOMApp::SaveCustomState()
{
}

// CIBOMApp 消息处理程序
#ifdef __SCANNER_LOADER_
UINT DownloadImageFileFromScanner(LPVOID pParam)
{
	HWND hWnd=(HWND)pParam;
	while(1){
		int nNewFileCount=g_scannerLoader.QueryAllImages();
		if(nNewFileCount>=0)
		{
			CXhChar200 sTempDir;
			IMindRobot::GetWorkDirectory(sTempDir,201);
			//ARRAY_LIST<CXhChar500> filePathList;
			int nCount=0;
			for(IMAGE_DOWNLOAD *pImage=g_scannerLoader.EnumFirst();pImage;pImage=g_scannerLoader.EnumNext())
			{
				g_scannerLoader.DownloadImageToFile(pImage->sName,sTempDir,false);
				//filePathList.append(CXhChar500("%s%s",(char*)sTempDir,pImage->sName));
				nCount++;
			}
			if(nCount>0)
				::PostMessage(hWnd,WM_REFRESH_SCANNER_FILE_TO_TREE,0,0);
			/*CMainFrame *pMainFrm=(CMainFrame*)AfxGetMainWnd();
			CFileTreeDlg *pFilePage=pMainFrm?pMainFrm->GetFileTreePage():NULL;
			if(pFilePage)
				pFilePage->AppendFile(filePathList,FALSE);*/
		}
		else
			CScannerLoader::AutoConnectScannerWifi(NULL);
		Sleep(20000);
	}
	return 0;
}
#endif

void CIBOMApp::StartScannerListener()
{
	if(!VerifyValidFunction(LICFUNC::FUNC_IDENTITY_INTERNAL_TEST))
		return;
#ifdef __SCANNER_LOADER_
	if(!CConfig::m_bListenScanner)
		return;
	if(m_pScannerListen!=NULL||m_pMainWnd==NULL)
		return;
	m_pScannerListen=AfxBeginThread(DownloadImageFileFromScanner,m_pMainWnd->GetSafeHwnd(),THREAD_PRIORITY_NORMAL);
#endif
}

void CIBOMApp::GetAppVersion(char *file_version,char *product_version)
{
	DWORD dwLen=GetFileVersionInfoSize(__argv[0],0);
	BYTE *data=new BYTE[dwLen];
	WORD file_ver[4],product_ver[4];
	if(GetFileVersionInfo(__argv[0],0,dwLen,data))
	{
		VS_FIXEDFILEINFO *info;
		UINT uLen;
		VerQueryValue(data,"\\",(void**)&info,&uLen);
		memcpy(file_ver,&info->dwFileVersionMS,4);
		memcpy(&file_ver[2],&info->dwFileVersionLS,4);
		memcpy(product_ver,&info->dwProductVersionMS,4);
		memcpy(&product_ver[2],&info->dwProductVersionLS,4);
		if(file_version)
			sprintf(file_version,"%d.%d.%d.%d",file_ver[1],file_ver[0],file_ver[3],file_ver[2]);
		if(product_version)
			sprintf(product_version,"%d.%d.%d.%d",product_ver[1],product_ver[0],product_ver[3],product_ver[2]);
	}
	delete data;
}
CString CIBOMApp::GetProductVersion()
{
	CString product_version;
	DWORD dwLen=GetFileVersionInfoSize(__argv[0],0);
	BYTE *data=new BYTE[dwLen];
	WORD product_ver[4];
	if(GetFileVersionInfo(__argv[0],0,dwLen,data))
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
void CIBOMApp::GetProductVersion(char byteVer[4])
{
	DWORD dwLen=GetFileVersionInfoSize(__argv[0],0);
	BYTE *data=new BYTE[dwLen];
	WORD product_ver[4];
	if(GetFileVersionInfo(__argv[0],0,dwLen,data))
	{
		VS_FIXEDFILEINFO *info;
		UINT uLen;
		VerQueryValue(data,"\\",(void**)&info,&uLen);
		memcpy(product_ver,&info->dwProductVersionMS,4);
		memcpy(&product_ver[2],&info->dwProductVersionLS,4);
	}
	delete data;
	byteVer[0]=(char)product_ver[2];
	byteVer[1]=(char)product_ver[3];
	byteVer[2]=(char)product_ver[0];
	byteVer[3]=(char)product_ver[1];
}

BOOL CIBOMApp::PreTranslateMessage(MSG* pMsg)
{
	// CG: The following lines were added by the Splash Screen component.
	if (CSplashWnd::PreTranslateAppMessage(pMsg))
		return TRUE;

	return CWinAppEx::PreTranslateMessage(pMsg);
}