#include "StdAfx.h"
#include "ExcelOper.h"
#include "SortFunc.h"
#include "ComparePartNoString.h"
#include "DataModel.h"
#include "ArrayList.h"
#include "Tool.h"
#include "IBOM.h"
#include "BOM.h"
#include "XhLicAgent.h"
#include "f_alg_fun.h"
#include "CryptBuffer.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

CDataCmpModel dataModel;
const int DWG_EXCEL_COL_COUNT  =17;
//图纸/放样BOM的Excel标题
const char* T_BOMPART_LABEL				= "编号";
const char* T_BOMPART_MAT				= "材质";
const char* T_BOMPART_LEN				= "长度";
const char* T_BOMPART_WIDE				= "宽度";
const char* T_BOMPART_SPEC				= "规格";
const char* T_BOMPART_NUM				= "件数";
//const char*	T_BOMPART_SUM				= "总件数";
const char* T_BOMPART_PER_WEIGHT		= "单重";
const char* T_BOMPART_PARTTYPE			= "类型";

static CXhChar100 VariantToString(VARIANT value)
{
	CXhChar100 sValue;
	if(value.vt==VT_BSTR)
		return sValue.Copy(CString(value.bstrVal));
	else if(value.vt==VT_R8)
		return sValue.Copy(CXhChar100(value.dblVal));
	else if(value.vt==VT_R4)
		return sValue.Copy(CXhChar100(value.fltVal));
	else if(value.vt==VT_INT)
		return sValue.Copy(CXhChar100(value.intVal));
	else 
		return sValue;
}
//////////////////////////////////////////////////////////////////////////
CBomFileData::CBomFileData()
{
}
CBomFileData::~CBomFileData()
{

}
void CBomFileData::FromBuffer(CBuffer& buffer,long version/*=0*/)
{
	buffer.ReadString(m_sName);
	int nPartNum=0;
	buffer.ReadInteger(&nPartNum);
	CXhChar16 sPartNo;
	IRecoginizer::BOMPART* pBomPart=NULL;
	for(int i=0;i<nPartNum;i++)
	{
		buffer.ReadString(sPartNo);
		pBomPart=AddBomPart(sPartNo);
		strcpy(pBomPart->sLabel,sPartNo);
		buffer.ReadInteger(&pBomPart->id);
		buffer.ReadWord(&pBomPart->wPartType);
		buffer.ReadString(pBomPart->sSizeStr);
		buffer.ReadString(pBomPart->materialStr);
		buffer.ReadWord(&pBomPart->count);
		buffer.ReadDouble(&pBomPart->length);
		buffer.ReadDouble(&pBomPart->weight);
		if(version==0||version>=1000800)
			buffer.ReadDouble(&pBomPart->sumWeight);
	}
}
void CBomFileData::ToBuffer(CBuffer& buffer,long version/*=0*/)
{
	buffer.WriteString(m_sName);
	int nPartNum=hashParts.GetNodeNum();
	buffer.WriteInteger(nPartNum);
	for(IRecoginizer::BOMPART* pBomPart=EnumFirstPart();pBomPart;pBomPart=EnumNextPart())
	{
		buffer.WriteString(pBomPart->sLabel);
		buffer.WriteInteger(pBomPart->id);
		buffer.WriteWord(pBomPart->wPartType);
		buffer.WriteString(pBomPart->sSizeStr);
		buffer.WriteString(pBomPart->materialStr);
		buffer.WriteWord(pBomPart->count);
		buffer.WriteDouble(pBomPart->length);
		buffer.WriteDouble(pBomPart->weight);
		if(version==0||version>=1000800)
			buffer.WriteDouble(pBomPart->sumWeight);
	}
}

BOOL CBomFileData::ImportExcelBomFileByDllFromat(CVariant2dArray &sheetContextMap)
{
	if(CIBOMApp::GetBomExcelFormat==NULL)
		return FALSE;
	int colIndexArr[50]={0},iStartRow=0;
	int nCount=CIBOMApp::GetBomExcelFormat(colIndexArr,&iStartRow);
	if(nCount<4)
		return FALSE;
	//colIndexArr[0]=3;	//件号
	//colIndexArr[1]=5;	//规格
	//colIndexArr[2]=6;	//材质
	//colIndexArr[3]=8;	//数量
	//colIndexArr[4]=9;	//单重
	//colIndexArr[5]=7;	//长度
	//colIndexArr[6]=6;	//宽度
	//colIndexArr[7]=4;	//类型
	CXhChar100 titleArr[8]={T_BOMPART_LABEL,
							T_BOMPART_SPEC,
							T_BOMPART_MAT,
							T_BOMPART_NUM,
							T_BOMPART_PER_WEIGHT,
							T_BOMPART_LEN,
							T_BOMPART_WIDE,
							T_BOMPART_PARTTYPE};
	CHashStrList<DWORD> hashColIndexByColTitle;
	for(int i=0;i<min(8,nCount);i++)
	{
		if(colIndexArr[i]<=0)
			continue;
		hashColIndexByColTitle.SetValue(titleArr[i],colIndexArr[i]-1);
	}
	return ImportExcelBomFileCore(sheetContextMap,hashColIndexByColTitle,iStartRow-1);
}
static int GetClsIdByPartType(const char* part_type)
{
	if(part_type==NULL)
		return 0;
	CXhChar16 sPartType(part_type);
	if( sPartType.EqualNoCase("角钢")||
		sPartType.EqualNoCase("L")||
		sPartType.EqualNoCase("∠"))
		return BOMPART::ANGLE;
	else if(sPartType.EqualNoCase("钢管")||
			sPartType.EqualNoCase("φ")||
			sPartType.EqualNoCase("Φ"))
		return BOMPART::TUBE;
	else if(sPartType.EqualNoCase("钢板")||
			sPartType.EqualNoCase("板")||
			sPartType.EqualNoCase("板材")||
			sPartType.EqualNoCase("-")||
			sPartType.EqualNoCase("_")||
			sPartType.EqualNoCase("—"))
		return BOMPART::PLATE;
	else if(sPartType.EqualNoCase("槽钢")||
			sPartType.EqualNoCase("["))
		return BOMPART::SLOT;
	else
		return 0;
}
static  CXhChar16 GetPartTypeBriefMark(const char* part_type)
{
	CXhChar16 sPartType;
	int cls_id=GetClsIdByPartType(part_type);
	if(cls_id==BOMPART::ANGLE)
		sPartType.Copy("L");
	else if(cls_id==BOMPART::PLATE)
		sPartType.Copy("-");
	else if(cls_id==BOMPART::TUBE)
		sPartType.Copy("φ");
	else if(cls_id==BOMPART::SLOT)
		sPartType.Copy("[");
	return sPartType;
}
BOOL CBomFileData::ImportExcelBomFileCore(CVariant2dArray &sheetContentMap,CHashStrList<DWORD> &hashColIndexByColTitle,int startRowIndex)
{
	DWORD *pColIndex =NULL;
	for(int i=startRowIndex;i<=sheetContentMap.RowsCount();i++)
	{ 
		VARIANT value;
		CXhChar16 sPartType,sPartTypeBriefMark;
		CXhChar100 sPartNo,sMaterial,sSpec;
		//编号
		pColIndex=hashColIndexByColTitle.GetValue(T_BOMPART_LABEL);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		if(value.vt==VT_EMPTY)
			continue;
		sPartNo=VariantToString(value);
		if(sPartNo.Length<=0)
			continue;
		//类型
		if((pColIndex=hashColIndexByColTitle.GetValue(T_BOMPART_PARTTYPE))!=NULL)
		{
			sheetContentMap.GetValueAt(i,*pColIndex,value);
			sPartType = VariantToString(value);
			sPartTypeBriefMark=GetPartTypeBriefMark(sPartType);
		}
		//规格
		int cls_id=0,sub_type=0;
		double thick=0,width=0;
		if((pColIndex=hashColIndexByColTitle.GetValue(T_BOMPART_SPEC))!=NULL)
		{
			sheetContentMap.GetValueAt(i,*pColIndex,value);
			sSpec = VariantToString(value);
			BOOL bRetCode=FALSE;
			if(sMaterial.GetLength()<=0)
				bRetCode=ParseSpec(sSpec,&thick,&width,&cls_id,&sub_type,false,sSpec,sMaterial);
			else
				bRetCode=ParseSpec(sSpec,&thick,&width,&cls_id,&sub_type,false,sSpec,NULL);
			if(!bRetCode)
			{
				sSpec.InsertBefore(sPartTypeBriefMark,0);
				if(sMaterial.GetLength()<=0)
					bRetCode=ParseSpec(sSpec,&thick,&width,&cls_id,&sub_type,false,sSpec,sMaterial);
				else
					bRetCode=ParseSpec(sSpec,&thick,&width,&cls_id,&sub_type,false,sSpec,NULL);
			}
			if(sMaterial.Length==0)
			{
				if(strstr(sSpec,"Q345")!=NULL)
					sMaterial="Q345";
				else if(strstr(sSpec,"Q390")!=NULL)
					sMaterial="Q390";
				else if(strstr(sSpec,"Q420")!=NULL)
					sMaterial="Q420";
				else if(strstr(sSpec,"Q460")!=NULL)
					sMaterial="Q460";
				else
					sMaterial="Q235";
			}
		}
		//材质
		if((pColIndex=hashColIndexByColTitle.GetValue(T_BOMPART_MAT))!=NULL)
		{
			sheetContentMap.GetValueAt(i,*pColIndex,value);
			sMaterial = VariantToString(value);
			sMaterial.Remove('B');
			sMaterial.Remove(' ');
			if(sMaterial.GetLength()<=0)
				sMaterial.Copy("Q235");
		}
		//长度
		float fLength=0;
		if((pColIndex=hashColIndexByColTitle.GetValue(T_BOMPART_LEN))!=NULL)
		{
			sheetContentMap.GetValueAt(i,*pColIndex,value);
			CXhChar100 sLen=VariantToString(value);
			if(strstr(sLen,"×")||strstr(sLen,"x")||strstr(sLen,"*")||strstr(sLen,"X"))
			{
				sLen.Replace("×"," ");
				sLen.Replace("X"," ");
				sLen.Replace("x"," ");
				sLen.Replace("*"," ");
				sLen.Replace("?"," ");	//调试状态下识别错误时系统默认待处理字符
				sscanf(sLen,"%lf%lf",&width,&fLength);
			}
			else
				fLength=(float)atof(sLen);
		}
		if((pColIndex=hashColIndexByColTitle.GetValue(T_BOMPART_WIDE))!=NULL)
		{
			sheetContentMap.GetValueAt(i,*pColIndex,value);
			int fWidth=atoi(VariantToString(value));
			if(fWidth>0)
				width=fWidth;
		}
		//单基数
		int nNumPerTower=0;
		if((pColIndex=hashColIndexByColTitle.GetValue(T_BOMPART_NUM))!=NULL)
		{
			sheetContentMap.GetValueAt(i,*pColIndex,value);
			nNumPerTower=atoi(VariantToString(value));
		}
		//加工数
		/*int nProcessNum=0;
		if((pColIndex=hashColIndexByColTitle.GetValue(T_BOMPART_SUM))!=NULL)
		{
			sheetContentMap.GetValueAt(i,*pColIndex,value);
			nProcessNum=atoi(VariantToString(value));
		}*/
		//单基重量
		double fWeight=0;
		if((pColIndex=hashColIndexByColTitle.GetValue(T_BOMPART_PER_WEIGHT))!=NULL)
		{
			sheetContentMap.GetValueAt(i,*pColIndex,value);
			fWeight=atof(VariantToString(value));
		}
		if(cls_id<=0&&sPartType.GetLength()>0)
			cls_id=GetClsIdByPartType(sPartType);
		//初始化哈希表
		IRecoginizer::BOMPART* pBomPart=AddBomPart(sPartNo);
		pBomPart->wPartType=cls_id;
		pBomPart->siSubType=sub_type;
		pBomPart->width=width;
		pBomPart->thick=thick;
		strcpy(pBomPart->sLabel,sPartNo);
		strcpy(pBomPart->materialStr,sMaterial);
		if(cls_id==IRecoginizer::BOMPART::ANGLE)
			sprintf(pBomPart->sSizeStr,"L%sX%s",(char*)CXhChar16(width),(char*)CXhChar16(thick));
		else if(cls_id==IRecoginizer::BOMPART::PLATE)
		{
			if(width>0)
				sprintf(pBomPart->sSizeStr,"-%sX%s",(char*)CXhChar16(thick),(char*)CXhChar16(width));
			else
				sprintf(pBomPart->sSizeStr,"-%s",(char*)CXhChar16(thick));
		}
		else
			strncpy(pBomPart->sSizeStr,sSpec,16);
		pBomPart->count=nNumPerTower;
		pBomPart->length=fLength;
		pBomPart->weight=fWeight;
		CalAndSyncTheoryWeightToWeight(pBomPart,FALSE);
	}
	return TRUE;
}

