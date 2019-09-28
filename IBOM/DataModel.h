#pragma once
#include "f_ent_list.h"
#include "Variant.h"
#include "XhCharString.h"
#include "LogFile.h"
#include "Buffer.h"
#include "HashTable.h"
#include "ImageHost.h"
#include "PartLibraryEx.h"
#include "objptr_list.h"

struct SEGITEM
{
	SEGI iSeg;
	CXhChar16 sSegName;
};
//Bom数据
class CBomFileData
{
	CHashStrList<IRecoginizer::BOMPART> hashParts;	//构件记录集合
	BOOL ImportExcelBomFileByDllFromat(CVariant2dArray &sheetContextMap);
	BOOL ImportExcelBomFileCore(CVariant2dArray &sheetContextMap,CHashStrList<DWORD> &hashColIndexByColTitle,int startRowIndex);
public:
	CXhChar100 m_sName;
public:
	CBomFileData();
	~CBomFileData();
	void FromBuffer(CBuffer& buffer,long version=0);
	void ToBuffer(CBuffer& buffer,long version=0);
	//
	IRecoginizer::BOMPART* FromPartNo(const char* sPartNo){return hashParts.GetValue(sPartNo);}
	IRecoginizer::BOMPART* AddBomPart(const char* sPartNo){return hashParts.Add(sPartNo);}
	IRecoginizer::BOMPART* EnumFirstPart(){return hashParts.GetFirst();}
	IRecoginizer::BOMPART* EnumNextPart(){return hashParts.GetNext();}
	BOOL ImportExcelBomFile(const char* sFileName);
	BOOL ImportDwgTxtFile(const char* sFileName);
};
//////////////////////////////////////////////////////////////////////////
struct PROGRESS_CONTEXT{
	static int F2I(double fval);
	const char* title;
	BYTE ciStartProgressNumber,ciToProgressNumber;
	int Mapping(double percent);	//percent为百分数,将0~100的百分比映射转换至ciStartProgressNumber~ciToProgressNumber区间
	PROGRESS_CONTEXT();
};
class CDataCmpModel
{
public:
	static const BYTE DATA_CMP_MODEL			= 0;
	static const BYTE DWG_DATA_MODEL			= 1;
	static const BYTE LOFT_DATA_MODEL			= 2;
	static const BYTE DWG_BOM_GROUP				= 3;	//图纸BOM组
	static const BYTE DWG_BOM_DATA				= 4;	//图纸BOM数据
	static const BYTE DWG_IMAGE_GROUP			= 5;	//图纸图片组
	static const BYTE IMAGE_SEG_GROUP			= 6;	//段号分组
	static const BYTE DWG_IMAGE_DATA			= 7;	//图纸图片数据
	static const BYTE IMAGE_REGION_DATA			= 8;	//图片区域数据
	static const BYTE LOFT_BOM_GROUP			= 9;	//放样BOM组
	static const BYTE LOFT_BOM_DATA				= 10;	//放样BOM数据
public:
	struct COMPARE_PART_RESULT
	{
		IRecoginizer::BOMPART *pDwgPart;
		IRecoginizer::BOMPART *pLoftPart;
		CHashStrList<BOOL> hashBoolByPropName;
		COMPARE_PART_RESULT(){pDwgPart = NULL;pLoftPart = NULL;};
	};
public:
	CXhChar50 m_sCompanyName;	//设计单位
	CXhChar50 m_sPrjCode;		//工程编号
	CXhChar50 m_sPrjName;		//工程名称
	CXhChar50 m_sTowerTypeName; //塔型名称
	CXhChar50 m_sTowerAlias;	//塔型代号
	CXhChar50 m_sTaStampNo;		//钢印号
	CXhChar50 m_sOperator;		//操作员（制表人）
	CXhChar50 m_sAuditor;		//审核人
	CXhChar50 m_sCritic;		//评审人
	CXhChar50 m_sFileVersion;	//当前文件版本
	int m_uiOriginalDogKey;		//原始加密锁号
	int m_uiLastSaveDogKey;		//最后一次保存时的加密锁号
	//
	PROGRESS_CONTEXT progress;
private:
	CHashList<SEGITEM> m_segHashList;
	ATOM_LIST<CImageFileHost> m_xDwgImageList;	//图纸-图片文件列表
	ATOM_LIST<CBomFileData> m_xDwgBomList;		//图纸-Bom文件列表
	ATOM_LIST<CBomFileData> m_xLoftBomList;		//放样-Bom文件列表
	CHashStrList<COMPARE_PART_RESULT> m_hashCompareResultByPartNo;
public:
	CDataCmpModel();
	~CDataCmpModel();
	void Empty();
	void InitFileProp();
	BOOL ParseEmbedImageFileToTempFile(const char* file_path);
	BOOL DeleteTempImageFile();
	static BOOL LoadIBomFileToBuffer(const char* sIBOMFilePath,CBuffer &modelBuffer);
	void FromBuffer(CBuffer& buffer,long version=0);
	void ToBuffer(CBuffer& buffer,long version=0,bool bAutoBakSave=false);
	void (*DisplayProcess)(int percent,const char *sTitle);	//进度显示回调函数
	//
	void ComplementSegItemHashList();
	void DeleteSegItem(DWORD key);
	SEGITEM* NewSegI(long iSegI=0);
	SEGITEM* EnumFirstSeg(){return m_segHashList.GetFirst();}
	SEGITEM* EnumNextSeg(){return m_segHashList.GetNext();}
	//图纸-图片文件操作
	int GetBomImageNum(){return m_xDwgImageList.GetNodeNum();}
	void DeleteImageFile(const char* sFileName,SEGI iSeg);
	CImageFileHost* GetImageFile(const char* sFileName,int page_no=1);
	CImageFileHost* AppendImageFile(const char* szPathFileName,SEGI iSeg,bool loadFileInstant=true,int iPageNo=1,int nRotationCount=0,double fZoomScale=0);
	CImageFileHost* EnumFirstImage(SEGI iSeg=-1);
	CImageFileHost* EnumNextImage(SEGI iSeg=-1);
	//图纸-BOM文件操作
	int GetDwgBomNum(){return m_xDwgBomList.GetNodeNum();}
	void DeleteDwgBomFile(const char* sFileName);
	CBomFileData* GetDwgBomFile(const char* sFileName);
	CBomFileData* AppendDwgBomFile(const char* sFileName);
	CBomFileData* EnumFirstDwgBom(){return m_xDwgBomList.GetFirst();}
	CBomFileData* EnumNextDwgBom(){return m_xDwgBomList.GetNext();}
	//放样-BOM文件操作
	int GetLoftBomNum(){return m_xLoftBomList.GetNodeNum();}
	void DeleteLoftBomFile(const char* sFileName);
	CBomFileData* GetLoftBomFile(const char* sFileName);
	CBomFileData* AppendLoftBomFile(const char* sFileName);
	CBomFileData* EnumFirstLoftBom(){return m_xLoftBomList.GetFirst();}
	CBomFileData* EnumNextLoftBom(){return m_xLoftBomList.GetNext();}
	//
	void SummarizeDwgParts(CHashStrList<IRecoginizer::BOMPART*>& hashPartList);
	void SummarizeLoftParts(CHashStrList<IRecoginizer::BOMPART*>& hashPartList);
	int SummaryBomParts(CXhPtrSet<IRecoginizer::BOMPART> &dwgPartSet,BYTE image0_bom1_all2,SEGI *pSegI=NULL);
	//校审操作
	int ComparePartData(double LEN_TOLERANCE=5.0,double WEIGHT_TOLERANCE=1.0);
	DWORD GetResultCount(){return m_hashCompareResultByPartNo.GetNodeNum();}
	COMPARE_PART_RESULT* GetResult(const char* part_no){return m_hashCompareResultByPartNo.GetValue(part_no);}
	COMPARE_PART_RESULT* EnumFirstResult(){return m_hashCompareResultByPartNo.GetFirst();}
	COMPARE_PART_RESULT* EnumNextResult(){return m_hashCompareResultByPartNo.GetNext();}
	//导出校审结果
	void AddBomResultSheet(LPDISPATCH pSheet,int iSheet);
	void ExportCompareResult();
	void ExportDwgDataExcel(BYTE image0_bom1_all2,SEGI *pSegI=NULL);
	void ExportDwgBomPartAlterExcel();
	bool ExportTmaBomFile(const char* file_path);
	//
	static void RestoreSpec(const char* spec,double *width,double *thick,char *matStr=NULL);
	static char QueryBriefMatMark(const char* sMatMark);
	static CXhChar16 QuerySteelMatMark(char cMat);
	static void ToBomModelBuffer(CBuffer &buffer,CXhPtrSet<IRecoginizer::BOMPART> &dwgPartSet,DWORD dwVersion=NULL);
};
extern CDataCmpModel dataModel;