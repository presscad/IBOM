
// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�

#pragma once

#ifndef _SECURE_ATL
#define _SECURE_ATL 1
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // �� Windows ͷ���ų�����ʹ�õ�����
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // ĳЩ CString ���캯��������ʽ��

// �ر� MFC ��ĳЩ�����������ɷ��ĺ��Եľ�����Ϣ������
#define _AFX_ALL_WARNINGS

#define _AFX_SECURE_NO_WARNINGS     // MFC
#define _ATL_SECURE_NO_WARNINGS     // ATL
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS     // C
#endif
#define _CRT_NONSTDC_NO_WARNINGS    // CPOSIX
#define _SCL_SECURE_NO_WARNINGS     // STL
#define _SCL_SECURE_NO_DEPRECATE
#pragma warning(disable:4996)		// VC++ 2005 deprecated function warnings

#include <afxwin.h>         // MFC ��������ͱ�׼���
#include <afxext.h>         // MFC ��չ
#include <afxdisp.h>        // MFC �Զ�����
#include <afxmt.h>			// MFC support for CEvent class
#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC �� Internet Explorer 4 �����ؼ���֧��
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // MFC �� Windows �����ؼ���֧��
#include <afxcview.h>
#include <afxtempl.h>
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxcontrolbars.h>     // �������Ϳؼ����� MFC ֧��

//�ڴ�й¶���
#define _CRTDBG_MAP_ALLO
#include<stdlib.h>
#include<malloc.h>
#include <crtdbg.h>

#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif

#ifdef __EXPORT_LOGFILE_
#include "LogBinaryFile.h"
extern CLogBinFile loglic;
#endif