BOOL CBomFileData::ImportExcelBomFile(const char* sFileName)
{
	//1.获取Excel内容存储至sheetContentMap中,建立列标题与列索引映射表hashColIndexByColTitle
	CVariant2dArray sheetContentMap(1,1);
	if(!CExcelOper::GetExcelContentOfSpecifySheet(sFileName,sheetContentMap,1))
		return FALSE;
	if(sheetContentMap.ColsCount()<4)//LOFT_EXCEL_COL_COUNT)
		return FALSE;
	if(ImportExcelBomFileByDllFromat(sheetContentMap))
		return TRUE;
	//
	CHashStrList<DWORD> hashColIndexByColTitle;
	int EXCEL_COL_COUNT=DWG_EXCEL_COL_COUNT;
	BYTE cbCellTitleFlag=0;
	for(int i=0;i<EXCEL_COL_COUNT;i++)
	{
		VARIANT value;
		sheetContentMap.GetValueAt(0,i,value);
		if(value.vt==VT_EMPTY||value.vt==VT_NULL)
			break;
		CString itemstr(value.bstrVal);
		if(strstr(itemstr,"编号")!=0||strstr(itemstr,"件号")!=0) //T_BOMPART_LABEL="编号"
		{
			hashColIndexByColTitle.SetValue(T_BOMPART_LABEL,i);
			cbCellTitleFlag|=0x01;
		}
		else if(itemstr.CompareNoCase(T_BOMPART_MAT)==0)
		{
			hashColIndexByColTitle.SetValue(T_BOMPART_MAT,i);
			cbCellTitleFlag|=0x02;
		}
		else if(itemstr.CompareNoCase(T_BOMPART_LEN)==0)
		{
			hashColIndexByColTitle.SetValue(T_BOMPART_LEN,i);
			cbCellTitleFlag|=0x04;
		}
		else if(itemstr.CompareNoCase(T_BOMPART_SPEC)==0)
		{
			hashColIndexByColTitle.SetValue(T_BOMPART_SPEC,i);
			cbCellTitleFlag|=0x08;
		}
		else if(strstr(itemstr,T_BOMPART_NUM)!=0)	//T_BOMPART_NUM="件数"
		{
			hashColIndexByColTitle.SetValue(T_BOMPART_NUM,i);
			cbCellTitleFlag|=0x10;
		}
		else if(itemstr.CompareNoCase(T_BOMPART_PER_WEIGHT)==0)
		{
			hashColIndexByColTitle.SetValue(T_BOMPART_PER_WEIGHT,i);
			cbCellTitleFlag|=0x20;
		}
		/*else  if (itemstr.CompareNoCase(T_BOMPART_SUM)==0)
		{
			hashColIndexByColTitle.SetValue(T_BOMPART_SUM,i);
			cbCellTitleFlag|=0x40;
		}*/
		if(cbCellTitleFlag==0x3f)
			break;
	}
	if (hashColIndexByColTitle.GetValue(T_BOMPART_LABEL)==NULL||
		//hashColIndexByColTitle.GetValue(T_BOMPART_MAT)==NULL||
		hashColIndexByColTitle.GetValue(T_BOMPART_LEN)==NULL||
		hashColIndexByColTitle.GetValue(T_BOMPART_SPEC)==NULL||
		//hashColIndexByColTitle.GetValue(T_BOMPART_PER_WEIGHT)==NULL||
		hashColIndexByColTitle.GetValue(T_BOMPART_NUM)==NULL)//
		//hashColIndexByColTitle.GetValue(T_BOMPART_SUM)==NULL)
	{
		logerr.Log("缺少关键列(件号、长度、规格或单基数等)");
		return FALSE;
	}
	//2.获取Excel所有单元格的值
	ImportExcelBomFileCore(sheetContentMap,hashColIndexByColTitle,1);
	return TRUE;
}
static char* LocalQuerySteelMaterialStr(char cBriefMaterial)
{
	if (cBriefMaterial == 'S')
		return "Q235";
	else if (cBriefMaterial == 'H')
		return "Q345";
	else if (cBriefMaterial == 'G')
		return "Q390";
	else if (cBriefMaterial == 'P' || cBriefMaterial == 'E')
		return "Q420";
	else if (cBriefMaterial == 'T')
		return "Q460";
	else
		return "";
}
BOOL CBomFileData::ImportDwgTxtFile(const char* sFileName)
{
	FILE *fp=fopen(sFileName,"rt");
	if(fp==NULL)
	{
		AfxMessageBox("打不开指定的TXT文件!");
		return FALSE;
	}
	const char *PLACE_HOLDER="NULL";
	CXhChar200 line_txt;
	SEGI validSegI;
	while(!feof(fp))
	{
		if(fgets(line_txt,200,fp)==NULL)
			break;
		line_txt.Replace("%%c","φ");
		line_txt.Replace("%%C","φ");
		line_txt[strlen(line_txt)-1]='\0';
		char szTokens[] = " \n\t" ;

		SEGI segI;
		CXhChar50 sPartLabel,sSpec;
		//件号
		char* sKey=strtok(line_txt,szTokens);
		if(sKey!=NULL&&stricmp(PLACE_HOLDER,sKey)!=0)
			sPartLabel.Copy(sKey);
		//材质+规格
		sKey=strtok(NULL,szTokens);
		if(sKey!=NULL&&stricmp(PLACE_HOLDER,sKey)!=0)
			sSpec.Copy(sKey);
		if(sPartLabel.GetLength()==0)
		{	//使用规格作为件号
			sPartLabel.Copy(sSpec);
			sSpec.Empty();
			if(validSegI.iSeg>0)
				segI=validSegI;
		}
		else
		{
			if(!ParsePartNo(sPartLabel,&segI,NULL))
				segI.iSeg=0;
			else if(validSegI.iSeg==0)
				validSegI=segI;
		}
		if(sPartLabel.GetLength()<=0)
			continue;
		IRecoginizer::BOMPART* pBomPart=AddBomPart(sPartLabel);
		strcpy(pBomPart->sLabel,sPartLabel);
		pBomPart->iSeg=segI.iSeg;
		//材质+规格
		if(sSpec.GetLength()>0)
		{	
			CXhChar50 sSize(sSpec);
			sSize.Remove(' ');
			//材质
			char cFirstChar=sSize.At(0),cTailChar=sSize.At(sSize.GetLength()-1);
			strcpy(pBomPart->materialStr,"Q235");
			if(strstr(sKey,"Q"))
			{
				strncpy(pBomPart->materialStr,sKey,4);
				sSize.Replace(pBomPart->materialStr,"");
			}
			else if(CSteelMatLibrary::Q235BriefMark()==cFirstChar||CSteelMatLibrary::Q235BriefMark()==cTailChar)
				strcpy(pBomPart->materialStr,"Q235");
			else if(CSteelMatLibrary::Q345BriefMark()==cFirstChar||CSteelMatLibrary::Q345BriefMark()==cTailChar)
				strcpy(pBomPart->materialStr,"Q345");
			else if(CSteelMatLibrary::Q390BriefMark()==cFirstChar||CSteelMatLibrary::Q390BriefMark()==cTailChar)
				strcpy(pBomPart->materialStr,"Q390");
			else if(CSteelMatLibrary::Q420BriefMark()==cFirstChar||CSteelMatLibrary::Q420BriefMark()==cTailChar)
				strcpy(pBomPart->materialStr,"Q420");
			else if(CSteelMatLibrary::Q460BriefMark()==cFirstChar||CSteelMatLibrary::Q460BriefMark()==cTailChar)
				strcpy(pBomPart->materialStr,"Q460");
			else
			{
				CXhChar16 sMat(LocalQuerySteelMaterialStr(cFirstChar));
				if (sMat.GetLength() <= 0)
					sMat.Copy(LocalQuerySteelMaterialStr(cTailChar));
				if (sMat.GetLength() > 0)
					strcpy(pBomPart->materialStr, sMat);
			}
			//规格
			/*int width=0,thick=0,cls_id=0;
			ParseSpec(sSize,thick,width,cls_id);
			pBomPart->wPartType=cls_id;
			if(cls_id==1)
				sprintf(pBomPart->sSizeStr,"L%dX%d",width,thick);
			else if(cls_id==2)
				sprintf(pBomPart->sSizeStr,"-%d",thick);*/
			strncpy(pBomPart->sSizeStr,sSize,16);
			//因为CAD提取时槽钢符号用三条直线绘制导致丢失[,此处补齐 wht 18-12-25
			if(stricmp(pBomPart->sSizeStr,"5")==0)
				strcpy(pBomPart->sSizeStr,"[5");
			ParseSpec(pBomPart);
		}
		//长度
		sKey=strtok(NULL,szTokens);
		if(sKey!=NULL&&strlen(sKey)>0&&stricmp(PLACE_HOLDER,sKey)!=0)
			pBomPart->length=atof(sKey);
		//单基数
		sKey=strtok(NULL,szTokens);
		if(sKey!=NULL&&strlen(sKey)>0&&stricmp(PLACE_HOLDER,sKey)!=0)
			pBomPart->count=atoi(sKey);
		//单重
		sKey=strtok(NULL,szTokens);
		if(sKey!=NULL&&strlen(sKey)>0&&stricmp(PLACE_HOLDER,sKey)!=0)
		{
			pBomPart->weight=atof(sKey);
			pBomPart->calWeight=pBomPart->weight;
		}
		//总重
		sKey=strtok(NULL,szTokens);
		if(sKey!=NULL&&strlen(sKey)>0&&stricmp(PLACE_HOLDER,sKey)!=0)
		{
			pBomPart->sumWeight=atof(sKey);
			pBomPart->calSumWeight=pBomPart->sumWeight;
		}
	}
	fclose(fp);
	return TRUE;
}
//////////////////////////////////////////////////////////////////////////
//
PROGRESS_CONTEXT::PROGRESS_CONTEXT(){
	title=NULL;
	ciStartProgressNumber=0;
	ciToProgressNumber=100;
}
int PROGRESS_CONTEXT::F2I(double fval)
{
	if(fval>=0)
		return (int)(fval+0.5);
	else
		return (int)(fval-0.5);
}
int PROGRESS_CONTEXT::Mapping(double percent)//将0~100的百分比映射转换至ciStartProgressNumber~ciToProgressNumber区间
{
	if(ciStartProgressNumber==0&&ciToProgressNumber==100)
		return F2I(percent+0.5);
	else if(ciToProgressNumber-ciStartProgressNumber<=0)
		return F2I(percent);	//区间起止点不合理，不做转换
	int step=F2I((ciToProgressNumber-ciStartProgressNumber)*percent/100.0);
	return ciStartProgressNumber+step;
}
//////////////////////////////////////////////////////////////////////////
CDataCmpModel::CDataCmpModel()
{
	m_uiOriginalDogKey=DogSerialNo();
	DisplayProcess=NULL;
	m_sOperator=CConfig::m_sOperator;
	m_sAuditor=CConfig::m_sAuditor;
	m_sCritic=CConfig::m_sCritic;
}
CDataCmpModel::~CDataCmpModel()
{
	Empty();
}
void CDataCmpModel::InitFileProp()
{
	if(m_sOperator.Length<=0)
		m_sOperator=CConfig::m_sOperator;
	if(m_sAuditor.Length<=0)
		m_sAuditor=CConfig::m_sAuditor;
	if(m_sCritic.Length<=0)
		m_sCritic=CConfig::m_sCritic;
}
void CDataCmpModel::Empty()
{
	m_segHashList.Empty();
	m_xDwgBomList.Empty();
	m_xDwgImageList.Empty();
	m_xLoftBomList.Empty();
	m_sTowerTypeName.Empty();
	m_sTowerAlias.Empty();
	m_sPrjCode.Empty();
	m_sPrjName.Empty();
	m_sTaStampNo.Empty();
}
BOOL CDataCmpModel::LoadIBomFileToBuffer(const char* sIBOMFilePath,CBuffer &modelBuffer)
{
	if(sIBOMFilePath==NULL)
		return FALSE;
	FILE* fp=fopen(sIBOMFilePath,"rb");
	if(fp==NULL)
		return FALSE;
	fseek(fp,0,SEEK_END);
	UINT uiBlobSize=ftell(fp);
	CHAR_ARRAY pool(uiBlobSize);
	fseek(fp,0,SEEK_SET);
	fread((char*)pool,1,uiBlobSize,fp);
	fclose(fp);
	CBuffer fileBuffer(pool,pool.Size());
	uiBlobSize=fileBuffer.ReadInteger();
	char ciEncryptMode=0;
	int cursor_pipeline_no=0;
	if(uiBlobSize==0)
	{
		CXhChar100 sDocType,sFileVersion;
		fileBuffer.ReadString(sDocType,101);
		fileBuffer.ReadString(sFileVersion,101);
		fileBuffer.ReadInteger();
		cursor_pipeline_no=fileBuffer.ReadInteger();
		uiBlobSize=fileBuffer.ReadInteger();
		if(compareVersion(sFileVersion,"1.0.7.1")>=0)
			ciEncryptMode=2;
		else
			ciEncryptMode=true;
	}
	modelBuffer.ClearContents();
	modelBuffer.SetBlockSize(uiBlobSize);
	modelBuffer.Write(NULL,uiBlobSize);
	fileBuffer.Read(modelBuffer.GetBufferPtr(),uiBlobSize);
	if(ciEncryptMode>0)
		DecryptBuffer(modelBuffer,ciEncryptMode,cursor_pipeline_no);
	modelBuffer.SeekToBegin();
	return TRUE;
}
BOOL CDataCmpModel::ParseEmbedImageFileToTempFile(const char* file_path)
{
	CBuffer buffer;
	if(!LoadIBomFileToBuffer(file_path,buffer))
		return FALSE;
	CXhChar200 sTempDir;
	IMindRobot::GetWorkDirectory(sTempDir,201);
	for(CImageFileHost* pImageHost=EnumFirstImage();pImageHost;pImageHost=EnumNextImage())
		pImageHost->InitTempFileBuffer(buffer,sTempDir);
	return TRUE;
}
BOOL CDataCmpModel::DeleteTempImageFile()
{
	for(CImageFileHost* pImageHost=EnumFirstImage();pImageHost;pImageHost=EnumNextImage())
		pImageHost->DeleteTempFile();
	return TRUE;
}

struct FILE_POS{
	CXhPtrSet<CImageFileHost> fileList;
	DWORD m_dwFileAddress;
	DWORD m_dwFileSize;
	BYTE m_biSaveType;
	FILE_POS(){m_dwFileAddress=m_dwFileSize=0;m_biSaveType=0;}
};
void CDataCmpModel::FromBuffer(CBuffer& buffer,long version/*=0*/)
{
	Empty();
	long buffer_len=buffer.GetLength();
	double process_scale=100.0/buffer_len;
	if(DisplayProcess!=NULL)
		DisplayProcess(progress.Mapping(buffer.GetCursorPosition()*process_scale),progress.title);
	if(version==NULL||version>=1000700)
	{	//存储文件基本信息
		buffer.ReadString(m_sCompanyName);	//设计单位
		buffer.ReadString(m_sPrjCode);		//工程编号
		buffer.ReadString(m_sPrjName);		//工程名称
		buffer.ReadString(m_sTowerTypeName);//塔型名称
		buffer.ReadString(m_sTowerAlias);	//塔型代号
		buffer.ReadString(m_sTaStampNo);	//钢印号
		buffer.ReadString(m_sOperator);		//操作员（制表人）
		buffer.ReadString(m_sAuditor);		//审核人
		buffer.ReadString(m_sCritic);		//评审人
	}
	int nNum=0;
	//图纸图片信息
	buffer.ReadInteger(&nNum);
	for(int i=0;i<nNum;i++)
	{
		int key=0;
		buffer.ReadInteger(&key);
		SEGITEM* pSegItem=m_segHashList.Add(key);
		pSegItem->iSeg=key;
		buffer.ReadString(pSegItem->sSegName);
		if(DisplayProcess!=NULL)
			DisplayProcess(progress.Mapping(buffer.GetCursorPosition()*process_scale),progress.title);
	}
	buffer.ReadInteger(&nNum);
	SEGI iSeg;
	CXhChar500 szPathFileName;
	double fPDFZoomScale=0;
	BYTE ciPDFLineMode=0xFF;
	double fPDFMinLineWeight=0,fPDFMinLineFlatness=0,fPDFMaxLineWeight=0,fPDFMaxLineFlatness=0;
	int iPageNo=0,niRotDegAngle=0;
	for(int i=0;i<nNum;i++)
	{
		buffer.ReadInteger(&iSeg.iSeg);
		buffer.ReadString(szPathFileName);
		if(version==NULL||version>=10004)
		{
			buffer.ReadInteger(&iPageNo);
			buffer.ReadInteger(&niRotDegAngle);
		}
		if(version==NULL||version>=10005)
			buffer.ReadDouble(&fPDFZoomScale);
		if(version==NULL||version>=1000700)
		{
			buffer.ReadByte(&ciPDFLineMode);
			buffer.ReadDouble(&fPDFMinLineWeight);
			buffer.ReadDouble(&fPDFMinLineFlatness);
			buffer.ReadDouble(&fPDFMaxLineWeight);
			buffer.ReadDouble(&fPDFMaxLineFlatness);
		}
		CImageFileHost* pImageHost=AppendImageFile(szPathFileName,iSeg,false,iPageNo,niRotDegAngle,fPDFZoomScale);
		pImageHost->ciPDFLineMode=ciPDFLineMode;
		pImageHost->fPDFZoomScale=fPDFZoomScale;
		pImageHost->fPDFMinLineWeight=fPDFMinLineWeight;
		pImageHost->fPDFMinLineFlatness=fPDFMinLineFlatness;
		pImageHost->fPDFMaxLineWeight=fPDFMaxLineWeight;
		pImageHost->fPDFMaxLineFlatness=fPDFMaxLineFlatness;
		pImageHost->FromBuffer(buffer,version);
		//if(pImageHost->Height==0)
		//	pImageHost->InitImageFile(NULL);
		pImageHost->UpdateImageRegions();
		if(DisplayProcess!=NULL)
			DisplayProcess(progress.Mapping(buffer.GetCursorPosition()*process_scale),progress.title);
	}
	//按文件路径分组，重新设置文件对应图像文件对应的地址 wht 18-06-21
	CHashStrList<FILE_POS> hashFilePosByFilePath;
	for(CImageFileHost* pImageHost=EnumFirstImage();pImageHost;pImageHost=EnumNextImage())
	{
		FILE_POS *pFileSet=hashFilePosByFilePath.GetValue(pImageHost->szPathFileName);
		if(pFileSet==NULL)
			pFileSet=hashFilePosByFilePath.Add(pImageHost->szPathFileName);
		pFileSet->fileList.append(pImageHost);
		if(pImageHost->dwFileAddressPos>0&&pImageHost->dwFileSize>0)
		{
			pFileSet->m_biSaveType=pImageHost->biImageSaveType;
			pFileSet->m_dwFileAddress=pImageHost->dwFileAddressPos;
			pFileSet->m_dwFileSize=pImageHost->dwFileSize;
		}
	}
	for(CImageFileHost* pImageHost=EnumFirstImage();pImageHost;pImageHost=EnumNextImage())
	{
		if(pImageHost->dwFileAddressPos>0)
			continue;
		FILE_POS *pFileSet=hashFilePosByFilePath.GetValue(pImageHost->szPathFileName);
		if(pFileSet==NULL||pFileSet->m_dwFileAddress<=0)
			continue;
		pImageHost->InitFileAddressAndSize(pFileSet->m_dwFileAddress,pFileSet->m_dwFileSize,pFileSet->m_biSaveType);
	}
	//图纸BOM信息
	buffer.ReadInteger(&nNum);
	for(int i=0;i<nNum;i++)
	{
		CBomFileData* pDwgBom=m_xDwgBomList.append();
		pDwgBom->FromBuffer(buffer);
		if(DisplayProcess!=NULL)
			DisplayProcess(progress.Mapping(buffer.GetCursorPosition()*process_scale),progress.title);
	}
	//放样BOM信息
	buffer.ReadInteger(&nNum);
	for(int i=0;i<nNum;i++)
	{
		CBomFileData* pLoftBom=m_xLoftBomList.append();
		pLoftBom->FromBuffer(buffer);
		if(DisplayProcess!=NULL)
			DisplayProcess(progress.Mapping(buffer.GetCursorPosition()*process_scale),progress.title);
	}
	if(DisplayProcess!=NULL)
		DisplayProcess(100,NULL);
}
void CDataCmpModel::ToBuffer(CBuffer& buffer,long version/*=0*/,bool bAutoBakSave/*=false*/)
{
	int nNum=0;
	if(version==NULL||version>=1000700)
	{	//存储文件基本信息
		buffer.WriteString(m_sCompanyName);	//设计单位
		buffer.WriteString(m_sPrjCode);		//工程编号
		buffer.WriteString(m_sPrjName);		//工程名称
		buffer.WriteString(m_sTowerTypeName);//塔型名称
		buffer.WriteString(m_sTowerAlias);	//塔型代号
		buffer.WriteString(m_sTaStampNo);	//钢印号
		buffer.WriteString(m_sOperator);	//操作员（制表人）
		buffer.WriteString(m_sAuditor);		//审核人
		buffer.WriteString(m_sCritic);		//评审人
	}
	//图纸图片信息
	nNum=m_segHashList.GetNodeNum();
	buffer.WriteInteger(nNum);
	for(SEGITEM* pSegItem=m_segHashList.GetFirst();pSegItem;pSegItem=m_segHashList.GetNext())
	{
		buffer.WriteInteger(pSegItem->iSeg.iSeg);
		buffer.WriteString(pSegItem->sSegName);
	}
	nNum=m_xDwgImageList.GetNodeNum();
	buffer.WriteInteger(nNum);
	CHashStrList<FILE_POS> hashFilePosByFilePath;
	for(CImageFileHost* pImageHost=EnumFirstImage();pImageHost;pImageHost=EnumNextImage())
	{
		FILE_POS *pFileSet=hashFilePosByFilePath.GetValue(pImageHost->szPathFileName);
		if(pFileSet==NULL)
			pFileSet=hashFilePosByFilePath.Add(pImageHost->szPathFileName);
		pFileSet->fileList.append(pImageHost);
	}
	for(CImageFileHost* pImageHost=EnumFirstImage();pImageHost;pImageHost=EnumNextImage())
	{
		buffer.WriteInteger(pImageHost->m_iSeg.iSeg);
		buffer.WriteString(pImageHost->szPathFileName);
		if(version==NULL||version>=10004)
		{
			buffer.WriteInteger(pImageHost->iPageNo);
			buffer.WriteInteger(pImageHost->niRotDegAngle);
		}
		if(version==NULL||version>=10005)
			buffer.WriteDouble(pImageHost->fPDFZoomScale);
		if(version==NULL||version>=1000700)
		{
			buffer.WriteByte(pImageHost->ciPDFLineMode);
			buffer.WriteDouble(pImageHost->fPDFMinLineWeight);
			buffer.WriteDouble(pImageHost->fPDFMinLineFlatness);
			buffer.WriteDouble(pImageHost->fPDFMaxLineWeight);
			buffer.WriteDouble(pImageHost->fPDFMaxLineFlatness);
		}
		FILE_POS *pFilePos=hashFilePosByFilePath.GetValue(pImageHost->szPathFileName);
		if(pFilePos)
		{
			pImageHost->ToBuffer(buffer,version,pFilePos->m_dwFileAddress,pFilePos->m_dwFileSize,pFilePos->m_biSaveType,bAutoBakSave);
			if(pFilePos->m_dwFileAddress==0)
			{
				pFilePos->m_dwFileAddress=pImageHost->dwFileAddressPos;
				pFilePos->m_dwFileSize=pImageHost->dwFileSize;
				pFilePos->m_biSaveType=pImageHost->biImageSaveType;
			}
		}
		else
			pImageHost->ToBuffer(buffer,version,0,0,0,bAutoBakSave);
		
	}
	//图纸BOM信息
	nNum=m_xDwgBomList.GetNodeNum();
	buffer.WriteInteger(nNum);
	for(CBomFileData* pDwgBom=EnumFirstDwgBom();pDwgBom;pDwgBom=EnumNextDwgBom())
		pDwgBom->ToBuffer(buffer);
	//放样BOM信息
	nNum=m_xLoftBomList.GetNodeNum();
	buffer.WriteInteger(nNum);
	for(CBomFileData* pLoftBom=EnumFirstLoftBom();pLoftBom;pLoftBom=EnumNextLoftBom())
		pLoftBom->ToBuffer(buffer);
}
void CDataCmpModel::ComplementSegItemHashList()
{
	for(CImageFileHost* pImageHost=EnumFirstImage();pImageHost;pImageHost=EnumNextImage())
	{
		if (m_segHashList.GetValue(pImageHost->m_iSeg)!=NULL)
			continue;	//只补充之前未手动设定名称的分段
		SEGITEM *pSegItem=m_segHashList.Add(pImageHost->m_iSeg);
		pSegItem->iSeg=pImageHost->m_iSeg;
		pSegItem->sSegName=pImageHost->m_iSeg.ToString();
	}
}
//
void CDataCmpModel::DeleteSegItem(DWORD key)
{
	//删除归属段号的图片文件
	for(CImageFileHost* pImageHost=EnumFirstImage();pImageHost;pImageHost=EnumNextImage())
	{
		if(pImageHost->m_iSeg.iSeg==key)
		{
			for(CImageRegionHost* pRegion=pImageHost->EnumFirstRegionHost();pRegion;pRegion=pImageHost->EnumNextRegionHost())
				pImageHost->DeleteRegion(pRegion->GetKey());
			//
			//IMindRobot::DestroyImageFile(pImageHost->GetSerial());
			m_xDwgImageList.DeleteCursor();
		}
	}
	m_xDwgImageList.Clean();
	//删除段号
	for(SEGITEM* pSeg=EnumFirstSeg();pSeg;pSeg=EnumNextSeg())
	{
		if(pSeg->iSeg==key)
			m_segHashList.DeleteCursor();
	}
	m_segHashList.Clean();
}
SEGITEM* CDataCmpModel::NewSegI(long iSegI/*=0*/)
{
	SEGITEM* pSegItem=m_segHashList.Add(SEGI(iSegI));
	pSegItem->iSeg=m_segHashList.GetCursorKey();
	if(iSegI==0)
		pSegItem->sSegName="***";
	else
		pSegItem->sSegName=pSegItem->iSeg.ToString();
	return pSegItem;
}
//
void CDataCmpModel::DeleteImageFile(const char* sFileName,SEGI iSeg)
{
	for(CImageFileHost* pImageHost=EnumFirstImage();pImageHost;pImageHost=EnumNextImage())
	{
		if(stricmp(pImageHost->szPathFileName,sFileName)==0 && pImageHost->m_iSeg==iSeg)
		{
			for(CImageRegionHost* pRegion=pImageHost->EnumFirstRegionHost();pRegion;pRegion=pImageHost->EnumNextRegionHost())
				pImageHost->DeleteRegion(pRegion->GetKey());
			//
			//IMindRobot::DestroyImageFile(pImageHost->GetSerial());
			m_xDwgImageList.DeleteCursor();
		}
	}
	m_xDwgImageList.Clean();
}
CImageFileHost* CDataCmpModel::GetImageFile(const char* sFileName,int page_no/*=1*/)
{
	CImageFileHost* pImageFile=NULL;
	for(pImageFile=EnumFirstImage();pImageFile;pImageFile=EnumNextImage())
	{
		if(stricmp(pImageFile->szPathFileName,sFileName)==0&&pImageFile->iPageNo==page_no)
			break;
	}
	return pImageFile;
}
CImageFileHost* CDataCmpModel::AppendImageFile(const char* szPathFileName,SEGI iSeg,bool loadFileInstant/*=true*/,
											   int iPageNo/*==1*/,int niRotDegAngle/*=0*/,double fZoomScale/*=0*/)
{
	if(strlen(szPathFileName)<=0)
		return NULL;
	CImageFileHost* pImageFile=NULL;
	IImageFile* pInterImageFile=IMindRobot::AddImageFile(szPathFileName,loadFileInstant);
	if(pInterImageFile)
	{
		pImageFile=m_xDwgImageList.append();
		pImageFile->m_iSeg=iSeg;
		pImageFile->iPageNo=iPageNo;
		pImageFile->niRotDegAngle=niRotDegAngle;
		pImageFile->fPDFZoomScale=fZoomScale;
		pImageFile->SetImageFile(pInterImageFile);
		//打开JPG等图像文件时需要根据设置的旋转次数调整图片 wht 19-11-30
		if (niRotDegAngle!=0)
			pImageFile->SetTurnCount(niRotDegAngle / 90);
		//pImageFile->szPathFileName=szPathFileName;
	}
	return pImageFile;
}
CImageFileHost* CDataCmpModel::EnumFirstImage(SEGI iSeg/*=-1*/)
{
	CImageFileHost* pImageHost=m_xDwgImageList.GetFirst();
	while(pImageHost)
	{
		if(iSeg.iSeg==-1 || pImageHost->m_iSeg==iSeg)
			break;
		pImageHost=m_xDwgImageList.GetNext();
	}
	return pImageHost;
}
CImageFileHost* CDataCmpModel::EnumNextImage(SEGI iSeg/*=-1*/)
{
	CImageFileHost* pImageHost=m_xDwgImageList.GetNext();
	while(pImageHost)
	{
		if(iSeg==-1 || pImageHost->m_iSeg.iSeg==iSeg.iSeg)
			break;
		pImageHost=m_xDwgImageList.GetNext();
	}
	return pImageHost;
}
//
void CDataCmpModel::DeleteDwgBomFile(const char* sFileName)
{
	for(CBomFileData* pBomFile=EnumFirstDwgBom();pBomFile;pBomFile=EnumNextDwgBom())
	{
		if(stricmp(pBomFile->m_sName,sFileName)==0)
			m_xDwgBomList.DeleteCursor();
	}
	m_xDwgBomList.Clean();
}
CBomFileData* CDataCmpModel::GetDwgBomFile(const char* sFileName)
{
	CBomFileData *pDwgBom=NULL;
	for(pDwgBom=EnumFirstDwgBom();pDwgBom;pDwgBom=EnumNextDwgBom())
	{
		if(stricmp(pDwgBom->m_sName,sFileName)==0)
			break;
	}
	return pDwgBom;
}
CBomFileData* CDataCmpModel::AppendDwgBomFile(const char* sFileName)
{
	if(strlen(sFileName)<=0)
		return NULL;
	char ext[MAX_PATH]="";
	_splitpath(sFileName,NULL,NULL,NULL,ext);
	//
	BOOL bImport=FALSE;
	CBomFileData *pDwgBom=NULL;
	if(stricmp(ext,".bomd")==0)
	{
		pDwgBom=m_xDwgBomList.append();
		pDwgBom->m_sName=sFileName;
		bImport=pDwgBom->ImportDwgTxtFile(sFileName);
	}
	else if(stricmp(ext,".xls")==0||stricmp(ext,".xlsx")==0)
	{
		pDwgBom=m_xDwgBomList.append();
		pDwgBom->m_sName=sFileName;
		bImport=pDwgBom->ImportExcelBomFile(sFileName);
	}
	if(bImport==FALSE)
	{
		m_xDwgBomList.GetTail();
		m_xDwgBomList.DeleteCursor(TRUE);
		pDwgBom=NULL;
	}
	return pDwgBom;
}
//
void CDataCmpModel::DeleteLoftBomFile(const char* sFileName)
{
	for(CBomFileData *pLoftBom=EnumFirstLoftBom();pLoftBom;pLoftBom=EnumNextLoftBom())
	{
		if(stricmp(pLoftBom->m_sName,sFileName)==0)
			m_xLoftBomList.DeleteCursor();
	}
	m_xLoftBomList.Clean();
}
CBomFileData* CDataCmpModel::GetLoftBomFile(const char* sFileName)
{
	CBomFileData *pLoftBom=NULL;
	for(pLoftBom=EnumFirstLoftBom();pLoftBom;pLoftBom=EnumNextLoftBom())
	{
		if(stricmp(pLoftBom->m_sName,sFileName)==0)
			break;
	}
	return pLoftBom;
}
CBomFileData* CDataCmpModel::AppendLoftBomFile(const char* sFileName)
{
	if(strlen(sFileName)<=0)
		return NULL;
	CBomFileData *pLoftBom=m_xLoftBomList.append();
	if(pLoftBom->ImportExcelBomFile(sFileName))
		pLoftBom->m_sName.Copy(sFileName);
	else
	{
		m_xLoftBomList.GetTail();
		m_xLoftBomList.DeleteCursor(TRUE);
		pLoftBom=NULL;
	}
	return pLoftBom;
}
//
void CDataCmpModel::SummarizeDwgParts(CHashStrList<IRecoginizer::BOMPART*>& dwgPartSet)
{
	IRecoginizer::BOMPART* pPart=NULL;
	for(CImageFileHost *pImageHost=EnumFirstImage();pImageHost;pImageHost=EnumNextImage())
	{
		pImageHost->SummarizePartInfo();
		for(pPart=pImageHost->EnumFirstPart();pPart;pPart=pImageHost->EnumNextPart())
		{
			if(dwgPartSet.GetValue(pPart->sLabel)==NULL)
				dwgPartSet.SetValue(pPart->sLabel,pPart);
		}
	}
	for(CBomFileData* pDwgBom=EnumFirstDwgBom();pDwgBom;pDwgBom=EnumNextDwgBom())
	{
		for(pPart=pDwgBom->EnumFirstPart();pPart;pPart=pDwgBom->EnumNextPart())
		{
			if(dwgPartSet.GetValue(pPart->sLabel)==NULL)
				dwgPartSet.SetValue(pPart->sLabel,pPart);
		}
	}
}
void CDataCmpModel::SummarizeLoftParts(CHashStrList<IRecoginizer::BOMPART*>& loftPartSet)
{
	IRecoginizer::BOMPART* pPart=NULL;
	for(CBomFileData *pLoftBom=EnumFirstLoftBom();pLoftBom;pLoftBom=EnumNextLoftBom())
	{
		for(pPart=pLoftBom->EnumFirstPart();pPart;pPart=pLoftBom->EnumNextPart())
		{
			if(loftPartSet.GetValue(pPart->sLabel)==NULL)
				loftPartSet.SetValue(pPart->sLabel,pPart);
		}
	}
}
static BOOL IsEqualBomPart(IRecoginizer::BOMPART *pSrcPart,IRecoginizer::BOMPART *pDestPart,
						   CHashStrList<BOOL> &hashBoolByPropName,double LEN_TOLERANCE,double WEIGTH_TOLERANCE)
{
	hashBoolByPropName.Empty();
	ParseSpec(pSrcPart);
	ParseSpec(pDestPart);
	CXhChar16 sSrcMat(pSrcPart->materialStr),sDestMat(pDestPart->materialStr);
	if(sSrcMat.GetLength()<=0)
		sSrcMat.Copy("Q235");
	if(sDestMat.GetLength()<=0)
		sDestMat.Copy("Q235");
	if( pSrcPart->wPartType==pDestPart->wPartType&&
		(pSrcPart->wPartType==IRecoginizer::BOMPART::ANGLE||
		 pSrcPart->wPartType==IRecoginizer::BOMPART::TUBE||
		 pSrcPart->wPartType==IRecoginizer::BOMPART::FLAT||
		 pSrcPart->wPartType==IRecoginizer::BOMPART::SLOT||
		 pSrcPart->wPartType==IRecoginizer::BOMPART::PLATE||
		 pSrcPart->wPartType==IRecoginizer::BOMPART::ACCESSORY||
		 pSrcPart->wPartType==IRecoginizer::BOMPART::ROUND))
	{
		double width1=pSrcPart->width,width2=pDestPart->width;
		double thick1=pSrcPart->thick,thick2=pDestPart->thick;
		double len1=pSrcPart->length,len2=pDestPart->length;
		if(pSrcPart->wPartType==IRecoginizer::BOMPART::PLATE)
		{
			if(width1>len1)
			{
				double w=width1;
				width1=len1;
				len1=w;
			}
			if(width2>len2)
			{
				double w=width2;
				width2=len2;
				len2=w;
			}
		}
		if(pSrcPart->wPartType==IRecoginizer::BOMPART::ANGLE||
			pSrcPart->wPartType==IRecoginizer::BOMPART::TUBE||
			pSrcPart->wPartType==IRecoginizer::BOMPART::FLAT||
			pSrcPart->wPartType==IRecoginizer::BOMPART::SLOT)
		{	//杆件，对比规格及长度
			if(fabs(len1-len2)>LEN_TOLERANCE)	//根据客户需求，长度比较误差在50以内
				hashBoolByPropName.SetValue("length",TRUE);
			if(fabs(width1-width2)>EPS2||fabs(thick1-thick2)>EPS2)
				hashBoolByPropName.SetValue("spec",TRUE);		//规格不同
			if(stricmp(sSrcMat,sDestMat)!=0)
				hashBoolByPropName.SetValue("material",TRUE);	//对比材质
		}
		else if(pSrcPart->wPartType==IRecoginizer::BOMPART::PLATE)
		{
			if( pSrcPart->siSubType!=pDestPart->siSubType||		//子类型不同
				pSrcPart->siSubType==0&&stricmp(sSrcMat,sDestMat)!=0)
				hashBoolByPropName.SetValue("material",TRUE);
			if(pSrcPart->siSubType==0&&fabs(thick1-thick2)>EPS2)
				hashBoolByPropName.SetValue("spec",TRUE);		//规格不同
			if(pSrcPart->siSubType!=0&&stricmp(pSrcPart->sSizeStr,pDestPart->sSizeStr)!=0)
				hashBoolByPropName.SetValue("spec",TRUE);		//规格不同
		}
		else 
		{
			if(stricmp(pSrcPart->materialStr,pDestPart->materialStr)!=0)	//材质
				hashBoolByPropName.SetValue("material",TRUE);
			if(stricmp(pSrcPart->sSizeStr,pDestPart->sSizeStr)!=0)
				hashBoolByPropName.SetValue("spec",TRUE);					//规格不同
		}
	}
	else
	{
		if(stricmp(sSrcMat,sDestMat)!=0)	//材质
			hashBoolByPropName.SetValue("material",TRUE);
		if(stricmp(pSrcPart->sSizeStr,pDestPart->sSizeStr)!=0)	//规格
			hashBoolByPropName.SetValue("spec",TRUE);
	}
	if(pSrcPart->count!=pDestPart->count)	//单基数
		hashBoolByPropName.SetValue("count",TRUE);
	if(fabsl(pSrcPart->weight-pDestPart->weight)>WEIGTH_TOLERANCE)
		hashBoolByPropName.SetValue("weight",TRUE);
	return hashBoolByPropName.GetNodeNum()==0;
}
int CDataCmpModel::ComparePartData(double LEN_TOLERANCE,double WEIGHT_TOLERANCE)
{
	const double COMPARE_EPS=0.5;
	m_hashCompareResultByPartNo.Empty();
	//统计BOMPART
	CHashStrList<IRecoginizer::BOMPART*> loftPartSet;
	CHashStrList<IRecoginizer::BOMPART*> dwgPartSet;
	SummarizeDwgParts(dwgPartSet);
	SummarizeLoftParts(loftPartSet);
	//对比数据
	CHashStrList<BOOL> hashBoolByPropName;
	for(IRecoginizer::BOMPART **ppDwgPart=dwgPartSet.GetFirst();ppDwgPart;ppDwgPart=dwgPartSet.GetNext())
	{
		IRecoginizer::BOMPART *pDwgPart=*ppDwgPart;
		IRecoginizer::BOMPART **ppLoftPart = loftPartSet.GetValue(pDwgPart->sLabel);
		if(ppLoftPart== NULL||*ppLoftPart==NULL)
		{	//1.存在原始构件，不存在放样构件
			COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add((*ppDwgPart)->sLabel);
			pResult->pDwgPart=pDwgPart;
			pResult->pLoftPart=NULL;
		}
		else 
		{	//2. 对比同一件号构件属性
			IRecoginizer::BOMPART *pLoftPart=*ppLoftPart;
			if(!IsEqualBomPart(pDwgPart,pLoftPart,hashBoolByPropName,LEN_TOLERANCE,WEIGHT_TOLERANCE))
			{	
				COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pLoftPart->sLabel);
				pResult->pDwgPart=pDwgPart;
				pResult->pLoftPart=pLoftPart;
				for(BOOL *pValue=hashBoolByPropName.GetFirst();pValue;pValue=hashBoolByPropName.GetNext())
					pResult->hashBoolByPropName.SetValue(hashBoolByPropName.GetCursorKey(),*pValue);
			}
		}
	}
	//2.3 遍历导入的原始表,查找是否有漏件情况
	for(IRecoginizer::BOMPART **ppLoftPart=loftPartSet.GetFirst();ppLoftPart;ppLoftPart=loftPartSet.GetNext())
	{
		IRecoginizer::BOMPART *pLoftPart=*ppLoftPart;
		if(dwgPartSet.GetValue(pLoftPart->sLabel))
			continue;
		COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pLoftPart->sLabel);
		pResult->pDwgPart=NULL;
		pResult->pLoftPart=pLoftPart;
	}
	if(m_hashCompareResultByPartNo.GetNodeNum()==0)
		return 0;
	else
		return 1;
}
static int ComparePartNo(const CXhChar16& str1,const CXhChar16& str2)
{
	return ComparePartNoString(str1,str2);
}
//
void CDataCmpModel::AddBomResultSheet(LPDISPATCH pSheet,int iSheet)
{
	//对校审结果进行排序
	ARRAY_LIST<CXhChar16> keyStrArr;
	CDataCmpModel::COMPARE_PART_RESULT *pResult=NULL;
	for (pResult=m_hashCompareResultByPartNo.GetFirst();pResult;pResult=EnumNextResult())//遍历存储的结果表
	{
		if(pResult->pLoftPart)
			keyStrArr.append(pResult->pLoftPart->sLabel);
		else
			keyStrArr.append(pResult->pDwgPart->sLabel);
	}
	CQuickSort<CXhChar16>::QuickSort(keyStrArr.m_pData,keyStrArr.GetSize(),ComparePartNo);
	//添加标题行
	_Worksheet excel_sheet;
	excel_sheet.AttachDispatch(pSheet,FALSE);
	excel_sheet.Select();
	CStringArray str_arr;
	str_arr.SetSize(6);
	str_arr[0]="构件编号";str_arr[1]="设计规格";str_arr[2]="材质";
	str_arr[3]="长度";str_arr[4]="单基数";str_arr[5]="数据来源";
	double col_arr[6]={15,15,15,15,15,15};
	CExcelOper::AddRowToExcelSheet(excel_sheet,1,str_arr,col_arr,TRUE);
	//填充内容
	char cell_start[16]="A1";
	char cell_end[16]="A1";
	int nResult=GetResultCount();
	CVariant2dArray map(nResult*2,6);//获取Excel表格的范围
	int index=0;
	if(iSheet==1)
	{	//第一种结果：原始数据和放样数据不同
		excel_sheet.SetName("同号差异");
		for(int i=0;i<keyStrArr.GetSize();i++)
		{
			COMPARE_PART_RESULT *pResult=GetResult(keyStrArr[i]);
			if(pResult==NULL || pResult->pLoftPart==NULL || pResult->pDwgPart==NULL)
				continue;
			_snprintf(cell_start,15,"A%d",index+2);
			_snprintf(cell_end,15,"A%d",index+3);
			CExcelOper::MergeRowRange(excel_sheet,cell_start,cell_end);	//合并行
			map.SetValueAt(index,0,COleVariant(pResult->pDwgPart->sLabel));
			map.SetValueAt(index,1,COleVariant(pResult->pDwgPart->sSizeStr));
			map.SetValueAt(index,2,COleVariant(pResult->pDwgPart->materialStr));
			map.SetValueAt(index,3,COleVariant(CXhChar50("%.f",pResult->pDwgPart->length)));
			map.SetValueAt(index,4,COleVariant((long)pResult->pDwgPart->count));
			map.SetValueAt(index,5,COleVariant(CXhChar16("图纸")));
			//
			if(pResult->hashBoolByPropName.GetValue("spec"))
			{
				map.SetValueAt(index+1,1,COleVariant(pResult->pLoftPart->sSizeStr));
				_snprintf(cell_start,15,"B%d",index+2);
				CExcelOper::SetRangeBackColor(excel_sheet,42,cell_start);
				_snprintf(cell_start,15,"B%d",index+3);
				CExcelOper::SetRangeBackColor(excel_sheet,44,cell_start);
			}
			if(pResult->hashBoolByPropName.GetValue("material"))
			{	
				map.SetValueAt(index+1,2,COleVariant(pResult->pLoftPart->materialStr));
				_snprintf(cell_start,15,"C%d",index+2);
				CExcelOper::SetRangeBackColor(excel_sheet,42,cell_start);
				_snprintf(cell_start,15,"C%d",index+3);
				CExcelOper::SetRangeBackColor(excel_sheet,44,cell_start);
			}
			if(pResult->hashBoolByPropName.GetValue("length"))
			{	
				map.SetValueAt(index+1,3,COleVariant(CXhChar50("%.f",pResult->pLoftPart->length)));
				_snprintf(cell_start,15,"D%d",index+2);
				CExcelOper::SetRangeBackColor(excel_sheet,42,cell_start);
				_snprintf(cell_start,15,"D%d",index+3);
				CExcelOper::SetRangeBackColor(excel_sheet,44,cell_start);
			}
			if(pResult->hashBoolByPropName.GetValue("count"))
			{	
				map.SetValueAt(index+1,4,COleVariant((long)pResult->pLoftPart->count));
				_snprintf(cell_start,15,"E%d",index+2);
				CExcelOper::SetRangeBackColor(excel_sheet,42,cell_start);
				_snprintf(cell_start,15,"E%d",index+3);
				CExcelOper::SetRangeBackColor(excel_sheet,44,cell_start);
			}
			map.SetValueAt(index+1,5,COleVariant(CXhChar16("放样")));
			index+=2;
		}
	}
	else if(iSheet==2)
	{	//第二种结果：放样表里有数据，导入原始表里没有数据
		excel_sheet.SetName("放样多件");
		for(int i=0;i<keyStrArr.GetSize();i++)
		{
			COMPARE_PART_RESULT *pResult=GetResult(keyStrArr[i]);
			if(pResult==NULL || pResult->pLoftPart==NULL || pResult->pDwgPart)
				continue;
			map.SetValueAt(index,0,COleVariant(pResult->pLoftPart->sLabel));
			map.SetValueAt(index,1,COleVariant(pResult->pLoftPart->sSizeStr));
			map.SetValueAt(index,2,COleVariant(pResult->pLoftPart->materialStr));
			map.SetValueAt(index,3,COleVariant(CXhChar50("%.f",pResult->pLoftPart->length)));
			map.SetValueAt(index,4,COleVariant((long)pResult->pLoftPart->count));
			index++;
		}
	}
	else if(iSheet==3)
	{	//第三种结果：导入原始表里有数据，放样表里没有数据
		excel_sheet.SetName("放样缺件");
		for(int i=0;i<keyStrArr.GetSize();i++)
		{
			COMPARE_PART_RESULT *pResult=GetResult(keyStrArr[i]);
			if(pResult==NULL || pResult->pLoftPart || pResult->pDwgPart==NULL)
				continue;
			map.SetValueAt(index,0,COleVariant(pResult->pDwgPart->sLabel));
			map.SetValueAt(index,1,COleVariant(pResult->pDwgPart->sSizeStr));
			map.SetValueAt(index,2,COleVariant(pResult->pDwgPart->materialStr));
			map.SetValueAt(index,3,COleVariant(CXhChar50("%.f",pResult->pDwgPart->length)));
			map.SetValueAt(index,4,COleVariant((long)pResult->pDwgPart->count));
			index++;
		}
	}
	_snprintf(cell_end,15,"F%d",index+1);
	if(index>0)
		CExcelOper::SetRangeValue(excel_sheet,"A2",cell_end,map.var);
	CExcelOper::SetRangeHorizontalAlignment(excel_sheet,"A1",cell_end,COleVariant((long)3));
	CExcelOper::SetRangeBorders(excel_sheet,"A1",cell_end,COleVariant(10.5));
}
//
void CDataCmpModel::ExportCompareResult()
{
	LPDISPATCH pWorksheets=CExcelOper::CreateExcelWorksheets(3);
	ASSERT(pWorksheets!= NULL);
	Sheets excel_sheets;
	excel_sheets.AttachDispatch(pWorksheets);
	for(int i=1;i<=3;i++)
	{
		LPDISPATCH pWorksheet=excel_sheets.GetItem(COleVariant((short)i));
		AddBomResultSheet(pWorksheet,i);
	}
	excel_sheets.ReleaseDispatch();
}
static char LocalQuerySteelBriefMatMark(const char* material_str)
{
	if(material_str==NULL)
		return 0;
	else if(strstr(material_str,"Q235")!=NULL)
		return 'S';
	else if(strstr(material_str,"Q345")!=NULL)
		return 'H';
	else if(strstr(material_str,"Q390")!=NULL)
		return 'G';
	else if(strstr(material_str,"Q420")!=NULL)
		return 'P';
	else if(strstr(material_str,"Q460")!=NULL)
		return 'T';
	else
		return 0;
}

