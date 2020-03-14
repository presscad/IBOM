// DBP.cpp : 定义 DLL 的初始化例程。
//

#include "stdafx.h"
#include <afxwin.h>
#include <afxdllx.h>
#include "MenuFunc.h"
#include "XhLdsLm.h"
#include "LicFuncDef.h"
#include "XhCharString.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

void GetLicFile(char* lic_file)
{
	char path[MAX_PATH];
	DWORD dwDataType,dwLength=MAX_PATH;
	char sSubKey[MAX_PATH]="Software\\Xerofox\\IBOM\\Settings";
	HKEY hKey;
	RegOpenKeyEx(HKEY_CURRENT_USER,sSubKey,0,KEY_READ,&hKey);
	DWORD dwRetCode=RegQueryValueEx(hKey,_T("lic_file"),NULL,&dwDataType,(BYTE*)&path[0],&dwLength);
	if(dwRetCode==ERROR_SUCCESS)
		strcpy(lic_file,path);
	else 
	{
		RegCloseKey(hKey);
		AfxMessageBox(CXhChar50("证书文件路径读取错误(#%d)！请将CAD设置为以管理员身份运行，重试一次！",dwRetCode));
		return;
	}
	RegCloseKey(hKey);
}
void RegisterServerComponents ()
{	
#ifdef _ARX_2007
	// 智能提取板信息
	acedRegCmds->addCommand(L"DBP-MENU",           // Group name
		L"RT",             // Global function name
		L"RT",             // Local function name
		ACRX_CMD_MODAL,   // Type
		&RecogizePartBomToBomdFile);            // Function pointer
#else
	// 智能提取板信息
	acedRegCmds->addCommand( "DBP-MENU",           // Group name
		"RT",        // Global function name
		"RT",        // Local function name
		ACRX_CMD_MODAL,   // Type
		&RecogizePartBomToBomdFile);            // Function pointer
#endif
}
static char* SearchChar(char* srcStr, char ch, bool reverseOrder/*=false*/)
{
	if (!reverseOrder)
		return strchr(srcStr, ch);
	else
	{
		int len = strlen(srcStr);
		for (int i = len - 1; i >= 0; i--)
		{
			if (srcStr[i] == ch)
				return &srcStr[i];
		}
	}
	return NULL;
}
bool DetectSpecifiedHaspKeyFile(const char* default_file)
{
	FILE* fp = fopen(default_file, "rt");
	if (fp == NULL)
		return false;
	bool detected = false;
	CXhChar200 line_txt;//[MAX_PATH];
	CXhChar200 scope_xmlstr;
	scope_xmlstr.Append(
		"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"
		"<haspscope>");
	while (!feof(fp))
	{
		if (fgets(line_txt, line_txt.GetLengthMax(), fp) == NULL)
			break;
		line_txt.Replace("＝", "=");
		char* final = SearchChar(line_txt, ';', true);
		if (final != NULL)
			*final = 0;
		char *skey = strtok(line_txt, " =,");
		//常规设置
		if (_stricmp(skey, "Key") == 0)
		{
			if (skey = strtok(NULL, "=,"))
			{
				scope_xmlstr.Append("<hasp id=\"");
				scope_xmlstr.Append(skey);
				scope_xmlstr.Append("\" />");
				detected = true;
			}
		}
	}
	fclose(fp);
	scope_xmlstr.Append("</haspscope>");
	if (detected)
		SetHaspLoginScope(scope_xmlstr);
	return detected;
}

