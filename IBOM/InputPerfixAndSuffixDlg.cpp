// InputPerfixAndSuffixDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "IBOM.h"
#include "InputPerfixAndSuffixDlg.h"
#include "afxdialogex.h"


// CInputPerfixAndSuffixDlg 对话框

IMPLEMENT_DYNAMIC(CInputPerfixAndSuffixDlg, CDialogEx)

CInputPerfixAndSuffixDlg::CInputPerfixAndSuffixDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CInputPerfixAndSuffixDlg::IDD, pParent)
{

	m_sPrefix = _T("");
	m_sSuffix = _T("");
	m_iRotateAngle = 0;
}

CInputPerfixAndSuffixDlg::~CInputPerfixAndSuffixDlg()
{
}

void CInputPerfixAndSuffixDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_E_PREFIX, m_sPrefix);
	DDX_Text(pDX, IDC_E_SUFFIX, m_sSuffix);
	DDX_CBIndex(pDX, IDC_CMD_ROTATE_ANGLE, m_iRotateAngle);
}


BEGIN_MESSAGE_MAP(CInputPerfixAndSuffixDlg, CDialogEx)
END_MESSAGE_MAP()


// CInputPerfixAndSuffixDlg 消息处理程序


BOOL CInputPerfixAndSuffixDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	m_iRotateAngle=0;
	return TRUE;
}
