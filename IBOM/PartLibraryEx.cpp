#include "StdAfx.h"
#include "PartLibraryEx.h"
#include "Recog.h"
#include "Tool.h"
#include "XhCharString.h"
#include "database.h"
#include "Config.h"

WORD aided_part_N=28;
AIDED_PART aided_part_table[100]=
{
	"GJ-25",	0.228,		1,		//钢绞线
	"GJ-35",	0.318,		1,
	"GJ-50",	0.424,		1,
	"GJ-70",	0.615,		1,
	"GJ-95",	0.795,		1,
	"GJ-100",	0.8594,		1,
	"GJ-120",	0.995,		1,
	"NUT-1",	2.1,		0,		//UT型线夹
	"NUT-2",	3.2,		0,
	"NUT-3",	5.4,		0,
	"NX-1",		1.5,		0,		//楔型线夹
	"NX-2",		2.4,		0,
	"NX-2.1",	2.4,		0,
	"NX-3",		4.5,		0,
	"U-4",		0.44,		0,		//U型挂环
	"U-10",		0.54,		0,
	"U-12",		0.95,		0,
	"U-25",		2.79,		0,
	"U-30",		3.75,		0,
	"JK-1",		0.2,		0,		//拉线卡子
	"JK-2",		0.32,		0,		
	"T2105",	10.64,		0,		//爬梯
	"T2605",	12.86,		0,
	"T3605",	17.82,		0,
	"T3980",	19.64,		0,
	"T4480",	22.70,		0,
	"T5855",	28.64,		0,
	"T6355",	30.86,		0,
};

double CalPartWeight(IRecoginizer::BOMPART *pPart)
{
	if(pPart==NULL)
		return 0;
	if((pPart->wPartType==0||pPart->width==0||pPart->thick==0)&&!ParseSpec(pPart))
		return 0;
	double calWeight=0;
	if(pPart->wPartType==IRecoginizer::BOMPART::ANGLE)
	{	//角钢
		JG_PARA_TYPE *pType=NULL;
		for(int i=0;i<jgguige_N;i++)
		{
			if( fabs(jgguige_table[i].wing_wide-pPart->width)<EPS&&
				fabs(jgguige_table[i].wing_thick-pPart->thick)<EPS)
			{
				pType=&jgguige_table[i];
				break;
			}
		}
		if(pType)
		{
			calWeight=pType->theroy_weight*pPart->length*0.001;
			return calWeight;
		}
		else
			return 0;
	}
	else if(pPart->wPartType==IRecoginizer::BOMPART::PLATE)
	{
		if(pPart->siSubType==0)
			calWeight=pPart->length*pPart->width*pPart->thick*7.85e-6;
		else
		{
			FL_PARAM *pFlParam=NULL;
			INSERT_PLATE_PARAM *pInsertPlateParam=NULL;
			if(pPart->siSubType==IRecoginizer::BOMPART::SUB_TYPE_PLATE_C)
				pInsertPlateParam=FindRollEndParam(pPart->sLabel);
			else if(pPart->siSubType==IRecoginizer::BOMPART::SUB_TYPE_PLATE_X)
				pInsertPlateParam=FindXEndParam(pPart->sLabel);
			else if(pPart->siSubType==IRecoginizer::BOMPART::SUB_TYPE_PLATE_U)
				pInsertPlateParam=FindUEndParam(pPart->sLabel);
			else if(pPart->siSubType==IRecoginizer::BOMPART::SUB_TYPE_PLATE_FLD)
				pFlParam=FindFlDParam(pPart->sLabel);
			else if(pPart->siSubType==IRecoginizer::BOMPART::SUB_TYPE_PLATE_FLP)
				pFlParam=FindFlPParam(pPart->sLabel);
			if(pInsertPlateParam)
				calWeight=pInsertPlateParam->theroy_weight;
			else if(pFlParam)
				calWeight=pFlParam->Weight();
		}
		return calWeight;
	}
	else if(pPart->wPartType==IRecoginizer::BOMPART::TUBE)
	{
		TUBE_PARA_TYPE *pType=NULL;
		for(int i=0;i<tubeguige_N;i++)
		{
			if( fabs(tubeguige_table[i].D-pPart->width)<EPS&&
				fabs(tubeguige_table[i].thick-pPart->thick)<EPS)
			{
				pType=&tubeguige_table[i];
				break;
			}
		}
		if(pType)
		{
			calWeight=pType->theroy_weight*pPart->length*0.001;
			return calWeight;
		}
		else
		{
			double area=Pi*(pPart->width*pPart->thick-pPart->thick*pPart->thick);
			//double R=0.5*pPart->width,r=(pPart->width-2*pPart->thick)*0.5;
			//double area1=Pi*(R*R-r*r);
			double theroy_weight=area*7.85e-3;
			calWeight=theroy_weight*pPart->length*0.001;
			return calWeight;
		}
	}
	else if(pPart->wPartType==IRecoginizer::BOMPART::SLOT)
	{
		CXhChar16 sSize(pPart->sSizeStr);
		sSize.Remove('[');
		sSize.Remove('C');
		SLOT_PARA_TYPE *pSlotType=FindSlotType(sSize);
		if(pSlotType)
			calWeight=pSlotType->theroy_weight*pPart->length*0.001;
		return calWeight;
	}
	else //if(pPart->wPartType==IRecoginizer::BOMPART::ROUND||
		 //	  pPart->wPartType==IRecoginizer::BOMPART::ACCESSORY||
		 //	  pPart->wPartType==0)
	{
		AIDED_PART *pAidedPart=CPartLibrary::FindAidedPart(pPart->sLabel);
		if(pAidedPart)
		{
			if(pAidedPart->siUnitType==1)
				calWeight=pAidedPart->weight*pPart->length*0.001;
			else
				calWeight=pAidedPart->weight;
			return calWeight;
		}
		else
			return 0;
	}
}

