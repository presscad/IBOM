// Recog.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <math.h>
#include "Recog.h"
#include "Robot.h"
#include "XhLicAgent.h"

// 这是已导出类的构造函数。
// 有关类定义的信息，请参阅 Recog.h
#ifdef APP_EMBEDDED_MODULE_ENCRYPT
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
#endif

//*********************************************************************************
//全局加密数据
const int CRYPTITEM_COUNT=15;
static CRYPT_PROCITEM g_arrProcItem[CRYPTITEM_COUNT];
static int FuncMark_ToGreyPixelByte;
static int FuncMark_CalRectPolyTransCoef;
static int FuncMark_TransPointToOrginalCoord;
static int FuncMark_AddFloat;
static int FuncMark_DeductFloat;
static int FuncMark_MultiFloat;
static int FuncMark_DivFloat;
static int FuncMark_AddLong;
static int FuncMark_DeductLong;
static int FuncMark_MultiLong;
static int FuncMark_DivLong;
static int FuncMark_GetHoriMargin;
//初始化加密数据函数
#ifdef lksadfsfdj
//No.1 Procedure Item
struct Shell_LayoutWorkPart_Para{
	int _count1,_count2;
	TEMPWORK_PART *_work_part_arr;
	TEMPSTOCK_PARTENT *_temppart_ent_arr;
	Shell_LayoutWorkPart_Para(TEMPWORK_PART *work_part_arr,int count1,TEMPSTOCK_PARTENT *temppart_ent_arr,int count2)
	{
		_count1=count1;
		_count2=count2;
		_work_part_arr=work_part_arr;
		_temppart_ent_arr=temppart_ent_arr;
	}
	//operator DWORD(){return (DWORD)this;};	//将当前结构指针以无符号整数形式返回
};
void LayoutWorkPart_ByShell(TEMPWORK_PART *work_part_arr,int count1,TEMPSTOCK_PARTENT *temppart_ent_arr,int count2)
{
	CallProcItem(0,LayoutWorkPartFuncMark,(DWORD)&Shell_LayoutWorkPart_Para(work_part_arr,count1,temppart_ent_arr,count2));
}
//No.2 new REAL_PART[n]
REAL_PART* NewRealPartArray_ByShell(DWORD dwParam)
{
	return (REAL_PART*)CallProcItem(0,NewRealPartArrayFuncMark,dwParam);
}
//No.3 Procedure Item
struct Shell_SortPartEnt_Para{
	ATOM_LIST<CStockPartEnt> *_part_ent_arr;
	int _by_orglen0_remantlen1_partno2;
	Shell_SortPartEnt_Para(ATOM_LIST<CStockPartEnt> *part_ent_arr/*=NULL*/,int by_orglen0_remantlen1_partno2/*=0*/)
	{
		_part_ent_arr=part_ent_arr;
		_by_orglen0_remantlen1_partno2=by_orglen0_remantlen1_partno2;
	}
};
void SortPartEnt_ByShell(ATOM_LIST<CStockPartEnt> *part_ent_arr/*=NULL*/,int by_orglen0_remantlen1_partno2/*=0*/)
{
	CallProcItem(0,SortPartEntArrayFuncMark,(DWORD)&Shell_SortPartEnt_Para(part_ent_arr,by_orglen0_remantlen1_partno2));
}
//No.4 Procedure Item
struct Shell_GetPendingRealPartMinLen_Para{
	REAL_PART *_real_part_arr;
	int _n;
	Shell_GetPendingRealPartMinLen_Para(REAL_PART *real_part_arr,int n)
	{
		_real_part_arr=real_part_arr;
		_n=n;
	}
};
REAL_PART* GetPendingRealPartMinLen_ByShell(REAL_PART *real_part_arr,int n)
{
	return (REAL_PART*)CallProcItem(0,GetPendingRealPartMinLenFuncMark,
		(DWORD)&Shell_GetPendingRealPartMinLen_Para(real_part_arr,n));
}
//No.5 new TEMPWORK_PART[n]
TEMPWORK_PART* NewTEMPWORK_PARTArray_ByShell(DWORD dwParam)
{
	return (TEMPWORK_PART*)CallProcItem(0,NewTEMPWORK_PARTArrayFuncMark,dwParam);
}
//No.6 new TEMPSTOCK_PARTENT[n]
TEMPSTOCK_PARTENT* NewTEMPSTOCK_PARTENTArray_ByShell(DWORD dwParam)
{
	return (TEMPSTOCK_PARTENT*)CallProcItem(0,NewTEMPSTOCK_PARTENTArrayFuncMark,dwParam);
}
//No.7 返回合并后的综合利用率，０表示不宜合并（无法合并或合并后利用率下降）
struct Shell_MergeStockPartEnt_Para{
	CStockPartEnt* _pStockPartEnt1;
	CStockPartEnt* _pStockPartEnt2;
	CStockPart* _arrOrderedMarketStockPart;
	CStockPartEnt* _pMergedStockPartEnt;
	int _marketpart_num;
	double merged_ratio;
	Shell_MergeStockPartEnt_Para(CStockPartEnt* pStockPartEnt1,CStockPartEnt* pStockPartEnt2,
					   CStockPart* arrOrderedMarketStockPart,int marketpart_num,CStockPartEnt* pMergedStockPartEnt)
	{
		_pStockPartEnt1=pStockPartEnt1;
		_pStockPartEnt2=pStockPartEnt2;
		_arrOrderedMarketStockPart=arrOrderedMarketStockPart;
		_pMergedStockPartEnt=pMergedStockPartEnt;
		_marketpart_num=marketpart_num;
		merged_ratio=0;
	}
};
double MergeStockPartEnt_ByShell(CStockPartEnt* pStockPartEnt1,CStockPartEnt* pStockPartEnt2,
					   CStockPart* arrOrderedMarketStockPart,int marketpart_num,CStockPartEnt* pMergedStockPartEnt)
{
	Shell_MergeStockPartEnt_Para pack_para(pStockPartEnt1,pStockPartEnt2,
		arrOrderedMarketStockPart,marketpart_num,pMergedStockPartEnt);
	CallProcItem(0,MergeStockPartEntFuncMark,(DWORD)&pack_para);
	return pack_para.merged_ratio;
}
#endif
//一、常量数据加密区
const BYTE CMindRobot::get_FRAMELINE_INTEGRITY_PERCENT_LOW()
{
	return 40;	//0.4	存在线宽为1且错行的情况
}
const BYTE CMindRobot::get_FRAMELINE_INTEGRITY_PERCENT_HIGH()
{
	return 90;	//0.9
}
WORD CMindRobot::get_MAX_SPLIT_IMG_ITER_DEPTH()
{
	return 15;
}
float CMindRobot::get_MIN_CHAR_WIDTH_SCALE()
{
	return 0.15f;
}
float CMindRobot::get_PREFER_CHAR_WIDTH_SCALE()
{
	return 0.60f;
}
float CMindRobot::get_MAX_CHAR_WIDTH_SCALE()
{
	return 0.8f;
}
float CMindRobot::get_MATCHFACTORtoSMALL()
{
	return 0.382f;
}
float CMindRobot::get_MATCHFACTORtoBIG()
{
	return 0.618f;
}
static const BYTE _localConstBitBytes[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
static BYTE _localCryptConstBitBytes[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
const BYTE* CMindRobot::get_constBitBytes()
{
	return _localCryptConstBitBytes;
}
BYTE CMindRobot::get_PixelByteWidth()
{
	return 8;
}

#ifdef _DEBUG
void CMindRobot::WriteDataCore(const char* path_file)
{
	CBuffer buffer;
	buffer.WriteByte(get_FRAMELINE_INTEGRITY_PERCENT_LOW());
	buffer.WriteByte(get_FRAMELINE_INTEGRITY_PERCENT_HIGH());
	buffer.WriteWord(get_MAX_SPLIT_IMG_ITER_DEPTH());
	buffer.WriteFloat(get_MIN_CHAR_WIDTH_SCALE());
	buffer.WriteFloat(get_MAX_CHAR_WIDTH_SCALE());
	buffer.WriteFloat(get_MATCHFACTORtoSMALL());
	buffer.WriteFloat(get_MATCHFACTORtoSMALL());
	buffer.WriteFloat(get_MATCHFACTORtoBIG());

	FILE *fp=fopen(path_file,"wb");
	int nLen=buffer.GetLength();
	fwrite(&nLen,4,1,fp);
	fwrite(buffer.GetBufferPtr(),buffer.GetLength(),1,fp);
	fclose(fp);
}
#endif

//二、核心算法函数加密区
//No.1 
DWORD __Shell_ToGreyPixelByte(DWORD dwParam)	//从真彩24位转换为灰度值像素
{
	BYTE* pcbPixelRGB=(BYTE*)dwParam;
	BYTE r=pcbPixelRGB[2];
	BYTE g=pcbPixelRGB[1];
	BYTE b=pcbPixelRGB[0];
	/*RGB按照300:300:400比例分配
	字体颜色RGB(103,101,114)  综合值106.8
	背景颜色RGB(165,166,171)  综合值150.6
	*/
	BYTE cbGreyness=(b*114+g*587+r*299)/1000;
	return cbGreyness;
}
BYTE CMindRobot::ToGreyPixelByte(BYTE* pcbPixelRGB)	//从真彩24位转换为灰度值像素
{
#ifndef __ENCRYPT_VERSION_
	BYTE r=pcbPixelRGB[2];
	BYTE g=pcbPixelRGB[1];
	BYTE b=pcbPixelRGB[0];
	/*RGB按照300:300:400比例分配
	字体颜色RGB(103,101,114)  综合值106.8
	背景颜色RGB(165,166,171)  综合值150.6
	*/
	BYTE cbGreyness=(b*114+g*587+r*299)/1000;
	return cbGreyness;
#else
	return (BYTE)CallProcItem(0,FuncMark_ToGreyPixelByte,(DWORD)pcbPixelRGB);
#endif
}
//No.2 
struct Shell_CalRectPolyTransCoef_Para{
	POINT vertexes[4];
	double kesai[4];
	Shell_CalRectPolyTransCoef_Para(POINT _vertexes[4],double _kesai[4])
	{
		memcpy(vertexes,_vertexes,sizeof(POINT)*4);
		memcpy(kesai,_kesai,sizeof(double)*4);
	}
};
DWORD __Shell_CalRectPolyTransCoef(DWORD dwParam)
{
	Shell_CalRectPolyTransCoef_Para* params=(Shell_CalRectPolyTransCoef_Para*)dwParam;
	POINT leftup=params->vertexes[0],leftdown=params->vertexes[1];
	POINT rightup=params->vertexes[2],rightdown=params->vertexes[3];
	int width =(rightup.x+rightdown.x-leftup.x-leftdown.x)/2;
	int height=(leftdown.y+rightdown.y-leftup.y-rightup.y)/2;
	params->kesai[0]=(double)(rightup.x-leftup.x)/width;
	params->kesai[1]=(double)(rightdown.x-leftdown.x)/width;
	params->kesai[2]=(double)(leftdown.y-leftup.y)/height;
	params->kesai[3]=(double)(rightdown.y-rightup.y)/height;
	return true;
}
bool  CMindRobot::CalRectPolyTransCoef(POINT vertexes[4],double kesai[4])
{
#ifndef __ENCRYPT_VERSION_
	POINT leftup=vertexes[0],leftdown=vertexes[1],rightup=vertexes[2],rightdown=vertexes[3];
	int width =(rightup.x+rightdown.x-leftup.x-leftdown.x)/2;
	int height=(leftdown.y+rightdown.y-leftup.y-rightup.y)/2;
	kesai[0]=(double)(rightup.x-leftup.x)/width;
	kesai[1]=(double)(rightdown.x-leftdown.x)/width;
	kesai[2]=(double)(leftdown.y-leftup.y)/height;
	kesai[3]=(double)(rightdown.y-rightup.y)/height;
	return true;
#else
	Shell_CalRectPolyTransCoef_Para pack_para(vertexes,kesai);
	CallProcItem(0,FuncMark_CalRectPolyTransCoef,(DWORD)&pack_para);
	memcpy(kesai,pack_para.kesai,sizeof(double)*4);
	return true;
#endif
}
//No.3 
struct Shell_TransPointToOrginalCoord_Para{
	POINT p;
	double kesai[4];
	POINT vertexes[4];
	int width,height;
	Shell_TransPointToOrginalCoord_Para(POINT _p,double _kesai[4],POINT _vertexes[4],int _width,int _height)
	{
		p=_p;
		memcpy(kesai,_kesai,sizeof(double)*4);
		memcpy(vertexes,_vertexes,sizeof(POINT)*4);
		width=_width;
		height=_height;
	}
public:
	POINT rslt;
};
DWORD __Shell_TransPointToOrginalCoord(DWORD dwParam)
{
	Shell_TransPointToOrginalCoord_Para* params=(Shell_TransPointToOrginalCoord_Para*)dwParam;
	POINT leftup=params->vertexes[0],leftdown=params->vertexes[1];
	POINT rightup=params->vertexes[2],rightdown=params->vertexes[3];
	double yitax=(double)params->p.x/params->width;	//ηx
	double yitay=(double)params->p.y/params->height;//ηy
	params->rslt.x=(long)(0.5+(params->kesai[0]*params->p.x+leftup.x)*(1-yitay)+(params->kesai[1]*params->p.x+leftdown.x)*yitay);
	params->rslt.y=(long)(0.5+(params->kesai[2]*params->p.y+leftup.y)*(1-yitax)+(params->kesai[3]*params->p.y+rightup.y)*yitax);
	return 0;
}
POINT CMindRobot::TransPointToOrginalCoord(POINT p,double kesai[4],POINT vertexes[4],int width,int height)
{
#ifndef __ENCRYPT_VERSION_
	POINT point,leftup=vertexes[0],leftdown=vertexes[1],rightup=vertexes[2],rightdown=vertexes[3];
	double yitax=(double)p.x/width;	//ηx
	double yitay=(double)p.y/height;//ηy
	point.x=(long)((kesai[0]*p.x+leftup.x)*(1-yitay)+(kesai[1]*p.x+leftdown.x)*yitay);
	point.y=(long)((kesai[2]*p.y+leftup.y)*(1-yitax)+(kesai[3]*p.y+rightup.y)*yitax);
	return point;
#else
	Shell_TransPointToOrginalCoord_Para pack_para(p,kesai,vertexes,width,height);
	CallProcItem(0,FuncMark_TransPointToOrginalCoord,(DWORD)&pack_para);
	return pack_para.rslt;
#endif
}
//No.4 
struct Shell_DualFloatOperation_Para{
	double fv1,fv2;
	Shell_DualFloatOperation_Para(double _fv1,double _fv2)
	{
		fv1=_fv1;
		fv2=_fv2;
	}
public:
	double rslt;
};
DWORD __Shell_AddFloat(DWORD dwParam)
{
	Shell_DualFloatOperation_Para* params=(Shell_DualFloatOperation_Para*)dwParam;
	params->rslt=params->fv1+params->fv2;
	return 0;
}
double CMindRobot::Shell_AddFloat(double fv1,double fv2)
{
#ifndef __ENCRYPT_VERSION_
	return fv1+fv2;
#else
	Shell_DualFloatOperation_Para pack_para(fv1,fv2);
	CallProcItem(0,FuncMark_AddFloat,(DWORD)&pack_para);
	return pack_para.rslt;
#endif
}
//No.5 
DWORD __Shell_DeductFloat(DWORD dwParam)
{
	Shell_DualFloatOperation_Para* params=(Shell_DualFloatOperation_Para*)dwParam;
	params->rslt=params->fv1-params->fv2;
	return 0;
}
double CMindRobot::Shell_DeductFloat(double fv1,double fv2)
{
#ifndef __ENCRYPT_VERSION_
	return fv1-fv2;
#else
	Shell_DualFloatOperation_Para pack_para(fv1,fv2);
	CallProcItem(0,FuncMark_DeductFloat,(DWORD)&pack_para);
	return pack_para.rslt;
#endif
}
//No.6 
DWORD __Shell_MultiFloat(DWORD dwParam)
{
	Shell_DualFloatOperation_Para* params=(Shell_DualFloatOperation_Para*)dwParam;
	params->rslt=params->fv1*params->fv2;
	return 0;
}
double CMindRobot::Shell_MultiFloat(double fv1,double fv2)
{
#ifndef __ENCRYPT_VERSION_
	return fv1*fv2;
#else
	Shell_DualFloatOperation_Para pack_para(fv1,fv2);
	CallProcItem(0,FuncMark_MultiFloat,(DWORD)&pack_para);
	return pack_para.rslt;
#endif
}
//No.7 
DWORD __Shell_DivFloat(DWORD dwParam)
{
	Shell_DualFloatOperation_Para* params=(Shell_DualFloatOperation_Para*)dwParam;
	params->rslt=params->fv1/params->fv2;
	return 0;
}
double CMindRobot::Shell_DivFloat(double fv1,double fv2)
{
#ifndef __ENCRYPT_VERSION_
	return fv1/fv2;
#else
	Shell_DualFloatOperation_Para pack_para(fv1,fv2);
	CallProcItem(0,FuncMark_DivFloat,(DWORD)&pack_para);
	return pack_para.rslt;
#endif
}
//No.8 
struct Shell_DualLongOperation_Para{
	long iv1,iv2;
	Shell_DualLongOperation_Para(long _iv1,long _iv2)
	{
		iv1=_iv1;
		iv2=_iv2;
	}
public:
	long rslt;
};
DWORD __Shell_AddLong(DWORD dwParam)
{
	Shell_DualLongOperation_Para* params=(Shell_DualLongOperation_Para*)dwParam;
	params->rslt=params->iv1+params->iv2;
	return 0;
}
long   CMindRobot::Shell_AddLong(long iv1,long iv2)
{
#ifndef __ENCRYPT_VERSION_
	return iv1+iv2;
#else
	Shell_DualLongOperation_Para pack_para(iv1,iv2);
	CallProcItem(0,FuncMark_AddLong,(DWORD)&pack_para);
	return pack_para.rslt;
#endif
}
//No.9 
DWORD __Shell_DeductLong(DWORD dwParam)
{
	Shell_DualLongOperation_Para* params=(Shell_DualLongOperation_Para*)dwParam;
	params->rslt=params->iv1-params->iv2;
	return 0;
}
long   CMindRobot::Shell_DeductLong(long iv1,long iv2)
{
#ifndef __ENCRYPT_VERSION_
	return iv1-iv2;
#else
	Shell_DualLongOperation_Para pack_para(iv1,iv2);
	CallProcItem(0,FuncMark_DeductLong,(DWORD)&pack_para);
	return pack_para.rslt;
#endif
}
//No.10 
DWORD __Shell_MultiLong(DWORD dwParam)
{
	Shell_DualLongOperation_Para* params=(Shell_DualLongOperation_Para*)dwParam;
	params->rslt=params->iv1*params->iv2;
	return 0;
}
long   CMindRobot::Shell_MultiLong(long iv1,long iv2)
{
#ifndef __ENCRYPT_VERSION_
	return iv1*iv2;
#else
	Shell_DualLongOperation_Para pack_para(iv1,iv2);
	CallProcItem(0,FuncMark_MultiLong,(DWORD)&pack_para);
	return pack_para.rslt;
#endif
}
//No.11 
DWORD __Shell_DivLong(DWORD dwParam)
{
	Shell_DualLongOperation_Para* params=(Shell_DualLongOperation_Para*)dwParam;
	params->rslt=params->iv1/params->iv2;
	return 0;
}
long   CMindRobot::Shell_DivLong(long iv1,long iv2)
{
#ifndef __ENCRYPT_VERSION_
	return iv1/iv2;
#else
	Shell_DualLongOperation_Para pack_para(iv1,iv2);
	CallProcItem(0,FuncMark_DivLong,(DWORD)&pack_para);
	return pack_para.rslt;
#endif
}
//No.12
struct Shell_GetHoriMargin_Para{
	int _width;
	double _vfScaleCoef;
	Shell_GetHoriMargin_Para(int width,double vfScaleCoef){
		_width=width;
		_vfScaleCoef=vfScaleCoef;
		margin=5;
	}
public:	//输出
	DWORD margin;
};
DWORD __Shell_GetHoriMargin(DWORD dwParam)
{
	Shell_GetHoriMargin_Para* params=(Shell_GetHoriMargin_Para*)dwParam;
	int margin=params->_width-(int)(35*params->_vfScaleCoef+0.5);
	return params->margin=margin<10?margin:10;	//min(10,margin);
}
UINT CMindRobot::GetHoriMargin(int width,double vfScaleCoef/*=1.0*/)
{	//存在数量列文本不居中现像，故此横向边距项也不宜取值太大
#ifndef __ENCRYPT_VERSION_
	int margin=width-(int)(35*vfScaleCoef+0.5);
	return margin<10?margin:10;	//min(10,margin);
#else
	Shell_GetHoriMargin_Para pack_para(width, vfScaleCoef);
	CallProcItem(0,FuncMark_GetHoriMargin,(DWORD)&pack_para);
	return pack_para.margin;
#endif
}
//CImageDataRegion::AnalyzeLinePixels
//CImageDataRegion::RecognizeDatas
void CMindRobot::InitCryptData()
{
	for(int i=0;i<CRYPTITEM_COUNT;i++)
		g_arrProcItem[i].cType=1;	//函数
	g_arrProcItem[ 0].val.func=__Shell_ToGreyPixelByte;//BYTE CMindRobot::ToGreyPixelByte(BYTE* pcbPixelRGB)	//从真彩24位转换为灰度值像素
	g_arrProcItem[ 1].val.func=__Shell_CalRectPolyTransCoef;
	g_arrProcItem[ 2].val.func=__Shell_TransPointToOrginalCoord;
	g_arrProcItem[ 3].val.func=__Shell_AddFloat;
	g_arrProcItem[ 4].val.func=__Shell_DeductFloat;
	g_arrProcItem[ 5].val.func=__Shell_MultiFloat;
	g_arrProcItem[ 6].val.func=__Shell_DivFloat;
	g_arrProcItem[ 7].val.func=__Shell_AddLong;
	g_arrProcItem[ 8].val.func=__Shell_DeductLong;
	g_arrProcItem[ 9].val.func=__Shell_MultiLong;
	g_arrProcItem[10].val.func=__Shell_DivLong;
	g_arrProcItem[11].val.func=__Shell_GetHoriMargin;
	
	DWORD order_arr[CRYPTITEM_COUNT];
	bool result=InitOrders(0,g_arrProcItem,CRYPTITEM_COUNT);
	result=GetCallOrders(0,order_arr,CRYPTITEM_COUNT);
	FuncMark_ToGreyPixelByte=order_arr[0];
	FuncMark_CalRectPolyTransCoef=order_arr[1];
	FuncMark_TransPointToOrginalCoord=order_arr[2];
	FuncMark_AddFloat=order_arr[3];
	FuncMark_DeductFloat=order_arr[4];
	FuncMark_MultiFloat=order_arr[5];
	FuncMark_DivFloat=order_arr[6];
	FuncMark_AddLong=order_arr[7];
	FuncMark_DeductLong=order_arr[8];
	FuncMark_MultiLong=order_arr[9];
	FuncMark_DivLong=order_arr[10];
	FuncMark_GetHoriMargin=order_arr[11];
}