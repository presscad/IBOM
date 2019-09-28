// DisplaySaveFileParameter.cpp : 实现文件
//

#include "stdafx.h"
#include "IBOM.h"
#include "AutoSaveParamDlg.h"
#include "afxdialogex.h"

// CAutoSaveParamDlg 对话框
IMPLEMENT_DYNAMIC(CAutoSaveParamDlg, CDialogEx)

CAutoSaveParamDlg::CAutoSaveParamDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CAutoSaveParamDlg::IDD, pParent)
	, m_bAnymorePrompt(FALSE)
	, m_sAutoSaveTime(_T(""))
{
	m_sAutoSaveTime='2';
	m_iAutoSaveType=0;
}

CAutoSaveParamDlg::~CAutoSaveParamDlg()
{
}

void CAutoSaveParamDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_AUTO_SAVE_TIME, m_sAutoSaveTime);
	DDX_Control(pDX, IDC_COM_SAVE_FILE_TYPE, m_cmbSvaeFileType);
}


BEGIN_MESSAGE_MAP(CAutoSaveParamDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CAutoSaveParamDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CDisplaySaveFileParameter 消息处理程序

void CAutoSaveParamDlg::OnBnClickedOk()
{
	CDialogEx::OnOK();
	UpdateData(TRUE);
	 m_iAutoSaveType=m_cmbSvaeFileType.GetCurSel();
	if(m_bAnymorePrompt)
		theApp.WriteProfileString("Settings","displayAutoSaveParamDlg","TRUE");
}
BOOL CAutoSaveParamDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	m_cmbSvaeFileType.AddString("单文件备份");
	m_cmbSvaeFileType.AddString("多文件备份");
	m_cmbSvaeFileType.SetCurSel(0);
	return TRUE;
}

