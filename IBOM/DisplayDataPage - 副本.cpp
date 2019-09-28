// DisplayDdataPage.cpp : ʵ���ļ�
//
#include "stdafx.h"
#include "IBOM.h"
#include "IBOMView.h"
#include "DisplayDataPage.h"
#include "afxdialogex.h"
#include "ArrayList.h"
#include "Config.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

static void InsertOrUpdatePartTreeItem(CSuperGridCtrl &xListCtrl,IRecoginizer::BOMPART *pPart,CSuperGridCtrl::CTreeItem *pItem=NULL)
{
	CListCtrlItemInfo *lpInfo=NULL;
	if(pItem)
		lpInfo=pItem->m_lpNodeInfo;
	else
		lpInfo=new CListCtrlItemInfo();
	//���
	lpInfo->SetSubItemText(0,"",TRUE);
	if(pPart)
		lpInfo->SetSubItemText(0,pPart->sLabel,TRUE);
	//����
	lpInfo->SetSubItemText(1,"",TRUE);
	if(pPart&&stricmp(pPart->materialStr,"Q235")!=0)
		lpInfo->SetSubItemText(1,pPart->materialStr,TRUE);
	//���
	lpInfo->SetSubItemText(2,"",TRUE);
	if(pPart)
		lpInfo->SetSubItemText(2,pPart->sSizeStr,TRUE);
	//����
	lpInfo->SetSubItemText(3,"",TRUE);
	if(pPart)
		lpInfo->SetSubItemText(3,CXhChar50("%.f",pPart->length),TRUE);	
	//������
	lpInfo->SetSubItemText(4,"",TRUE);
	if(pPart)
		lpInfo->SetSubItemText(4,CXhChar50("%d",pPart->count),TRUE);
	//����
	lpInfo->SetSubItemText(5,"",TRUE);
	if(pPart)
		lpInfo->SetSubItemText(5,CXhChar50("%.2f",pPart->weight),TRUE);
	/*
	//����
	lpInfo->SetSubItemText(6,"",TRUE);
	if(pPart)
		lpInfo->SetSubItemText(6,CXhChar50("%.2f",pPart->weight*pPart->count),TRUE);
	*/
	if(pItem==NULL)
		pItem=pItem=xListCtrl.InsertRootItem(lpInfo);
	else
		xListCtrl.RefreshItem(pItem);
	pItem->m_idProp=(long)pPart;
	return;
}
static int ftol(double fVal)
{
	if(fVal>0)
		return (int)(fVal+0.5);
	else
		return (int)(fVal-0.5);
}
static BOOL FireItemChanged(CSuperGridCtrl* pListCtrl,CSuperGridCtrl::CTreeItem* pItem,NM_LISTVIEW* pNMListView)
{	
	CDisplayDataPage *pDlg=(CDisplayDataPage*)pListCtrl->GetParent();
	if(pDlg->m_cDisType!=CDataCmpModel::IMAGE_REGION_DATA ||pDlg->m_pData==NULL)
		return FALSE;

	CRect rc;
	CImageRegionHost* pRegion=(CImageRegionHost*)pDlg->m_pData;
	IRecoginizer::BOMPART* pBomPart=(IRecoginizer::BOMPART*)pItem->m_idProp;
	if(pBomPart&&!pRegion->GetBomPartRect(pBomPart->id,&rc))
		rc=pBomPart->rc;
	pDlg->Focus(rc,pListCtrl);
	pDlg->Invalidate(FALSE);
	return TRUE;
}
static BOOL FireDeleteItem(CSuperGridCtrl* pListCtrl,CSuperGridCtrl::CTreeItem* pItem)
{
	//�ӹ�ϣ����ɾ��
	CDisplayDataPage *pDlg=(CDisplayDataPage*)pListCtrl->GetParent();
	if(pDlg->m_cDisType!=CDataCmpModel::IMAGE_REGION_DATA ||pDlg->m_pData==NULL)
		return FALSE;
	CImageRegionHost* pRegionHost=(CImageRegionHost*)pDlg->m_pData;
	CString sValue=pItem->m_lpNodeInfo->GetSubItemText(0);
	if(sValue.GetLength()>0)
		pRegionHost->DeleteBomPart(sValue);
	pListCtrl->DeleteItem(pItem->GetIndex());
	return 1;
}
static BOOL FireMouseWheel(CSuperGridCtrl* pListCtrl,CSuperGridCtrl::CTreeItem* pItem,UINT nFlags, short zDelta, CPoint pt)
{
	CDisplayDataPage *pDlg=(CDisplayDataPage*)pListCtrl->GetParent();
	if(pDlg->m_cDisType!=CDataCmpModel::IMAGE_REGION_DATA ||pDlg->m_pData==NULL)
		return FALSE;
	if(pItem==NULL)
		return FALSE;
	CRect rc;
	CImageRegionHost* pRegion=(CImageRegionHost*)pDlg->m_pData;
	IRecoginizer::BOMPART* pBomPart=(IRecoginizer::BOMPART*)pItem->m_idProp;
	if(pBomPart&&!pRegion->GetBomPartRect(pBomPart->id,&rc))
		rc=pBomPart->rc;
	pDlg->Focus(rc,pListCtrl);
	pDlg->Invalidate(FALSE);
	return true;
}

int ConvertLabelToNum(const char* sLabel)
{
	int i=0,nLen=strlen(sLabel);
	for(i=0;i<nLen;i++)
	{
		if(!isdigit(sLabel[i]))
			break;
	}
	if(i<nLen)
		return 0;
	else
		return atoi(sLabel);
}

bool IsInputTextKey(WORD wVKeyCode)
{
	if( wVKeyCode==VK_NUMPAD0||
		wVKeyCode==VK_NUMPAD1||
		wVKeyCode==VK_NUMPAD2||
		wVKeyCode==VK_NUMPAD3||
		wVKeyCode==VK_NUMPAD4||
		wVKeyCode==VK_NUMPAD5||
		wVKeyCode==VK_NUMPAD6||
		wVKeyCode==VK_NUMPAD7||
		wVKeyCode==VK_NUMPAD8||
		wVKeyCode==VK_NUMPAD9||
		wVKeyCode==VK_SUBTRACT)
		return true;
	else
		return false;
}

CXhChar16 GetInputKeyText(WORD wVKeyCode)
{
	if( wVKeyCode==VK_NUMPAD0||
		wVKeyCode==VK_NUMPAD1||
		wVKeyCode==VK_NUMPAD2||
		wVKeyCode==VK_NUMPAD3||
		wVKeyCode==VK_NUMPAD4||
		wVKeyCode==VK_NUMPAD5||
		wVKeyCode==VK_NUMPAD6||
		wVKeyCode==VK_NUMPAD7||
		wVKeyCode==VK_NUMPAD8||
		wVKeyCode==VK_NUMPAD9)
	{
		return CXhChar16("%d",wVKeyCode-VK_NUMPAD0);
	}
	else if(wVKeyCode==VK_SUBTRACT)
		return CXhChar16("-");
	else
		return CXhChar16("");
}

static BOOL FireKeyDownItem(CSuperGridCtrl* pListCtrl,CSuperGridCtrl::CTreeItem* pItem,LV_KEYDOWN* pLVKeyDow)
{
	CDisplayDataPage *pDlg=(CDisplayDataPage*)pListCtrl->GetParent();
	if(pDlg->m_cDisType!=CDataCmpModel::IMAGE_REGION_DATA ||pDlg->m_pData==NULL)
		return FALSE;
	if(pItem==NULL)
		return FALSE;
	if(pDlg->biLayoutType!=3&&pDlg->biLayoutType!=4)
		return FALSE;
	BYTE biCurCol=pDlg->biCurCol;
	WORD wVKey=CConfig::GetCfgVKCode(pLVKeyDow->wVKey);
	IRecoginizer::BOMPART *pCurPart=pItem!=NULL?(IRecoginizer::BOMPART*)pItem->m_idProp:NULL;
	if(wVKey==CConfig::CFG_VK_REPEAT)
	{
		int iCurSel=pItem->GetIndex();
		CSuperGridCtrl::CTreeItem *pPrevItem=iCurSel>0?pListCtrl->GetTreeItem(iCurSel-1):NULL;
		IRecoginizer::BOMPART *pPrevPart=pPrevItem!=NULL?(IRecoginizer::BOMPART*)pPrevItem->m_idProp:NULL;
		if(pPrevItem&&pPrevPart)
		{
			if(biCurCol==0)
			{	//�����У����ż�1
				int nLabel=ConvertLabelToNum(pPrevPart->sLabel);
				if(nLabel>0)
				{
					strcpy(pCurPart->sLabel,CXhChar16("%d",nLabel+1));
					pListCtrl->SetSubItemText(pItem,0,pCurPart->sLabel);
				}
			}
			else //if(biCurCol==1)
			{	//�������ظ�ǰһ��
				CString sText=pPrevItem->m_lpNodeInfo->GetSubItemText(biCurCol);
				pListCtrl->SetSubItemText(pItem,biCurCol,sText);
				if(biCurCol==1)
					strcpy(pCurPart->materialStr,sText);
				else if(biCurCol==2)
					strcpy(pCurPart->sSizeStr,sText);
				else if(biCurCol==3)
					pCurPart->length=atoi(sText);
				else if(biCurCol==4)
					pCurPart->count=atoi(sText);
			}
			//�޸���֮����ת����һ��
			wVKey=CConfig::CFG_VK_DOWN;
		}
		else
			return FALSE;
	}
	else if(wVKey==CConfig::CFG_VK_RETURN||wVKey==CConfig::CFG_VK_SPACE)
	{	//��������״̬ȷ��,��ת����һ��
		wVKey=CConfig::CFG_VK_DOWN;
	}
	else if( wVKey==CConfig::CFG_VK_Q235||
			 wVKey==CConfig::CFG_VK_Q345||
			 wVKey==CConfig::CFG_VK_Q390||
			 wVKey==CConfig::CFG_VK_Q420)
	{	//ͨ����ݼ�¼�����
		if(pItem&&pCurPart&&pDlg->biCurCol==CDisplayDataPage::COL_MATERIAL)
		{
			CXhChar16 sMaterial;
			if(wVKey==CConfig::CFG_VK_Q345)
				sMaterial.Copy("Q345");
			else if(wVKey==CConfig::CFG_VK_Q390)
				sMaterial.Copy("Q390");
			else if(wVKey==CConfig::CFG_VK_Q420)
				sMaterial.Copy("Q420");
			pListCtrl->SetSubItemText(pItem,biCurCol,sMaterial);
			strcpy(pCurPart->materialStr,sMaterial);
		}
	}
	else if(IsInputTextKey(wVKey))
	{	//���ֵ�����������ַ�
		if(!pListCtrl->IsDisplayCellCtrl(pItem->GetIndex(),biCurCol))
			pListCtrl->DisplayCellCtrl(pItem->GetIndex(),biCurCol,GetInputKeyText(wVKey));
		return FALSE;
	}

	if(wVKey==CConfig::CFG_VK_UP||wVKey==CConfig::CFG_VK_DOWN)
	{
		CRect rc;
		pListCtrl->GetItemRect(pItem->GetIndex(),&rc,LVIR_BOUNDS);
		CSize sz(0,wVKey==CConfig::CFG_VK_UP?rc.Height()*-1:rc.Height());
		pListCtrl->Scroll(sz);
		int iCurSel=pItem->GetIndex();
		if(wVKey==CConfig::CFG_VK_UP)
		{
			if(iCurSel>=CDisplayDataPage::HEAD_FIX_ROW_COUNT)
				iCurSel--;
		}
		else if(wVKey==CConfig::CFG_VK_DOWN)
		{
			if(iCurSel<pListCtrl->GetItemCount()-CDisplayDataPage::TAIL_FIX_ROW_COUNT-1)
				iCurSel++;
		}
		if(iCurSel!=pItem->GetIndex()&&iCurSel>=0&&iCurSel<=pListCtrl->GetItemCount())
		{
			CSuperGridCtrl::CTreeItem *pNextItem=pListCtrl->GetTreeItem(iCurSel);
			if(pNextItem&&pNextItem->m_idProp!=NULL)
				pListCtrl->SelectItem(pNextItem);
		}
		return TRUE;
	}
	else if(wVKey==CConfig::CFG_VK_LEFT||wVKey==CConfig::CFG_VK_RIGHT)
	{
		int iCurCol=biCurCol;
		if(wVKey==CConfig::CFG_VK_LEFT)
		{
			if(iCurCol>0)
				iCurCol--;
		}
		else if(wVKey==CConfig::CFG_VK_RIGHT)
		{
			if(iCurCol<5)
				iCurCol++;
		}
		if(iCurCol!=pListCtrl->GetCurSubItem()&&iCurCol>=0&&iCurCol<5)
		{
			pListCtrl->SelectItem(pItem,iCurCol);
			pDlg->ShiftCheckColumn(iCurCol);
		}
		return TRUE;
	}
	else if(wVKey==CConfig::CFG_VK_HOME||wVKey==CConfig::CFG_VK_END)
	{
		int iCurSel=(wVKey==CConfig::CFG_VK_HOME)?CDisplayDataPage::HEAD_FIX_ROW_COUNT:pListCtrl->GetItemCount()-CDisplayDataPage::TAIL_FIX_ROW_COUNT-1;
		CSuperGridCtrl::CTreeItem *pNewSelItem=pListCtrl->GetTreeItem(iCurSel);
		if(pNewSelItem)
		{
			if(wVKey==CConfig::CFG_VK_HOME)
				pListCtrl->SendMessage(LVM_SCROLL, SB_TOP, 0);
			pListCtrl->SelectItem(pNewSelItem,pDlg->biCurCol);
			IRecoginizer::BOMPART *pBomPart=(IRecoginizer::BOMPART*)pNewSelItem->m_idProp;
			CImageRegionHost *pRegion=(CImageRegionHost*)pDlg->m_pData;
			RECT rc;
			if(pBomPart&&pRegion&&pRegion->GetBomPartRect(pBomPart->id,&rc))
			{
				rc=pBomPart->rc;
				pDlg->Focus(rc,pListCtrl);
			}
		}
		return TRUE;
	}
	else
		return FALSE;
}

