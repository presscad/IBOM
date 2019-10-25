// FileTreeDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "IBOM.h"
#include "FileTreeDlg.h"
#include "image.h"
#include "Recog.h"
#include "XhCharString.h"
#include "IBOMView.h"
#include "DataModel.h"
#include "MainFrm.h"
#include "InputAnValDlg.h"
#include "Tool.h"
#include "folder_dialog.h"
#include "InputDlg.h"
#include "ParseAdaptNo.h"
#include "InputPrefixAndSuffixDlg.h"
#include "ComparePartNoString.h"
#include "SortFunc.h"
#include "SettingDlg.h"
#include "EnlargeFileDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// CFileTreeDlg 对话框
static BOOL IsValidDragS(CXhTreeCtrl *pTreeCtrl, HTREEITEM hItemDragS)
{
	if(hItemDragS==NULL)
		return FALSE;
	if(pTreeCtrl->ItemHasChildren(hItemDragS))
		return FALSE;
	//允许拖动图片
	TREEITEM_INFO *pItemInfo=(TREEITEM_INFO*)pTreeCtrl->GetItemData(hItemDragS);
	if(pItemInfo==NULL || pItemInfo->itemType!=CDataCmpModel::DWG_IMAGE_DATA)
		return FALSE;
	return TRUE;
}

static BOOL IsValidDragD(CXhTreeCtrl *pTreeCtrl, HTREEITEM hItemDragS, HTREEITEM hItemDragD)
{
	if(hItemDragS==NULL||hItemDragD==NULL)
		return FALSE;
	TREEITEM_INFO *pItemInfoS=(TREEITEM_INFO*)pTreeCtrl->GetItemData(hItemDragS);
	TREEITEM_INFO *pItemInfoD=(TREEITEM_INFO*)pTreeCtrl->GetItemData(hItemDragD);
	if(pItemInfoS==NULL||pItemInfoD==NULL)
		return FALSE;
	if(pItemInfoS->itemType==CDataCmpModel::DWG_IMAGE_DATA && pItemInfoD->itemType==CDataCmpModel::IMAGE_SEG_GROUP)
		return TRUE;
	return FALSE;
}

static int CompareDataLevel(CXhTreeCtrl *pTreeCtrl, HTREEITEM hItemDragS, HTREEITEM hItemDragD)
{
	if(hItemDragS==NULL||hItemDragD==NULL)
		return 0;
	TREEITEM_INFO *pItemInfoS=(TREEITEM_INFO*)pTreeCtrl->GetItemData(hItemDragS);
	TREEITEM_INFO *pItemInfoD=(TREEITEM_INFO*)pTreeCtrl->GetItemData(hItemDragD);
	if(pItemInfoS==NULL||pItemInfoD==NULL)
		return 0;
	if(pItemInfoS->itemType==CDataCmpModel::DWG_IMAGE_DATA && pItemInfoD->itemType==CDataCmpModel::IMAGE_SEG_GROUP)
	{
		CImageFileHost* pImageHost=(CImageFileHost*)pItemInfoS->dwRefData;
		SEGITEM* pSegItem=(SEGITEM*)pItemInfoD->dwRefData;
		pImageHost->m_iSeg=pSegItem->iSeg;
		return 1;
	}
	return 0;
}

IMPLEMENT_DYNCREATE(CFileTreeDlg, CDialog)

CFileTreeDlg::CFileTreeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFileTreeDlg::IDD, pParent)
{
	m_pSelItemInfo=NULL;
	bEditState=FALSE;
	m_hDwgBomGroup=NULL;
	m_hImageGroup=NULL;
	m_hLoftBomGroup=NULL;
	m_hLoftTmaGroup=NULL;
	m_hSelect=NULL;
}

CFileTreeDlg::~CFileTreeDlg()
{
}

void CFileTreeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE_CTRL, m_xFileCtrl);
}


BEGIN_MESSAGE_MAP(CFileTreeDlg, CDialog)
	ON_WM_SIZE()
	ON_NOTIFY(NM_RCLICK, IDC_TREE_CTRL, &OnNMRClickTreeCtrl)
	ON_NOTIFY(NM_CLICK, IDC_TREE_CTRL, &OnNMClickTreeCtrl)
	ON_COMMAND(ID_COMPARE_DATA,OnCompareData)
	ON_COMMAND(ID_EXPORT_RESULT,OnExportCompResult)
	ON_COMMAND(ID_IMPORT_IMAGE_FOLDER,OnImportImageFolder)
	ON_COMMAND(ID_IMPORT_IMAGE_FILE,OnImportImageFile)
	ON_COMMAND(ID_IMPORT_LOFT_BOM,OnImportLoftBom)
	ON_COMMAND(ID_ADD_SEG_DATA,OnAddSegData)
	ON_COMMAND(ID_RENAME_SEGI,OnRenameItem)
	ON_COMMAND(ID_IMPORT_DWG_BOM,OnImportDwgBom)
	ON_COMMAND(ID_RETRIEVE_DATA,OnRetrieveData)
	ON_COMMAND(ID_EXPORT_IMAGE_FILE,OnExportImageFile)
	ON_COMMAND(ID_DELETE_ITEM,OnDeleteItem)
	ON_COMMAND(ID_EXPORT_DWG_BOM,OnExportDwgExcel)
	ON_COMMAND(ID_MOVE_TO_TOP,OnMoveToTop)
	ON_COMMAND(ID_MOVE_UP,OnMoveUp)
	ON_COMMAND(ID_MOVE_DOWN,OnMoveDown)
	ON_COMMAND(ID_MOVE_TO_BOTTOM,OnMoveToBottom)
	ON_COMMAND(ID_EDIT_ITEM_PROP,OnEditItemProp)
	ON_NOTIFY(TVN_BEGINLABELEDIT, IDC_TREE_CTRL,OnBeginLabelEditTree)
	ON_NOTIFY(TVN_ENDLABELEDIT, IDC_TREE_CTRL,OnEndLabelEditTree)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_CTRL, &CFileTreeDlg::OnTvnSelchangedTreeCtrl)
END_MESSAGE_MAP()


