// FontsLibraryDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "IBOM.h"
#include "FontsLibraryDlg.h"
#include "afxdialogex.h"


// CFontsLibraryDlg 对话框

IMPLEMENT_DYNAMIC(CFontsLibraryDlg, CDialogEx)

CFontsLibraryDlg::CFontsLibraryDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CFontsLibraryDlg::IDD, pParent)
	, m_iFontsFamily(0)
	, m_sImageChar(_T("L"))
{

}

CFontsLibraryDlg::~CFontsLibraryDlg()
{
}

void CFontsLibraryDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_CBIndex(pDX, IDC_CMB_FONTS, m_iFontsFamily);
	DDX_CBString(pDX, IDC_CMB_CHAR_IMAGE, m_sImageChar);
}


BEGIN_MESSAGE_MAP(CFontsLibraryDlg, CDialogEx)
END_MESSAGE_MAP()


// CFontsLibraryDlg 消息处理程序