int CDataCmpModel::SummaryBomParts(CXhPtrSet<IRecoginizer::BOMPART> &dwgPartSet,BYTE image0_bom1_all2,SEGI *pSegI/*=NULL*/)
{
	dwgPartSet.Empty();
	if(image0_bom1_all2!=0)
	{
		for(CBomFileData* pDwgBom=EnumFirstDwgBom();pDwgBom;pDwgBom=EnumNextDwgBom())
		{
			for(IRecoginizer::BOMPART* pBomPart=pDwgBom->EnumFirstPart();pBomPart;pBomPart=pDwgBom->EnumNextPart())
			{
				CalAndSyncTheoryWeightToWeight(pBomPart,FALSE);
				dwgPartSet.append(pBomPart);
			}
		}
	}
	if(image0_bom1_all2!=1)
	{
		ExportDwgBomPartAlterExcel();
		SEGI segI=pSegI?*pSegI:-1;
		for(CImageFileHost *pImageFile=EnumFirstImage(segI);pImageFile;pImageFile=EnumNextImage(segI))
		{
			pImageFile->SummarizePartInfo();
			SEGI xPrevSegI,xCurrSegI=segI;
			for(IRecoginizer::BOMPART* pBomPart=pImageFile->EnumFirstPart();pBomPart;pBomPart=pImageFile->EnumNextPart())
			{
				CalAndSyncTheoryWeightToWeight(pBomPart,FALSE);
				//if(pImageFile->m_iSeg.iSeg!=-1&&pImageFile->m_iSeg.iSeg>0)
				//	pBomPart->iSeg=pImageFile->m_iSeg.iSeg;
				if(segI.iSeg==-1||segI.iSeg<=0)
				{
					pBomPart->iSeg==xPrevSegI;
					if(pBomPart->wPartType==IRecoginizer::BOMPART::TUBE||pBomPart->wPartType==IRecoginizer::BOMPART::ANGLE||
						pBomPart->wPartType==IRecoginizer::BOMPART::PLATE)
					{
						if(ParsePartNo(pBomPart->sLabel,&xCurrSegI,NULL,NULL,NULL))
						{
							xPrevSegI=pBomPart->iSeg=xCurrSegI;
							pImageFile->m_iSeg=xCurrSegI;
						}
					}
				}
				else 
					pBomPart->iSeg= xPrevSegI;
				dwgPartSet.append(pBomPart);
			}
		}
	}
	return dwgPartSet.GetNodeNum();
}
void CDataCmpModel::ToBomModelBuffer(CBuffer &buffer,CXhPtrSet<IRecoginizer::BOMPART> &dwgPartSet,DWORD dwVersion/*=NULL*/)
{
	CModelBOM bomModel;
	SEGI segI;
	for(IRecoginizer::BOMPART *pBomPart=dwgPartSet.GetFirst();pBomPart;pBomPart=dwgPartSet.GetNext())
	{
		//构件分类，1:螺栓2:角钢3:钢板4:钢管5:扁铁6:槽钢
		ParseSpec(pBomPart);
		BYTE cPartType=(BYTE)pBomPart->wPartType;
		SUPERLIST_NODE<BOMPART> *pNode=bomModel.listParts.AttachNode(cPartType);
		pNode->pDataObj->sPartNo.Copy(pBomPart->sLabel);
		pNode->pDataObj->sSpec.Copy(pBomPart->sSizeStr);
		pNode->pDataObj->cMaterial=LocalQuerySteelBriefMatMark(pBomPart->materialStr);
		pNode->pDataObj->sMaterial.Copy(pBomPart->materialStr);
		pNode->pDataObj->length=pBomPart->length;
		pNode->pDataObj->thick=pBomPart->thick;
		pNode->pDataObj->wide=pBomPart->width;
		pNode->pDataObj->cPartType=cPartType;
		pNode->pDataObj->siSubType=pBomPart->siSubType;
		pNode->pDataObj->fPieceWeight=pBomPart->weight;
		pNode->pDataObj->fSumWeight = pBomPart->calSumWeight;
		pNode->pDataObj->fMapSumWeight = pBomPart->sumWeight;
		pNode->pDataObj->AddPart(pBomPart->count);
		if(pBomPart->iSeg==-1||pBomPart->iSeg<=0)
		{
			ParsePartNo(pBomPart->sLabel,&segI,NULL);
			pBomPart->iSeg=segI;
		}
		pNode->pDataObj->iSeg=pBomPart->iSeg;
		pNode->pDataObj->fSumWeight=pBomPart->sumWeight;
	}
	//bomModel.m_sSegStr.Copy("*");
	bomModel.m_sCompanyName=dataModel.m_sCompanyName;//设计单位
	bomModel.m_sTowerTypeName=dataModel.m_sTowerTypeName;
	bomModel.m_sTaAlias=dataModel.m_sTowerAlias;	//代号
	bomModel.m_sPrjCode=dataModel.m_sPrjCode;		//工程编号
	bomModel.m_sPrjName=dataModel.m_sPrjName;		//工程名称
	bomModel.m_sTaStampNo=dataModel.m_sTaStampNo;	//钢印号
	bomModel.m_sOperator=dataModel.m_sOperator;		//操作员（制表人）
	bomModel.m_sAuditor=dataModel.m_sAuditor;		//审核人
	bomModel.m_sCritic=dataModel.m_sCritic;			//评审人
	bomModel.ToBuffer(buffer,dwVersion);
}

