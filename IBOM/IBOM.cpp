
// IBOM.cpp : ����Ӧ�ó��������Ϊ��
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
	// �����ļ��ı�׼�ĵ�����
	ON_COMMAND(ID_FILE_NEW, &CWinAppEx::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinAppEx::OnFileOpen)
	// ��׼��ӡ��������
	ON_COMMAND(ID_FILE_PRINT_SETUP, &CWinAppEx::OnFilePrintSetup)
END_MESSAGE_MAP()


// CIBOMApp ����
DefGetSupportBOMType CIBOMApp::GetSupportBOMType=NULL;
DefGetSupportDataBufferVersion CIBOMApp::GetSupportDataBufferVersion=NULL;
DefCreateExcelBomFile CIBOMApp::CreateExcelBomFile=NULL;
DefGetExcelFormat CIBOMApp::GetBomExcelFormat=NULL;
CIBOMApp::CIBOMApp()
{
	m_bHiColorIcons = TRUE;

	// ֧����������������
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_ALL_ASPECTS;
#ifdef _MANAGED
	// ���Ӧ�ó��������ù�����������ʱ֧��(/clr)�����ģ���:
	//     1) �����д˸������ã�������������������֧�ֲ�������������
	//     2) ��������Ŀ�У������밴������˳���� System.Windows.Forms ������á�
	System::Windows::Forms::Application::SetUnhandledExceptionMode(System::Windows::Forms::UnhandledExceptionMode::ThrowException);
#endif

	// TODO: ������Ӧ�ó��� ID �ַ����滻ΪΨһ�� ID �ַ�����������ַ�����ʽ
	//Ϊ CompanyName.ProductName.SubProduct.VersionInformation
	SetAppID(_T("IBOM.AppID.NoVersion"));

	// TODO: �ڴ˴���ӹ�����룬
	// ��������Ҫ�ĳ�ʼ�������� InitInstance ��
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
	BOOL bFind=finderFile.FindFile(CString(workpath)+_T("\\*.*"));//���ļ��У���ʼ���� 
	while(bFind) 
	{
		bFind = finderFile.FindNextFile();//�����ļ� 
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

// Ψһ��һ�� CIBOMApp ����

CIBOMApp theApp;

#include <DbgHelp.h>
#pragma comment(lib,"DbgHelp.Lib")  //MiniDumpWriteDump����ʱ�õ�
LONG WINAPI  IBomExceptionFilter(EXCEPTION_POINTERS *pExptInfo)
{
	// �������ʱ����д�����Ŀ¼�µ�ExceptionDump.dmp�ļ�  
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
			//���ڴ��ջdump���ļ���
			//MiniDumpWriteDump������dbghelpͷ�ļ�
			BOOL bOK = ::MiniDumpWriteDump(::GetCurrentProcess(),
				::GetCurrentProcessId(), hFile, MiniDumpNormal,
				&exptInfo, NULL, NULL);
		}
		return EXCEPTION_EXECUTE_HANDLER;
	}
	else
		return 0;
}
// CIBOMApp ��ʼ��
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
	::SetUnhandledExceptionFilter(IBomExceptionFilter);	//����δ�����쳣����������dmp�ļ� wht 19-04-09

	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// ��������Ϊ��������Ҫ��Ӧ�ó�����ʹ�õ�
	// �����ؼ��ࡣ
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();
	// ��ʼ�� OLE ��
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();
	EnableTaskbarInteraction(FALSE);

	SetRegistryKey(_T("Xerofox"));
	LoadStdProfileSettings();  // ���ر�׼ INI �ļ�ѡ��(���� MRU)
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
	//���м��ܴ���
	DWORD version[2]={0,20150318};
	BYTE* pByteVer=(BYTE*)version;
	pByteVer[0]=1;
	pByteVer[1]=0;
	pByteVer[2]=0;
	pByteVer[3]=1;
	char lic_file[MAX_PATH];
	WriteProfileString("SETUP","APP_PATH", APP_PATH);	//Ӧ�ó���·��
	sprintf(lic_file,"%sIBOM.lic",APP_PATH);
	strcpy(execute_path,APP_PATH);
	m_version[0]=version[0];
	m_version[1]=version[1];
	DWORD retCode=0;
	bool passed_liccheck=true;
	for(int i=0;true;i++){
		retCode=ImportLicFile(lic_file,PRODUCT_IBOM,version);
		CXhChar50 errormsgstr("δָ֤���ȡ����");
		if(retCode==0)
		{
			passed_liccheck=false;
			if(GetLicVersion()<1000004)
#ifdef AFX_TARG_ENU_ENGLISH
				errormsgstr.Copy("The certificate file has been out of date, the current software's version must use the new certificate file��");
#else 
				errormsgstr.Copy("��֤���ļ��ѹ�ʱ����ǰ����汾����ʹ����֤�飡");
#endif
			else if(!VerifyValidFunction(LICFUNC::FUNC_IDENTITY_BASIC))
#ifdef AFX_TARG_ENU_ENGLISH
				errormsgstr.Copy("Software Lacks of legitimate use authorized!");
#else 
				errormsgstr.Copy("���ȱ�ٺϷ�ʹ����Ȩ!");
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
				errormsgstr.Copy("0#���ܹ���ʼ��ʧ��!");
			else if(retCode==1)
				errormsgstr.Copy("1#�޷���֤���ļ�");
			else if(retCode==2)
				errormsgstr.Copy("2#֤���ļ��⵽�ƻ�");
			else if(retCode==3||retCode==4)
				errormsgstr.Copy("3#ִ��Ŀ¼�²�����֤���ļ���֤���ļ�����ܹ���ƥ��");
			else if(retCode==5)
				errormsgstr.Copy("5#֤������ܹ���Ʒ��Ȩ�汾��ƥ��");
			else if(retCode==6)
				errormsgstr.Copy("6#�����汾ʹ����Ȩ��Χ");
			else if(retCode==7)
				errormsgstr.Copy("7#������Ѱ汾������Ȩ��Χ");
			else if(retCode==8)
				errormsgstr.Copy("8#֤������뵱ǰ���ܹ���һ��");
			else if(retCode==9)
				errormsgstr.Copy("9#��Ȩ���ڣ���������Ȩ");
			else if(retCode==10)
				errormsgstr.Copy("10#����ȱ����Ӧִ��Ȩ�ޣ����Թ���ԱȨ�����г���");
			else if (retCode == 11)
				errormsgstr.Copy("11#��Ȩ�쳣����ʹ�ù���ԱȨ����������֤��");
			else
				errormsgstr.Printf("δ֪���󣬴������%d#", retCode);
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
				break;	//�ڲ��ѳɹ�����֤���ļ�
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
		AfxMessageBox("֤����ȱ��IBOM����������Ȩ!");
		return FALSE;
	}
	IAlphabets* pAlphabets=IMindRobot::GetAlphabetKnowledge();
	//����ʻ�����
	CXhChar500 szStrokeFileFolder("%sfonts\\Strokes\\",theApp.execute_path);
	pAlphabets->InitStrokeFeatureByFiles(szStrokeFileFolder);
	//��ʼ������
	CXhChar200 fontsfile("%sibom.fonts",theApp.execute_path);
	//pAlphabets->SetActiveFontSerial(CConfig::m_iFontsLibSerial);
	if(!pAlphabets->ImportFontsFile(fontsfile))
	{	//��������
		CXhChar500 sFontsFileRootPath("%sfonts\\",theApp.execute_path);
		pAlphabets->SetFontLibRootPath(sFontsFileRootPath);
		int i=0;
		bool bAppendMode=true;
		for(i=0;i<100;i++)
		{
			CXhChar500 folder=GetFontsLibPath(i,CIBOMApp::FONTS_LIB_FOLDER);
			if(pAlphabets->InitFromFolder(folder,i,bAppendMode)==false)
			{
				//AfxMessageBox("ʶ����������ʧ�ܣ�");
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
			AfxMessageBox("ʶ����������ʧ�ܣ�");
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
	// ע��Ӧ�ó�����ĵ�ģ�塣�ĵ�ģ��
	// �������ĵ�����ܴ��ں���ͼ֮�������
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CIBOMDoc),
		RUNTIME_CLASS(CMainFrame),       // �� SDI ��ܴ���
		RUNTIME_CLASS(CIBOMView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);
	// ������׼ shell ���DDE�����ļ�������������
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	// ���á�DDE ִ�С�
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);
	// ��������������ָ����������
	// �� /RegServer��/Register��/Unregserver �� /Unregister ����Ӧ�ó����򷵻� FALSE��
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;
	// Ψһ��һ�������ѳ�ʼ���������ʾ����������и���
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
	//TODO: �����������ӵĸ�����Դ
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
// CIBOMApp ��Ϣ�������


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();
	CString	m_sVersion;
// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
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

// �������жԻ����Ӧ�ó�������
void CIBOMApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.m_sVersion.Format("IBOM %s �� ��Ȩ�ţ�%d",GetProductVersion(),DogSerialNo());
	aboutDlg.DoModal();
}

// CIBOMApp �Զ������/���淽��
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

// CIBOMApp ��Ϣ�������
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