// CFileTreeDlg 消息处理程序
BOOL CFileTreeDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	//初始化树列表
	m_imageList.Create(IDB_IL_PROJECT, 16, 1, RGB(0,255,0));
	m_xFileCtrl.SetImageList(&m_imageList,TVSIL_NORMAL);
	m_xFileCtrl.ModifyStyle(0,TVS_HASLINES|TVS_HASBUTTONS);
	m_xFileCtrl.SetIsValidDragSFunc(IsValidDragS);
	m_xFileCtrl.SetIsValidDragDFunc(IsValidDragD);
	m_xFileCtrl.SetCompareDataLevelFunc(CompareDataLevel);
	RefreshTreeCtrl();
	return TRUE;
}
void CFileTreeDlg::RefreshTreeCtrl()
{
	CString ss;
	CXhChar100 sName;
	itemInfoList.Empty();
	m_xFileCtrl.DeleteAllItems();
	m_hImageGroup=NULL;
	m_hDwgBomGroup=NULL;
	m_hLoftBomGroup=NULL;
	m_hLoftTmaGroup=NULL;
	//
	TREEITEM_INFO *pItemInfo;
	//图纸数据节点
	HTREEITEM hDwgGroup=m_xFileCtrl.InsertItem("图纸数据",PRJ_IMG_CALMODULE,PRJ_IMG_CALMODULE,TVI_ROOT);
	pItemInfo=itemInfoList.append(TREEITEM_INFO(CDataCmpModel::DWG_DATA_MODEL,NULL));
	m_xFileCtrl.SetItemData(hDwgGroup,(DWORD)pItemInfo);
	InsertImageGroup(hDwgGroup);	//添加"图纸数据->图像文件"树节点
	InsertDwgBomGroup(hDwgGroup);	//添加"图纸数据->明细文件"树节点
	if(dataModel.GetBomImageNum()>0)
	{
		RefreshImageTreeCtrl();
		m_xFileCtrl.Expand(m_hImageGroup,TVE_EXPAND);
	}
	if(dataModel.GetDwgBomNum()>0)
	{
		for(CBomFileData* pBomFile=dataModel.EnumFirstDwgBom();pBomFile;pBomFile=dataModel.EnumNextDwgBom())
		{
			_splitpath(pBomFile->m_sName,NULL,NULL,sName,NULL);
			HTREEITEM hSonItem=m_xFileCtrl.InsertItem(sName,PRJ_IMG_FILE,PRJ_IMG_FILE,m_hDwgBomGroup);
			pItemInfo=itemInfoList.append(TREEITEM_INFO(CDataCmpModel::DWG_BOM_DATA,(DWORD)pBomFile));
			m_xFileCtrl.SetItemData(hSonItem,(DWORD)pItemInfo);
		}
		m_xFileCtrl.Expand(m_hDwgBomGroup,TVE_EXPAND);
	}
	//模型数据节点
	HTREEITEM hLoftGroup=m_xFileCtrl.InsertItem("放样数据",PRJ_IMG_MODULECASE,PRJ_IMG_MODULECASE,TVI_ROOT);
	pItemInfo=itemInfoList.append(TREEITEM_INFO(CDataCmpModel::LOFT_DATA_MODEL,NULL));
	m_xFileCtrl.SetItemData(hLoftGroup,(DWORD)pItemInfo);
	if(dataModel.GetLoftBomNum()>0)
	{
		InsertLoftBomGroup(hLoftGroup);
		for(CBomFileData* pBomFile=dataModel.EnumFirstLoftBom();pBomFile;pBomFile=dataModel.EnumNextLoftBom())
		{
			_splitpath(pBomFile->m_sName,NULL,NULL,sName,NULL);
			HTREEITEM hSonItem=m_xFileCtrl.InsertItem(sName,PRJ_IMG_FILE,PRJ_IMG_FILE,m_hLoftBomGroup);
			pItemInfo=itemInfoList.append(TREEITEM_INFO(CDataCmpModel::LOFT_BOM_DATA,(DWORD)pBomFile));
			m_xFileCtrl.SetItemData(hSonItem,(DWORD)pItemInfo);
		}
		m_xFileCtrl.Expand(m_hLoftBomGroup,TVE_EXPAND);
	}
	HTREEITEM hDataCompareItem=m_xFileCtrl.InsertItem("数据对比",PRJ_IMG_CALMODULE,PRJ_IMG_CALMODULE,TVI_ROOT);
	pItemInfo=itemInfoList.append(TREEITEM_INFO(CDataCmpModel::DATA_CMP_MODEL,NULL));
	m_xFileCtrl.SetItemData(hDataCompareItem,(DWORD)pItemInfo);

	m_xFileCtrl.Expand(hDwgGroup,TVE_EXPAND);
	m_xFileCtrl.Expand(hLoftGroup,TVE_EXPAND);
	m_xFileCtrl.Expand(hDataCompareItem,TVE_EXPAND);
	m_xFileCtrl.SelectItem(m_hImageGroup);
}
void CFileTreeDlg::RefreshImageTreeCtrl()
{
	if(m_hImageGroup==NULL)
		return;
	TREEITEM_INFO *pItemInfo=NULL;
	HTREEITEM hSegItem,hImageHost,hImageRegion;
	for(SEGITEM* pSeg=dataModel.EnumFirstSeg();pSeg;pSeg=dataModel.EnumNextSeg())
	{
		hSegItem=m_xFileCtrl.InsertItem(pSeg->sSegName,PRJ_IMG_WINDLOADPARA,PRJ_IMG_WINDLOADPARA,m_hImageGroup);
		pItemInfo=itemInfoList.append(TREEITEM_INFO(CDataCmpModel::IMAGE_SEG_GROUP,(DWORD)pSeg));
		m_xFileCtrl.SetItemData(hSegItem,(DWORD)pItemInfo);
		for(CImageFileHost* pImageHost=dataModel.EnumFirstImage();pImageHost;pImageHost=dataModel.EnumNextImage())
		{
			if(pImageHost->m_iSeg!=pSeg->iSeg)
				continue;
			//_splitpath(pImageHost->m_sPathFileName,NULL,NULL,sName,NULL);
			//CXhChar100 sName=pImageHost->szFileName;
			hImageHost=m_xFileCtrl.InsertItem(pImageHost->szAliasName,PRJ_IMG_FILE,PRJ_IMG_FILE,hSegItem);
			pItemInfo=itemInfoList.append(TREEITEM_INFO(CDataCmpModel::DWG_IMAGE_DATA,(DWORD)pImageHost));
			m_xFileCtrl.SetItemData(hImageHost,(DWORD)pItemInfo);
			for(CImageRegionHost* pRegion=pImageHost->EnumFirstRegionHost();pRegion;pRegion=pImageHost->EnumNextRegionHost())
			{
				hImageRegion=m_xFileCtrl.InsertItem(pRegion->m_sName,PRJ_IMG_PARA,PRJ_IMG_PARA,hImageHost);
				pItemInfo=itemInfoList.append(TREEITEM_INFO(CDataCmpModel::IMAGE_REGION_DATA,(DWORD)pRegion));
				m_xFileCtrl.SetItemData(hImageRegion,(DWORD)pItemInfo);
			}
		}
		m_xFileCtrl.Expand(hSegItem,TVE_EXPAND);
	}
}
void CFileTreeDlg::InsertImageGroup(HTREEITEM hParentItem)
{
	if(m_hImageGroup || hParentItem==NULL)
		return;
	m_hImageGroup=m_xFileCtrl.InsertItem("BOM图像文件",PRJ_IMG_FILEGROUP,PRJ_IMG_FILEGROUP,hParentItem);
	TREEITEM_INFO *pItemInfo=itemInfoList.append(TREEITEM_INFO(CDataCmpModel::DWG_IMAGE_GROUP,NULL));
	m_xFileCtrl.SetItemData(m_hImageGroup,(DWORD)pItemInfo);
}
void CFileTreeDlg::InsertDwgBomGroup(HTREEITEM hParentItem)
{
	if(m_hDwgBomGroup || hParentItem==NULL)
		return;
	m_hDwgBomGroup=m_xFileCtrl.InsertItem("BOM明细文件",PRJ_IMG_FILEGROUP,PRJ_IMG_FILEGROUP,hParentItem);
	TREEITEM_INFO *pItemInfo=itemInfoList.append(TREEITEM_INFO(CDataCmpModel::DWG_BOM_GROUP,NULL));
	m_xFileCtrl.SetItemData(m_hDwgBomGroup,(DWORD)pItemInfo);
}
void CFileTreeDlg::InsertLoftBomGroup(HTREEITEM hParentItem)
{
	if(m_hLoftBomGroup || hParentItem==NULL)
		return;
	m_hLoftBomGroup=m_xFileCtrl.InsertItem("BOM文件组",PRJ_IMG_FILEGROUP,PRJ_IMG_FILEGROUP,hParentItem);
	TREEITEM_INFO *pItemInfo=itemInfoList.append(TREEITEM_INFO(CDataCmpModel::LOFT_BOM_GROUP,NULL));
	m_xFileCtrl.SetItemData(m_hLoftBomGroup,(DWORD)pItemInfo);
}
HTREEITEM CFileTreeDlg::InsertImageSegItem(SEGITEM *pSegItem)
{
	if(m_hImageGroup==NULL)
		return NULL;
	HTREEITEM hCurItem=m_xFileCtrl.InsertItem(pSegItem->sSegName,PRJ_IMG_WINDLOADPARA,PRJ_IMG_WINDLOADPARA,m_hImageGroup);
	TREEITEM_INFO *pItemInfo=itemInfoList.append(TREEITEM_INFO(CDataCmpModel::IMAGE_SEG_GROUP,(DWORD)pSegItem));
	m_xFileCtrl.SetItemData(hCurItem,(DWORD)pItemInfo);
	m_xFileCtrl.Expand(hCurItem,TVE_EXPAND);
	return hCurItem;
}
BOOL CFileTreeDlg::GetSelItemInfo()
{
	m_pSelItemInfo=NULL;
	m_hSelect=m_xFileCtrl.GetSelectedItem();
	if(m_hSelect==NULL)
		return FALSE;
	m_pSelItemInfo=(TREEITEM_INFO*)m_xFileCtrl.GetItemData(m_hSelect);
	if(m_pSelItemInfo==NULL)
		return FALSE;
	return TRUE;
}
void CFileTreeDlg::RefreshView()
{
	if(GetSelItemInfo()!=NULL)
		theApp.GetIBomView()->ShiftView((BYTE)m_pSelItemInfo->itemType,m_pSelItemInfo->dwRefData);
}
void CFileTreeDlg::ContextMenu(CWnd *pWnd, CPoint point)
{
	if(m_pSelItemInfo==NULL)
		return;
	CMenu popMenu;
	popMenu.LoadMenu(IDR_ITEM_CMD_POPUP);
	CMenu *pMenu=popMenu.GetSubMenu(0);
	while(pMenu->GetMenuItemCount()>0)
		pMenu->DeleteMenu(0,MF_BYPOSITION);
	if(m_pSelItemInfo->itemType==CDataCmpModel::DATA_CMP_MODEL)
	{
		pMenu->AppendMenu(MF_STRING,ID_COMPARE_DATA,"数据对比");
		pMenu->AppendMenu(MF_STRING,ID_EXPORT_RESULT,"导出对比结果");
	}
	/*else if(m_pSelItemInfo->itemType==CDataCmpModel::DWG_DATA_MODEL)
	{
		pMenu->AppendMenu(MF_STRING,ID_IMPORT_IMAGE_FILE,"添加BOM图像文件");
		pMenu->AppendMenu(MF_STRING,ID_IMPORT_DWG_BOM,"导入构件明细文件");
	}*/
	else if(m_pSelItemInfo->itemType==CDataCmpModel::DWG_BOM_GROUP)	
	{
		pMenu->AppendMenu(MF_STRING,ID_IMPORT_DWG_BOM,"导入构件明细文件");
		pMenu->AppendMenu(MF_STRING,ID_EXPORT_DWG_BOM,"汇总生成Excel表");
	}
	else if (m_pSelItemInfo->itemType==CDataCmpModel::DWG_BOM_DATA)
		pMenu->AppendMenu(MF_STRING,ID_DELETE_ITEM,"删除BOM文件");
	else if(m_pSelItemInfo->itemType==CDataCmpModel::DWG_IMAGE_GROUP)
	{
		pMenu->AppendMenu(MF_STRING,ID_ADD_SEG_DATA,"添加段");
		pMenu->AppendMenu(MF_STRING,ID_IMPORT_IMAGE_FILE,"导入图片");
		pMenu->AppendMenu(MF_SEPARATOR,ID_SEPARATOR,"");
		pMenu->AppendMenu(MF_STRING,ID_EXPORT_DWG_BOM,"汇总生成Excel表");
	}
	else if (m_pSelItemInfo->itemType==CDataCmpModel::IMAGE_SEG_GROUP)
	{
		pMenu->AppendMenu(MF_STRING,ID_IMPORT_IMAGE_FILE,"导入图片");
		pMenu->AppendMenu(MF_SEPARATOR,ID_SEPARATOR,"");
		pMenu->AppendMenu(MF_STRING,ID_RENAME_SEGI,"修改段名");
		pMenu->AppendMenuA(MF_STRING,ID_DELETE_ITEM,"删除段");
		pMenu->AppendMenu(MF_SEPARATOR,ID_SEPARATOR,"");
		pMenu->AppendMenu(MF_STRING,ID_EXPORT_DWG_BOM,"汇总生成Excel表");
	}
	else if (m_pSelItemInfo->itemType==CDataCmpModel::DWG_IMAGE_DATA)
	{
		pMenu->AppendMenu(MF_STRING,ID_RETRIEVE_DATA,"提取数据");
		if(theApp.IsHasInternalTest())
			pMenu->AppendMenuA(MF_STRING,ID_EXPORT_IMAGE_FILE,"导出图片");
		pMenu->AppendMenuA(MF_STRING,ID_DELETE_ITEM,"删除图片");
		pMenu->AppendMenu(MF_SEPARATOR,ID_SEPARATOR,"");
		pMenu->AppendMenu(MF_STRING,ID_RENAME_SEGI,"重命名文件");
		pMenu->AppendMenu(MF_STRING,ID_EDIT_ITEM_PROP,"文件属性");
	}
	else if(m_pSelItemInfo->itemType==CDataCmpModel::IMAGE_REGION_DATA)
	{
		pMenu->AppendMenu(MF_STRING,ID_RETRIEVE_DATA,"提取数据");
		pMenu->AppendMenuA(MF_STRING,ID_EXPORT_IMAGE_FILE,"导出图片");
		pMenu->AppendMenu(MF_SEPARATOR,ID_SEPARATOR,"");
		pMenu->AppendMenu(MF_STRING,ID_RENAME_SEGI,"重命名区域");
		pMenu->AppendMenuA(MF_SEPARATOR,ID_SEPARATOR,"");
		pMenu->AppendMenuA(MF_STRING,ID_DELETE_ITEM,"删除区域");
	}
	else if(m_pSelItemInfo->itemType==CDataCmpModel::LOFT_DATA_MODEL||
		m_pSelItemInfo->itemType==CDataCmpModel::LOFT_BOM_GROUP)
		pMenu->AppendMenu(MF_STRING,ID_IMPORT_LOFT_BOM,"导入构件明细文件");
	else if(m_pSelItemInfo->itemType==CDataCmpModel::LOFT_BOM_DATA)
		pMenu->AppendMenuA(MF_STRING,ID_DELETE_ITEM,"删除BOM文件");
	pMenu->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON,point.x,point.y,this);
}
HTREEITEM CFileTreeDlg::FindItem(const char* strText)
{
	HTREEITEM hSelect=m_xFileCtrl.GetSelectedItem();
	if(hSelect==NULL || m_hImageGroup==NULL)
		return NULL;
	HTREEITEM hFind=m_xFileCtrl.GetChildItem(m_hImageGroup);
	while(hFind)
	{
		if(hFind!=hSelect && m_xFileCtrl.GetItemText(hFind).Compare(strText)==0)
			return hFind;
		hFind=m_xFileCtrl.GetNextSiblingItem(hFind);
	}
	return hFind;
}
//////////////////////////////////////////////////////////////////////////
//
void CFileTreeDlg::OnOK()
{

}
void CFileTreeDlg::OnCancel()
{

}
void CFileTreeDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
	if(m_xFileCtrl.GetSafeHwnd()==NULL)
		return;
	m_xFileCtrl.MoveWindow(0,0,cx,cy);
}
void CFileTreeDlg::OnNMClickTreeCtrl(NMHDR *pNMHDR, LRESULT *pResult)
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	//pNMTreeView->
	TVHITTESTINFO HitTestInfo;
	GetCursorPos(&HitTestInfo.pt);
	RECT rc;
	m_xFileCtrl.GetWindowRect(&rc);
	m_xFileCtrl.ScreenToClient(&HitTestInfo.pt);
	HTREEITEM hSelItem;
	if((hSelItem = m_xFileCtrl.HitTest(&HitTestInfo))!=NULL)
	{
		m_xFileCtrl.Select(HitTestInfo.hItem,TVGN_CARET);
		RefreshView();
		UpdateData(FALSE);
	}
	*pResult = 0;
}

