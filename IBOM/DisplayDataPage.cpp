// DisplayDdataPage.cpp : 实现文件
//
#include "stdafx.h"
#include "IBOM.h"
#include "IBOMView.h"
#include "DisplayDataPage.h"
#include "afxdialogex.h"
#include "ArrayList.h"
#include "Config.h"
#include "ScreenAdapter.h"
#include "imm.h"
#include "MainFrm.h"
#include "Tool.h"
#include "folder_dialog.h"
#include "PartLibraryEx.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
/////////////////////////////////////////////////////////////////////////////////////////
static void SimplifiedNumString(CString &num_str)
{
	int str_len = num_str.GetLength();
	int pointPos = -1;
	for(int i=str_len-1;i>=0;i--)
	{
		if(num_str[i]=='.')
		{
			pointPos=i;
			break;
		}
	}
	if(pointPos<0)
		return;
	if(pointPos>0&&!isdigit((BYTE)num_str[pointPos-1]))
		return; //小数点前一位为数字时才进行下一步的简化 wht 11-04-01
	int iPos=str_len-1;
	while(iPos>=pointPos)
	{
		if(num_str[iPos]=='0'||num_str[iPos]=='.')
			iPos--;
		else
			break;
	}
	num_str=num_str.Left(iPos+1);
	if(num_str.Compare("-0")==0)
		num_str="0";
}
static int f2i(double fVal)
{
	if(fVal>0)
		return (int)(fVal+0.5);
	else
		return (int)(fVal-0.5);
}
POINT PIXEL_TRANS::TransToLocal(const POINT& xPixelGlobal)
{
	POINT xPixel=xPixelGlobal;
	xPixel.x-=xOrg.x;
	xPixel.y-=xOrg.y;
	xPixel.x=f2i(xPixel.x*this->fScale2Global);
	xPixel.y=f2i(xPixel.y*this->fScale2Global);
	return xPixel;
}
POINT PIXEL_TRANS::TransToGlobal(const POINT& xPixelLocal)
{
	POINT xPixel=xPixelLocal;
	xPixel.x=f2i(xPixel.x/this->fScale2Global);
	xPixel.y=f2i(xPixel.y/this->fScale2Global);
	xPixel.x+=xOrg.x;
	xPixel.y+=xOrg.y;
	return xPixel;
}
/////////////////////////////////////////////////////////////////////////////////////////
const BYTE constByteArr[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
static COLORREF GetBomPartCellBackColor(CSuperGridCtrl &xListCtrl,IRecoginizer::BOMPART *pPart,int iSubItem)
{
	CDisplayDataPage *pDlg=(CDisplayDataPage*)xListCtrl.GetParent();
	if(pDlg==NULL||iSubItem<0||iSubItem>6)
		return 0;
	if(pPart->ciSrcFromMode&constByteArr[iSubItem])
		return CConfig::MODIFY_COLOR;
	else 
	{	//根据匹配程度显示背景颜色
		BYTE cLevel=pPart->arrItemWarningLevel[iSubItem];
		if(cLevel==0xFF)
			return CConfig::DRAWING_ERROR_COLOR;
		else if(cLevel>2)
			return 0;
		else
			return CConfig::warningLevelClrArr[cLevel];
	}
}
static CSuperGridCtrl::CTreeItem* InsertOrUpdatePartTreeItem(CSuperGridCtrl &xListCtrl,IRecoginizer::BOMPART *pPart,CSuperGridCtrl::CTreeItem *pItem=NULL,BOOL bUpdate=FALSE)
{
	CListCtrlItemInfo *lpInfo=NULL;
	if(pItem)
		lpInfo=pItem->m_lpNodeInfo;
	else
		lpInfo=new CListCtrlItemInfo();
	COLORREF clr=0;
	BOOL bReadOnly=TRUE;
	if(pPart&&pPart->bInput)
		bReadOnly=FALSE;
	//编号
	lpInfo->SetSubItemText(0,"",bReadOnly);
	if(pPart)
	{
		lpInfo->SetSubItemText(0,pPart->sLabel,bReadOnly);
		if((clr=GetBomPartCellBackColor(xListCtrl,pPart,0))>0)
			lpInfo->SetSubItemColor(0,clr);
	}
	//材质
	lpInfo->SetSubItemText(1,"",TRUE);
	if(pPart&&stricmp(pPart->materialStr,"Q235")!=0)
	{
		lpInfo->SetSubItemText(1,pPart->materialStr,TRUE);
		if((clr=GetBomPartCellBackColor(xListCtrl,pPart,1))>0)
			lpInfo->SetSubItemColor(1,clr);
	}
	//规格
	lpInfo->SetSubItemText(2,"",bReadOnly);
	if(pPart)
	{
		lpInfo->SetSubItemText(2,pPart->sSizeStr,bReadOnly);
		if((clr=GetBomPartCellBackColor(xListCtrl,pPart,2))>0)
			lpInfo->SetSubItemColor(2,clr);
	}
	//长度
	lpInfo->SetSubItemText(3,"",bReadOnly);
	if(pPart)
	{
		if(pPart->wPartType==IRecoginizer::BOMPART::ACCESSORY&&pPart->siSubType==IRecoginizer::BOMPART::SUB_TYPE_STEEL_GRATING)
			lpInfo->SetSubItemText(3,CXhChar50("%.fX%.f",pPart->width,pPart->length),bReadOnly);	//钢格栅
		else
			lpInfo->SetSubItemText(3,CXhChar50("%.f",pPart->length),bReadOnly);	
		if((clr=GetBomPartCellBackColor(xListCtrl,pPart,3))>0)
			lpInfo->SetSubItemColor(3,clr);
	}
	//单基数
	lpInfo->SetSubItemText(4,"",bReadOnly);
	if(pPart)
	{
		lpInfo->SetSubItemText(4,CXhChar50("%d",pPart->count),bReadOnly);
		if((clr=GetBomPartCellBackColor(xListCtrl,pPart,4))>0)
			lpInfo->SetSubItemColor(4,clr);
	}
	//单重
	CalAndSyncTheoryWeightToWeight(pPart,FALSE);
	double fScrWeight=pPart==NULL?0:((int)(pPart->weight*100+0.5))*0.01;
	CXhChar50 sWeight("%.2f",fScrWeight);
	lpInfo->SetSubItemText(5,"",bReadOnly);
	if(pPart)
	{
		lpInfo->SetSubItemText(5,sWeight,bReadOnly);
		if(CConfig::m_bRecogPieceWeight||!CConfig::m_bRecogSumWeight)
		{	//识别单重或不识别总重时标记单重错误提示 wht 19-01-12
			if((clr=GetBomPartCellBackColor(xListCtrl,pPart,5))>0)
				lpInfo->SetSubItemColor(5,clr);
		}
	}
	//总重
	lpInfo->SetSubItemText(6,"",TRUE);
	if(pPart)
	{
		double fCalWeight=atof(CXhChar50("%.2f",pPart->calWeight))*pPart->count;
		//if(fSumWeight<=0)
		//	fSumWeight=fScrWeight*pPart->count;
		lpInfo->SetSubItemText(6,CXhChar50("%.1f",fCalWeight),TRUE);
		double fSumWeight=pPart->sumWeight;
		if(CConfig::m_bRecogSumWeight&&fSumWeight==0)
			pPart->arrItemWarningLevel[6]=2;
		else if(pPart->arrItemWarningLevel[6]!=0xFF)
			pPart->arrItemWarningLevel[6]=0;
		//图纸总重
		if(CConfig::m_bRecogSumWeight)
		{
			if(pPart->arrItemWarningLevel[6]!=2&&fabs(fCalWeight-fSumWeight)>fabs(CConfig::m_fWeightEPS))
				pPart->arrItemWarningLevel[6]=1;
			lpInfo->SetSubItemText(7,CXhChar50("%.1f",fSumWeight),bReadOnly);
			if((clr=GetBomPartCellBackColor(xListCtrl,pPart,7))>0)
				lpInfo->SetSubItemColor(7,clr);
		}
		if((clr=GetBomPartCellBackColor(xListCtrl,pPart,6))>0)
			lpInfo->SetSubItemColor(6,clr);
	}

	if(pItem==NULL)
		pItem=pItem=xListCtrl.InsertRootItem(lpInfo,bUpdate);
	else
		xListCtrl.RefreshItem(pItem);
	pItem->m_idProp=(long)pPart;
	return pItem;
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
	if(pDlg==NULL||pDlg->m_cDisType!=CDataCmpModel::IMAGE_REGION_DATA ||pDlg->m_pData==NULL)
		return FALSE;
	pDlg->Focus(pListCtrl,pItem);
	pDlg->Invalidate(FALSE);
	return TRUE;
}

static BOOL FireLButtonDown(CSuperGridCtrl* pListCtrl,CSuperGridCtrl::CTreeItem* pItem,int iSubItem)
{	
	CDisplayDataPage *pDlg=(CDisplayDataPage*)pListCtrl->GetParent();
	if(pDlg==NULL||pDlg->m_cDisType!=CDataCmpModel::IMAGE_REGION_DATA ||pDlg->m_pData==NULL)
		return FALSE;
	if(pDlg->biLayoutType==3)
	{	//按列校审
		int iCurCol=pListCtrl->GetCurSubItem();
		if(pDlg->biCurCol!=iCurCol)
			pDlg->ShiftCheckColumn(iCurCol);
	}
	pDlg->Focus(pListCtrl,pItem);
	pDlg->Invalidate(FALSE);
	return TRUE;
}

static BOOL FireColumnClick(CSuperGridCtrl* pListCtrl,int iSubItem,bool bAscending)
{
	CDisplayDataPage *pDlg=(CDisplayDataPage*)pListCtrl->GetParent();
	if(pDlg==NULL||pDlg->m_cDisType!=CDataCmpModel::IMAGE_REGION_DATA ||pDlg->m_pData==NULL)
		return FALSE;
	if(pDlg->biLayoutType==3)
	{	//按列校审
		if(pDlg->biCurCol!=iSubItem)
			pDlg->ShiftCheckColumn(iSubItem);
		pDlg->Focus(pListCtrl,NULL);
		pDlg->Invalidate(FALSE);
		return TRUE;
	}
	else
		return FALSE;
}
static BOOL FireScroll(CSuperGridCtrl* pListCtrl,UINT nSBCode, UINT nPos, CScrollBar* pScrollBar,BOOL bHScroll)
{
	CDisplayDataPage *pDlg=(CDisplayDataPage*)pListCtrl->GetParent();
	if(pDlg==NULL||pDlg->m_cDisType!=CDataCmpModel::IMAGE_REGION_DATA ||pDlg->m_pData==NULL)
		return FALSE;
	pDlg->Focus(pListCtrl,NULL);
	pDlg->Invalidate(FALSE);
	return true;
}
static BOOL FireEndtrack(CSuperGridCtrl* pListCtrl)
{
	CDisplayDataPage *pDlg=(CDisplayDataPage*)pListCtrl->GetParent();
	if(pDlg==NULL||pDlg->m_pData==NULL)
		return FALSE;
	if(pDlg->m_cDisType!=CDataCmpModel::IMAGE_REGION_DATA&&pDlg->m_cDisType!=CDataCmpModel::DWG_IMAGE_DATA)
		return FALSE;
	for(int i=0;i<pListCtrl->GetColumnCount();i++)
	{
		if(pListCtrl->GetColumnWidth(i)==0)
			continue;
		CDisplayDataPage::m_arrColWidth[i]=pListCtrl->GetColumnWidth(i);
	}
	pDlg->RelayoutWnd();
	return true;
}
static BOOL FireDeleteItem(CSuperGridCtrl* pListCtrl,CSuperGridCtrl::CTreeItem* pItem)
{
	return FALSE;
	//从哈希表中删除
	/*CDisplayDataPage *pDlg=(CDisplayDataPage*)pListCtrl->GetParent();
	if(pDlg==NULL||pDlg->m_cDisType!=CDataCmpModel::IMAGE_REGION_DATA ||pDlg->m_pData==NULL)
		return FALSE;
	CImageRegionHost* pRegionHost=(CImageRegionHost*)pDlg->m_pData;
	CString sValue=pItem->m_lpNodeInfo->GetSubItemText(0);
	if(sValue.GetLength()>0)
		pRegionHost->DeleteBomPart(sValue);
	pListCtrl->DeleteItem(pItem->GetIndex());
	return 1;*/
}
static BOOL FireMouseWheel(CSuperGridCtrl* pListCtrl,CSuperGridCtrl::CTreeItem* pItem,UINT nFlags, short zDelta, CPoint pt)
{
	CDisplayDataPage *pDlg=(CDisplayDataPage*)pListCtrl->GetParent();
	if(pDlg==NULL||pDlg->m_cDisType!=CDataCmpModel::IMAGE_REGION_DATA ||pDlg->m_pData==NULL)
		return FALSE;
	int nPos=pListCtrl->GetScrollPos(SB_VERT);
	//if(nPos==0)
	//	return FALSE;
	pDlg->HideInputCtrl();
	pDlg->Focus(pListCtrl,pItem);
	return true;
}

static BOOL FireContextMenu(CSuperGridCtrl* pListCtrl,CSuperGridCtrl::CTreeItem* pItem,CPoint point)
{
	CDisplayDataPage *pDlg=(CDisplayDataPage*)pListCtrl->GetParent();
	if( pDlg==NULL||
		(pListCtrl==&pDlg->m_listFileCtrl&&pDlg->m_cDisType!=CDataCmpModel::DWG_IMAGE_GROUP)||
		(pListCtrl==&pDlg->m_listDataCtrl&&pDlg->biLayoutType!=CDisplayDataPage::LAYOUT_BROWSE&&pDlg->biLayoutType!=CDisplayDataPage::LAYOUT_COLREV))
		return FALSE;
	CMenu popMenu;
	popMenu.LoadMenu(IDR_ITEM_CMD_POPUP);
	CMenu *pMenu=popMenu.GetSubMenu(0);
	while(pMenu->GetMenuItemCount()>0)
		pMenu->DeleteMenu(0,MF_BYPOSITION);
	if(pListCtrl==&pDlg->m_listDataCtrl)
	{
		int iCurSubItem=pListCtrl->GetCurSubItem();
		IRecoginizer::BOMPART *pBomPart=pItem?(IRecoginizer::BOMPART*)pItem->m_idProp:NULL;
		if(pBomPart)
		{
			pMenu->AppendMenu(MF_STRING,ID_COPY_ITEM,"复制");
			if(pBomPart->bInput)
			{
				if(pDlg->IsCanPasteItem())
					pMenu->AppendMenu(MF_STRING,ID_PASTE_ITEM,"粘贴");
				pMenu->AppendMenu(MF_SEPARATOR);
				pMenu->AppendMenu(MF_STRING,ID_DELETE_PART,"删除构件");
			}
			pMenu->AppendMenu(MF_SEPARATOR);
			if(pBomPart->arrItemWarningLevel[iCurSubItem]==0xFF)
				pMenu->AppendMenu(MF_STRING,ID_MARK_NORMAL,"标记为正常");
			else 
				pMenu->AppendMenu(MF_STRING,ID_MARK_ERROR,"标记为错误");
		}
		else
			pMenu->AppendMenu(MF_STRING,ID_ADD_PART,"添加构件");
	}
	else if(pListCtrl==&pDlg->m_listFileCtrl)
		pMenu->AppendMenu(MF_STRING,ID_SELECT_FOLDER,"设定图片路径");
	CPoint menu_pos=point;
	pListCtrl->ClientToScreen(&menu_pos);
	popMenu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON,menu_pos.x,menu_pos.y,pDlg);
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
bool IsSpecialCharShortCut(WORD wVKeyCode);
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
		wVKeyCode==VK_SUBTRACT||
		(wVKeyCode>=0x30&&wVKeyCode<=39)||		//0-9
		(wVKeyCode>=0x40&&wVKeyCode<=0x42)||	//A,B,C
		IsSpecialCharShortCut(wVKeyCode))		//特殊字符快捷键
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
	else if(wVKeyCode>=0x30&&wVKeyCode<=0x39)	//0-9
		return CXhChar16("%d",wVKeyCode-0x30);
	else if(wVKeyCode>=0x41&&wVKeyCode<=0x5A)	//A-Z
		return CXhChar16("%C",'A'+(wVKeyCode-0x41));
	else if(wVKeyCode==VK_SUBTRACT)
		return CXhChar16("-");
	else
		return CXhChar16("");
}

bool IsSpecialCharShortCut(WORD wVKeyCode)
{
	CXhChar16 sKey=GetInputKeyText(wVKeyCode);
	if( sKey.EqualNoCase(CConfig::KEY_BIG_FAI)||
		sKey.EqualNoCase(CConfig::KEY_LITTLE_FAI))
		return true;
	else
		return false;
}

CXhChar16 GetRealInputText(WORD wVKeyCode)
{
	CXhChar16 sText=GetInputKeyText(wVKeyCode);
	if(sText.EqualNoCase(CConfig::KEY_BIG_FAI))
		sText.Copy("Φ");
	else if(sText.EqualNoCase(CConfig::KEY_LITTLE_FAI))
		sText.Copy("φ");
	return sText;
}

