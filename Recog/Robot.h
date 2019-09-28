#pragma once

#include "Hashtable.h"
#include "MemberProperty.h"
#include ".\Recog.h"
#include ".\ImageRecognizer.h"

class CMindRobot : public IMindRobot
{
public:
	CAlphabets knowledge;
	CHashPtrList<CImageDataFile> listImageFiles;
	char szTempFilePath[201];
	CMindRobot();
public:	//加密数据常量
	static DWORD IMG_TYPE_BMP;	//"bmp0"
	static DWORD IMG_TYPE_JPG;	//"jpg1"
	static DWORD IMG_TYPE_PNG;	//"png2"
	static DWORD IMG_TYPE_PDF;	//"pdf3"
public:	//加密算法函数
	static void InitCryptData();
	static BYTE ToGreyPixelByte(BYTE* pcbPixelRGB);	//从真彩24位转换为灰度值像素
	static bool CalRectPolyTransCoef(POINT vertexes[4],double kesai[4]);
	static POINT TransPointToOrginalCoord(POINT p,double kesai[4],POINT vertexes[4],int width,int height);
	double Shell_AddFloat(double fv1,double fv2);
	double Shell_DeductFloat(double fv1,double fv2);
	double Shell_MultiFloat(double fv1,double fv2);
	double Shell_DivFloat(double fv1,double fv2);
	long   Shell_AddLong(long iv1,long iv2);
	long   Shell_DeductLong(long iv1,long iv2);
	long   Shell_MultiLong(long iv1,long iv2);
	long   Shell_DivLong(long iv1,long iv2);
	UINT GetHoriMargin(int width,double vfScaleCoef=1.0);
	//READONLY_PROPERTY(BYTE*,constBitBytes)
	const BYTE* get_constBitBytes();
	__declspec(property(get=get_constBitBytes)) const BYTE* constBitBytes;
	//READONLY_PROPERTY(BYTE,PIXELBYTE_WIDTH)
	BYTE get_PixelByteWidth();
	__declspec(property(get=get_PixelByteWidth)) BYTE PIXELBYTE_WIDTH;
	//READONLY_PROPERTY(BYTE,FRAMELINE_INTEGRITY_PERCENT)
	const BYTE get_FRAMELINE_INTEGRITY_PERCENT_LOW();
	__declspec(property(get=get_FRAMELINE_INTEGRITY_PERCENT_LOW)) BYTE FRAMELINE_INTEGRITY_PERCENT_LOW;
	//READONLY_PROPERTY(BYTE,FRAMELINE_INTEGRITY_PERCENT_HIGH)
	const BYTE get_FRAMELINE_INTEGRITY_PERCENT_HIGH();
	__declspec(property(get=get_FRAMELINE_INTEGRITY_PERCENT_HIGH)) BYTE FRAMELINE_INTEGRITY_PERCENT_HIGH;
	//READONLY_PROPERTY(WORD,MAX_SPLIT_IMG_ITER_DEPTH)
	WORD get_MAX_SPLIT_IMG_ITER_DEPTH();
	__declspec(property(get=get_MAX_SPLIT_IMG_ITER_DEPTH)) WORD MAX_SPLIT_IMG_ITER_DEPTH;
	//READONLY_PROPERTY(float,MIN_CHAR_WIDTH_SCALE)
	float get_MIN_CHAR_WIDTH_SCALE();
	__declspec(property(get=get_MIN_CHAR_WIDTH_SCALE)) float MIN_CHAR_WIDTH_SCALE;
	//READONLY_PROPERTY(float,PREFER_CHAR_WIDTH_SCALE)
	float get_PREFER_CHAR_WIDTH_SCALE();
	__declspec(property(get=get_PREFER_CHAR_WIDTH_SCALE)) float PREFER_CHAR_WIDTH_SCALE;
	//READONLY_PROPERTY(float,MAX_CHAR_WIDTH_SCALE)
	float get_MAX_CHAR_WIDTH_SCALE();
	__declspec(property(get=get_MAX_CHAR_WIDTH_SCALE)) float MAX_CHAR_WIDTH_SCALE;
	
	//READONLY_PROPERTY(BYTE,LESSTHAN_SPLIT_PIXELS)
	//READONLY_PROPERTY(BYTE,MIN_ISLAND_PIXELS)
	//READONLY_PROPERTY(float,MATCHFACTORtoSMALL)
	float get_MATCHFACTORtoSMALL();
	__declspec(property(get=get_MATCHFACTORtoSMALL)) float MATCHFACTORtoSMALL;
	//READONLY_PROPERTY(float,MATCHFACTORtoBIG)
	float get_MATCHFACTORtoBIG();
	__declspec(property(get=get_MATCHFACTORtoBIG)) float MATCHFACTORtoBIG;
	
#ifdef _DEBUG
	void WriteDataCore(const char* path_file);
#endif
public:
	//获取识别机器人字母知识库
	static IAlphabets* GetAlphabetKnowledge();
	//原始待识别BOM图像文件函数
	static IImageFile* AddImageFile(const char* imagefile,bool loadFileInstant=true);
	static IImageFile* FromImageFileSerial(long serial);
	static IImageFile* EnumFirstImage();
	static IImageFile* EnumNextImage();
	static BOOL DestroyImageFile(long serial);
};
extern CMindRobot robot;
