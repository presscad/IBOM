// DBP.cpp : ���� DLL �ĳ�ʼ�����̡�
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
		AfxMessageBox(CXhChar50("֤���ļ�·����ȡ����(#%d)���뽫CAD����Ϊ�Թ���Ա������У�����һ�Σ�",dwRetCode));
		return;
	}
	RegCloseKey(hKey);
}
void RegisterServerComponents ()
{	
#ifdef _ARX_2007
	// ������ȡ����Ϣ
	acedRegCmds->addCommand(L"DBP-MENU",           // Group name
		L"RT",             // Global function name
		L"RT",             // Local function name
		ACRX_CMD_MODAL,   // Type
		&RecogizePartBomToBomdFile);            // Function pointer
#else
	// ������ȡ����Ϣ
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
		line_txt.Replace("��", "=");
		char* final = SearchChar(line_txt, ';', true);
		if (final != NULL)
			*final = 0;
		char *skey = strtok(line_txt, " =,");
		//��������
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
	//���м��ܴ���
	DWORD version[2]={0,20150318};
	BYTE* pByteVer=(BYTE*)version;
	pByteVer[0]=1;
	pByteVer[1]=0;
	pByteVer[2]=0;
	pByteVer[3]=1;
	char lic_file[MAX_PATH]={0};
	GetLicFile(lic_file);
	//��lic_file�л�ȡAPP_PATH,���������ȡ�Ĳ��ϱ�
	char drive[4];
	char dir[MAX_PATH];
	char fname[MAX_PATH];
	char ext[MAX_PATH];
	_splitpath(lic_file,drive,dir,fname,ext);
	strcpy(g_sAppPath,drive);
	strcat(g_sAppPath,dir);
	//�����Ƿ����ָ���������ŵ��ļ� wht-2019.11.05
	char key_file[MAX_PATH];
	strcpy(key_file, lic_file);
	char* separator = SearchChar(key_file, '.', true);
	strcpy(separator, ".key");
	DetectSpecifiedHaspKeyFile(key_file);
	//
	ULONG retCode=ImportLicFile(lic_file,PRODUCT_IBOM,version);
	if(retCode!=0)
	{
		CXhChar200 errormsgstr("δָ֤���ȡ����");
		if(retCode==-2)
			errormsgstr.Copy("�״�ʹ�ã���δָ����֤���ļ���");
		else if(retCode==-1)
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
		errormsgstr.Append(CXhChar200(" ֤��·��%s",lic_file));
		AfxMessageBox(errormsgstr);
		exit(0);
	}
	/*if(!VerifyValidFunction(IBOM_LICFUNC::FUNC_IDENTITY_BASIC))
	{
		AfxMessageBox("���ȱ�ٺϷ�ʹ����Ȩ!");
		exit(0);
	}*/
	HWND hWnd = adsw_acadMainWnd();
	::SetWindowText(hWnd,"DBP");
	RegisterServerComponents();
}
void UnloadApplication()
{
	ExitCurrentDogSession();	//�˳����ܹ�
	char sGroupName[100]="DBP-MENU";
#ifdef _ARX_2007
	acedRegCmds->removeGroup((ACHAR*)_bstr_t(sGroupName));
#else
	acedRegCmds->removeGroup(sGroupName);
#endif
	//ɾ��acad.rx
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
	// ���ʹ�� lpReserved���뽫���Ƴ�
	UNREFERENCED_PARAMETER(lpReserved);

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		TRACE0("DBP.DLL ���ڳ�ʼ��!\n");

		// ��չ DLL һ���Գ�ʼ��
		if (!AfxInitExtensionModule(DBPDLL, hInstance))
			return 0;

		new CDynLinkLibrary(DBPDLL);

	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		TRACE0("DBP.DLL ������ֹ!\n");

		// �ڵ�����������֮ǰ��ֹ�ÿ�
		AfxTermExtensionModule(DBPDLL);
	}
	return 1;   // ȷ��
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
		//kLoadDwgMsg�в��ܵ���sendStringToExecute��acedCommand,acDocManager->curDocument()==NULL,�޷�ִ�� wht 18-12-25
#ifdef _ARX_2007
		ads_queueexpr(L"(command\"RT\")");
#else
		ads_queueexpr("(command\"RT\")");
#endif
		break;
	}
	return AcRx::kRetOK;
}