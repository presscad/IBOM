#pragma once
//#include "f_ent.h"
#include <stdio.h>
#include "Buffer.h"
#include "TempFile.h"
#include "PdfEngine.h"
#include "Recog.h"

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

class CSamplePixelLine
{
public:
	int m_nValidBlackSeg;	//象素线内检测到黑色线段的数量
	//[i][0]表示第i段的起始点位，[i][1]表示第i段的终止点位
	int shiftSegArr[5][2];
	int m_iPosIndex;
public:
	CSamplePixelLine();
	~CSamplePixelLine();
	//pixels中true表示黑点;false表示白点
	bool AnalyzeLinePixels(bool* pixels,int nNum);
	bool FindBlackSegInSpecScope(int scope_start,int scope_end,int* seg_start, int* seg_end);
};
class CFileLifeObject{
	FILE* m_fp;
public:
	CFileLifeObject(FILE* fp=NULL){m_fp=fp;}
	~CFileLifeObject(){
		if(m_fp!=NULL)
			fclose(m_fp);
	}
public:
	virtual void AttachFile(FILE* fp){m_fp=fp;}
	virtual void DetachFile(){m_fp=NULL;}
};
struct PDF_FILE_CONFIG;
//将图像由灰度图转为黑白单色图的划分阈值区域类，对应CImageDataRegion类
class CMonoThresholder
{
	bool m_bInitThresholder;
	BYTE *m_lpMonoThreshold;	//记录每个像素黑白分界灰度值
public:
	int m_nRgnMonoForwardPixels;	
	POINT m_xTopLeft;
	SIZE m_xRgnSize;
public:
	class ROW_PIXELS{
		BYTE* pixels;
		int xiStartIndex,xiRowWidth;
	public:
		ROW_PIXELS(BYTE* pxPixels,int _xiStartIndex,int _xiWidth);
		BYTE operator[](int xiColIndex);
	};
	CMonoThresholder();
	void SetRegion(int xiTopLeft,int yiTopLeft,int cx,int cy);
	void SetRgnMonoForward(int nMonoForward){m_nRgnMonoForwardPixels=nMonoForward;}
	bool InitByGreyImgPixels(BYTE *m_lpGrayBitsMap,int width,int height);
	ROW_PIXELS GetRowPixels(int yJ);
	ROW_PIXELS operator[](int yJ){return GetRowPixels(yJ);}
};
struct IMGROT{
	POINT xOrg;
	IMGROT(int xiOrg,int yjOrg,double dfEdgeWidthX,double yjRotPixels);
	void Init(int xiOrg,int yjOrg,double dfEdgeWidthX,double yjRotPixels);
	POINT Rotate(const POINT& point,bool inverse=false) const;
	POINT Rotate(int xiPoint,int yjPoint,bool inverse=false) const;
private:
	double sina,cosa;	//旋转角的正弦与余弦
};
#include "Interpolator.h"
struct XHPOINT{
	double x,y;
	static int f2i(double fv){return fv>0?(int)(fv+0.5):(int)(fv-0.5);}
	XHPOINT(double _x=0,double _y=0){x=_x;y=_y;}
	operator POINT(){
		POINT p;
		p.x=f2i(x);p.y=f2i(y);
		return p;
	}
	friend XHPOINT operator-(XHPOINT& point,XHPOINT& point2){
		return XHPOINT(point.x-point2.x,point.y-point2.y);
	}	
	friend XHPOINT operator+(XHPOINT& point,XHPOINT& point2){
		return XHPOINT(point.x+point2.x,point.y+point2.y);
	}
	friend XHPOINT operator*(double fact,XHPOINT& point){
		return XHPOINT(fact*point.x,fact*point.y);
	}	
	XHPOINT& operator*(double c){
		x*=c;
		y*=c;
		return *this;
	}
	XHPOINT& operator/(double c){
		x/=c;
		y/=c;
		return *this;
	}
	XHPOINT& operator+=(const XHPOINT& other){
		x+=other.x;
		y+=other.y;
		return *this;
	}
	XHPOINT& operator-=(const XHPOINT& other){
		x-=other.x;
		y-=other.y;
		return *this;
	}
	XHPOINT& operator*=(const double& fact){
		x*=fact;
		y*=fact;
		return *this;
	}
	XHPOINT& operator/=(const double& fact){
		x/=fact;
		y/=fact;
		return *this;
	}
};
struct TRANSFORM{
	int m_iMonoBalance;			//黑白平衡调节值，+值表示向白调节, -值表示向黑调节
	CFittingMap<XHPOINT> gridmap;	//网格映射点坐标
	bool IsValid(){gridmap.RowCount>=2&&gridmap.ColCount>=2;}
	TRANSFORM(){m_iMonoBalance=0;}
};
class CImageTransform  
{
	//LPBITMAPINFOHEADER m_lpBMIH;	//图像的表头信息（含调色板数据区）
	bool m_bInternalRawBitsBuff;	//True:内部分配内存；False:m_lpRawBits为外部传入共享内存，不能在内部释放
	DWORD m_uiExterRawBitsuffSize;
	BYTE *m_lpRawBits;				//行扫描(Downward top->bottom)形式的位图字节数据(真彩24位格式），处理前图像数据,m_nEffByteWidth*m_nHeight
	BYTE *m_lpPixelGrayThreshold;	//记录每个像素黑白分界灰度值
	BYTE *m_lpGrayBitsMap;			//单字节灰度型的图像像素数据表，主要用于图像处理
	int m_nWidth,m_nHeight;			//图片的宽度和高度(像素单位)
	int m_nEffByteWidth;			//图片行对齐后的实际字节数（4 byte对齐）
	UINT m_uBitcount;				//每个显示象素的颜色显示位数：1为二色;4为16色;8为256色;16、24、32为真彩色
	int m_nDrawingRgnWidth,m_nDrawingRgnHeight;//图片的宽度和高度(像素单位)
	double kesai[4];				//ψ[4]四边形与矩形空间变换的参数
	POINT m_leftUp,m_leftDown,m_rightUp,m_rightDown;	//工艺卡边框的四个点
	BYTE m_ubGreynessThreshold;		//在进行二值化(黑白二色)处理时的阈值，灰度大于此值为白点，小于此值为黑点
	BOOL m_bExternalPdfEngine;
	BaseEngine* m_pPdfEngine;
	BYTE m_ciRawImageFileType;
	PDF_FILE_CONFIG m_xPdfCfg;
	int m_nMonoForward;
private:
	void Clear();
	bool ReadBmpFile(FILE* fp,BYTE* lpExterRawBitsBuff=NULL,UINT uiBitsBuffSize=0);
	bool ReadBmpFileByTempFile(CTempFileBuffer &tempBuffer,BYTE* lpExterRawBitsBuff=NULL,UINT uiBitsBuffSize=0);
	bool WriteBmpFile(FILE* fp);
	bool WriteJpegFile(FILE* fp);
	bool WritePngFile(FILE* fp);
	bool InitFromBITMAP(BITMAP &image, RGBQUAD *pPalette=NULL,BYTE *lpBmBits=NULL,BYTE* lpExterRawBitsBuff=NULL,UINT uiBitsBuffSize=0);
public:
	void UnloadRawBytesToVirtualMemBuff(BUFFER_IO* pIO,bool write2vm=true);
	bool LoadRawBytesFromVirtualMemBuff(BUFFER_IO* pIO);
	void UnloadGreyBytesToVirtualMemBuff(BUFFER_IO* pIO,bool write2vm=true);
	bool LoadGreyBytesFromVirtualMemBuff(BUFFER_IO* pIO);
	int m_nRectColNum,m_nRectRowNum;
	int m_nRectWidth,m_nRectHeight;
	void ReleaseRawImageBits();	//当m_lpRawBits占用新分配内存（与外部传入内存相对）时，释放内存以降低内存消耗
	void ReleaseGreyImageBits();
	UINT GetInternalAllocRawImageBytes();	//获取内部分配给m_lpRawBits内存大小
	//int GetRectPixelGrayThreshold(int iRow,int iCol);
	//void SetRectPixelGrayThreshold(int iRow,int iCol,int threshold);
	//根据统计数据决定黑白化图像时背景色灰度阈值
private:
	bool UpdateRowRectPixelGrayThreshold(int xiTopLeft,int yjTopLeft,int niRowRectWidth,int niRowRectHeight,int nMonoForward=20);
	void CalRectPixelGrayThreshold(int iRow,int iCol,int RECT_WIDTH,int RECT_HEIGHT,int nMonoForward=20);
public:
	bool IntelliRecogMonoPixelThreshold(int nMonoForward=20);
	bool LocalFiningMonoPixelThreshold(int xiTopLeft,int yjTopLeft,int nRgnWidth,int nRgnHeight,int nGridWidth=100,int nGridHeight=100);
	bool UpdateGreyImageBits(bool bUpdateMonoThreshold=false,int nMonoForward=20);		//更新保存灰度及黑白图像
public:
	CImageTransform();
	virtual ~CImageTransform();
	//
	LPBITMAPINFOHEADER GetBitmapInfoHeader(){return NULL;}//m_lpBMIH;}
	void SetBitMap(BITMAP& bmp);
	BYTE get_ciRawImageFileType(){return this->m_ciRawImageFileType;}
	__declspec(property(get=get_ciRawImageFileType)) BYTE ciRawImageFileType;
	UINT get_uiExterRawBitsBuffSize();
	__declspec(property(get=get_uiExterRawBitsBuffSize)) UINT uiExterRawBitsBuffSize;
	BYTE *GetRawBits(){return m_lpRawBits;}
	BYTE *GetGrayBits(){return m_lpGrayBitsMap;}
	UINT GetBitCount(){return m_uBitcount;}
	int GetWidth(){return m_nWidth;}
	int GetEffWidth(){return m_nEffByteWidth;}
	int GetHeight(){return m_nHeight;}
	int GetDrawingWidth(){return m_nDrawingRgnWidth;}
	int GetDrawingHeight(){return m_nDrawingRgnHeight;}
	PDF_FILE_CONFIG GetPdfConfig(){return m_xPdfCfg;}
	//
	bool Turn90(bool byClockwise);		//将文件中图像顺(逆)时针转90度
	BYTE CalGreynessThreshold();			//计算灰度阈值
	bool CalDrawingRgnRect();				//计算有效区域的矩形
	//定位有效区域（工艺卡图框）的矩形
	void LocateDrawingRgnRect(POINT leftU,POINT leftD,POINT rightU,POINT rightD);
	BYTE GetPixelGrayness(int x,int y);		//指定象素点的灰度值
	BYTE GetPixelGraynessThresold(int x,int y);	//指定像素点黑白阈值
	bool IsBlackPixel(int x,int y,int nMonoForwardBalance=0);	//指定象素点是否为黑点
	void ClearImage();
	bool ReadImageFile(FILE* fp,char ciBmp0Jpeg1Png2Tif3,BYTE* lpExterRawBitsBuff=NULL,UINT uiBitsBuffSize=0,int nMonoForward=20);
	bool ReadImageFile(const char* file_path,BYTE* lpExterRawBitsBuff=NULL,UINT uiBitsBuffSize=0,int nMonoForward=20);	//读取图片
	bool ReadPdfFile(char* file_path,const PDF_FILE_CONFIG& pdfConfig,int nMonoForward=20,RectD *pRect=NULL,BOOL bInitGrayBitMap=FALSE);
	bool RetrievePdfRegion(RectD *pRect,double zoom_scale,CImageTransform &image);
	bool TransPdfRegionCornerPoint(double zoom_scale,PointD &leftTop,PointD &leftBtm,PointD &rightTop,PointD &rightBtm);
	void SetPdfEngine(BaseEngine *pPdfEngine);
	bool ReadImageFileHeader(FILE* fp,char ciBmp0Jpeg1Png2);
	bool WriteImageFile(const char* file_path);	//保存图片
	bool GapDetect(int start_x,int start_y,int maxGap,bool bScanByRow,int *gaplen);	//出现断点后是否延续的判断
	bool RetrieveSpecifyRegion( double pos_x_coef,double pos_y_coef,double width_coef,double height_coef, 
		BYTE** outBitsMap,long *outWidth,long *effWidth,long *outHeight);	//识别指定区域
	POINT TransformPointToOriginalField(POINT p,TRANSFORM* pTransform=NULL);	//将指定区域点转换到原始图像中
	RECT TransformRectToOriginalField(POINT p,TRANSFORM* pTransform=NULL);	//将指定区域点转换到原始图像的指定矩形区域
	//int StretchBlt(HDC hDC,CRect imageShowRect,DWORD rop);
};