void CFileTreeDlg::OnTvnSelchangedTreeCtrl(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	RefreshView();
	*pResult = 0;
}

void CFileTreeDlg::OnNMRClickTreeCtrl(NMHDR *pNMHDR, LRESULT *pResult)
{
	TVHITTESTINFO HitTestInfo;
	GetCursorPos(&HitTestInfo.pt);
	m_xFileCtrl.ScreenToClient(&HitTestInfo.pt);
	m_xFileCtrl.HitTest(&HitTestInfo);
	m_xFileCtrl.Select(HitTestInfo.hItem,TVGN_CARET);
	if(GetSelItemInfo())
	{
		m_xFileCtrl.ClientToScreen(&HitTestInfo.pt);
		ContextMenu(&m_xFileCtrl,HitTestInfo.pt);
	}
	*pResult = 0;
}

struct IMAGE_FILE_INFO{
	CXhChar500 sFilePath;
	CXhChar500 sPageName;
	int iPageNo;
	IMAGE_FILE_INFO(const char* file_path=NULL,int page_no=1,const char* page_name=NULL){
		iPageNo=page_no;
		if(file_path!=NULL)
			sFilePath.Copy(file_path);
		if(page_name)
			sPageName.Copy(page_name);
	}
};
static void AddFileInfoToHashTbl(CHashStrList<ATOM_LIST<IMAGE_FILE_INFO>> &hashFilePathsBySegStr,
								CHashList<SEGI> &segNoHashList,IMAGE_FILE_INFO &fileInfo,SEGI iSeg,
								const char* file_prefix=NULL,const char* file_suffix=NULL)
{
	CXhChar100 sName;
	CString sSeg,sFileName;
	if(iSeg>0)
		sSeg.Format("%s",(char*)iSeg.ToString());
	else
	{
		if(fileInfo.sPageName.GetLength()>0)
			sName.Copy(fileInfo.sPageName);
		else
		{
			CXhChar500 sPathName=fileInfo.sFilePath;
			_splitpath(sPathName,NULL,NULL,sName,NULL);
		}
		int i=0,i2=0;
		if(file_prefix!=NULL)
			sName.Replace(file_prefix,"");
		if(file_suffix!=NULL)
			sName.Replace(file_suffix,"");
		sSeg=sName,sFileName=sName;
		sFileName=sFileName.Trim();
		if((i2=sFileName.Find('-',i)) != -1)
			sSeg=sFileName.Mid(i,i2-i);
	}
	SEGI segI(sSeg);
	if(segI.Digital()<=0||segI.Digital()>200)	//无法从图片中提取段号时都归为65535段
		segI.iSeg=65535;
	segNoHashList.SetValue(segI,segI);
	ATOM_LIST<IMAGE_FILE_INFO> *pFilePathList=hashFilePathsBySegStr.GetValue(segI.ToString());
	if(pFilePathList==NULL)
		pFilePathList=hashFilePathsBySegStr.Add(segI.ToString());
	pFilePathList->append(fileInfo);
}

