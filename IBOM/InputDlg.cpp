// InputDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "InputDlg.h"
#include "afxdialogex.h"


// CInputDlg �Ի���

IMPLEMENT_DYNAMIC(CInputDlg, CDialog)

CInputDlg::CInputDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CInputDlg::IDD, pParent)
	, m_sInputValue(_T(""))
{

}

CInputDlg::~CInputDlg()
{
}

void CInputDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_E_INPUT, m_sInputValue);
}


BEGIN_MESSAGE_MAP(CInputDlg, CDialog)
END_MESSAGE_MAP()


// CInputDlg ��Ϣ�������
