#include "StdAfx.h"
#include <math.h>
#include "Recog.h"
#include "ImageHost.h"
#include "Tool.h"
#include "ArrayList.h"
#include "XhCharString.h"
#include "LogFile.h"
#include "DataModel.h"
#include "TempFile.h"
#include "Config.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
//////////////////////////////////////////////////////////////////////////
//CImageRegionHost
void SetInputBomPartKey(IRecoginizer::BOMPART *pPart,DWORD key)
{
	pPart->id=key;
}
CImageRegionHost::CImageRegionHost()
{
	m_pInternalRegion=NULL;
	m_dwKey=0;
	m_pBelongImageFile=NULL;
	hashInputPartById.SetMinmalId(0x10000000);
	hashInputPartById.LoadDefaultObjectInfo=SetInputBomPartKey;
}
CImageRegionHost::~CImageRegionHost()
{
}
//balancecoef 划分黑白点间的界限调整平衡系数,取值-0.5~0.5;0时对应最高频率像素前移20位
double CImageRegionHost::SetMonoThresholdBalanceCoef(double mono_balance_coef/*=0*/)
{
	return m_pInternalRegion->SetMonoThresholdBalanceCoef(mono_balance_coef);
}
double CImageRegionHost::GetMonoThresholdBalanceCoef()
{
	return m_pInternalRegion->GetMonoThresholdBalanceCoef();
}
int CImageRegionHost::get_MonoForwardPixels(){return m_pInternalRegion->MonoForwardPixels;}
//显示及操作函数
void CImageRegionHost::InitImageShowPara(RECT showRect)
{
	m_pInternalRegion->InitImageShowPara(showRect);
}
double CalPartWeight(IRecoginizer::BOMPART *pPart);

