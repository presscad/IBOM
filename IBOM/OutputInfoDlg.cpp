// OutputInfoDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "IBOM.h"
#include "OutputInfoDlg.h"
#include "DataModel.h"
#include "SortFunc.h"

// COutputInfoDlg 对话框
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(COutputInfoDlg, CDialog)

COutputInfoDlg::COutputInfoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(COutputInfoDlg::IDD, pParent)
	, m_fLenTolerance(5)
{
	m_fWeightTolerance=0.5;
}

COutputInfoDlg::~COutputInfoDlg()
{
}

void COutputInfoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_OUTPUT_INFO_LIST, m_listInfoCtrl);
	DDX_Control(pDX, IDC_TAB_GROUP, m_tabInfoGroup);
	DDX_Text(pDX,IDC_E_RECORD,m_sRecord);
	DDX_Text(pDX, IDC_E_COMPARE_TOLERANCE, m_fLenTolerance);
	DDX_Text(pDX, IDC_E_COMPARE_WEIGHT_TOLERANCE, m_fWeightTolerance);
}


BEGIN_MESSAGE_MAP(COutputInfoDlg, CDialog)
	ON_WM_SIZE()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_GROUP, OnSelchangeTabGroup)
	ON_EN_CHANGE(IDC_E_COMPARE_TOLERANCE, OnChangeCompareTolerance)
	ON_EN_CHANGE(IDC_E_COMPARE_WEIGHT_TOLERANCE, OnChangeCompareTolerance)
	ON_WM_CHAR()
END_MESSAGE_MAP()