void CDataCmpModel::ExportDwgDataExcel(BYTE image0_bom1_all2,SEGI *pSegI/*=NULL*/)
{
	//构件统计
	CXhPtrSet<IRecoginizer::BOMPART> dwgPartSet;
	int nPartNum=0;
	if((nPartNum=SummaryBomParts(dwgPartSet,image0_bom1_all2,pSegI))<=0)
		return;
	const int BOM_SHEET_PART		= 0x01;	//构件明细表
	const int BOM_SHEET_BOLT		= 0x02;	//螺栓明细表
	const int BOM_SHEET_WELDPART	= 0x04;	//组焊件明细表
	if(CIBOMApp::CreateExcelBomFile!=NULL&&CIBOMApp::GetSupportDataBufferVersion!=NULL)
	{
		DWORD dwFlag=BOM_SHEET_PART,nCount=1;
		CWaitCursor waitCursor;
		CBuffer buffer;
		ToBomModelBuffer(buffer,dwgPartSet,CIBOMApp::GetSupportDataBufferVersion());
		CIBOMApp::CreateExcelBomFile(buffer.GetBufferPtr(),buffer.GetLength(),NULL,dwFlag);
		return;
	}
	//生成Excel表格
	LPDISPATCH pWorksheets=CExcelOper::CreateExcelWorksheets(1);
	ASSERT(pWorksheets!= NULL);
	Sheets excel_sheets;
	excel_sheets.AttachDispatch(pWorksheets);
	LPDISPATCH pWorksheet=excel_sheets.GetItem(COleVariant((long)1));
	_Worksheet excel_sheet;
	excel_sheet.AttachDispatch(pWorksheet,FALSE);
	excel_sheet.Select();
	excel_sheet.SetName("图纸构件");
	
	CStringArray str_arr;
	const int COL_COUNT=5;
	str_arr.SetSize(COL_COUNT);
	str_arr[0]="编号";str_arr[1]="材质";str_arr[2]="规格";
	str_arr[3]="长度";str_arr[4]="件数";
	double col_arr[5]={8.5,8.5,8.5,8.5,8.5};
	CExcelOper::AddRowToExcelSheet(excel_sheet,1,str_arr,col_arr,TRUE);
	int index=0;
	CVariant2dArray map(nPartNum*2,COL_COUNT);//获取Excel表格的范围
	for(IRecoginizer::BOMPART* pBomPart=dwgPartSet.GetFirst();pBomPart;pBomPart=dwgPartSet.GetNext())
	{	
		ParseSpec(pBomPart);	//构件分类，1:螺栓2:角钢3:钢板4:钢管5:扁铁6:槽钢
		map.SetValueAt(index,0,COleVariant(pBomPart->sLabel));
		CXhChar16 materialStr(pBomPart->materialStr);
		if(pBomPart->wPartType==BOMPART::PLATE&&pBomPart->siSubType!=0)
		{
			char cMaterial='0';
			if(pBomPart->siSubType==BOMPART::SUB_TYPE_PLATE_FLD||pBomPart->siSubType==BOMPART::SUB_TYPE_PLATE_FLP)
			{
				cMaterial=(strlen(pBomPart->sLabel)>=9)?pBomPart->sLabel[8]:'0';
				if(cMaterial=='D')
					cMaterial='P';
			}
			else if(pBomPart->siSubType==BOMPART::SUB_TYPE_PLATE_U||
				pBomPart->siSubType==BOMPART::SUB_TYPE_PLATE_X||
				pBomPart->siSubType==BOMPART::SUB_TYPE_PLATE_C)
				cMaterial=(strlen(pBomPart->sLabel)>=6)?pBomPart->sLabel[5]:'0';
			materialStr=cMaterial!='0'?QuerySteelMatMark(cMaterial):"";
			if (materialStr.GetLength() <= 0)
				materialStr.Copy(pBomPart->materialStr);
		}
		map.SetValueAt(index,1,COleVariant(materialStr));
		if(pBomPart->wPartType==IRecoginizer::BOMPART::SLOT)
		{
			map.SetValueAt(index,2,COleVariant(pBomPart->sSizeStr));
			if(pBomPart->length>0)
				map.SetValueAt(index,3,COleVariant(CXhChar50("%.f",pBomPart->length)));
		}
		else if(pBomPart->wPartType==IRecoginizer::BOMPART::ANGLE)
		{
			map.SetValueAt(index,2,COleVariant(CXhChar50("∠%s×%s",(char*)CXhChar16(pBomPart->width),(char*)CXhChar16(pBomPart->thick))));
			if(pBomPart->length>0)
				map.SetValueAt(index,3,COleVariant(CXhChar50("%.f",pBomPart->length)));
		}
		else if(pBomPart->wPartType==IRecoginizer::BOMPART::PLATE)
		{
			if(pBomPart->siSubType==0)
			{
				map.SetValueAt(index,2,COleVariant(CXhChar50("-%s",(char*)CXhChar16(pBomPart->thick))));
				map.SetValueAt(index,3,COleVariant(CXhChar50("%d×%.f",(int)pBomPart->width,pBomPart->length)));
			}
		}
		else if(pBomPart->wPartType==IRecoginizer::BOMPART::TUBE)
		{
			if(pBomPart->siSubType==0)
				map.SetValueAt(index,2,COleVariant(CXhChar50("Φ%s×%s",(char*)CXhChar16(pBomPart->width),(char*)CXhChar16(pBomPart->thick))));
			else
				map.SetValueAt(index,2,COleVariant(pBomPart->sSizeStr));
			if(pBomPart->length>0)
				map.SetValueAt(index,3,COleVariant(CXhChar50("%.f",pBomPart->length)));
		}
		else
		{
			map.SetValueAt(index,2,COleVariant(pBomPart->sSizeStr));
			if(pBomPart->length>0)
				map.SetValueAt(index,3,COleVariant(CXhChar50("%.f",pBomPart->length)));
		}
		map.SetValueAt(index,4,COleVariant((long)pBomPart->count));		
		index++;
	}
	CXhChar16 cell_end("E%d",index+1);

	/*const int COL_COUNT=10;
	str_arr.SetSize(COL_COUNT);
	str_arr[0]="塔型";str_arr[1]="段号";str_arr[2]="部件编号";
	str_arr[3]="材料名称";str_arr[4]="设计规格";str_arr[5]="材质";
	str_arr[6]="长度";str_arr[7]="单基件数";str_arr[8]="单件重量";str_arr[9]="小计重量";
	double col_arr[10]={8.5,8.5,8.5,8.5,8.5,8.5,8.5,8.5,8.5,10.5};
	CExcelOper::AddRowToExcelSheet(excel_sheet,1,str_arr,col_arr,TRUE);
	int index=0;
	CVariant2dArray map(nPartNum*2,COL_COUNT);//获取Excel表格的范围
	for(IRecoginizer::BOMPART** ppBomPart=dwgPartSet.GetFirst();ppBomPart;ppBomPart=dwgPartSet.GetNext())
	{
		IRecoginizer::BOMPART *pBomPart=*ppBomPart;
		map.SetValueAt(index,0,COleVariant(""));
		SEGI segI;
		ParsePartNo(pBomPart->sLabel,&segI,NULL);
		map.SetValueAt(index,1,COleVariant(segI.ToString()));
		map.SetValueAt(index,2,COleVariant(pBomPart->sLabel));
		if(pBomPart->wPartType==1)
			map.SetValueAt(index,3,COleVariant("角钢"));
		else if(pBomPart->wPartType==2)
			map.SetValueAt(index,3,COleVariant("板材"));
		else if(pBomPart->wPartType==3)
			map.SetValueAt(index,3,COleVariant("钢管"));
		else 
			map.SetValueAt(index,3,COleVariant(""));
		if(stricmp(pBomPart->materialStr,"Q235")==0)
			map.SetValueAt(index,5,COleVariant("A3"));
		else
			map.SetValueAt(index,5,COleVariant(pBomPart->materialStr));
		if(pBomPart->wPartType==2)
		{
			int width=0,thick=0,cls_id=0;
			ParseSpec(pBomPart->sSizeStr,thick,width,cls_id);
			map.SetValueAt(index,4,COleVariant(CXhChar50("-%d",thick)));
			map.SetValueAt(index,6,COleVariant(CXhChar50("%dx%.f",width,pBomPart->length)));
		}
		else
		{
			map.SetValueAt(index,4,COleVariant(pBomPart->sSizeStr));
			map.SetValueAt(index,6,COleVariant(CXhChar50("%.f",pBomPart->length)));
		}
		map.SetValueAt(index,7,COleVariant((long)pBomPart->count));
		map.SetValueAt(index,8,COleVariant(CXhChar50("%.2f",pBomPart->weight)));
		map.SetValueAt(index,9,COleVariant(CXhChar50("=H%d*I%d",index+2,index+2)));
		index++;
	}
	CXhChar16 cell_end("J%d",index+1);*/
	CExcelOper::SetRangeValue(excel_sheet,"A2",cell_end,map.var);
	CExcelOper::SetRangeHorizontalAlignment(excel_sheet,"A1",cell_end,COleVariant((long)2));
	CExcelOper::SetRangeBorders(excel_sheet,"A1",cell_end,COleVariant(10.5));
	excel_sheet.ReleaseDispatch();
	excel_sheets.ReleaseDispatch();
}