typedef struct tagPART_LABEL 
{
	IRecoginizer::BOMPART *m_pPart;
	SEGI segI;
	SEGI serialNo;
	tagPART_LABEL(IRecoginizer::BOMPART *pPart=NULL){
		Init(pPart);
	}
	void Init(IRecoginizer::BOMPART *pPart){
		m_pPart=pPart;
		if(m_pPart)
		{
			CXhChar50 sSerialNo;
			ParsePartNo(pPart->sLabel,&segI,sSerialNo);
			serialNo=SEGI(sSerialNo);
		}
		else
		{
			segI.iSeg=0;
			serialNo=SEGI((long)0);
		}
	}
}PART_LABEL;
//根据规则检查构件属性
void CheckBomPartByRule(IRecoginizer::BOMPART *pBomPart,CImageRegionHost *pRegion=NULL)
{	
	if(pBomPart!=NULL)
	{	//1、检查规格合理性
		if(pBomPart->arrItemWarningLevel[2]==0)
		{
			if(pBomPart->wPartType==IRecoginizer::BOMPART::ANGLE)
			{	//角钢
				JG_PARA_TYPE *pJgPara=CPartLibrary::FindJgParaBySpec(pBomPart->sSizeStr);
				if(pJgPara==NULL)
					pBomPart->arrItemWarningLevel[2]=2;
			}
			else if(pBomPart->wPartType==IRecoginizer::BOMPART::PLATE)
			{	//钢板
				if(strlen(pBomPart->sSizeStr)>0&&(pBomPart->thick>100||pBomPart->width<10))	//判断钢板厚度合理性
					pBomPart->arrItemWarningLevel[2]=2;
			}
			else if(pBomPart->wPartType==IRecoginizer::BOMPART::TUBE)
			{	//钢管
				TUBE_PARA_TYPE *pTubePara=CPartLibrary::FindTubeParaBySpec(pBomPart->sSizeStr);
				if(pTubePara==NULL)
					pBomPart->arrItemWarningLevel[2]=2;
			}
			else if(pBomPart->wPartType==IRecoginizer::BOMPART::ROUND)
			{

			}
		}
		//2、材质列
		if(pBomPart->arrItemWarningLevel[1]==0)
		{
			if( strlen(pBomPart->materialStr)>0&&
				strstr(pBomPart->materialStr,"Q235")==NULL&&
				strstr(pBomPart->materialStr,"Q345")==NULL&&
				strstr(pBomPart->materialStr,"Q390")==NULL&&
				strstr(pBomPart->materialStr,"Q420")==NULL)
				pBomPart->arrItemWarningLevel[1]=2;
		}
		//3、件号
		if(pBomPart->arrItemWarningLevel[0]==0)
		{
			if( pBomPart->siSubType==IRecoginizer::BOMPART::SUB_TYPE_PLATE_X||
				pBomPart->siSubType==IRecoginizer::BOMPART::SUB_TYPE_PLATE_C||
				pBomPart->siSubType==IRecoginizer::BOMPART::SUB_TYPE_PLATE_U||
				pBomPart->siSubType==IRecoginizer::BOMPART::SUB_TYPE_PLATE_FLD||
				pBomPart->siSubType==IRecoginizer::BOMPART::SUB_TYPE_PLATE_FLP)
			{	//标准件
				FL_PARAM *pFlPara=NULL;
				INSERT_PLATE_PARAM* pInsertPlate=NULL;
				if(pBomPart->siSubType==IRecoginizer::BOMPART::SUB_TYPE_PLATE_C)
					pInsertPlate=FindRollEndParam(pBomPart->sLabel);
				else if(pBomPart->siSubType==IRecoginizer::BOMPART::SUB_TYPE_PLATE_U)
					pInsertPlate=FindUEndParam(pBomPart->sLabel);
				else if(pBomPart->siSubType==IRecoginizer::BOMPART::SUB_TYPE_PLATE_X)
					pInsertPlate=FindXEndParam(pBomPart->sLabel);
				else if(pBomPart->siSubType==IRecoginizer::BOMPART::SUB_TYPE_PLATE_FLD)
					pFlPara=FindFlDParam(pBomPart->sLabel);
				else if(pBomPart->siSubType==IRecoginizer::BOMPART::SUB_TYPE_PLATE_FLP)
					pFlPara=FindFlPParam(pBomPart->sLabel);
				if(pInsertPlate==NULL&&pFlPara==NULL)
					pBomPart->arrItemWarningLevel[0]=1;
			}
			else if(pBomPart->wPartType==IRecoginizer::BOMPART::ROUND||pBomPart->wPartType==0)
			{
				AIDED_PART *pPart=CPartLibrary::FindAidedPart(pBomPart->sLabel);
				if(pPart==NULL)
					pBomPart->arrItemWarningLevel[0]=1;
			}
		}
		//4、单重列
		if(CConfig::m_bRecogPieceWeight&&pBomPart->arrItemWarningLevel[5]==0)
		{
			if(fabs(pBomPart->weight-pBomPart->calWeight)>CConfig::m_fWeightEPS)
				pBomPart->arrItemWarningLevel[5]=1;	//单重与理论单重不一致
		}
		//5、总重列
		if(CConfig::m_bRecogSumWeight&&pBomPart->arrItemWarningLevel[6]==0)
		{
			double theorySumWeight=atof(CXhChar50("%.2f",pBomPart->calWeight))*pBomPart->count;
			if(fabs(pBomPart->sumWeight-theorySumWeight)>CConfig::m_fWeightEPS)
				pBomPart->arrItemWarningLevel[6]=1;	//总重与理论总重不一致
		}
		if(CConfig::m_bRecogPieceWeight&&CConfig::m_bRecogSumWeight)
		{	
			double fCalSumWeight=pBomPart->weight*pBomPart->count;
			if( fabs(fCalSumWeight-pBomPart->sumWeight)<0.1&&
				pBomPart->arrItemWarningLevel[5]>0&&
				pBomPart->arrItemWarningLevel[2]>0)
			{	//总重=单重x件数,但是单重与通过规格计算的重量不一致，此时根据反向查找规格 wht 18-12-22
				double unit_weight=pBomPart->weight/(pBomPart->length*0.001);	//根据长度计算单位重量
				//根据单位重量查表匹配规格
				JG_PARA_TYPE *pJgType=(pBomPart->wPartType==IRecoginizer::BOMPART::ANGLE)?CPartLibrary::FindJgParaByUnitWeight(unit_weight):NULL;
				TUBE_PARA_TYPE *pTubeType=(pBomPart->wPartType==IRecoginizer::BOMPART::TUBE)?CPartLibrary::FindTubeParaByUnitWeight(unit_weight):NULL;
				if(pJgType)
				{
					pBomPart->wPartType=IRecoginizer::BOMPART::ANGLE;
					pBomPart->width=pJgType->wing_wide;
					pBomPart->thick=pJgType->wing_thick;
					sprintf(pBomPart->sSizeStr,"L%.0fX%.0f",pBomPart->width,pBomPart->thick);
					pBomPart->arrItemWarningLevel[5]=0;
				}
				else if(pTubeType)
				{
					pBomPart->wPartType=IRecoginizer::BOMPART::TUBE;
					pBomPart->width=pTubeType->D;
					pBomPart->thick=pTubeType->thick;
					sprintf(pBomPart->sSizeStr,"φ%.0fX%.0f",pBomPart->width,pBomPart->thick);
					pBomPart->arrItemWarningLevel[5]=0;
				}
			}
			else if(pBomPart->arrItemWarningLevel[0]==0&&
					pBomPart->arrItemWarningLevel[1]==0&&
					pBomPart->arrItemWarningLevel[2]==0&&
					pBomPart->arrItemWarningLevel[3]==0&&
					pBomPart->arrItemWarningLevel[4]==0&&
					pBomPart->count>0&&
					(pBomPart->arrItemWarningLevel[5]+pBomPart->arrItemWarningLevel[6])==1)
			{	//前5列识别没有错误，只有单重或总重错误，可以通过一个反算另外一个，减少手动修改 wht 18-12-22
				if(pBomPart->arrItemWarningLevel[5]==1)
				{	//根据总重反算单重
					double weight=pBomPart->sumWeight/pBomPart->count;
					if(fabs(weight-pBomPart->weight)>1)
						pBomPart->weight=weight;
				}
				else if(pBomPart->arrItemWarningLevel[6]==1)
				{	//根据单重计算总重
					double sumWeight=atof(CXhChar50("%.2f",pBomPart->weight))*pBomPart->count;
					if(sumWeight>1)
						pBomPart->sumWeight=sumWeight;
				}
			}
		}
	}
	else if(pBomPart==NULL&&pRegion)
	{
		ARRAY_LIST<PART_LABEL> partLabelArr;
		for(pBomPart=pRegion->EnumFirstPart();pBomPart;pBomPart=pRegion->EnumNextPart())
		{	//sPartNo="Φ1Φ.Φ110Φ"
			PART_LABEL *pPartLabel=partLabelArr.append();
			pPartLabel->Init(pBomPart);
		}
		DWORD nPartNum=partLabelArr.Size();
		for(DWORD i=0;i<partLabelArr.Size();i++)
		{
			if(partLabelArr[i].m_pPart==NULL)
				continue;
			PART_LABEL *pPrevPart=i>0?&partLabelArr[(i-1)]:NULL;
			PART_LABEL *pNextPart=i<nPartNum-1?&partLabelArr[i+1]:NULL;
			bool bTubeStdPartPart=false;
			if(strlen(partLabelArr[i].m_pPart->sSizeStr)>0&&partLabelArr[i].m_pPart->length>0)
			{
				if(pPrevPart&&pPrevPart->m_pPart&&pPrevPart->m_pPart->arrItemWarningLevel[0]==0)
				{	
					if(partLabelArr[i].segI!=pPrevPart->segI)
						partLabelArr[i].m_pPart->arrItemWarningLevel[0]=2;
					else if(partLabelArr[i].serialNo.Suffix().Length<=0&&pPrevPart->serialNo.Suffix().Length<=0&&
							partLabelArr[i].serialNo!=pPrevPart->serialNo+1)
						partLabelArr[i].m_pPart->arrItemWarningLevel[0]=2;
					else if(partLabelArr[i].serialNo.iSeg>200)	//存在前后缀字母且序号大于200 wht 18-07-25
						partLabelArr[i].m_pPart->arrItemWarningLevel[0]=2;
				}
				if(pNextPart&&pNextPart->m_pPart&&pNextPart->m_pPart->arrItemWarningLevel[0]==0)
				{	
					if(partLabelArr[i].segI!=pNextPart->segI)
						partLabelArr[i].m_pPart->arrItemWarningLevel[0]=2;
					else if(partLabelArr[i].serialNo.Suffix().Length<=0&&pNextPart->serialNo.Suffix().Length<=0&&
							partLabelArr[i].serialNo!=pNextPart->serialNo-1)
						partLabelArr[i].m_pPart->arrItemWarningLevel[0]=2;
					else if(partLabelArr[i].serialNo.iSeg>200)	//存在前后缀字母且序号大于200 wht 18-07-25
						partLabelArr[i].m_pPart->arrItemWarningLevel[0]=2;
				}
				else if(pNextPart==NULL)
				{
					if(partLabelArr[i].serialNo.iSeg>200)	//存在前后缀字母且序号大于200 wht 18-07-25
						partLabelArr[i].m_pPart->arrItemWarningLevel[0]=2;
				}
			}
		}
	}
}

//NX-21 修订为 NX-2.1 
//临时修订特殊构件，以后应根据用户输入自动修订错误构件，避免同一套图纸中重复录入同一种错误 wht 18-06-10
void TempCorrectBomPartProperty(CImageRegionHost *pRegion)
{
	IRecoginizer::BOMPART *pBomPart=NULL;
	for(pBomPart=pRegion->EnumFirstPart();pBomPart;pBomPart=pRegion->EnumNextPart())
	{
		if(pBomPart->arrItemWarningLevel[0]!=0)
		{	//拉锁附件
			if(stricmp("JK0-241",pBomPart->sLabel)==0||strstr(pBomPart->sLabel,"NX-2")!=NULL||strstr(pBomPart->sLabel,"-241")!=NULL)
				strcpy(pBomPart->sLabel,"NX-2.1");
			else if(strstr(pBomPart->sLabel,"N-10")!=NULL)
				strcpy(pBomPart->sLabel,"U-10");
			else if(stricmp("N0-2",pBomPart->sLabel)==0)
				strcpy(pBomPart->sLabel,"NUT-2");
		}
		//钢格栅
		if( pBomPart->arrItemWarningLevel[2]!=0&&pBomPart->arrItemWarningLevel[3]!=0&&
			(strstr(pBomPart->sSizeStr,"G253/")!=NULL||
			(strlen(pBomPart->sSizeStr)>=10&&strstr(pBomPart->sSizeStr,"L")==NULL&&strstr(pBomPart->sSizeStr,"X")==NULL)))
		{
			strcpy(pBomPart->sSizeStr,"G253/400/100WFG");
			pBomPart->length=4600;
			pBomPart->width=500;
			pBomPart->wPartType=IRecoginizer::BOMPART::ACCESSORY;
			pBomPart->siSubType=IRecoginizer::BOMPART::SUB_TYPE_STEEL_GRATING;	//附件钢格栅
		}
	}
}

BOOL CImageRegionHost::SummarizePartInfo()
{
	int iLogCellRow=-1,iLogCellRowEnd=0xffff,iLogCellCol=-1;
	//int iLogCellRow=0,iLogCellRowEnd=1,iLogCellCol=1;
#ifdef _DEBUG
	if(!Recognize(0,iLogCellRow,iLogCellRowEnd,iLogCellCol))
#else
	if(!Recognize())
#endif 
		return FALSE;
	IRecoginizer::BOMPART bompart,*pBomPart=NULL,*pExistBomPart=NULL;
	bompart.Clear();
	hashParts.Empty();
	for(bool validate=EnumFirstBomPart(&bompart);validate;validate=EnumNextBomPart(&bompart))
	{
		int labelstrlen=strlen(bompart.sLabel);
		CXhChar100 keylabel=bompart.sLabel;
		do
		{
			if(pExistBomPart=hashParts.GetValue(keylabel))
			{
				keylabel.Append('+');
				labelstrlen++;
				if(pExistBomPart->arrItemWarningLevel[0]==0)
					pExistBomPart->arrItemWarningLevel[0]=1;
				bompart.arrItemWarningLevel[0]=3;
			}
		}while(labelstrlen<16&&pExistBomPart!=NULL);
		pBomPart=AddBomPart(bompart.sLabel[0]==0?"???":keylabel);
		pBomPart->id=bompart.id;
		strcpy(pBomPart->sLabel,bompart.sLabel);
		strcpy(pBomPart->sSizeStr,bompart.sSizeStr);
		strcpy(pBomPart->materialStr,bompart.materialStr);
		memcpy(pBomPart->arrItemWarningLevel,bompart.arrItemWarningLevel,7);
		pBomPart->ciSrcFromMode=0;
		pBomPart->rc=bompart.rc;
		if(strlen(bompart.sSizeStr)>0)
		{
			if(ParseSpec(pBomPart))
			{
				if(pBomPart->wPartType==IRecoginizer::BOMPART::ANGLE)
					sprintf(pBomPart->sSizeStr,"L%sX%s",(char*)CXhChar16(pBomPart->width),(char*)CXhChar16(pBomPart->thick));
				else if(pBomPart->wPartType==IRecoginizer::BOMPART::TUBE)
					sprintf(pBomPart->sSizeStr,"φ%sX%s",(char*)CXhChar16(pBomPart->width),(char*)CXhChar16(pBomPart->thick));
				else if(pBomPart->wPartType==IRecoginizer::BOMPART::PLATE)
				{
					char* pszSizeStr=strchr(pBomPart->sSizeStr,'-');
					if(pszSizeStr)
						StrCopy(pBomPart->sSizeStr,pszSizeStr,16);
					else
						sprintf(pBomPart->sSizeStr,"-%d",(int)pBomPart->thick);
				}
				if( pBomPart->wPartType==IRecoginizer::BOMPART::ANGLE||
					pBomPart->wPartType==IRecoginizer::BOMPART::TUBE||
					pBomPart->wPartType==IRecoginizer::BOMPART::PLATE)
				{
					if(pBomPart->thick<1||pBomPart->width<1)
						pBomPart->arrItemWarningLevel[2]=2;	//存在识别错误的字符
				}
			}
			else
				pBomPart->arrItemWarningLevel[2]=2;	//存在识别错误的字符
		}
		pBomPart->count=bompart.count;
		pBomPart->length=bompart.length;
		if(CConfig::m_bRecogPieceWeight)
			pBomPart->weight=bompart.weight;
		else
			pBomPart->weight=0;
		if(CConfig::m_bRecogSumWeight)
			pBomPart->sumWeight=bompart.sumWeight;
		else
			pBomPart->sumWeight=0;
		CalAndSyncTheoryWeightToWeight(pBomPart,FALSE);
		if(pBomPart->weight==0||pBomPart->count==0)
			pBomPart->arrItemWarningLevel[6]=2;	//总重为0时显示红色
		//
		CheckBomPartByRule(pBomPart);
		memcpy(bompart.arrItemWarningLevel,pBomPart->arrItemWarningLevel,7);
	}
	CheckBomPartByRule(NULL,this);
	TempCorrectBomPartProperty(this);
	//
	
	return TRUE;
}

BOOL CImageRegionHost::SyncInputPartToSummaryParts()
{
	//1.从hashParts中删除inputPart
	for(IRecoginizer::BOMPART *pBomPart=hashParts.GetFirst();pBomPart;pBomPart=hashParts.GetNext())
	{
		if(hashInputPartById.GetValue(pBomPart->id))
			hashParts.DeleteCursor();
	}
	hashParts.Clean();
	//2.添加inputPart至hashParts
	IRecoginizer::BOMPART *pExistBomPart=NULL;
	for(IRecoginizer::BOMPART *pBomPart=hashInputPartById.GetFirst();pBomPart;pBomPart=hashInputPartById.GetNext())
	{
		int labelstrlen=strlen(pBomPart->sLabel);
		CXhChar100 keylabel=pBomPart->sLabel;
		do
		{
			if(pExistBomPart=hashParts.GetValue(keylabel))
			{
				keylabel.Append('+');
				labelstrlen++;
				if(pExistBomPart->arrItemWarningLevel[0]==0)
					pExistBomPart->arrItemWarningLevel[0]=1;
				pBomPart->arrItemWarningLevel[0]=3;
			}
		}while(labelstrlen<16&&pExistBomPart!=NULL);
		IRecoginizer::BOMPART *pNewBomPart=AddBomPart(pBomPart->sLabel[0]==0?"???":keylabel);
		pNewBomPart->id=pBomPart->id;
		pNewBomPart->bInput=pBomPart->bInput;
		strcpy(pNewBomPart->sLabel,pBomPart->sLabel);
		strcpy(pNewBomPart->sSizeStr,pBomPart->sSizeStr);
		strcpy(pNewBomPart->materialStr,pBomPart->materialStr);
		memcpy(pNewBomPart->arrItemWarningLevel,pBomPart->arrItemWarningLevel,7);
		if(strlen(pNewBomPart->sSizeStr)>0)
			ParseSpec(pNewBomPart);
		pNewBomPart->ciSrcFromMode=0;
		pNewBomPart->rc=pBomPart->rc;
		pNewBomPart->count=pBomPart->count;
		pNewBomPart->length=pBomPart->length;
		pNewBomPart->weight=pBomPart->weight;
		pNewBomPart->sumWeight=pBomPart->sumWeight;
		CalAndSyncTheoryWeightToWeight(pNewBomPart,FALSE);
		if(pNewBomPart->weight==0||pBomPart->count==0)
			pNewBomPart->arrItemWarningLevel[6]=2;	//总重为0时显示红色
		CheckBomPartByRule(pNewBomPart);
	}
	return TRUE;
}

