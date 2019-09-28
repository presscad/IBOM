
// IBOM.h : IBOM Ӧ�ó������ͷ�ļ�
//
#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"       // ������
#include "IBOMView.h"
#include "IBOMDoc.h"

#include <WinSvc.h>

// CIBOMApp:
// �йش����ʵ�֣������ IBOM.cpp
//

#define WM_REFRESH_SCANNER_FILE_TO_TREE		WM_USER+10

typedef DWORD (*DefGetSupportBOMType)();
typedef DWORD (*DefGetSupportDataBufferVersion)();
typedef int (*DefCreateExcelBomFile)(char* data_buf,int buf_len,const char* file_name,DWORD dwFlag);
typedef int (*DefGetExcelFormat)(int* colIndexArr,int *startRowIndex);
class CIBOMApp : public CWinAppEx
{
	HMODULE m_hBomExport;
	CWinThread *m_pScannerListen;
public:
	DWORD m_version[2];
	char execute_path[MAX_PATH];//��ȡִ���ļ�������Ŀ¼
	CIBOMApp();
	CIBOMDoc* GetDocument();
	CIBOMView* GetIBomView();
	static const int FONTS_LIB_FOLDER	= 0;
	static const int FONTS_LIB_FILE		= 1;
	CXhChar500 GetFontsLibPath(int iFontSerial,BYTE biPathType);
public:
	static DefGetSupportBOMType GetSupportBOMType;
	static DefGetSupportDataBufferVersion GetSupportDataBufferVersion;
	static DefCreateExcelBomFile CreateExcelBomFile;
	static DefGetExcelFormat GetBomExcelFormat;
	CString GetProductVersion();
	void StartScannerListener();
// ��д
public:
	virtual BOOL IsHasInternalTest();
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	void GetAppVersion(char *file_version,char *product_version);
	void GetFileVersion(CString &file_version);
	void GetProductVersion(CString &product_version);
	void GetProductVersion(char byteVer[4]);
// ʵ��
	UINT  m_nAppLook;
	BOOL  m_bHiColorIcons;

	virtual void PreLoadState();
	virtual void LoadCustomState();
	virtual void SaveCustomState();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CIBOMApp theApp;
