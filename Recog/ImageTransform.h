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
	int m_nValidBlackSeg;	//�������ڼ�⵽��ɫ�߶ε�����
	//[i][0]��ʾ��i�ε���ʼ��λ��[i][1]��ʾ��i�ε���ֹ��λ
	int shiftSegArr[5][2];
	int m_iPosIndex;
public:
	CSamplePixelLine();
	~CSamplePixelLine();
	//pixels��true��ʾ�ڵ�;false��ʾ�׵�
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
//��ͼ���ɻҶ�ͼתΪ�ڰ׵�ɫͼ�Ļ�����ֵ�����࣬��ӦCImageDataRegion��
class CMonoThresholder
{
	bool m_bInitThresholder;
	BYTE *m_lpMonoThreshold;	//��¼ÿ�����غڰ׷ֽ�Ҷ�ֵ
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
	double sina,cosa;	//��ת�ǵ�����������
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
	int m_iMonoBalance;			//�ڰ�ƽ�����ֵ��+ֵ��ʾ��׵���, -ֵ��ʾ��ڵ���
	CFittingMap<XHPOINT> gridmap;	//����ӳ�������
	bool IsValid(){gridmap.RowCount>=2&&gridmap.ColCount>=2;}
	TRANSFORM(){m_iMonoBalance=0;}
};
class CImageTransform  
{
	//LPBITMAPINFOHEADER m_lpBMIH;	//ͼ��ı�ͷ��Ϣ������ɫ����������
	bool m_bInternalRawBitsBuff;	//True:�ڲ������ڴ棻False:m_lpRawBitsΪ�ⲿ���빲���ڴ棬�������ڲ��ͷ�
	DWORD m_uiExterRawBitsuffSize;
	BYTE *m_lpRawBits;				//��ɨ��(Downward top->bottom)��ʽ��λͼ�ֽ�����(���24λ��ʽ��������ǰͼ������,m_nEffByteWidth*m_nHeight
	BYTE *m_lpPixelGrayThreshold;	//��¼ÿ�����غڰ׷ֽ�Ҷ�ֵ
	BYTE *m_lpGrayBitsMap;			//���ֽڻҶ��͵�ͼ���������ݱ���Ҫ����ͼ����
	int m_nWidth,m_nHeight;			//ͼƬ�Ŀ�Ⱥ͸߶�(���ص�λ)
	int m_nEffByteWidth;			//ͼƬ�ж�����ʵ���ֽ�����4 byte���룩
	UINT m_uBitcount;				//ÿ����ʾ���ص���ɫ��ʾλ����1Ϊ��ɫ;4Ϊ16ɫ;8Ϊ256ɫ;16��24��32Ϊ���ɫ
	int m_nDrawingRgnWidth,m_nDrawingRgnHeight;//ͼƬ�Ŀ�Ⱥ͸߶�(���ص�λ)
	double kesai[4];				//��[4]�ı�������οռ�任�Ĳ���
	POINT m_leftUp,m_leftDown,m_rightUp,m_rightDown;	//���տ��߿���ĸ���
	BYTE m_ubGreynessThreshold;		//�ڽ��ж�ֵ��(�ڰ׶�ɫ)����ʱ����ֵ���Ҷȴ��ڴ�ֵΪ�׵㣬С�ڴ�ֵΪ�ڵ�
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
	void ReleaseRawImageBits();	//��m_lpRawBitsռ���·����ڴ棨���ⲿ�����ڴ���ԣ�ʱ���ͷ��ڴ��Խ����ڴ�����
	void ReleaseGreyImageBits();
	UINT GetInternalAllocRawImageBytes();	//��ȡ�ڲ������m_lpRawBits�ڴ��С
	//int GetRectPixelGrayThreshold(int iRow,int iCol);
	//void SetRectPixelGrayThreshold(int iRow,int iCol,int threshold);
	//����ͳ�����ݾ����ڰ׻�ͼ��ʱ����ɫ�Ҷ���ֵ
private:
	bool UpdateRowRectPixelGrayThreshold(int xiTopLeft,int yjTopLeft,int niRowRectWidth,int niRowRectHeight,int nMonoForward=20);
	void CalRectPixelGrayThreshold(int iRow,int iCol,int RECT_WIDTH,int RECT_HEIGHT,int nMonoForward=20);
public:
	bool IntelliRecogMonoPixelThreshold(int nMonoForward=20);
	bool LocalFiningMonoPixelThreshold(int xiTopLeft,int yjTopLeft,int nRgnWidth,int nRgnHeight,int nGridWidth=100,int nGridHeight=100);
	bool UpdateGreyImageBits(bool bUpdateMonoThreshold=false,int nMonoForward=20);		//���±���Ҷȼ��ڰ�ͼ��
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
	bool Turn90(bool byClockwise);		//���ļ���ͼ��˳(��)ʱ��ת90��
	BYTE CalGreynessThreshold();			//����Ҷ���ֵ
	bool CalDrawingRgnRect();				//������Ч����ľ���
	//��λ��Ч���򣨹��տ�ͼ�򣩵ľ���
	void LocateDrawingRgnRect(POINT leftU,POINT leftD,POINT rightU,POINT rightD);
	BYTE GetPixelGrayness(int x,int y);		//ָ�����ص�ĻҶ�ֵ
	BYTE GetPixelGraynessThresold(int x,int y);	//ָ�����ص�ڰ���ֵ
	bool IsBlackPixel(int x,int y,int nMonoForwardBalance=0);	//ָ�����ص��Ƿ�Ϊ�ڵ�
	void ClearImage();
	bool ReadImageFile(FILE* fp,char ciBmp0Jpeg1Png2Tif3,BYTE* lpExterRawBitsBuff=NULL,UINT uiBitsBuffSize=0,int nMonoForward=20);
	bool ReadImageFile(const char* file_path,BYTE* lpExterRawBitsBuff=NULL,UINT uiBitsBuffSize=0,int nMonoForward=20);	//��ȡͼƬ
	bool ReadPdfFile(char* file_path,const PDF_FILE_CONFIG& pdfConfig,int nMonoForward=20,RectD *pRect=NULL,BOOL bInitGrayBitMap=FALSE);
	bool RetrievePdfRegion(RectD *pRect,double zoom_scale,CImageTransform &image);
	bool TransPdfRegionCornerPoint(double zoom_scale,PointD &leftTop,PointD &leftBtm,PointD &rightTop,PointD &rightBtm);
	void SetPdfEngine(BaseEngine *pPdfEngine);
	bool ReadImageFileHeader(FILE* fp,char ciBmp0Jpeg1Png2);
	bool WriteImageFile(const char* file_path);	//����ͼƬ
	bool GapDetect(int start_x,int start_y,int maxGap,bool bScanByRow,int *gaplen);	//���ֶϵ���Ƿ��������ж�
	bool RetrieveSpecifyRegion( double pos_x_coef,double pos_y_coef,double width_coef,double height_coef, 
		BYTE** outBitsMap,long *outWidth,long *effWidth,long *outHeight);	//ʶ��ָ������
	POINT TransformPointToOriginalField(POINT p,TRANSFORM* pTransform=NULL);	//��ָ�������ת����ԭʼͼ����
	RECT TransformRectToOriginalField(POINT p,TRANSFORM* pTransform=NULL);	//��ָ�������ת����ԭʼͼ���ָ����������
	//int StretchBlt(HDC hDC,CRect imageShowRect,DWORD rop);
};