char g_sAppPath[MAX_PATH]={0};
void InitApplication()
{
	//进行加密处理
	DWORD version[2]={0,20150318};
	BYTE* pByteVer=(BYTE*)version;
	pByteVer[0]=1;
	pByteVer[1]=0;
	pByteVer[2]=0;
	pByteVer[3]=1;
	char lic_file[MAX_PATH]={0};
	GetLicFile(lic_file);
	//从lic_file中获取APP_PATH,用于输出提取的材料表
	char drive[4];
	char dir[MAX_PATH];
	char fname[MAX_PATH];
	char ext[MAX_PATH];
	_splitpath(lic_file,drive,dir,fname,ext);
	strcpy(g_sAppPath,drive);
	strcat(g_sAppPath,dir);
	//查找是否存在指定加密锁号的文件 wht-2019.11.05
	char key_file[MAX_PATH];
	strcpy(key_file, lic_file);
	char* separator = SearchChar(key_file, '.', true);
	strcpy(separator, ".key");
	DetectSpecifiedHaspKeyFile(key_file);
	//
	ULONG retCode=ImportLicFile(lic_file,PRODUCT_IBOM,version);
	if(retCode!=0)
	{
		CXhChar200 errormsgstr("未指证书读取错误");
		if(retCode==-2)
			errormsgstr.Copy("首次使用，还未指定过证书文件！");
		else if(retCode==-1)
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
		errormsgstr.Append(CXhChar200(" 证书路径%s",lic_file));
		AfxMessageBox(errormsgstr);
		exit(0);
	}
	/*if(!VerifyValidFunction(IBOM_LICFUNC::FUNC_IDENTITY_BASIC))
	{
		AfxMessageBox("软件缺少合法使用授权!");
		exit(0);
	}*/
	HWND hWnd = adsw_acadMainWnd();
	::SetWindowText(hWnd,"DBP");
	RegisterServerComponents();
}
void UnloadApplication()
{
	ExitCurrentDogSession();	//退出加密狗
	char sGroupName[100]="DBP-MENU";
#ifdef _ARX_2007
	acedRegCmds->removeGroup((ACHAR*)_bstr_t(sGroupName));
#else
	acedRegCmds->removeGroup(sGroupName);
#endif
	//删除acad.rx
	char cad_path[MAX_PATH]="";
	GetModuleFileName(NULL,cad_path,MAX_PATH);
	if(strlen(cad_path)>0)
	{
		char *separator=SearchChar(cad_path,'.',true);
		strcpy(separator,".rx");
		::DeleteFile(cad_path);
	}
}
//////////////////////////////////////////////////////////////////////////
static AFX_EXTENSION_MODULE DBPDLL = { NULL, NULL };
#if !defined(__AFTER_ARX_2007)
extern "C" void ads_queueexpr(ACHAR *);
#endif
extern "C" int APIENTRY
	DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	// 如果使用 lpReserved，请将此移除
	UNREFERENCED_PARAMETER(lpReserved);

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		TRACE0("DBP.DLL 正在初始化!\n");

		// 扩展 DLL 一次性初始化
		if (!AfxInitExtensionModule(DBPDLL, hInstance))
			return 0;

		new CDynLinkLibrary(DBPDLL);

	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		TRACE0("DBP.DLL 正在终止!\n");

		// 在调用析构函数之前终止该库
		AfxTermExtensionModule(DBPDLL);
	}
	return 1;   // 确定
}
extern "C" AcRx::AppRetCode acrxEntryPoint(AcRx::AppMsgCode msg, void* pkt)
{
	switch (msg) {
	case AcRx::kInitAppMsg:
		acrxDynamicLinker->unlockApplication(pkt);
		acrxDynamicLinker->registerAppMDIAware(pkt);
		InitApplication();
		break;
	case AcRx::kUnloadAppMsg:
		UnloadApplication();
		break;
	case AcRx::kLoadDwgMsg:
		//InitDrawingEnv();
		//kLoadDwgMsg中不能调用sendStringToExecute和acedCommand,acDocManager->curDocument()==NULL,无法执行 wht 18-12-25
#ifdef _ARX_2007
		ads_queueexpr(L"(command\"RT\")");
#else
		ads_queueexpr("(command\"RT\")");
#endif
		break;
	}
	return AcRx::kRetOK;
}