void UpdatePartWeight(CDisplayDataPage *pDlg,CSuperGridCtrl *pListCtrl,CSuperGridCtrl::CTreeItem *pItem,
					  IRecoginizer::BOMPART *pCurPart,BOOL bCalWeight=TRUE)
{
	if(pDlg==NULL||pListCtrl==NULL||pItem==NULL||pCurPart==NULL)
		return;
	BYTE biCurCol=pDlg->biCurCol;
	if(pDlg->biLayoutType!=CDisplayDataPage::LAYOUT_COLREV)
		biCurCol=pListCtrl->GetCurSubItem();
	BOOL bModify=FALSE;
	if(biCurCol>=0&&biCurCol<=6)
	{
		BYTE byteArr[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
		bModify=(pCurPart->ciSrcFromMode&byteArr[biCurCol]);
	}
	if(bModify)
		pItem->m_lpNodeInfo->SetSubItemColor(biCurCol,CConfig::MODIFY_COLOR);
	CalAndSyncTheoryWeightToWeight(pCurPart,bCalWeight);
	CXhChar50 sWeight("%.2f",pCurPart?pCurPart->weight:0);
	pListCtrl->SetSubItemText(pItem,5,sWeight);
	pListCtrl->SetSubItemText(pItem,6,CXhChar50("%.1f",pCurPart->calSumWeight));
	if(CConfig::m_bRecogSumWeight)
		pListCtrl->SetSubItemText(pItem,7,CXhChar50("%.1f",pCurPart->sumWeight));
	if(pDlg->biLayoutType==CDisplayDataPage::LAYOUT_COLREV)
	{
		CSuperGridCtrl::CTreeItem *pCoupleItem=pDlg->m_listDataCtrl1.GetTreeItem(pItem->GetIndex());
		if(pCoupleItem)
		{
			if(bModify)
				pCoupleItem->m_lpNodeInfo->SetSubItemColor(biCurCol,CConfig::MODIFY_COLOR);
			pDlg->m_listDataCtrl1.SetSubItemText(pItem,biCurCol,pItem->m_lpNodeInfo->GetSubItemText(biCurCol));
			pDlg->m_listDataCtrl1.SetSubItemText(pCoupleItem,5,sWeight);
			pDlg->m_listDataCtrl1.SetSubItemText(pCoupleItem,6,CXhChar50("%.1f",pCurPart->calSumWeight));
			if(CConfig::m_bRecogSumWeight)
				pDlg->m_listDataCtrl1.SetSubItemText(pItem,7,CXhChar50("%.1f",pCurPart->sumWeight));
		}
	}
	double fSumWeight=0;
	CSuperGridCtrl::CTreeItem *pCurItem=NULL;
	for(int i=0;i<pListCtrl->GetItemCount();i++)
	{
		pCurItem=pListCtrl->GetTreeItem(i);
		IRecoginizer::BOMPART *pBomPart=(IRecoginizer::BOMPART*)pCurItem->m_idProp;
		if(pBomPart==NULL)
			continue;
		//sWeight.Printf("%.2f",pBomPart->weight);
		double fPartSumWeight=atof(CXhChar50("%.1f",pBomPart->sumWeight));
		if(!CConfig::m_bRecogSumWeight)
			fPartSumWeight=atof(CXhChar50("%.1f",pBomPart->calSumWeight));
		int iOldLevel=pBomPart->arrItemWarningLevel[6];
		if(fPartSumWeight==0)
			pBomPart->arrItemWarningLevel[6]=2;
		else if(pBomPart->arrItemWarningLevel[6]!=0xFF)
			pBomPart->arrItemWarningLevel[6]=0;
		if(iOldLevel!=pBomPart->arrItemWarningLevel[6])
		{
			COLORREF clr;
			if((clr=GetBomPartCellBackColor(*pListCtrl,pBomPart,6))>0)
				pCurItem->m_lpNodeInfo->SetSubItemColor(6,clr);
		}
		fSumWeight+=fPartSumWeight;

	}
	pDlg->GetDlgItem(IDC_S_SUM_WEIGHT)->SetWindowText(CXhChar50("%.1f",fSumWeight));
}
static BOOL FireEditBoxKeyUpItem(CSuperEdit* pEdit,UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CSuperGridCtrl *pListCtrl = (CSuperGridCtrl*)pEdit->GetParent();
	if(pListCtrl==NULL)
		return FALSE;
	CDisplayDataPage *pDlg=(CDisplayDataPage*)pListCtrl->GetParent();
	if(pDlg->m_cDisType!=CDataCmpModel::IMAGE_REGION_DATA ||pDlg->m_pData==NULL)
		return FALSE;
	if(pDlg->biLayoutType!=2&&pDlg->biLayoutType!=3&&pDlg->biLayoutType!=4)
		return FALSE;
	int iSel=pListCtrl->GetSelectedItem();
	CSuperGridCtrl::CTreeItem *pItem = pListCtrl->GetTreeItem(iSel);
	if(pItem==NULL)
		return FALSE;
	BYTE biCurCol=pDlg->biCurCol;
	if(pDlg->biLayoutType!=3)
		biCurCol=pListCtrl->GetCurSubItem();
	WORD wVKey=CConfig::GetCfgVKCode(nChar,pListCtrl->GetSafeHwnd());
	if(IsInputTextKey(wVKey))
	{	//数字等用于输入的字符
		if(pItem&&pListCtrl->IsDisplayCellCtrl(pItem->GetIndex(),biCurCol)&&biCurCol==2)
		{	//规格列启用特殊字符快捷键
			CString sText;
			if(IsSpecialCharShortCut(wVKey))
			{
				pEdit->GetWindowText(sText);
				CString sInput=GetInputKeyText(wVKey);
				CString sRealInput=GetRealInputText(wVKey);
				sText.Replace(sInput,sRealInput);
				sInput=sInput.MakeLower();
				sText.Replace(sInput,sRealInput);
				pEdit->SetWindowText(sText);
				pEdit->SetFocus();
				int len = pEdit->GetWindowTextLength();
				pEdit->SetSel(len, len);
				return TRUE;
			}
		}
	}
	return FALSE;
}
static BOOL FireKeyUpItem(CSuperGridCtrl* pListCtrl,CSuperGridCtrl::CTreeItem *pItem, UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CDisplayDataPage *pDlg=(CDisplayDataPage*)pListCtrl->GetParent();
	if(pDlg->m_cDisType!=CDataCmpModel::IMAGE_REGION_DATA ||pDlg->m_pData==NULL)
		return FALSE;
	if(pDlg->biLayoutType!=2&&pDlg->biLayoutType!=3&&pDlg->biLayoutType!=4)
		return FALSE;
	BYTE biCurCol=pDlg->biCurCol;
	if(pDlg->biLayoutType!=3)
		biCurCol=pListCtrl->GetCurSubItem();
	WORD wVKey=CConfig::GetCfgVKCode(nChar,pListCtrl->GetSafeHwnd());
	if(IsInputTextKey(wVKey))
	{	//数字等用于输入的字符
		if(pItem&&pListCtrl->IsDisplayCellCtrl(pItem->GetIndex(),biCurCol)&&biCurCol==2)
		{	//规格列启用特殊字符快捷键
			CString sText;
			pListCtrl->m_editBox.GetWindowText(sText);
			if(IsSpecialCharShortCut(wVKey))
			{
				sText=sText.MakeUpper();
				CString sInput=GetInputKeyText(wVKey);
				CString sRealInput=GetRealInputText(wVKey);
				sText.Replace(sInput,sRealInput);
				pListCtrl->m_editBox.SetWindowText(sText);
				pListCtrl->m_editBox.SetFocus();
				int len = pListCtrl->m_editBox.GetWindowTextLength();
				pListCtrl->m_editBox.SetSel(len, len);
				return TRUE;
			}
		}
	}
	return FALSE;
}
static BOOL FireKeyDownItem(CSuperGridCtrl* pListCtrl,CSuperGridCtrl::CTreeItem* pItem,LV_KEYDOWN* pLVKeyDow)
{
	CDisplayDataPage *pDlg=(CDisplayDataPage*)pListCtrl->GetParent();
	if(pDlg->m_cDisType==CDataCmpModel::DWG_IMAGE_DATA)
	{
		if(pLVKeyDow->wVKey==VK_ESCAPE)
			pDlg->OperOther();
	}
	if(pDlg->m_cDisType!=CDataCmpModel::IMAGE_REGION_DATA ||pDlg->m_pData==NULL)
		return FALSE;
	if(pDlg->biLayoutType!=2&&pDlg->biLayoutType!=3&&pDlg->biLayoutType!=4)
		return FALSE;
	BYTE biCurCol=pDlg->biCurCol;
	if(pDlg->biLayoutType!=3)
		biCurCol=pListCtrl->GetCurSubItem();
	WORD wVKey=CConfig::GetCfgVKCode(pLVKeyDow->wVKey,pListCtrl->GetSafeHwnd());
	IRecoginizer::BOMPART *pCurPart=pItem!=NULL?(IRecoginizer::BOMPART*)pItem->m_idProp:NULL;
	if(wVKey==VK_F2)
	{
		if( biCurCol==CDisplayDataPage::COL_LABEL||biCurCol==CDisplayDataPage::COL_LEN||
			biCurCol==CDisplayDataPage::COL_SPEC||biCurCol==CDisplayDataPage::COL_WEIGHT||
			biCurCol==CDisplayDataPage::COL_COUNT||biCurCol==CDisplayDataPage::COL_MAP_SUM_WEIGHT)
		{
			//CString sText=pItem->m_lpNodeInfo->GetSubItemText(biCurCol);
			pListCtrl->DisplayCellCtrl(pItem->GetIndex(),biCurCol,NULL);
		}
		return TRUE;
	}
	else if(wVKey==CConfig::CFG_VK_REPEAT)
	{
		if(pItem==NULL)
			return FALSE;
		if(pListCtrl->IsDisplayCellCtrl(pItem->GetIndex(),biCurCol))	//biCurCol==CDisplayDataPage::COL_WEIGHT&&
		{
			CString newStr;
			pListCtrl->m_editBox.GetWindowText(newStr);
			if(newStr.CompareNoCase(".")!=0)
				return FALSE;
		}
		int iCurSel=pItem->GetIndex();
		CSuperGridCtrl::CTreeItem *pPrevItem=iCurSel>0?pListCtrl->GetTreeItem(iCurSel-1):NULL;
		IRecoginizer::BOMPART *pPrevPart=pPrevItem!=NULL?(IRecoginizer::BOMPART*)pPrevItem->m_idProp:NULL;
		CString sOldText=pItem->m_lpNodeInfo->GetSubItemText(biCurCol);
		if(pPrevItem&&pPrevPart)
		{
			if(biCurCol==0)
			{	//件号列，件号加1
				SEGI segI;
				CXhChar16 sSerialNo,sMaterial="H",sSeparator;
				if(ParsePartNo(pPrevPart->sLabel,&segI,sSerialNo,sMaterial,sSeparator))
				{
					if(sSeparator.Length>0)
					{	//件号中有分隔符
						int nSerialNo=ConvertLabelToNum(sSerialNo);
						if(nSerialNo>0)
						{
							if(sSerialNo.StartWith('0')&&nSerialNo<9)
								strcpy(pCurPart->sLabel,CXhChar16("%s%s0%d",(char*)segI.ToString(),(char*)sSeparator,nSerialNo+1));
							else
								strcpy(pCurPart->sLabel,CXhChar16("%s%s%d",(char*)segI.ToString(),(char*)sSeparator,nSerialNo+1));
							pListCtrl->SetSubItemText(pItem,0,pCurPart->sLabel);
						}
					}
					else 
					{	//无分隔符
						int nLabel=ConvertLabelToNum(sSerialNo);
						if(nLabel>0)
						{
							if(nLabel==99)	//大于100
								strcpy(pCurPart->sLabel,CXhChar16("%s-100",(char*)segI.ToString()));
							else if((nLabel+1)<10)
								strcpy(pCurPart->sLabel,CXhChar16("%s%s0%d",(char*)segI.ToString(),(char*)sSeparator,nLabel+1));
							else
								strcpy(pCurPart->sLabel,CXhChar16("%s%s%d",(char*)segI.ToString(),(char*)sSeparator,nLabel+1));
							pListCtrl->SetSubItemText(pItem,0,pCurPart->sLabel);
						}
					}
				}
			}
			else //if(biCurCol==1)
			{	//其它列重复前一列
				CString sText=pPrevItem->m_lpNodeInfo->GetSubItemText(biCurCol);
				pListCtrl->SetSubItemText(pItem,biCurCol,sText);
				if (biCurCol == 1)
					strcpy(pCurPart->materialStr, sText);
				else if (biCurCol == 2)
					strcpy(pCurPart->sSizeStr, sText);
				else if (biCurCol == 3)
					pCurPart->length = atoi(sText);
				else if (biCurCol == 4)
					pCurPart->count = atoi(sText);
				else if (biCurCol == 5)
					pCurPart->weight = atof(sText);
				else if (biCurCol == CDisplayDataPage::COL_MAP_SUM_WEIGHT)
					pCurPart->sumWeight = atof(sText);
			}
			//修改完之后跳转至下一行
			wVKey=CConfig::CFG_VK_DOWN;
			if(sOldText.CompareNoCase(pItem->m_lpNodeInfo->GetSubItemText(biCurCol))!=0)
			{
				BYTE byteArr[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
				if(biCurCol>=0&&biCurCol<=7)
					pCurPart->ciSrcFromMode|=byteArr[biCurCol];
				ParseSpec(pCurPart);
				UpdatePartWeight(pDlg,pListCtrl,pItem,pCurPart);
			}
			if(pListCtrl->m_editBox.GetSafeHwnd()&&pListCtrl->m_editBox.IsWindowVisible())
			{
				pListCtrl->m_editBox.SetWindowText(".");
				pListCtrl->KillfocusEditBox();
			}
		}
		else
			return FALSE;
	}
	else if(wVKey==CConfig::CFG_VK_RETURN||wVKey==CConfig::CFG_VK_SPACE)
	{	//结束输入状态确认,跳转至下一行
		wVKey=CConfig::CFG_VK_DOWN;
	}
	else if( wVKey==CConfig::CFG_VK_Q235||
			 wVKey==CConfig::CFG_VK_Q345||
			 wVKey==CConfig::CFG_VK_Q390||
			 wVKey==CConfig::CFG_VK_Q420)
	{	//通过快捷键录入材质
		if(pItem&&pCurPart&&
		   ((pDlg->biLayoutType==3&&pDlg->biCurCol==CDisplayDataPage::COL_MATERIAL)||
		    (pListCtrl->GetCurSubItem()==CDisplayDataPage::COL_MATERIAL)))
		{
			CString sOldText=pItem->m_lpNodeInfo->GetSubItemText(biCurCol);
			CXhChar16 sMaterial;
			if(wVKey==CConfig::CFG_VK_Q345)
				sMaterial.Copy("Q345");
			else if(wVKey==CConfig::CFG_VK_Q390)
				sMaterial.Copy("Q390");
			else if(wVKey==CConfig::CFG_VK_Q420)
				sMaterial.Copy("Q420");
			pListCtrl->SetSubItemText(pItem,biCurCol,sMaterial);
			strcpy(pCurPart->materialStr,sMaterial);
			if(sOldText.CompareNoCase(pItem->m_lpNodeInfo->GetSubItemText(biCurCol))!=0)
			{
				pItem->m_lpNodeInfo->SetSubItemColor(biCurCol,CConfig::MODIFY_COLOR);
				UpdatePartWeight(pDlg,pListCtrl,pItem,pCurPart);
				pCurPart->ciSrcFromMode|=IRecoginizer::BOMPART::MODIFY_MATERIAL;
			}
		}
	}
	else if(IsInputTextKey(wVKey))
	{	//数字等用于输入的字符
		if(pItem&&!pListCtrl->IsDisplayCellCtrl(pItem->GetIndex(),biCurCol))
		{
			CString sText=GetRealInputText(wVKey);
			if(pLVKeyDow->flags&MK_CONTROL)
			{
				sText=pListCtrl->GetItemText(pItem->GetIndex(),biCurCol);
				sText+=GetRealInputText(wVKey);
			}
			pListCtrl->DisplayCellCtrl(pItem->GetIndex(),biCurCol,sText);
		}
		else if(pItem&&pListCtrl->IsDisplayCellCtrl(pItem->GetIndex(),biCurCol)&&biCurCol==2)
		{	//规格列启用特殊字符快捷键
			/*CString sText;
			pListCtrl->m_editBox.GetWindowText(sText);
			if(IsSpecialCharShortCut(wVKey))
			{
				CString sRealInput=GetRealInputText(wVKey);
				sText+=sRealInput;
				pListCtrl->DisplayCellCtrl(pItem->GetIndex(),biCurCol,sText);
			}*/
		}
		return TRUE;
	}
	else if(wVKey==VK_TAB)
	{
		wVKey=CConfig::CFG_VK_RIGHT;
		if(pDlg->biLayoutType==2&&biCurCol==CDisplayDataPage::COL_WEIGHT)
		{	//按行查看时，如果当前列为单重列，按TAB键跳转至下一行件号列 wht 18-06-24
			wVKey=CConfig::CFG_VK_DOWN;
			biCurCol=0;
		}
	}

	if(wVKey==CConfig::CFG_VK_UP||wVKey==CConfig::CFG_VK_DOWN)
	{
		if(pItem==NULL)
			return FALSE;
		if(pListCtrl->m_editBox.GetSafeHwnd()&&pListCtrl->m_editBox.IsWindowVisible())
			return FALSE;
		int iCurSel=pItem->GetIndex();
		if(wVKey==CConfig::CFG_VK_UP)
		{
			int iMinRow=pDlg->GetHeadOrTailValidItem(pListCtrl,CDisplayDataPage::LISTCTRL_HEAD_ITEM);
			if(iCurSel>=iMinRow)
				iCurSel--;
		}
		else if(wVKey==CConfig::CFG_VK_DOWN)
		{
			int iMaxRow=pDlg->GetHeadOrTailValidItem(pListCtrl,CDisplayDataPage::LISTCTRL_TAIL_ITEM);
			if(iCurSel<iMaxRow)
				iCurSel++;
		}
		if(iCurSel!=pItem->GetIndex())
		{
			if(pItem->GetIndex()>3)
			{	//当前行为前两行时不滚动滚动条 wht 18-06-28
				CRect rc;
				pListCtrl->GetItemRect(pItem->GetIndex(),&rc,LVIR_BOUNDS);
				CSize sz(0,wVKey==CConfig::CFG_VK_UP?rc.Height()*-1:rc.Height());
				pListCtrl->Scroll(sz);
			}
		}
		if(iCurSel!=pItem->GetIndex()&&iCurSel>=0&&iCurSel<=pListCtrl->GetItemCount())
		{
			CSuperGridCtrl::CTreeItem *pNextItem=pListCtrl->GetTreeItem(iCurSel);
			if(pNextItem&&pNextItem->m_idProp!=NULL)
				pListCtrl->SelectItem(pNextItem,biCurCol);
		}
		return TRUE;
	}
	else if(wVKey==CConfig::CFG_VK_LEFT||wVKey==CConfig::CFG_VK_RIGHT)
	{
		if(pListCtrl->m_editBox.GetSafeHwnd()&&pListCtrl->m_editBox.IsWindowVisible())
			return FALSE;
		int iCurCol=biCurCol;
		if(wVKey==CConfig::CFG_VK_LEFT)
		{
			if(iCurCol>0)
				iCurCol--;
		}
		else if(wVKey==CConfig::CFG_VK_RIGHT)
		{
			if(iCurCol<=CDisplayDataPage::REV_COL_COUNT)
				iCurCol++;
		}
		if(iCurCol!=pListCtrl->GetCurSubItem()&&iCurCol>=0&&iCurCol<=CDisplayDataPage::REV_COL_COUNT)
		{
			pListCtrl->SelectItem(pItem,iCurCol);
			if(pDlg->biLayoutType==3)
				pDlg->ShiftCheckColumn(iCurCol);
		}
		return TRUE;
	}
	else if(wVKey==CConfig::CFG_VK_HOME||wVKey==CConfig::CFG_VK_END)
	{
		if(pListCtrl->m_editBox.GetSafeHwnd()&&pListCtrl->m_editBox.IsWindowVisible())
			return FALSE;
		int iCurSel=pDlg->GetHeadOrTailValidItem(pListCtrl,wVKey==CConfig::CFG_VK_HOME?CDisplayDataPage::LISTCTRL_HEAD_ITEM:CDisplayDataPage::LISTCTRL_TAIL_ITEM);
		CSuperGridCtrl::CTreeItem *pNewSelItem=pListCtrl->GetTreeItem(iCurSel);
		if(pNewSelItem)
		{
			/*int iOldCurSel=pListCtrl->GetSelectedItem();
			CRect rc;
			pListCtrl->GetItemRect(pItem->GetIndex(),&rc,LVIR_BOUNDS);
			CSize offset(0,-(iOldCurSel-iCurSel)*rc.Height());
			if(wVKey==CConfig::CFG_VK_END)
			{
				CRect rcListCtrl,rcHeader;
				pListCtrl->GetClientRect(&rcListCtrl);
				pListCtrl->GetHeaderCtrl()->GetWindowRect(&rcHeader);
				int nSumRowCount=(rcListCtrl.Height()-rcHeader.Height())/rc.Height();
				if(nSumRowCount-CDisplayDataPage::HEAD_FIX_ROW_COUNT>0)
					offset.cy+=(nSumRowCount-CDisplayDataPage::HEAD_FIX_ROW_COUNT)*rc.Height();
			}
			pListCtrl->Scroll(offset);*/

			pListCtrl->SelectItem(pNewSelItem,pDlg->biCurCol);
			pDlg->Focus(pListCtrl,pNewSelItem);
		}
		return TRUE;
	}
	else if(wVKey==VK_PRIOR||wVKey==VK_NEXT)
	{
		if(pListCtrl->m_editBox.GetSafeHwnd()&&pListCtrl->m_editBox.IsWindowVisible())
			return FALSE;
		int iCurSel=pDlg->GetHeadOrTailValidItem(pListCtrl,pListCtrl->GetSelectedItem());
		CSuperGridCtrl::CTreeItem *pNewSelItem=pListCtrl->GetTreeItem(iCurSel);
		if(pNewSelItem)
		{
			/*
			int iOldCurSel=pListCtrl->GetSelectedItem();
			CRect rc;
			pListCtrl->GetItemRect(pItem->GetIndex(),&rc,LVIR_BOUNDS);
			CSize offset(0,-(iOldCurSel-iCurSel)*rc.Height());
			if(wVKey==CConfig::CFG_VK_END)
			{
				CRect rcListCtrl,rcHeader;
				pListCtrl->GetClientRect(&rcListCtrl);
				pListCtrl->GetHeaderCtrl()->GetWindowRect(&rcHeader);
				int nSumRowCount=(rcListCtrl.Height()-rcHeader.Height())/rc.Height();
				if(nSumRowCount-CDisplayDataPage::HEAD_FIX_ROW_COUNT>0)
					offset.cy+=(nSumRowCount-CDisplayDataPage::HEAD_FIX_ROW_COUNT)*rc.Height();
			}
			pListCtrl->Scroll(offset);
			*/
			
			pListCtrl->SelectItem(pNewSelItem,pDlg->biCurCol);
			pDlg->Focus(pListCtrl,pNewSelItem);
		}
		return TRUE;
	}
	else
		return FALSE;
}

void CheckBomPartByRule(IRecoginizer::BOMPART *pBomPart,CImageRegionHost *pRegion=NULL);
static BOOL FireModifyValue(CSuperGridCtrl* pListCtrl,CSuperGridCtrl::CTreeItem* pItem,int iSubItem,CString& sTextValue)
{
	CDisplayDataPage *pDlg=(CDisplayDataPage*)pListCtrl->GetParent();
	if(pDlg->m_cDisType!=CDataCmpModel::IMAGE_REGION_DATA ||pDlg->m_pData==NULL)
		return FALSE;
	if(pItem==NULL)
		return FALSE;
	if(pDlg->biLayoutType!=2&&pDlg->biLayoutType!=3&&pDlg->biLayoutType!=4)
		return FALSE;
	BOOL bChange=FALSE;
	BYTE biCurCol=pDlg->biCurCol;
	if(pDlg->biLayoutType==2)
		biCurCol=pListCtrl->GetCurSubItem();
	if(biCurCol<0)
		return FALSE;
	IRecoginizer::BOMPART *pCurPart=pItem!=NULL?(IRecoginizer::BOMPART*)pItem->m_idProp:NULL;
	if(sTextValue.CompareNoCase(".")==0)
	{
		if(biCurCol==CDisplayDataPage::COL_LABEL)
			sTextValue=pCurPart->sLabel;
		else if(biCurCol==CDisplayDataPage::COL_SPEC)
			sTextValue=pCurPart->sSizeStr;
		else if(biCurCol==CDisplayDataPage::COL_LEN)
			sTextValue.Format("%d",(int)(pCurPart->length));
		else if(biCurCol==CDisplayDataPage::COL_COUNT)
			sTextValue.Format("%d",(int)(pCurPart->count));
		else if(biCurCol==CDisplayDataPage::COL_WEIGHT)
		{
			sTextValue.Format("%.2f",pCurPart->weight);
			SimplifiedNumString(sTextValue);
		}
		else if (biCurCol == CDisplayDataPage::COL_MAP_SUM_WEIGHT)
		{
			sTextValue.Format("%.2f", pCurPart->sumWeight);
			SimplifiedNumString(sTextValue);
		}
		return TRUE;	//输入点表示重复上一次，已经在FireKeyDown中处理
	}
	if(biCurCol==CDisplayDataPage::COL_LABEL)
	{
		bChange=(stricmp(pCurPart->sLabel,sTextValue)!=0);
		StrCopy(pCurPart->sLabel,sTextValue,16);
		if(bChange)
			pCurPart->ciSrcFromMode|=IRecoginizer::BOMPART::MODIFY_LABEL;
	}
	else if(biCurCol==CDisplayDataPage::COL_SPEC)
	{
		CString sOldSpec=pCurPart->sSizeStr;
		if(sTextValue.FindOneOf("-")>=0)
		{
			if(sTextValue.FindOneOf("*")>=0)
				sTextValue.Replace('*','X');
			StrCopy(pCurPart->sSizeStr,sTextValue,16);
		}
		else if(sTextValue.FindOneOf("*")>=0)
		{
			if(sTextValue.FindOneOf("L")>=0||sTextValue.FindOneOf("-")>=0)
				sTextValue.Replace('*','X');
			//输入内容包括钢管标识符，识别为钢管 wht 19-10-23
			else if (sTextValue.FindOneOf("Φ")|| sTextValue.FindOneOf("φ"))
				sTextValue.Replace('*', 'X');
			else 
				sTextValue.Format("L%s",sTextValue);
			StrCopy(pCurPart->sSizeStr,sTextValue,16);
		}
		else //规格简化输入法
			StrCopy(pCurPart->sSizeStr,CConfig::GetAngleSpecByKey(sTextValue),16);
		//未初始化构件类型是根据规格初始化构件类型
		int thick=0,width=0,cls_id=0;
		if(pCurPart->wPartType==0)
			ParseSpec(pCurPart);
		sTextValue=pCurPart->sSizeStr;
		bChange=sTextValue.CompareNoCase(sOldSpec)!=0;
		if(bChange)
		{
			ParseSpec(pCurPart);	//根据规格初始化宽度与厚度
			pCurPart->ciSrcFromMode|=IRecoginizer::BOMPART::MODIFY_SPEC;
		}
	}
	else if(biCurCol==CDisplayDataPage::COL_LEN)
	{
		double length=pCurPart->length;
		if(sTextValue.GetLength()>0)
			pCurPart->length=atoi(sTextValue);
		bChange=(length!=pCurPart->length);
		if(bChange)
			pCurPart->ciSrcFromMode|=IRecoginizer::BOMPART::MODIFY_LENGTH;
	}
	else if(biCurCol==CDisplayDataPage::COL_COUNT)
	{
		int count=pCurPart->count;
		if(sTextValue.GetLength()>0)
			pCurPart->count=atoi(sTextValue);
		bChange=(count!=pCurPart->count);
		if(bChange)
			pCurPart->ciSrcFromMode|=IRecoginizer::BOMPART::MODIFY_COUNT;
	}
	else if(biCurCol==CDisplayDataPage::COL_WEIGHT)
	{
		double fWeight=pCurPart->weight;
		if(sTextValue.GetLength()>0)
			pCurPart->weight=atof(sTextValue);
		pCurPart->calWeight=pCurPart->weight;
		bChange=fabs(fWeight-pCurPart->weight)>EPS;
		if(bChange)
			pCurPart->ciSrcFromMode|=IRecoginizer::BOMPART::MODIFY_WEIGHT;
	}
	else if (biCurCol == CDisplayDataPage::COL_MAP_SUM_WEIGHT)
	{
		double fSumWeight = pCurPart->sumWeight;
		if (sTextValue.GetLength() > 0)
			pCurPart->sumWeight = atof(sTextValue);
		bChange = fabs(fSumWeight - pCurPart->sumWeight) > EPS;
		if (bChange)
			pCurPart->ciSrcFromMode |= IRecoginizer::BOMPART::MODIFY_MAP_SUM_WEIGHT;
	}
	CheckBomPartByRule(pCurPart);
	if(bChange)
		UpdatePartWeight(pDlg,pListCtrl,pItem,pCurPart,biCurCol!=CDisplayDataPage::COL_WEIGHT);
	return TRUE;
}

static BYTE GetOffsetState(CRect panelRect,CRect imageRect)
{	//上下左右
	BYTE byteState=0xFF;
	BYTE byteArr[4]={0x01,0x02,0x04,0x08};
	CSize offset(imageRect.left,imageRect.top);
	//判断是否能够左右移动
	if(offset.cx<0&&(imageRect.Width()-abs(offset.cx))<panelRect.Width())
		byteState&=~byteArr[2]; //不能左移
	else if(offset.cx>0)
		byteState&=~byteArr[3];	//不能右移
	//判断是否能够上下移动
	if(offset.cy<0&&(imageRect.Height()-abs(offset.cy))<panelRect.Height())
		byteState&=~byteArr[0];	//不能上移
	else if(offset.cy>0)
		byteState&=~byteArr[1];	//不能下移
	return byteState;
}
//////////////////////////////////////////////////////////////////////////
// CDisplayDdataPage 对话框
IMPLEMENT_DYNCREATE(CDisplayDataPage, CDialog)

long CDisplayDataPage::m_arrColWidth[8];
CDisplayDataPage::CDisplayDataPage(CWnd* pParent /*=NULL*/)
	: CDialog(CDisplayDataPage::IDD, pParent)
	, m_nRecordNum(0)
	, m_iModel(0)
	, m_sSumWeight(_T(""))
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
	m_fZoomCoefOfScrX=1.0;
}

CDisplayDataPage::~CDisplayDataPage()
{
	ClearImage();
}

void CDisplayDataPage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_CTRL, m_listDataCtrl);
	DDX_Control(pDX, IDC_PREV_LIST_CTRL, m_listDataCtrl1);
	DDX_Text(pDX, IDC_E_RECORD_NUM, m_nRecordNum);
	DDX_Control(pDX, IDC_SLIDER_BALANCE_COEF, m_sliderBalanceCoef);
	DDX_Control(pDX, IDC_SLIDER_ZOOM_SCALE, m_sliderZoomScale);
	DDX_Control(pDX, IDC_S_BALANCE_COEF, m_balanceCoef);
	DDX_Control(pDX, IDC_CMB_MODEL, m_cmdModel);
	DDX_CBIndex(pDX, IDC_CMB_MODEL, m_iModel);
	DDX_Control(pDX, IDC_S_SUM_WEIGHT, m_ctrlWeightLabel);
	DDX_Control(pDX, IDC_TAB_CTRL, m_tabCtrl);
	DDX_Control(pDX, IDC_FILE_LIST_CTRL, m_listFileCtrl);
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
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER_ZOOM_SCALE, &CDisplayDataPage::OnNMCustomdrawSliderZoomScale)
	ON_CBN_SELCHANGE(IDC_CMB_MODEL, &CDisplayDataPage::OnCbnSelchangeCmbModel)
	ON_WM_MOUSELEAVE()
	ON_WM_RBUTTONDOWN()
	ON_COMMAND(ID_RETRIEVE_DATA,OnRetrieveData)
	ON_COMMAND(ID_SELECT_FOLDER,OnSelectFolder)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SLIDER_BALANCE_COEF, &CDisplayDataPage::OnNMReleasedcaptureSliderBalanceCoef)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SLIDER_ZOOM_SCALE, &CDisplayDataPage::OnNMReleasedcaptureSliderZoomScale)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SLIDER_SPIN, &CDisplayDataPage::OnDeltaposSliderSpin)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_CTRL, OnSelchangeTabGroup)
	ON_COMMAND(ID_MARK_ERROR,OnMarkError)
	ON_COMMAND(ID_MARK_NORMAL,OnMarkNormal)
	ON_COMMAND(ID_ADD_PART,OnAddPart)
	ON_COMMAND(ID_DELETE_PART,OnDeletePart)
	ON_COMMAND(ID_COPY_ITEM,OnCopyItem)
	ON_COMMAND(ID_PASTE_ITEM,OnPasteItem)
