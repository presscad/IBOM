// InputPartInfoDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "IBOM.h"
#include "InputPartInfoDlg.h"
#include "afxdialogex.h"
#include "DataModel.h"
#include "ArrayList.h"
#include "Tool.h"
#include "PartLib.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

void AddJgRecord(CComboBox *pCMB)
{
	for(int i=0;i<jgguige_N;i++)
	{
		CString type;
		if( (int)(jgguige_table[i].wing_wide*10)%10>0||
			(int)(jgguige_table[i].wing_thick*10)%10>0)
		{
			type.Format("L%.1fX%.1f",	
				jgguige_table[i].wing_wide, jgguige_table[i].wing_thick);
		}
		else
		{
			type.Format("L%.0fX%.0f",	
				jgguige_table[i].wing_wide, jgguige_table[i].wing_thick);
		}
		if(pCMB->FindString(-1,type)==CB_ERR)
			pCMB->AddString(type);
	}
}
void AddPlateRecord(CComboBox *pCMB)
{
	pCMB->AddString("-2");
	pCMB->AddString("-4");
	pCMB->AddString("-5");
	pCMB->AddString("-6");
	pCMB->AddString("-8");
	pCMB->AddString("-10");
	pCMB->AddString("-12");
	pCMB->AddString("-14");
	pCMB->AddString("-16");
	pCMB->AddString("-18");
	pCMB->AddString("-20");

}
void AddTubeRecord(CComboBox *pCMB)
{
	for(int i=0;i<tubeguige_N;i++)
	{
		CString type;
		if((int)(tubeguige_table[i].D*10)%10>0&&(int)(tubeguige_table[i].thick*10)%10==0)
		{
			type.Format("%.1fX%.0f",	
				tubeguige_table[i].D, tubeguige_table[i].thick);
		}
		else if((int)(tubeguige_table[i].D*10)%10==0&&(int)(tubeguige_table[i].thick*10)%10>0)
		{
			type.Format("%.0fX%.1f",	
				tubeguige_table[i].D, tubeguige_table[i].thick);
		}
		else if((int)(tubeguige_table[i].D*10)%10>0&&(int)(tubeguige_table[i].thick*10)%10>0)
		{
			type.Format("%.1fX%.1f",	
				tubeguige_table[i].D, tubeguige_table[i].thick);
		}
		else
		{
			type.Format("%.0fX%.0f",	
				tubeguige_table[i].D, tubeguige_table[i].thick);
		}
		if(pCMB->FindString(-1,type)==CB_ERR)
			pCMB->AddString(type);
	}
}
void AddPartTypeRecord(CComboBox *pCMB)
{
	pCMB->AddString("0.角钢");
	pCMB->AddString("1.钢板");
	pCMB->AddString("2.钢管");
}
void AddMaterialRecord(CComboBox *pCMB)
{
	pCMB->AddString("Q235");
	pCMB->AddString("Q345");
	pCMB->AddString("Q420");
};
// CInputPartInfoDlg 对话框
IMPLEMENT_DYNAMIC(CInputPartInfoDlg, CDialog)
CInputPartInfoDlg::CInputPartInfoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CInputPartInfoDlg::IDD, pParent)
	, m_sPartNo(_T(""))
	, m_fLength(0)
	, m_nNum(0)
{
	m_fSumWeight=0;
	m_pBomPart=NULL;
}

CInputPartInfoDlg::~CInputPartInfoDlg()
{
}

void CInputPartInfoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_E_PARTNO, m_sPartNo);
	DDX_Text(pDX, IDC_E_LENGHT, m_fLength);
	DDX_Text(pDX, IDC_E_NUM, m_nNum);
	DDX_Text(pDX, IDC_E_WEIGHT, m_sWeight);
	DDX_Text(pDX, IDC_E_SUM_WEIGHT, m_fSumWeight);
	DDX_CBString(pDX, IDC_CMB_MATERIAL, m_sMaterial);
	DDX_CBString(pDX, IDC_CMB_GUIGE, m_sJgGuiGe);
}


BEGIN_MESSAGE_MAP(CInputPartInfoDlg, CDialog)
	ON_WM_PAINT()
	ON_EN_CHANGE(IDC_E_NUM,	&CInputPartInfoDlg::OnEnChangeSumWeight)
	ON_EN_CHANGE(IDC_E_WEIGHT, &CInputPartInfoDlg::OnEnChangeSumWeight)
END_MESSAGE_MAP()


