// SettingDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "IBOM.h"
#include "SettingDlg.h"
#include "afxdialogex.h"
#include "PropertyListOper.h"
#include "LicFuncDef.h"
#include "XhLicAgent.h"
#include "XhLdsLm.h"

static int UpdatePdfLineModePropItem(CPropertyListOper<CSettingDlg> *pOper,CPropTreeItem *pParentItem,BOOL bUpdate,BYTE ciPdfLineMode)
{
	if(pOper==NULL)
		return 0;
	CPropertyList *pPropList=pOper->GetPropList();
	if(pPropList==NULL||pParentItem==NULL)
		return 0;
	pPropList->DeleteAllSonItems(pParentItem);
	if(ciPdfLineMode==1)
	{	//细字体
		pOper->InsertEditPropItem(pParentItem,"m_fMinPdfLineWeight",NULL,NULL,-1,bUpdate);
		//pOper->InsertEditPropItem(pParentItem,"m_fMinPdfLineFlatness");
		//pOper->InsertEditPropItem(pParentItem,"m_fMaxPdfLineWeight");
		//pOper->InsertEditPropItem(pParentItem,"m_fMaxPdfLineFlatness");
	}
	else if(ciPdfLineMode==2)
	{	//细字体
		//pOper->InsertEditPropItem(pParentItem,"m_fMinPdfLineWeight");
		//pOper->InsertEditPropItem(pParentItem,"m_fMinPdfLineFlatness");
		pOper->InsertEditPropItem(pParentItem,"m_fMaxPdfLineWeight",NULL,NULL,-1,bUpdate);
		//pOper->InsertEditPropItem(pParentItem,"m_fMaxPdfLineFlatness");
	}
	else if(ciPdfLineMode==3)
	{
		pOper->InsertEditPropItem(pParentItem,"m_fMinPdfLineWeight",NULL,NULL,-1,bUpdate);
		//pOper->InsertEditPropItem(pParentItem,"m_fMinPdfLineFlatness");
		pOper->InsertEditPropItem(pParentItem,"m_fMaxPdfLineWeight",NULL,NULL,-1,bUpdate);
		//pOper->InsertEditPropItem(pParentItem,"m_fMaxPdfLineFlatness");
	}
	return 0;
}

static BOOL ModifyBomFilePropValue(CPropertyList* pPropList,CPropTreeItem* pItem,CString& valueStr)
{	
	CSettingDlg *pDlg=(CSettingDlg*)pPropList->GetParent();
	if(pDlg==NULL||pDlg->m_pModel==NULL)
		return FALSE;
	COLORREF curClr = 0;
	char tem_str[101]="";
	_snprintf(tem_str,100,"%s",valueStr);
	memmove(tem_str, tem_str+3, 97);	
	sscanf(tem_str,"%X",&curClr);
	//
	BOOL bSaveToReg=FALSE;
	CDataCmpModel *pModel=pDlg->m_pModel;
	if(CSettingDlg::GetPropID("m_sPrjCode")==pItem->m_idProp)
		pModel->m_sPrjCode.Copy(valueStr);
	else if(CSettingDlg::GetPropID("m_sPrjName")==pItem->m_idProp)
		pModel->m_sPrjName.Copy(valueStr);
	else if(CSettingDlg::GetPropID("m_sTowerTypeName")==pItem->m_idProp)
		pModel->m_sTowerTypeName.Copy(valueStr);
	else if(CSettingDlg::GetPropID("m_sTowerAlias")==pItem->m_idProp)
		pModel->m_sTowerAlias.Copy(valueStr);
	else if(CSettingDlg::GetPropID("m_sTaStampNo")==pItem->m_idProp)
		pModel->m_sTaStampNo.Copy(valueStr);
	else if(CSettingDlg::GetPropID("m_sOperator")==pItem->m_idProp)
	{
		pModel->m_sOperator.Copy(valueStr);
		CConfig::m_sOperator.Copy(valueStr);
		bSaveToReg=TRUE;
	}
	else if(CSettingDlg::GetPropID("m_sAuditor")==pItem->m_idProp)
	{
		pModel->m_sAuditor.Copy(valueStr);
		CConfig::m_sAuditor.Copy(valueStr);
		bSaveToReg=TRUE;
	}
	else if(CSettingDlg::GetPropID("m_sCritic")==pItem->m_idProp)
	{
		pModel->m_sCritic.Copy(valueStr);
		pModel->m_sCritic.Copy(valueStr);
		bSaveToReg=TRUE;
	}
	if(bSaveToReg)
		CConfig::WriteSysParaToReg();
	return true;
}

static BOOL ModifyFilePropValue(CPropertyList* pPropList,CPropTreeItem* pItem,CString& valueStr)
{	
	CSettingDlg *pDlg=(CSettingDlg*)pPropList->GetParent();
	if(pDlg==NULL||pDlg->m_pImageFile==NULL)
		return FALSE;
	COLORREF curClr = 0;
	char tem_str[101]="";
	_snprintf(tem_str,100,"%s",valueStr);
	memmove(tem_str, tem_str+3, 97);	
	sscanf(tem_str,"%X",&curClr);
	//
	BOOL bReloadFile=FALSE;
	CImageFileHost *pFile=pDlg->m_pImageFile;
	if(CSettingDlg::GetPropID("m_fInitPDFZoomScale")==pItem->m_idProp)
	{	
		double fOldValue=pFile->fPDFZoomScale;
		pFile->fPDFZoomScale=atof(valueStr);
		bReloadFile=(pFile->fPDFZoomScale!=fOldValue);
	}
	else if(CSettingDlg::GetPropID("PdfLineMode")==pItem->m_idProp)
	{
		int iOldType=pFile->ciPDFLineMode;
		pFile->ciPDFLineMode=valueStr[0]-'0';
		if(pFile->ciPDFLineMode==0)
		{	//正常
			pFile->fPDFMinLineWeight=pFile->fPDFMinLineFlatness=0;
			pFile->fPDFMaxLineWeight=pFile->fPDFMaxLineFlatness=0;
		}
		else if(pFile->ciPDFLineMode==1)
		{	//细
			pFile->fPDFMinLineWeight=pFile->fPDFMinLineFlatness=0.5;
			pFile->fPDFMaxLineWeight=pFile->fPDFMaxLineFlatness=0;
		}
		else if(pFile->ciPDFLineMode==2)
		{	//粗
			pFile->fPDFMinLineWeight=pFile->fPDFMinLineFlatness=0;
			pFile->fPDFMaxLineWeight=pFile->fPDFMaxLineFlatness=0.6;
		}
		else if(pFile->ciPDFLineMode==3)
		{	//自定义
			pFile->fPDFMinLineWeight=pFile->fPDFMinLineFlatness=0.4;
			pFile->fPDFMaxLineWeight=pFile->fPDFMaxLineFlatness=1.2;
		}
		if(iOldType!=pFile->ciPDFLineMode)
		{	//更新属性栏UI
			CPropertyListOper<CSettingDlg> oper(pPropList,(CSettingDlg*)pPropList->GetParent());
			UpdatePdfLineModePropItem(&oper,pItem,TRUE,pFile->ciPDFLineMode);
			bReloadFile=TRUE;
		}
	}
	else if(CSettingDlg::GetPropID("m_fMinPdfLineWeight")==pItem->m_idProp)
	{
		double fOld=pFile->fPDFMinLineWeight;
		pFile->fPDFMinLineWeight=atof(valueStr);
		pFile->fPDFMinLineFlatness=pFile->fPDFMinLineWeight;
		bReloadFile=(fOld!=pFile->fPDFMinLineWeight);
	}
	else if(CSettingDlg::GetPropID("m_fMinPdfLineFlatness")==pItem->m_idProp)
	{
		double fOld=pFile->fPDFMinLineFlatness;
		pFile->fPDFMinLineFlatness=atof(valueStr);
		bReloadFile=(fOld!=pFile->fPDFMaxLineFlatness);
	}
	else if(CSettingDlg::GetPropID("m_fMaxPdfLineWeight")==pItem->m_idProp)
	{
		double fOld=pFile->fPDFMaxLineWeight;
		pFile->fPDFMaxLineWeight=atof(valueStr);
		pFile->fPDFMaxLineFlatness=pFile->fPDFMaxLineWeight;
		bReloadFile=(fOld!=pFile->fPDFMaxLineWeight);
	}
	else if(CSettingDlg::GetPropID("m_fMaxPdfLineFlatness")==pItem->m_idProp)
	{
		double fOld=pFile->fPDFMaxLineFlatness;
		pFile->fPDFMaxLineFlatness=atof(valueStr);
		bReloadFile=(fOld!=pFile->fPDFMaxLineFlatness);
	}
	if(bReloadFile)
	{
		CIBOMDoc *pDoc=theApp.GetDocument();
		CIBOMView *pView=theApp.GetIBomView();
		CDisplayDataPage *pDisplayPage=pView?pView->GetDataPage():NULL;
		if(pDoc!=NULL&&pFile!=NULL&&pView!=NULL&&pDisplayPage!=NULL)
		{
			CWaitCursor waitCursor;
			pFile->InitImageFile(pFile->szPathFileName,pDoc->GetFilePath());
			pDisplayPage->InvalidateImageWorkPanel();
		}
	}
	return true;
}

static BOOL ModifySettingValue(CPropertyList* pPropList,CPropTreeItem* pItem,CString& valueStr)
{	
	COLORREF curClr = 0;
	char tem_str[101]="";
	_snprintf(tem_str,100,"%s",valueStr);
	memmove(tem_str, tem_str+3, 97);	
	sscanf(tem_str,"%X",&curClr);
	//
	if(ModifyFilePropValue(pPropList,pItem,valueStr)||ModifyBomFilePropValue(pPropList,pItem,valueStr))
		return TRUE;
	if(CSettingDlg::GetPropID("MODIFY_COLOR")==pItem->m_idProp)
		CConfig::MODIFY_COLOR=curClr;
	else if(CSettingDlg::GetPropID("warningLevelClrArr[1]")==pItem->m_idProp)
		CConfig::warningLevelClrArr[1]=curClr;
	else if(CSettingDlg::GetPropID("warningLevelClrArr[2]")==pItem->m_idProp)
		CConfig::warningLevelClrArr[2]=curClr;
	else if(CSettingDlg::GetPropID("m_ciWaringMatchCoef")==pItem->m_idProp)
	{
		BYTE ciWaringMatchCoef=atoi(valueStr);
		if(ciWaringMatchCoef>100)
			ciWaringMatchCoef=100;
		if(ciWaringMatchCoef<=0)
			ciWaringMatchCoef=0;
		CConfig::m_ciWaringMatchCoef=ciWaringMatchCoef;
		IMindRobot::SetMaxWaringMatchCoef(CConfig::m_ciWaringMatchCoef);
	}
	else if(CSettingDlg::GetPropID("KEY_Q235")==pItem->m_idProp)
		CConfig::KEY_Q235=valueStr;
	else if(CSettingDlg::GetPropID("KEY_Q345")==pItem->m_idProp)
		CConfig::KEY_Q345=valueStr;
	else if(CSettingDlg::GetPropID("KEY_Q390")==pItem->m_idProp)
		CConfig::KEY_Q390=valueStr;
	else if(CSettingDlg::GetPropID("KEY_Q420")==pItem->m_idProp)
		CConfig::KEY_Q420=valueStr;
	else if(CSettingDlg::GetPropID("KEY_UP")==pItem->m_idProp)
		CConfig::KEY_UP=valueStr;
	else if(CSettingDlg::GetPropID("KEY_DOWN")==pItem->m_idProp)
		CConfig::KEY_DOWN=valueStr;
	else if(CSettingDlg::GetPropID("KEY_LEFT")==pItem->m_idProp)
		CConfig::KEY_LEFT=valueStr;
	else if(CSettingDlg::GetPropID("KEY_RIGHT")==pItem->m_idProp)
		CConfig::KEY_RIGHT=valueStr;
	else if(CSettingDlg::GetPropID("KEY_REPEAT")==pItem->m_idProp)
		CConfig::KEY_REPEAT=valueStr;
	else if(CSettingDlg::GetPropID("KEY_BIG_FAI")==pItem->m_idProp)
		CConfig::KEY_BIG_FAI=valueStr;
	else if(CSettingDlg::GetPropID("KEY_LITTLE_FAI")==pItem->m_idProp)
		CConfig::KEY_LITTLE_FAI=valueStr;
	else if(CSettingDlg::GetPropID("m_ciEditModel")==pItem->m_idProp)
		CConfig::m_ciEditModel=valueStr[0]-'0';
	else if(CSettingDlg::GetPropID("m_iFontsLibSerial")==pItem->m_idProp)
	{
		/*int iNew=valueStr[0]-'0';
		IAlphabets* pAlphabets=IMindRobot::GetAlphabetKnowledge();
		if(pAlphabets->ImportFontsFile(theApp.GetFontsLibPath(iNew,CIBOMApp::FONTS_LIB_FILE)))
		{
			pAlphabets->SetActiveFontSerial(iNew);
			CConfig::m_iFontsLibSerial=iNew;
		}
		else
			AfxMessageBox(CXhChar200("加载字体库-%d失败，恢复原有字体库！",iNew));*/
	}
	else if(CSettingDlg::GetPropID("m_bAutoSelectFontLib")==pItem->m_idProp)
	{
		if(valueStr.CompareNoCase("是")==0)
			CConfig::m_bAutoSelectFontLib=TRUE;
		else
			CConfig::m_bAutoSelectFontLib=FALSE;
		/*IAlphabets* pAlphabets=IMindRobot::GetAlphabetKnowledge();
		if(pAlphabets&&CConfig::m_bAutoSelectFontLib!=pAlphabets->IsAutoSelectFontLib())
			pAlphabets->SetAutoSelectFontLib(CConfig::m_bAutoSelectFontLib);*/
	}
	else if(CSettingDlg::GetPropID("m_bDisplayPromptMsg")==pItem->m_idProp)
	{
		BOOL bDisplay=CConfig::m_bDisplayPromptMsg;
		if(valueStr.CompareNoCase("是")==0)
			CConfig::m_bDisplayPromptMsg=TRUE;
		else
			CConfig::m_bDisplayPromptMsg=FALSE;
		if(bDisplay!=CConfig::m_bDisplayPromptMsg)
		{
			CIBOMView *pView=theApp.GetIBomView();
			CDisplayDataPage *pDisplayPage=pView?pView->GetDataPage():NULL;
			if(pDisplayPage)
				pDisplayPage->RelayoutWnd();
		}
	}
	else if(CSettingDlg::GetPropID("m_fInitPDFZoomScale")==pItem->m_idProp)
		CConfig::m_fInitPDFZoomScale=atof(valueStr);
	else if(CSettingDlg::GetPropID("m_iPdfLineMode")==pItem->m_idProp)
		CConfig::m_iPdfLineMode=atoi(valueStr);
	else if(CSettingDlg::GetPropID("PdfLineMode")==pItem->m_idProp)
	{
		int iOldType=CConfig::m_iPdfLineMode;
		CConfig::m_iPdfLineMode=valueStr[0]-'0';
		if(CConfig::m_iPdfLineMode==0)
		{	//正常
			CConfig::m_fMinPdfLineWeight=CConfig::m_fMinPdfLineFlatness=0;
			CConfig::m_fMaxPdfLineWeight=CConfig::m_fMaxPdfLineFlatness=0;
		}
		else if(CConfig::m_iPdfLineMode==1)
		{	//细
			CConfig::m_fMinPdfLineWeight=CConfig::m_fMinPdfLineFlatness=0.5;
			CConfig::m_fMaxPdfLineWeight=CConfig::m_fMaxPdfLineFlatness=0;
		}
		else if(CConfig::m_iPdfLineMode==2)
		{	//粗
			CConfig::m_fMinPdfLineWeight=CConfig::m_fMinPdfLineFlatness=0;
			CConfig::m_fMaxPdfLineWeight=CConfig::m_fMaxPdfLineFlatness=0.6;
		}
		else if(CConfig::m_iPdfLineMode==3)
		{	//自定义
			CConfig::m_fMinPdfLineWeight=CConfig::m_fMinPdfLineFlatness=0.4;
			CConfig::m_fMaxPdfLineWeight=CConfig::m_fMaxPdfLineFlatness=1.2;
		}
		if(iOldType!=CConfig::m_iPdfLineMode)
		{	//更新属性栏UI
			CPropertyListOper<CSettingDlg> oper(pPropList,(CSettingDlg*)pPropList->GetParent());
			UpdatePdfLineModePropItem(&oper,pItem,TRUE,CConfig::m_iPdfLineMode);
		}
	}
	else if(CSettingDlg::GetPropID("m_fMinPdfLineWeight")==pItem->m_idProp)
	{
		CConfig::m_fMinPdfLineWeight=atof(valueStr);
		CConfig::m_fMinPdfLineFlatness=CConfig::m_fMinPdfLineWeight;
	}
	else if(CSettingDlg::GetPropID("m_fMinPdfLineFlatness")==pItem->m_idProp)
		CConfig::m_fMinPdfLineFlatness=atof(valueStr);
	else if(CSettingDlg::GetPropID("m_fMaxPdfLineWeight")==pItem->m_idProp)
	{
		CConfig::m_fMaxPdfLineWeight=atof(valueStr);
		CConfig::m_fMaxPdfLineFlatness=CConfig::m_fMaxPdfLineWeight;
	}
	else if(CSettingDlg::GetPropID("m_fMaxPdfLineFlatness")==pItem->m_idProp)
		CConfig::m_fMaxPdfLineFlatness=atof(valueStr);
	else if(CSettingDlg::GetPropID("m_bListenScanner")==pItem->m_idProp)
	{
		if(valueStr.CompareNoCase("是")==0)
			CConfig::m_bListenScanner=TRUE;
		else
			CConfig::m_bListenScanner=FALSE;
	}
	else if(CSettingDlg::GetPropID("m_iAutoSaveType")==pItem->m_idProp)
	{
		if(valueStr.CompareNoCase("单文件备份")==0)
			CConfig::m_iAutoSaveType=0;
		else
			CConfig::m_iAutoSaveType=1;
		theApp.GetDocument()->ResetAutoSaveTimer();
	}
	else if(CSettingDlg::GetPropID("m_nAutoSaveTime")==pItem->m_idProp)
	{
		CConfig::m_nAutoSaveTime=atoi(valueStr)*60000;
		theApp.GetDocument()->ResetAutoSaveTimer();
	}
	else if(CSettingDlg::GetPropID("m_bRecogPieceWeight")==pItem->m_idProp)
	{
		if(valueStr.CompareNoCase("是")==0)
			CConfig::m_bRecogPieceWeight=TRUE;
		else
			CConfig::m_bRecogPieceWeight=FALSE;
	}
	else if(CSettingDlg::GetPropID("m_bRecogSumWeight")==pItem->m_idProp)
	{
		if(valueStr.CompareNoCase("是")==0)
			CConfig::m_bRecogSumWeight=TRUE;
		else
			CConfig::m_bRecogSumWeight=FALSE;
	}
	else if(CSettingDlg::GetPropID("m_fWeightEPS")==pItem->m_idProp)
	{
		CConfig::m_fWeightEPS=atof(valueStr);
	}
	return true;
}

// CSettingDlg 对话框

IMPLEMENT_DYNAMIC(CSettingDlg, CDialogEx)

int CSettingDlg::m_iCurrentTabPage=0;
CSettingDlg::CSettingDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSettingDlg::IDD, pParent)
{
	m_pImageFile=NULL;
	m_pModel=NULL;
}

CSettingDlg::~CSettingDlg()
{
}

void CSettingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SETTING_LIST, m_propList);
	DDX_Control(pDX, IDC_TAB_GROUP, m_ctrlPropGroup);
}


BEGIN_MESSAGE_MAP(CSettingDlg, CDialogEx)
	ON_BN_CLICKED(ID_BTN_DEFAULT, &CSettingDlg::OnBnClickedBtnDefault)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_GROUP, OnSelchangeTabGroup)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CSettingDlg message handlers
BOOL CSettingDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_ctrlPropGroup.DeleteAllItems();
	if(m_pImageFile||m_pModel)
	{
		m_ctrlPropGroup.InsertItem(0,"常规");
		SetWindowText("文件属性");
		CWnd *pDefaultBtn=GetDlgItem(ID_BTN_DEFAULT);
		if(pDefaultBtn)
			pDefaultBtn->ShowWindow(SW_HIDE);
		CWnd *pOKBtn=GetDlgItem(IDOK);
		if(pOKBtn)
		{	//左移确定按钮
			const int OFFSET_DIST=100;
			CRect rc;
			pOKBtn->GetWindowRect(&rc);
			ScreenToClient(&rc);
			rc.left-=OFFSET_DIST;
			rc.right-=OFFSET_DIST;
			pOKBtn->MoveWindow(rc);
		}
	}
	else
	{
		m_ctrlPropGroup.InsertItem(0,"常规");
		m_ctrlPropGroup.InsertItem(1,"快捷键");
	}
	m_ctrlPropGroup.SetCurSel(0);
	m_propList.m_hPromptWnd=GetDlgItem(IDC_E_PROP_HELP_STR)->GetSafeHwnd();
	m_propList.m_iPropGroup=0;
	m_propList.SetDividerScale(0.5);
	//
	DisplayPropSetting();
	UpdateData(FALSE);
	return TRUE;
}

void CSettingDlg::OnOK() 
{
	CConfig::SaveToFile(NULL);
	CIBOMView *pView=theApp.GetIBomView();
	if(pView)
		pView->ShiftView(pView->GetDisType());
	CDialog::OnOK();
}
void CSettingDlg::OnCancel() 
{
	CDialog::OnCancel();
}

IMPLEMENT_PROP_FUNC(CSettingDlg)

void CSettingDlg::InitBomFilePropHashTbl()
{
	int id = 1;
	const DWORD HASHTABLESIZE = 500;
	const DWORD STATUSHASHTABLESIZE = 50;
	propHashtable.Empty();
	propStatusHashtable.Empty();
	propHashtable.SetHashTableGrowSize(HASHTABLESIZE);
	propStatusHashtable.CreateHashTable(STATUSHASHTABLESIZE);
	AddPropItem("FilePara",PROPLIST_ITEM(id++,"文件属性"));
	AddPropItem("m_sFileVersion",PROPLIST_ITEM(id++,"文件版本号"));
	AddPropItem("m_uiOriginalDogKey",PROPLIST_ITEM(id++,"原始加密锁号"));
	AddPropItem("m_uiLastSaveDogKey",PROPLIST_ITEM(id++,"当前加密锁号"));
	AddPropItem("m_sPrjCode",PROPLIST_ITEM(id++,"工程代号"));
	AddPropItem("m_sPrjName",PROPLIST_ITEM(id++,"工程名称"));
	AddPropItem("m_sTowerTypeName",PROPLIST_ITEM(id++,"塔型名"));
	AddPropItem("m_sTowerAlias",PROPLIST_ITEM(id++,"塔型代号"));
	AddPropItem("m_sTaStampNo",PROPLIST_ITEM(id++,"钢印号"));
	AddPropItem("m_sOperator",PROPLIST_ITEM(id++,"操作员"));
	AddPropItem("m_sAuditor",PROPLIST_ITEM(id++,"审核员"));
	AddPropItem("m_sCritic",PROPLIST_ITEM(id++,"评审员"));
}

void CSettingDlg::InitImageFilePropHashTbl()
{
	int id = 1;
	const DWORD HASHTABLESIZE = 500;
	const DWORD STATUSHASHTABLESIZE = 50;
	propHashtable.Empty();
	propStatusHashtable.Empty();
	propHashtable.SetHashTableGrowSize(HASHTABLESIZE);
	propStatusHashtable.CreateHashTable(STATUSHASHTABLESIZE);
	AddPropItem("FilePara",PROPLIST_ITEM(id++,"文件属性"));
	AddPropItem("m_sFileName",PROPLIST_ITEM(id++,"文件名称"));
	AddPropItem("m_sFileType",PROPLIST_ITEM(id++,"文件类型"));
	AddPropItem("m_sFilePath",PROPLIST_ITEM(id++,"文件路径"));
	AddPropItem("PDFPara",PROPLIST_ITEM(id++,"PDF参数"));
	AddPropItem("m_fInitPDFZoomScale",PROPLIST_ITEM(id++,"PDF缩放比例"));
	AddPropItem("PdfLineMode",PROPLIST_ITEM(id++,"PDF线宽模式",NULL,"0.正常|1.加粗|2.描细|3.自动"));
	AddPropItem("m_fMinPdfLineWeight",PROPLIST_ITEM(id++,"PDF线条最小宽度"));
	AddPropItem("m_fMinPdfLineFlatness",PROPLIST_ITEM(id++,"PDF线条最小厚度"));
	AddPropItem("m_fMaxPdfLineWeight",PROPLIST_ITEM(id++,"PDF线条最大宽度"));
	AddPropItem("m_fMaxPdfLineFlatness",PROPLIST_ITEM(id++,"PDF线条最大厚度"));
}

void CSettingDlg::InitSystemSettingPropHashTbl()
{	//常规
	int id = 1;
	const DWORD HASHTABLESIZE = 500;
	const DWORD STATUSHASHTABLESIZE = 50;
	propHashtable.Empty();
	propStatusHashtable.Empty();
	propHashtable.SetHashTableGrowSize(HASHTABLESIZE);
	propStatusHashtable.CreateHashTable(STATUSHASHTABLESIZE);
	AddPropItem("UI",PROPLIST_ITEM(id++,"操作模式"));
	AddPropItem("m_ciEditModel",PROPLIST_ITEM(id++,"编辑模式"));
	AddPropItem("m_bDisplayPromptMsg",PROPLIST_ITEM(id++,"显示帮助"));
	AddPropItem("SysPara",PROPLIST_ITEM(id++,"系统配置"));
	AddPropItem("m_iFontsLibSerial",PROPLIST_ITEM(id++,"字体库"));
	AddPropItem("m_bAutoSelectFontLib",PROPLIST_ITEM(id++,"自动选择字体库"));
	if(VerifyValidFunction(LICFUNC::FUNC_IDENTITY_INTERNAL_TEST))
		AddPropItem("m_bListenScanner",PROPLIST_ITEM(id++,"监听扫描仪"));
	AddPropItem("m_iAutoSaveType",PROPLIST_ITEM(id++,"备份类型"));
	AddPropItem("m_nAutoSaveTime",PROPLIST_ITEM(id++,"自动保存时间"));
	AddPropItem("m_bRecogPieceWeight",PROPLIST_ITEM(id++,"识别单重"));
	AddPropItem("m_bRecogSumWeight",PROPLIST_ITEM(id++,"识别总重"));
	AddPropItem("m_fWeightEPS",PROPLIST_ITEM(id++,"重量对比误差值"));
	AddPropItem("PDFPara",PROPLIST_ITEM(id++,"PDF参数"));
	AddPropItem("m_fInitPDFZoomScale",PROPLIST_ITEM(id++,"PDF初始缩放比例"));
	AddPropItem("PdfLineMode",PROPLIST_ITEM(id++,"PDF线宽模式",NULL,"0.正常|1.加粗|2.描细|3.自动"));
	AddPropItem("m_fMinPdfLineWeight",PROPLIST_ITEM(id++,"PDF线条最小宽度"));
	AddPropItem("m_fMinPdfLineFlatness",PROPLIST_ITEM(id++,"PDF线条最小厚度"));
	AddPropItem("m_fMaxPdfLineWeight",PROPLIST_ITEM(id++,"PDF线条最大宽度"));
	AddPropItem("m_fMaxPdfLineFlatness",PROPLIST_ITEM(id++,"PDF线条最大厚度"));
	AddPropItem("Color",PROPLIST_ITEM(id++,"颜色"));
	AddPropItem("MODIFY_COLOR",PROPLIST_ITEM(id++,"修改内容背景色","修改内容背景色"));
	AddPropItem("warningLevelClrArr[1]",PROPLIST_ITEM(id++,"警告颜色","识别警告颜色"));
	AddPropItem("warningLevelClrArr[2]",PROPLIST_ITEM(id++,"错误颜色","识别错误颜色"));
	AddPropItem("m_ciWaringMatchCoef",PROPLIST_ITEM(id++,"识别匹配系数","取值范围0-100，100表示完全匹配"));
	AddPropItem("Material",PROPLIST_ITEM(id++,"材质快捷键"));
	AddPropItem("KEY_Q235",PROPLIST_ITEM(id++,"Q235"));
	AddPropItem("KEY_Q345",PROPLIST_ITEM(id++,"Q345"));
	AddPropItem("KEY_Q390",PROPLIST_ITEM(id++,"Q390"));
	AddPropItem("KEY_Q420",PROPLIST_ITEM(id++,"Q420"));
	AddPropItem("Operation",PROPLIST_ITEM(id++,"操作快捷键"));
	AddPropItem("KEY_UP",PROPLIST_ITEM(id++,"上"));
	AddPropItem("KEY_DOWN",PROPLIST_ITEM(id++,"下"));
	AddPropItem("KEY_LEFT",PROPLIST_ITEM(id++,"左"));
	AddPropItem("KEY_RIGHT",PROPLIST_ITEM(id++,"右"));
	AddPropItem("KEY_REPEAT",PROPLIST_ITEM(id++,"重复上一单元格内容"));
	AddPropItem("SpecialChar",PROPLIST_ITEM(id++,"特殊字符快捷键"));
	AddPropItem("KEY_BIG_FAI",PROPLIST_ITEM(id++,"大写Φ快捷键"));
	AddPropItem("KEY_LITTLE_FAI",PROPLIST_ITEM(id++,"小写φ快捷键"));
}
void CSettingDlg::InitPropHashtable()
{
	//InitImageFilePropHashTbl(id);
	//InitSystemSettingPropHashTbl(id);
}
int CSettingDlg::GetImageFilePropValueStr(long id, char *valueStr,UINT nMaxStrBufLen/*=100*/,CPropTreeItem *pItem/*=NULL*/)
{
	if(m_pImageFile==NULL)
		return 0;
	CXhChar100 sText;
	if(GetPropID("m_sFileName")==id)
		sText.Copy(m_pImageFile->szFileName);
	else if(GetPropID("m_sFileType")==id)
	{
		//static const BYTE FILE_TYPE_NONE	= 0;
		//static const BYTE FILE_TYPE_JPEG	= 1;
		//static const BYTE FILE_TYPE_PNG	= 2;
		//static const BYTE FILE_TYPE_PDF	= 3;
		BYTE biFileType=m_pImageFile->GetRawFileType();
		if(IImageFile::RAW_IMAGE_JPG==biFileType)
			sText.Copy("1.Jpg图像文件");
		else if(IImageFile::RAW_IMAGE_PNG==biFileType)
			sText.Copy("2.Png图像文件");
		else if(IImageFile::RAW_IMAGE_PDF==biFileType)
			sText.Copy("3.Pdf矢量文件");
		else if(IImageFile::RAW_IMAGE_PDF_IMG==biFileType)
			sText.Copy("4.Pdf图像文件");
		else if(IImageFile::RAW_IMAGE_NONE==biFileType)
			sText.Copy("0.未知类型");
	}
	else if(GetPropID("m_sFilePath")==id)
		sText.NCopy(m_pImageFile->szPathFileName,100);
	else if(GetPropID("m_fInitPDFZoomScale")==id)
		sText.Printf("%.2f",m_pImageFile->fPDFZoomScale);
	else if(GetPropID("PdfLineMode")==id)
	{
		BYTE ciPdfLineMode=m_pImageFile->ciPDFLineMode;
		CXhChar100 sModelArr[4]={"0.正常","1.加粗","2.描细","3.自动"};
		if(ciPdfLineMode>=1&&ciPdfLineMode<=3)
			sText.Copy(sModelArr[ciPdfLineMode]);
		else
			sText.Copy(sModelArr[0]);
	}
	else if(GetPropID("m_fMinPdfLineWeight")==id)
		sText.Printf("%.4f",m_pImageFile->fPDFMinLineWeight);
	else if(GetPropID("m_fMinPdfLineFlatness")==id)
		sText.Printf("%.4f",m_pImageFile->fPDFMinLineFlatness);
	else if(GetPropID("m_fMaxPdfLineWeight")==id)
		sText.Printf("%.4f",m_pImageFile->fPDFMaxLineWeight);
	else if(GetPropID("m_fMaxPdfLineFlatness")==id)
		sText.Printf("%.4f",m_pImageFile->fPDFMaxLineFlatness);
	if(valueStr)
		StrCopy(valueStr,sText,nMaxStrBufLen);
	return strlen(sText);
}

int CSettingDlg::GetBomFilePropValueStr(long id, char *valueStr,UINT nMaxStrBufLen/*=100*/,CPropTreeItem *pItem/*=NULL*/)
{
	if(m_pModel==NULL)
		return 0;
	CXhChar100 sText;
	if(GetPropID("m_sFileVersion")==id)
		sText.Copy(m_pModel->m_sFileVersion);
	else if (GetPropID("m_uiOriginalDogKey")==id)
		sText.Printf("%d",m_pModel->m_uiOriginalDogKey);
	else if (GetPropID("m_uiLastSaveDogKey")==id)
		sText.Printf("%d",m_pModel->m_uiLastSaveDogKey);
	else if (GetPropID("m_sPrjCode")==id)
		sText.Printf("%s",(char*)m_pModel->m_sPrjCode);
	else if (GetPropID("m_sPrjName")==id)
		sText.Printf("%s",(char*)m_pModel->m_sPrjName);
	else if (GetPropID("m_sTowerTypeName")==id)
		sText.Printf("%s",(char*)m_pModel->m_sTowerTypeName);
	else if (GetPropID("m_sTowerAlias")==id)
		sText.Printf("%s",(char*)m_pModel->m_sTowerAlias);
	else if (GetPropID("m_sTaStampNo")==id)
		sText.Printf("%s",(char*)m_pModel->m_sTaStampNo);
	else if (GetPropID("m_sOperator")==id)
		sText.Printf("%s",(char*)m_pModel->m_sOperator);
	else if (GetPropID("m_sAuditor")==id)
		sText.Printf("%s",(char*)m_pModel->m_sAuditor);
	else if (GetPropID("m_sCritic")==id)
		sText.Printf("%s",(char*)m_pModel->m_sAuditor);
	if(valueStr)
		StrCopy(valueStr,sText,nMaxStrBufLen);
	return strlen(sText);
}

int CSettingDlg::GetPropValueStr(long id,char* valueStr,UINT nMaxStrBufLen/*=100*/,CPropTreeItem *pItem/*=NULL*/)
{
	if(m_pImageFile)
		return GetImageFilePropValueStr(id,valueStr,nMaxStrBufLen,pItem);
	else if(m_pModel)
		return GetBomFilePropValueStr(id,valueStr,nMaxStrBufLen,pItem);
	BOOL bContinueFind=FALSE;	
	long handle=0;
	double dLineWeitht=0;
	CXhChar100 sText;
	if(GetPropID("MODIFY_COLOR")==id)
		sText.Printf("RGB%X",CConfig::MODIFY_COLOR);
	else if(GetPropID("warningLevelClrArr[1]")==id)
		sText.Printf("RGB%X",CConfig::warningLevelClrArr[1]);
	else if(GetPropID("warningLevelClrArr[2]")==id)
		sText.Printf("RGB%X",CConfig::warningLevelClrArr[2]);
	else if(GetPropID("KEY_Q235")==id)
		sText.Copy(CConfig::KEY_Q235);
	else if(GetPropID("KEY_Q345")==id)
		sText.Copy(CConfig::KEY_Q345);
	else if(GetPropID("KEY_Q390")==id)
		sText.Copy(CConfig::KEY_Q390);
	else if(GetPropID("KEY_Q420")==id)
		sText.Copy(CConfig::KEY_Q420);
	else if(GetPropID("KEY_LEFT")==id)
		sText.Copy(CConfig::KEY_LEFT);
	else if(GetPropID("KEY_RIGHT")==id)
		sText.Copy(CConfig::KEY_RIGHT);
	else if(GetPropID("KEY_UP")==id)
		sText.Copy(CConfig::KEY_UP);
	else if(GetPropID("KEY_DOWN")==id)
		sText.Copy(CConfig::KEY_DOWN);
	else if(GetPropID("KEY_REPEAT")==id)
		sText.Copy(CConfig::KEY_REPEAT);
	else if(GetPropID("KEY_BIG_FAI")==id)
		sText.Copy(CConfig::KEY_BIG_FAI);
	else if(GetPropID("KEY_LITTLE_FAI")==id)
		sText.Copy(CConfig::KEY_LITTLE_FAI);
	else if(GetPropID("m_ciWaringMatchCoef")==id)
		sText.Printf("%d",CConfig::m_ciWaringMatchCoef);
	else if(GetPropID("m_ciEditModel")==id)
	{
		if(CConfig::m_ciEditModel==1)
			sText.Copy("1.双击编辑");
		else
			sText.Copy("0.单击编辑");
	}
	else if(GetPropID("m_iFontsLibSerial")==id)
	{
		/*IAlphabets* pAlphabets=IMindRobot::GetAlphabetKnowledge();
		if(pAlphabets&&CConfig::m_iFontsLibSerial!=pAlphabets->GetActiveFontSerial())
			CConfig::m_iFontsLibSerial=pAlphabets->GetActiveFontSerial();*/
		sText.Printf("%d",CConfig::m_iFontsLibSerial);
	}
	else if(GetPropID("m_bAutoSelectFontLib")==id)
	{
		if(CConfig::m_bAutoSelectFontLib)
			sText.Copy("是");
		else
			sText.Copy("否");

	}
	else if(GetPropID("m_bDisplayPromptMsg")==id)
	{
		if(CConfig::m_bDisplayPromptMsg)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if(GetPropID("m_fInitPDFZoomScale")==id)
		sText.Printf("%.2f",CConfig::m_fInitPDFZoomScale);
	else if(GetPropID("PdfLineMode")==id)
	{
		CXhChar100 sModelArr[4]={"0.正常","1.加粗","2.描细","3.自动"};
		if(CConfig::m_iPdfLineMode>=1&&CConfig::m_iPdfLineMode<=3)
			sText.Copy(sModelArr[CConfig::m_iPdfLineMode]);
		else
			sText.Copy(sModelArr[0]);
	}
	else if(GetPropID("m_fMinPdfLineWeight")==id)
		sText.Printf("%.4f",CConfig::m_fMinPdfLineWeight);
	else if(GetPropID("m_fMinPdfLineFlatness")==id)
		sText.Printf("%.4f",CConfig::m_fMinPdfLineFlatness);
	else if(GetPropID("m_fMaxPdfLineWeight")==id)
		sText.Printf("%.4f",CConfig::m_fMaxPdfLineWeight);
	else if(GetPropID("m_fMaxPdfLineFlatness")==id)
		sText.Printf("%.4f",CConfig::m_fMaxPdfLineFlatness);
	else if(GetPropID("m_bListenScanner")==id)
	{
		if(CConfig::m_bListenScanner)
			sText.Copy("是");
		else
			sText.Copy("否");
	}
	else if(GetPropID("m_iAutoSaveType")==id)
	{
		if(CConfig::m_iAutoSaveType==0)
			strcpy(sText,"单文件备份");
		else
			strcpy(sText,"多文件备份");
	}
	else if(GetPropID("m_nAutoSaveTime")==id)
	{
		sprintf(sText,"%d", CConfig::m_nAutoSaveTime/60000);
	}
	else if(GetPropID("m_bRecogPieceWeight")==id)
	{
		if(CConfig::m_bRecogPieceWeight)
			strcpy(sText,"是");
		else
			strcpy(sText,"否");
	}
	else if(GetPropID("m_bRecogSumWeight")==id)
	{
		if(CConfig::m_bRecogSumWeight)
			strcpy(sText,"是");
		else
			strcpy(sText,"否");
	}
	else if(GetPropID("m_fWeightEPS")==id)
	{
		sText.Printf("%.1f",CConfig::m_fWeightEPS);
		SimplifiedNumString(sText);
	}
	if(valueStr)
		StrCopy(valueStr,sText,nMaxStrBufLen);
	return strlen(sText);
}
void CSettingDlg::DisplaySystemSetting()
{
	CPropTreeItem *pPropItem=NULL,*pGroupItem=NULL,*pSonPropItem=NULL,*pRootItem=m_propList.GetRootItem();
	//设置回调函数
	m_propList.CleanTree();
	m_propList.SetModifyValueFunc(ModifySettingValue);
	const int GROUP_GENERAL=1,GROUP_SHORTCUT=2;
	//颜色
	CPropertyListOper<CSettingDlg> oper(&m_propList,this);
	pGroupItem=oper.InsertPropItem(pRootItem,"SysPara");
	pGroupItem->m_bHideChildren=FALSE;
	pGroupItem->m_dwPropGroup=GetSingleWord(GROUP_GENERAL);
	oper.InsertCmbEditPropItem(pGroupItem,"m_ciWaringMatchCoef","40|45|50|55|60|65|70|75|80|85|90|85|100");
	//oper.InsertCmbListPropItem(pGroupItem,"m_iFontsLibSerial","0|1|2|3|4|5|6|7|8|9|10|");
	//oper.InsertCmbListPropItem(pGroupItem,"m_bAutoSelectFontLib","是|否");
	oper.InsertCmbListPropItem(pGroupItem,"m_bListenScanner","是|否");
	oper.InsertCmbListPropItem(pGroupItem,"m_iAutoSaveType","单文件备份|多文件备份");
	oper.InsertEditPropItem(pGroupItem,"m_nAutoSaveTime");
	oper.InsertCmbListPropItem(pGroupItem,"m_bRecogPieceWeight","是|否");
	oper.InsertCmbListPropItem(pGroupItem,"m_bRecogSumWeight","是|否");
	oper.InsertEditPropItem(pGroupItem,"m_fWeightEPS");
	pGroupItem=oper.InsertPropItem(pRootItem,"UI");
	pGroupItem->m_bHideChildren=FALSE;
	oper.InsertCmbListPropItem(pGroupItem,"m_ciEditModel","0.单击编辑|1.双击编辑");
	oper.InsertCmbListPropItem(pGroupItem,"m_bDisplayPromptMsg","是|否");
	pGroupItem=oper.InsertPropItem(pRootItem,"PDFPara");
	pGroupItem->m_bHideChildren=FALSE;
	oper.InsertEditPropItem(pGroupItem,"m_fInitPDFZoomScale");
	CPropTreeItem *pPdfGroupItem=oper.InsertCmbListPropItem(pGroupItem,"PdfLineMode","0.正常|1.加粗|2.描细|3.自动");
	UpdatePdfLineModePropItem(&oper,pPdfGroupItem,FALSE,CConfig::m_iPdfLineMode);
	pGroupItem=oper.InsertPropItem(pRootItem,"Color");
	pGroupItem->m_bHideChildren=FALSE;
	pGroupItem->m_dwPropGroup=GetSingleWord(GROUP_GENERAL);
	oper.InsertCmbColorPropItem(pGroupItem,"MODIFY_COLOR");
	oper.InsertCmbColorPropItem(pGroupItem,"warningLevelClrArr[1]");
	oper.InsertCmbColorPropItem(pGroupItem,"warningLevelClrArr[2]");
	//
	pGroupItem=oper.InsertPropItem(pRootItem,"Material");
	pGroupItem->m_bHideChildren=FALSE;
	pGroupItem->m_dwPropGroup=GetSingleWord(GROUP_SHORTCUT);
	oper.InsertEditPropItem(pGroupItem,"KEY_Q235");
	oper.InsertEditPropItem(pGroupItem,"KEY_Q345");
	oper.InsertEditPropItem(pGroupItem,"KEY_Q390");
	oper.InsertEditPropItem(pGroupItem,"KEY_Q420");
	//
	pGroupItem=oper.InsertPropItem(pRootItem,"Operation");
	pGroupItem->m_bHideChildren=FALSE;
	pGroupItem->m_dwPropGroup=GetSingleWord(GROUP_SHORTCUT);
	oper.InsertEditPropItem(pGroupItem,"KEY_UP");
	oper.InsertEditPropItem(pGroupItem,"KEY_DOWN");
	oper.InsertEditPropItem(pGroupItem,"KEY_LEFT");
	oper.InsertEditPropItem(pGroupItem,"KEY_RIGHT");
	oper.InsertEditPropItem(pGroupItem,"KEY_REPEAT");
	pGroupItem=oper.InsertPropItem(pRootItem,"SpecialChar");
	pGroupItem->m_bHideChildren=FALSE;
	pGroupItem->m_dwPropGroup=GetSingleWord(GROUP_SHORTCUT);
	oper.InsertEditPropItem(pGroupItem,"KEY_BIG_FAI");
	oper.InsertEditPropItem(pGroupItem,"KEY_LITTLE_FAI");
	m_propList.Redraw();
}
void CSettingDlg::DisplayImageFileProp()
{
	CPropTreeItem *pPropItem=NULL,*pGroupItem=NULL,*pSonPropItem=NULL,*pRootItem=m_propList.GetRootItem();
	//设置回调函数
	m_propList.CleanTree();
	m_propList.SetModifyValueFunc(ModifySettingValue);
	//颜色
	CPropertyListOper<CSettingDlg> oper(&m_propList,this);
	pGroupItem=oper.InsertPropItem(pRootItem,"FilePara");
	pGroupItem->m_bHideChildren=FALSE;
	CPropTreeItem *pItem=oper.InsertEditPropItem(pGroupItem,"m_sFileName");
	pItem->SetReadOnly();
	pItem=oper.InsertEditPropItem(pGroupItem,"m_sFileType");
	pItem->SetReadOnly();
	pItem=oper.InsertEditPropItem(pGroupItem,"m_sFilePath");
	pItem->SetReadOnly();
	if(m_pImageFile->IsSrcFromPdfFile())
	{
		pGroupItem=oper.InsertPropItem(pRootItem,"PDFPara");
		pGroupItem->m_bHideChildren=FALSE;
		oper.InsertEditPropItem(pGroupItem,"m_fInitPDFZoomScale");
		CPropTreeItem *pPdfGroupItem=oper.InsertCmbListPropItem(pGroupItem,"PdfLineMode","0.正常|1.加粗|2.描细|3.自动");
		UpdatePdfLineModePropItem(&oper,pPdfGroupItem,FALSE,m_pImageFile->ciPDFLineMode);
	}
	m_propList.Redraw();
}
void CSettingDlg::DisplayBomFileProp()
{
	CPropTreeItem *pPropItem=NULL,*pGroupItem=NULL,*pSonPropItem=NULL,*pRootItem=m_propList.GetRootItem();
	//设置回调函数
	m_propList.CleanTree();
	m_propList.SetModifyValueFunc(ModifySettingValue);
	if(m_pModel==NULL)
		return;
	CPropertyListOper<CSettingDlg> oper(&m_propList,this);
	pGroupItem=oper.InsertPropItem(pRootItem,"FilePara");
	pGroupItem->m_bHideChildren=FALSE;
	CPropTreeItem *pItem=oper.InsertEditPropItem(pGroupItem,"m_sFileVersion");
	pItem->SetReadOnly();
	if(m_pModel->m_uiOriginalDogKey==m_pModel->m_uiLastSaveDogKey)
	{
		pItem=oper.InsertEditPropItem(pGroupItem,"m_uiLastSaveDogKey","加密锁号");
		pItem->SetReadOnly();
	}
	else
	{
		pItem=oper.InsertEditPropItem(pGroupItem,"m_uiOriginalDogKey");
		pItem->SetReadOnly();
		pItem=oper.InsertEditPropItem(pGroupItem,"m_uiLastSaveDogKey");
		pItem->SetReadOnly();
	}
	oper.InsertEditPropItem(pGroupItem,"m_sPrjCode");		//工程代号
	oper.InsertEditPropItem(pGroupItem,"m_sPrjName");		//工程的名称
	oper.InsertEditPropItem(pGroupItem,"m_sTowerTypeName");	//塔型
	oper.InsertEditPropItem(pGroupItem,"m_sTowerAlias");	//代号
	oper.InsertEditPropItem(pGroupItem,"m_sTaStampNo");		//钢印号
	oper.InsertEditPropItem(pGroupItem,"m_sOperator");		//操作员
	oper.InsertEditPropItem(pGroupItem,"m_sAuditor");		//审查员
	oper.InsertEditPropItem(pGroupItem,"m_sCritic");		//评审人

	m_propList.Redraw();
}
void CSettingDlg::DisplayPropSetting()
{
	if(m_pImageFile)
	{
		InitImageFilePropHashTbl();
		DisplayImageFileProp();
	}
	else if(m_pModel)
	{
		InitBomFilePropHashTbl();
		DisplayBomFileProp();
	}
	else
	{
		InitSystemSettingPropHashTbl();
		DisplaySystemSetting();
	}
}
void CSettingDlg::OnBnClickedBtnDefault()
{
	CConfig::InitDefaultSetting();
	DisplayPropSetting();
}

void CSettingDlg::OnSelchangeTabGroup(NMHDR* pNMHDR, LRESULT* pResult) 
{
	int iCurSel = m_ctrlPropGroup.GetCurSel();
	m_propList.m_iPropGroup=iCurSel;
	m_propList.Redraw();
	*pResult = 0;
}