static BOOL FireModifyValue(CSuperGridCtrl* pListCtrl,CSuperGridCtrl::CTreeItem* pItem,int iSubItem,CString& sTextValue)
{
	CDisplayDataPage *pDlg=(CDisplayDataPage*)pListCtrl->GetParent();
	if(pDlg->m_cDisType!=CDataCmpModel::IMAGE_REGION_DATA ||pDlg->m_pData==NULL)
		return FALSE;
	if(pItem==NULL)
		return FALSE;
	if(pDlg->biLayoutType!=3&&pDlg->biLayoutType!=4)
		return FALSE;
	BYTE biCurCol=pDlg->biCurCol;
	IRecoginizer::BOMPART *pCurPart=pItem!=NULL?(IRecoginizer::BOMPART*)pItem->m_idProp:NULL;
	if(biCurCol==CDisplayDataPage::COL_LABEL)
		StrCopy(pCurPart->sLabel,sTextValue,16);
	else if(biCurCol==CDisplayDataPage::COL_SPEC)
	{
		if(sTextValue.FindOneOf("-")>=0)
			StrCopy(pCurPart->sSizeStr,sTextValue,16);
		else if(sTextValue.FindOneOf("*")>=0)
		{
			if(sTextValue.FindOneOf("L")>=0)
				sTextValue.Replace('*','X');
			else 
				sTextValue.Format("L%s",sTextValue);
			StrCopy(pCurPart->sSizeStr,sTextValue,16);
		}
		else //�������뷨
			StrCopy(pCurPart->sSizeStr,CConfig::GetAngleSpecByKey(sTextValue),16);
	}
	else if(biCurCol==CDisplayDataPage::COL_LEN)
	{
		if(sTextValue.GetLength()>0)
			pCurPart->length=atoi(sTextValue);
	}
	else if(biCurCol==CDisplayDataPage::COL_COUNT)
	{
		if(sTextValue.GetLength()>0)
			pCurPart->count=atoi(sTextValue);
	}
	return TRUE;
}

static BYTE GetOffsetState(CRect panelRect,CRect imageRect)
{	//��������
	BYTE byteState=0xFF;
	BYTE byteArr[4]={0x01,0x02,0x04,0x08};
	CSize offset(imageRect.left,imageRect.top);
	//�ж��Ƿ��ܹ������ƶ�
	if(offset.cx<0&&(imageRect.Width()-abs(offset.cx))<panelRect.Width())
		byteState&=~byteArr[2]; //��������
	else if(offset.cx>0)
		byteState&=~byteArr[3];	//��������
	//�ж��Ƿ��ܹ������ƶ�
	if(offset.cy<0&&(imageRect.Height()-abs(offset.cy))<panelRect.Height())
		byteState&=~byteArr[0];	//��������
	else if(offset.cy>0)
		byteState&=~byteArr[1];	//��������
	return byteState;
}
//////////////////////////////////////////////////////////////////////////
// CDisplayDdataPage �Ի���
IMPLEMENT_DYNCREATE(CDisplayDataPage, CDialog)

CDisplayDataPage::CDisplayDataPage(CWnd* pParent /*=NULL*/)
	: CDialog(CDisplayDataPage::IDD, pParent)
	, m_sContext(_T(""))
	, m_nRecordNum(0)
	, m_iModel(0)
{
	m_cDisType=0;
	m_pData=NULL;
	m_nOldHorzY=0;
	m_nInputDlgH=0;
	m_bHorzTracing=FALSE;
	m_biLayoutType=0;
	m_biCurCol=0;
	m_fDividerYScale=0.5;
	m_bStartDrag=FALSE;
	m_cCurTask=0;
	m_hCursor=NULL;
	m_bTraced=false;
}

CDisplayDataPage::~CDisplayDataPage()
{
}

void CDisplayDataPage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_CTRL, m_listDataCtrl);
	DDX_Control(pDX, IDC_PREV_LIST_CTRL, m_listDataCtrl1);
	DDX_Text(pDX, IDC_E_CONTEXT, m_sContext);
	DDX_Text(pDX, IDC_E_RECORD_NUM, m_nRecordNum);
	DDX_Control(pDX, IDC_SLIDER_BALANCE_COEF, m_sliderBalanceCoef);
	DDX_Control(pDX, IDC_S_BALANCE_COEF, m_balanceCoef);
	DDX_Control(pDX, IDC_CMB_MODEL, m_cmdModel);
	DDX_CBIndex(pDX, IDC_CMB_MODEL, m_iModel);
}


BEGIN_MESSAGE_MAP(CDisplayDataPage, CDialog)
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_KEYDOWN()
	ON_WM_CHAR()
	ON_WM_SETCURSOR()
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER_BALANCE_COEF, &CDisplayDataPage::OnNMCustomdrawSliderBalanceCoef)
	ON_CBN_SELCHANGE(IDC_CMB_MODEL, &CDisplayDataPage::OnCbnSelchangeCmbModel)
	ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()