void CFileTreeDlg::AppendFile(ARRAY_LIST<CXhChar500> &filePathList,BOOL bDisplayDlg)
{	//0.建立分段与HTREEITEM映射关系
	CHashList<HTREEITEM> hashItemPtrBySegI;
	HTREEITEM hItem=NULL,hSelectedItem=m_xFileCtrl.GetSelectedItem();
	TREEITEM_INFO* pItemInfo=(TREEITEM_INFO*)m_xFileCtrl.GetItemData(hSelectedItem);
	SEGI iSeg;
	if(pItemInfo->itemType==CDataCmpModel::IMAGE_SEG_GROUP)
		iSeg=((SEGITEM*)m_pSelItemInfo->dwRefData)->iSeg;
	else
	{
		HTREEITEM hChildItem=m_xFileCtrl.GetChildItem(hSelectedItem);
		while(hChildItem!=NULL)
		{
			TREEITEM_INFO* pItemInfo=(TREEITEM_INFO*)m_xFileCtrl.GetItemData(hChildItem);
			if(pItemInfo->itemType==CDataCmpModel::IMAGE_SEG_GROUP)
			{
				SEGI iCurSeg=((SEGITEM*)pItemInfo->dwRefData)->iSeg;
				hashItemPtrBySegI.SetValue(iCurSeg,hChildItem);
			}
			hChildItem=m_xFileCtrl.GetNextSiblingItem(hChildItem);
		}
	}
	//1.将选择的文件按段分类
	CHashList<SEGI> segNoHashList;
	CHashStrList<ATOM_LIST<IMAGE_FILE_INFO>> hashFilePathsBySegStr;
	CXhChar500 *pFirstPathName=filePathList.GetFirst();
	CXhChar100 sName;
	char drive[4],dir[MAX_PATH],ext[MAX_PATH];
	_splitpath(*pFirstPathName,drive,dir,sName,ext);
	BOOL bInputPrefix=TRUE;
	CString sSeg,sFileName;
	CInputPerfixAndSuffixDlg dlg;
	if(stricmp(ext,".pdf")==0)
	{
		for(CXhChar500 *pFilePath=filePathList.GetFirst();pFilePath;pFilePath=filePathList.GetNext())
		{
			CXhChar500 sPathName=*pFilePath;
			CPdfReaderInstance pdfReader;
			IPdfReader *pReader=pdfReader.GetPdfReader();
			if(pReader->OpenPdfFile(sPathName)&&pReader->PageCount()>1)
			{
				IPdfReader::PDF_PAGE_INFO *pPageInfo=pReader->EnumFirstPage();
				dlg.m_sPrefix=pPageInfo->sPageName;
				if(bDisplayDlg)
				{
					if(dlg.DoModal()!=IDOK)
						return;
				}
				for(pPageInfo=pReader->EnumFirstPage();pPageInfo;pPageInfo=pReader->EnumNextPage())
				{
					CXhChar500 sPageName=pPageInfo->sPageName;
					sPageName.Replace(dlg.m_sPrefix,"");
					sPageName.Replace(dlg.m_sSuffix,"");
					AddFileInfoToHashTbl(hashFilePathsBySegStr,segNoHashList,IMAGE_FILE_INFO(sPathName,pPageInfo->iPageNo,sPageName),iSeg);
				}
			}
			else
			{
				if(bInputPrefix)
				{
					dlg.m_sPrefix=sName;
					if(bDisplayDlg)
					{
						if(dlg.DoModal()!=IDOK)
						return;
					}
					bInputPrefix=false;
				}
				AddFileInfoToHashTbl(hashFilePathsBySegStr,segNoHashList,IMAGE_FILE_INFO(sPathName),iSeg,dlg.m_sPrefix,dlg.m_sSuffix);
			}
		}
	}
	else
	{
		for(CXhChar500 *pFilePath=filePathList.GetFirst();pFilePath;pFilePath=filePathList.GetNext())
		{
			CXhChar500 sPathName=*pFilePath;
			AddFileInfoToHashTbl(hashFilePathsBySegStr,segNoHashList,IMAGE_FILE_INFO(sPathName),iSeg);
		}
	}
	//2.添加数据并更新树节点
	ATOM_LIST<IMAGE_FILE_INFO> *pFilePathList=NULL;
	ATOM_LIST<SEGI> segList;
	GetSortedSegNoList(segNoHashList,segList);
	for(SEGI *pSegI=segList.GetFirst();pSegI;pSegI=segList.GetNext())
	{
		pFilePathList=hashFilePathsBySegStr.GetValue(pSegI->ToString());
		if(pFilePathList==NULL)
			continue;
		HTREEITEM hSegItem=hSelectedItem;
		if(pItemInfo->itemType==CDataCmpModel::DWG_IMAGE_GROUP)
		{
			HTREEITEM* pItemPtr=hashItemPtrBySegI.GetValue(pSegI->iSeg);
			if(pItemPtr)
				hSegItem=*pItemPtr;
			else
			{
				SEGITEM *pSegItem=dataModel.NewSegI(*pSegI);
				if(pSegI->iSeg==65535)
					pSegItem->sSegName.Copy("***");
				hSegItem=InsertImageSegItem(pSegItem);
				hashItemPtrBySegI.SetValue(pSegI->iSeg,hSegItem);
			}
		}
		for(IMAGE_FILE_INFO *pPath=pFilePathList->GetFirst();pPath;pPath=pFilePathList->GetNext())
		{
			CXhChar500 sFileName(pPath->sFilePath);
			CImageFileHost* pImage=dataModel.GetImageFile(sFileName,pPath->iPageNo);
			if(pImage)
			{
				logerr.Log("%s图片文件已打开",(char*)sFileName);
				continue;
			}
			pImage=dataModel.AppendImageFile(sFileName,*pSegI,false,pPath->iPageNo,dlg.m_iRotateAngle*90);
			if(pImage)
			{
				_splitpath(sFileName,NULL,NULL,sName,NULL);
				if(pPath->sPageName.GetLength()>0)
					hItem=m_xFileCtrl.InsertItem(pPath->sPageName,PRJ_IMG_FILE,PRJ_IMG_FILE,hSegItem);
				else
					hItem=m_xFileCtrl.InsertItem(sName,PRJ_IMG_FILE,PRJ_IMG_FILE,hSegItem);
				TREEITEM_INFO *pItemInfo=itemInfoList.append(TREEITEM_INFO(CDataCmpModel::DWG_IMAGE_DATA,(DWORD)pImage));
				m_xFileCtrl.SetItemData(hItem,(DWORD)pItemInfo);
			}	
		}
		m_xFileCtrl.Expand(hSegItem,TVE_EXPAND);
	}
	m_xFileCtrl.Expand(hSelectedItem,TVE_EXPAND);
	UpdateData(FALSE);
	//if(hItem)
	//	m_xFileCtrl.SelectItem(hItem);
	if(GetSelItemInfo())
		RefreshView();
}
//导入图片文件（图片文件）
void CFileTreeDlg::OnImportImageFile()
{	//选择图片
	CLogErrorLife logErrLift;
	DWORD nFileNumbers = 500;    //CFileDialog最大选择文件数量
	//CString filter="BOM明细表|*.PDF;*.jpg;*.png;*.bmp;*.tif|PDF(*.pdf)|*.pdf|图片(*.jpg)|*.jpg|图片(*.bmp)|*.bmp|图片(*.png)|*.png|图片(*.tif)|*.tif|所有文件(*.*)|*.*||";
	CString filter = "BOM明细表|*.PDF;*.jpg;*.png;*.bmp|PDF(*.pdf)|*.pdf|图片(*.jpg)|*.jpg|图片(*.bmp)|*.bmp|图片(*.png)|*.png|所有文件(*.*)|*.*||";
	CFileDialog fileDlg(TRUE,"pdf","PDF.pdf",
		OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_ALLOWMULTISELECT,filter);
	fileDlg.m_ofn.lpstrTitle="选择图纸数据的图片文件";
	CEnlargeFileDialog enlarge(&fileDlg);
	if(fileDlg.DoModal()==IDOK)
	{	//0.建立分段与HTREEITEM映射关系
		BOOL bIncPreCent = FALSE;
		ARRAY_LIST<CXhChar500> filePathList;
		POSITION pos=fileDlg.GetStartPosition();
		while(pos)
		{	//不能使用CXhChar500(sPathName)，字符串带有特殊字符时会导致路径错误 wht 19-07-08
			CString sPathName=fileDlg.GetNextPathName(pos);		//获取文件名 
			if (sPathName.FindOneOf("%") >= 0)
				bIncPreCent = TRUE;
			CXhChar500 sFilePathName = sPathName;
			filePathList.append(sFilePathName);
		}
		AppendFile(filePathList,true);
		if (bIncPreCent)
			AfxMessageBox("文件名中包含特殊字符'%',请修改文件名称后重试！");
	}
}
//导入放样BOM文件
void CFileTreeDlg::OnImportLoftBom()
{
	//打开放样BOM文件，读取数据
	CFileDialog dlg(TRUE,"xls",NULL,NULL,"Excel|*.xls;*.xlsx|Excel(*.xls)|*.xls|Excel(*.xlsx)|*.xlsx|所有文件(*.*)|*.*||");
	dlg.m_ofn.lpstrTitle="选择放样BOM文件";
	if(dlg.DoModal()!=IDOK)
		return;
	CString sFileName =dlg.GetPathName();
	CBomFileData* pLoftBomFile=dataModel.GetLoftBomFile(sFileName);
	if(pLoftBomFile)
	{
		MessageBox("已打开指定文件！");
		return;
	}
	pLoftBomFile=dataModel.AppendLoftBomFile(sFileName);
	if(pLoftBomFile==NULL)
	{
		MessageBox("读取Excel文件失败！");
		return ;
	}
	//添加BOM组节点
	HTREEITEM hSelectedItem=m_xFileCtrl.GetSelectedItem();
	TREEITEM_INFO* pItemInfo=(TREEITEM_INFO*)m_xFileCtrl.GetItemData(hSelectedItem);
	if(pItemInfo->itemType==CDataCmpModel::LOFT_DATA_MODEL)
	{
		if(m_hLoftBomGroup==NULL)
			InsertLoftBomGroup(hSelectedItem);
	}
	//添加相应节点
	HTREEITEM hCurItem=m_xFileCtrl.InsertItem(dlg.GetFileName(),PRJ_IMG_FILE,PRJ_IMG_FILE,m_hLoftBomGroup);
	pItemInfo=itemInfoList.append(TREEITEM_INFO(CDataCmpModel::LOFT_BOM_DATA,(DWORD)pLoftBomFile));
	m_xFileCtrl.SetItemData(hCurItem,(DWORD)pItemInfo);
	m_xFileCtrl.Expand(m_hLoftBomGroup,TVE_EXPAND);
	m_xFileCtrl.SelectItem(hCurItem);
	RefreshView();
	UpdateData(FALSE);
}
static int CompareBomFilePath(const CXhChar500 &item1,const CXhChar500 &item2)
{
	CXhChar16 sLabel1,sLable2;
	CXhChar100 sName1,sName2;
	CXhChar500 sPrefix1,sPrefix2;
	_splitpath(item1,NULL,NULL,sName1,NULL);
	_splitpath(item2,NULL,NULL,sName2,NULL);
	sPrefix1.Copy(strtok(sName1,"#"));
	if(sPrefix1.GetLength()>0)
		sLabel1.Copy(strtok(NULL,"~"));
	else
		sLabel1.Copy(strtok(sName1,"~"));
	sPrefix2.Copy(strtok(sName2,"#"));
	if(sPrefix2.GetLength()>0)
		sLable2.Copy(strtok(NULL,"~"));
	else
		sLable2.Copy(strtok(sName2,"~"));
	if(sPrefix1.GetLength()==0)
		sPrefix1.Copy("NULL");
	if(sPrefix2.GetLength()==0)
		sPrefix2.Copy("NULL");
	//先比较前缀字符串，前缀一致表示来自同一张图纸，属于同一段
	SEGI segI1(sPrefix1),segI2(sPrefix2);
	int nRetCode=CompareSegI(segI1,segI2);
	if(nRetCode==0)
		nRetCode=ComparePartNoString(sLabel1,sLable2);
	return nRetCode;
}
//导入图纸BOM
void CFileTreeDlg::OnImportDwgBom()
{
	CLogErrorLife logErrLife;
	DWORD nFileNumbers = 500;    //CFileDialog最大选择文件数量
	//打开文件，读取数据
	CString filter;
	filter="Dwg Bom文件(*.bomd)|*.bomd|Excel(*.xls)|*.xls|Excel(*.xlsx)|*.xlsx|所有文件(*.*)|*.*||";
	CFileDialog fileDlg(TRUE,"xls",NULL,OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_ALLOWMULTISELECT,filter);
	fileDlg.m_ofn.lpstrTitle="选择图纸BOM文件";
	CEnlargeFileDialog enlarge(&fileDlg);
	if(fileDlg.DoModal()!=IDOK)
		return;
	//添加BOM组节点
	HTREEITEM hSelectedItem=m_xFileCtrl.GetSelectedItem(),hItem=NULL;
	TREEITEM_INFO* pItemInfo=(TREEITEM_INFO*)m_xFileCtrl.GetItemData(hSelectedItem);
	if(pItemInfo->itemType==CDataCmpModel::DWG_DATA_MODEL)
	{
		if(m_hDwgBomGroup==NULL)
			InsertDwgBomGroup(hSelectedItem);
	}
	//添加文件
	ARRAY_LIST<CXhChar500> filePathList;
	POSITION pos=fileDlg.GetStartPosition();
	while(pos)
	{
		CString sFileName=fileDlg.GetNextPathName(pos);		//获取文件名 
		filePathList.append(CXhChar500(sFileName));
	}
	CHeapSort<CXhChar500>::HeapSort(filePathList.m_pData,filePathList.Size(),CompareBomFilePath);
	for(CXhChar500 *pFileName=filePathList.GetFirst();pFileName;pFileName=filePathList.GetNext())
	{
		CXhChar500 sFileName(*pFileName);
		CBomFileData* pDwgBomFile=dataModel.GetDwgBomFile(sFileName);
		if(pDwgBomFile)
		{
			logerr.Log("%s文件已打开",(char*)sFileName);
			continue;
		}
		pDwgBomFile=dataModel.AppendDwgBomFile(sFileName);
		if(pDwgBomFile)
		{
			CXhChar100 sName="";
			_splitpath(sFileName,NULL,NULL,sName,NULL);
			hItem=m_xFileCtrl.InsertItem(sName,PRJ_IMG_FILE,PRJ_IMG_FILE,m_hDwgBomGroup);
			pItemInfo=itemInfoList.append(TREEITEM_INFO(CDataCmpModel::DWG_BOM_DATA,(DWORD)pDwgBomFile));
			m_xFileCtrl.SetItemData(hItem,(DWORD)pItemInfo);
		}
	}
	//更新树节点
	m_xFileCtrl.Expand(m_hDwgBomGroup,TVE_EXPAND);
	if(hItem)
		m_xFileCtrl.SelectItem(hItem);
	RefreshView();
	UpdateData(FALSE);
}
//添加段文件
void CFileTreeDlg::OnAddSegData()
{
	HTREEITEM hSelectedItem=m_xFileCtrl.GetSelectedItem();
	if(hSelectedItem==NULL ||hSelectedItem!=m_hImageGroup)
		return;
	CInputDlg dlg;
	if(dlg.DoModal()!=IDOK)
		return;
	ATOM_LIST<SEGI> segNoList;
	GetSortedSegNoList(dlg.m_sInputValue,segNoList);
	for(SEGI *pSegI=segNoList.GetFirst();pSegI;pSegI=segNoList.GetNext())
	{
		SEGITEM *pSegItem=dataModel.NewSegI(pSegI->iSeg);
		InsertImageSegItem(pSegItem);
	}
	m_xFileCtrl.Expand(m_hImageGroup,TVE_EXPAND);
}
//修改段名
void CFileTreeDlg::OnRenameItem()
{
	HTREEITEM hSelectedItem=m_xFileCtrl.GetSelectedItem();
	if(hSelectedItem==NULL)
		return;
	TREEITEM_INFO* pItemInfo=(TREEITEM_INFO*)m_xFileCtrl.GetItemData(hSelectedItem);
	if(pItemInfo==NULL)
		return;
	//m_xFileCtrl.EditLabel(hSelectedItem);
	if( pItemInfo->itemType==CDataCmpModel::IMAGE_SEG_GROUP||
		pItemInfo->itemType==CDataCmpModel::DWG_IMAGE_DATA||
		pItemInfo->itemType==CDataCmpModel::IMAGE_REGION_DATA)
	{
		if(pItemInfo->dwRefData==NULL)
			return;
		CInputAnStringValDlg dlg;
		if(pItemInfo->itemType==CDataCmpModel::IMAGE_SEG_GROUP)
			dlg.m_sItemTitle="分段名称：";
		else if(pItemInfo->itemType==CDataCmpModel::DWG_IMAGE_DATA)
			dlg.m_sItemTitle="文件名称：";
		else if(pItemInfo->itemType==CDataCmpModel::IMAGE_REGION_DATA)
			dlg.m_sItemTitle="区域名称：";
		dlg.m_sItemValue=m_xFileCtrl.GetItemText(hSelectedItem);
		if(dlg.DoModal()!=IDOK)
			return;
		m_xFileCtrl.SetItemText(hSelectedItem,dlg.m_sItemValue);
		 if(pItemInfo->itemType==CDataCmpModel::IMAGE_SEG_GROUP)
		 {
			 SEGITEM *pSegItem=(SEGITEM*)pItemInfo->dwRefData;
			 pSegItem->sSegName=dlg.m_sItemValue;
		 }
		 else if(pItemInfo->itemType==CDataCmpModel::DWG_IMAGE_DATA)
		 {
			 CImageFileHost* pImage=(CImageFileHost*)pItemInfo->dwRefData;
			 pImage->szFileName=dlg.m_sItemValue;
		 }
		 else if(pItemInfo->itemType==CDataCmpModel::IMAGE_REGION_DATA)
		 {
			 CImageRegionHost *pRegion=(CImageRegionHost*)pItemInfo->dwRefData;
			 pRegion->m_sName=dlg.m_sItemValue;
		 }
	}
}
BOOL CFileTreeDlg::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_KEYDOWN)
	{
		if(pMsg->wParam ==VK_F2 && !bEditState)
			OnRenameItem();
		else if(pMsg->wParam==VK_RETURN && bEditState)
		{
			bEditState=FALSE;
			this->SetFocus(); //编辑框失去焦点，结束编辑
			return TRUE;
		}
		else if(pMsg->wParam==VK_ESCAPE && bEditState)
		{
			bEditState=FALSE;
			this->SetFocus(); //编辑框失去焦点，结束编辑
			return TRUE;
		}
		else if(pMsg->wParam==VK_DELETE)
		{
			OnDeleteItem();
			return TRUE;
		}
	}
	return CDialog::PreTranslateMessage(pMsg);
}
void CFileTreeDlg::OnBeginLabelEditTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);
	TREEITEM_INFO* pItemInfo=(TREEITEM_INFO*)m_xFileCtrl.GetItemData(m_xFileCtrl.GetSelectedItem());
	if(pItemInfo->itemType!=CDataCmpModel::IMAGE_SEG_GROUP)
	{	//节点不能编辑，让*pResult返回非0即可
		*pResult=1;
		return;
	}
	bEditState=TRUE;
	*pResult=0;
}
void CFileTreeDlg::OnEndLabelEditTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);
	CString strNewName = pTVDispInfo->item.pszText;
	if(bEditState && strNewName.GetLength()>0)
	{	//可以修改节点名称
		HTREEITEM hFind=FindItem(strNewName);
		if(hFind==NULL)
		{
			HTREEITEM hSelectedItem=m_xFileCtrl.GetSelectedItem();
			TREEITEM_INFO* pItemInfo=(TREEITEM_INFO*)m_xFileCtrl.GetItemData(hSelectedItem);
			SEGITEM* pSegItem=(SEGITEM*)pItemInfo->dwRefData;
			strcpy(pSegItem->sSegName,strNewName);
			m_xFileCtrl.SetItemText(hSelectedItem,strNewName);
			bEditState=FALSE;
		}
		else
		{
			MessageBox("该段名已存在");
			return;
		}
	}
	*pResult=0;
}
//进行数据对比
void CFileTreeDlg::OnCompareData()
{
	if(m_pSelItemInfo==NULL || m_pSelItemInfo->itemType!=CDataCmpModel::DATA_CMP_MODEL)
		return;
	CIBOMView *pView=(CIBOMView*)theApp.GetIBomView();
	if(dataModel.ComparePartData(pView->GetCompareLenTolerance(),pView->GetCompareWeightTolerance()))
		RefreshView();
	else
		AfxMessageBox("数据完全一致！");
}
//到处对比结果
void CFileTreeDlg::OnExportCompResult()
{
	if(m_pSelItemInfo==NULL || m_pSelItemInfo->itemType!=CDataCmpModel::DATA_CMP_MODEL)
		return;
	dataModel.ExportCompareResult();
}
//进行数据提取
void CFileTreeDlg::OnRetrieveData()
{
	RetrieveData();
}
static int WriteMonoBmpFile(const char* fileName, unsigned int width,unsigned effic_width, 
							   unsigned int height, unsigned char* image)
{
	BITMAPINFOHEADER bmpInfoHeader;
	BITMAPFILEHEADER bmpFileHeader;
	FILE *filep;
	unsigned int row, column;
	unsigned int extrabytes, bytesize;
	unsigned char *paddedImage = NULL, *paddedImagePtr, *imagePtr;

	/* The .bmp format requires that the image data is aligned on a 4 byte boundary.  For 24 - bit bitmaps,
	   this means that the width of the bitmap  * 3 must be a multiple of 4. This code determines
	   the extra padding needed to meet this requirement. */
   extrabytes = width%8>0?1:0;//(4 - (width * 3) % 4) % 4;
   long bmMonoImageRowBytes=width/8+extrabytes;
   long bmWidthBytes=((bmMonoImageRowBytes+3)/4)*4;

	// This is the size of the padded bitmap
	bytesize = bmWidthBytes*height;//(width * 3 + extrabytes) * height;

	// Fill the bitmap file header structure
	bmpFileHeader.bfType = 'MB';   // Bitmap header
	bmpFileHeader.bfSize = 0;      // This can be 0 for BI_RGB bitmaps
	bmpFileHeader.bfReserved1 = 0;
	bmpFileHeader.bfReserved2 = 0;
	bmpFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	// Fill the bitmap info structure
	bmpInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpInfoHeader.biWidth = width;
	bmpInfoHeader.biHeight = height;
	bmpInfoHeader.biPlanes = 1;
	bmpInfoHeader.biBitCount = 1;            // 1 - bit mono bitmap
	bmpInfoHeader.biCompression = BI_RGB;
	bmpInfoHeader.biSizeImage = bytesize;     // includes padding for 4 byte alignment
	bmpInfoHeader.biXPelsPerMeter = 2952;
	bmpInfoHeader.biYPelsPerMeter = 2952;
	bmpInfoHeader.biClrUsed = 0;
	bmpInfoHeader.biClrImportant = 0;

	// Open file
	if ((filep = fopen(fileName, "wb")) == NULL) {
		AfxMessageBox("Error opening file");
		return FALSE;
	}

	// Write bmp file header
	if (fwrite(&bmpFileHeader, 1, sizeof(BITMAPFILEHEADER), filep) < sizeof(BITMAPFILEHEADER)) {
		AfxMessageBox("Error writing bitmap file header");
		fclose(filep);
		return FALSE;
	}

	// Write bmp info header
	if (fwrite(&bmpInfoHeader, 1, sizeof(BITMAPINFOHEADER), filep) < sizeof(BITMAPINFOHEADER)) {
		AfxMessageBox("Error writing bitmap info header");
		fclose(filep);
		return FALSE;
	}
	RGBQUAD palette[2];
	palette[0].rgbBlue=palette[0].rgbGreen=palette[0].rgbRed=palette[0].rgbReserved=palette[1].rgbReserved=0;	//黑色
	palette[1].rgbBlue=palette[1].rgbGreen=palette[1].rgbRed=255;	//白色
	fwrite(palette,8,1,filep);
	// Allocate memory for some temporary storage
	paddedImage = (unsigned char *)calloc(sizeof(unsigned char), bytesize);
	if (paddedImage == NULL) {
		AfxMessageBox("Error allocating memory");
		fclose(filep);
		return FALSE;
	}

	/*	This code does three things.  First, it flips the image data upside down, as the .bmp
		format requires an upside down image.  Second, it pads the image data with extrabytes 
		number of bytes so that the width in bytes of the image data that is written to the
		file is a multiple of 4.  Finally, it swaps (r, g, b) for (b, g, r).  This is another
		quirk of the .bmp file format. */
	for (row = 0; row < height; row++) {
		//imagePtr = image + (height - 1 - row) * width * 3;
		imagePtr = image + (height-row-1) * effic_width;
		paddedImagePtr = paddedImage + row * bmWidthBytes;
		for (column = 0; column < effic_width; column++) {
			*paddedImagePtr = *imagePtr;
			imagePtr += 1;
			paddedImagePtr += 1;
			if((unsigned int)(paddedImagePtr-paddedImage)>=bytesize)
				break;
		}
	}

	// Write bmp data
	if (fwrite(paddedImage, 1, bytesize, filep) < bytesize) {
		AfxMessageBox("Error writing bitmap data");
		free(paddedImage);
		fclose(filep);
		return FALSE;
	}

	// Close file
	fclose(filep);
	free(paddedImage);
	return TRUE;
}
void CFileTreeDlg::OnExportImageFile()
{
	if(!GetSelItemInfo())
		return;
	if(m_pSelItemInfo->itemType==CDataCmpModel::DWG_IMAGE_DATA&&theApp.IsHasInternalTest())
	{
		CString filter="图片(*.bmp)|*.bmp|所有文件(*.*)|*.*||";
		CFileDialog savedlg(FALSE,"bmp","imgfile.bmp",OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,filter);
		//savedlg.m_ofn.lpstrTitle="导出为位图文件";
		if(savedlg.DoModal()!=IDOK)
			return;
		CImageFileHost *pImgFile=(CImageFileHost*)m_pSelItemInfo->dwRefData;
		if(pImgFile==NULL)
			return;
		IMAGE_DATA image;
		if(!pImgFile->GetMonoImageData(&image))
			return;
		CHAR_ARRAY pool(image.nEffWidth*image.nHeight);
		image.imagedata=pool;
		pImgFile->GetMonoImageData(&image);
		WriteMonoBmpFile(savedlg.GetPathName(),image.nWidth,image.nEffWidth,image.nHeight,(BYTE*)image.imagedata);
	}
	else if(m_pSelItemInfo->itemType==CDataCmpModel::IMAGE_REGION_DATA)
	{
		CString filter="图片(*.bmp)|*.bmp|所有文件(*.*)|*.*||";
		CFileDialog savedlg(FALSE,"bmp","region.bmp",OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,filter);
		//savedlg.m_ofn.lpstrTitle="导出为位图文件";
		if(savedlg.DoModal()!=IDOK)
			return;
		CImageRegionHost *pRegion=(CImageRegionHost*)m_pSelItemInfo->dwRefData;
		if(pRegion==NULL||pRegion->BelongImageFile()==NULL)
			return;
		IMAGE_DATA image;
		if(!pRegion->GetMonoImageData(&image))
			return;
		CHAR_ARRAY pool(image.nEffWidth*image.nHeight);
		image.imagedata=pool;
		pRegion->GetMonoImageData(&image);
		WriteMonoBmpFile(savedlg.GetPathName(),image.nWidth,image.nEffWidth,image.nHeight,(BYTE*)image.imagedata);
	}
}
void CFileTreeDlg::RetrieveData()
{
	if(!GetSelItemInfo())
		return;
	CDisplayDataPage* pDataPage=theApp.GetIBomView()->GetDataPage();
	if(pDataPage==NULL)
		return;
	BOOL bValidRecog=FALSE;
	if(m_pSelItemInfo->itemType==CDataCmpModel::DWG_IMAGE_DATA)
	{
		CImageFileHost* pImage=(CImageFileHost*)m_pSelItemInfo->dwRefData;
		if(pImage==NULL)
			return;
		if(pDataPage->cPolyLineState!=POLYLINE_4PT::STATE_VALID)
		{
			AfxMessageBox("没有选中数据区域,请重新选择!");
			pDataPage->InitPolyLine();
			return;
		}
		CPoint *ptArr=pDataPage->GetPolyLinePtArr();
		CImageRegionHost* pRegion=pImage->AppendRegion();
		if(pRegion)
		{
			CWaitCursor waitCursor;
			pRegion->SetRegion(ptArr[0],ptArr[3],ptArr[2],ptArr[1]);
			pRegion->SummarizePartInfo();
			pRegion->m_sName.Printf("区域%d",pRegion->GetKey());
			//更新数列表
			HTREEITEM hRegionItem=m_xFileCtrl.InsertItem(pRegion->m_sName,PRJ_IMG_PARA,PRJ_IMG_PARA,m_hSelect);
			TREEITEM_INFO* pItemInfo=itemInfoList.append(TREEITEM_INFO(CDataCmpModel::IMAGE_REGION_DATA,(DWORD)pRegion));
			m_xFileCtrl.SetItemData(hRegionItem,(DWORD)pItemInfo);
			m_xFileCtrl.Expand(m_hSelect,TVE_EXPAND);
			m_xFileCtrl.SelectItem(hRegionItem);
			pDataPage->InitPolyLine();
			RefreshView();
			bValidRecog=TRUE;
		}
	}
	else if(m_pSelItemInfo->itemType==CDataCmpModel::IMAGE_REGION_DATA)
	{	//
		CImageRegionHost *pRegion=(CImageRegionHost*)m_pSelItemInfo->dwRefData;
		if(pRegion==NULL||pRegion->BelongImageFile()==NULL)
			return;
		CWaitCursor cursor;
		pRegion->SummarizePartInfo();
		//更新数列表
		RefreshView();
		bValidRecog=TRUE;
	}
	else if(m_pSelItemInfo->itemType==CDataCmpModel::LOFT_BOM_DATA)
	{	//
		CBomFileData *pLoftData=(CBomFileData*)m_pSelItemInfo->dwRefData;
		//if(pLoftData==NULL||pLoftData->IsFromExcelFile())
			//return;
		//pLoftData->RunSummarize(&Ta);
		RefreshView();
	}
	CIBOMDoc *pDoc=theApp.GetDocument();
	if(bValidRecog&&pDoc&&pDoc->m_nTimer<=0)
	{
		pDoc->ResetAutoSaveTimer();
	}
}
void CFileTreeDlg::OnDeleteItem()
{
	DWORD i=0;
	DYN_ARRAY<HTREEITEM> itemArr(m_xFileCtrl.GetSelectedCount());
	for(HTREEITEM hItem=m_xFileCtrl.GetFirstSelectedItem();hItem;hItem=m_xFileCtrl.GetNextSelectedItem(hItem),i++)
		itemArr[i]=hItem;
	for(i=0;i<itemArr.Size();i++)
		DeleteItem(itemArr[i]);
}
BOOL CFileTreeDlg::DeleteItem(HTREEITEM hItem)
{
	m_pSelItemInfo=NULL;
	m_hSelect=hItem;
	if(m_hSelect==NULL)
		return FALSE;
	m_pSelItemInfo=(TREEITEM_INFO*)m_xFileCtrl.GetItemData(m_hSelect);
	if(m_pSelItemInfo==NULL)
		return FALSE;
	if(m_pSelItemInfo->itemType==CDataCmpModel::IMAGE_REGION_DATA)
	{
		CImageRegionHost *pRegion=(CImageRegionHost*)m_pSelItemInfo->dwRefData;
		if(pRegion)
			pRegion->BelongImageFile()->DeleteRegion(pRegion->GetKey());
		m_xFileCtrl.DeleteItem(m_hSelect);
	}
	else if(m_pSelItemInfo->itemType==CDataCmpModel::LOFT_BOM_DATA)	    //放样BOM组
	{
		CBomFileData* pBomFileData = (CBomFileData*)m_pSelItemInfo->dwRefData;
		if(pBomFileData)
			dataModel.DeleteLoftBomFile(pBomFileData->m_sName);
		m_xFileCtrl.DeleteItem(m_hSelect);
	}
	else if(m_pSelItemInfo->itemType==CDataCmpModel::DWG_BOM_DATA)		//图纸BOM
	{
		CBomFileData *pBomFileData=(CBomFileData*)m_pSelItemInfo->dwRefData;
		if(pBomFileData)
			dataModel.DeleteDwgBomFile(pBomFileData->m_sName);
		m_xFileCtrl.DeleteItem(m_hSelect);
	}
	else if(m_pSelItemInfo->itemType==CDataCmpModel::DWG_IMAGE_DATA)	//图纸图片
	{
		CImageFileHost *pFileHost=(CImageFileHost*)m_pSelItemInfo->dwRefData;
		if(pFileHost)
			dataModel.DeleteImageFile(pFileHost->szPathFileName,pFileHost->m_iSeg);
		m_xFileCtrl.DeleteItem(m_hSelect);
	}
	else if(m_pSelItemInfo->itemType==CDataCmpModel::IMAGE_SEG_GROUP)			//删除段
	{
		SEGITEM *pSegItem=(SEGITEM*)m_pSelItemInfo->dwRefData;
		if(pSegItem)
			dataModel.DeleteSegItem(pSegItem->iSeg);
		m_xFileCtrl.DeleteItem(m_hSelect);
	}
	RefreshView();
	return TRUE;
}
void CFileTreeDlg::OnExportDwgExcel()
{
	if(!GetSelItemInfo())
		return;
	if(m_pSelItemInfo->itemType==CDataCmpModel::DWG_IMAGE_GROUP)
		dataModel.ExportDwgDataExcel(0);
	else if(m_pSelItemInfo->itemType==CDataCmpModel::IMAGE_SEG_GROUP)
	{
		SEGITEM *pSegItem=(SEGITEM*)m_pSelItemInfo->dwRefData;
		dataModel.ExportDwgDataExcel(0,pSegItem?&pSegItem->iSeg:NULL);
	}
	else if(m_pSelItemInfo->itemType==CDataCmpModel::DWG_BOM_GROUP)
		dataModel.ExportDwgDataExcel(1);
	else if(m_pSelItemInfo->itemType==CDataCmpModel::DWG_DATA_MODEL)
		dataModel.ExportDwgDataExcel(2);
}