bool CDataCmpModel::ExportTmaBomFile(const char* file_path)
{
	CXhPtrSet<IRecoginizer::BOMPART> dwgPartSet;
	if(SummaryBomParts(dwgPartSet,2,NULL)<=0)
		return false;
	int version=8;
	CBuffer buffer;
	ToBomModelBuffer(buffer,dwgPartSet,version);
	FILE *fp=fopen(file_path,"wb");
	long buf_size=buffer.GetLength();
	fwrite(&version,4,1,fp);	//版本号
	fwrite(&buf_size,4,1,fp);	//后续缓存长度
	fwrite(buffer.GetBufferPtr(),buffer.GetLength(),1,fp);
	fclose(fp);
	return true;
}

void CDataCmpModel::ExportDwgBomPartAlterExcel()
{
	//构件统计
	CXhPtrSet<IRecoginizer::BOMPART> dwgPartSet;
	for(CImageFileHost *pImageFile=EnumFirstImage();pImageFile;pImageFile=EnumNextImage())
	{
		pImageFile->SummarizePartInfo();
		for(IRecoginizer::BOMPART* pBomPart=pImageFile->EnumFirstPart();pBomPart;pBomPart=pImageFile->EnumNextPart())
		{
			if( pBomPart->arrItemWarningLevel[0]!=0xFF&&
				pBomPart->arrItemWarningLevel[1]!=0xFF&&
				pBomPart->arrItemWarningLevel[2]!=0xFF&&
				pBomPart->arrItemWarningLevel[3]!=0xFF&&
				pBomPart->arrItemWarningLevel[4]!=0xFF&&
				pBomPart->arrItemWarningLevel[5]!=0xFF)
				continue;
			CalAndSyncTheoryWeightToWeight(pBomPart,FALSE);
			if(pImageFile->m_iSeg.iSeg>0)
				pBomPart->iSeg=pImageFile->m_iSeg.iSeg;
			else
			{
				SEGI segI;
				ParsePartNo(pBomPart->sLabel,&segI,NULL);
				pBomPart->iSeg=segI.iSeg;
			}
			dwgPartSet.append(pBomPart);
		}
	}
	int nPartNum=dwgPartSet.GetNodeNum();
	if(nPartNum<=0)
		return;
	//生成Excel表格
	LPDISPATCH pWorksheets=CExcelOper::CreateExcelWorksheets(1);
	ASSERT(pWorksheets!= NULL);
	Sheets excel_sheets;
	excel_sheets.AttachDispatch(pWorksheets);
	LPDISPATCH pWorksheet=excel_sheets.GetItem(COleVariant((long)1));
	_Worksheet excel_sheet;
	excel_sheet.AttachDispatch(pWorksheet,FALSE);
	excel_sheet.Select();
	excel_sheet.SetName("图纸构件");
	
	CStringArray str_arr;
	const int COL_COUNT=13;
	str_arr.SetSize(COL_COUNT);
	str_arr[0]="";str_arr[1]="合同编号：";str_arr[2]="";
	str_arr[3]="";str_arr[4]="";str_arr[5]="";str_arr[6]="";
	str_arr[7]="";str_arr[8]="单位：KG";str_arr[9]="";str_arr[10]="";
	str_arr[11]="";str_arr[12]="";
	double col_arr[13]={10,10,10,10,10,10,10,10,10,10,10,10,10};
	CExcelOper::SetMainTitle(excel_sheet,"A1","M1","变更明细表");
	CExcelOper::AddRowToExcelSheet(excel_sheet,2,str_arr,col_arr,TRUE);
	str_arr[0]="名称";str_arr[1]="";str_arr[2]="图纸重量";
	str_arr[3]="";str_arr[4]="";str_arr[5]="";str_arr[6]="实际重量";
	str_arr[7]="";str_arr[8]="";str_arr[9]="";str_arr[10]="差额";
	str_arr[11]="";str_arr[12]="";
	CExcelOper::MergeColRange(excel_sheet,"A3","B3");
	CExcelOper::MergeColRange(excel_sheet,"C3","F3");
	CExcelOper::MergeColRange(excel_sheet,"G3","J3");
	CExcelOper::MergeColRange(excel_sheet,"K3","M3");
	CExcelOper::AddRowToExcelSheet(excel_sheet,3,str_arr,col_arr,TRUE);
	str_arr[0]="塔型/段号";str_arr[1]="名称";str_arr[2]="规格";
	str_arr[3]="数量";str_arr[4]="单重";str_arr[5]="重量";str_arr[6]="规格";
	str_arr[7]="数量";str_arr[8]="单重";str_arr[9]="重量";str_arr[10]="差额";
	str_arr[11]="基数";str_arr[12]="总重";
	CExcelOper::AddRowToExcelSheet(excel_sheet,4,str_arr,col_arr,TRUE);
	int index=0;
	CVariant2dArray map(nPartNum*2,COL_COUNT);//获取Excel表格的范围
	for(IRecoginizer::BOMPART* pBomPart=dwgPartSet.GetFirst();pBomPart;pBomPart=dwgPartSet.GetNext())
	{
		map.SetValueAt(index,0,COleVariant(SEGI(pBomPart->iSeg)));
		map.SetValueAt(index,1,COleVariant(pBomPart->sLabel));
		ParseSpec(pBomPart);
		CXhChar16 sSpec(pBomPart->sSizeStr);
		if(pBomPart->wPartType==IRecoginizer::BOMPART::ANGLE)
			sSpec.Printf("∠%s×%s",(char*)CXhChar16(pBomPart->width),(char*)CXhChar16(pBomPart->thick));
		else if(pBomPart->wPartType==IRecoginizer::BOMPART::PLATE)
			sSpec.Printf("-%d",(int)pBomPart->thick);
		else if(pBomPart->wPartType==IRecoginizer::BOMPART::TUBE)
			sSpec.Printf("Φ%s×%s",(char*)CXhChar16(pBomPart->width),(char*)CXhChar16(pBomPart->thick));
		map.SetValueAt(index,2,COleVariant(sSpec));
		map.SetValueAt(index,3,COleVariant((long)pBomPart->count));		
		map.SetValueAt(index,4,COleVariant(CXhChar16("%.1f",pBomPart->weight)));
		map.SetValueAt(index,5,COleVariant(CXhChar50("=D%d*E%d",index+5,index+5)));
		index++;
	}
	CXhChar16 cell_end("M%d",index+4);
	CExcelOper::SetRangeValue(excel_sheet,"A5",cell_end,map.var);
	CExcelOper::SetRangeHorizontalAlignment(excel_sheet,"A2","M3",COleVariant((long)3));
	CExcelOper::SetRangeHorizontalAlignment(excel_sheet,"A4",cell_end,COleVariant((long)2));
	CExcelOper::SetRangeBorders(excel_sheet,"A3",cell_end,COleVariant(10.5));
	excel_sheet.ReleaseDispatch();
	excel_sheets.ReleaseDispatch();
}