// CInputPartInfoDlg 消息处理程序
BOOL CInputPartInfoDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	RefreshDlg();
	return TRUE;
}
void CInputPartInfoDlg::RelayoutWnd()
{
	RECT rcFull,rcPanel;
	GetClientRect(&rcFull);
	CWnd *pPanelWnd = GetDlgItem(IDC_PICTURE_PANEL);
	if(pPanelWnd==NULL)
		return;
	pPanelWnd->GetWindowRect(&rcPanel);
	ScreenToClient(&rcPanel);
	rcPanel.right=rcFull.right;
	pPanelWnd->MoveWindow(&rcPanel);
	//
	CImageRegionHost* pRegionHost=GetRegionHost();
	if(pRegionHost==NULL)
		return;
	int nCol=pRegionHost->GetImageColumnWidth();
	if(nCol>6&&m_pBomPart)
	{	//TODO:存在m_pBomPart==NULL情况
		IMAGE_DATA xImageData;
		pRegionHost->GetRectMonoImageData(&m_pBomPart->rc,&xImageData);
		double vfHoriScalePanel2Pic=(rcPanel.right-rcPanel.left+1)/(double)xImageData.nWidth;

		DYN_ARRAY<int> widthArr(nCol);
		pRegionHost->GetImageColumnWidth(widthArr);
		for(int i=0;i<nCol;i++)
			widthArr[i]=int(0.5+widthArr[i]*vfHoriScalePanel2Pic);
		//顶部标题控件调整
		CRect topRc1,topRc2,topRc3,topRc4,topRc5,topRc6,topRc7;
		//件号标题列widthArr[0]
		GetDlgItem(IDC_S_PARTNO)->GetWindowRect(&topRc1);
		ScreenToClient(&topRc1);
		topRc1.left=rcPanel.left;
		topRc1.right=topRc1.left+widthArr[0]-1;
		GetDlgItem(IDC_S_PARTNO)->MoveWindow(&topRc1);
		//材质、规格widthArr[1]
		GetDlgItem(IDC_S_MAT_GUIGE)->GetWindowRect(&topRc2);
		ScreenToClient(&topRc2);
		topRc2.left=topRc1.right+1;
		topRc2.right=topRc2.left+widthArr[1]-1;
		GetDlgItem(IDC_S_MAT_GUIGE)->MoveWindow(&topRc2);
		//长度widthArr[2]
		GetDlgItem(IDC_S_LEN)->GetWindowRect(&topRc3);
		ScreenToClient(&topRc3);
		topRc3.left=topRc2.right+1;
		topRc3.right=topRc3.left+widthArr[2]-1;
		GetDlgItem(IDC_S_LEN)->MoveWindow(&topRc3);
		//单基数widthArr[3]
		GetDlgItem(IDC_S_NUM)->GetWindowRect(&topRc4);
		ScreenToClient(&topRc4);
		topRc4.left=topRc3.right+1;
		topRc4.right=topRc4.left+widthArr[3]-1;
		GetDlgItem(IDC_S_NUM)->MoveWindow(&topRc4);
		//单重widthArr[4]
		GetDlgItem(IDC_S_WEIGHT)->GetWindowRect(&topRc5);
		ScreenToClient(&topRc5);
		topRc5.left=topRc4.right+1;
		topRc5.right=topRc5.left+widthArr[4]-1;
		GetDlgItem(IDC_S_WEIGHT)->MoveWindow(&topRc5);
		//总重widthArr[5]
		GetDlgItem(IDC_S_SUM_WEIGHT)->GetWindowRect(&topRc6);
		ScreenToClient(&topRc6);
		topRc6.left=topRc5.right+1;
		topRc6.right=topRc6.left+widthArr[5]-1;
		GetDlgItem(IDC_S_SUM_WEIGHT)->MoveWindow(&topRc6);
		//备注widthArr[6]
		GetDlgItem(IDC_S_NOTE)->GetWindowRect(&topRc7);
		ScreenToClient(&topRc7);
		topRc7.left=topRc6.right+1;
		topRc7.right=topRc7.left+widthArr[6]-1;
		GetDlgItem(IDC_S_NOTE)->MoveWindow(&topRc7);
		//底部编辑控件调整
		CRect btmRc1,btmRc2,btmRc3,btmRc4,btmRc5,btmRc6,btmRc7,btmBtn;
		//顶部件号编辑列widthArr[0]
		GetDlgItem(IDC_E_PARTNO)->GetWindowRect(&btmRc1);
		ScreenToClient(&btmRc1);
		btmRc1.left=rcPanel.left;
		btmRc1.right=btmRc1.left+widthArr[0]-1;
		GetDlgItem(IDC_E_PARTNO)->MoveWindow(&btmRc1);
		//材质、规格widthArr[1]
		int pre_width=(int)(widthArr[1]*0.4);
		GetDlgItem(IDC_CMB_MATERIAL)->GetWindowRect(&btmRc2);
		ScreenToClient(&btmRc2);
		btmRc2.left=btmRc1.right+1;
		btmRc2.right=btmRc2.left+pre_width-1;
		GetDlgItem(IDC_CMB_MATERIAL)->MoveWindow(&btmRc2);
		int next_width=widthArr[1]-pre_width;
		GetDlgItem(IDC_CMB_GUIGE)->GetWindowRect(&btmRc3);
		ScreenToClient(&btmRc3);
		btmRc3.left=btmRc2.right+1;
		btmRc3.right=btmRc3.left+next_width-1;
		GetDlgItem(IDC_CMB_GUIGE)->MoveWindow(&btmRc3);
		//长度widthArr[2]
		GetDlgItem(IDC_E_LENGHT)->GetWindowRect(&btmRc4);
		ScreenToClient(&btmRc4);
		btmRc4.left=btmRc3.right+1;
		btmRc4.right=btmRc4.left+widthArr[2]-1;
		GetDlgItem(IDC_E_LENGHT)->MoveWindow(&btmRc4);
		//单基数widthArr[3]
		GetDlgItem(IDC_E_NUM)->GetWindowRect(&btmRc5);
		ScreenToClient(&btmRc5);
		btmRc5.left=btmRc4.right+1;
		btmRc5.right=btmRc5.left+widthArr[3]-1;
		GetDlgItem(IDC_E_NUM)->MoveWindow(&btmRc5);
		//单重widthArr[4]
		GetDlgItem(IDC_E_WEIGHT)->GetWindowRect(&btmRc6);
		ScreenToClient(&btmRc6);
		btmRc6.left=btmRc5.right+1;
		btmRc6.right=btmRc6.left+widthArr[4]-1;
		GetDlgItem(IDC_E_WEIGHT)->MoveWindow(&btmRc6);
		//总重widthArr[5]
		GetDlgItem(IDC_E_SUM_WEIGHT)->GetWindowRect(&btmRc7);
		ScreenToClient(&btmRc7);
		btmRc7.left=btmRc6.right+1;
		btmRc7.right=btmRc7.left+widthArr[5]-1;
		GetDlgItem(IDC_E_SUM_WEIGHT)->MoveWindow(&btmRc7);
	}
}
void CInputPartInfoDlg::RefreshDlg()
{
	CImageRegionHost* pRegionHost=GetRegionHost();
	if(pRegionHost==NULL)
		return;
	CComboBox* pCMBMate=(CComboBox*)GetDlgItem(IDC_CMB_MATERIAL);
	AddMaterialRecord(pCMBMate);
	pCMBMate->SetCurSel(0);
	CComboBox* pCMBGuige=(CComboBox*)GetDlgItem(IDC_CMB_GUIGE);
	AddJgRecord(pCMBGuige);
	pCMBGuige->SetCurSel(0);
	if(m_pBomPart)
	{
		CComboBox* pCMBGuige = (CComboBox*)GetDlgItem(IDC_CMB_GUIGE);
		if(m_pBomPart->wPartType==IRecoginizer::BOMPART::ANGLE)
			AddJgRecord(pCMBGuige);
		else if(m_pBomPart->wPartType==IRecoginizer::BOMPART::PLATE)
			AddPlateRecord(pCMBGuige);
		else
			AddTubeRecord(pCMBGuige);
		m_sPartNo.Format("%s",m_pBomPart->sLabel);
		m_sMaterial.Format("%s",m_pBomPart->materialStr);
		m_sJgGuiGe.Format("%s",m_pBomPart->sSizeStr);
		m_fLength=m_pBomPart->length;
		m_nNum=m_pBomPart->count;
		m_sWeight.Format("%.2f",m_pBomPart->weight);
		m_fSumWeight=m_nNum*atof(m_sWeight);
	}
	else
	{
		m_sPartNo.Empty();
		m_sWeight.Empty();
		m_fLength=0;
		m_nNum=0;
	}
	RelayoutWnd();
	Invalidate();
	UpdateData(FALSE);
}
int CInputPartInfoDlg::GetRectHeight() 
{
	CRect rect;
	GetClientRect(&rect);
	return rect.Height();
}
void CInputPartInfoDlg::SetEditFocus()
{
	CWnd* pWnd=GetDlgItem(IDC_E_PARTNO);
	pWnd->SetFocus();
}
CImageRegionHost* CInputPartInfoDlg::GetRegionHost()
{
	CDisplayDataPage* pDisPage=theApp.GetIBomView()->GetDataPage();
	if(pDisPage==NULL)
		return NULL;
	CImageRegionHost* pRegionHost=(CImageRegionHost*)pDisPage->m_pData;
	return pRegionHost;
}
//
void CInputPartInfoDlg::OnPaint()
{
	CPaintDC dc(this);
	//绘制指定区域
	CImageRegionHost* pRegionHost=GetRegionHost();
	if(pRegionHost==NULL||m_pBomPart==NULL)
		return;
	CWnd *pWorkPanel=GetDlgItem(IDC_PICTURE_PANEL);
	CDC*  pDC=pWorkPanel->GetDC();	// 绘图控件的设备上下文
	if(pDC==NULL)
		return;
	CRect panelRect;
	pWorkPanel->GetClientRect(&panelRect);
	int width=m_pBomPart->rc.right-m_pBomPart->rc.left+1;
	int height=m_pBomPart->rc.bottom-m_pBomPart->rc.top+1;
	if(width>1&&height>1)
	{	
		IMAGE_DATA xImagedata;
		pRegionHost->GetRectMonoImageData(&m_pBomPart->rc,&xImagedata);
		CHAR_ARRAY bomPartBmpData(xImagedata.nEffWidth*xImagedata.nHeight);
		xImagedata.imagedata = bomPartBmpData;
		pRegionHost->GetRectMonoImageData(&m_pBomPart->rc,&xImagedata);
		//
		CBitmap bmp;
		CDC dcMemory;
		bmp.CreateBitmap(xImagedata.nWidth,xImagedata.nHeight,1,1,xImagedata.imagedata);
		dcMemory.CreateCompatibleDC(pDC);
		dcMemory.SelectObject(&bmp);
		pDC->StretchBlt(0,0,panelRect.Width(),panelRect.Height(),&dcMemory,0,0,xImagedata.nWidth,xImagedata.nHeight,SRCCOPY);
		dcMemory.DeleteDC();
	}
}
void CInputPartInfoDlg::OnOK()
{
	UpdateData();
	if(strlen(m_sPartNo)<=0)
	{
		AfxMessageBox("输入的件号有误，请重新输入！");
		return;
	}
	CImageRegionHost* pRegionHost=GetRegionHost();
	if(pRegionHost==NULL)
		return;
	if(m_pBomPart==NULL)
	{
		if(pRegionHost->FromPartNo(m_sPartNo))
		{
			AfxMessageBox("已存在此编号构件，请重新输入！");
			return;
		}
		m_pBomPart=pRegionHost->AddBomPart(m_sPartNo);
		strcpy(m_pBomPart->sLabel,m_sPartNo);
	}
	if(strcmp(m_pBomPart->sLabel,m_sPartNo)!=0)
	{	//更新件号
		if(pRegionHost->FromPartNo(m_sPartNo))
		{
			AfxMessageBox("已存在此编号构件，请重新输入！");
			return;
		}
		pRegionHost->ReplaceKey(m_pBomPart->sLabel,m_sPartNo);
		strcpy(m_pBomPart->sLabel,m_sPartNo);
	}
	strcpy(m_pBomPart->materialStr,m_sMaterial);
	strcpy(m_pBomPart->sSizeStr,m_sJgGuiGe);
	ParseSpec(m_pBomPart);
	m_pBomPart->count=(short)m_nNum;
	m_pBomPart->weight=atof(m_sWeight);
	m_pBomPart->length=m_fLength;
	//
	CDisplayDataPage* pDisPage=theApp.GetIBomView()->GetDataPage();
	pDisPage->RefreshListCtrl((long)m_pBomPart);
}
void CInputPartInfoDlg::OnCancel()
{
	CDisplayDataPage* pDisPage=theApp.GetIBomView()->GetDataPage();
	pDisPage->RefreshListCtrl((long)m_pBomPart);
}
void CInputPartInfoDlg::OnEnChangeSumWeight()
{
	UpdateData();
	m_fSumWeight=m_nNum*atof(m_sWeight);
	UpdateData(FALSE);
}
