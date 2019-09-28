// RunDBPDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "IBOM.h"
#include "RunDBPDlg.h"
#include "afxdialogex.h"
#include "Tool.h"
#include "InputAnValDlg.h"

// CRunDBPDlg 对话框

IMPLEMENT_DYNAMIC(CRunDBPDlg, CDialogEx)

CRunDBPDlg::CRunDBPDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CRunDBPDlg::IDD, pParent)
	, m_sCadPath(_T(""))
{

}

CRunDBPDlg::~CRunDBPDlg()
{
}

void CRunDBPDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CMB_CAD_PATH, m_xPathCmbBox);
	DDX_CBString(pDX, IDC_CMB_CAD_PATH, m_sCadPath);
}


BEGIN_MESSAGE_MAP(CRunDBPDlg, CDialogEx)
	ON_CBN_SELCHANGE(IDC_CMB_CAD_PATH, &CRunDBPDlg::OnCbnSelchangeCmbPath)
END_MESSAGE_MAP()


// CRunDBPDlg 消息处理程序
BOOL CRunDBPDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	//
	char cad_path[MAX_PATH]="",sSubKey[MAX_PATH]="";
	strcpy(sSubKey,"Software\\Xerofox\\IBOM\\Settings");
	GetRegKey(HKEY_CURRENT_USER,sSubKey,"CAD_PATH",cad_path);
	m_xPathCmbBox.InsertString(0,CString("<编辑...>"));
	m_xPathCmbBox.InsertString(1,CString("<浏览...>"));
	m_xPathCmbBox.InsertString(2,CString("<自动查找...>"));
	if(strlen(cad_path)>0)
	{
		m_xPathCmbBox.InsertString(3,CString(cad_path));
		m_xPathCmbBox.SetCurSel(3);
	}
	else
		m_xPathCmbBox.SetCurSel(2);
	return TRUE;
}
void CRunDBPDlg::OnOK()
{
	return CDialog::OnOK();
}
void CRunDBPDlg::OnCbnSelchangeCmbPath()
{
	char cadPath[MAX_PATH],sSubKey[MAX_PATH];
	int nIndex=m_xPathCmbBox.GetCurSel();
	CString path_str;
	if(nIndex==0)
	{
		CInputAnStringValDlg dlg;
		dlg.m_sItemTitle="输入CAD路径:";
		if(dlg.DoModal()==IDOK)
			path_str=dlg.m_sItemValue;
	}
	else if(nIndex==1)
	{
		CFileDialog dlg(TRUE,"exe","acad.exe",
			OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_ALLOWMULTISELECT,
			"可执行文件(*.exe)|*.exe|所有文件(*.*)|*.*||");
		if(dlg.DoModal()==IDOK)
			path_str=dlg.GetPathName();
	}
	else if(nIndex==2)
	{
		strcpy(sSubKey,"Software\\Xerofox\\IBOM\\Settings");
		SetRegKey(HKEY_CURRENT_USER,sSubKey,"CAD_PATH","");
	}
	else
		m_xPathCmbBox.GetLBText(nIndex,path_str);
	nIndex=m_xPathCmbBox.FindString(0,path_str);
	if(nIndex<0 && path_str.GetLength()>0)
	{
		int nCount=m_xPathCmbBox.GetCount();
		m_xPathCmbBox.InsertString(nCount,path_str);
	}
	UpdateData();
	m_sCadPath=path_str;
	UpdateData(FALSE);
	//保存CAD路径到注册表中
	if(GetCadPath(cadPath))
	{
		strcpy(sSubKey,"Software\\Xerofox\\IBOM\\Settings");
		SetRegKey(HKEY_CURRENT_USER,sSubKey,"CAD_PATH",cadPath);
	}
}
BOOL CRunDBPDlg::GetCadPath(char* cad_path)
{
	if(m_sCadPath.GetLength()<=0)
		return FALSE;
	char drive[4],dir[MAX_PATH];
	_splitpath(m_sCadPath,drive,dir,NULL,NULL);
	strcpy(cad_path,drive);
	strcat(cad_path,dir);
	return TRUE;
}