// CDisplayDdataPage ��Ϣ�������
int FireCompareItem(const CSuperGridCtrl::CSuperGridCtrlItemPtr& pItem1,const CSuperGridCtrl::CSuperGridCtrlItemPtr& pItem2,DWORD lPara);
BOOL CDisplayDataPage::OnInitDialog()
{
	CDialog::OnInitDialog();
	((CEdit*)GetDlgItem(IDC_E_CONTEXT))->SetReadOnly(TRUE);
	((CEdit*)GetDlgItem(IDC_E_RECORD_NUM))->SetReadOnly(TRUE);
	m_xInputPartInfoPage.Create(IDD_INPUT_PARTINFO_DLG,GetDlgItem(IDC_SUB_WORK_PANEL));
	m_nInputDlgH=m_xInputPartInfoPage.GetRectHeight();
	//
	m_arrColWidth[0]=80;
	m_arrColWidth[1]=90;
	m_arrColWidth[2]=120;
	m_arrColWidth[3]=90;
	m_arrColWidth[4]=90;
	m_arrColWidth[5]=120;
	m_arrColWidth[6]=30;
	CXhChar100 colNameArr[7]={"����","����","���","����","������","����(Kg)",".."};
	m_listDataCtrl.EmptyColumnHeader();
	m_listDataCtrl1.EmptyColumnHeader();
	for(int i=0;i<7;i++)
	{
		m_listDataCtrl.AddColumnHeader(colNameArr[i],m_arrColWidth[i]);	
		m_listDataCtrl1.AddColumnHeader(colNameArr[i],m_arrColWidth[i]);
	}
	m_listDataCtrl.InitListCtrl();
	m_listDataCtrl.EnableSortItems(TRUE);
	m_listDataCtrl.SetDeleteItemFunc(FireDeleteItem);
	m_listDataCtrl.SetItemChangedFunc(FireItemChanged);
	m_listDataCtrl.SetCompareItemFunc(FireCompareItem);
	m_listDataCtrl.SetMouseWheelFunc(FireMouseWheel);
	m_listDataCtrl.SetKeyDownItemFunc(FireKeyDownItem);
	m_listDataCtrl.SetModifyValueFunc(FireModifyValue);
	//
	m_listDataCtrl1.InitListCtrl();
	m_listDataCtrl1.EnableSortItems(FALSE);
	m_listDataCtrl1.SetDeleteItemFunc(FireDeleteItem);
	m_listDataCtrl1.SetItemChangedFunc(FireItemChanged);
	//m_listDataCtrl1.SetCompareItemFunc(FireCompareItem);
	m_listDataCtrl1.SetMouseWheelFunc(FireMouseWheel);
	//m_listDataCtrl1.SetKeyDownItemFunc(FireKeyDownItem);

	//��������ͷͼ��
	GetClientRect(&m_rcClient);
	m_hCursor=NULL;
	m_hArrowCursor=LoadCursorA(NULL,IDC_ARROW);	//��֪ΪʲôAfxGetApp()->LoadCursorA(IDC_ARROW)�����ʧ��
	m_hPin=AfxGetApp()->LoadCursorA(IDC_CUR_PIN);
	m_hHorzSize=AfxGetApp()->LoadStandardCursor(IDC_SIZENS);
	m_hSnap=AfxGetApp()->LoadStandardCursor(IDC_SIZEALL);
	m_hMove=AfxGetApp()->LoadStandardCursor(IDC_HAND);
	m_hCross=AfxGetApp()->LoadStandardCursor(IDC_CROSS);
	//
	m_sliderBalanceCoef.SetRange(0,100);
	m_sliderBalanceCoef.SetPos(50);
	while(m_cmdModel.GetCount()>0)
		m_cmdModel.DeleteString(0);
	m_cmdModel.AddString("�鿴ģʽ");
	m_cmdModel.AddString("����У��");
	m_cmdModel.AddString("����У��");
	m_cmdModel.SetCurSel(1);
	return TRUE;
}
BOOL CDisplayDataPage::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_KEYDOWN)
	{
		if(CConfig::IsInputKey(pMsg->wParam))
		{
			int iItem=m_listDataCtrl.GetSelectedItem();
			int iCol=m_listDataCtrl.GetCurSubItem();
			CWnd *pFocusWnd=GetFocus();
			if(!m_listDataCtrl.IsDisplayCellCtrl(iItem,iCol))
			{
				if(pFocusWnd==NULL&&pFocusWnd!=&m_listDataCtrl&&pFocusWnd->GetParent()!=&m_listDataCtrl)
					m_listDataCtrl.SetFocus();
			}
			m_listDataCtrl.SendMessage(pMsg->message,pMsg->wParam,pMsg->lParam);
		}
		else
			SendMessage(pMsg->message,pMsg->wParam,pMsg->lParam);
		return 0;
	}
	else
		return CDialog::PreTranslateMessage(pMsg);
}
bool CDisplayDataPage::Focus(RECT& rc,CSuperGridCtrl *pListCtrl)
{
	CWnd *pWorkPanel=GetDlgItem(IDC_WORK_PANEL);
	if(pWorkPanel==NULL)
		return false;
	RECT rcPanel;
	pWorkPanel->GetClientRect(&rcPanel);
	CImageRegionHost* pRegion=(CImageRegionHost*)m_pData;
	CPoint topLeft =TransPoint(CPoint(rc.left,rc.top),FALSE,TRUE);
	CPoint btmRight=TransPoint(CPoint(rc.right,rc.bottom),FALSE,TRUE);
	CSize xOffset=pRegion->GetScrOffset();
	if(m_biLayoutType==3||m_biLayoutType==2)
	{	//���в鿴
		CSuperGridCtrl *pOtherListCtrl=NULL;
		if(pListCtrl==&m_listDataCtrl)
			pOtherListCtrl=&m_listDataCtrl1;
		else if(pListCtrl==&m_listDataCtrl1)
			pOtherListCtrl=&m_listDataCtrl;
		else if(pListCtrl==NULL)
		{
			pListCtrl=&m_listDataCtrl;
			pOtherListCtrl=&m_listDataCtrl1;
		}
		
		int iCurSel=pListCtrl->GetSelectedItem();
		CRect rect;
		pListCtrl->GetItemRect(iCurSel,&rect,LVIR_BOUNDS);
		double fPictureMidY=(topLeft.y+btmRight.y)*0.5;
		double fListMidY=(rect.top+rect.bottom)*0.5;
		xOffset.cy+=(int)(fListMidY-fPictureMidY);
		//������һ���б���ڶ����б�ͬ��
		if(m_biCurCol>0)
		{
			CRect rcItem;
			/*int iDestCurSel=m_listDataCtrl.GetSelectedItem();
			int iSrcCurSel=m_listDataCtrl1.GetSelectedItem();
			int i1=m_listDataCtrl.GetItemCount();
			int i2=m_listDataCtrl1.GetItemCount();
			m_listDataCtrl.GetItemRect(iDestCurSel, rcItem, LVIR_BOUNDS);
			CSize sz(0, (iDestCurSel - iSrcCurSel)*rcItem.Height());
			m_listDataCtrl1.Scroll(sz);*/
			int pos=pListCtrl->GetScrollPos(SB_VERT);
			//pOtherListCtrl->SetScrollPos(SB_VERT,pos,TRUE);
			int otherPos=pOtherListCtrl->GetScrollPos(SB_VERT);
			int wParam=0,lParam=0;
			lParam = (pos - otherPos) * rect.Height();
			pOtherListCtrl->SendMessage(LVM_SCROLL, wParam, lParam);
			int iCurSel=pListCtrl->GetSelectedItem();
			if(iCurSel>=0)
			{
				CSuperGridCtrl::CTreeItem *pSelItem1=pOtherListCtrl->GetTreeItem(iCurSel);
				pOtherListCtrl->SelectItem(pSelItem1,m_biCurCol);
			}
		}
	}
	else
	{
		if(topLeft.y<0)
			xOffset.cy+=(-topLeft.y+5);
		else if(btmRight.y>rcPanel.bottom)
			xOffset.cy-=(btmRight.y-rcPanel.bottom+5);
	}
	pRegion->SetScrOffset(xOffset);
	Invalidate(FALSE);
	return true;
}
void CDisplayDataPage::RefreshListCtrl(long idSelProp/*=0*/)
{
	m_listDataCtrl.DeleteAllItems();
	m_listDataCtrl1.DeleteAllItems();
	if(m_cDisType==CDataCmpModel::DWG_DATA_MODEL)
	{
		m_sContext="����ͼֽ�Ĺ�����ϸ";
		m_nRecordNum=0;
		CHashStrList<IRecoginizer::BOMPART*> dwgPartSet;
		dataModel.SummarizeDwgParts(dwgPartSet);
		for(IRecoginizer::BOMPART** ppBomPart=dwgPartSet.GetFirst();ppBomPart;ppBomPart=dwgPartSet.GetNext())
		{
			InsertOrUpdatePartTreeItem(m_listDataCtrl,*ppBomPart);
			m_nRecordNum++;
		}
	}
	else if(m_cDisType==CDataCmpModel::LOFT_DATA_MODEL)
	{
		m_sContext="����ģ�͵ĵĹ�����ϸ";
		m_nRecordNum=0;
		CHashStrList<IRecoginizer::BOMPART*> loftPartSet;
		dataModel.SummarizeLoftParts(loftPartSet);
		for(IRecoginizer::BOMPART** ppBomPart=loftPartSet.GetFirst();ppBomPart;ppBomPart=loftPartSet.GetNext())
		{
			InsertOrUpdatePartTreeItem(m_listDataCtrl,*ppBomPart);
			m_nRecordNum++;
		}
	}
	else if(m_cDisType==CDataCmpModel::DWG_IMAGE_GROUP)
	{
		m_sContext="ͼֽ����-ͼƬ�ļ��Ĺ�����ϸ";
		m_nRecordNum=0;
		for(CImageFileHost *pImageFile=dataModel.EnumFirstImage();pImageFile;pImageFile=dataModel.EnumNextImage())
		{
			pImageFile->SummarizePartInfo();
			for(IRecoginizer::BOMPART* pBomPart=pImageFile->EnumFirstPart();pBomPart;pBomPart=pImageFile->EnumNextPart())
			{
				InsertOrUpdatePartTreeItem(m_listDataCtrl,pBomPart);
				m_nRecordNum++;
			}
		}
	}
	else if(m_cDisType==CDataCmpModel::DWG_BOM_GROUP)
	{
		m_sContext="ͼֽ����-BOM�ļ��Ĺ�����ϸ";
		m_nRecordNum=0;
		for(CBomFileData* pBomFile=dataModel.EnumFirstDwgBom();pBomFile;pBomFile=dataModel.EnumNextDwgBom())
		{
			for(IRecoginizer::BOMPART* pBomPart=pBomFile->EnumFirstPart();pBomPart;pBomPart=pBomFile->EnumNextPart())
			{
				InsertOrUpdatePartTreeItem(m_listDataCtrl,pBomPart);
				m_nRecordNum++;
			}
		}
	}
	else if(m_cDisType==CDataCmpModel::LOFT_BOM_GROUP)
	{
		m_sContext="ģ������-BOM�ļ��Ĺ�����ϸ";
		m_nRecordNum=0;
		for(CBomFileData* pBomFile=dataModel.EnumFirstLoftBom();pBomFile;pBomFile=dataModel.EnumNextLoftBom())
		{
			for(IRecoginizer::BOMPART* pBomPart=pBomFile->EnumFirstPart();pBomPart;pBomPart=pBomFile->EnumNextPart())
			{
				InsertOrUpdatePartTreeItem(m_listDataCtrl,pBomPart);
				m_nRecordNum++;
			}
		}
	}
	else if(m_cDisType==CDataCmpModel::LOFT_BOM_DATA && m_pData)
	{	//����Bom�ļ���ϸ
		CBomFileData* pLoftBomFile=(CBomFileData*)m_pData;
		m_sContext.Format("�ļ�(%s)�ж�ȡ�Ĺ�����ϸ",(char*)pLoftBomFile->m_sName);
		m_nRecordNum=0;
		for(IRecoginizer::BOMPART* pBomPart=pLoftBomFile->EnumFirstPart();pBomPart;pBomPart=pLoftBomFile->EnumNextPart())
		{
			InsertOrUpdatePartTreeItem(m_listDataCtrl,pBomPart);
			m_nRecordNum++;
		}
	}
	else if(m_cDisType==CDataCmpModel::IMAGE_SEG_GROUP && m_pData)
	{	//ͼֽ�ֶ�����
		SEGITEM* pSegItem=(SEGITEM*)m_pData;
		m_sContext.Format("%s����ͼƬ�Ĺ�����ϸ",(char*)pSegItem->sSegName);
		m_nRecordNum=0;
		for(CImageFileHost *pImageFile=dataModel.EnumFirstImage(pSegItem->iSeg);pImageFile;pImageFile=dataModel.EnumNextImage(pSegItem->iSeg))
		{
			pImageFile->SummarizePartInfo();
			for(IRecoginizer::BOMPART* pBomPart=pImageFile->EnumFirstPart();pBomPart;pBomPart=pImageFile->EnumNextPart())
			{
				InsertOrUpdatePartTreeItem(m_listDataCtrl,pBomPart);
				m_nRecordNum++;
			}
		}
	}
	else if(m_cDisType==CDataCmpModel::DWG_BOM_DATA && m_pData)
	{	//ͼֽ-BOM�ļ���ϸ
		CBomFileData* pBomFile=(CBomFileData*)m_pData;
		m_sContext.Format("�ļ�(%s)�ж�ȡ�Ĺ�����ϸ",(char*)pBomFile->m_sName);
		m_nRecordNum=0;
		for(IRecoginizer::BOMPART* pBomPart=pBomFile->EnumFirstPart();pBomPart;pBomPart=pBomFile->EnumNextPart())
		{
			InsertOrUpdatePartTreeItem(m_listDataCtrl,pBomPart);
			m_nRecordNum++;
		}
	}
	else if(m_cDisType==CDataCmpModel::DWG_IMAGE_DATA && m_pData)
	{	//��ʾѡ��ͼƬ������
		CImageFileHost* pImageHost=(CImageFileHost*)m_pData;
		m_sContext.Format("�ļ�(%s)�ж�ȡ�Ĺ�����ϸ",(char*)pImageHost->szFileName);
		m_nRecordNum=0;
		pImageHost->SummarizePartInfo();
		for(IRecoginizer::BOMPART* pBomPart=pImageHost->EnumFirstPart();pBomPart;pBomPart=pImageHost->EnumNextPart())
		{
			InsertOrUpdatePartTreeItem(m_listDataCtrl,pBomPart);
			m_nRecordNum++;
		}
	}
	else if(m_cDisType==CDataCmpModel::IMAGE_REGION_DATA && m_pData)
	{	//��ʾ����������ȡ���
		CImageRegionHost *pRegion=(CImageRegionHost*)m_pData;
		m_sContext.Format("%s����ȡ�Ĺ�����ϸ",(char*)pRegion->m_sName);
		if(m_biLayoutType==3)
		{	//����հ���,
			for(int i=0;i<HEAD_FIX_ROW_COUNT;i++)
			{
				InsertOrUpdatePartTreeItem(m_listDataCtrl,NULL);
				InsertOrUpdatePartTreeItem(m_listDataCtrl1,NULL);
			}
		}
		m_nRecordNum=0;
		for(IRecoginizer::BOMPART* pBomPart=pRegion->EnumFirstPart();pBomPart;pBomPart=pRegion->EnumNextPart())
		{
			InsertOrUpdatePartTreeItem(m_listDataCtrl,pBomPart);
			InsertOrUpdatePartTreeItem(m_listDataCtrl1,pBomPart);
			m_nRecordNum++;
		}
		if(m_biLayoutType==3)
		{	//������50���հ���,���㽫У�����̶�����Ļ����
			for(int i=0;i<TAIL_FIX_ROW_COUNT;i++)
			{
				InsertOrUpdatePartTreeItem(m_listDataCtrl,NULL);
				InsertOrUpdatePartTreeItem(m_listDataCtrl1,NULL);
			}
		}
		CScrollBar *pVertBar=m_listDataCtrl1.GetScrollBarCtrl(SB_VERT);
		CScrollBar *pHorzBar=m_listDataCtrl1.GetScrollBarCtrl(SB_HORZ);
		if(pVertBar)
			pVertBar->ShowWindow(SW_HIDE);
		if(pHorzBar)
			pHorzBar->ShowWindow(SW_HIDE);
	}
	//����ѡ����
	if(m_listDataCtrl.GetItemCount()>0)
	{
		CSuperGridCtrl::CTreeItem *pSelItem=NULL,*pFirItem=NULL;
		POSITION pos=m_listDataCtrl.GetRootHeadPosition();
		while(pos!=NULL)
		{
			CSuperGridCtrl::CTreeItem* pItem=m_listDataCtrl.GetNextRoot(pos);
			if(pFirItem==NULL)
				pFirItem=pItem;
			if(idSelProp>0 && pItem->m_idProp==idSelProp)
			{
				pSelItem=pItem;
				break;
			}
		}
		if(pSelItem==NULL)
			pSelItem=pFirItem;
		m_listDataCtrl.SelectItem(pSelItem,0,FALSE,TRUE);
		if(m_cDisType==CDataCmpModel::IMAGE_REGION_DATA && m_pData&&m_biLayoutType==3)
		{
			CSuperGridCtrl::CTreeItem *pSelItem1=m_listDataCtrl1.GetTreeItem(pSelItem->GetIndex());
			//m_listDataCtrl1.SelectItem(pSelItem1,0,FALSE,TRUE);
		}
		m_listDataCtrl.SetFocus();
	}
	UpdateData(FALSE);
}
void CDisplayDataPage::InitPage(BYTE cType,DWORD dwRefData)
{
	m_cDisType=cType;
	m_pData=dwRefData;
	if(cType==CDataCmpModel::DWG_IMAGE_DATA)
		m_biLayoutType=1;
	else if(cType==CDataCmpModel::IMAGE_REGION_DATA)
		m_biLayoutType=2;
	else
		m_biLayoutType=0;
	RefreshListCtrl();
	RelayoutWnd();
	Invalidate(TRUE);
}
//����ͼƬ
int CDisplayDataPage::StretchBlt(HDC hDC,CImageFileHost* pImageFile)
{
	CDC *pDC=CDC::FromHandle(hDC);
	if(pDC==NULL||pImageFile==NULL)
		return 0;
	int width=pImageFile->Width;
	int height=pImageFile->Height;
	int extrabytes=(4-(width * 3)%4)%4;
	int bytesize=(width*3+extrabytes)*height;
	IMAGE_DATA xImagedata;
	pImageFile->Get24BitsImageData(&xImagedata);
	CHAR_ARRAY imagedata(xImagedata.nEffWidth*xImagedata.nHeight);
	xImagedata.imagedata = imagedata;
	pImageFile->Get24BitsImageData(&xImagedata);
	CRect imageShowRect=pImageFile->MappingToScrRect();
	//λͼͷ��Ϣ
	BITMAPINFOHEADER bmpInfoHeader;
	bmpInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpInfoHeader.biWidth = width;
	bmpInfoHeader.biHeight = height;
	bmpInfoHeader.biPlanes = 1;
	bmpInfoHeader.biBitCount = 24;
	bmpInfoHeader.biCompression = BI_RGB;
	bmpInfoHeader.biSizeImage = bytesize;
	bmpInfoHeader.biXPelsPerMeter = 2952;
	bmpInfoHeader.biYPelsPerMeter = 2952;
	bmpInfoHeader.biClrUsed = 0;
	bmpInfoHeader.biClrImportant = 0;
	//IMAGE_FILE_DATA�е�ͼ����ɨ��ģʽΪ��������downward��ӦתΪStretchDIBits�����upwardģʽ
	CHAR_ARRAY imagebits(imagedata.Size());
	for(int i=0;i<height;i++)
	{
		int rowi=height-1-i;
		memcpy(&imagebits[rowi*xImagedata.nEffWidth],&imagedata[i*xImagedata.nEffWidth],xImagedata.nEffWidth);
	}
	int oldmode=SetStretchBltMode(hDC,HALFTONE);
	::StretchDIBits( hDC,imageShowRect.left,imageShowRect.top,imageShowRect.Width(),imageShowRect.Height(),
		0,0,pImageFile->GetWidth(),pImageFile->GetHeight(),imagebits,(LPBITMAPINFO)&bmpInfoHeader,DIB_RGB_COLORS,SRCCOPY);
	SetStretchBltMode(hDC,oldmode);
	return 1;
}
int CDisplayDataPage::StretchBlt(HDC hDC,CImageRegionHost *pRegionHost)
{
	CDC *pDC=CDC::FromHandle(hDC);
	if(pDC==NULL||pRegionHost==NULL)
		return 0;

	CRect imageShowRect=pRegionHost->MappingToScrRect();
	CBitmap bmp;
	CDC dcMemory;
	IMAGE_DATA xImagedata;
	pRegionHost->GetMonoImageData(&xImagedata);
	CHAR_ARRAY imagedata(xImagedata.nEffWidth*xImagedata.nHeight);
	xImagedata.imagedata = imagedata;
	pRegionHost->GetMonoImageData(&xImagedata);
	bmp.CreateBitmap(pRegionHost->Width,pRegionHost->Height,1,1,xImagedata.imagedata);
	dcMemory.CreateCompatibleDC(pDC);
	dcMemory.SelectObject(&bmp);
	//
	pDC->StretchBlt(imageShowRect.left,imageShowRect.top,imageShowRect.Width(),imageShowRect.Height(),
		&dcMemory,0,0,pRegionHost->GetWidth(),pRegionHost->GetHeight(),SRCCOPY);
	dcMemory.DeleteDC();
	return 1;
}
CWnd* CDisplayDataPage::GetWorkPanelRect(CRect& rect)
{
	CWnd *pWnd=GetDlgItem(IDC_WORK_PANEL);
	if(pWnd)
	{
		pWnd->GetWindowRect(&rect);
		ScreenToClient(&rect);
	}
	return pWnd;
}
static int f2i(double fVal)
{
	if(fVal>0)
		return (int)(fVal+0.5);
	else
		return (int)(fVal-0.5);
}
bool ClipLine(RECT& rect,POINT *pS,POINT *pE)
{
	bool bInvalidLine=false;
	if(pS->x<rect.left&&pE->x<rect.left)
		bInvalidLine=true;
	else if(pS->y<rect.top&&pE->y<rect.top)
		bInvalidLine=true;
	else if(pS->x>rect.right&&pE->x>rect.right)
		bInvalidLine=true;
	else if(pS->y>rect.bottom&&pE->y>rect.bottom)
		bInvalidLine=true;
	if(bInvalidLine)
		return false;

	//ת������������ʾ�߽�ܶ�Ķ�ά��Ϊ��Ч��(����WINDOWS��ʾ������)
	if(pE->y-pS->y==0)	//ˮƽ��
	{
		pE->x=min(pE->x,rect.right);
		pS->x=min(pS->x,rect.right);
		pE->x=max(pE->x,rect.left);
		pS->x=max(pS->x,rect.left);
		return true;
	}
	else if(pE->x-pS->x==0)	//��ֱ��
	{
		pE->y=min(pE->y,rect.bottom);
		pS->y=min(pS->y,rect.bottom);
		pE->y=max(pE->y,rect.top);
		pS->y=max(pS->y,rect.top);
		return true;
	}
	double tana_y2x=(pE->y-pS->y)/(double)(pE->x-pS->x);
	double tana_x2y=(pE->x-pS->x)/(double)(pE->y-pS->y);
	if(pS->x<rect.left)
	{
		pS->y -= f2i((pS->x-rect.left)*tana_y2x);
		pS->x  = rect.left;
	}
	else if(pS->x>rect.right)
	{
		pS->y -= f2i((pS->x-rect.right)*tana_y2x);
		pS->x  = rect.right;
	}
	if(pS->y<rect.top)
	{
		pS->x-= f2i((pS->y-rect.top)*tana_x2y);
		pS->y = rect.top;
	}
	else if(pS->y>rect.bottom)
	{
		pS->x -= f2i((pS->y-rect.bottom)*tana_x2y);
		pS->y  = rect.bottom;
	}
	if(pE->x<rect.left)
	{
		pE->y -= f2i((pE->x-rect.left)*tana_y2x);
		pE->x  = rect.left;
	}
	else if(pE->x>rect.right)
	{
		pE->y -= f2i((pE->x-rect.right)*tana_y2x);
		pE->x  = rect.right;
	}
	if(pE->y<rect.top)
	{
		pE->x-= f2i((pE->y-rect.top)*tana_x2y);
		pE->y = rect.top;
	}
	else if(pE->y>rect.bottom)
	{
		pE->x -= f2i((pE->y-rect.bottom)*tana_x2y);
		pE->y = rect.bottom;
	}
	return true;
}
void CDisplayDataPage::DrawClipLine(CDC* pDC,CPoint from,CPoint to,RECT* pClipRect/*=NULL*/)
{
	if(pClipRect&&!ClipLine(*pClipRect,&from,&to))
		return;
	pDC->MoveTo(from);
	pDC->LineTo(to);
}
void CDisplayDataPage::InvertLine(CDC* pDC,CPoint ptFrom,CPoint ptTo,RECT* pClipRect/*=NULL*/)
{
	CPen pen(PS_DASH,2,RGB(150,150,150));
	CPen *pOldPen=pDC->SelectObject(&pen);
	int nOldMode=pDC->SetROP2(R2_NOT);
	if(pClipRect)
		ClipLine(*pClipRect,&ptFrom,&ptTo);
	pDC->MoveTo(ptFrom);
	pDC->LineTo(ptTo);
	pDC->SelectObject(pOldPen);
	pDC->SetROP2(nOldMode);
}
void CDisplayDataPage::InvertEllipse(CDC* pDC,CPoint center)
{
	CPen pen(PS_SOLID,2,RGB(0,255,0));
	CPen *pOldPen=pDC->SelectObject(&pen);
	int nOldMode=pDC->SetROP2(R2_NOT);
	pDC->Ellipse(CRect(center.x-5,center.y-5,center.x+5,center.y+5));
	pDC->SelectObject(pOldPen);
	pDC->SetROP2(nOldMode);
}
void CDisplayDataPage::RelayoutWnd()
{
	CWnd* pWorkPanel=NULL,*pSubWorkPanel=NULL,*pHorzSplitter=NULL;
	pWorkPanel=GetDlgItem(IDC_WORK_PANEL);
	pSubWorkPanel=GetDlgItem(IDC_SUB_WORK_PANEL);
	pHorzSplitter=GetDlgItem(IDC_SPLITTER_HORIZ);
	if(pWorkPanel==NULL || pSubWorkPanel==NULL)
		return;
	typedef CWnd* CWndPtr;
	CWndPtr sliderCoefWndArr[3]={NULL,NULL,NULL};
	CRect rect;
	long ctrIDArr[9]={IDC_S_CONTEXT,IDC_E_CONTEXT,IDC_S_RECORD_NUM,IDC_E_RECORD_NUM,
					  IDC_S_COEF_TITLE,IDC_SLIDER_BALANCE_COEF,IDC_S_BALANCE_COEF,
					  IDC_S_DISPLAY_MODEL,IDC_CMB_MODEL};
	int nLeft=0,nSplitY=0;
	for(int i=0;i<9;i++)
	{
		CWnd *pWnd=GetDlgItem(ctrIDArr[i]);
		if(pWnd->GetSafeHwnd())
		{
			if(m_cDisType==CDataCmpModel::DWG_IMAGE_DATA)
			{
				if(i==4||i==5||i==6)
					pWnd->ShowWindow(SW_SHOW);
				else if(i==7||i==8)
				{
					pWnd->ShowWindow(SW_HIDE);
					continue;
				}
			}
			else if(m_cDisType==CDataCmpModel::IMAGE_REGION_DATA)
			{
				if(i==4||i==5||i==6)
				{
					pWnd->ShowWindow(SW_HIDE);
					continue;
				}
				else if(i==7||i==8)
					pWnd->ShowWindow(SW_SHOW);
			}
			pWnd->GetWindowRect(&rect);
			ScreenToClient(&rect);
			int nWidth=rect.right-rect.left;
			int nHight=rect.bottom-rect.top;
			if(i==0 || i==2 || i==4 || i==6 || i==7)
				rect.top=4;
			else
				rect.top=1;
			rect.bottom=rect.top+nHight;
			rect.left=nLeft;
			rect.right=nLeft+nWidth;
			pWnd->MoveWindow(&rect);
			if(i==1 || i==3||i==5 || i==8)
				nLeft+=nWidth+10;
			else
				nLeft+=nWidth+1;
			if(rect.bottom>nSplitY)
				nSplitY=rect.bottom;
			if(i>=4&&i<7)
				sliderCoefWndArr[i-4]=pWnd;
		}
	}
	//
	if(m_cDisType==CDataCmpModel::DWG_IMAGE_DATA)
	{
		CImageFileHost* pImage=(CImageFileHost*)m_pData;
		double balancecoef=pImage->GetMonoThresholdBalanceCoef();
		int scalerPos=50+f2i(balancecoef*100);
		if(scalerPos<0)
			scalerPos=0;
		else if(scalerPos>100)
			scalerPos=100;
		this->m_sliderBalanceCoef.SetPos(scalerPos);
	}
	int nListCtrlWidth=30;
	for(int i=0;i<m_listDataCtrl.GetColumnCount();i++)
	{
		m_listDataCtrl.SetColumnWidth(i,m_arrColWidth[i]);
		nListCtrlWidth+=m_listDataCtrl.GetColumnWidth(i);
	}
	int SPLITTER_HEIGHT=5;
	if(pHorzSplitter&&m_biLayoutType!=1)
		pHorzSplitter->ShowWindow(SW_HIDE);
	if(m_biLayoutType==0)
	{	//0.������Ϣ
		rect=m_rcClient;
		rect.top=nSplitY+1;	
		m_listDataCtrl.MoveWindow(rect);
		pWorkPanel->ShowWindow(SW_HIDE);
		pSubWorkPanel->ShowWindow(SW_HIDE);
		m_xInputPartInfoPage.ShowWindow(SW_HIDE);
		for(int i=0;i<3;i++)
		{
			if(sliderCoefWndArr[i])
				sliderCoefWndArr[i]->ShowWindow(SW_HIDE);
		}
		m_listDataCtrl1.ShowWindow(SW_HIDE);
	}
	else if(m_biLayoutType==1)
	{	//1.ͼƬ��Ϣ
		m_nOldHorzY=(int)(m_fDividerYScale*(m_rcClient.bottom-m_rcClient.top));
		rect=m_rcClient;
		rect.top=nSplitY+1;
		rect.right-=(nListCtrlWidth+2);
		pWorkPanel->MoveWindow(rect);
		for(int i=0;i<3;i++)
		{
			if(sliderCoefWndArr[i])
				sliderCoefWndArr[i]->ShowWindow(SW_SHOW);
		}
		/*if(pHorzSplitter)
		{
			rect=m_rcClient;
			rect.top=nSplitY+1;
			rect.bottom=nSplitY+SPLITTER_HEIGHT;
			pHorzSplitter->MoveWindow(&rect);
			nSplitY+=SPLITTER_HEIGHT;
			pHorzSplitter->ShowWindow(SW_SHOW);
		}*/

		rect=m_rcClient;
		rect.top=nSplitY+1;
		rect.left=rect.right-nListCtrlWidth;
		m_listDataCtrl.MoveWindow(rect);
		//
		pWorkPanel->ShowWindow(SW_NORMAL);
		pSubWorkPanel->ShowWindow(SW_HIDE);
		m_xInputPartInfoPage.ShowWindow(SW_HIDE);
		m_listDataCtrl1.ShowWindow(SW_HIDE);
	}
	else if(m_biLayoutType==2)
	{	//����������Ϣ
		CImageRegionHost* pRegion=(CImageRegionHost*)m_pData;
		if(pRegion)
		{
			CRect headRect;
			CHeaderCtrl *pHeadCtrl=m_listDataCtrl.GetHeaderCtrl();
			pHeadCtrl->GetWindowRect(headRect);
			SIZE size;
			size.cx=0;
			size.cy=headRect.Height();
			pRegion->SetScrOffset(size);
		}
		for(int i=0;i<m_listDataCtrl.GetItemCount();i++)
		{
			for(int j=0;j<m_listDataCtrl.GetColumnCount();j++)
			{
				if((j==CDisplayDataPage::COL_LABEL||
					j==CDisplayDataPage::COL_SPEC||
					j==CDisplayDataPage::COL_LEN||
					j==CDisplayDataPage::COL_COUNT))
					m_listDataCtrl.SetCellReadOnly(i,j,FALSE);
				else
					m_listDataCtrl.SetCellReadOnly(i,j,TRUE);
			}
		}
		m_nOldHorzY=(int)(m_fDividerYScale*(m_rcClient.bottom-m_rcClient.top));
		rect=m_rcClient;
		rect.top=nSplitY+1;
		rect.right-=(nListCtrlWidth+2);
		pWorkPanel->MoveWindow(rect);
		rect=m_rcClient;
		rect.top=nSplitY+1;
		rect.left=rect.right-nListCtrlWidth;
		/*rect.bottom=rect.top+m_nInputDlgH;
		pSubWorkPanel->MoveWindow(rect);
		rect=m_rcClient;
		rect.top=m_nOldHorzY+4+m_nInputDlgH;*/
		m_listDataCtrl.MoveWindow(rect);
		//
		pWorkPanel->ShowWindow(SW_NORMAL);
		pSubWorkPanel->ShowWindow(SW_HIDE);
		m_xInputPartInfoPage.ShowWindow(SW_NORMAL);
		m_listDataCtrl1.ShowWindow(SW_HIDE);
	}
	else if(m_biLayoutType==3)
	{	//����У��
		for(int i=0;i<m_listDataCtrl.GetItemCount();i++)
		{
			for(int j=0;j<m_listDataCtrl.GetColumnCount();j++)
			{
				if( j==m_biCurCol&&
					(m_biCurCol==CDisplayDataPage::COL_LABEL||
					m_biCurCol==CDisplayDataPage::COL_SPEC||
					m_biCurCol==CDisplayDataPage::COL_LEN||
					m_biCurCol==CDisplayDataPage::COL_COUNT))
					m_listDataCtrl.SetCellReadOnly(i,j,FALSE);
				else
					m_listDataCtrl.SetCellReadOnly(i,j,TRUE);
			}
		}
		//
		pWorkPanel->ShowWindow(SW_SHOW);
		pSubWorkPanel->ShowWindow(SW_HIDE);
		pHorzSplitter->ShowWindow(SW_HIDE);
		int nCurColWidth=60;
		CImageRegionHost* pRegion=(CImageRegionHost*)m_pData;
		if(pRegion&&m_biCurCol>=0&&m_biCurCol<=4)
		{
			int nCol=pRegion->GetImageColumnWidth();
			DYN_ARRAY<int> widthArr(nCol);
			if(nCol>0)
			{
				pRegion->GetImageColumnWidth(widthArr);
				nCurColWidth=widthArr[m_biCurCol];
			}
			else
			{
				widthArr.Resize(7);
				widthArr[0]=60;
				widthArr[1]=150;
				widthArr[2]=90;
				widthArr[3]=60;
				widthArr[4]=90;
				widthArr[5]=90;
				widthArr[6]=180;
			}
			CRect headerRect;
			CHeaderCtrl *pHeader=m_listDataCtrl.GetHeaderCtrl();
			pHeader->GetClientRect(headerRect);
			SIZE offset;
			offset.cx=0;
			nCurColWidth=widthArr[0];
			if(m_biCurCol==1||m_biCurCol==2)
			{
				offset.cx=-widthArr[0];
				nCurColWidth=widthArr[1];
			}
			else if(m_biCurCol==3)
			{
				offset.cx=-(widthArr[0]+widthArr[1]);
				nCurColWidth=widthArr[2];
			}
			else if(m_biCurCol==4)
			{
				offset.cx=-(widthArr[0]+widthArr[1]+widthArr[2]);
				nCurColWidth=widthArr[3];
			}
			offset.cy=headerRect.Height();
			pRegion->SetScrOffset(offset);
		}
		rect=m_rcClient;
		rect.top=nSplitY+1;	
		if(m_biCurCol==0)
		{
			rect.right=rect.left+nCurColWidth;
			pWorkPanel->MoveWindow(rect);
			rect=m_rcClient;
			rect.top=nSplitY+1;	
			rect.left+=nCurColWidth;
			for(int i=0;i<m_listDataCtrl.GetItemCount();i++)
				m_listDataCtrl.SetColumnWidth(i,m_arrColWidth[i]);
			m_listDataCtrl.MoveWindow(rect);
			m_listDataCtrl1.ShowWindow(SW_HIDE);
		}
		else
		{
			rect=m_rcClient;
			rect.top=nSplitY+1;	
			rect.right=rect.left;
			for(int i=0;i<m_listDataCtrl.GetItemCount();i++)
			{
				m_listDataCtrl.SetColumnWidth(i,i>=m_biCurCol?m_arrColWidth[i]:0);
				m_listDataCtrl1.SetColumnWidth(i,i<m_biCurCol?m_arrColWidth[i]:0);
				if(i<m_biCurCol)
					rect.right+=m_arrColWidth[i];
			}
			m_listDataCtrl1.ShowWindow(SW_SHOW);
			m_listDataCtrl1.MoveWindow(rect);
			rect.left=rect.right+1;
			rect.right=rect.left+nCurColWidth;
			pWorkPanel->MoveWindow(rect);
			rect.left=rect.right+1;
			rect.right=m_rcClient.right;
			m_listDataCtrl.MoveWindow(rect);
			m_listDataCtrl.SetFocus();
		}
	}
	else if(m_biLayoutType==4)
	{	//����У��
		pWorkPanel->MoveWindow(rect);
		m_listDataCtrl1.ShowWindow(SW_HIDE);
	}

	if(m_biLayoutType==2||m_biLayoutType==3)
	{
		CImageRegionHost* pRegion=(CImageRegionHost*)m_pData;
		int iCurSel=m_listDataCtrl.GetSelectedItem();
		CSuperGridCtrl::CTreeItem *pItem=m_listDataCtrl.GetTreeItem(iCurSel<=0?0:iCurSel);
		if(pRegion&&pItem&&pItem->m_idProp>0)
		{
			CRect rc;
			IRecoginizer::BOMPART* pBomPart=(IRecoginizer::BOMPART*)pItem->m_idProp;
			if(pBomPart&&!pRegion->GetBomPartRect(pBomPart->id,&rc))
			{
				rc=pBomPart->rc;
				Focus(rc,&m_listDataCtrl);
			}
		}
	}
}
void CDisplayDataPage::ShiftCheckColumn(int iCol)
{
	m_biCurCol=iCol;
	RelayoutWnd();
}
POINT CDisplayDataPage::TransPoint(CPoint pt,BOOL bTransToCanvasPanel,BOOL bPanelDC/*=FALSE*/)
{
	if(m_pData==NULL)
		return pt;
	CSize  offset;
	double fRatio=0;
	if(m_cDisType==CDataCmpModel::DWG_IMAGE_DATA)
	{
		CImageFileHost* pImage=(CImageFileHost*)m_pData;
		offset=pImage->GetScrOffset();
		fRatio=pImage->ZoomRatio();
	}
	else if(m_cDisType==CDataCmpModel::IMAGE_REGION_DATA)
	{
		CImageRegionHost* pRegion=(CImageRegionHost*)m_pData;
		offset=pRegion->GetScrOffset();
		fRatio=pRegion->ZoomRatio();
	}
	//�����׼��s
	CPoint orgion;
	if(bPanelDC)
		orgion.SetPoint(offset.cx,offset.cy);
	else
	{
		CRect rect;
		GetWorkPanelRect(rect);
		orgion.SetPoint(rect.left+offset.cx,rect.top+offset.cy);
	}
	//����ת��ƫ��
	CPoint point=pt;
	double fScale=1;
	if(bTransToCanvasPanel)
	{//��������ڵ�ת��Ϊ������
		fScale=1.0/fRatio;
		point-=orgion;
		point.x=(int)(point.x*fScale);
		point.y=(int)(point.y*fScale);
	}
	else
	{//��������ת��Ϊ��ʾ��
		fScale=fRatio;
		point.x=(int)(point.x*fScale);
		point.y=(int)(point.y*fScale);
		point+=orgion;
	}
	return point;
}
POINT CDisplayDataPage::IntelliAnchorCrossHairPoint(POINT point,BYTE ciLT0RT1RB2LB3)
{
	if(m_pData==NULL)
		return point;
	IMonoImage* pSquareImage=IMonoImage::PresetObject();
	int SNAP_DIST=10;
	if(pSquareImage->GetImageWidth()==0)
		pSquareImage->InitImageSize(SNAP_DIST+1,SNAP_DIST+1);
	CImageFileHost* pImage=(m_cDisType==CDataCmpModel::DWG_IMAGE_DATA)?(CImageFileHost*)m_pData:NULL;
	if(pImage==NULL)
		return point;
	int i,j;
	for(i=-SNAP_DIST;i<=SNAP_DIST;i++)
	{
		for(j=-SNAP_DIST;j<=SNAP_DIST;j++)
		{
			bool black=pImage->IsBlackPixel(point.x+i,point.y+j);
			pSquareImage->SetPixelState(i+SNAP_DIST,j+SNAP_DIST,black);
		}
	}
	//pSquareImage->RemoveNosiePixels();
	if(ciLT0RT1RB2LB3==0)
	{	//���ο����Ͻ�
		/*int SNAP_DIST2=SNAP_DIST+SNAP_DIST;
		for(i=0;i<=SNAP_DIST2;i++)
		{
			if(pSquareImage->IsBlackPixel(i,i))
				break;
		}
		bool modified=false;
		int xi=i-1,yj=i-1;
		do
		{
			modified=false;
			if(!pSquareImage->IsBlackPixel(xi+1,yj))
			{
				xi++;
				modified=true;
			}
			if(!pSquareImage->IsBlackPixel(xi,yj+1))
			{
				yj++;
				modified=true;
			}
			if(!pSquareImage->IsBlackPixel(xi+2,yj-1))
			{
				xi+=2;
				yj--;
				modified=true;
			}
			if(!pSquareImage->IsBlackPixel(xi-1,yj+2))
			{
				xi--;
				yj+=2;
				modified=true;
			}
		}while(modified&&xi<=SNAP_DIST2&&yj<=SNAP_DIST2);
		int xiPin=point.x-SNAP_DIST+xi,yiPin=point.y-SNAP_DIST+i;
		return CPoint(xiPin,yiPin);*/
	}
	else if(ciLT0RT1RB2LB3==1)
	{	//���ο����Ͻ�
	}
	else if(ciLT0RT1RB2LB3==2)
	{	//���ο����½�
	}
	else if(ciLT0RT1RB2LB3==3)
	{	//���ο����½�
	}
	return point;
}
void CDisplayDataPage::SetCurTask(int iCurTask)
{
	m_cCurTask=iCurTask;
	if(m_cCurTask==TASK_SNAP)	//��ղ�׽��Ϣ
		m_xPolyline.Init();
}
void CDisplayDataPage::OperOther()
{
	SetCurTask(TASK_OTHER);
	m_bStartDrag=FALSE;
	m_xPolyline.m_iActivePt=-1;
	Invalidate(FALSE);
}
//////////////////////////////////////////////////////////////////////////
//��Ϣ����
void CDisplayDataPage::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	m_rcClient.right=cx;
	m_rcClient.bottom=cy;
	RelayoutWnd();
}
void CDisplayDataPage::OnOK()
{

}
void CDisplayDataPage::OnCancel()
{

}
//
void CDisplayDataPage::OnPaint()
{
	CPaintDC dc(this);	//�豸������
	if(m_cDisType!=CDataCmpModel::DWG_IMAGE_DATA&&m_cDisType!=CDataCmpModel::IMAGE_REGION_DATA)
		return;
	if(m_cDisType==CDataCmpModel::IMAGE_REGION_DATA)
	{
		//���Ʒָ���
		/*CPen psPen(PS_SOLID,1,RGB(120,120,120));
		CPen* pOldPen = dc.SelectObject(&psPen);
		dc.MoveTo(m_rcClient.left,m_nOldHorzY+1);
		dc.LineTo(m_rcClient.right,m_nOldHorzY+1);
		dc.MoveTo(m_rcClient.left,m_nOldHorzY+m_nInputDlgH);
		dc.LineTo(m_rcClient.right,m_nOldHorzY+m_nInputDlgH);
		dc.SelectObject(pOldPen);
		psPen.DeleteObject();*/
		//
		int iCurSel=m_listDataCtrl.GetSelectedItem();
		CSuperGridCtrl::CTreeItem *pItem=m_listDataCtrl.GetTreeItem(iCurSel);
		if(pItem)
			m_xInputPartInfoPage.m_pBomPart=(IRecoginizer::BOMPART*)pItem->m_idProp;
		m_xInputPartInfoPage.RefreshDlg();
	}
	//
	CWnd *pWorkPanel=GetDlgItem(IDC_WORK_PANEL);
	if(pWorkPanel==NULL)
		return;
	CRect panelRect;
	pWorkPanel->GetClientRect(&panelRect);
	if(m_cDisType==CDataCmpModel::IMAGE_REGION_DATA)
	{	//��ʾͼƬ����ʱ������ʾ���
		int nMaxWidth=640;
		if(panelRect.Width()>nMaxWidth)
			panelRect.right=panelRect.left+nMaxWidth;
	}
	CDC*  pDC=pWorkPanel->GetDC();	// ��ͼ�ؼ����豸������
	int nWidth=panelRect.Width();
	int nHeight=panelRect.Height();
	CBitmap memoryBP;
	CDC     memoryDC;
	memoryBP.CreateCompatibleBitmap(pDC,panelRect.Width(),panelRect.Height());
	memoryDC.CreateCompatibleDC(NULL);
	memoryDC.SelectObject(&memoryBP);
	memoryDC.FillSolidRect(0,0,nWidth,nHeight,RGB(196,196,196));//ͨ��ָ���������λͼ
	if(m_cDisType==CDataCmpModel::DWG_IMAGE_DATA)
	{	//����ͼƬ����
		CImageFileHost* pImage=(CImageFileHost*)m_pData;
		pImage->InitImageShowPara(panelRect);
		StretchBlt(memoryDC.GetSafeHdc(),pImage);	//���ڴ��豸�ϻ���ѡ��ͼƬλͼ
		pDC->BitBlt(0,0,nWidth,nHeight,&memoryDC,0,0,SRCCOPY);//���ڴ���λͼ���Ƶ������豸��
		//�������ڽ���ѡ��ľ��ο�
		CPen bluePen(PS_SOLID,2,RGB(0,0,255)),redPen(PS_SOLID,2,RGB(255,0,0)),greenPen(PS_SOLID,2,RGB(0,255,0));
		CPen *pOldPen=pDC->SelectObject(&bluePen);
		if(m_xPolyline.m_cState==POLYLINE_4PT::STATE_VALID)
		{	//����ʰȡ����������
			CPoint ptArr[5];
			for(int i=0;i<5;i++)
			{
				CPoint pt=m_xPolyline.m_ptArr[i%4];
				ptArr[i]=TransPoint(pt,FALSE,TRUE);
				if(i==m_xPolyline.m_iActivePt)
					pDC->SelectObject(&greenPen);
				else
					pDC->SelectObject(&bluePen);
				CRect marginRect(panelRect.left+5,panelRect.top+5,panelRect.right-5,panelRect.bottom-5);
				if(i!=4 && marginRect.PtInRect(ptArr[i]))	//marginRect����panelRect��Ϊ�˱����ʶԲ������ͼ����
					pDC->Ellipse(CRect(ptArr[i].x-5,ptArr[i].y-5,ptArr[i].x+5,ptArr[i].y+5));
				if(i==0)
					continue;
				POINT xS=ptArr[i-1],xE=ptArr[i];
				if(ClipLine(panelRect,&xS,&xE))
				{
					pDC->SelectObject(&bluePen);
					pDC->MoveTo(xS);
					pDC->LineTo(xE);
				}
			}
		}
		// ����������ȷ������������
		pDC->SelectObject(&redPen);
		for(IImageRegion *pRegion=pImage->EnumFirstRegion();pRegion;pRegion=pImage->EnumNextRegion())
		{
			POINT ptArr[5]={pRegion->TopLeft(),pRegion->TopRight(),pRegion->BtmRight(),pRegion->BtmLeft(),pRegion->TopLeft()};
			for(int i=0;i<5;i++)
			{
				ptArr[i].y=ptArr[i].y;
				ptArr[i]=TransPoint(ptArr[i],FALSE,TRUE);
				if(i==0)
					continue;
				POINT xS=ptArr[i-1],xE=ptArr[i];
				if(ClipLine(panelRect,&xS,&xE))
				{
					pDC->SelectObject(&bluePen);
					pDC->MoveTo(xS);
					pDC->LineTo(xE);
				}
			}
		}
		pDC->SelectObject(pOldPen);
	}
	else if(m_cDisType==CDataCmpModel::IMAGE_REGION_DATA)
	{	//������������
		CImageRegionHost* pRegion=(CImageRegionHost*)m_pData;
		pRegion->InitImageShowPara(panelRect);
		StretchBlt(memoryDC.GetSafeHdc(),pRegion);
		pDC->BitBlt(0,0,nWidth,nHeight,&memoryDC,0,0,SRCCOPY);
		//����ѡ����������
		int iCurSel=m_listDataCtrl.GetSelectedItem();
		CSuperGridCtrl::CTreeItem *pItem=NULL;
		if(iCurSel>=0)
			pItem=m_listDataCtrl.GetTreeItem(iCurSel);
		if(pItem)
		{
			CRect rc;
			IRecoginizer::BOMPART* pBomPart=(IRecoginizer::BOMPART*)pItem->m_idProp;
			if(pBomPart&&!pRegion->GetBomPartRect(pBomPart->id,&rc))
				rc=pBomPart->rc;
			if(rc.Width()>0&&rc.Height()>0)
			{
				CPoint topLeft =TransPoint(rc.TopLeft(),FALSE,TRUE);
				CPoint btmRight=TransPoint(rc.BottomRight(),FALSE,TRUE);
				CPen redPen(PS_SOLID,2,RGB(255,0,0));
				CPen* pOldPen=(CPen*)pDC->SelectObject(&redPen);
				DrawClipLine(pDC,topLeft,CPoint(btmRight.x,topLeft.y),&panelRect);
				DrawClipLine(pDC,CPoint(btmRight.x,topLeft.y),btmRight,&panelRect);
				DrawClipLine(pDC,btmRight,CPoint(topLeft.x,btmRight.y),&panelRect);
				DrawClipLine(pDC,CPoint(topLeft.x,btmRight.y),topLeft,&panelRect);
				pDC->SelectObject(pOldPen);
			}
		}
	}
	// �ͷŶ����DC
	memoryBP.DeleteObject();
	memoryDC.DeleteDC();
	ReleaseDC(pDC);
}
void CDisplayDataPage::OnLButtonDown(UINT nFlags, CPoint point)
{
	CRect rect;
	GetWorkPanelRect(rect);
	if(GetCurTask()==TASK_OTHER)
	{	//
		if(rect.PtInRect(point))
			m_cCurTask=TASK_SNAP;
		if((point.y>=m_nOldHorzY-5) && (point.y<=m_nOldHorzY+5))
		{
			::SetCursor(m_hHorzSize);
			m_nOldHorzY = point.y;
			CClientDC dc(this);
			InvertLine(&dc,CPoint(m_rcClient.left,m_nOldHorzY),CPoint(m_rcClient.right,m_nOldHorzY));
			ReleaseDC(&dc);
			SetCapture();
			m_bHorzTracing=TRUE;
		}
	}
	if(GetCurTask()==TASK_SNAP)
	{	//ͼƬ��׽����
		if(!rect.PtInRect(point) || m_cDisType!=CDataCmpModel::DWG_IMAGE_DATA)
		{
			OperOther();
			return;
		}
		SetCapture();
		//��ʼ������������ѡ��
		if(m_xPolyline.m_cState==POLYLINE_4PT::STATE_NONE)
		{
			::SetCursor(m_hCross);
			m_ptLBDownPos=point;
			m_xPolyline.m_ptArr[0]=TransPoint(point,TRUE);
			m_xPolyline.m_cState=POLYLINE_4PT::STATE_WAITNEXTPT;
			Invalidate(FALSE);
		}
		else if(m_xPolyline.m_cState==POLYLINE_4PT::STATE_VALID)
		{	
			m_xPolyline.m_iActivePt=-1;
			for(int i=0;i<4;i++)
			{
				CPoint pt=TransPoint(m_xPolyline.m_ptArr[i],FALSE);
				CRect rect(pt.x-6,pt.y-6,pt.x+6,pt.y+6);
				if(rect.PtInRect(point))
				{
					m_xPolyline.m_iActivePt=i;
					break;
				}
			}
			if(m_xPolyline.m_iActivePt>=0&&m_xPolyline.m_iActivePt<=4)
				m_ptLBDownPos=point;
			Invalidate(FALSE);
		}
	}
	if(GetCurTask()==TASK_MOVE || nFlags&MK_CONTROL)
	{	//ͼƬ�ƶ�����(�����CTRL+������)
		if(!rect.PtInRect(point))
		{
			OperOther();
			return;
		}
		SetCapture();
		if(m_cDisType==CDataCmpModel::DWG_IMAGE_DATA)
		{
			CImageFileHost* pImage=(CImageFileHost*)m_pData;
			::SetCursor(m_hMove);
			m_ptLBDownPos=point;
			m_bStartDrag=TRUE;
		}
		else if(m_cDisType==CDataCmpModel::IMAGE_REGION_DATA)
		{
			CImageRegionHost *pRegion=(CImageRegionHost*)m_pData;
			::SetCursor(m_hMove);
			m_ptLBDownPos=point;
			m_bStartDrag=TRUE;
		}
	}
	CDialog::OnLButtonDown(nFlags, point);
}
void CDisplayDataPage::OnLButtonUp(UINT nFlags, CPoint point)
{
	CRect rect;
	GetWorkPanelRect(rect);
	if(GetCurTask()==TASK_OTHER &&m_bHorzTracing)
	{
		if(GetCapture()==this)
			ReleaseCapture();
		::ClipCursor(NULL);
		//
		CClientDC dc(this);
		if(point.y<m_rcClient.top+50)
			point.y=m_rcClient.top+50;
		if(point.y>m_rcClient.bottom-50)
			point.y=m_rcClient.bottom-50;
		if(m_bHorzTracing)
		{
			InvertLine(&dc,CPoint(m_rcClient.left,point.y),CPoint(m_rcClient.right,point.y));
			m_nOldHorzY = point.y;
			m_fDividerYScale=((double)m_nOldHorzY)/(m_rcClient.bottom-m_rcClient.top);
		}
		m_bHorzTracing=FALSE;
		ReleaseDC(&dc);
		RelayoutWnd();
		Invalidate();
	}
	if(GetCurTask()==TASK_SNAP)
	{	//ͼƬ�������
		if(GetCapture()==this)
			ReleaseCapture();
		::ClipCursor(NULL);
		if(!rect.PtInRect(point)||m_cDisType!=CDataCmpModel::DWG_IMAGE_DATA)
		{
			OperOther();
			return;
		}
		if(m_xPolyline.m_cState==POLYLINE_4PT::STATE_VALID)
		{
			int iActive=m_xPolyline.m_iActivePt;
			if(iActive>=0&&iActive<=4)
			{
				m_xPolyline.m_ptArr[iActive]=TransPoint(point,TRUE);
				InvalidateRect(&rect,FALSE);
			}
		}
		else if(m_xPolyline.m_cState==POLYLINE_4PT::STATE_WAITNEXTPT)
		{
			CPoint startPt=m_xPolyline.m_ptArr[0];
			CPoint endPt=TransPoint(point,TRUE);
			if(abs(startPt.x-endPt.x)<10||abs(startPt.y-endPt.y)<10)
			{
				m_xPolyline.m_cState=POLYLINE_4PT::STATE_NONE;
				return;
			}
			if(startPt.x>endPt.x)
			{	//��ʼ��Ϊ�Ҳ��
				if(startPt.y>endPt.y)
				{	//��ʼ��Ϊ����
					m_xPolyline.m_ptArr[0]=endPt;						//����
					m_xPolyline.m_ptArr[1].SetPoint(startPt.x,endPt.y);	//����
					m_xPolyline.m_ptArr[2]=startPt;						//����
					m_xPolyline.m_ptArr[3].SetPoint(endPt.x,startPt.y);	//����
				}
				else
				{	//��ʼ��Ϊ����
					m_xPolyline.m_ptArr[0].SetPoint(endPt.x,startPt.y);	//����
					m_xPolyline.m_ptArr[1]=startPt;						//����
					m_xPolyline.m_ptArr[2].SetPoint(startPt.x,endPt.y);	//����
					m_xPolyline.m_ptArr[3]=endPt;						//����
				}
			}
			else 
			{	//��ʼ��Ϊ����
				if(startPt.y>endPt.y)
				{	//��ʼ��Ϊ����
					m_xPolyline.m_ptArr[0].SetPoint(startPt.x,endPt.y);	//����
					m_xPolyline.m_ptArr[1]=endPt;						//����
					m_xPolyline.m_ptArr[2].SetPoint(endPt.x,startPt.y);	//����
					m_xPolyline.m_ptArr[3]=startPt;						//����
				}
				else
				{	//��ʼ��Ϊ����
					m_xPolyline.m_ptArr[0]=startPt;						//����
					m_xPolyline.m_ptArr[1].SetPoint(endPt.x,startPt.y);	//����
					m_xPolyline.m_ptArr[2]=endPt;						//����
					m_xPolyline.m_ptArr[3].SetPoint(startPt.x,endPt.y);	//����
				}
			}
			m_xPolyline.m_iActivePt=-1;
			m_xPolyline.m_cState=POLYLINE_4PT::STATE_VALID;
			//TODO:�Զ���λ���½������ϽǶ�������
			for(int i=0;i<4;i++)
				m_xPolyline.m_ptArr[i]=IntelliAnchorCrossHairPoint(m_xPolyline.m_ptArr[i],i);
			InvalidateRect(&rect,FALSE);
		}
	}
	if(GetCurTask()==TASK_MOVE || nFlags&MK_CONTROL)
	{
		if(GetCapture()==this)
			ReleaseCapture();
		::ClipCursor(NULL);
		if(!rect.PtInRect(point))
		{
			OperOther();
			return;
		}
		CSize offset=point-m_ptLBDownPos;
		if(m_cDisType==CDataCmpModel::DWG_IMAGE_DATA&&m_bStartDrag)
		{
			CImageFileHost* pImage=(CImageFileHost*)m_pData;
			CRect imageRect=pImage->GetImageDisplayRect();
			BYTE byteState=GetOffsetState(rect,imageRect);
			BYTE byteArr[4]={0x01,0x02,0x04,0x08};	//��������
			if(( offset.cy<0&&!(byteArr[0]&byteState))||
				(offset.cy>0&&!(byteArr[1]&byteState)))
				offset.cy=0;	//�����ϻ�����
			if(( offset.cx<0&&!(byteArr[2]&byteState))||
				(offset.cx>0&&!(byteArr[3]&byteState)))
				offset.cx=0;	//�����������
			if(pImage&&(abs(offset.cx)>2||abs(offset.cy)>2)&&!pImage->IsCanZoom())
			{	//�����϶�
				pImage->SetScrOffset(CSize(pImage->GetScrOffset())+offset);
				InvalidateRect(&rect,FALSE);
			}
			m_bStartDrag=FALSE;
		}
		else if(m_cDisType==CDataCmpModel::IMAGE_REGION_DATA&&m_bStartDrag)
		{
			CImageRegionHost *pRegion=(CImageRegionHost*)m_pData;
			//IImageFile* pImage=pRegion->BelongImageData();
			CRect imageRect;//=pImage->GetImageDisplayRect(pRegion->GetSerial());
			BYTE byteState=GetOffsetState(rect,imageRect);
			BYTE byteArr[4]={0x01,0x02,0x04,0x08};	//��������
			if(( offset.cy<0&&!(byteArr[0]&byteState))||
				(offset.cy>0&&!(byteArr[1]&byteState)))
				offset.cy=0;	//�����ϻ�����
			//ͼƬ����֧�������ƶ� wht 18-03-26
			//if(( offset.cx<0&&!(byteArr[2]&byteState))||
			//	(offset.cx>0&&!(byteArr[3]&byteState)))
				offset.cx=0;	//�����������
			if((abs(offset.cx)>2||abs(offset.cy)>2)&&!pRegion->IsCanZoom())
			{	//�����϶�
				pRegion->SetScrOffset(CSize(pRegion->GetScrOffset())+offset);
				Invalidate(FALSE);
			}
			m_bStartDrag=FALSE;
		}
	}
	CDialog::OnLButtonUp(nFlags, point);
}
void CDisplayDataPage::OnMButtonDown(UINT nFlags, CPoint point)
{
	CRect rect;
	GetWorkPanelRect(rect);
	if(!rect.PtInRect(point))
	{
		OperOther();
		return;
	}
	SetCapture();
	if(m_cDisType==CDataCmpModel::DWG_IMAGE_DATA ||
		m_cDisType==CDataCmpModel::IMAGE_REGION_DATA)
	{
		::SetCursor(m_hMove);
		m_ptLBDownPos=point;
		m_bStartDrag=TRUE;
	}
	CDialog::OnMButtonDown(nFlags,point);
}
void CDisplayDataPage::OnMButtonUp(UINT nFlags, CPoint point)
{
	if(GetCapture()==this)
		ReleaseCapture();
	::ClipCursor(NULL);
	CRect rect;
	GetWorkPanelRect(rect);
	if(!rect.PtInRect(point))
	{
		OperOther();
		return;
	}
	CSize offset=point-m_ptLBDownPos;
	if(m_cDisType==CDataCmpModel::DWG_IMAGE_DATA)
	{
		CImageFileHost* pImage=(CImageFileHost*)m_pData;
		CRect imageRect=pImage->GetImageDisplayRect();
		BYTE byteState=GetOffsetState(rect,imageRect);
		BYTE byteArr[4]={0x01,0x02,0x04,0x08};	//��������
		if(( offset.cy<0&&!(byteArr[0]&byteState))||
			(offset.cy>0&&!(byteArr[1]&byteState)))
			offset.cy=0;	//�����ϻ�����
		if(( offset.cx<0&&!(byteArr[2]&byteState))||
			(offset.cx>0&&!(byteArr[3]&byteState)))
			offset.cx=0;	//�����������
		if(pImage&&(abs(offset.cx)>2||abs(offset.cy)>2)&&!pImage->IsCanZoom())
		{	//�����϶�
			pImage->SetScrOffset(CSize(pImage->GetScrOffset())+offset);
			InvalidateRect(&rect,FALSE);
		}
	}
	else if(m_cDisType==CDataCmpModel::IMAGE_REGION_DATA)
	{
		CImageRegionHost *pRegion=(CImageRegionHost*)m_pData;
		CRect imageRect;
		BYTE byteState=GetOffsetState(rect,imageRect);
		BYTE byteArr[4]={0x01,0x02,0x04,0x08};	//��������
		if(( offset.cy<0&&!(byteArr[0]&byteState))||
			(offset.cy>0&&!(byteArr[1]&byteState)))
			offset.cy=0;	//�����ϻ�����
		//ͼƬ����֧�������ƶ� wht 18-03-26
		//if(( offset.cx<0&&!(byteArr[2]&byteState))||
		//	(offset.cx>0&&!(byteArr[3]&byteState)))
			offset.cx=0;	//�����������
		if((abs(offset.cx)>2||abs(offset.cy)>2)&&!pRegion->IsCanZoom())
		{	//�����϶�
			pRegion->SetScrOffset(CSize(pRegion->GetScrOffset())+offset);
			Invalidate(FALSE);
		}
	}
	m_bStartDrag=FALSE;
	CDialog::OnMButtonUp(nFlags,point);
}
void CDisplayDataPage::OnMouseMove(UINT nFlags, CPoint point)
{
	CRect panelRect,listCtrlRect;
	CWnd* pWorkPanel=GetWorkPanelRect(panelRect);
	if(panelRect.PtInRect(point))
		pWorkPanel->SetFocus();		//���ý���
	else
		m_listDataCtrl.SetFocus();
	m_hCursor=LoadCursor(NULL,IDC_ARROW);
	if(GetCurTask()==TASK_OTHER)
	{
		if(m_bHorzTracing&&point.y>(m_rcClient.top+50)&&point.y<(m_rcClient.bottom-50))
		{	//��������������ˮƽ�ƶ�
			CClientDC dc(this);
			InvertLine(&dc,CPoint(m_rcClient.left,m_nOldHorzY),CPoint(m_rcClient.right,m_nOldHorzY));
			InvertLine(&dc,CPoint(m_rcClient.left,point.y),CPoint(m_rcClient.right,point.y));
			ReleaseDC(&dc);
			m_nOldHorzY = point.y;
		}
		else if((point.y >= m_nOldHorzY-5) && (point.y <= m_nOldHorzY+5))
			::SetCursor(m_hCursor=m_hHorzSize);
	}
	if(GetCurTask()==TASK_SNAP && m_cDisType==CDataCmpModel::DWG_IMAGE_DATA)
	{	//ͼƬ���������
		if(m_xPolyline.m_cState==POLYLINE_4PT::STATE_VALID&&	//ѡ�����Ч
			!(nFlags&MK_LBUTTON)&&!(nFlags&MK_RBUTTON))			//δ��������Ҽ�
		{
			m_xPolyline.m_iActivePt=-1;
			for(int i=0;i<4;i++)
			{
				CPoint pt=TransPoint(m_xPolyline.m_ptArr[i],FALSE);
				CRect rect(pt.x-6,pt.y-6,pt.x+6,pt.y+6);
				if(rect.PtInRect(point))
				{
					m_xPolyline.m_iActivePt=i;
					::SetCursor(m_hCursor=m_hPin);
					break;
				}
			}
			if(m_xPolyline.m_iActivePt>=0&&m_xPolyline.m_iActivePt<=4)
				InvalidateRect(&panelRect,FALSE);
		}
		else if(m_xPolyline.m_cState==POLYLINE_4PT::STATE_VALID&&(nFlags&MK_LBUTTON)&&
			m_xPolyline.m_iActivePt>=0&&m_xPolyline.m_iActivePt<=4)
		{	//�������������Լ��������ƶ�
			::SetCursor(m_hCursor=m_hPin);
			int i=m_xPolyline.m_iActivePt;
			CPoint prePt =TransPoint(m_xPolyline.m_ptArr[(i+4-1)%4],FALSE);
			CPoint nextPt=TransPoint(m_xPolyline.m_ptArr[(i+1)%4],FALSE);
			CClientDC dc(this);

			InvertLine(&dc,prePt,m_ptLBDownPos,&panelRect);
			InvertLine(&dc,prePt,point,&panelRect);
			//
			InvertLine(&dc,nextPt,m_ptLBDownPos,&panelRect);
			InvertLine(&dc,nextPt,point,&panelRect);
			//
			InvertEllipse(&dc,m_ptLBDownPos);
			InvertEllipse(&dc,point);
			ReleaseDC(&dc);
			m_ptLBDownPos=point;
		}
		else if(m_xPolyline.m_cState==POLYLINE_4PT::STATE_WAITNEXTPT&&(nFlags&MK_LBUTTON))
		{	//�����������ƶ�����ʼ��ʰȡ����
			::SetCursor(m_hCursor=m_hCross);
			CPoint startPt=TransPoint(m_xPolyline.m_ptArr[0],FALSE);
			CClientDC dc(this);
			InvertLine(&dc,startPt,CPoint(m_ptLBDownPos.x,startPt.y));
			InvertLine(&dc,startPt,CPoint(point.x,startPt.y));
			//
			InvertLine(&dc,CPoint(m_ptLBDownPos.x,startPt.y),m_ptLBDownPos);
			InvertLine(&dc,CPoint(point.x,startPt.y),point);
			//
			InvertLine(&dc,m_ptLBDownPos,CPoint(startPt.x,m_ptLBDownPos.y));
			InvertLine(&dc,point,CPoint(startPt.x,point.y));
			//
			InvertLine(&dc,CPoint(startPt.x,m_ptLBDownPos.y),startPt);
			InvertLine(&dc,CPoint(startPt.x,point.y),startPt);
			ReleaseDC(&dc);
			m_ptLBDownPos=point;
		}
	}
	if((GetCurTask()==TASK_MOVE||((nFlags&MK_CONTROL)&&(nFlags&MK_LBUTTON))||nFlags&MK_MBUTTON)&& m_bStartDrag)
	{
		CSize offset=point-m_ptLBDownPos;
		if(m_cDisType==CDataCmpModel::DWG_IMAGE_DATA)
		{
			CImageFileHost* pImage=(CImageFileHost*)m_pData;
			::SetCursor(m_hCursor=m_hMove);
			pImage->SetScrOffset(CSize(pImage->GetScrOffset())+offset);
			InvalidateRect(&panelRect,FALSE);
			m_ptLBDownPos=point;
		}
		else if(m_cDisType==CDataCmpModel::IMAGE_REGION_DATA)
		{
			CImageRegionHost *pRegion=(CImageRegionHost*)m_pData;
			::SetCursor(m_hCursor=m_hMove);
			//ͼƬ����֧�������ƶ� wht 18-03-26
			if(offset.cx!=0)
				offset.cx=0;
			pRegion->SetScrOffset(CSize(pRegion->GetScrOffset())+offset);
			InvalidateRect(&panelRect,FALSE);
			m_ptLBDownPos=point;
		}
	}
	SetCursor(m_hCursor);
	if(!m_bTraced)
	{
		TRACKMOUSEEVENT tme;  
		tme.cbSize =sizeof(tme);  
		tme.dwFlags =TME_LEAVE;  
		tme.dwHoverTime = 0;  
		tme.hwndTrack =m_hWnd;  
		m_bTraced=TrackMouseEvent(&tme);
	}
	CDialog::OnMouseMove(nFlags, point);
}
void CDisplayDataPage::OnMouseLeave()
{
	m_bTraced=false;
	m_hCursor=LoadCursor(NULL,IDC_ARROW);

	CDialog::OnMouseLeave();
}
BOOL CDisplayDataPage::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	CRect rect;
	CWnd* pWorkPanel=GetWorkPanelRect(rect);
	ScreenToClient(&pt);
	if(!rect.PtInRect(pt))
		return CDialog::OnMouseWheel(nFlags,zDelta,pt);
	CPoint pointToPanel(pt.x-rect.left,pt.y-rect.top);
	if(m_cDisType==CDataCmpModel::DWG_IMAGE_DATA)
	{
		//::SetCursor(m_hHorzSize);
		CImageFileHost* pImage=(CImageFileHost*)m_pData;
		pImage->Zoom(zDelta,&pointToPanel);
		InvalidateRect(&rect,FALSE);
	}
	else if(m_cDisType==CDataCmpModel::IMAGE_REGION_DATA&&m_biLayoutType!=3)
	{
		//::SetCursor(m_hHorzSize);
		CImageRegionHost *pRegion=(CImageRegionHost*)m_pData;
		//pRegion->Zoom(zDelta,&pointToPanel);
		SIZE offset=pRegion->GetScrOffset();
		double fRealHeight=pRegion->GetHeight()*pRegion->ZoomRatio();
		double fRectHeight=0.9*rect.Height();
		CRect headerRect;
		m_listDataCtrl.GetHeaderCtrl()->GetWindowRect(&headerRect);
		int maxOffsetY=(m_biLayoutType==2)?headerRect.Height():0;
		if(offset.cy<=maxOffsetY&&abs(offset.cy)<=(fRealHeight-fRectHeight))
		{
			offset.cy+=zDelta;
			if(offset.cy>maxOffsetY)
				offset.cy=maxOffsetY;
			if(abs(offset.cy)>=(fRealHeight-fRectHeight))
				offset.cy=-(int)(fRealHeight-fRectHeight);
			pRegion->SetScrOffset(offset);
		}
		InvalidateRect(&rect,FALSE);
	}
	return CDialog::OnMouseWheel(nFlags,zDelta,pt);
}
void CDisplayDataPage::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if(nChar==VK_ESCAPE)
		OperOther();
	CDialog::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CDisplayDataPage::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ

	CDialog::OnChar(nChar, nRepCnt, nFlags);
}