int CImageRegionHost::GetRecogPartNum()
{
	int nCount=0;
	for(IRecoginizer::BOMPART *pPart=hashParts.GetFirst();pPart;pPart=hashParts.GetNext())
	{
		if(hashInputPartById.GetValue(pPart->id))
			nCount++;
	}
	return (hashParts.GetNodeNum()-nCount);
}
//接口函数实现
long CImageRegionHost::GetSerial()
{
	return m_pInternalRegion->GetSerial();
}
IImageFile* CImageRegionHost::BelongImageData() const
{
	return m_pInternalRegion->BelongImageData();
}
POINT CImageRegionHost::TopLeft(bool bRealRegion/*=false*/)
{
	return m_pInternalRegion->TopLeft(bRealRegion);
}
POINT CImageRegionHost::TopRight(bool bRealRegion/*=false*/)
{
	return m_pInternalRegion->TopRight(bRealRegion);
}
POINT CImageRegionHost::BtmLeft(bool bRealRegion/*=false*/)
{
	return m_pInternalRegion->BtmLeft(bRealRegion);
}
POINT CImageRegionHost::BtmRight(bool bRealRegion/*=false*/)
{
	return m_pInternalRegion->BtmRight(bRealRegion);
}
bool CImageRegionHost::SetRegion(POINT topLeft,POINT btmLeft,POINT btmRight,POINT topRight,bool bRealRegion/*=false*/,bool bRetrieveRegion/*=true*/)
{
	return m_pInternalRegion->SetRegion(topLeft,btmLeft,btmRight,topRight,bRealRegion,bRetrieveRegion);
}
int CImageRegionHost::GetMonoImageData(IMAGE_DATA* imagedata)	//每位代表1个像素，0表示白点,1表示黑点
{
	return m_pInternalRegion->GetMonoImageData(imagedata);
}
int CImageRegionHost::GetRectMonoImageData(RECT* rc,IMAGE_DATA* imagedata)		//每位代表1个像素，0表示白点,1表示黑点
{
	return m_pInternalRegion->GetRectMonoImageData(rc,imagedata);
}
int CImageRegionHost::GetImageColumnWidth(int* col_width_arr/*=NULL*/)		//返回列数
{
	int nCol=m_pInternalRegion->GetImageColumnWidth(col_width_arr);
	if(col_width_arr==NULL)
	{
		if(nCol==0)
			return m_arrColWidth.Size();
		else
			return nCol;
	}
	else //if(col_width_arr!=NULL)
	{
		if(nCol==0)
		{
			nCol=m_arrColWidth.Size();
			for(int i=0;i<nCol;i++)
				col_width_arr[i]=m_arrColWidth[i];
		}
		double fZoomScale=m_pInternalRegion->ZoomRatio();
		for(int i=0;i<nCol;i++)
		{
			double fRelaWidth=col_width_arr[i]*fZoomScale;
			col_width_arr[i]=(int)fRelaWidth;
		}
		return nCol;
	}
}
int CImageRegionHost::GetImageOrgColumnWidth(int* col_width_arr/*=NULL*/)
{
	int nCol=m_pInternalRegion->GetImageColumnWidth(col_width_arr);
	if(col_width_arr==NULL)
	{
		if(nCol==0)
			return m_arrColWidth.Size();
		else
			return nCol;
	}
	else //if(col_width_arr!=NULL)
	{
		if(nCol==0)
		{
			nCol=m_arrColWidth.Size();
			for(int i=0;i<nCol;i++)
				col_width_arr[i]=m_arrColWidth[i];
		}
		return nCol;
	}
}
#ifdef _DEBUG
int CImageRegionHost::Recognize(char modeOfAuto0BomPart1Summ2/*=0*/,int iCellRowStart/*=-1*/,int iCellRowEnd/*=-1*/,int iCellCol/*=-1*/)	//-1表示全部,>=0表示指定行或列 wjh-2018.3.14
{	return m_pInternalRegion->Recognize(modeOfAuto0BomPart1Summ2,iCellRowStart,iCellRowEnd,iCellCol);}
#else
int CImageRegionHost::Recognize(char modeOfAuto0BomPart1Summ2/*=0*/)
{	return m_pInternalRegion->Recognize(modeOfAuto0BomPart1Summ2);}
#endif
bool CImageRegionHost::RetrieveCharImageData(int iCellRow,int iCellCol,IMAGE_CHARSET_DATA* imagedata)
{
	return m_pInternalRegion->RetrieveCharImageData(iCellRow,iCellCol,imagedata);
}
bool CImageRegionHost::Destroy()
{
	return m_pInternalRegion->Destroy();
}
bool CImageRegionHost::EnumFirstBomPart(IRecoginizer::BOMPART* pBomPart)
{
	return m_pInternalRegion->EnumFirstBomPart(pBomPart);
}
bool CImageRegionHost::EnumNextBomPart(IRecoginizer::BOMPART* pBomPart)
{
	return m_pInternalRegion->EnumNextBomPart(pBomPart);
}
void CImageRegionHost::FromBuffer(CBuffer& buffer,long version/*=0*/)
{	//读入图像识别分区信息
	int nNum=0,nPartNum=0;
	CXhChar100 sName;
	CPoint topLeft,btmLeft,btmRight,topRight;
	buffer.ReadString(sName);
	buffer.ReadInteger(&topLeft.x);
	buffer.ReadInteger(&topLeft.y);
	buffer.ReadInteger(&btmLeft.x);
	buffer.ReadInteger(&btmLeft.y);
	buffer.ReadInteger(&btmRight.x);
	buffer.ReadInteger(&btmRight.y);
	buffer.ReadInteger(&topRight.x);
	buffer.ReadInteger(&topRight.y);
	SetRegion(topLeft,btmLeft,btmRight,topRight,false,false);
	m_sName=sName;
	if(version==NULL||version>=10003)	//读取区域明暗度
	{
		double fCoef=0;
		buffer.ReadDouble(&fCoef);
		SetMonoThresholdBalanceCoef(fCoef);
	}
	//保存图像中各列宽度
	int nColSize=0;
	buffer.ReadInteger(&nColSize);
	if(nColSize>0)
	{
		m_arrColWidth.Resize(nColSize);
		buffer.Read(m_arrColWidth,sizeof(int)*nColSize);
	}
	//
	buffer.ReadInteger(&nPartNum);
	CXhChar16 sPartNo;
	IRecoginizer::BOMPART* pBomPart=NULL;
	hashParts.Empty();
	hashInputPartById.Empty();
	for(int j=0;j<nPartNum;j++)
	{
		buffer.ReadString(sPartNo);
		CXhChar100 keylabel=sPartNo;
		if(sPartNo.Length==0)
			keylabel.Copy("???");
		pBomPart=AddBomPart(keylabel);
		strcpy(pBomPart->sLabel,sPartNo);
		buffer.ReadInteger(&pBomPart->id);
		buffer.ReadByte(&pBomPart->ciSrcFromMode);
		buffer.ReadInteger(&pBomPart->rc.left);
		buffer.ReadInteger(&pBomPart->rc.top);
		buffer.ReadInteger(&pBomPart->rc.right);
		buffer.ReadInteger(&pBomPart->rc.bottom);
		buffer.ReadWord(&pBomPart->wPartType);
		buffer.ReadString(pBomPart->sSizeStr);
		buffer.ReadString(pBomPart->materialStr);
		buffer.ReadWord(&pBomPart->count);
		buffer.ReadDouble(&pBomPart->length);
		buffer.ReadDouble(&pBomPart->weight);
		if(version==0||version>=1000800)
			buffer.ReadDouble(&pBomPart->sumWeight);
		if(version==0||version>=10002)
			buffer.Read(&pBomPart->arrItemWarningLevel,7);
		if(version==0||version>=10006)
			buffer.ReadBooleanThin(&pBomPart->bInput);
		if(pBomPart->bInput)
		{
			IRecoginizer::BOMPART *pNewBomPart=hashInputPartById.Append(pBomPart->id);
			strcpy(pNewBomPart->sLabel,pBomPart->sLabel);
			pNewBomPart->id=pBomPart->id;
			pNewBomPart->ciSrcFromMode=pBomPart->ciSrcFromMode;
			pNewBomPart->rc=pBomPart->rc;
			pNewBomPart->wPartType=pBomPart->wPartType;
			strcpy(pNewBomPart->sSizeStr,pBomPart->sSizeStr);
			strcpy(pNewBomPart->materialStr,pBomPart->materialStr);
			pNewBomPart->count=pBomPart->count;
			pNewBomPart->length=pBomPart->length;
			pNewBomPart->weight=pBomPart->weight;
			memcpy(pNewBomPart->arrItemWarningLevel,pBomPart->arrItemWarningLevel,7);
			pNewBomPart->bInput=pBomPart->bInput;
		}
	}
}
void CImageRegionHost::ToBuffer(CBuffer& buffer,long version/*=0*/)
{
	buffer.WriteString(m_sName);
	buffer.WriteInteger(TopLeft().x);
	buffer.WriteInteger(TopLeft().y);
	buffer.WriteInteger(BtmLeft().x);
	buffer.WriteInteger(BtmLeft().y);
	buffer.WriteInteger(BtmRight().x);
	buffer.WriteInteger(BtmRight().y);
	buffer.WriteInteger(TopRight().x);
	buffer.WriteInteger(TopRight().y);
	if(version==NULL||version>=10003)	//存储区域明暗度
		buffer.WriteDouble(GetMonoThresholdBalanceCoef());
	//保存图像中各列宽度
	int nColSize=GetImageColumnWidth();
	buffer.WriteInteger(nColSize);
	if(nColSize>0)
	{
		DYN_ARRAY<int> colarr(nColSize);
		GetImageOrgColumnWidth(colarr);
		buffer.Write(colarr,sizeof(int)*nColSize);
	}
	SyncInputPartToSummaryParts();
	buffer.WriteInteger(GetPartNum());
	for(IRecoginizer::BOMPART* pPart=EnumFirstPart();pPart;pPart=EnumNextPart())
	{
		buffer.WriteString(pPart->sLabel);
		buffer.WriteInteger(pPart->id);
		buffer.WriteByte(pPart->ciSrcFromMode);
		buffer.WriteInteger(pPart->rc.left);
		buffer.WriteInteger(pPart->rc.top);
		buffer.WriteInteger(pPart->rc.right);
		buffer.WriteInteger(pPart->rc.bottom);
		buffer.WriteWord(pPart->wPartType);
		buffer.WriteString(pPart->sSizeStr);
		buffer.WriteString(pPart->materialStr);
		buffer.WriteWord(pPart->count);
		buffer.WriteDouble(pPart->length);
		buffer.WriteDouble(pPart->weight);
		if(version==0||version>=1000800)
			buffer.WriteDouble(pPart->sumWeight);
		if(version==0||version>=10002)
			buffer.Write(pPart->arrItemWarningLevel,7);
		if(version==0||version>=10006)
			buffer.WriteBooleanThin(pPart->bInput);
	}
}
/////////////////////////////////////////////////////////////////////////////////
//CImageFileHost
void POLYLINE_4PT::Init()
{
	m_iActivePt=-1;
	m_cState=STATE_NONE;
	memset(m_ptArr,0,sizeof(POINT)*4);
}
CImageFileHost::CImageFileHost()
{
	m_pInternalImgFile=NULL;
	m_biImageSaveType=0;
	m_dwAddressJgpFile=0;
	m_dwSizeJgpFile=0;
	m_pdfConfig.page_no=1;
	m_pdfConfig.rotation=0;
	m_pdfConfig.zoom_scale=0;
	m_pdfConfig.min_line_weight=0;
	m_pdfConfig.min_line_flatness=0;
	m_pdfConfig.max_line_weight=0;
	m_pdfConfig.max_line_flatness=0;
}
CImageFileHost::~CImageFileHost()
{
	//if(m_pInternalImgFile)
		//delete m_pInternalImgFile;
	if(m_pInternalImgFile)
		IMindRobot::DestroyImageFile(m_pInternalImgFile->GetSerial());
}
BOOL CImageFileHost::InitFileAddressAndSize(DWORD file_pos,DWORD file_size,BYTE save_type)
{
	m_dwAddressJgpFile=file_pos;
	m_dwSizeJgpFile=file_size;
	m_biImageSaveType=save_type;
	return (m_dwAddressJgpFile>0);
}
void CImageFileHost::FromBuffer(CBuffer& buffer,long version/*=0*/)
{
	buffer.ReadString(m_sAliasName,m_sAliasName.LengthMax);
	//读入图像信息
	//biType=0.表示按灰度图像压缩存储;
	//		=1.表示按原始jpg文件存储（实践证明原始的jpg文件图像压缩率更高而且不存在IBOM转换失真问题）
	buffer.ReadByte(&m_biImageSaveType);
	UINT uiImageSize=0;
	m_dwAddressJgpFile=buffer.GetCursorPosition();
	buffer.ReadInteger(&uiImageSize);
	m_dwSizeJgpFile=uiImageSize;
	buffer.SeekOffset(uiImageSize);
	/*CHAR_ARRAY image(uiImageSize);
	buffer.Read(image,uiImageSize);
	if(uiImageSize>0)
	{
		if(m_biImageSaveType==0)
			m_pInternalImgFile->SetCompressImageData(image,uiImageSize);
		else
			m_pInternalImgFile->SetJpegImageData(image,uiImageSize);
	}*/
	//读入临时选取的区域矩形框（未提取为CImageRegion之前出于调试错误考虑也需临时存储） wjh-2018.4.8
	if(version==0||version>=10001)
	{
		buffer.ReadByte(&m_xPolyline.m_cState);
		buffer.ReadByte(&m_xPolyline.m_iActivePt);
		for(int i=0;i<4;i++)
		{
			buffer.ReadInteger(&m_xPolyline.m_ptArr[i].x);
			buffer.ReadInteger(&m_xPolyline.m_ptArr[i].y);
		}
	}
	//读入图像识别分区信息
	int nNum=0,nPartNum=0;
	buffer.ReadInteger(&nNum);
	CXhChar100 sName;
	CPoint topLeft,btmLeft,btmRight,topRight;
	for(int i=0;i<nNum;i++)
	{
		CImageRegionHost* pRegionHost=AppendRegion();
		pRegionHost->FromBuffer(buffer,version);
	}
}