void CFileTreeDlg::OnImportImageFolder()
{
	char ss[MAX_PATH]="";
	GetAppPath(ss);
	CString sWorkPath=ss;
	if(!InvokeFolderPickerDlg(sWorkPath))
		return;
	//1.遍历文件夹中子文件夹获取分段图像路径
	CHashList<SEGI> segNoHashList;
	CHashStrList<ATOM_LIST<CXhChar500>> hashFilePathsBySegStr;
	CFileFind finderFolder,finderFile; 
	BOOL bFindFolder=finderFolder.FindFile(sWorkPath+_T("\\*.*"));//总文件夹，开始遍历 
	while(bFindFolder) 
	{
		bFindFolder = finderFolder.FindNextFile();//查找文件 
		if (finderFolder.IsDots()) 
			continue;
		CString sFileName=finderFolder.GetFileName();
		if(finderFolder.IsDirectory())
		{
			segNoHashList.SetValue(SEGI(sFileName),SEGI(sFileName));
			ATOM_LIST<CXhChar500> *pFilePathList=hashFilePathsBySegStr.Add(sFileName);
			CString sDir=finderFolder.GetFilePath();
			BOOL bFindFile=finderFile.FindFile(sDir+_T("\\*.jpg"));
			while(bFindFile)
			{ 
				bFindFile = finderFile.FindNextFile();//查找文件 
				if(finderFile.IsDirectory()||finderFile.IsDots())
					continue;
				CString sFilePath=finderFile.GetFilePath();
				pFilePathList->append(CXhChar500(sFilePath));
			}
		}
		else 
		{
			int i=0,i2=0;
			CString sSeg=sFileName;
			if((i2=sFileName.Find('-',i)) != -1)
				sSeg=sFileName.Mid(i,i2-i);
			segNoHashList.SetValue(SEGI(sSeg),SEGI(sSeg));
			ATOM_LIST<CXhChar500> *pFilePathList=hashFilePathsBySegStr.GetValue(sSeg);
			if(pFilePathList==NULL)
				pFilePathList=hashFilePathsBySegStr.Add(sSeg);
			CString sFilePath=finderFolder.GetFilePath();
			pFilePathList->append(CXhChar500(sFilePath));
		}
	}
	//2、读取图形
	dataModel.Empty();
	CXhChar100 sName;
	ATOM_LIST<CXhChar500> *pFilePathList=NULL;
	ATOM_LIST<SEGI> segList;
	GetSortedSegNoList(segNoHashList,segList);
	for(SEGI *pSegI=segList.GetFirst();pSegI;pSegI=segList.GetNext())
	{
		pFilePathList=hashFilePathsBySegStr.GetValue(pSegI->ToString());
		SEGITEM *pSegItem=dataModel.NewSegI(*pSegI);
		HTREEITEM hSegItem=InsertImageSegItem(pSegItem);
		for(CXhChar500 *pPath=pFilePathList->GetFirst();pPath;pPath=pFilePathList->GetNext())
		{
			CXhChar500 sFileName(*pPath);
			CImageFileHost* pImage=dataModel.GetImageFile(sFileName);
			if(pImage)
				continue;
			pImage=dataModel.AppendImageFile(sFileName,*pSegI,false);
			if(pImage)
			{
				_splitpath(sFileName,NULL,NULL,sName,NULL);
				HTREEITEM hItem=m_xFileCtrl.InsertItem(sName,PRJ_IMG_FILE,PRJ_IMG_FILE,hSegItem);
				TREEITEM_INFO *pItemInfo=itemInfoList.append(TREEITEM_INFO(CDataCmpModel::DWG_IMAGE_DATA,(DWORD)pImage));
				m_xFileCtrl.SetItemData(hItem,(DWORD)pItemInfo);
			}	
		}
		m_xFileCtrl.Expand(hSegItem,TVE_EXPAND);
	}
	m_xFileCtrl.Expand(m_hImageGroup,TVE_EXPAND);
}