void CDataCmpModel::RestoreSpec(const char* spec,double *width,double *thick,char *matStr/*=NULL*/)
{
	char sMat[16]="",cMark1=' ',cMark2=' ';
	if(strstr(spec,"Q")==(char*)spec)
	{
		if(strstr(spec,"L"))
			sscanf(spec,"%[^L]%c%lf%c%lf",sMat,&cMark1,width,&cMark2,thick);
		else if(strstr(spec,"-"))
			sscanf(spec,"%[^-]%c%lf",sMat,&cMark1,thick);
	}
	else if(strstr(spec,"L"))
	{
		CXhChar16 sSpec(spec);
		sSpec.Replace("L","");
		sSpec.Replace("*"," ");
		sSpec.Replace("X"," ");
		sSpec.Replace("x"," ");
		sscanf(sSpec,"%lf%lf",width,thick);
	}
	else if (strstr(spec,"-"))
		sscanf(spec,"%c%lf",sMat,thick);
	//else if(spec,"φ")
	//sscanf(spec,"%c%d%c%d",sMat,)
	if(matStr)
		strcpy(matStr,sMat);
}
CXhChar16 CDataCmpModel::QuerySteelMatMark(char cMat)
{
	CXhChar16 sMatMark;
	if('H'==cMat)
		sMatMark.Copy("Q345");
	else if('G'==cMat)
		sMatMark.Copy("Q390");
	else if('P'==cMat)
		sMatMark.Copy("Q420");
	else if('T'==cMat)
		sMatMark.Copy("Q460");
	else if('S'==cMat)
		sMatMark.Copy("Q235");
	return sMatMark;
}
char CDataCmpModel::QueryBriefMatMark(const char* sMatMark)
{
	char cMat=0;
	if (strstr(sMatMark, "Q345"))
		cMat = 'H';
	else if (strstr(sMatMark, "Q390"))
		cMat = 'G';
	else if (strstr(sMatMark, "Q420"))
		cMat = 'P';
	else if (strstr(sMatMark, "Q460"))
		cMat = 'T';
	else if (strstr(sMatMark, "Q235"))
		cMat = 'S';
	return cMat;
}