static UINT ReadImageFile(const char* file_path,char* imageData,UINT uiMaxDataSize,BYTE *pbiImageSaveType=NULL)
{
	if(file_path==NULL)
		return 0;
	char extname[9]={0};
	_splitpath(file_path,NULL,NULL,NULL,extname);
	BYTE iType=0;
	if(stricmp(extname,".jpg")==0||stricmp(extname,".jpeg")==0)
		iType=1;
	else if(stricmp(extname,".pdf")==0)
		iType=2;
	else
		return 0;
	FILE* fp=fopen(file_path,"r+b");
	if(fp==NULL)
		return 0;
	if(imageData==NULL)
	{
		fseek(fp,0,SEEK_END);
		return ftell(fp);
	}
	const int POOLSIZE=0x80000;
	char pool[POOLSIZE]={0};//512k
	DWORD dwReadLen=0;
	CBuffer buff(imageData,uiMaxDataSize);
	fseek(fp,0,SEEK_SET);
	while(feof(fp)!=EOF)
	{
		dwReadLen=(DWORD)fread(pool,1,POOLSIZE,fp);
		DWORD dwWrite=min(dwReadLen,buff.GetRemnantSize());
		buff.Write(pool,dwWrite);
		if(dwWrite<dwReadLen||dwReadLen<POOLSIZE)
			break;
	}
	if(pbiImageSaveType)
		*pbiImageSaveType=iType;
	return buff.GetLength();
}