//
#include "ComparePartNoString.h"
int FireCompareItem(const CSuperGridCtrl::CSuperGridCtrlItemPtr& pItem1,const CSuperGridCtrl::CSuperGridCtrlItemPtr& pItem2,DWORD lPara)
{
	COMPARE_FUNC_EXPARA* pExPara=(COMPARE_FUNC_EXPARA*)lPara;
	int iSubItem=0;
	BOOL bAscending=true;
	if(pExPara)
	{
		iSubItem=pExPara->iSubItem;
		bAscending=pExPara->bAscending;
	}
	CString sText1=pItem1->m_lpNodeInfo->GetSubItemText(iSubItem);
	CString sText2=pItem2->m_lpNodeInfo->GetSubItemText(iSubItem);
	int status=0;
	if(pExPara&&iSubItem==0)
		status=ComparePartNoString(sText1,sText2);
	else if(iSubItem==1)	//规格
		status=CompareMultiSectionString(sText1,sText2);	
	//无论升序或降序空字符串都应排在非空字符串之后 wht 12-05-25
	else if(sText1.GetLength()==0&&sText2.GetLength()>0)
		return 1;	
	else if(sText1.GetLength()>0&&sText2.GetLength()==0)
		return -1;
	else if(iSubItem==3)	//长度
	{	//按整数排序
		int nItem1=atoi(sText1);
		int nItem2=atoi(sText2);
		if(nItem1>nItem2)
			status=1;
		else if(nItem1<nItem2)
			status=-1;
		else
			status=0;
	}
	else if(iSubItem==5||iSubItem==6)	//重量
	{	//按浮点数排序
		double fItem1=atof(sText1);
		double fItem2=atof(sText2);
		if(fItem1>fItem2)
			status=1;
		else if(fItem1<fItem2)
			status=-1;
		else
			status=0;
	}
	else//按字符串排序
		status=sText1.CompareNoCase(sText2);
	if(!bAscending)
		status*=-1;
	return status;
}
static CSuperGridCtrl::CTreeItem *InsertPartToList(CSuperGridCtrl &list,CSuperGridCtrl::CTreeItem *pParentItem,
	IRecoginizer::BOMPART *pPart,CHashStrList<BOOL> *pHashBoolByPropName=NULL,BOOL bReadOnly=TRUE)
{
	COLORREF clr=RGB(230,100,230);
	CListCtrlItemInfo *lpInfo=new CListCtrlItemInfo();
	//编号
	lpInfo->SetSubItemText(0,pPart->sLabel,bReadOnly);
	//规格
	lpInfo->SetSubItemText(1,pPart->sSizeStr,bReadOnly);
	if(pHashBoolByPropName&&pHashBoolByPropName->GetValue("spec"))
		lpInfo->SetSubItemColor(1,clr);
	//材质
	lpInfo->SetSubItemText(2,pPart->materialStr,bReadOnly);
	if(pHashBoolByPropName&&pHashBoolByPropName->GetValue("material"))
		lpInfo->SetSubItemColor(2,clr);
	//长度
	lpInfo->SetSubItemText(3,CXhChar50("%.f",pPart->length),bReadOnly);	
	if(pHashBoolByPropName&&pHashBoolByPropName->GetValue("length"))
		lpInfo->SetSubItemColor(3,clr);
	//单基数
	lpInfo->SetSubItemText(4,CXhChar50("%d",pPart->count),bReadOnly);
	if(pHashBoolByPropName&&pHashBoolByPropName->GetValue("count"))
		lpInfo->SetSubItemColor(4,clr);
	//单量
	CXhChar50 sValue("%.2f",pPart->weight);
	lpInfo->SetSubItemText(5,sValue,bReadOnly);
	if(pHashBoolByPropName&&pHashBoolByPropName->GetValue("weight"))
		lpInfo->SetSubItemColor(5,clr);
	//
	CSuperGridCtrl::CTreeItem *pItem=NULL;
	if(pParentItem)
		pItem=list.InsertItem(pParentItem,lpInfo,-1,true);
	else
		pItem=list.InsertRootItem(lpInfo);
	pItem->m_idProp=(long)pPart;
	return pItem;
}
// COutputInfoDlg 消息处理程序
BOOL COutputInfoDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	//
	m_tabInfoGroup.DeleteAllItems();
	m_tabInfoGroup.InsertItem(0,"同号差异");
	m_tabInfoGroup.InsertItem(1,"放样多件");
	m_tabInfoGroup.InsertItem(2,"放样缺件");
	m_tabInfoGroup.SetCurSel(0);
	//
	m_listInfoCtrl.EmptyColumnHeader();
	m_listInfoCtrl.AddColumnHeader("编号",90);	
	m_listInfoCtrl.AddColumnHeader("规格",90);	
	m_listInfoCtrl.AddColumnHeader("材质",70);	
	m_listInfoCtrl.AddColumnHeader("长度",75);	
	m_listInfoCtrl.AddColumnHeader("件数",70);
	m_listInfoCtrl.AddColumnHeader("单量",75);
	m_listInfoCtrl.AddColumnHeader("总重",70);
	m_listInfoCtrl.EnableSortItems(false,true);
	m_listInfoCtrl.SetHeaderFontHW(18,0);
	m_listInfoCtrl.SetGridLineColor(RGB(220,220,220));
	m_listInfoCtrl.SetEvenRowBackColor(RGB(224,237,236));
	m_listInfoCtrl.InitListCtrl();
	m_listInfoCtrl.SetCompareItemFunc(FireCompareItem);
	m_listInfoCtrl.DeleteAllItems();
	m_listInfoCtrl.SetExtendedStyle(m_listInfoCtrl.GetExtendedStyle()&~LVS_EX_HEADERDRAGDROP);
	//
	RelayoutWnd();
	return TRUE;
}
void COutputInfoDlg::RelayoutWnd()
{
	RECT rcFull,rcTop,rcBtm,rcRecord;
	GetClientRect(&rcFull);
	CWnd *pTopWnd = GetDlgItem(IDC_TAB_GROUP);
	CWnd* pRecWnd = GetDlgItem(IDC_E_RECORD);
	CWnd* pToleranceLabelWnd = GetDlgItem(IDC_S_TOLERANCE_LABEL);
	CWnd* pToleranceWnd = GetDlgItem(IDC_E_COMPARE_TOLERANCE);
	CWnd* pToleranceLabelWnd2 = GetDlgItem(IDC_S_TOLERANCE_LABEL2);
	CWnd* pToleranceWnd2 = GetDlgItem(IDC_E_COMPARE_WEIGHT_TOLERANCE);
	CWnd *pBtmWnd = GetDlgItem(IDC_OUTPUT_INFO_LIST);
	if(pTopWnd==NULL || pBtmWnd==NULL || pRecWnd==NULL)
		return;
	//
	int nWidth=0,nHight=0,moveup=0;
	pTopWnd->GetWindowRect(&rcTop);
	ScreenToClient(&rcTop);
	nWidth=rcTop.right-rcTop.left;
	nHight=rcTop.bottom-rcTop.top;
	moveup=rcTop.top;
	rcTop.top=0;
	rcTop.bottom=nHight;
	rcTop.left=0;
	rcTop.right=nWidth;
	pTopWnd->MoveWindow(&rcTop);
	//
	pRecWnd->GetWindowRect(&rcRecord);
	ScreenToClient(&rcRecord);
	nWidth=rcRecord.right-rcRecord.left;
	nHight=rcRecord.bottom-rcRecord.top;
	rcRecord.left=rcTop.right+5;
	rcRecord.right=rcRecord.left+nWidth;
	if(moveup>rcRecord.top)
		moveup=rcRecord.top;
	rcRecord.top-=moveup;
	rcRecord.bottom=rcRecord.top+nHight;
	pRecWnd->MoveWindow(&rcRecord);
	int iCurrPos=rcRecord.right;
	//
	pToleranceLabelWnd->GetWindowRect(&rcRecord);
	ScreenToClient(&rcRecord);
	nWidth=rcRecord.right-rcRecord.left;
	nHight=rcRecord.bottom-rcRecord.top;
	rcRecord.left=iCurrPos+10;
	rcRecord.right=rcRecord.left+nWidth;
	rcRecord.top-=moveup;
	rcRecord.bottom=rcRecord.top+nHight;
	pToleranceLabelWnd->MoveWindow(&rcRecord);
	iCurrPos=rcRecord.right;
	//
	pToleranceWnd->GetWindowRect(&rcRecord);
	ScreenToClient(&rcRecord);
	nWidth=rcRecord.right-rcRecord.left;
	nHight=rcRecord.bottom-rcRecord.top;
	rcRecord.left=iCurrPos+5;
	rcRecord.right=rcRecord.left+nWidth;
	rcRecord.top-=moveup;
	rcRecord.bottom=rcRecord.top+nHight;
	pToleranceWnd->MoveWindow(&rcRecord);
	iCurrPos=rcRecord.right;
	//
	pToleranceLabelWnd2->GetWindowRect(&rcRecord);
	ScreenToClient(&rcRecord);
	nWidth=rcRecord.right-rcRecord.left;
	nHight=rcRecord.bottom-rcRecord.top;
	rcRecord.left=iCurrPos+10;
	rcRecord.right=rcRecord.left+nWidth;
	rcRecord.top-=moveup;
	rcRecord.bottom=rcRecord.top+nHight;
	pToleranceLabelWnd2->MoveWindow(&rcRecord);
	iCurrPos=rcRecord.right;
	//
	pToleranceWnd2->GetWindowRect(&rcRecord);
	ScreenToClient(&rcRecord);
	nWidth=rcRecord.right-rcRecord.left;
	nHight=rcRecord.bottom-rcRecord.top;
	rcRecord.left=iCurrPos+5;
	rcRecord.right=rcRecord.left+nWidth;
	rcRecord.top-=moveup;
	rcRecord.bottom=rcRecord.top+nHight;
	pToleranceWnd2->MoveWindow(&rcRecord);
	//
	pBtmWnd->GetWindowRect(&rcBtm);
	ScreenToClient(&rcBtm);
	rcBtm.left=0;
	rcBtm.right=rcFull.right;
	rcBtm.top=(rcTop.bottom>rcRecord.bottom-moveup)?rcTop.bottom:rcRecord.bottom-moveup+2;
	rcBtm.bottom=rcFull.bottom;
	pBtmWnd->MoveWindow(&rcBtm);
}
void COutputInfoDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	RelayoutWnd();
}
void COutputInfoDlg::OnOK()
{

}
void COutputInfoDlg::OnCancel()
{

}
void COutputInfoDlg::OnSelchangeTabGroup(NMHDR* pNMHDR, LRESULT* pResult) 
{
	RefreshListCtrl();
}
void COutputInfoDlg::RefreshListCtrl()
{
	int iCurSel=m_tabInfoGroup.GetCurSel();
	CDataCmpModel::COMPARE_PART_RESULT *pResult=NULL;
	m_listInfoCtrl.DeleteAllItems();
	int nRecordNum=0;
	if(iCurSel==0)
	{	//图纸与放样不同
		for(pResult=dataModel.EnumFirstResult();pResult;pResult=dataModel.EnumNextResult())
		{
			if(pResult->pDwgPart&&pResult->pLoftPart)
			{	//有原始数据
				CSuperGridCtrl::CTreeItem *pItem=InsertPartToList(m_listInfoCtrl,NULL,pResult->pDwgPart);
				pItem->m_idProp=(long)pResult->pDwgPart;
				InsertPartToList(m_listInfoCtrl,pItem,pResult->pLoftPart,&pResult->hashBoolByPropName);
				nRecordNum++;
			}
		}
	}
	else if(iCurSel==1)
	{	//放样专有
		for(pResult=dataModel.EnumFirstResult();pResult;pResult=dataModel.EnumNextResult())
		{
			if(pResult->pDwgPart==NULL&&pResult->pLoftPart)
			{	
				CSuperGridCtrl::CTreeItem *pItem=InsertPartToList(m_listInfoCtrl,NULL,pResult->pLoftPart);
				pItem->m_idProp=(long)pResult->pLoftPart;
				nRecordNum++;
			}
		}
	}
	else if(iCurSel==2)
	{	//图纸专有
		for(pResult=dataModel.EnumFirstResult();pResult;pResult=dataModel.EnumNextResult())
		{
			if(pResult->pDwgPart&&pResult->pLoftPart==NULL)
			{	
				CSuperGridCtrl::CTreeItem *pItem=InsertPartToList(m_listInfoCtrl,NULL,pResult->pDwgPart);
				pItem->m_idProp=(long)pResult->pDwgPart;
				nRecordNum++;
			}
		}
	}
	//
	m_sRecord.Format("当前有%d项记录",nRecordNum);
	UpdateData(FALSE);
}

void COutputInfoDlg::OnChangeCompareTolerance()
{
	UpdateData();
}


void COutputInfoDlg::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialog::OnChar(nChar, nRepCnt, nFlags);
}