END_MESSAGE_MAP()
// CDisplayDdataPage 消息处理程序
int FireCompareItem(const CSuperGridCtrl::CSuperGridCtrlItemPtr& pItem1,const CSuperGridCtrl::CSuperGridCtrlItemPtr& pItem2,DWORD lPara);
BOOL CDisplayDataPage::OnInitDialog()
{
	CDialog::OnInitDialog();
	CClientDC dc(this);
	CSize fontsize=dc.GetTextExtent("件",2);
	m_fZoomCoefOfScrX=fontsize.cx/15.0;
	CScreenAdapter::InitByCurrDC(&dc);
	((CEdit*)GetDlgItem(IDC_E_RECORD_NUM))->SetReadOnly(TRUE);
	m_xInputPartInfoPage.Create(IDD_INPUT_PARTINFO_DLG,GetDlgItem(IDC_SUB_WORK_PANEL));
	m_nInputDlgH=m_xInputPartInfoPage.GetRectHeight();
	m_ctrlWeightLabel.SetTextColor(RGB(255,0,0));
	m_ctrlWeightLabel.SetBackgroundColour(RGB(240,240,240));
	//
	m_arrColWidth[0]=f2i(m_fZoomCoefOfScrX*80);
	m_arrColWidth[1]=f2i(m_fZoomCoefOfScrX*70);
	m_arrColWidth[2]=f2i(m_fZoomCoefOfScrX*120);
	m_arrColWidth[3]=f2i(m_fZoomCoefOfScrX*80);
	m_arrColWidth[4]=f2i(m_fZoomCoefOfScrX*60);
	m_arrColWidth[5]=f2i(m_fZoomCoefOfScrX*90);
	m_arrColWidth[6]=f2i(m_fZoomCoefOfScrX*110);
	m_arrColWidth[7]=f2i(m_fZoomCoefOfScrX*110);
	long colFmtArr[8]={LVCFMT_LEFT,LVCFMT_LEFT,LVCFMT_LEFT,LVCFMT_RIGHT,LVCFMT_CENTER,LVCFMT_RIGHT,LVCFMT_RIGHT,LVCFMT_RIGHT};
	CXhChar100 colNameArr[COL_SUM_COUNT]={"件号","材质","规格","长度","数量","单重","总重","图纸总重"};
	m_listDataCtrl.EmptyColumnHeader();
	m_listDataCtrl1.EmptyColumnHeader();
	for(int i=0;i<COL_SUM_COUNT;i++)
	{
		m_listDataCtrl.AddColumnHeader(colNameArr[i],m_arrColWidth[i]);	
		m_listDataCtrl1.AddColumnHeader(colNameArr[i],m_arrColWidth[i]);
	}
	m_listDataCtrl.SetHeaderFontHW(18,0);
	m_listDataCtrl.SetGridLineColor(RGB(220,220,220));
	m_listDataCtrl.SetEvenRowBackColor(RGB(224,237,236));
	m_listDataCtrl.SetTextVerticalAligment(DT_BOTTOM);
	m_listDataCtrl.InitListCtrl(m_arrColWidth,TRUE,colFmtArr);
	m_listDataCtrl.EnableSortItems(FALSE,TRUE);
	m_listDataCtrl.SetDeleteItemFunc(FireDeleteItem);
	m_listDataCtrl.SetItemChangedFunc(FireItemChanged);
	m_listDataCtrl.SetCompareItemFunc(FireCompareItem);
	m_listDataCtrl.SetMouseWheelFunc(FireMouseWheel);
	m_listDataCtrl.SetKeyDownItemFunc(FireKeyDownItem);
	m_listDataCtrl.SetModifyValueFunc(FireModifyValue);
	m_listDataCtrl.SetScrollFunc(FireScroll);
	m_listDataCtrl.SetColumnClickFunc(FireColumnClick);
	m_listDataCtrl.SetLButtonDownFunc(FireLButtonDown);
	m_listDataCtrl.SetContextMenuFunc(FireContextMenu);
	//m_listDataCtrl.SetKeyUpItemFunc(FireKeyUpItem);
	m_listDataCtrl.SetEndtrackFunc(FireEndtrack);
	m_listDataCtrl.SetEditBoxKeyUpItemFunc(FireEditBoxKeyUpItem);
	//
	m_listDataCtrl1.SetHeaderFontHW(18,0);
	m_listDataCtrl1.SetGridLineColor(RGB(220,220,220));
	m_listDataCtrl1.SetEvenRowBackColor(RGB(224,237,236));
	m_listDataCtrl1.SetTextVerticalAligment(DT_BOTTOM);
	m_listDataCtrl1.InitListCtrl(m_arrColWidth,TRUE,colFmtArr);
	m_listDataCtrl1.EnableSortItems(FALSE,TRUE);
	m_listDataCtrl1.SetDeleteItemFunc(FireDeleteItem);
	m_listDataCtrl1.SetItemChangedFunc(FireItemChanged);
	//m_listDataCtrl1.SetCompareItemFunc(FireCompareItem);
	m_listDataCtrl1.SetMouseWheelFunc(FireMouseWheel);
	//m_listDataCtrl1.SetKeyDownItemFunc(FireKeyDownItem);
	m_listDataCtrl1.SetScrollFunc(FireScroll);
	m_listDataCtrl1.SetColumnClickFunc(FireColumnClick);
	m_listDataCtrl1.SetLButtonDownFunc(FireLButtonDown);
	m_listDataCtrl1.SetEndtrackFunc(FireEndtrack);
	//关闭m_listDataCtrl输入法
	m_listDataCtrl.m_bCloseEditBoxImm=TRUE;
	if (m_listDataCtrl.GetSafeHwnd())
	{  
		HIMC hImc = ImmGetContext(m_listDataCtrl.GetSafeHwnd());  
		if (hImc)  
		{
			ImmAssociateContext(m_listDataCtrl.GetSafeHwnd(), NULL);  
			ImmReleaseContext(m_listDataCtrl.GetSafeHwnd(), hImc);  
		}
	} 
	//
	m_listFileCtrl.EmptyColumnHeader();
	m_listFileCtrl.SetGridLineColor(RGB(220,220,220));
	m_listFileCtrl.SetEvenRowBackColor(RGB(224,237,236));
	m_listFileCtrl.AddColumnHeader("文件",f2i(m_fZoomCoefOfScrX*150));	
	m_listFileCtrl.AddColumnHeader("路径",f2i(m_fZoomCoefOfScrX*700));
	m_listFileCtrl.InitListCtrl(NULL,FALSE);
	m_listFileCtrl.EnableSortItems(FALSE,TRUE);
	m_listFileCtrl.SetContextMenuFunc(FireContextMenu);
	//
	m_tabCtrl.DeleteAllItems();
	m_tabCtrl.InsertItem(0,"文件");
	m_tabCtrl.InsertItem(1,"构件");
	m_tabCtrl.SetCurFocus(1);
	//加载鼠标箭头图标
	GetClientRect(&m_rcClient);
	m_hCursor=NULL;
	m_hArrowCursor=LoadCursorA(NULL,IDC_ARROW);	//不知为什么AfxGetApp()->LoadCursorA(IDC_ARROW)会调用失败
	m_hPin=AfxGetApp()->LoadCursorA(IDC_CUR_PIN);
	m_hHorzSize=AfxGetApp()->LoadStandardCursor(IDC_SIZENS);
	m_hSnap=AfxGetApp()->LoadStandardCursor(IDC_SIZEALL);
	m_hMove=AfxGetApp()->LoadStandardCursor(IDC_HAND);
	m_hCross=AfxGetApp()->LoadStandardCursor(IDC_CROSS);
	//
	m_sliderBalanceCoef.SetRange(0,100);
	m_sliderBalanceCoef.SetPos(50);
	m_balanceCoef.SetWindowText("明50");
	m_sliderZoomScale.SetRange(100,1100);
	m_sliderZoomScale.SetPos(200);
	while(m_cmdModel.GetCount()>0)
		m_cmdModel.DeleteString(0);
	m_cmdModel.AddString("查看模式");
	m_cmdModel.AddString("按列校审");
	//m_cmdModel.AddString("按行校审");
	m_cmdModel.SetCurSel(0);
	//
	CWnd *pPromptWnd=GetDlgItem(IDC_S_PROMPT);
	if(pPromptWnd)
		pPromptWnd->SetWindowText(CConfig::GetShortcutPromptStr());
	RelayoutWnd();
	return TRUE;
}
BOOL CDisplayDataPage::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_KEYDOWN)
	{
		if(m_xPolyline.m_iActivePt<0&&CConfig::IsInputKey(pMsg->wParam))
		{
			CWnd *pFocusWnd=GetFocus();
			if(pFocusWnd&&pFocusWnd==&m_listDataCtrl)
				return CDialog::PreTranslateMessage(pMsg);
			else
			{
				int iItem=m_listDataCtrl.GetSelectedItem();
				int iCol=m_listDataCtrl.GetCurSubItem();
				if(!m_listDataCtrl.IsDisplayCellCtrl(iItem,iCol))
				{
					if(pFocusWnd==NULL&&pFocusWnd!=&m_listDataCtrl&&pFocusWnd->GetParent()!=&m_listDataCtrl)
						m_listDataCtrl.SetFocus();
				}
				//pMsg->hwnd=m_listDataCtrl.GetSafeHwnd();
				//::PostMessage(m_listDataCtrl.GetSafeHwnd(),pMsg->message,pMsg->wParam,pMsg->lParam);
				m_listDataCtrl.SendMessage(pMsg->message,pMsg->wParam,pMsg->lParam);
			}
		}
		else
			SendMessage(pMsg->message,pMsg->wParam,pMsg->lParam);
		return 0;
	}
	else
		return CDialog::PreTranslateMessage(pMsg);
}
void CDisplayDataPage::HideInputCtrl()
{
	m_listDataCtrl.HideInputCtrl();
}
bool CDisplayDataPage::Focus(CSuperGridCtrl *pListCtrl,CSuperGridCtrl::CTreeItem* pItem)
{
	CWnd *pWorkPanel=GetDlgItem(IDC_WORK_PANEL);
	if(pWorkPanel==NULL||m_cDisType!=CDataCmpModel::IMAGE_REGION_DATA)
		return false;
	int iCurSel=pListCtrl->GetSelectedItem();
	CImageRegionHost* pRegion=(CImageRegionHost*)m_pData;
	if(pListCtrl->GetSelectedItem()>=0)
	{
		pItem=pListCtrl->GetTreeItem(pListCtrl->GetSelectedItem());
		if(pItem->m_idProp==NULL)
			pItem=NULL;	//选中行无对应构件，自动选中有效行
	}
	if(pItem==NULL&&pListCtrl->GetItemCount()>0)
	{
		int iTopIndex=GetHeadOrTailValidItem(pListCtrl,pListCtrl->GetTopIndex());
		pItem=pListCtrl->GetTreeItem(iTopIndex>=0?iTopIndex:0);
		iCurSel=pItem?pItem->GetIndex():-1;
	}
	if(pItem==NULL||pRegion==NULL)
		return false;
	CRect rc;
	RECT rcPanel;
	pWorkPanel->GetClientRect(&rcPanel);
	IRecoginizer::BOMPART* pBomPart=(IRecoginizer::BOMPART*)pItem->m_idProp;
	if(pBomPart&&!pRegion->GetBomPartRect(pBomPart->id,&rc))
		rc=pBomPart->rc;
	if(pBomPart&&pBomPart->bInput)
		return false;
	CPoint topLeft =m_xCurrTrans.TransToGlobal(CPoint(rc.left,rc.top));
	CPoint btmRight=m_xCurrTrans.TransToGlobal(CPoint(rc.right,rc.bottom));
	CSize xOffset=pRegion->GetScrOffset();
	if(m_biLayoutType==3||m_biLayoutType==2)
	{	//按列查看
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
		
		CRect rect;
		pListCtrl->GetItemRect(iCurSel,&rect,LVIR_BOUNDS);
		double fPictureMidY=(topLeft.y+btmRight.y)*0.5;
		double fListMidY=(rect.top+rect.bottom)*0.5;
		xOffset.cy+=(int)(fListMidY-fPictureMidY);
		CRect rcHeader;
		pListCtrl->GetHeaderCtrl()->GetWindowRect(&rcHeader);
		xOffset.cy-=rcHeader.Height();
		//调整第一个列表与第二个列表同步
		if(m_biCurCol>0)
		{
			int pos=pListCtrl->GetScrollPos(SB_VERT);
			int otherPos=pOtherListCtrl->GetScrollPos(SB_VERT);
			int wParam=0,lParam=0;
			lParam = (pos - otherPos) * rect.Height();
			int iOldCurSel=pOtherListCtrl->GetSelectedItem();
			pOtherListCtrl->SendMessage(LVM_SCROLL, wParam, lParam);
			int iTopIndex=pListCtrl->GetTopIndex(),iBottomIndex=iTopIndex;
			if(pListCtrl->GetItemCount()>0)
			{
				CRect rcItem,listCtrlRect;
				pListCtrl->GetItemRect(0, rcItem, LVIR_BOUNDS);
				pListCtrl->GetWindowRect(listCtrlRect);
				int nItemCount=listCtrlRect.Height()/rcItem.Height();
				iBottomIndex+=nItemCount;
			}
			if(iCurSel>=0&&iOldCurSel!=iCurSel&&iCurSel>=iTopIndex&&iCurSel<=iBottomIndex)
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
	m_xCurrTrans.xOrg.y=xOffset.cy;
	Invalidate(FALSE);
	return true;
}
CSuperGridCtrl::CTreeItem* CDisplayDataPage::RefreshListCtrl(long idSelProp/*=0*/)
{
	m_listDataCtrl.DeleteAllItems();
	m_listDataCtrl1.DeleteAllItems();
	m_listDataCtrl.SetDblclkDisplayCellCtrl(CConfig::m_ciEditModel==1);
	GetDlgItem(IDC_S_SUM_WEIGHT)->SetWindowText("");
	GetDlgItem(IDC_S_SUM_WEIGHT_TITLE)->SetWindowText("总  计:");
	if(m_cDisType==CDataCmpModel::DWG_DATA_MODEL)
	{
		m_nRecordNum=0;
		CHashStrList<IRecoginizer::BOMPART*> dwgPartSet;
		dataModel.SummarizeDwgParts(dwgPartSet);
		for(IRecoginizer::BOMPART** ppBomPart=dwgPartSet.GetFirst();ppBomPart;ppBomPart=dwgPartSet.GetNext())
		{
			InsertOrUpdatePartTreeItem(m_listDataCtrl,*ppBomPart);
			m_nRecordNum++;
		}
		m_listDataCtrl.Redraw();
	}
	else if(m_cDisType==CDataCmpModel::LOFT_DATA_MODEL)
	{
		m_nRecordNum=0;
		CHashStrList<IRecoginizer::BOMPART*> loftPartSet;
		dataModel.SummarizeLoftParts(loftPartSet);
		for(IRecoginizer::BOMPART** ppBomPart=loftPartSet.GetFirst();ppBomPart;ppBomPart=loftPartSet.GetNext())
		{
			InsertOrUpdatePartTreeItem(m_listDataCtrl,*ppBomPart);
			m_nRecordNum++;
		}
		m_listDataCtrl.Redraw();
	}
	else if(m_cDisType==CDataCmpModel::DWG_IMAGE_GROUP)
	{
		m_nRecordNum=0;
		double fSumWeight=0,fCalSumWeight=0;
		m_listFileCtrl.DeleteAllItems();
		for(CImageFileHost *pImageFile=dataModel.EnumFirstImage();pImageFile;pImageFile=dataModel.EnumNextImage())
		{
			CListCtrlItemInfo *lpInfo=new CListCtrlItemInfo();
			lpInfo->SetSubItemText(0,pImageFile->szFileName,TRUE);
			lpInfo->SetSubItemText(1,pImageFile->szPathFileName,TRUE);
			CSuperGridCtrl::CTreeItem *pItem=m_listFileCtrl.InsertRootItem(lpInfo);
			pItem->m_idProp=(long)pImageFile;
			//
			pImageFile->SummarizePartInfo();
			for(IRecoginizer::BOMPART* pBomPart=pImageFile->EnumFirstPart();pBomPart;pBomPart=pImageFile->EnumNextPart())
			{
				InsertOrUpdatePartTreeItem(m_listDataCtrl,pBomPart);
				if(CConfig::m_bRecogSumWeight)
				{
					fSumWeight+=atof(CXhChar50("%.1f",pBomPart->sumWeight));
					fCalSumWeight+=atof(CXhChar50("%.1f",pBomPart->calSumWeight));
				}
				else
					fSumWeight+=atof(CXhChar50("%.1f",pBomPart->calSumWeight));
				m_nRecordNum++;
			}
		}
		//在最后一行插入总重
		CListCtrlItemInfo *lpInfo=new CListCtrlItemInfo();
		lpInfo->SetSubItemText(5,"总计:",TRUE);
		if(CConfig::m_bRecogSumWeight)
		{
			lpInfo->SetSubItemText(6,CXhChar50("%.1f",fCalSumWeight),TRUE);
			lpInfo->SetSubItemText(7,CXhChar50("%.1f",fSumWeight),TRUE);
			GetDlgItem(IDC_S_SUM_WEIGHT)->SetWindowText(CXhChar200("%.1f | %.1f",fCalSumWeight,fSumWeight));
		}
		else
		{
			lpInfo->SetSubItemText(6,CXhChar50("%.1f",fSumWeight),TRUE);
			GetDlgItem(IDC_S_SUM_WEIGHT)->SetWindowText(CXhChar50("%.1f",fSumWeight));
		}
		CSuperGridCtrl::CTreeItem *pItem=m_listDataCtrl.InsertRootItem(lpInfo,FALSE);
		pItem->m_idProp=0;
		m_listDataCtrl.Redraw();
	}
	else if(m_cDisType==CDataCmpModel::DWG_BOM_GROUP)
	{
		m_nRecordNum=0;
		for(CBomFileData* pBomFile=dataModel.EnumFirstDwgBom();pBomFile;pBomFile=dataModel.EnumNextDwgBom())
		{
			for(IRecoginizer::BOMPART* pBomPart=pBomFile->EnumFirstPart();pBomPart;pBomPart=pBomFile->EnumNextPart())
			{
				InsertOrUpdatePartTreeItem(m_listDataCtrl,pBomPart);
				m_nRecordNum++;
			}
		}
		m_listDataCtrl.Redraw();
	}
	else if(m_cDisType==CDataCmpModel::LOFT_BOM_GROUP)
	{
		m_nRecordNum=0;
		for(CBomFileData* pBomFile=dataModel.EnumFirstLoftBom();pBomFile;pBomFile=dataModel.EnumNextLoftBom())
		{
			for(IRecoginizer::BOMPART* pBomPart=pBomFile->EnumFirstPart();pBomPart;pBomPart=pBomFile->EnumNextPart())
			{
				InsertOrUpdatePartTreeItem(m_listDataCtrl,pBomPart);
				m_nRecordNum++;
			}
		}
		m_listDataCtrl.Redraw();
	}
	else if(m_cDisType==CDataCmpModel::LOFT_BOM_DATA && m_pData)
	{	//放样Bom文件明细
		CBomFileData* pLoftBomFile=(CBomFileData*)m_pData;
		m_nRecordNum=0;
		for(IRecoginizer::BOMPART* pBomPart=pLoftBomFile->EnumFirstPart();pBomPart;pBomPart=pLoftBomFile->EnumNextPart())
		{
			InsertOrUpdatePartTreeItem(m_listDataCtrl,pBomPart);
			m_nRecordNum++;
		}
		m_listDataCtrl.Redraw();
	}
	else if(m_cDisType==CDataCmpModel::IMAGE_SEG_GROUP && m_pData)
	{	//图纸分段数据
		SEGITEM* pSegItem=(SEGITEM*)m_pData;
		m_nRecordNum=0;
		double fSumWeight=0,fCalSumWeight=0;
		for(CImageFileHost *pImageFile=dataModel.EnumFirstImage(pSegItem->iSeg);pImageFile;pImageFile=dataModel.EnumNextImage(pSegItem->iSeg))
		{
			pImageFile->SummarizePartInfo();
			for(IRecoginizer::BOMPART* pBomPart=pImageFile->EnumFirstPart();pBomPart;pBomPart=pImageFile->EnumNextPart())
			{
				InsertOrUpdatePartTreeItem(m_listDataCtrl,pBomPart);
				if(CConfig::m_bRecogSumWeight)
				{
					fSumWeight+=atof(CXhChar50("%.1f",pBomPart->sumWeight));
					fCalSumWeight+=atof(CXhChar50("%.1f",pBomPart->calSumWeight));
				}
				else
					fSumWeight+=atof(CXhChar50("%.1f",pBomPart->calSumWeight));
				m_nRecordNum++;
			}
		}
		//在最后一行插入总重
		CListCtrlItemInfo *lpInfo=new CListCtrlItemInfo();
		lpInfo->SetSubItemText(5,"总计:",TRUE);
		if(CConfig::m_bRecogSumWeight)
		{
			lpInfo->SetSubItemText(6,CXhChar50("%.1f",fCalSumWeight),TRUE);
			lpInfo->SetSubItemText(7,CXhChar50("%.1f",fSumWeight),TRUE);
			GetDlgItem(IDC_S_SUM_WEIGHT)->SetWindowText(CXhChar200("%.1f | %.1f",fCalSumWeight,fSumWeight));
		}
		else
		{
			lpInfo->SetSubItemText(6,CXhChar50("%.1f",fSumWeight),TRUE);
			GetDlgItem(IDC_S_SUM_WEIGHT)->SetWindowText(CXhChar50("%.1f",fSumWeight));
		}
		CSuperGridCtrl::CTreeItem *pItem=m_listDataCtrl.InsertRootItem(lpInfo,FALSE);
		pItem->m_idProp=0;
		m_listDataCtrl.Redraw();
	}
	else if(m_cDisType==CDataCmpModel::DWG_BOM_DATA && m_pData)
	{	//图纸-BOM文件明细
		CBomFileData* pBomFile=(CBomFileData*)m_pData;
		m_nRecordNum=0;
		for(IRecoginizer::BOMPART* pBomPart=pBomFile->EnumFirstPart();pBomPart;pBomPart=pBomFile->EnumNextPart())
		{
			InsertOrUpdatePartTreeItem(m_listDataCtrl,pBomPart);
			m_nRecordNum++;
		}
		m_listDataCtrl.Redraw();
	}
	else if(m_cDisType==CDataCmpModel::DWG_IMAGE_DATA && m_pData)
	{	//显示选中图片的数据
		CImageFileHost* pImageHost=(CImageFileHost*)m_pData;
		m_nRecordNum=0;
		double fSumWeight=0,fCalSumWeight=0;
		pImageHost->SummarizePartInfo();
		for(IRecoginizer::BOMPART* pBomPart=pImageHost->EnumFirstPart();pBomPart;pBomPart=pImageHost->EnumNextPart())
		{
			InsertOrUpdatePartTreeItem(m_listDataCtrl,pBomPart);
			if(CConfig::m_bRecogSumWeight)
			{
				fSumWeight+=atof(CXhChar50("%.1f",pBomPart->sumWeight));
				fCalSumWeight+=atof(CXhChar50("%.1f",pBomPart->calSumWeight));
			}
			else
				fSumWeight+=atof(CXhChar50("%.1f",pBomPart->calSumWeight));
			m_nRecordNum++;
		}
		//计算分段总重
		int nImageFile=0;
		double fSegSumWeight=0;
		SEGI segI=pImageHost->m_iSeg;
		for(CImageFileHost *pImageFile=dataModel.EnumFirstImage(segI);pImageFile;pImageFile=dataModel.EnumNextImage(segI))
		{
			nImageFile++;
			pImageFile->SummarizePartInfo();
			for(IRecoginizer::BOMPART* pBomPart=pImageFile->EnumFirstPart();pBomPart;pBomPart=pImageFile->EnumNextPart())
			{
				if(CConfig::m_bRecogSumWeight)
					fSegSumWeight+=atof(CXhChar50("%.1f",pBomPart->sumWeight));
				else
					fSegSumWeight+=atof(CXhChar50("%.1f",pBomPart->calSumWeight));
			}
		}
		if(nImageFile>1)
			GetDlgItem(IDC_S_SUM_WEIGHT_TITLE)->SetWindowText(CXhChar50("分段总计:%.1f  总计:",fSegSumWeight));
		//在最后一行插入总重
		CListCtrlItemInfo *lpInfo=new CListCtrlItemInfo();
		lpInfo->SetSubItemText(5,"总计:",TRUE);
		if(CConfig::m_bRecogSumWeight)
		{
			lpInfo->SetSubItemText(6,CXhChar50("%.1f",fCalSumWeight),TRUE);
			lpInfo->SetSubItemText(7,CXhChar50("%.1f",fSumWeight),TRUE);
			GetDlgItem(IDC_S_SUM_WEIGHT)->SetWindowText(CXhChar200("%.1f | %.1f",fCalSumWeight,fSumWeight));
		}
		else
		{
			lpInfo->SetSubItemText(6,CXhChar50("%.1f",fSumWeight),TRUE);
			GetDlgItem(IDC_S_SUM_WEIGHT)->SetWindowText(CXhChar50("%.1f",fSumWeight));
		}
		CSuperGridCtrl::CTreeItem *pItem=m_listDataCtrl.InsertRootItem(lpInfo,FALSE);
		pItem->m_idProp=0;
		m_listDataCtrl.Redraw();
	}
	else if(m_cDisType==CDataCmpModel::IMAGE_REGION_DATA && m_pData)
	{	//显示单个区域提取结果
		CImageRegionHost *pRegion=(CImageRegionHost*)m_pData;
		if(m_biLayoutType==3)
		{	//插入空白行,
			m_listDataCtrl.SetImmerseMode(CConfig::m_bImmerseMode);
			m_listDataCtrl1.SetImmerseMode(CConfig::m_bImmerseMode);
			for(int i=0;i<GetHeadFixRowCount();i++)
			{
				InsertOrUpdatePartTreeItem(m_listDataCtrl,NULL);
				InsertOrUpdatePartTreeItem(m_listDataCtrl1,NULL);
			}
		}
		else 
		{
			m_listDataCtrl.SetImmerseMode(FALSE);
			m_listDataCtrl1.SetImmerseMode(FALSE);
		}
		m_nRecordNum=0;
		double fSumWeight=0,fCalSumWeight=0;
		pRegion->SyncInputPartToSummaryParts();
		for(IRecoginizer::BOMPART* pBomPart=pRegion->EnumFirstPart();pBomPart;pBomPart=pRegion->EnumNextPart())
		{
			CalAndSyncTheoryWeightToWeight(pBomPart,FALSE);
			CSuperGridCtrl::CTreeItem *pItem=InsertOrUpdatePartTreeItem(m_listDataCtrl,pBomPart);
			for(int i=0;i<pItem->m_lpNodeInfo->GetItemCount();i++)
			{
				if((i==CDisplayDataPage::COL_LABEL||
					i==CDisplayDataPage::COL_SPEC||
					i==CDisplayDataPage::COL_LEN||
					i==CDisplayDataPage::COL_COUNT||
					i==CDisplayDataPage::COL_WEIGHT||
					i==CDisplayDataPage::COL_MAP_SUM_WEIGHT))
					pItem->m_lpNodeInfo->SetSubItemReadOnly(i,FALSE);
			}
			InsertOrUpdatePartTreeItem(m_listDataCtrl1,pBomPart);
			if(CConfig::m_bRecogSumWeight)
			{
				fSumWeight+=atof(CXhChar50("%.1f",pBomPart->sumWeight));
				fCalSumWeight+=atof(CXhChar50("%.1f",pBomPart->calSumWeight));
			}
			else
				fSumWeight+=atof(CXhChar50("%.1f",pBomPart->calSumWeight));
			m_nRecordNum++;
		}
		if(m_biLayoutType==3)
		{	//最后插入50个空白行,方便将校审区固定在屏幕中央
			for(int i=0;i<GetTailFixRowCount();i++)
			{
				InsertOrUpdatePartTreeItem(m_listDataCtrl,NULL);
				InsertOrUpdatePartTreeItem(m_listDataCtrl1,NULL);
			}
		}
		m_listDataCtrl.Redraw();
		m_listDataCtrl1.Redraw();
		CScrollBar *pVertBar=m_listDataCtrl1.GetScrollBarCtrl(SB_VERT);
		CScrollBar *pHorzBar=m_listDataCtrl1.GetScrollBarCtrl(SB_HORZ);
		if(pVertBar)
			pVertBar->ShowWindow(SW_HIDE);
		if(pHorzBar)
			pHorzBar->ShowWindow(SW_HIDE);
		//更新总重量
		if(CConfig::m_bRecogSumWeight)
			GetDlgItem(IDC_S_SUM_WEIGHT)->SetWindowText(CXhChar200("%.1f | %.1f",fCalSumWeight,fSumWeight));
		else
			GetDlgItem(IDC_S_SUM_WEIGHT)->SetWindowText(CXhChar50("%.1f",fSumWeight));
		//自动调整列宽,调整之后更新列宽数组
		/*int ignoreColArr[1]={7};
		m_listDataCtrl.AutoAdjustColumnWidth(ignoreColArr,1);
		for(int i=0;i<COL_SUM_COUNT;i++)
			m_arrColWidth[i]=m_listDataCtrl.GetColumnWidth(i);*/
	}
	//设置选中项
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
	return FALSE;
}
void CDisplayDataPage::InitPage(BYTE cType,DWORD dwRefData)
{
	m_cDisType=cType;
	m_pData=dwRefData;
	CImageRegionHost *pRegion=NULL;
	if(cType==CDataCmpModel::DWG_IMAGE_DATA)
	{
		m_biLayoutType=1;
		CIBOMDoc *pDoc=theApp.GetDocument();
		CImageFileHost* pImageFile=(CImageFileHost*)m_pData;
		if(pDoc!=NULL&&pImageFile!=NULL&&pImageFile->Height==0)
		{
			CWaitCursor waitCursor;
			pImageFile->InitImageFile(pImageFile->szPathFileName,pDoc->GetFilePath());
			CRect rcPanelClient;
			GetWorkPanelRect(rcPanelClient);
			pImageFile->InitImageShowPara(rcPanelClient);
		}
	}
	else if(cType==CDataCmpModel::IMAGE_REGION_DATA)
	{
		if(m_iModel==1)
			m_biLayoutType=3;
		else if(m_iModel==2)
			m_biLayoutType=4; 
		else 
			m_biLayoutType=2;
		pRegion=(CImageRegionHost*)m_pData;
	}
	else
		m_biLayoutType=0;
	if(pActiveImgFile)
	{
		if(pActiveImgFile->m_xPolyline.m_cState==POLYLINE_4PT::STATE_VALID)
			m_xPolyline=pActiveImgFile->m_xPolyline;
		/*if(m_xPolyline.m_cState==POLYLINE_4PT::STATE_WAITNEXTPT)
		{
			if(m_xPolyline.m_ptArr[0].x<m_xPolyline.m_ptArr[2].x)
				m_xPolyline.m_cState=POLYLINE_4PT::STATE_VALID;
			else
				m_xPolyline.m_cState=POLYLINE_4PT::STATE_NONE;
		}*/
	}
	IRecoginizer::BOMPART *pBomPart=pRegion?pRegion->EnumFirstPart():NULL;
	CSuperGridCtrl::CTreeItem *pSelItem=RefreshListCtrl((long)pBomPart);
	RelayoutWnd();
	Focus(&m_listDataCtrl,pSelItem);
	Invalidate(TRUE);
}
//绘制图片
int CDisplayDataPage::StretchBlt(HDC hDC,CImageFileHost* pImageFile)
{
	CDC *pDC=CDC::FromHandle(hDC);
	if(pDC==NULL||pImageFile==NULL)
		return 0;
	int width=pImageFile->Width;
	int height=pImageFile->Height;
	if(pImageFile->Height==0)
	{
		CIBOMDoc *pDoc=theApp.GetDocument();
		if(pDoc)
		{
			pImageFile->InitImageFile(pImageFile->szPathFileName,pDoc->GetFilePath());
			height=pImageFile->Height;
		}
	}
	int extrabytes=(4-(width * 3)%4)%4;
	int bytesize=(width*3+extrabytes)*height;
	if( m_xCurrImageData.imagedata!=NULL&&(m_xCurrImageData.dwRelaObj!=(DWORD)pImageFile||
		m_xCurrImageData.ciRelaObjType!=CDataCmpModel::DWG_IMAGE_DATA))
	{
		delete[]m_xCurrImageData.imagedata;
		m_xCurrImageData.imagedata=NULL;
	}
	if(m_xCurrImageData.imagedata==NULL)
	{
		m_xCurrImageData.dwRelaObj=(DWORD)pImageFile;
		m_xCurrImageData.ciRelaObjType=CDataCmpModel::DWG_IMAGE_DATA;
		pImageFile->Get24BitsImageData(&m_xCurrImageData);
		m_xCurrImageData.imagedata=new char[m_xCurrImageData.nEffWidth*m_xCurrImageData.nHeight];
		pImageFile->Get24BitsImageData(&m_xCurrImageData);
		if (m_xCurrImageData.nHeight == width && m_xCurrImageData.nWidth == height)
		{	//长宽翻转，需要重新设置widht、height、bytesize、extrabytes wht 19-11-30
			width = m_xCurrImageData.nWidth;
			height = m_xCurrImageData.nHeight;
			extrabytes = (4 - (width * 3) % 4) % 4;
			bytesize = (width * 3 + extrabytes)*height;
		}
		//IMAGE_FILE_DATA中的图像行扫描模式为自上至下downward，应转为StretchDIBits所需的upward模式
		int imagebytes_size=m_xCurrImageData.nEffWidth*m_xCurrImageData.nHeight;
		CHAR_ARRAY imagebits(imagebytes_size);
		for(int i=0;i<height;i++)
		{
			int rowi=height-1-i;
			memcpy(&imagebits[rowi*m_xCurrImageData.nEffWidth],&m_xCurrImageData.imagedata[i*m_xCurrImageData.nEffWidth],m_xCurrImageData.nEffWidth);
		}
		memcpy(m_xCurrImageData.imagedata,imagebits,imagebytes_size);
	}
	CRect imageShowRect=pImageFile->MappingToScrRect();
	//位图头信息
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
	int oldmode=SetStretchBltMode(hDC,HALFTONE);
	::StretchDIBits( hDC,imageShowRect.left,imageShowRect.top,imageShowRect.Width(),imageShowRect.Height(),
		0,0,pImageFile->GetWidth(),pImageFile->GetHeight(),m_xCurrImageData.imagedata,(LPBITMAPINFO)&bmpInfoHeader,DIB_RGB_COLORS,SRCCOPY);
	SetStretchBltMode(hDC,oldmode);
	return 1;
}

void LazyLoadImageRegion(CImageRegionHost *pRegion)
{
	if(pRegion==NULL||pRegion->Height>0)
		return;
	CImageFileHost *pImageFile=pRegion->BelongImageFile();
	if(pImageFile==NULL)
		return;
	if(pImageFile->Height==0)
	{
		CIBOMDoc *pDoc=theApp.GetDocument();
		if(pDoc)
			pImageFile->InitImageFile(pImageFile->szPathFileName,pDoc->GetFilePath());
	}
	pRegion->SetRegion(pRegion->TopLeft(),pRegion->BtmLeft(),pRegion->BtmRight(),pRegion->TopRight());
}

int CDisplayDataPage::StretchBlt(CDC* pDC,CImageRegionHost *pRegion,RECT& rcPanel,PIXEL_TRANS* pPixelTranslator/*=NULL*/,BOOL bSubWorkPanel/*=FALSE*/)
{
	if(pDC==NULL||pRegion==NULL)
		return 0;
	CImageFileHost *pImageFile=pRegion->BelongImageFile();
	if(pImageFile==NULL)
		return 0;
	LazyLoadImageRegion(pRegion);

	SIZE xOrgOffset=pRegion->GetScrOffset();
	CRect rcImgClip(0,0,0,pRegion->Height);
	int nColCount=pRegion->GetImageOrgColumnWidth();
	if(nColCount==0)
		rcImgClip.right=pRegion->Width;
	if(nColCount>0&&(m_biLayoutType==LAYOUT_BROWSE||m_biLayoutType==LAYOUT_COLREV))
	{	
		DYN_ARRAY<int> arrColWidth(nColCount);
		pRegion->GetImageOrgColumnWidth(arrColWidth);
		if(m_biLayoutType==LAYOUT_BROWSE)
		{
			if(bSubWorkPanel)
			{
				if(nColCount>5)
				{
					rcImgClip.left=arrColWidth[0]+arrColWidth[1]+arrColWidth[2]+arrColWidth[3]+arrColWidth[4];
					rcImgClip.right=rcImgClip.left+arrColWidth[5];
				}
			}
			else
			{
				int nMaxColCount=min(nColCount,REV_COL_COUNT);
				for(int i=0;i<nMaxColCount;i++)
					rcImgClip.right+=arrColWidth[i];//显示前4列
			}
		}
		else if(m_biLayoutType==LAYOUT_COLREV)
		{	//截取列宽
			if(m_biCurCol==COL_LABEL)
				rcImgClip.right=arrColWidth[0];
			else if(m_biCurCol==COL_MATERIAL||m_biCurCol==COL_SPEC)
			{
				rcImgClip.left=arrColWidth[0];
				rcImgClip.right=rcImgClip.left+arrColWidth[1];
			}
			else if(m_biCurCol==COL_LEN&&nColCount>2)
			{
				rcImgClip.left=arrColWidth[0]+arrColWidth[1];
				rcImgClip.right=rcImgClip.left+arrColWidth[2];
			}
			else if(m_biCurCol==COL_COUNT&&nColCount>3)
			{
				rcImgClip.left=arrColWidth[0]+arrColWidth[1]+arrColWidth[2];
				rcImgClip.right=rcImgClip.left+arrColWidth[3];
			}
			else if(m_biCurCol==COL_WEIGHT&&nColCount>4)
			{
				rcImgClip.left=arrColWidth[0]+arrColWidth[1]+arrColWidth[2]+arrColWidth[3];
				rcImgClip.right=rcImgClip.left+arrColWidth[4];
			}
		}
	}
	if(rcImgClip.Width()<1||rcImgClip.Height()<1)
		return 0;
	CBitmap bmp;
	CDC dcMemory;
	IMAGE_DATA_EX *pCurImage=NULL;
	//从原始区域图像中提取指定列范围的矩形区域图像数据
	if(bSubWorkPanel)
	{
		pCurImage=&m_xCurrImageData2;
		if( m_xCurrImageData2.imagedata!=NULL&&(m_xCurrImageData2.dwRelaObj!=(DWORD)pRegion||
			m_xCurrImageData2.ciRelaObjType!=CDataCmpModel::IMAGE_REGION_DATA))
		{
			delete[]m_xCurrImageData2.imagedata;
			m_xCurrImageData2.imagedata=NULL;
		}
		if(m_xCurrImageData2.imagedata==NULL)
		{
			m_xCurrImageData2.dwRelaObj=(DWORD)pRegion;
			m_xCurrImageData2.ciRelaObjType=CDataCmpModel::IMAGE_REGION_DATA;
			pRegion->GetRectMonoImageData(&rcImgClip,&m_xCurrImageData2);
			m_xCurrImageData2.imagedata=new char[m_xCurrImageData2.nEffWidth*m_xCurrImageData2.nHeight];
			pRegion->GetRectMonoImageData(&rcImgClip,&m_xCurrImageData2);
		}
		bmp.CreateBitmap(m_xCurrImageData2.nWidth,m_xCurrImageData2.nHeight,1,1,m_xCurrImageData2.imagedata);
	}
	else
	{
		pCurImage=&m_xCurrImageData;
		if( m_xCurrImageData.imagedata!=NULL&&(m_xCurrImageData.dwRelaObj!=(DWORD)pRegion||
			m_xCurrImageData.ciRelaObjType!=CDataCmpModel::IMAGE_REGION_DATA))
		{
			delete[]m_xCurrImageData.imagedata;
			m_xCurrImageData.imagedata=NULL;
		}
		if(m_xCurrImageData.imagedata==NULL)
		{
			m_xCurrImageData.dwRelaObj=(DWORD)pRegion;
			m_xCurrImageData.ciRelaObjType=CDataCmpModel::IMAGE_REGION_DATA;
			pRegion->GetRectMonoImageData(&rcImgClip,&m_xCurrImageData);
			m_xCurrImageData.imagedata=new char[m_xCurrImageData.nEffWidth*m_xCurrImageData.nHeight];
			pRegion->GetRectMonoImageData(&rcImgClip,&m_xCurrImageData);
		}
		bmp.CreateBitmap(m_xCurrImageData.nWidth,m_xCurrImageData.nHeight,1,1,m_xCurrImageData.imagedata);
	}
	dcMemory.CreateCompatibleDC(NULL);//pDC);
	dcMemory.SelectObject(&bmp);
	//
	CRect rcPanelClip=rcPanel;
	pDC->FillSolidRect(0,0,rcPanelClip.Width(),rcPanelClip.Height(),RGB(196,196,196));//通过指定背景清空位图
	/*double scaleof_local2global=xImagedata.nWidth/(double)rcPanelClip.Width();
	PIXEL_TRANS trans;
	trans.fScale2Global=scaleof_local2global;
	trans.xOrg.x=xOrgOffset.cx;
	trans.xOrg.y=xOrgOffset.cy;*/
	if(pPixelTranslator)
		*pPixelTranslator=m_xCurrTrans;
	rcPanelClip.top=xOrgOffset.cy;
	rcPanelClip.bottom=xOrgOffset.cy+f2i(pCurImage->nHeight/m_xCurrTrans.fScale2Global);
	CPoint xSrcImgOrg(0,0);
	int nCopyHeight=pCurImage->nHeight;
	if(xOrgOffset.cy<0)
	{
		xSrcImgOrg=m_xCurrTrans.TransToLocal(CPoint(0,0));
		xSrcImgOrg.y-=rcImgClip.top;
		nCopyHeight-=xSrcImgOrg.y;
		rcPanelClip.top=0;
	}
	pDC->StretchBlt(0,rcPanelClip.top,rcPanelClip.Width(),rcPanelClip.Height(),&dcMemory,0,xSrcImgOrg.y,pCurImage->nWidth,nCopyHeight,SRCCOPY);
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

	//转换超出顶点显示边界很多的二维线为有效线(否则WINDOWS显示不正常)
	if(pE->y-pS->y==0)	//水平线
	{
		pE->x=min(pE->x,rect.right);
		pS->x=min(pS->x,rect.right);
		pE->x=max(pE->x,rect.left);
		pS->x=max(pS->x,rect.left);
		return true;
	}
	else if(pE->x-pS->x==0)	//竖直线
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
void CDisplayDataPage::ClearImage()
{
	if(m_xCurrImageData.imagedata!=NULL)
		delete []m_xCurrImageData.imagedata;
	m_xCurrImageData.imagedata=NULL;
	//
	if(m_xCurrImageData2.imagedata!=NULL)
		delete []m_xCurrImageData2.imagedata;
	m_xCurrImageData2.imagedata=NULL;
}

void CDisplayDataPage::RelayoutWnd()
{
	CWnd* pWorkPanel=NULL,*pSubWorkPanel=NULL,*pHorzSplitter=NULL,*pSubWorkPanel2=NULL;
	pWorkPanel=GetDlgItem(IDC_WORK_PANEL);
	pSubWorkPanel=GetDlgItem(IDC_SUB_WORK_PANEL);
	pSubWorkPanel2=GetDlgItem(IDC_SUB_WORK_PANEL2);
	pHorzSplitter=GetDlgItem(IDC_SPLITTER_HORIZ);
	if(pWorkPanel==NULL || pSubWorkPanel==NULL)
		return;
	typedef CWnd* CWndPtr;
	int nSliderCtrlCount=4;
	CWndPtr sliderCoefWndArr[4]={NULL,NULL,NULL,NULL};
	CRect rect,recNumRect;
	long ctrIDArr[9]={IDC_S_COEF_TITLE,IDC_SLIDER_BALANCE_COEF,IDC_S_BALANCE_COEF,IDC_SLIDER_SPIN,
					  IDC_S_DISPLAY_MODEL,IDC_CMB_MODEL,IDC_TAB_CTRL,IDC_S_RECORD_NUM,IDC_E_RECORD_NUM};
	m_tabCtrl.ShowWindow(SW_HIDE);
	int iCurSel=m_tabCtrl.GetCurSel();
	int nLeft=0,nSplitY=0,nSecRowLeft=0;
	for(int i=0;i<9;i++)
	{
		CWnd *pWnd=GetDlgItem(ctrIDArr[i]);
		if(pWnd->GetSafeHwnd())
		{
			if(m_cDisType==CDataCmpModel::IMAGE_REGION_DATA)
			{
				if(i==6)
				{
					pWnd->ShowWindow(SW_HIDE);
					continue;
				}
				else
					pWnd->ShowWindow(SW_SHOW);
			}
			else //if(m_cDisType==CDataCmpModel::DWG_IMAGE_GROUP||m_cDisType==CDataCmpModel::IMAGE_SEG_GROUP)
			{
				if(i<6)
				{
					pWnd->ShowWindow(SW_HIDE);
					continue;
				}
				else if(i==6)
				{
					if(m_cDisType==CDataCmpModel::DWG_IMAGE_GROUP)
						pWnd->ShowWindow(SW_SHOW);
					else
					{
						pWnd->ShowWindow(SW_HIDE);
						continue;
					}
				}
				else
				{
					if(iCurSel==1)
						pWnd->ShowWindow(SW_SHOW);
					else
					{
						pWnd->ShowWindow(SW_HIDE);
						continue;
					}
				}
			}
			pWnd->GetWindowRect(&rect);
			ScreenToClient(&rect);
			int nWidth=rect.right-rect.left;
			int nHight=rect.bottom-rect.top;
			if(i==0 || i==2 || i==4 || i==7)
				rect.top=4;
			else
				rect.top=1;
			rect.bottom=rect.top+nHight;
			if(m_biLayoutType==2&&i>=0&&i<nSliderCtrlCount)
			{	//明暗调整控件放在图片顶部
				rect.left=nSecRowLeft;
				rect.right=nSecRowLeft+nWidth;
				rect.top+=nHight;
				if(i==0||i==2)
					rect.top+=10;
				else if(i==1)
					rect.top+=5;
				rect.bottom=rect.top+nHight;
				pWnd->MoveWindow(&rect);
				if(i==3||i==5)
					nSecRowLeft+=nWidth+15;
				else
					nSecRowLeft+=nWidth;
			}
			else
			{
				rect.left=nLeft;
				rect.right=nLeft+nWidth;
				if(i==8)
					recNumRect=rect;
				pWnd->MoveWindow(&rect);
				if(i==3||i==5)
					nLeft+=nWidth+15;
				else
					nLeft+=nWidth+1;
				if(rect.bottom>nSplitY)
					nSplitY=rect.bottom;
			}
			if(i>=0&&i<nSliderCtrlCount)
				sliderCoefWndArr[i]=pWnd;
		}
	}
	CWnd *pZoomScaleTitleWnd=GetDlgItem(IDC_S_PDF_ZOOM_SCALE_TITLE);
	CWnd *pZoomScaleWnd=GetDlgItem(IDC_SLIDER_ZOOM_SCALE);
	CRect rectZoomScaleTitle,rectZoomScale;
	pZoomScaleTitleWnd->GetWindowRect(&rectZoomScaleTitle);
	pZoomScaleWnd->GetWindowRect(&rectZoomScale);
	ScreenToClient(&rectZoomScaleTitle);
	ScreenToClient(&rectZoomScale);
	CImageFileHost *pImageFile=(m_cDisType==CDataCmpModel::DWG_IMAGE_DATA)?(CImageFileHost*)m_pData:NULL;
	if( m_cDisType==CDataCmpModel::DWG_IMAGE_DATA&&
		pImageFile&&pImageFile->IsSrcFromPdfFile())
	{
		pZoomScaleTitleWnd->ShowWindow(SW_SHOW);
		pZoomScaleWnd->ShowWindow(SW_SHOW);
		int width=rectZoomScaleTitle.Width();
		rectZoomScaleTitle=recNumRect;
		rectZoomScaleTitle.left=recNumRect.right+2;
		rectZoomScaleTitle.right=rectZoomScaleTitle.left+width;
		pZoomScaleTitleWnd->MoveWindow(rectZoomScaleTitle);
		width=rectZoomScale.Width();
		rectZoomScale=recNumRect;
		rectZoomScale.left=rectZoomScaleTitle.right+1;
		rectZoomScale.right=rectZoomScale.left+width;
		pZoomScaleWnd->MoveWindow(rectZoomScale);
		//
		m_sliderZoomScale.SetPos((int)(pImageFile->fPDFZoomScale*100));
	}
	else
	{
		pZoomScaleTitleWnd->ShowWindow(SW_HIDE);
		pZoomScaleWnd->ShowWindow(SW_HIDE);
	}
	//
	if(m_cDisType==CDataCmpModel::DWG_IMAGE_DATA)
	{
		CImageFileHost* pImage=(CImageFileHost*)m_pData;
		if (pImage)
		{	//此处需要进行非NULL判断，否则启动软件时到此处会死机 wht 19-07-27
			double balancecoef = pImage->GetMonoThresholdBalanceCoef();
			int scalerPos = 50 + f2i(balancecoef * 50);
			if (scalerPos < 0)
				scalerPos = 0;
			else if (scalerPos > 100)
				scalerPos = 100;
			this->m_sliderBalanceCoef.SetPos(scalerPos);
		}
	}
	else if(m_cDisType==CDataCmpModel::IMAGE_REGION_DATA)
	{
		CImageRegionHost* pImageRegion=(CImageRegionHost*)m_pData;
		if (pImageRegion)
		{	//此处需要进行非NULL判断，否则启动软件时到此处会死机 wht 19-07-27
			LazyLoadImageRegion(pImageRegion);
			double balancecoef = pImageRegion->GetMonoThresholdBalanceCoef();
			int scalerPos = 50 + f2i(balancecoef * 50);
			if (scalerPos < 0)
				scalerPos = 0;
			else if (scalerPos > 100)
				scalerPos = 100;
			this->m_sliderBalanceCoef.SetPos(scalerPos);
		}
	}
	int nListCtrlWidth=f2i(m_fZoomCoefOfScrX*30);
	for(int i=0;i<m_listDataCtrl.GetColumnCount();i++)
	{
		if(i==7)
		{
			if(CConfig::m_bRecogSumWeight)
				m_arrColWidth[i]=(int)(110*m_fZoomCoefOfScrX);
			else
				m_arrColWidth[i]=0;
		}
		if(m_biLayoutType!=3)
			m_listDataCtrl.SetColumnWidth(i,m_arrColWidth[i]);
		nListCtrlWidth+=m_arrColWidth[i];
	}
	int SPLITTER_HEIGHT=5;
	if(pHorzSplitter&&m_biLayoutType!=1)
		pHorzSplitter->ShowWindow(SW_HIDE);
	
	double fScaleY=1;
	DYN_ARRAY<int> imageColWidthArr;
	if(m_cDisType==CDataCmpModel::IMAGE_REGION_DATA&&m_biLayoutType>=2&&m_biLayoutType<=4)
	{	//调整图像行高与列表行高一致
		int nMaxWidth=f2i(m_fZoomCoefOfScrX*640);
		CImageRegionHost* pRegion=(CImageRegionHost*)m_pData;
		if (pRegion)
		{	//此处需要进行非NULL判断，否则启动软件时到此处会死机 wht 19-07-27
			LazyLoadImageRegion(pRegion);
			int nCol = pRegion->GetImageColumnWidth();
			imageColWidthArr.Resize(nCol);
			CRect rcItem;
			m_listDataCtrl.GetItemRect(0, rcItem, LVIR_BOUNDS);
			int nRecogPart = pRegion->GetRecogPartNum();
			if (pRegion->Height > 0 && nRecogPart > 0 && rcItem.Height() > 0)
				fScaleY = (1.0*rcItem.Height()*nRecogPart) / pRegion->Height;
			if (nCol > 0)
				pRegion->GetImageOrgColumnWidth(imageColWidthArr);
			else
			{
				imageColWidthArr.Resize(7);
				imageColWidthArr[0] = f2i(m_fZoomCoefOfScrX * 60);
				imageColWidthArr[1] = f2i(m_fZoomCoefOfScrX * 150);
				imageColWidthArr[2] = f2i(m_fZoomCoefOfScrX * 90);
				imageColWidthArr[3] = f2i(m_fZoomCoefOfScrX * 60);
				imageColWidthArr[4] = f2i(m_fZoomCoefOfScrX * 90);
				imageColWidthArr[5] = f2i(m_fZoomCoefOfScrX * 90);
				imageColWidthArr[6] = f2i(m_fZoomCoefOfScrX * 180);
			}
			rect = m_rcClient;
			rect.top = nSplitY + 1;
			int nImageWidth = (int)(pRegion->GetWidth()*fScaleY);
			if (nImageWidth == 0)
			{
				for (WORD i = 0; i < imageColWidthArr.Size(); i++)
					nImageWidth += imageColWidthArr[i];
				nImageWidth = f2i(nImageWidth*fScaleY);
			}
			rect.right = rect.left + nImageWidth;
			pRegion->InitImageShowPara(rect);
			m_xCurrTrans.fScale2Global = 1.0 / fScaleY;
		}
	}
	CRect rcListCtrl;	//列表框位置
	DWORD dwStyle=m_listDataCtrl.GetStyle();
	if(m_biLayoutType==4)
		m_listDataCtrl.ModifyStyle(0,LVS_NOCOLUMNHEADER);
	else
		m_listDataCtrl.ModifyStyle(LVS_NOCOLUMNHEADER,0);
	m_listFileCtrl.ShowWindow(SW_HIDE);

	if(m_biLayoutType==0)
	{	//0.构件信息
		rect=m_rcClient;
		rect.top=nSplitY+1;	
		rcListCtrl=rect;
		m_listDataCtrl.MoveWindow(rect);
		m_listFileCtrl.MoveWindow(rect);
		if(iCurSel==0)
		{
			m_listDataCtrl.ShowWindow(SW_HIDE);
			m_listFileCtrl.ShowWindow(SW_SHOW);
		}
		else
		{
			m_listDataCtrl.ShowWindow(SW_SHOW);
			m_listFileCtrl.ShowWindow(SW_HIDE);
		}
		pWorkPanel->ShowWindow(SW_HIDE);
		pSubWorkPanel->ShowWindow(SW_HIDE);
		pSubWorkPanel2->ShowWindow(SW_HIDE);
		m_xInputPartInfoPage.ShowWindow(SW_HIDE);
		for(int i=0;i<nSliderCtrlCount;i++)
		{
			if(sliderCoefWndArr[i])
				sliderCoefWndArr[i]->ShowWindow(SW_HIDE);
		}
		m_listDataCtrl1.ShowWindow(SW_HIDE);
	}
	else if(m_biLayoutType==1)
	{	//1.图片信息
		CImageFileHost* pImageHost=NULL;
		if(m_cDisType==CDataCmpModel::DWG_IMAGE_DATA && m_pData)
			pImageHost=(CImageFileHost*)m_pData;
		if(pImageHost&&pImageHost->EnumFirstPart())
		{
			m_listDataCtrl.ShowWindow(SW_SHOW);
			m_nOldHorzY=(int)(m_fDividerYScale*(m_rcClient.bottom-m_rcClient.top));
			rect=m_rcClient;
			rect.top=nSplitY+1;
			rect.right-=(nListCtrlWidth+2);
			pWorkPanel->MoveWindow(rect);
			for(int i=0;i<nSliderCtrlCount;i++)
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
			rcListCtrl=rect;
			m_listDataCtrl.MoveWindow(rect);
		}
		else 
		{
			m_listDataCtrl.ShowWindow(SW_HIDE);
			rect=m_rcClient;
			rect.top=nSplitY+1;
			pWorkPanel->MoveWindow(rect);
		}
		//
		pWorkPanel->ShowWindow(SW_NORMAL);
		pSubWorkPanel->ShowWindow(SW_HIDE);
		pSubWorkPanel2->ShowWindow(SW_HIDE);
		m_xInputPartInfoPage.ShowWindow(SW_HIDE);
		m_listDataCtrl1.ShowWindow(SW_HIDE);
	}
	else if(m_biLayoutType==2)
	{	//数据区域信息
		CRect headRect;
		CImageRegionHost* pRegion=(CImageRegionHost*)m_pData;
		if(pRegion)
		{
			LazyLoadImageRegion(pRegion);
			CHeaderCtrl *pHeadCtrl=m_listDataCtrl.GetHeaderCtrl();
			pHeadCtrl->GetWindowRect(headRect);
			SIZE size;
			m_xCurrTrans.xOrg.x=size.cx=0;
			m_xCurrTrans.xOrg.y=size.cy=0;//headRect.Height();
			pRegion->SetScrOffset(size);
		}
		m_nOldHorzY=(int)(m_fDividerYScale*(m_rcClient.bottom-m_rcClient.top));
		rect=m_rcClient;
		rect.top=nSplitY+1+headRect.Height();
		int nValidWidth=0;
		for(WORD i=0;i<min(REV_COL_COUNT,imageColWidthArr.Size());i++)
			nValidWidth+=(int)(imageColWidthArr[i]*fScaleY);
		//int nImageWidth=(int)(pRegion->GetWidth()*fScaleY);
		rect.right=rect.left+nValidWidth;
		pWorkPanel->MoveWindow(rect);
		rect=m_rcClient;
		rect.top=nSplitY+1;
		rect.left=nValidWidth+1;
		if(imageColWidthArr.Size()>5)
		{
			rect.right=rect.left+nListCtrlWidth;//m_rcClient.right;
			rcListCtrl=rect;
			m_listDataCtrl.MoveWindow(rect);
			rect.top+=headRect.Height();
			rect.left=rect.right;
			rect.right=rect.left+(int)(imageColWidthArr[5]*fScaleY);
			pSubWorkPanel2->ShowWindow(SW_SHOW);
			pSubWorkPanel2->MoveWindow(rect);
		}
		else
		{
			rect.right=m_rcClient.right;
			rcListCtrl=rect;
			m_listDataCtrl.MoveWindow(rect);
			pSubWorkPanel2->ShowWindow(SW_HIDE);
		}
		/*rect.bottom=rect.top+m_nInputDlgH;
		pSubWorkPanel->MoveWindow(rect);
		rect=m_rcClient;
		rect.top=m_nOldHorzY+4+m_nInputDlgH;*/
		//
		pWorkPanel->ShowWindow(SW_NORMAL);
		m_xInputPartInfoPage.ShowWindow(SW_NORMAL);
		pSubWorkPanel->ShowWindow(SW_HIDE);
		m_listDataCtrl1.ShowWindow(SW_HIDE);
		m_listDataCtrl.ShowWindow(SW_SHOW);
	}
	else if(m_biLayoutType==3)
	{	//按列校审
		m_listDataCtrl.ShowWindow(SW_SHOW);
		pWorkPanel->ShowWindow(SW_SHOW);
		pSubWorkPanel->ShowWindow(SW_HIDE);
		pSubWorkPanel2->ShowWindow(SW_HIDE);
		pHorzSplitter->ShowWindow(SW_HIDE);
		int nCurColWidth=0;
		CRect headerRect;
		CImageRegionHost* pRegion=(CImageRegionHost*)m_pData;
		if(pRegion&&m_biCurCol>=0&&m_biCurCol<=min(REV_COL_COUNT,imageColWidthArr.Size()))
		{
			LazyLoadImageRegion(pRegion);
			CHeaderCtrl *pHeader=m_listDataCtrl.GetHeaderCtrl();
			pHeader->GetClientRect(headerRect);
			SIZE offset;
			offset.cx=0;
			nCurColWidth=imageColWidthArr[0];
			if(m_biCurCol==1||m_biCurCol==2)
			{
				offset.cx=-imageColWidthArr[0];
				nCurColWidth=imageColWidthArr[1];
			}
			else if(m_biCurCol==3)
			{
				offset.cx=-(imageColWidthArr[0]+imageColWidthArr[1]);
				nCurColWidth=imageColWidthArr[2];
			}
			else if(m_biCurCol==4)
			{
				offset.cx=-(imageColWidthArr[0]+imageColWidthArr[1]+imageColWidthArr[2]);
				nCurColWidth=imageColWidthArr[3];
			}
			else if(m_biCurCol==5)
			{
				offset.cx=-(imageColWidthArr[0]+imageColWidthArr[1]+imageColWidthArr[2]+imageColWidthArr[3]);
				nCurColWidth=imageColWidthArr[4];
			}
			m_xCurrTrans.xOrg.x=offset.cx=(int)(offset.cx*fScaleY);
			m_xCurrTrans.xOrg.y=offset.cy=0;//headerRect.Height();
			pRegion->SetScrOffset(offset);
			nCurColWidth=(int)(nCurColWidth*fScaleY);
		}
		rect=m_rcClient;
		rect.top=(nSplitY+1+headerRect.Height());	
		if(m_biCurCol==0)
		{
			rect.right=rect.left+nCurColWidth;
			pWorkPanel->MoveWindow(rect);
			rect=m_rcClient;
			rect.top=nSplitY+1;	
			rect.left+=nCurColWidth;
			for(int i=0;i<m_listDataCtrl.GetItemCount();i++)
				m_listDataCtrl.SetColumnWidth(i,m_arrColWidth[i]);
			rcListCtrl=rect;
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
			rect.right+=20;	//多出10个宽度，避免出现水平滚动条
			m_listDataCtrl1.ShowWindow(SW_SHOW);
			m_listDataCtrl1.MoveWindow(rect);
			rect.top=(nSplitY+1+headerRect.Height());	
			rect.left=rect.right+1;
			rect.right=rect.left+nCurColWidth;
			pWorkPanel->MoveWindow(rect);
			rect.top=(nSplitY+1);
			rect.left=rect.right+1;
			rect.right=m_rcClient.right;
			rcListCtrl=rect;
			m_listDataCtrl.MoveWindow(rect);
			m_listDataCtrl.SetFocus();
		}
	}
	else if(m_biLayoutType==4)
	{	//按行校审
		m_listDataCtrl1.ShowWindow(SW_SHOW);
		m_listDataCtrl.ShowWindow(SW_SHOW);
		pWorkPanel->ShowWindow(SW_SHOW);
		//
		rect=m_rcClient;
		rect.top=nSplitY+1;
		rect.bottom=rect.top+(int)((rect.bottom-rect.top-nSplitY-1)*0.5);
		m_listDataCtrl1.MoveWindow(rect);
		rect.top=rect.bottom+1;
		rect.bottom=rect.top+27;
		pWorkPanel->MoveWindow(rect);
		rect.top=rect.bottom+1;
		rect.bottom=m_rcClient.bottom;
		rcListCtrl=rect;
		m_listDataCtrl.MoveWindow(rect);
	}

	if(m_biLayoutType==LAYOUT_BROWSE||m_biLayoutType==LAYOUT_COLREV)
	{
		CImageRegionHost* pRegion=(CImageRegionHost*)m_pData;
		int iCurSel=m_listDataCtrl.GetSelectedItem();
		CSuperGridCtrl::CTreeItem *pItem=m_listDataCtrl.GetTreeItem(iCurSel<=0?0:iCurSel);
		if(pRegion&&pItem&&pItem->m_idProp>0)
			Focus(&m_listDataCtrl,pItem);
	}
	//单张图像或分段显示时，总重列顶部显示总重
	CWnd *pSumWeightTitleWnd=GetDlgItem(IDC_S_SUM_WEIGHT_TITLE);
	CWnd *pSumWeightWnd=GetDlgItem(IDC_S_SUM_WEIGHT);
	if( m_cDisType==CDataCmpModel::DWG_IMAGE_DATA||
		m_cDisType==CDataCmpModel::IMAGE_SEG_GROUP||
		(m_cDisType==CDataCmpModel::DWG_IMAGE_GROUP&&iCurSel==1)||
		(m_cDisType==CDataCmpModel::IMAGE_REGION_DATA&&m_biLayoutType!=3))
	{	
		if(m_listDataCtrl.IsWindowVisible())
		{
			pSumWeightWnd->ShowWindow(SW_SHOW);
			pSumWeightTitleWnd->ShowWindow(SW_SHOW);
			CRect rectTitle,rectWeight;
			pSumWeightWnd->GetWindowRect(&rectWeight);
			pSumWeightTitleWnd->GetWindowRect(&rectTitle);
			ScreenToClient(&rectTitle);
			ScreenToClient(&rectWeight);
			//
			int height=rectTitle.Height();
			rectTitle.top=4;
			rectTitle.bottom=rectTitle.top+height;
			int nLeft=0;
			for(int i=0;i<5;i++)
				nLeft+=m_arrColWidth[i];
			rectTitle.right=rcListCtrl.left+nLeft+m_arrColWidth[5];
			if(m_cDisType==CDataCmpModel::DWG_IMAGE_DATA)
				rectTitle.left=rectTitle.right-350;	//增大宽度，用于显示分段总重 wht 18-06-10
			else
				rectTitle.left=rcListCtrl.left+nLeft;
			pSumWeightTitleWnd->MoveWindow(rectTitle);
			rectWeight=rectTitle;
			rectWeight.left=rectTitle.right;
			rectWeight.right=rectWeight.left+m_arrColWidth[6];
			if(CConfig::m_bRecogSumWeight)
				rectWeight.right+=(m_arrColWidth[7]*2);
			pSumWeightWnd->MoveWindow(rectWeight);
		}
		else
		{
			pSumWeightWnd->ShowWindow(SW_HIDE);
			pSumWeightTitleWnd->ShowWindow(SW_HIDE);
		}
	}
	else
	{
		pSumWeightWnd->ShowWindow(SW_HIDE);
		pSumWeightTitleWnd->ShowWindow(SW_HIDE);
	}
	CWnd *pPromptWnd=GetDlgItem(IDC_S_PROMPT);
	if(CConfig::m_bDisplayPromptMsg&&(m_biLayoutType==2||m_biLayoutType==3))
	{
		pPromptWnd->ShowWindow(SW_SHOW);
		CRect rc=rcListCtrl,rcPropmt;
		pPromptWnd->GetWindowRect(&rcPropmt);
		ScreenToClient(&rcPropmt);
		int width=rcPropmt.Width();
		rcPropmt=rcListCtrl;
		int margin=10;
		if(m_biLayoutType==3)
		{
			rcPropmt.left=rcPropmt.right-width;
			pPromptWnd->MoveWindow(rcPropmt);
			rc.right=rcPropmt.left-margin;
			m_listDataCtrl.MoveWindow(rc);
		}
		else
		{
			rcPropmt.left=rcListCtrl.right+margin;
			if(imageColWidthArr.Size()>5)
				rcPropmt.left+=(int)(imageColWidthArr[5]*fScaleY);
			rcPropmt.right=rcPropmt.left+width;
			pPromptWnd->MoveWindow(rcPropmt);
		}
	}
	else
		pPromptWnd->ShowWindow(SW_HIDE);
}
void CDisplayDataPage::ShiftCheckColumn(int iCol)
{
	if(iCol>REV_COL_COUNT)
		return;
	m_biCurCol=iCol;
	ClearImage();
	RelayoutWnd();
}
CImageFileHost* CDisplayDataPage::GetActiveImageFile()
{
	if(m_cDisType==CDataCmpModel::DWG_IMAGE_DATA)
		return (CImageFileHost*)m_pData;
	else if(m_cDisType==CDataCmpModel::IMAGE_REGION_DATA)
	{
		CImageRegionHost* pRegion=(CImageRegionHost*)m_pData;
		return pRegion?pRegion->BelongImageFile():NULL;
	}
	else
		return NULL;
}
POINT CDisplayDataPage::TransPoint(CPoint pt,BOOL bTransFromCanvasPanel,BOOL bPanelDC/*=FALSE*/)
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
	//计算基准点
	CPoint orgion;
	if(bPanelDC)
		orgion.SetPoint(offset.cx,offset.cy);
	else
	{
		CRect rect;
		GetWorkPanelRect(rect);
		orgion.SetPoint(rect.left+offset.cx,rect.top+offset.cy);
	}
	//进行转换偏移
	CPoint point=pt;
	double fScale=1;
	if(bTransFromCanvasPanel)
	{//将鼠标所在点转换为工作点
		fScale=1.0/fRatio;
		point-=orgion;
		point.x=f2i(point.x*fScale);
		point.y=f2i(point.y*fScale);
	}
	else
	{//将工作点转换为显示点
		fScale=fRatio;
		point.x=f2i(point.x*fScale);
		point.y=f2i(point.y*fScale);
		point+=orgion;
	}
	return point;
}
POINT CDisplayDataPage::IntelliAnchorCrossHairPoint(POINT point,BYTE ciLT0RT1RB2LB3,double *pfTurnGradientToOrthoImg/*=NULL*/)
{	//ciLT0RT1RB2LB3
	if(m_pData==NULL||pActiveImgFile==NULL)
		return point;
	int width=m_xPolyline.m_ptArr[1].x-m_xPolyline.m_ptArr[0].x;
	double fMaxTestWidth=width*0.12;	//框选5列或6列,最后一列所占比重都在15%左右,此处测试宽度取12% wht 18-12-12
	int uiTestWidth=200,uiTestHeight=100;
	if(uiTestWidth>fMaxTestWidth)
	{
		uiTestWidth=(int)fMaxTestWidth;
		uiTestHeight=(int)(fMaxTestWidth*0.5);
	}
	int yjHoriTipOffset=0;
	bool corrected=pActiveImgFile->IntelliRecogCornerPoint(point,&point,ciLT0RT1RB2LB3,&yjHoriTipOffset,uiTestWidth,uiTestHeight);
	if(pfTurnGradientToOrthoImg)
		*pfTurnGradientToOrthoImg=corrected?yjHoriTipOffset/(uiTestWidth*1.0):0;
	return point;
}
void CDisplayDataPage::SetCurTask(int iCurTask)
{
	m_cCurTask=iCurTask;
	if(m_cCurTask==TASK_SNAP)	//清空捕捉信息
	{
		m_xPolyline.Init();
		if(pActiveImgFile)
			pActiveImgFile->m_xPolyline.Init();
	}
	else if(m_cCurTask==TASK_ZOOM)
		ZoomAll();
}
void CDisplayDataPage::OperOther()
{
	SetCurTask(TASK_OTHER);
	m_bStartDrag=FALSE;
	m_xPolyline.m_iActivePt=-1;
	CIBOMView *pView=theApp.GetIBomView();
	if(pView)
		pView->SetPinpointIndex(-1);
	if(pActiveImgFile)
		pActiveImgFile->m_xPolyline.m_iActivePt=-1;
	Invalidate(FALSE);
}
//////////////////////////////////////////////////////////////////////////
//消息处理
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

static void DrawFocusRect(CDC *pDC,PIXEL_TRANS &trans,CRect &rc,CRect &rcPanelClient,COLORREF clr,BYTE flag=0xFF)
{
	CPoint topLeft =trans.TransToGlobal(rc.TopLeft());
	CPoint btmRight=trans.TransToGlobal(rc.BottomRight());
	CPen redPen(PS_SOLID,2,clr);
	CPen* pOldPen=(CPen*)pDC->SelectObject(&redPen);
	if(flag&0x01)
		CDisplayDataPage::DrawClipLine(pDC,topLeft,CPoint(btmRight.x,topLeft.y),&rcPanelClient);
	if(flag&0x02)
		CDisplayDataPage::DrawClipLine(pDC,btmRight,CPoint(topLeft.x,btmRight.y),&rcPanelClient);
	if(flag&0x04)
		CDisplayDataPage::DrawClipLine(pDC,CPoint(topLeft.x,btmRight.y),topLeft,&rcPanelClient);
	if(flag&0x08)
		CDisplayDataPage::DrawClipLine(pDC,CPoint(btmRight.x,topLeft.y),btmRight,&rcPanelClient);
	pDC->SelectObject(pOldPen);
}
//
void CDisplayDataPage::OnPaint()
{
	CPaintDC dc(this);	//设备上下文
	if(m_cDisType!=CDataCmpModel::DWG_IMAGE_DATA&&m_cDisType!=CDataCmpModel::IMAGE_REGION_DATA)
		return;
	if(m_cDisType==CDataCmpModel::IMAGE_REGION_DATA)
	{
		//绘制分割线
		/*CPen psPen(PS_SOLID,1,RGB(120,120,120));
		CPen* pOldPen = dc.SelectObject(&psPen);
		dc.MoveTo(m_rcClient.left,m_nOldHorzY+1);
		dc.LineTo(m_rcClient.right,m_nOldHorzY+1);
		dc.MoveTo(m_rcClient.left,m_nOldHorzY+m_nInputDlgH);
		dc.LineTo(m_rcClient.right,m_nOldHorzY+m_nInputDlgH);
		dc.SelectObject(pOldPen);
		psPen.DeleteObject();*/
		//
		if(m_xInputPartInfoPage.IsWindowVisible())
		{
			int iCurSel=m_listDataCtrl.GetSelectedItem();
			CSuperGridCtrl::CTreeItem *pItem=m_listDataCtrl.GetTreeItem(iCurSel);
			if(pItem)
				m_xInputPartInfoPage.m_pBomPart=(IRecoginizer::BOMPART*)pItem->m_idProp;
			m_xInputPartInfoPage.RefreshDlg();
		}
	}
	//
	CWnd *pWorkPanel=GetDlgItem(IDC_WORK_PANEL);
	if(pWorkPanel==NULL)
		return;
	CRect rcPanelClient;
	pWorkPanel->GetClientRect(&rcPanelClient);
	double fScaleY=1.0;
	if(m_cDisType==CDataCmpModel::IMAGE_REGION_DATA)
	{	//显示图片区域时设置显示宽度
		int nMaxWidth=640;
		CImageRegionHost* pRegion=(CImageRegionHost*)m_pData;
		IRecoginizer::BOMPART* pBomPart=pRegion?pRegion->EnumFirstPart():NULL;
		if(pBomPart)
		{
			CRect rcImage,rcItem;
			if(!pRegion->GetBomPartRect(pBomPart->id,&rcImage))
				rcImage=pBomPart->rc;
			m_listDataCtrl.GetItemRect(0,rcItem,LVIR_BOUNDS);
			fScaleY=(1.0*rcItem.Height())/rcImage.Height();
			//nMaxWidth=(int)(pRegion->GetWidth()*fScaleY);
			//panelRect.right=panelRect.left+nMaxWidth;
		}
		else if(rcPanelClient.Width()>nMaxWidth)
		{
			fScaleY=(1.0*nMaxWidth)/pRegion->Width;
			rcPanelClient.right=rcPanelClient.left+nMaxWidth;
		}
	}
	
	CDC*  pSubDC=NULL;
	CBitmap subMemoryBP;
	CDC     subMemoryDC;
	CDC*  pDC=pWorkPanel->GetDC();	// 绘图控件的设备上下文,如果用父窗口上下文，会导致涂改其余控件
	int nWidth=rcPanelClient.Width();
	int nHeight=rcPanelClient.Height();
	CBitmap memoryBP;
	CDC     memoryDC;
	memoryBP.CreateCompatibleBitmap(pDC,rcPanelClient.Width(),rcPanelClient.Height());
	memoryDC.CreateCompatibleDC(NULL);
	memoryDC.SelectObject(&memoryBP);
	memoryDC.FillSolidRect(0,0,nWidth,nHeight,RGB(196,196,196));//通过指定背景清空位图
	if(m_cDisType==CDataCmpModel::DWG_IMAGE_DATA)
	{	//绘制图片区域
		CImageFileHost* pImage=(CImageFileHost*)m_pData;
		pImage->InitImageShowPara(rcPanelClient);
		StretchBlt(memoryDC.GetSafeHdc(),pImage);	//在内存设备上绘制选中图片位图
		pDC->BitBlt(0,0,nWidth,nHeight,&memoryDC,0,0,SRCCOPY);//将内存上位图复制到物理设备上
		//绘制正在进行选择的矩形框
		CPen bluePen(PS_SOLID,2,RGB(0,0,255)),redPen(PS_SOLID,2,RGB(255,0,0)),greenPen(PS_SOLID,2,RGB(0,255,0));
		CPen *pOldPen=pDC->SelectObject(&bluePen);
		if(m_xPolyline.m_cState==POLYLINE_4PT::STATE_VALID)
		{	//绘制拾取区域特征点
			CPoint ptArr[5];
			for(int i=0;i<5;i++)
			{
				CPoint pt=m_xPolyline.m_ptArr[i%4];
				ptArr[i]=TransPoint(pt,FALSE,TRUE);
				if(i==m_xPolyline.m_iActivePt)
					pDC->SelectObject(&greenPen);
				else
					pDC->SelectObject(&bluePen);
				CRect marginRect(rcPanelClient.left+6,rcPanelClient.top+6,rcPanelClient.right-6,rcPanelClient.bottom-6);
				if(i!=4 && marginRect.PtInRect(ptArr[i]))	//marginRect代替panelRect是为了避免标识圆超出绘图区域
					pDC->Ellipse(CRect(ptArr[i].x-5,ptArr[i].y-5,ptArr[i].x+5,ptArr[i].y+5));
				if(i==0)
					continue;
				POINT xS=ptArr[i-1],xE=ptArr[i];
				if(ClipLine(rcPanelClient,&xS,&xE))
				{
					pDC->SelectObject(&bluePen);
					pDC->MoveTo(xS);
					pDC->LineTo(xE);
				}
			}
		}
		// 绘制所有已确定的数据区域
		pDC->SelectObject(&redPen);
		int iRegion=1;
		for(IImageRegion *pRegion=pImage->EnumFirstRegion();pRegion;pRegion=pImage->EnumNextRegion())
		{
			POINT ptArr[5]={pRegion->TopLeft(),pRegion->TopRight(),pRegion->BtmRight(),pRegion->BtmLeft(),pRegion->TopLeft()};
			CPen *pCurPen=&bluePen;
			if(iRegion%2==1)
				pCurPen=&redPen;
			else if(iRegion%2==2)
				pCurPen=&greenPen;
			iRegion++;
			for(int i=0;i<5;i++)
			{
				ptArr[i].y=ptArr[i].y;
				ptArr[i]=TransPoint(ptArr[i],FALSE,TRUE);
				if(i==0)
					continue;
				POINT xS=ptArr[i-1],xE=ptArr[i];
				if(ClipLine(rcPanelClient,&xS,&xE))
				{
					pDC->SelectObject(pCurPen);
					pDC->MoveTo(xS);
					pDC->LineTo(xE);
				}
			}
		}
		pDC->SelectObject(pOldPen);
	}
	else if(m_cDisType==CDataCmpModel::IMAGE_REGION_DATA)
	{	//绘制数据区域
		CImageRegionHost* pRegion=(CImageRegionHost*)m_pData;
		PIXEL_TRANS trans;
		CRect rcSubPanelClient;
		StretchBlt(pDC,pRegion,rcPanelClient,&trans);
		CWnd *pSubWorkPanel2=GetDlgItem(IDC_SUB_WORK_PANEL2);
		pSubDC=pSubWorkPanel2?pSubWorkPanel2->GetDC():NULL;	// 绘图控件的设备上下文,如果用父窗口上下文，会导致涂改其余控件
		if(m_biLayoutType==LAYOUT_BROWSE&&pSubDC)
		{
			pSubWorkPanel2->GetClientRect(&rcSubPanelClient);
			int nSubWidth=rcSubPanelClient.Width();
			int nSubHeight=rcSubPanelClient.Height();
			subMemoryBP.CreateCompatibleBitmap(pSubDC,rcSubPanelClient.Width(),rcSubPanelClient.Height());
			subMemoryDC.CreateCompatibleDC(NULL);
			subMemoryDC.SelectObject(&subMemoryBP);
			subMemoryDC.FillSolidRect(0,0,nSubWidth,nSubHeight,RGB(196,196,196));//通过指定背景清空位图
			StretchBlt(pSubDC,pRegion,rcSubPanelClient,&trans,TRUE);
		}
		//绘制选中数据区域
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
				DrawFocusRect(pDC,trans,rc,rcPanelClient,RGB(255,0,0));
				if(pSubDC)
					DrawFocusRect(pSubDC,trans,rc,rcSubPanelClient,RGB(255,0,0));
			}
		}
		int nColCount=pRegion->GetImageOrgColumnWidth();
		if(nColCount)
		{
			CRect rc;
			DYN_ARRAY<int> arrColWidth(nColCount);
			pRegion->GetImageOrgColumnWidth(arrColWidth);
			for(UINT i=0;i<m_listDataCtrl.GetCount();i++)
			{
				CSuperGridCtrl::CTreeItem *pCurItem=m_listDataCtrl.GetTreeItem(i);
				IRecoginizer::BOMPART* pBomPart=pCurItem?(IRecoginizer::BOMPART*)pCurItem->m_idProp:NULL;
				if(pBomPart==NULL)
					continue;
				if(!pRegion->GetBomPartRect(pBomPart->id,&rc))
					rc=pBomPart->rc;
				int left=0,iCell=0;
				for(int j=0;j<m_listDataCtrl.GetColumnCount();j++)
				{
					iCell=j>1?j-1:j;
					if(!(pBomPart->ciSrcFromMode&constByteArr[j])) 
					{	//根据匹配程度显示背景颜色
						BYTE cLevel=pBomPart->arrItemWarningLevel[j];
						if(cLevel>0)
						{
							CRect cellRect=rc;
							cellRect.left=left;
							cellRect.right=left+arrColWidth[iCell];
							cellRect.left+=5;
							cellRect.right-=5;
							cellRect.top+=5;
							cellRect.bottom-=5;
							if(j==6)
							{
								if(pSubDC)
									DrawFocusRect(pSubDC,trans,cellRect,rcSubPanelClient,CConfig::warningLevelClrArr[cLevel],0x02);
							}
							else
								DrawFocusRect(pDC,trans,cellRect,rcPanelClient,CConfig::warningLevelClrArr[cLevel],0x02);
						}
					}
					if(j!=1)
						left+=arrColWidth[iCell];
				}
			}
		}
	}
	// 释放对象和DC
	memoryBP.DeleteObject();
	memoryDC.DeleteDC();
	ReleaseDC(pDC);
	if(pSubDC)
	{	// 释放对象和DC
		subMemoryBP.DeleteObject();
		subMemoryDC.DeleteDC();
		ReleaseDC(pSubDC);
	}
}
void CDisplayDataPage::OnLButtonDown(UINT nFlags, CPoint point)
{
	CRect rect;
	GetWorkPanelRect(rect);
	if(GetCurTask()==TASK_OTHER)
	{	//
		if(rect.PtInRect(point))
			m_cCurTask=TASK_SNAP;
		/*if((point.y>=m_nOldHorzY-5) && (point.y<=m_nOldHorzY+5))
		{
			::SetCursor(m_hHorzSize);
			m_nOldHorzY = point.y;
			CClientDC dc(this);
			InvertLine(&dc,CPoint(m_rcClient.left,m_nOldHorzY),CPoint(m_rcClient.right,m_nOldHorzY));
			ReleaseDC(&dc);
			SetCapture();
			m_bHorzTracing=TRUE;
		}*/
	}
	if(GetCurTask()==TASK_SNAP)
	{	//图片捕捉操作
		if(!rect.PtInRect(point) || m_cDisType!=CDataCmpModel::DWG_IMAGE_DATA)
		{
			OperOther();
			return;
		}
		CIBOMView *pBomView=theApp.GetIBomView();
		if(pBomView&&pBomView->IsCanPinpoint())
			return;
		SetCapture();
		//开始进行数据区域选择
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
					POINT scrCursorPos=pt;
					ClientToScreen(&scrCursorPos);
					SetCursorPos(scrCursorPos.x,scrCursorPos.y);
					break;
				}
			}
			if(m_xPolyline.m_iActivePt>=0&&m_xPolyline.m_iActivePt<=4)
				m_ptLBDownPos=point;
			Invalidate(FALSE);
		}
		if(pActiveImgFile)
			pActiveImgFile->m_xPolyline=m_xPolyline;
	}
	if(GetCurTask()==TASK_MOVE || nFlags&MK_CONTROL)
	{	//图片移动操作(命令或CTRL+鼠标左键)
		if(!rect.PtInRect(point))
		{
			OperOther();
			return;
		}
		SetCapture();
		if(m_cDisType==CDataCmpModel::DWG_IMAGE_DATA)
		{
			CImageFileHost* pImage=(CImageFileHost*)m_pData;
			if(!m_bStartDrag)
			{
				::SetCursor(m_hMove);
				m_ptLBDownPos=point;
				m_bStartDrag=TRUE;
			}
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
	{	//图片区域操作
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
			CIBOMView *pBomView=theApp.GetIBomView();
			if(pBomView&&pBomView->IsCanPinpoint())
			{
				int iCurPt=pBomView->GetPinpointIndex();
				pBomView->SetPinpointIndex(iCurPt<3?iCurPt+1:-1);
				ZoomInPolyPointAt(iCurPt<3?iCurPt+1:-1);
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
			{	//初始点为右侧点
				if(startPt.y>endPt.y)
				{	//初始点为右下
					m_xPolyline.m_ptArr[0]=endPt;						//左上
					m_xPolyline.m_ptArr[1].SetPoint(startPt.x,endPt.y);	//右上
					m_xPolyline.m_ptArr[2]=startPt;						//右下
					m_xPolyline.m_ptArr[3].SetPoint(endPt.x,startPt.y);	//左下
				}
				else
				{	//初始点为右上
					m_xPolyline.m_ptArr[0].SetPoint(endPt.x,startPt.y);	//左上
					m_xPolyline.m_ptArr[1]=startPt;						//右上
					m_xPolyline.m_ptArr[2].SetPoint(startPt.x,endPt.y);	//右下
					m_xPolyline.m_ptArr[3]=endPt;						//左下
				}
			}
			else 
			{	//初始点为左侧点
				if(startPt.y>endPt.y)
				{	//初始点为左下
					m_xPolyline.m_ptArr[0].SetPoint(startPt.x,endPt.y);	//左上
					m_xPolyline.m_ptArr[1]=endPt;						//右上
					m_xPolyline.m_ptArr[2].SetPoint(endPt.x,startPt.y);	//右下
					m_xPolyline.m_ptArr[3]=startPt;						//左下
				}
				else
				{	//初始点为左上
					m_xPolyline.m_ptArr[0]=startPt;						//左上
					m_xPolyline.m_ptArr[1].SetPoint(endPt.x,startPt.y);	//右上
					m_xPolyline.m_ptArr[2]=endPt;						//右下
					m_xPolyline.m_ptArr[3].SetPoint(startPt.x,endPt.y);	//左下
				}
			}
			m_xPolyline.m_iActivePt=-1;
			m_xPolyline.m_cState=POLYLINE_4PT::STATE_VALID;
			//TODO:自动定位左下角与右上角顶点坐标
			int yjHoriTipOffset=0;
			double dfTurnGradientToOrthoImg=0;
			int width=m_xPolyline.m_ptArr[1].x-m_xPolyline.m_ptArr[0].x;
			for(int i=0;i<4;i++)
			{
				m_xPolyline.m_ptArr[i]=IntelliAnchorCrossHairPoint(m_xPolyline.m_ptArr[i],i,&dfTurnGradientToOrthoImg);
				if(i==0)
				{
					double ctana=1/(1+dfTurnGradientToOrthoImg*dfTurnGradientToOrthoImg);
					//width=f2i(width*ctana);
				}
				if(i==1)
				{
					double dy=m_xPolyline.m_ptArr[1].y-m_xPolyline.m_ptArr[0].y;
					double dx=m_xPolyline.m_ptArr[1].x-m_xPolyline.m_ptArr[0].x;
					if(fabs(dfTurnGradientToOrthoImg-dy/dx)>0.01)
					{	//需要修正右上角锚点
						m_xPolyline.m_ptArr[1].x=m_xPolyline.m_ptArr[0].x+width;
						m_xPolyline.m_ptArr[1].y=m_xPolyline.m_ptArr[0].y+f2i(width*dfTurnGradientToOrthoImg)-10;
						m_xPolyline.m_ptArr[1]=IntelliAnchorCrossHairPoint(m_xPolyline.m_ptArr[1],1);
					}
				}
				else if(i==3)
				{
					double dy=m_xPolyline.m_ptArr[2].y-m_xPolyline.m_ptArr[3].y;
					double dx=m_xPolyline.m_ptArr[2].x-m_xPolyline.m_ptArr[3].x;
					if(fabs(dfTurnGradientToOrthoImg-dy/dx)>0.01)
					{	//需要修正左下角锚点
						m_xPolyline.m_ptArr[3].x=m_xPolyline.m_ptArr[2].x-width;
						m_xPolyline.m_ptArr[3].y=m_xPolyline.m_ptArr[2].y-f2i(width*dfTurnGradientToOrthoImg)+10;
						m_xPolyline.m_ptArr[3]=IntelliAnchorCrossHairPoint(m_xPolyline.m_ptArr[3],3);
					}
				}
			}
			InvalidateRect(&rect,FALSE);
			SetFocus();
			//框选完成之后，跳转至第一个节点，细调
			CIBOMView *pBomView=theApp.GetIBomView();
			if(pBomView)
			{
				pBomView->SetPinpointIndex(0);
				ZoomInPolyPointAt(0);
			}
		}
		if(pActiveImgFile)
			pActiveImgFile->m_xPolyline=m_xPolyline;
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
			BYTE byteArr[4]={0x01,0x02,0x04,0x08};	//上下左右
			if(( offset.cy<0&&!(byteArr[0]&byteState))||
				(offset.cy>0&&!(byteArr[1]&byteState)))
				offset.cy=0;	//不能上或下移
			if(( offset.cx<0&&!(byteArr[2]&byteState))||
				(offset.cx>0&&!(byteArr[3]&byteState)))
				offset.cx=0;	//不能左或右移
			if(pImage&&(abs(offset.cx)>2||abs(offset.cy)>2)&&!pImage->IsCanZoom())
			{	//可以拖动
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
			BYTE byteArr[4]={0x01,0x02,0x04,0x08};	//上下左右
			if(( offset.cy<0&&!(byteArr[0]&byteState))||
				(offset.cy>0&&!(byteArr[1]&byteState)))
				offset.cy=0;	//不能上或下移
			//图片区域不支持左右移动 wht 18-03-26
			//if(( offset.cx<0&&!(byteArr[2]&byteState))||
			//	(offset.cx>0&&!(byteArr[3]&byteState)))
				offset.cx=0;	//不能左或右移
			if((abs(offset.cx)>2||abs(offset.cy)>2)&&!pRegion->IsCanZoom())
			{	//可以拖动
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
		BYTE byteArr[4]={0x01,0x02,0x04,0x08};	//上下左右
		if(( offset.cy<0&&!(byteArr[0]&byteState))||
			(offset.cy>0&&!(byteArr[1]&byteState)))
			offset.cy=0;	//不能上或下移
		if(( offset.cx<0&&!(byteArr[2]&byteState))||
			(offset.cx>0&&!(byteArr[3]&byteState)))
			offset.cx=0;	//不能左或右移
		if(pImage&&(abs(offset.cx)>2||abs(offset.cy)>2)&&!pImage->IsCanZoom())
		{	//可以拖动
			pImage->SetScrOffset(CSize(pImage->GetScrOffset())+offset);
			InvalidateRect(&rect,FALSE);
		}
	}
	else if(m_cDisType==CDataCmpModel::IMAGE_REGION_DATA&&m_biLayoutType==1)
	{
		CImageRegionHost *pRegion=(CImageRegionHost*)m_pData;
		CRect imageRect;
		BYTE byteState=GetOffsetState(rect,imageRect);
		BYTE byteArr[4]={0x01,0x02,0x04,0x08};	//上下左右
		if(( offset.cy<0&&!(byteArr[0]&byteState))||
			(offset.cy>0&&!(byteArr[1]&byteState)))
			offset.cy=0;	//不能上或下移
		//图片区域不支持左右移动 wht 18-03-26
		//if(( offset.cx<0&&!(byteArr[2]&byteState))||
		//	(offset.cx>0&&!(byteArr[3]&byteState)))
			offset.cx=0;	//不能左或右移
		if((abs(offset.cx)>2||abs(offset.cy)>2)&&!pRegion->IsCanZoom())
		{	//可以拖动
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
		pWorkPanel->SetFocus();		//设置焦点
	else
		m_listDataCtrl.SetFocus();
	m_hCursor=LoadCursor(NULL,IDC_ARROW);
	if(GetCurTask()==TASK_OTHER)
	{
		if(m_bHorzTracing&&point.y>(m_rcClient.top+50)&&point.y<(m_rcClient.bottom-50))
		{	//按下鼠标左键进行水平移动
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
	{	//图片区域处理操作
		CIBOMView *pBomView=theApp.GetIBomView();
		BOOL bPinpoint=pBomView?pBomView->IsCanPinpoint():FALSE;
		if(m_xPolyline.m_cState==POLYLINE_4PT::STATE_VALID&&	//选择框有效
			!(nFlags&MK_LBUTTON)&&!(nFlags&MK_RBUTTON)&&!bPinpoint)		//未按下左或右键
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
		else if(m_xPolyline.m_cState==POLYLINE_4PT::STATE_VALID&&(nFlags&MK_LBUTTON||bPinpoint)&&
			m_xPolyline.m_iActivePt>=0&&m_xPolyline.m_iActivePt<=4)
		{	//按下鼠标左键，对激活点进行移动
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
		{	//按下鼠标左键移动，初始化拾取区域
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
		else if(m_cDisType==CDataCmpModel::IMAGE_REGION_DATA&&m_biLayoutType==1)
		{
			CImageRegionHost *pRegion=(CImageRegionHost*)m_pData;
			::SetCursor(m_hCursor=m_hMove);
			//图片区域不支持左右移动 wht 18-03-26
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
	else if(m_cDisType==CDataCmpModel::IMAGE_REGION_DATA&&m_biLayoutType==1)
	{
		//::SetCursor(m_hHorzSize);
		CImageRegionHost *pRegion=(CImageRegionHost*)m_pData;
		//pRegion->Zoom(zDelta,&pointToPanel);
		SIZE offset=pRegion->GetScrOffset();
		double fRealHeight=pRegion->GetHeight()*pRegion->ZoomRatio();
		double fRectHeight=0.9*rect.Height();
		CRect headerRect;
		m_listDataCtrl.GetHeaderCtrl()->GetWindowRect(&headerRect);
		int maxOffsetY=0;//(m_biLayoutType==2)?headerRect.Height():0;
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
	{
		m_xPolyline.m_iActivePt=-1;
		OperOther();
	}
	else if((nChar==VK_SPACE||nChar==VK_RETURN)&&m_xPolyline.m_cState==POLYLINE_4PT::STATE_VALID)
	{
		CIBOMView *pBomView=theApp.GetIBomView();
		int iActive=m_xPolyline.m_iActivePt;
 		if(m_xPolyline.m_iActivePt>=0&&m_xPolyline.m_iActivePt<3&&pBomView&&pBomView->IsCanPinpoint())
		{
			m_xPolyline.m_iActivePt++;
			pBomView->SetPinpointIndex(m_xPolyline.m_iActivePt);
			ZoomInPolyPointAt(m_xPolyline.m_iActivePt);
		}
		else
		{
			m_xPolyline.m_iActivePt=-1;
			pBomView->SetPinpointIndex(-1);
			ZoomInPolyPointAt(-1);
		}
	}
	CRect rect;
	CWnd* pWorkPanel=GetWorkPanelRect(rect);
	InvalidateRect(&rect,FALSE);
	CDialog::OnKeyDown(nChar, nRepCnt, nFlags);
}

BOOL CDisplayDataPage::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if(m_hCursor!=NULL)
	{
		SetCursor(m_hCursor);
		return TRUE;// TODO: 在此添加消息处理程序代码和/或调用默认值
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
	sCoef.Format("明%d",nPos);
	SetDlgItemText(IDC_S_BALANCE_COEF,sCoef);
	*pResult = 0;
}

void CDisplayDataPage::OnNMReleasedcaptureSliderBalanceCoef(NMHDR *pNMHDR, LRESULT *pResult)
{
	CRect rect;
	CWaitCursor waiting;
	CWnd* pWorkPanel=GetWorkPanelRect(rect);
	/*if(m_cDisType==CDataCmpModel::DWG_IMAGE_DATA)
	{
		CImageFileHost* pImage=(CImageFileHost*)m_pData;
		pImage->SetMonoThresholdBalanceCoef((_localPrevSliderPosition-50)*0.02,true);
		InvalidateRect(&rect,FALSE);
	}
	else*/
	if(m_cDisType==CDataCmpModel::IMAGE_REGION_DATA)
	{
		CImageRegionHost *pRegion=(CImageRegionHost*)m_pData;
		pRegion->SetMonoThresholdBalanceCoef((_localPrevSliderPosition-50)*0.02);
		InvalidateRect(&rect,FALSE);
	}
	ClearImage();
	*pResult = 0;
}

static int _localPrevZoomScale=200;
void CDisplayDataPage::OnNMCustomdrawSliderZoomScale(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	int nPos = m_sliderZoomScale.GetPos();
	if(nPos==_localPrevZoomScale)
		return;
	_localPrevZoomScale=nPos;
	/*CString sCoef;
	sCoef.Format("明%d",nPos);
	SetDlgItemText(IDC_S_BALANCE_COEF,sCoef);*/
	*pResult = 0;
}

void CDisplayDataPage::OnNMReleasedcaptureSliderZoomScale(NMHDR *pNMHDR, LRESULT *pResult)
{
	CRect rect;
	CWaitCursor waiting;
	CWnd* pWorkPanel=GetWorkPanelRect(rect);
	if(m_cDisType==CDataCmpModel::DWG_IMAGE_DATA)
	{
		CImageFileHost* pImageFile=(CImageFileHost*)m_pData;
		CIBOMDoc *pDoc=theApp.GetDocument();
		if(pDoc!=NULL&&pImageFile!=NULL)
		{
			CWaitCursor waitCursor;
			pImageFile->fPDFZoomScale=_localPrevZoomScale*0.01;
			pImageFile->InitImageFile(pImageFile->szPathFileName,pDoc->GetFilePath());
			InvalidateRect(&rect,FALSE);
			ZoomAll();
		}
	}
	ClearImage();
	*pResult = 0;
}

void CDisplayDataPage::InvalidateImageWorkPanel()
{
	CRect rect;
	CWaitCursor waiting;
	CWnd* pWorkPanel=GetWorkPanelRect(rect);
	if(m_cDisType==CDataCmpModel::DWG_IMAGE_DATA)
	{
		CImageFileHost* pImageFile=(CImageFileHost*)m_pData;
		CIBOMDoc *pDoc=theApp.GetDocument();
		if(pDoc!=NULL&&pImageFile!=NULL)
			InvalidateRect(&rect,FALSE);
	}
	ClearImage();
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
	ClearImage();
	RelayoutWnd();
	//m_listDataCtrl.EnableSortItems((m_biLayoutType==3)?false:true);
	CImageRegionHost *pRegion=(CImageRegionHost*)m_pData;
	IRecoginizer::BOMPART *pBomPart=pRegion?pRegion->EnumFirstPart():NULL;
	CSuperGridCtrl::CTreeItem *pSelItem=RefreshListCtrl((long)pBomPart);
	Focus(&m_listDataCtrl,pSelItem);
	Invalidate(FALSE);
}

int CDisplayDataPage::GetHeadOrTailValidItem(CListCtrl *pListCtrl,int iCurSel/*=-1*/)
{
	if(iCurSel==LISTCTRL_HEAD_ITEM)
		return biLayoutType==3?GetHeadFixRowCount():0;
	else if(iCurSel==LISTCTRL_TAIL_ITEM)
		return biLayoutType==3?pListCtrl->GetItemCount()-GetTailFixRowCount()-1:pListCtrl->GetItemCount()-1;
	else 
	{
		if(biLayoutType==LAYOUT_COLREV)
		{
			if(iCurSel<CDisplayDataPage::GetHeadFixRowCount())
				iCurSel=CDisplayDataPage::GetHeadFixRowCount();
			else if(iCurSel>pListCtrl->GetItemCount()-CDisplayDataPage::GetTailFixRowCount())
				iCurSel=pListCtrl->GetItemCount()-CDisplayDataPage::GetTailFixRowCount()-1;
		}
		return iCurSel;
	}
}

void CDisplayDataPage::InitPolyLine()
{
	m_xPolyline.Init();
	if(pActiveImgFile!=NULL)
	{
		pActiveImgFile->m_xPolyline.m_cState=m_xPolyline.m_cState;
		pActiveImgFile->m_xPolyline.m_iActivePt=-1;
	}
}

void CDisplayDataPage::OnRButtonDown(UINT nFlags, CPoint point)
{
	TVHITTESTINFO HitTestInfo;
	GetCursorPos(&HitTestInfo.pt);
	CRect rect;
	GetWorkPanelRect(rect);
	if(m_xPolyline.m_iActivePt>=0)
	{
		m_xPolyline.m_iActivePt=-1;
		InvalidateRect(&rect,FALSE);
	}
	if((m_biLayoutType==1||m_biLayoutType==2)&&rect.PtInRect(point))
	{	//工作区添加右键菜单
		CMenu popMenu;
		popMenu.LoadMenu(IDR_ITEM_CMD_POPUP);
		CMenu *pMenu=popMenu.GetSubMenu(0);
		while(pMenu->GetMenuItemCount()>0)
			pMenu->DeleteMenu(0,MF_BYPOSITION);
		pMenu->AppendMenuA(MF_STRING,ID_RETRIEVE_DATA,"提取数据");
		pMenu->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON,HitTestInfo.pt.x,HitTestInfo.pt.y,this);
	}
	CDialog::OnRButtonDown(nFlags, point);
}

void CDisplayDataPage::OnRetrieveData()
{
	CFileTreeDlg *pFilePage=((CMainFrame*)AfxGetMainWnd())->GetFileTreePage();
	pFilePage->RetrieveData();
}

bool CDisplayDataPage::ZoomInPolyPointAt(int index)
{
	if(m_xPolyline.m_cState!=POLYLINE_4PT::STATE_VALID)
		return false;
	if(m_biLayoutType!=1||m_xPolyline.m_cState!=POLYLINE_4PT::STATE_VALID)
		return false;
	CImageFileHost *pFile=(CImageFileHost*)m_pData;
	if(pFile==NULL)
		return false;
	CRect panelRect;
	GetWorkPanelRect(panelRect);
	if(index>=0&&index<=3)
	{
		CPoint pt=m_xPolyline.m_ptArr[index];
		pFile->FocusPoint(pt,NULL,4.0);
		InvalidateRect(&panelRect,FALSE);
		//
		m_cCurTask=TASK_SNAP;
		CPoint screenPt=TransPoint(pt,FALSE);
		ClientToScreen(&screenPt);
		m_xPolyline.m_iActivePt=index;
		::SetCursorPos(screenPt.x,screenPt.y);
		::SetCursor(m_hMove);
		m_ptLBDownPos=screenPt;
	}
	else if(index==-1)
	{	//全显整个区域
		CPoint curPt=m_xPolyline.m_ptArr[0];
		curPt+=m_xPolyline.m_ptArr[1];
		curPt+=m_xPolyline.m_ptArr[2];
		curPt+=m_xPolyline.m_ptArr[3];
		curPt.x=f2i(curPt.x*0.25);
		curPt.y=f2i(curPt.y*0.25);
		double scale_h=(1.0*panelRect.Width()-30)/(m_xPolyline.m_ptArr[1].x-m_xPolyline.m_ptArr[0].x);
		double scale_v=(1.0*panelRect.Height()-30)/(m_xPolyline.m_ptArr[2].y-m_xPolyline.m_ptArr[1].y);
		double scale=min(fabs(scale_v),fabs(scale_h));	//取绝对值以防因为矩形错乱导致负比例值出现 wjh-2018.9.12
		pFile->FocusPoint(curPt,NULL,scale);
		InvalidateRect(&panelRect,FALSE);
		::SetCursor(m_hArrowCursor);
	}
	return true;
}


void CDisplayDataPage::OnDeltaposSliderSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	int nOrgPos = m_sliderBalanceCoef.GetPos(),nCurPos=nOrgPos;
	if(pNMUpDown->iDelta<0)
		nCurPos=min(nCurPos+1,100);
	else if(pNMUpDown->iDelta>0)
		nCurPos=max(nCurPos-1,0);
	if(nCurPos!=nOrgPos)
	{
		m_sliderBalanceCoef.SetPos(nCurPos);
		CString sCoef;
		sCoef.Format("明%d",nCurPos);
		_localPrevSliderPosition=nCurPos;
		SetDlgItemText(IDC_S_BALANCE_COEF,sCoef);
		if(m_cDisType==CDataCmpModel::IMAGE_REGION_DATA)
		{
			CRect rect;
			CWaitCursor waiting;
			CWnd* pWorkPanel=GetWorkPanelRect(rect);
			CImageRegionHost *pRegion=(CImageRegionHost*)m_pData;
			pRegion->SetMonoThresholdBalanceCoef((_localPrevSliderPosition-50)*0.02);
			InvalidateRect(&rect,FALSE);
			ClearImage();
		}
	}
	*pResult = 0;
}

void CDisplayDataPage::OnSelchangeTabGroup(NMHDR* pNMHDR, LRESULT* pResult) 
{
	RelayoutWnd();
}

void CDisplayDataPage::OnSelectFolder()
{
	if(m_listFileCtrl.GetItemCount()==0)
		return;
	char ss[MAX_PATH]="";
	GetAppPath(ss);
	CString sWorkPath=ss;
	if(!InvokeFolderPickerDlg(sWorkPath))
		return;
	for(int i=0;i<m_listFileCtrl.GetItemCount();i++)
	{
		CSuperGridCtrl::CTreeItem *pItem=m_listFileCtrl.GetTreeItem(i);
		if(pItem==NULL)
			continue;
		CImageFileHost *pFileHost=(CImageFileHost*)pItem->m_idProp;
		if(pFileHost==NULL)
			continue;
		CString sFileName=sWorkPath+"\\"+pFileHost->szFileName;
		pFileHost->InitImageFileHeader(sFileName);
		m_listFileCtrl.SetSubItemText(pItem,1,sFileName);
	}
}

bool CDisplayDataPage::ZoomAll()
{
	if(m_biLayoutType!=1)
		return false;
	CRect rcPanelClient;
	GetWorkPanelRect(rcPanelClient);
	CImageFileHost *pFile=(CImageFileHost*)m_pData;
	CPoint point;
	point.x=(int)(pFile->Width*0.5);
	point.y=(int)(pFile->Height*0.5);
	double fHeightScale=(1.0*rcPanelClient.Height())/pFile->Height;
	double fWidthScale=(1.0*rcPanelClient.Width())/pFile->Width;
	double fScale=min(fHeightScale,fWidthScale);
	pFile->FocusPoint(point,NULL,fScale);
	InvalidateRect(&rcPanelClient,FALSE);
	return true;
}

BOOL CDisplayDataPage::MarkDrawingError(BYTE flag)
{
	if(m_biLayoutType!=2&&m_biLayoutType!=3)
		return FALSE;
	int iItem=m_listDataCtrl.GetSelectedItem();
	CSuperGridCtrl::CTreeItem *pItem=m_listDataCtrl.GetTreeItem(iItem);
	if(pItem==NULL)
		return FALSE;
	int iCurCol=m_listDataCtrl.GetCurSubItem();
	if(m_biLayoutType==3)
		iCurCol=biCurCol;
	if(iCurCol<0||iCurCol>6)
		return FALSE;
	IRecoginizer::BOMPART *pBomPart=(IRecoginizer::BOMPART*)pItem->m_idProp;
	if(pBomPart==NULL)
		return FALSE;
	pBomPart->arrItemWarningLevel[iCurCol]=flag;
	COLORREF clr=GetBomPartCellBackColor(m_listDataCtrl,pBomPart,iCurCol);
	if(clr==0)
		clr=RGB(255,255,255);
	pItem->m_lpNodeInfo->SetSubItemColor(iCurCol,clr);
	return TRUE;
}

void CDisplayDataPage::OnMarkNormal()
{
	MarkDrawingError(0);	
}

void CDisplayDataPage::OnMarkError()
{
	MarkDrawingError(0xFF);
}
void CDisplayDataPage::OnAddPart()
{
	if(m_biLayoutType!=LAYOUT_BROWSE)
		return;
	CImageRegionHost *pRegion=(CImageRegionHost*)m_pData;
	if(pRegion==NULL)
		return;
	IRecoginizer::BOMPART *pBomPart=pRegion->AddInputPart();
	pBomPart->bInput=true;
	CSuperGridCtrl::CTreeItem *pNewItem=InsertOrUpdatePartTreeItem(m_listDataCtrl,pBomPart,NULL,TRUE);
	m_listDataCtrl.SelectItem(pNewItem);
}
void CDisplayDataPage::OnDeletePart()
{
	if(m_biLayoutType!=LAYOUT_BROWSE)
		return;
	CImageRegionHost *pRegion=(CImageRegionHost*)m_pData;
	if(pRegion==NULL)
		return;
	int iItem=m_listDataCtrl.GetSelectedItem();
	CSuperGridCtrl::CTreeItem *pItem=m_listDataCtrl.GetTreeItem(iItem);
	if(pItem==NULL)
		return;
	IRecoginizer::BOMPART *pBomPart=(IRecoginizer::BOMPART*)pItem->m_idProp;
	if(pBomPart==NULL||!pBomPart->bInput)
		return;
	if(pRegion->DeleteInputPart(pBomPart->id))
		m_listDataCtrl.DeleteItem(pItem->GetIndex());
}

void CDisplayDataPage::OnCopyItem()
{
	if(m_biLayoutType!=LAYOUT_BROWSE)
		return;
	int iItem=m_listDataCtrl.GetSelectedItem();
	CSuperGridCtrl::CTreeItem *pItem=m_listDataCtrl.GetTreeItem(iItem);
	if(pItem==NULL)
		return;
	IRecoginizer::BOMPART *pBomPart=(IRecoginizer::BOMPART*)pItem->m_idProp;
	if(pBomPart==NULL)
		return;
	m_bCanPasteItem=m_listDataCtrl.CopyDataToClipboard();
}

void CDisplayDataPage::OnPasteItem()
{
	if(m_biLayoutType!=LAYOUT_BROWSE)
		return;
	if(!m_bCanPasteItem)
		return;
	int iItem=m_listDataCtrl.GetSelectedItem();
	CSuperGridCtrl::CTreeItem *pItem=m_listDataCtrl.GetTreeItem(iItem);
	if(pItem==NULL)
		return;
	IRecoginizer::BOMPART *pBomPart=(IRecoginizer::BOMPART*)pItem->m_idProp;
	if(pBomPart==NULL||!pBomPart->bInput)
		return;
	int nColCount=m_listDataCtrl.GetColumnCount();
	if(m_listDataCtrl.PasteDataFromClipboard(0,NULL,FALSE,FALSE))
	{
		strcpy(pBomPart->sLabel,pItem->m_lpNodeInfo->GetSubItemText(0));
		strcpy(pBomPart->materialStr,pItem->m_lpNodeInfo->GetSubItemText(1));
		strcpy(pBomPart->sSizeStr,pItem->m_lpNodeInfo->GetSubItemText(2));
		pBomPart->length=atof(pItem->m_lpNodeInfo->GetSubItemText(3));
		pBomPart->count=atoi(pItem->m_lpNodeInfo->GetSubItemText(4));
		pBomPart->weight=atof(pItem->m_lpNodeInfo->GetSubItemText(5));
		ParseSpec(pBomPart);
	}
}