BOOL CImageFileHost::InitTempFileBuffer(CBuffer &buffer,const char* sTempPath)
{
	if(sTempPath==NULL)
		return FALSE;
	m_tempFileBuffer.DeleteFile();
	//创建新的临时文件
	HANDLE hCurrProcess=GetCurrentProcess();
	DWORD processid=GetProcessId(hCurrProcess);
	CXhChar200 szTempFile("%sprocess-%d#image_jpg_file_%d.jpg",sTempPath,processid,GetSerial());
	if(m_biImageSaveType==2)
		szTempFile.Printf("%sprocess-%d#image_pdf_file_%d.pdf",sTempPath,processid,GetSerial());
	m_tempFileBuffer.CreateFile(szTempFile,0xa00000);	//缓存大小为10M
	//
	buffer.SeekPosition(m_dwAddressJgpFile);
	long uiImageSize=buffer.ReadInteger();
	if(uiImageSize!=m_dwSizeJgpFile)
		return FALSE;
	CHAR_ARRAY image(uiImageSize);
	buffer.Read(image,uiImageSize);
	m_tempFileBuffer.Write(image,uiImageSize);
	m_tempFileBuffer.SeekToBegin();
	return TRUE;
}

void CImageFileHost::ToBuffer(CBuffer& buffer,long version/*=0*/,DWORD address_file_pos/*=0*/,
							  DWORD file_size/*=0*/,BYTE save_type/*=0*/,bool bAutoBakSave/*=false*/)
{
	buffer.WriteString(m_sAliasName,m_sAliasName.LengthMax);
	//写入图像信息
	CHAR_ARRAY image;
	UINT uiImageSize=0;
	if(address_file_pos==0)
	{
		if((uiImageSize=ReadImageFile(szPathFileName,NULL,NULL))>0)	//m_biImageSaveType==0&&
		{
			image.Resize(uiImageSize);
			ReadImageFile(szPathFileName,image,uiImageSize,&m_biImageSaveType);
		}
		else if((m_biImageSaveType==1||m_biImageSaveType==2)&&
				m_tempFileBuffer.IsValidFile()&&(uiImageSize=m_tempFileBuffer.GetLength())>0)
		{
			image.Resize(uiImageSize);
			m_tempFileBuffer.SeekToBegin();
			m_tempFileBuffer.Read(image,uiImageSize);
			//m_pInternalImgFile->GetJpegImageData(image,uiImageSize);
		}
		else
		{
			uiImageSize=m_pInternalImgFile->GetCompressImageData(NULL);
			image.Resize(uiImageSize);
			m_pInternalImgFile->GetCompressImageData(image,uiImageSize);
		}	
		//biType=0.表示按灰度图像压缩存储;
		//		=1.表示按原始jpg文件存储（实践证明原始的jpg文件图像压缩率更高而且不存在IBOM转换失真问题）
		buffer.WriteByte(m_biImageSaveType);
		if(!bAutoBakSave)
		{
			m_dwAddressJgpFile=buffer.GetCursorPosition();
			m_dwSizeJgpFile=uiImageSize;
		}
		buffer.WriteInteger(uiImageSize);
		buffer.Write(image,uiImageSize);
		if(uiImageSize==0)
		{
			CXhChar500 sFilePath(szPathFileName);
			if(sFilePath.GetLength()>0&&sFilePath.StartWith('\\'))
				logerr.Log("存储图像时出错。请将网络共享中的文件拷贝至本地，然后重试！");
			else
				logerr.Log("存储图像时出错！");
		}
	}
	else
	{
		if(!bAutoBakSave)
		{
			m_dwAddressJgpFile=address_file_pos;
			m_dwSizeJgpFile=file_size;
			m_biImageSaveType=save_type;
		}
		buffer.WriteByte(m_biImageSaveType);
		buffer.WriteInteger(uiImageSize);
		buffer.Write(image,uiImageSize);
	}
	//读入临时选取的区域矩形框（未提取为CImageRegion之前出于调试错误考虑也需临时存储） wjh-2018.4.8
	if(version==0||version>=10001)
	{
		buffer.WriteByte(m_xPolyline.m_cState);
		buffer.WriteByte(m_xPolyline.m_iActivePt);
		for(int i=0;i<4;i++)
		{
			buffer.WriteInteger(m_xPolyline.m_ptArr[i].x);
			buffer.WriteInteger(m_xPolyline.m_ptArr[i].y);
		}
	}
	//写入图像识别分区信息
	int nNum=m_hashRegionById.GetNodeNum();
	buffer.WriteInteger(nNum);
	CImageRegionHost* pRegionHost=NULL;
	for(pRegionHost=m_hashRegionById.GetFirst();pRegionHost;pRegionHost=m_hashRegionById.GetNext())
		pRegionHost->ToBuffer(buffer,version);
}
CXhChar500 CImageFileHost::get_PathFileName()
{
	CXhChar500 name;
	if(m_pInternalImgFile)
		m_pInternalImgFile->GetPathFileName(name,501);
	return name;
}
CXhChar100 CImageFileHost::get_AliasName()
{
	if(m_sAliasName.Length<=0)
		return m_sFileName;
	else
		return m_sAliasName;
}
CXhChar100 CImageFileHost::set_AliasName(const char* _szAliasName)
{
	m_sAliasName=_szAliasName;
	return m_sAliasName;
}
CXhChar100 CImageFileHost::set_FileName(const char* _szFileName)
{
	m_sFileName=_szFileName;
	return m_sFileName;
}
int CImageFileHost::get_PageNo()
{
	return m_pdfConfig.page_no;
}
int CImageFileHost::set_PageNo(int page_no)
{
	m_pdfConfig.page_no=page_no;
	return m_pdfConfig.page_no;
}
int CImageFileHost::get_RotationCount()
{
	return m_pdfConfig.rotation;
}
int CImageFileHost::set_RotationCount(int rotation_count)
{
	m_pdfConfig.rotation=rotation_count;
	return m_pdfConfig.rotation;
}
CImageRegionHost* CImageFileHost::AppendRegion()
{
	CImageRegionHost* pRegion=NULL;
	IImageRegion* pInterRegion=AddImageRegion();
	if(pInterRegion)
	{
		pRegion=m_hashRegionById.Add(0);
		pRegion->SetImageRegion(pInterRegion);
		pRegion->SetBelongImageFile(this);
	}
	return pRegion;
}
void CImageFileHost::DeleteRegion(DWORD dwRegionKey)
{
	CImageRegionHost* pRegHost=m_hashRegionById.GetValue(dwRegionKey);
	if(pRegHost)
	{
		DeleteImageRegion(pRegHost->GetSerial());
		m_hashRegionById.DeleteNode(pRegHost->GetKey());
	}
}
void CImageFileHost::SummarizePartInfo()
{
	CImageRegionHost* pRegionHost=NULL;
	IRecoginizer::BOMPART* pRegPart=NULL,*pSumPart=NULL;
	hashSumParts.Empty();
	for(pRegionHost=m_hashRegionById.GetFirst();pRegionHost;pRegionHost=m_hashRegionById.GetNext())
	{
		pRegionHost->SyncInputPartToSummaryParts();
		for(pRegPart=pRegionHost->EnumFirstPart();pRegPart;pRegPart=pRegionHost->EnumNextPart())
		{
			CXhChar100 keylabel=pRegPart->sLabel;
			if(keylabel.Length<=0)
				keylabel.Copy("???");
			pSumPart=FromPartNo(keylabel);
			if(pSumPart==NULL)
				pSumPart=AddBomPart(keylabel);
			strcpy(pSumPart->sLabel,pRegPart->sLabel);
			strcpy(pSumPart->materialStr,pRegPart->materialStr);
			strcpy(pSumPart->sSizeStr,pRegPart->sSizeStr);
			pSumPart->count=pRegPart->count;
			pSumPart->weight=pRegPart->weight;
			pSumPart->sumWeight=pRegPart->sumWeight;
			pSumPart->calWeight=pRegPart->calWeight;
			pSumPart->calSumWeight=pRegPart->calSumWeight;
			CalAndSyncTheoryWeightToWeight(pSumPart,FALSE);
			pSumPart->length=pRegPart->length;
			pSumPart->wPartType=pRegPart->wPartType;
			pSumPart->width=pRegPart->width;
			pSumPart->thick=pRegPart->thick;
			pSumPart->ciSrcFromMode=pRegPart->ciSrcFromMode;
			memcpy(pSumPart->arrItemWarningLevel,pRegPart->arrItemWarningLevel, IRecoginizer::BOMPART::WARNING_ARR_LEN);
		}
	}
}
//接口函数实现
long CImageFileHost::GetSerial()
{
	return m_pInternalImgFile->GetSerial();
}
void CImageFileHost::GetPathFileName(char* imagefilename,int maxStrBufLen/*=17*/)
{
	m_pInternalImgFile->GetPathFileName(imagefilename,maxStrBufLen);
}
void CImageFileHost::SetImageFile(IImageFile* pImageFile)
{
	CXhChar200 szPathFileName;
	char extname[9]={0};
	pImageFile->GetPathFileName(szPathFileName,szPathFileName.LengthMax);
	_splitpath(szPathFileName,NULL,NULL,this->m_sFileName,extname);
	m_sFileName.Append(extname);
	m_pInternalImgFile=pImageFile;
}
bool CImageFileHost::InitImageFileHeader(const char* imagefilename)
{
	if(imagefilename!=NULL)
	{
		CXhChar100 szPathFileName=imagefilename;
		_splitpath(szPathFileName,NULL,NULL,this->m_sFileName,NULL);
	}
	return m_pInternalImgFile->InitImageFileHeader(imagefilename);
}
bool CImageFileHost::InitImageFile(const char* imagefilename,const char* szIBomFilePath,bool update_file_path/*=true*/,PDF_FILE_CONFIG *pPDFConfig/*=NULL*/)
{
	if(imagefilename!=NULL)
	{
		CXhChar100 szPathFileName=imagefilename;
		_splitpath(szPathFileName,NULL,NULL,this->m_sFileName,NULL);
	}
	CXhChar200 tempDir;
	IMindRobot::GetWorkDirectory(tempDir,201);
	if(pPDFConfig==NULL)
		pPDFConfig=&m_pdfConfig;
	if(pPDFConfig->zoom_scale==0)
		pPDFConfig->zoom_scale=CConfig::m_fInitPDFZoomScale;
	if(pPDFConfig->m_ciPdfLineMode==PDF_FILE_CONFIG::LINE_MODE_NONE)
	{
		pPDFConfig->m_ciPdfLineMode=(BYTE)CConfig::m_iPdfLineMode;
		pPDFConfig->min_line_weight=CConfig::m_fMinPdfLineWeight;
		pPDFConfig->min_line_flatness=CConfig::m_fMinPdfLineFlatness;
		pPDFConfig->max_line_weight=CConfig::m_fMaxPdfLineWeight;
		pPDFConfig->max_line_flatness=CConfig::m_fMaxPdfLineFlatness;
	}
	bool bRetCode=m_pInternalImgFile->InitImageFile(imagefilename,NULL,true,pPDFConfig);
	if(!bRetCode)
	{	
		CBuffer modelbuf;
		if(!CDataCmpModel::LoadIBomFileToBuffer(szIBomFilePath,modelbuf))
			return FALSE;
		if(InitTempFileBuffer(modelbuf,tempDir))
		{
			CHAR_ARRAY imageArr(m_tempFileBuffer.GetLength());
			m_tempFileBuffer.SeekToBegin();
			m_tempFileBuffer.Read(imageArr,imageArr.Size());
			if(m_biImageSaveType==0)
				m_pInternalImgFile->SetCompressImageData(imageArr,imageArr.Size());
			else
				m_pInternalImgFile->SetImageRawFileData(imageArr,imageArr.Size(),m_biImageSaveType,pPDFConfig);
		}
	}
	return bRetCode;
}
BYTE CImageFileHost::ImageCompressType()
{
	return m_pInternalImgFile->ImageCompressType();
}
bool CImageFileHost::SetImageRawFileData(char* imagedata,UINT uiMaxDataSize,BYTE biFileType,PDF_FILE_CONFIG *pPDFConfig/*=NULL*/)
{
	return m_pInternalImgFile->SetImageRawFileData(imagedata,uiMaxDataSize,biFileType,pPDFConfig);
}
bool CImageFileHost::SetCompressImageData(char* imagedata,UINT uiMaxDataSize)
{
	return m_pInternalImgFile->SetCompressImageData(imagedata,uiMaxDataSize);
}
UINT CImageFileHost::GetCompressImageData(char* imagedata,UINT uiMaxDataSize/*=0*/)
{
	return m_pInternalImgFile->GetCompressImageData(imagedata,uiMaxDataSize);
}
double CImageFileHost::SetMonoThresholdBalanceCoef(double balance/*=0*/,bool bUpdateMonoImages/*=false*/)
{
	return m_pInternalImgFile->SetMonoThresholdBalanceCoef(balance,bUpdateMonoImages);
}
double CImageFileHost::GetMonoThresholdBalanceCoef()
{
	return m_pInternalImgFile->GetMonoThresholdBalanceCoef();
}
int CImageFileHost::GetHeight()
{
	return m_pInternalImgFile->GetHeight();
}
int CImageFileHost::GetWidth()
{
	return m_pInternalImgFile->GetWidth();
}
BYTE CImageFileHost::GetRawFileType()
{
	return m_pInternalImgFile->GetRawFileType();
}
//背景是否为低噪点 wjh-2019.11.28
bool CImageFileHost::SetLowBackgroundNoise(bool blValue){
	return m_pInternalImgFile->SetLowBackgroundNoise(blValue);
}
bool CImageFileHost::IsLowBackgroundNoise()const{
	return m_pInternalImgFile->IsLowBackgroundNoise();
}
//是否为细笔划字体
bool CImageFileHost::SetThinFontText(bool blValue){
	return m_pInternalImgFile->SetThinFontText(blValue);
}
bool CImageFileHost::IsThinFontText()const{
	return m_pInternalImgFile->IsThinFontText();
}
bool CImageFileHost::IsSrcFromPdfFile()
{
	return m_pInternalImgFile->IsSrcFromPdfFile();
}
int CImageFileHost::Get24BitsImageEffWidth()
{
	return m_pInternalImgFile->Get24BitsImageEffWidth();
}
int CImageFileHost::Get24BitsImageData(IMAGE_DATA* imagedata)
{
	return m_pInternalImgFile->Get24BitsImageData(imagedata);
}
int CImageFileHost::GetMonoImageData(IMAGE_DATA* imagedata)
{
	return m_pInternalImgFile->GetMonoImageData(imagedata);
}
//将文件中图像顺时针转90度，旋转不影响已提取区域，应在提取区域之前旋转
bool CImageFileHost::TurnClockwise90()
{
	bool bRetCode=m_pInternalImgFile->TurnClockwise90();
	if(bRetCode)
	{
		m_pdfConfig.rotation+=90;
		m_pdfConfig.rotation=m_pdfConfig.rotation%360;
	}
	return bRetCode;
}
//将文件中图像逆时针转90度，旋转不影响已提取区域，应在提取区域之前旋转
bool CImageFileHost::TurnAntiClockwise90()
{
	bool bRetCode=m_pInternalImgFile->TurnAntiClockwise90();
	if(bRetCode)
	{
		m_pdfConfig.rotation-=90;
		m_pdfConfig.rotation=m_pdfConfig.rotation%360;
	}
	return bRetCode;
}
bool CImageFileHost::IsBlackPixel(int i,int j)
{
	return m_pInternalImgFile->IsBlackPixel(i,j);
}
RECT CImageFileHost::GetImageDisplayRect(DWORD dwRegionKey/*=0*/)
{
	return m_pInternalImgFile->GetImageDisplayRect(dwRegionKey);
}
bool CImageFileHost::DeleteImageRegion(long idRegionSerial)
{
	return m_pInternalImgFile->DeleteImageRegion(idRegionSerial);
}
bool CImageFileHost::IntelliRecogCornerPoint(const POINT& pick,POINT* pCornerPoint,
	char ciType_LT0_RT1_RB2_LB3/*=0*/,int *pyjHoriTipOffset/*=NULL*/,UINT uiTestWidth/*=300*/,UINT uiTestHeight/*=200*/)
{
	return m_pInternalImgFile->IntelliRecogCornerPoint(pick,pCornerPoint,ciType_LT0_RT1_RB2_LB3,pyjHoriTipOffset,uiTestWidth,uiTestHeight);
}
IImageRegion* CImageFileHost::AddImageRegion()
{
	return m_pInternalImgFile->AddImageRegion();
}
IImageRegion* CImageFileHost::AddImageRegion(POINT topLeft,POINT btmLeft,POINT btmRight,POINT topRight)
{
	return m_pInternalImgFile->AddImageRegion(topLeft,btmLeft,btmRight,topRight);
}
IImageRegion* CImageFileHost::EnumFirstRegion()
{
	return m_pInternalImgFile->EnumFirstRegion();
}
IImageRegion* CImageFileHost::EnumNextRegion()
{
	return m_pInternalImgFile->EnumNextRegion();
}