BOOL CDisplayDataPage::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if(m_hCursor!=NULL)
	{
		SetCursor(m_hCursor);
		return TRUE;// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	}
	else
		SetCursor(LoadCursor(NULL,IDC_ARROW));  

	return CDialog::OnSetCursor(pWnd, nHitTest, message);
}
static int _localPrevSliderPosition=50;
void CDisplayDataPage::OnNMCustomdrawSliderBalanceCoef(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	int nPos = m_sliderBalanceCoef.GetPos();
	if(nPos==_localPrevSliderPosition)
		return;
	_localPrevSliderPosition=nPos;
	CString sCoef;
	sCoef.Format("%d",nPos);
	SetDlgItemText(IDC_S_BALANCE_COEF,sCoef);
	//
	CRect rect;
	CWnd* pWorkPanel=GetWorkPanelRect(rect);
	if(m_cDisType==CDataCmpModel::DWG_IMAGE_DATA)
	{
		CImageFileHost* pImage=(CImageFileHost*)m_pData;
		pImage->SetMonoThresholdBalanceCoef((nPos-50)*0.01,true);
		InvalidateRect(&rect,FALSE);
	}
	/*else if(m_cDisType==CDataCmpModel::IMAGE_REGION_DATA)
	{
		CImageRegionHost *pRegion=(CImageRegionHost*)m_pData;
		pRegion->SetMonoThresholdBalanceCoef(nPos*0.01);
		InvalidateRect(&rect,FALSE);
	}*/
	*pResult = 0;
}


void CDisplayDataPage::OnCbnSelchangeCmbModel()
{
	UpdateData(TRUE);
	if(m_iModel==1)
		m_biLayoutType=3;
	else if(m_iModel==2)
		m_biLayoutType=4; 
	else 
		m_biLayoutType=2;
	RelayoutWnd();
	m_listDataCtrl.EnableSortItems((m_biLayoutType==3)?false:true);
	CImageRegionHost *pRegion=(CImageRegionHost*)m_pData;
	IRecoginizer::BOMPART *pBomPart=pRegion?pRegion->EnumFirstPart():NULL;
	RefreshListCtrl((long)pBomPart);

	RECT rc;
	if(pBomPart&&!pRegion->GetBomPartRect(pBomPart->id,&rc))
	{
		rc=pBomPart->rc;
		Focus(rc,NULL);
		Invalidate(FALSE);
	}
}