void CFileTreeDlg::MoveItem(int head0_up1_down2_tail3)
{
	HTREEITEM hSelectedItem=m_xFileCtrl.GetSelectedItem();
	if(hSelectedItem==NULL)
		return;
	TREEITEM_INFO* pItemInfo=(TREEITEM_INFO*)m_xFileCtrl.GetItemData(hSelectedItem);
	if(pItemInfo==NULL||pItemInfo->dwRefData==NULL)
		return;
	if(pItemInfo->itemType==CDataCmpModel::DWG_IMAGE_DATA)
	{
		CImageFileHost* pImage=(CImageFileHost*)pItemInfo->dwRefData;
		
	}
	else if(pItemInfo->itemType==CDataCmpModel::IMAGE_REGION_DATA)
	{
		CImageRegionHost *pRegion=(CImageRegionHost*)pItemInfo->dwRefData;
		
	}
}

void CFileTreeDlg::OnMoveToTop()
{

}
void CFileTreeDlg::OnMoveUp()
{

}
void CFileTreeDlg::OnMoveDown()
{

}
void CFileTreeDlg::OnMoveToBottom()
{

}

void CFileTreeDlg::OnEditItemProp()
{
	CSettingDlg dlg;
	HTREEITEM hSelectedItem=m_xFileCtrl.GetSelectedItem();
	if(hSelectedItem==NULL)
		return;
	TREEITEM_INFO* pItemInfo=(TREEITEM_INFO*)m_xFileCtrl.GetItemData(hSelectedItem);
	if(pItemInfo==NULL||pItemInfo->dwRefData==NULL)
		return;
	if(pItemInfo->itemType==CDataCmpModel::DWG_IMAGE_DATA)
	{
		CImageFileHost* pImage=(CImageFileHost*)pItemInfo->dwRefData;
		CSettingDlg dlg;
		dlg.m_pImageFile=pImage;
		dlg.DoModal();
	}
}