void CalAndSyncTheoryWeightToWeight(IRecoginizer::BOMPART *pPart,BOOL bForceCalWeight)
{
	if(pPart==NULL)
		return;
	double weight=CalPartWeight(pPart);
	if(pPart->calWeight<=0||bForceCalWeight)
		pPart->calWeight=weight;
	//不识别重量时自动计算重量 wht 18-12-22
	if(!CConfig::m_bRecogPieceWeight&&!(pPart->ciSrcFromMode&IRecoginizer::BOMPART::MODIFY_WEIGHT))
		pPart->weight=pPart->calWeight;
	if(pPart->ciSrcFromMode&IRecoginizer::BOMPART::MODIFY_WEIGHT)
		pPart->calSumWeight=atof(CXhChar50("%.2f",pPart->weight))*pPart->count;
	else
		pPart->calSumWeight=atof(CXhChar50("%.2f",pPart->calWeight))*pPart->count;
	if(!CConfig::m_bRecogSumWeight)
		pPart->sumWeight=pPart->calSumWeight;
}

CString MakeAngleSetString()
{
	CString sizeStr;
	char text[101]="";
	for(int i=0;i<jgguige_N;i++)
	{
		sprintf(text,"L%gx%g",jgguige_table[i].wing_wide,jgguige_table[i].wing_thick);
		sizeStr+=text;
		if(i<jgguige_N-1)
			sizeStr+='|';
	}
	return sizeStr;
}

CHashStrList<JG_PARA_TYPE*> CPartLibrary::m_hashJgParaPtrBySpec;
CHashStrList<TUBE_PARA_TYPE*> CPartLibrary::m_hashTubeParaPtrBySpec;

void CPartLibrary::InitJgParaTbl()
{
	m_hashJgParaPtrBySpec.Empty();
	for(int i=0;i<jgguige_N;i++)
	{
		CXhChar50 sSpec("%dX%d",(int)jgguige_table[i].wing_wide,(int)jgguige_table[i].wing_thick);
		m_hashJgParaPtrBySpec.SetValue(sSpec,&jgguige_table[i]);
	}
}
void CPartLibrary::InitTubeParaTbl()
{
	m_hashTubeParaPtrBySpec.Empty();
	for(int i=0;i<tubeguige_N;i++)
	{
		CXhChar50 sSpec("%dX%d",(int)tubeguige_table[i].D,(int)tubeguige_table[i].thick);
		m_hashTubeParaPtrBySpec.SetValue(sSpec,&tubeguige_table[i]);
	}
}

JG_PARA_TYPE *CPartLibrary::FindJgParaBySpec(const char* spec)
{
	if(m_hashJgParaPtrBySpec.GetNodeNum()!=jgguige_N)
		InitJgParaTbl();
	int cls_id=0;
	double width=0,thick=0;
	CXhChar16 sSpec(spec);
	ParseSpec(sSpec,&thick,&width,&cls_id);
	sSpec.Printf("%sX%s",(char*)CXhChar16(width),(char*)CXhChar16(thick));
	JG_PARA_TYPE **ppPara=m_hashJgParaPtrBySpec.GetValue(sSpec);
	if(ppPara==NULL)
		return NULL;
	else
		return *ppPara;
}

TUBE_PARA_TYPE *CPartLibrary::FindTubeParaBySpec(const char* spec)
{
	if(m_hashTubeParaPtrBySpec.GetNodeNum()!=tubeguige_N)
		InitTubeParaTbl();
	int cls_id=0;
	double width=0,thick=0;
	CXhChar16 sSpec(spec);
	ParseSpec(sSpec,&thick,&width,&cls_id);
	sSpec.Printf("%dX%d",(char*)CXhChar16(width),(char*)CXhChar16(thick));
	TUBE_PARA_TYPE **ppPara=m_hashTubeParaPtrBySpec.GetValue(sSpec);
	if(ppPara==NULL)
		return NULL;
	else
		return *ppPara;
}
JG_PARA_TYPE *CPartLibrary::FindJgParaByUnitWeight(double unit_weight)
{
	JG_PARA_TYPE *pType=NULL;
	for(int i=0;i<jgguige_N;i++)
	{
		if(fabs(jgguige_table[i].theroy_weight-unit_weight)<EPS2)
		{
			pType=&jgguige_table[i];
			break;
		}
	}
	return pType;
}
TUBE_PARA_TYPE *CPartLibrary::FindTubeParaByUnitWeight(double unit_weight)
{
	TUBE_PARA_TYPE *pType=NULL;
	for(int i=0;i<tubeguige_N;i++)
	{
		if(fabs(tubeguige_table[i].theroy_weight-unit_weight)<EPS)
		{
			pType=&tubeguige_table[i];
			break;
		}
	}
	return pType;
}
AIDED_PART* CPartLibrary::FindAidedPart(const char* spec)
{
	AIDED_PART *pAidedPart=NULL;
	for(int i=0;i<aided_part_N;i++)
	{
		if(stricmp(aided_part_table[i].guige,spec)==0)
		{
			pAidedPart=&aided_part_table[i];
			break;
		}
	}
	return pAidedPart;
}

void CPartLibrary::LoadPartLibrary(const char* app_path)
{
	CString sLibPath;
	sLibPath.Format("%s\\Lib\\角钢.jgt",APP_PATH);
	JgGuiGeSerialize(sLibPath,FALSE,FALSE);
	sLibPath.Format("%s\\Lib\\钢管.pgt",APP_PATH);
	TubeGuiGeSerialize(sLibPath,FALSE,FALSE);
	sLibPath.Format("%s\\Lib\\槽钢.cgt",APP_PATH);
	SlotGuiGeSerialize(sLibPath,FALSE,FALSE);
	sLibPath.Format("%s\\Lib\\C型插板.cipl",APP_PATH);
	CInsertPlateSerialize(sLibPath,FALSE,FALSE);
	sLibPath.Format("%s\\Lib\\U型插板.uipl",APP_PATH);
	UInsertPlateSerialize(sLibPath,FALSE,FALSE);
	sLibPath.Format("%s\\Lib\\十字插板.xipl",APP_PATH);
	XInsertPlateSerialize(sLibPath,FALSE,FALSE);
	sLibPath.Format("%s\\Lib\\平焊法兰.flp",APP_PATH);
	FlPSerialize(sLibPath,FALSE,FALSE);
	sLibPath.Format("%s\\Lib\\对焊法兰.fld",APP_PATH);
	FlDSerialize(sLibPath,FALSE,FALSE);
	sLibPath.Format("%s\\Lib\\辅助构件.fzl",APP_PATH);
	AidedPartGuiGeSerialize(sLibPath,FALSE,FALSE);
}