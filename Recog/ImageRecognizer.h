#pragma once

#include "list.h"
#include "f_ent_list.h"	//以后应用CXhSimpleList完全取代
#include "HashTable.h"
#include "ArrayList.h"
#include "Buffer.h"
#include "TempFile.h"
#include "XhCharString.h"
#include "LogFile.h"
#include "ImageTransform.h"
#include "MemberProperty.h"
#include ".\Recog.h"
#include "SortFunc.h"

/////////////////////////////////////////////////////////////////////////
//工具类及工具函数
RECT RECT_C(int l=0, int t=0, int r=0, int b=0);
struct PRESET_OBJS2048{static const int COUNT=2048;};
template <class TYPE> class PRESET_ARRAY2048 : public PRESET_ARRAY<TYPE,PRESET_OBJS2048>{;};
template<class TYPE,class TYPE_PRESET_COUNT> struct MOVEWND_POOL
{
	TYPE DEFAULT_REDIRECT_ERROR_OBJ;
	WORD wiStartIndex,wiEndIndex;
	TYPE objs[TYPE_PRESET_COUNT::COUNT];
	MOVEWND_POOL(){wiStartIndex=wiEndIndex=0;memset(objs,0,sizeof(TYPE)*TYPE_PRESET_COUNT::COUNT);}
	TYPE& operator[](int index){
		if(index<wiStartIndex||index>wiEndIndex||index-wiStartIndex>=TYPE_PRESET_COUNT::COUNT)
			return DEFAULT_REDIRECT_ERROR_OBJ;
		return objs[index-wiStartIndex];
	}
	void Clear(){memset(objs,0,sizeof(TYPE)*TYPE_PRESET_COUNT::COUNT);}
};
template <class TYPE> class MOVEWND_POOL32 : public MOVEWND_POOL<TYPE,PRESET_OBJS32 >{};
template <class TYPE> class MOVEWND_POOL64 : public MOVEWND_POOL<TYPE,PRESET_OBJS64 >{};
template <class TYPE> class MOVEWND_POOL128: public MOVEWND_POOL<TYPE,PRESET_OBJS128>{};
/////////////////////////////////////////////////////////////////////////
static const BYTE DETECT_LEFT	= 0x01;	//X减小方向
static const BYTE DETECT_RIGHT	= 0x02;	//X增加方向
static const BYTE DETECT_UP		= 0x04;	//Y减小方向
static const BYTE DETECT_DOWN	= 0x08;	//Y增加方向
static const BYTE DETECT_X0		= 0x10;	//X保持不变
static const BYTE DETECT_Y0		= 0x20;	//Y保持不变
struct IImage{
	static const BYTE PIXEL_IS_BYTE = 0;//每个象素占1个Byte
	static const BYTE PIXEL_IS_BIT  = 1;//每个象素占1个Bit

	BYTE* m_lpBitMap;	//new BYTE[m_nEffWidth*m_nHeight]，行扫描downward (Top->Bottom)
	int m_nEffWidth;	//实际有效宽度
	int m_nWidth,m_nHeight;
public:
	IImage(){m_lpBitMap=NULL;m_nEffWidth=m_nWidth=m_nHeight=0;}
	virtual const BYTE* GetImageMap(){return m_lpBitMap;}
	virtual bool IsPixelBitMode()=0;	//True:每个象素占一行中的一个bit位;false占一个Byte字节
	virtual bool BlackBitIsTrue(){return true;}
	virtual bool IsBlackPixel(int i,int j);
	virtual bool SetPixelState(int i,int j,bool black=true);
	virtual int GetImageEffWidth(){return m_nEffWidth;}	//获取图像每行象素所占用的实际字节数
	virtual int GetImageWidth(){return m_nWidth;}		//获取图像每行的有效象素数
	virtual int GetImageHeight(){return m_nHeight;}		//获取图像每列的有效象素数
};
class DETECT_MAP{
	bool m_bExternalMemory;
	char* map;	//0.未探测到未初始化像素;'W'：已探测到的白点;其余值为已探测到的黑点
	int wDetectPixels;
public:
	int xiOrg,yiOrg;
	DETECT_MAP(WORD wDetectWidth,int iOrgX=0,int iOrgY=0,char* exterdata=NULL,UINT uiExterDataSize=0)
	{
		wDetectPixels=wDetectWidth;
		int width=wDetectPixels*2+1;
		UINT allocsize=width*width;
		if(exterdata!=NULL&&allocsize<=uiExterDataSize)
		{
			map=exterdata;
			m_bExternalMemory=true;
		}
		else
		{
			map=new char[allocsize];
			m_bExternalMemory=false;
		}
		memset(map,0,width*width);
		xiOrg=iOrgX;
		yiOrg=iOrgY;
	}
	~DETECT_MAP(){
		if(!m_bExternalMemory)
			delete []map;
	}
	bool IsDetect(int xi,int yi,char* pchPixel=NULL)
	{
		int width=wDetectPixels*2+1;
		xi+=wDetectPixels-xiOrg;
		yi+=wDetectPixels-yiOrg;
		if(xi<0||xi>=width||yi<0||yi>=width)
		{
			if(pchPixel)
				*pchPixel=0;
			return true;
		}
		else
		{
			if(pchPixel)
				*pchPixel=map[yi*width+xi];
			return map[yi*width+xi]!=0;
		}
	}
	bool SetDetect(int xi,int yi,char chDetectPixel=true)
	{
		int width=wDetectPixels*2+1;
		xi+=wDetectPixels-xiOrg;
		yi+=wDetectPixels-yiOrg;
		if(xi<0||xi>=width||yi<0||yi>=width)
			return false;
		else
			return (map[yi*width+xi]=chDetectPixel)!=0;
	}
};
template<class TYPE> class IMGPIXELS{
protected:
	long m_nRows,m_nCols,m_nActualElemSize;
	BYTE* data;
	bool m_bExternalMemory;	//下面data指向的数据区是否为外部分配的内存。若是不允许在内部释放
	struct ROW_DATA{
		TYPE defaultErrorObj;
		TYPE* rowdata;	//指向data的后缀附加内存，不需要独立分配和释放内存
		long liStartAddrIndex,liRowDataLength;
		ROW_DATA(BYTE* imgrowdata=NULL,long liBaseAddrIndex=0,long liRowLength=0)
		{
			rowdata=(TYPE*)imgrowdata;
			liStartAddrIndex=liBaseAddrIndex;
			liRowDataLength=liRowLength;
		}

		TYPE& operator[](long xI)	//index为像素的行索引值，即(X,Y)坐标中的X坐标，即（x,y)=[y][x]
		{
			if(xI<this->liStartAddrIndex||xI>=liStartAddrIndex+liRowDataLength)
				return defaultErrorObj;
			else
				return rowdata[xI-liStartAddrIndex];
		}
	};
	ROW_DATA* rowheader;
	virtual ROW_DATA GetRowData(long index)
	{	//index为像素的行索引值，即(X,Y)坐标中的Y坐标，即（x,y)=[y][x]
		if(index<0||index>=m_nRows)
			return ROW_DATA();
		return rowheader[index];
	}
public:
	//uiExterDataBytes为exterdata指向的外部存储空间的字节数
	IMGPIXELS(int width=0,int height=0,BYTE* exterdata=NULL,UINT uiExterDataBytes=0)
	{
		m_nRows=0;
		m_nCols=0;
		data=NULL;
		rowheader=NULL;
		m_bExternalMemory=false;
		if(width>0&&height>0)
			Resize(width,height,exterdata,uiExterDataBytes);
	}
	~IMGPIXELS()
	{
		if(data&&!m_bExternalMemory)
			delete []data;
		data=NULL;
	}
	void Resize(int width,int height,BYTE* exterdata=NULL,UINT uiExterDataBytes=0)
	{
		m_nRows=height;
		m_nCols=width;
		if(m_nRows<0)
			m_nRows=0;
		if(m_nCols<0)
			m_nCols=0;
		if((m_nActualElemSize=m_nRows*m_nCols)>0)
		{
			if(data!=NULL&&!m_bExternalMemory)
				delete []data;
			UINT uiAllocDataSize=m_nActualElemSize*sizeof(TYPE);
			UINT uiAllocSize=uiAllocDataSize+sizeof(ROW_DATA)*m_nRows;
			if(exterdata!=NULL&&uiAllocSize<=uiExterDataBytes)
			{
				data=exterdata;
				m_bExternalMemory=true;
			}
			else
			{
				data = new BYTE[uiAllocSize];
				m_bExternalMemory=false;
			}
			rowheader=(ROW_DATA*)(data+uiAllocDataSize);
			rowheader[0]=ROW_DATA(data,0,m_nCols);
			for(int j=1;j<m_nRows;j++)
			{
				rowheader[j]=rowheader[j-1];
				rowheader[j].rowdata+=m_nCols;
			}
		}
	}
	//index为像素的行索引值，即(X,Y)坐标中的Y坐标，即（x,y)=[y][x]
	ROW_DATA operator[](long index){return GetRowData(index);}
	READONLY_PROPERTY(long,ActualElemCount);	//返回二维数组中总计存储的TYPE类型的对象元素数
	GET(ActualElemCount){return m_nActualElemSize;}
	READONLY_PROPERTY(BYTE*,m_pData);
	GET(m_pData){return data;}
	READONLY_PROPERTY(long,ActualRowCount);
	GET(ActualRowCount){return m_nRows;}
	READONLY_PROPERTY(long,ActualColCount);
	GET(ActualColCount){return m_nCols;}
};
class CLIP_PIXELS : public IMGPIXELS<BYTE>{
protected:
	RECT rcClip;
	virtual ROW_DATA GetRowData(long index);
public:
	CLIP_PIXELS(RECT* prcClip=NULL,BYTE* exterdata=NULL,UINT uiExterDataBytes=0);
	CLIP_PIXELS(int width,int height,BYTE* exterdata=NULL,UINT uiExterDataBytes=0);
	~CLIP_PIXELS();
	static UINT CalAllocSize(int width,int height);
	void Resize(const RECT& rcClip,BYTE* exterdata=NULL,UINT uiExterDataBytes=0);
	READONLY_PROPERTY(RECT,Clipper);
	GET(Clipper){return rcClip;}
};
struct IImageNoiseDetect : public IImage{
//	static const BYTE DETECT_LEFT	= 0x01;
//	static const BYTE DETECT_RIGHT	= 0x02;
//	static const BYTE DETECT_UP		= 0x04;
//	static const BYTE DETECT_DOWN	= 0x08;
//public:
	//maxIslandBlackPixels：(x=i,y=j)作为孤岛识别条件的最多黑点像素数
	virtual bool IsNoisePixel(int i,int j,RECT* pValidRgn=NULL,int maxIslandBlackPixels=5,BYTE cDetectFlag=0x0f,DETECT_MAP* pDetectMap=NULL);
	virtual long RemoveNoisePixels(RECT* pValidRgn=NULL,int maxIslandBlackPixels=5);
protected:
	struct PIXEL_XY{
		int x,y;
		PIXEL_XY(int _x=0,int _y=0){x=_x;y=_y;}
	};
	virtual int StatNearBlackPixels(int i,int j,DETECT_MAP* pDetectMap,RECT* pValidRgn=NULL,PRESET_ARRAY16<PIXEL_XY>* listStatPixels=NULL,int maxBlackPixels=5,BYTE cDetectFlag=0x0f);
};
struct CBitImage : public IImageNoiseDetect{
	CBitImage(int width=0,int height=0,bool blackbit_is_true=false);
	CBitImage(BYTE* lpBitMap,int width,int height,int nEffWidth,bool blackbit_is_true=false);
	CBitImage(IMAGE_DATA& imgdata,bool blackbit_is_true=false);
	~CBitImage();
	virtual void InitBitImage(BYTE* lpBitMap,int width,int height,int nEffWidth);
	virtual bool BlackBitIsTrue(){return m_bBlackPixelIsTrue;}
	virtual bool IsPixelBitMode(){return true;}	//True:每个象素占一行中的一个bit位;false占一个Byte字节
	virtual bool IsBlackPixel(int i,int j);
	virtual void SetClipRect(const RECT& rc){clip=rc;}
	virtual RECT GetClipRect(){return clip;}
protected:
	RECT clip;	//有效的裁剪窗口
	bool m_bBlackPixelIsTrue;	//像素比特位为True表示黑点
	bool m_bExternalData;
};
class CMonoImage : public IMonoImage{
	long m_idSerial;
	CBitImage image;
public:
	CMonoImage(long serial=0);
	~CMonoImage();
	DWORD SetKey(DWORD key){return m_idSerial=key;}
	virtual long Serial();
	virtual bool IsNoisePixel(int i,int j,RECT* pValidRgn=NULL,int maxIslandBlackPixels=5);
	virtual long RemoveNoisePixels();
	virtual bool InitImageSize(int width=0,int height=0);
	virtual bool IsBlackPixel(int i,int j);
	virtual bool SetPixelState(int i,int j,bool black=true);
	virtual long GetImageWidth();	//获取图像每行的有效象素数
	virtual long GetImageHeight();	//获取图像每列的有效象素数
};
struct IImageZoomTrans{
protected:
	double minCoefDest2Src;
	double xCoefDest2Src,yCoefDest2Src,xOffsetOrg,yOffsetOrg;
	RECT _src,_dest;
public:
	virtual bool InitImageZoomTransDataByCenter(const RECT& src,const RECT& dest);
	virtual bool InitImageZoomTransData(const RECT& src,const RECT& dest);
	virtual POINT TransFromDestImage(int xiDest,int yjDest);
	virtual bool IsBlackPixelAtDest(int xiDest,int yjDest,IImage* pSrcImage);
};

struct MATCHCOEF{
	double original;	//修正前原始加权匹配系数
	double weighting;	//修正后（描粗笔划后反向统计未覆盖黑点数量）匹配系数 wjh-2018.3.22
	MATCHCOEF(double originalCoef=0.0,double weightingCoef=0.0);
	int CompareWith(const MATCHCOEF& coef);
};
//笔划位图,白(0x00)表示底色,黑(0x01)表示用于笔划像素统计基础像素
//扩展笔画位图,灰色(0x03)正向统计像素，红色(0x02)表示反向统计像素，均不讲稿笔划像素基数
class CImageChar;
struct STROKELINE{
	BYTE cnBitCount;
	union DATA{
		BYTE bytes[8];
		DWORD dword;
		__int64 i64;
	}bits;
	//cnStatBits：由高位向低位统计的bit位数
	static BYTE StatTrueBitCount(BYTE ciByte,short siStatBits=8);
	STROKELINE(const BYTE* pBytes=NULL,BYTE _ciBitOffset=0,BYTE _cnBitCount=0);
	void InitBits(const BYTE* pBytes,BYTE _ciBitOffset,BYTE _cnBitCount);
	BYTE StatMatchBitCountWith(STROKELINE& strokeline);
	bool SetPixelState(int i,bool state=true);
	bool GetPixelState(int i);
};
class CStrokeFeature : public IImage, public IStrokeFeature
{	//笔划位图,白(0x00)表示底色,黑(0x01)表示用于笔划像素统计基础像素
	UINT m_id;
protected:
	BYTE* m_lpBitMap;	//new BYTE[m_nEffWidth*m_nHeight]，行扫描downward (Top->Bottom)
	int m_nEffWidth;	//实际有效宽度
	int m_nWidth,m_nHeight;
	PRESET_ARRAY32<STROKELINE> blckrows;	//每位为1时表示为黑或灰点（即字符图你中进行正向统计的黑点区域）
	PRESET_ARRAY32<STROKELINE> redrows;	//每位为1时表示为红点（即字符图像中禁止为黑点的区域）
	static int PixelRowBytesToBits(const BYTE* rowbytes,BYTE* rowbits,int width);
public:
	CStrokeFeature();
	~CStrokeFeature();
	virtual const BYTE* GetImageMap(){return m_lpBitMap;}
	virtual BYTE PixelBitCount(){return 2;}	//True:每个象素占一行中的一个bit位;false占一个Byte字节
	virtual bool BlackBitIsTrue(){return true;}
	virtual bool IsBlackPixel(int i,int j);
	virtual bool SetPixelState(int i,int j,bool black=true);
	virtual bool IsPixelBitMode(void){return false;}
	virtual int GetImageEffWidth(){return m_nEffWidth;}	//获取图像每行象素所占用的实际字节数
	virtual int GetImageWidth(){return m_nWidth;}		//获取图像每行的有效象素数
	virtual int GetImageHeight(){return m_nHeight;}		//获取图像每列的有效象素数
public:
	WORD m_wiSumBlackPixels;	//笔划中黑色像素的统计总(基)数
	RECT rcSearch;		//特征的搜索范围（相对于40*40像素的字符）
	static const char STATE_UPPER  =0x01;	//上半部
	static const char STATE_DOWNER =0x02;	//下半部
	static const char STATE_MIDDLE =0x03;	//中间
	static const char STATE_EXCLUDE=  -1;	//互斥特征
	struct LIVESTATE{
		WORD wChar;
		char ciState;	//该特征在指定字符中的存在状态,0:无约定;>0在字符指定部位;<0不允许在该字符出现
		void SetKey(DWORD key){wChar=(WORD)key;}
	};
	CHashListEx<LIVESTATE> hashStateInChars;
	void SetKey(DWORD key){m_id=key;}
	struct CLR{
		static const BYTE WHITE = 0x00;	//0:白色;
		static const BYTE BLACK = 0x01;	//0x01:黑色;
		static const BYTE RED   = 0x02;	//0x02:红色(禁止色);
		static const BYTE GRAY  = 0x03;	//0x03:灰色(正向统计色)
	};
	bool SetPixelColor(int i,int j,BYTE ciPixelClrIndex=1);//0:白色;0x01:黑色;0x02:红色(禁止色);0x03:灰色(正向统计色)
	BYTE GetPixelColor(int i,int j);
	bool IsGreyPixel(int i,int j);	//若为黑色进行正向统计的像素
	bool IsRedPixel(int i,int j);	//禁止为黑色的像素
	bool InitTurboRecogInfo();
	bool RecogInImageChar(CImageChar* pImgChar,BYTE ciMinPercent=70);
	void ToBuffer(CBuffer &buffer,DWORD dwVersion=0,BOOL bIncSerial=TRUE);
	void FromBuffer(CBuffer &buffer,DWORD dwVersion=0,BOOL bIncSerial=TRUE);

	virtual UINT GetId(){return m_id;}
	__declspec(property(get=GetId)) UINT Id;
	virtual char SetLiveStateInChar(WORD wChar,char ciLiveState);	//该特征在指定字符中的存在状态,0:无约定;>0在字符指定部位;<0不允许在该字符出现
	virtual char LiveStateInChar(WORD wChar);	//该特征在指定字符中的存在状态,0:无约定;>0在字符指定部位;<0不允许在该字符出现
	virtual void AddSearchRegion(RECT& rcSearch);
	virtual bool FromBmpFile(const char* szStrokeBmpFile);
	virtual UINT ToBLOB(char* blob,UINT uBlobBufSize,DWORD dwVersion=0);
	virtual void FromBLOB(char* blob,UINT uBlobSize,DWORD dwVersion=0);
};
class CImageChar : public IImageZoomTrans, public IImage
{
protected:	//识别出来的一些字符关键特征
	WORD wTopBlackPixels;	//顶部20%的黑点数量，用于识别'7',排除'1'
	WORD wBtmBlackPixels;	//底部20%的黑点数量，用于识别'2',排除'7'
	RECT rcFeatureActual;	//黑点的实际包络矩形区，用于识别'1','-','_','.'
public:
	RECT get_rcActual(){return rcFeatureActual;}
	__declspec(property(get=get_rcActual)) RECT rcActual;
	struct ISLAND{
		//bool boundary;
		WORD maxy,miny;
		double x,y,count;
		ISLAND(){Clear();}
		void Clear(){x=y=count=maxy=miny=0;}//boundary=false;}
	};	//内部空白孤岛特征区域
	struct PIXEL{
		BYTE *pcbPixel;
		PIXEL(BYTE* _pcbImgPixel=0){pcbPixel=_pcbImgPixel;}
		bool get_Black();
		bool set_Black(bool black);
		char set_Connected(char connected);		//该像素是否为连通区域的像素
		char get_Connected();		//该像素是否为连通区域的像素
		const static char NONEVISIT	=0;	//还未访问到该像素
		const static char CONNECTED	=1;	//与边界连通的像素
		const static char UNDECIDED	=2;	//已检测到该像素，但状态未判定
		__declspec(property(put=set_Black,get=get_Black)) bool Black;
		__declspec(property(put=set_Connected,get=get_Connected)) char Connected;
	};
	int DetectIslands(CXhSimpleList<ISLAND>* listIslands);
	WORD StatLocalBlackPixelFeatures();
protected:
	int m_nPixels;
	//BYTE* m_lpBitMap;	//共m_nPixels个，每个字节表示1个象素, 0表示白点;1表示黑点
	int _nDestValidWidth;	//缩放转换到目标图像后的有效宽度
	bool IsDetect(int i,int j);
	bool SetDetect(int xi,int yi,bool detect=true);
	void ClearDetectMark();
	bool StatNearWhitePixels(int i,int j,ISLAND* pIsland,PRESET_ARRAY2048<PIXEL>* listStatPixels,BYTE cDetectFlag=0x0f,CLogFile* pLogFile=NULL);
public:
	bool m_bTemplEnabled;
	XHWCHAR wChar;
	int m_nBlckPixels;	//字符黑点数，在Localize()、Standardize()、StandardLocalize()函数中初始化
	int xiImgLeft,xiImgRight;	//该单元格字符位于原始单元格提取出的数据区图像的起始像素及终止像素
	void SetKey(DWORD key){wChar.wHz=(WORD)key;}
	bool IsCharX(){return wChar.wHz=='x'||wChar.wHz=='X';}
	static const WORD CHAR_FAI=0xb5a6,CHAR_FAI2=0xd5a6;	//Φ(GB2312:0xb5a6,46502)φ(GB2312:0xd5a6,54694)
	bool IsCharFAI(){return wChar.wHz==CHAR_FAI||wChar.wHz==CHAR_FAI2;}
	bool IsSoltTypeChar(){return wChar.wHz=='['||wChar.wHz=='C';}
	bool IsPlateTypeChar(){return wChar.wHz=='-'||wChar.wHz=='_';}
	bool IsAngleTypeChar(){return wChar.wHz=='L';}
	bool IsPartTypeChar(){return IsCharFAI()||IsSoltTypeChar()||IsPlateTypeChar()||IsAngleTypeChar();}
	bool IsMaterialChar(){ return CImageChar::IsMaterialChar(wChar.wHz);}
	static bool IsMaterialChar(WORD wChar){ return wChar=='S'||wChar=='H'||wChar=='P'||wChar=='E';}
	bool IsPartLabelPostfix(){return wChar.wHz=='A'||wChar.wHz=='B'||wChar.wHz=='C';}
	//RECT rect;	//初始化时原图像中的截取矩形(不含rect.right及rect.bottom边)
	//int m_nMaxTextH;
	READONLY_PROPERTY(int,m_nMaxTextH);
	GET(m_nMaxTextH){return m_nHeight;}
public:
	READONLY_PROPERTY(int,m_nImageW);
	GET(m_nImageW){return m_nWidth;}
	READONLY_PROPERTY(int,m_nImageH);
	GET(m_nImageH){return m_nHeight;}
	READONLY_PROPERTY(int,m_nDestValidWidth);
	GET(m_nDestValidWidth){return _nDestValidWidth;}
	//后续附加其它字体的字符
protected:
	WORD _serial;	//字体标识序号
public:
	void set_FontSerial(WORD fontserial){_serial=fontserial;}
	WORD get_FontSerial(){return _serial;}	//字体标识序号
	__declspec(property(get=get_FontSerial)) WORD FontSerial;
	CImageChar* linknext;
	CImageChar* AppendFontChar(WORD fontserial=0);	//fontserial=0表示自动累加一个不同的字体标识，否则追加指定字体标识
	bool RemoveChild();
	CImageChar* FontChar(WORD fontserial);
public:
	CImageChar(WORD fontserial=0);
	~CImageChar(); 
	virtual bool IsPixelBitMode(){return false;}	//True:每个象素占一行中的一个bit位;false占一个Byte字节
	virtual bool IsBlackPixel(int i,int j);
	void CopyImageChar(CImageChar* pChar);
	void InitTemplate(BYTE* lpBits,int eff_width,int width,RECT rect);
	void Localize(BYTE* lpBits,int eff_width,RECT rect);
	void Standardize(WORD wStdWidth,WORD wStdHeight);
	void StandardLocalize(BYTE* lpBits,bool blackbit_is_true,int eff_width,RECT& rc,int wStdHeight,bool onlyHighestBand=true);
	int CheckFeatures(CXhSimpleList<ISLAND>* pIslands=NULL);	//-1：确定不符合特征；0.不能确定是否符合特征；1.确定符合特征
	MATCHCOEF CalMatchCoef(CImageChar* pText,char* pciOffsetX=NULL,char* pciOffsetY=NULL);
	void ToBuffer(CBuffer &buffer);
	void FromBuffer(CBuffer &buffer,long version);
};
typedef CImageChar* CImageCharPtr;
//////////////////////////////////////////////////////////////////////////
// CImageDataRegion
class CAlphabets;
union WARNING_LEVEL{
	
	UINT uiWarning;
	static const WORD ABNORMAL_CHAR_EXIST = 0x01;	//存在不可识别的异常字符
	static const WORD ABNORMAL_TXT_REGION = 0x02;	//提取的单元格文本区宽高异常
	static const WORD LOWRECOG_CHAR_EXIST = 0x04;	//存在匹配度较低的字符
	static BYTE MAX_WARNING_ORIGIN_MATCH_COEF/*=65*/;
	static BYTE MAX_WARNING_WEIGHT_MATCH_COEF/*=80*/;
	struct{
		BYTE matchLevel[2];	//匹配识别度:vfOrgMatchCoef=matchLevel[0]*0.01,vfWeightMatchCoef=matchLevel[1]*0.01;
		WORD wWarnFlag;		//识别过程中的异常情况
	};
	BYTE WaringLevelNumber();//0~255,越高表示警示程度越高
	void VerifyMatchCoef(const MATCHCOEF& matchcoef);
	void SetAbnormalCharExist(){wWarnFlag=0x01;}
	void SetAbnormalTextRegion(){wWarnFlag=0x02;}
};
struct CELLTYPE_FLAG{
	BYTE cbFlag;
	CELLTYPE_FLAG(BYTE flag=0){cbFlag=flag;}
	CELLTYPE_FLAG(BYTE textTypeIdOrFlag,bool byTextTypeId);
	operator BYTE(){return cbFlag;}
	BYTE AddCellTextType(int idCellTextType=0);
	bool IsHasCellTextType(int idCellTextType);
};
class CImageCellRegion : public CBitImage
{
public:
	static const int PARTNO		= 1;
	static const int GUIGE		= 2;
	static const int LENGTH		= 3;
	static const int NUM		= 4;
	static const int PIECE_W	= 5;
	static const int SUM_W		= 6;
	static const int NOTE		= 7;
private:
	int m_nIterDepth;
	ATOM_LIST<CImageChar> listChars;
	int m_nPreSplitZeroCount;
	IMAGE_CHARSET_DATA::CHAR_IMAGE preSplitLChar,preSplitQChar,preSplitXChar;
	IMAGE_CHARSET_DATA::CHAR_IMAGE preSplitZeroCharArr[4];	//提前识别的字符，最多会出现4个0（Q420L100X10） wht 18-07-01
	bool ProcessBriefMaTemplChar(CImageChar *pTemplChar,CImageChar *pPartTypeChar,
								 CImageChar *pFirstWidth,int nWidthCharCount,
								 CImageChar *pFirstPlateThickChar,int nThickCharCount);
public:
	struct CHAR_SECT{
		BYTE ciMatchPercentW;	//匹配度百分数(加权后匹配度）
		BYTE ciMatchPercentO;	//匹配度百分数（原始匹配度）
		BYTE cnLeftBlckPixels,cnRightBlckPixels;	//右分界线黑色像素数
		WORD wChar;	//可表示汉字
		WORD xiStart,xiEnd;
		CHAR_SECT(WORD iStart=0,WORD iEnd=0){
			xiStart=iStart;
			xiEnd=iEnd;
			cnLeftBlckPixels=cnRightBlckPixels=0;
			wChar=ciMatchPercentW=ciMatchPercentO=0;
		}
	};
	int ParseImgCharRects(BYTE cbTextTypeFlag=0,IXhList<CHAR_SECT>* pCharSectList=NULL,
		WORD iBitStart=0,WORD iBitEnd=0xffff);
	void ClearPreSplitChar();
public:
	bool m_bStrokeVectorImage;			//笔画矢量图生成的图像，这种图像几乎不存在失真，故最大程度保留了各字符的特征信息，适用于特征识别优先 wjh-2018.8.31
	int m_nDataType;
	int m_nLeftMargin,m_nRightMargin;	//有效区域与单元格左\右侧边界线距离，用来处理表格线压字问题 wht 18-07-30
	int m_nMaxCharCout;
	BYTE m_biStdPartType;
	WARNING_LEVEL xWarningLevel;
	CImageCellRegion();
	~CImageCellRegion();
	//
	CAlphabets* get_Alphabets() const;
	__declspec(property(get=get_Alphabets)) CAlphabets* Alphabets;
public:
	CXhChar50 SpellCellStrValue();
	virtual bool IsPixelBitMode(){return true;}	//True:每个象素占一行中的一个bit位;false占一个Byte字节
	//virtual bool IsBlackPixel(int i,int j);
	BOOL IsSplitter(WORD movewnd[5],double lessThanSplitPixels=2);
	int RetrieveMatchChar(WORD iBitStart,WORD& wChar,MATCHCOEF* pMatchCoef=NULL,double min_recogmatch_coef=0.6);
	int RetrieveMatchChar(WORD iBitStart,WORD iSplitStart,WORD iSplitEnd,WORD& wChar,MATCHCOEF* pMatchCoef=NULL,double min_recogmatch_coef=0.6);
	void UpdateFontTemplCharState();
	void SplitImageCharSet(WORD iBitStart=0,WORD wGuessCharWidth=0,bool bIgnoreRepeatFlag=false,
		IMAGE_CHARSET_DATA* imagedata=NULL,BYTE cbTextTypeFlag=0);
#ifdef _DEBUG
	void PrintCharMark();
#endif
};
class CImageDataFile;
class BOM_PART
{
protected:
	long nPart;		//一个塔型中具有的当前编号单段（基）构件数量
public:
	static const BYTE OTHER = 0;//其余非常规构件
	static const BYTE BOLT  = 1;//螺栓
	static const BYTE ANGLE = 2;//角钢
	static const BYTE PLATE = 3;//钢板
	static const BYTE TUBE  = 4;//钢管
	static const BYTE FLAT  = 5;//扁钢
	static const BYTE SLOT  = 6;//槽钢
	static const BYTE ROUND = 7;//圆钢
	static const BYTE FITTINGS = 8;//附件
	//构件子类型
	const static BYTE SUB_TYPE_TUBE_MAIN	= 1;	//主管
	const static BYTE SUB_TYPE_TUBE_BRANCH	= 2;	//支管
	const static BYTE SUB_TYPE_PLATE_C		= 3;	//槽型插板
	const static BYTE SUB_TYPE_PLATE_U		= 4;	//U型插板
	const static BYTE SUB_TYPE_PLATE_X		= 5;	//十字插板
	const static BYTE SUB_TYPE_PLATE_FL		= 6;	//法兰
	const static BYTE SUB_TYPE_PLATE_WATER	= 7;	//遮水板
	const static BYTE SUB_TYPE_FOOT_FL		= 8;	//底脚法兰
	const static BYTE SUB_TYPE_ROD_Z		= 9;	//主材
	const static BYTE SUB_TYPE_ROD_F		= 10;	//辅材
	const static BYTE SUB_TYPE_ANGLE_SHORT	= 11;	//短角钢
	const static BYTE SUB_TYPE_PLATE_FLD	= 12;	//对焊法兰
	const static BYTE SUB_TYPE_PLATE_FLP	= 13;	//平焊法兰
	const static BYTE SUB_TYPE_PLATE_FLG	= 14;	//刚性法兰
	const static BYTE SUB_TYPE_PLATE_FLR	= 15;	//柔性法兰
	const static BYTE SUB_TYPE_BOLT_PAD		= 16;	//螺栓垫板

	char cPartType;
public:
	UINT id;			//标识号
	RECT rc;			//该行构件在CImageDataRegion中对应的矩形区域
	CXhChar16 sPartNo;	//构件编号
	int iSeg;			//段号
	short siSubType;	//
	char cMaterial;		//材质简化字符，如：'S','H','G'
	double wide;		//构件宽度参数
	double thick;		//构件厚度参数
	double wingWideY;	//角钢Y肢宽度
	CXhChar16 sSpec;	//规格
	double length;		//构件长度参数
	double fPieceWeight;//单件重(kg)，对于钢板板为最小包容矩形重量
	double fSumWeight;	//总重
	short siZhiWan;		//>0：制弯次数，<=0不需要制弯
	BOOL bWeldPart;		//是否需要焊接
	BOOL bLegPart;		//是否为腿部构件
	short nM16Ls;	//M16螺栓孔数
	short nM20Ls;	//M20螺栓孔数
	short nM24Ls;	//M24螺栓孔数
	short nMXLs;	//其它直径螺栓孔数
	short nMSumLs;	//螺栓孔总数
	char sNotes[50];	//构件备注说明
	long feature1;		//特征属性1，临时使用
	long feature2;		//特征属性2，临时使用
	bool bStdPart;		//标准件
	BYTE arrItemWarningLevel[7];//0.件号;1.材质&规格;2.长度;3.单基数;4.单重;5.总重;6.备注 列的警示信息，越高越需要警示
	//螺栓信息
	//ARRAY_LIST<BOMBOLT_RECORD> m_arrBoltRecs;
public:
	BOM_PART(){cPartType=0;nPart=0;bStdPart=FALSE;wide=thick=wingWideY=length=fPieceWeight=fSumWeight=0;}
	~BOM_PART(){memset(arrItemWarningLevel,0,7);}
	void SetKey(DWORD key){id=key;}
	long AddPart(int add_num=1){nPart+=add_num;return nPart;}
	long GetPartNum(){return nPart;}
	/*int GetBoltOrHoleNum(double min_d,double max_d=-1,BOOL bHoleD=TRUE);

	virtual short CutAngleN(){return 0;}
	virtual CXhChar16 GetSpec(){ return sSpec;}
	virtual short GetHuoquLineCount();
	virtual int GetLsHoleStr(char *hole_str,BOOL bIncUnit=TRUE);	//螺栓孔字符串
	virtual CXhChar16 GetPartTypeName();
	virtual CXhChar16 GetPartSubTypeName();
	virtual void FromBuffer(CBuffer &buffer,long version=NULL);
	virtual void ToBuffer(CBuffer &buffer,long version=NULL);*/
};
typedef struct tagCELL_RECT
{
	int yTop,yBtm;
	//
	RECT rect;
	tagCELL_RECT(){
		rect.top=rect.bottom=rect.left=rect.right=0;
		yTop=yBtm=0;
	}
	tagCELL_RECT(RECT rc){
		rect=rc;
		yTop=yBtm=0;
	}
	void set_RectLeft(int left){rect.left=left;}
	long get_RectLeft(){return rect.left;}
	void set_RectRight(int right){rect.right=right;}
	long get_RectRight(){return rect.right;}
	void set_RectTop(int top){rect.top=top;}
	long get_RectTop(){return rect.top;}
	void set_RectBottom(int bottom){rect.bottom=bottom;}
	long get_RectBottom(){return rect.bottom;}
	__declspec(property(put=set_RectLeft,get=get_RectLeft)) long left;
	__declspec(property(put=set_RectRight,get=get_RectRight)) long right;
	__declspec(property(put=set_RectTop,get=get_RectTop)) long top;
	__declspec(property(put=set_RectBottom,get=get_RectBottom)) long bottom;
}CELL_RECT;
class CImageDataRegion : public IImageNoiseDetect,public IImageRegion
{
	ARRAY_LIST<int> m_arrColCellsWidth;
public:
	struct PIXELLINE
	{
		int iStart;
		int nPixell;
		PIXELLINE(){iStart=nPixell=0;}
	};
	TRANSFORM transform;
	ARRAY_LIST<int> m_xarrBtmLinePosYOfRow,m_xarrRightLinePosXOfCol;
public:
	static int m_nInitMonoForwardPixels;//=20;	//记录前一次区域平衡值，新建区域时用来初始化新区域的平衡值 wht 18-07-22
private:
	union IJ{
		DWORD ij;
		struct{WORD i,j;};
		operator DWORD(){return ij;}
		IJ(WORD _i=0,WORD _j=0){i=_i;j=_j;}
	};
	long FillRgnPixel(int i,int j,RECT& rgn,DWORD crColorFrom,DWORD crColorTo);
private:
	CHashList<bool> hashImgShieldLayer;
	CImageDataFile *m_pImageData;
	int m_nMonoForwardPixels;		//在CImageDataFile基础上针对某一区域的划分黑白点间的界限调整平衡调整值
	POINT leftUp,leftDown,rightUp,rightDown;					//在图纸上框选的区域
	POINT realLeftUp,realLeftDown,realRightUp,realRightDown;	//放大后的PDF区域矩形框 wht 18-07-10
	virtual bool IsPixelBitMode(){return true;}	//True:每个象素占一行中的一个bit位;false占一个Byte字节
	virtual bool BlackBitIsTrue(){return false;}
	virtual bool IsBlackPixel(int i,int j);
	virtual bool SetPixelState(int i,int j,bool black=true);
	bool IsBlackPixel(int i,int j,BYTE* lpBits,WORD wBitWidth);

	void CalCellInternalRect(RECT& rcCellWithFrame,double vfScaleCoef,LONG& xStart,LONG& yStart,LONG& xEnd,LONG& yEnd);
	void CalCellTextRect(RECT& rcCellNoneFrame,double vfScaleCoef,LONG& xStart,LONG& yStart,LONG& xEnd,LONG& yEnd,int iDataType);
	int DetectConnBorderX(int xi,int xiDetectDepth,int yjTop,int yjBtm,bool* xarrColState=NULL,bool bToLeftTrueRightFalse=true);
	int CalMaxPixelsIndex(int start,int end,BOOL bLevel,IMAGE_DATA *pImageData=NULL);
	int StatColLinePixeCount(int start,int end,int iCol);
	BOOL AnalyzeSampleImage(int sample_width,int sample_height,int &rowHeight);
	BOOL AnalyzeLinePixels(int& start,int& end,BOOL bLevel,IMAGE_DATA *pImageData=NULL);
	bool IntelliRecogCornerPoint(const POINT& xPickPoint,const POINT& xTopLeft2Pick,POINT* pCornerPoint,WORD wiDetectWidth=100,WORD wiDetectHeight=100,char ciDetectRule='Y');
	int  RecognizeLines(ATOM_LIST<int> &lineStartList,BOOL bLevel,IMAGE_DATA *pImageData=NULL);
	void RetrieveCellRgn(CImageCellRegion& cell,RECT rect);
	//按材料汇总表模式提取	 wjh-2018.11.13
	int RecognizeSummaryBOM(DYN_ARRAY<CELL_RECT> &rowCells,BYTE *pWarningLevel=NULL);
	//按分段构件明细表模式提取
#ifdef _DEBUG
	int RecognizeSingleBomPart(DYN_ARRAY<CELL_RECT> &rowCells,BOOL bTestRecog=FALSE,BYTE *pWarningLevel=NULL,int iCellCol=-1,bool bExportLogFile=false);
#else
	int RecognizeSingleBomPart(DYN_ARRAY<CELL_RECT> &rowCells,BOOL bTestRecog=FALSE,BYTE *pWarningLevel=NULL);
#endif
public:
	bool RecognizeSampleCell(IMAGE_DATA *pImageData,RECT* prcText);
	void CalCharValidRect(RECT data_rect,int iDataType,LONG& xStart,LONG& yStart,LONG& xEnd,LONG& yEnd);
public:
	DWORD m_dwKey;
	CHashListEx<BOM_PART> hashBomParts;
#ifdef _DEBUG
	static bool EXPORT_CHAR_RECOGNITION;
#endif
public:	//与宿主程序中窗口显示状态相关的属性
	double m_fDisplayAllRatio;		//记录图像全显比例，部分显示时支持拖动
	double m_fDisplayRatio;			//记录图像缩放系数，绘制图片时使用
	int m_nOffsetX,m_nOffsetY;		//记录图片偏移位置，绘制图片时使用
public:
	CImageDataRegion(DWORD key=0);
	~CImageDataRegion();
	void SetKey(DWORD key){m_dwKey=key;}
#ifdef _DEBUG
	CXhChar50 RecognizeDatas(CELL_RECT &data_rect,int idCellTextType,IMAGE_CHARSET_DATA* imagedata=NULL,BYTE* pcbWarningLevel=NULL,bool bExportImgLogFile=false,BYTE biStdPart=0);
#else
	CXhChar50 RecognizeDatas(CELL_RECT &data_rect,int idCellTextType,IMAGE_CHARSET_DATA* imagedata=NULL,BYTE* pcbWarningLevel=NULL,BYTE biStdPart=0);
#endif
	void SetBelongImageData(CImageDataFile *pImageData){m_pImageData=pImageData;}
	void ParseDataValue(CXhChar50 sText,int data_type,char* sValue1,char* sValue2=NULL,int cls_id=0);
	void ToBuffer(CBuffer &buffer);
	void FromBuffer(CBuffer &buffer,long version);
public:	//调整灰度图转为黑白图分界线阈值的相关变量及函数
	virtual double SetMonoThresholdBalanceCoef(double mono_balance_coef=0);
	virtual double GetMonoThresholdBalanceCoef();
	int get_MonoForwardPixels(){return this->m_nMonoForwardPixels;}
	//__declspec(property(get=get_MonoForwardPixels)) int MonoForwardPixels;
	//balancecoef 划分黑白点间的界限调整平衡系数,取值-0.5~0.5;0时对应最高频率像素前移20位
public:
	virtual long GetSerial(){return m_dwKey;}
	virtual IImageFile* BelongImageData() const;
	virtual POINT TopLeft(bool bRealRegion=false);
	virtual POINT TopRight(bool bRealRegion=false);
	virtual POINT BtmLeft(bool bRealRegion=false);
	virtual POINT BtmRight(bool bRealRegion=false);
	virtual bool SetRegion(POINT topLeft,POINT btmLeft,POINT btmRight,POINT topRight,bool bRealRegion=false,bool bRetrieveRegion=true);
	virtual void UpdateImageRegion();
	virtual void InitImageShowPara(RECT showRect);
	virtual int GetMonoImageData(IMAGE_DATA* imagedata);	//每位代表1个像素，0表示白点,1表示黑点
	virtual int GetRectMonoImageData(RECT* rc,IMAGE_DATA* imagedata);	//不包含rc->right列及rc->bottom行
	virtual int GetImageColumnWidth(int* col_width_arr=NULL);		//返回列数
	virtual int GetHeight(){return m_nHeight;}
	virtual int GetWidth(){return m_nWidth;}
	virtual void SetScrOffset(SIZE offset);
	virtual SIZE GetScrOffset();
	virtual bool IsCanZoom(){return m_fDisplayRatio<=min(1,m_fDisplayAllRatio);}
	virtual double Zoom(short zDelta,POINT* pxScrPoint=NULL);
	virtual double ZoomRatio(){return m_fDisplayRatio;}	//缩放比例=屏幕尺寸/原始图像尺寸
	virtual POINT MappingToScrOrg();
	virtual RECT MappingToScrRect();
#ifdef _DEBUG
	virtual int Recognize(char modeOfAuto0BomPart1Summ2=0,int iCellRowStart=-1,int iCellRowEnd=-1,int iCellCol=-1);	//-1表示全部,>=0表示指定行或列 wjh-2018.3.14
#else
	virtual int Recognize(char modeOfAuto0BomPart1Summ2=0);
#endif
	virtual bool RetrieveCharImageData(int iCellRow,int iCellCol,IMAGE_CHARSET_DATA* imagechardata);
	virtual bool Destroy();	//BelongImageData()->DeleteImageRegion(GetSerial())
	virtual bool EnumFirstBomPart(IRecoginizer::BOMPART* pBomPart);
	virtual bool EnumNextBomPart(IRecoginizer::BOMPART* pBomPart);
	virtual UINT GetBomPartNum();
	virtual bool GetBomPartRect(UINT idPart,RECT* rc);
};
class CImageDataFile : public IImageFile
{
private:
	RECT m_rcShow;
	double m_fDisplayAllRatio;		//记录图像全显比例，部分显示时支持拖动
	double m_fDisplayRatio;			//记录图像缩放系数，绘制图片时使用
	int m_nOffsetX,m_nOffsetY;		//记录图片偏移位置，绘制图片时使用
	BYTE m_ciRawImageFileType;		//0.非Jpeg文件，按灰度图存储；1.Jpeg文件按原始文件存储
	bool m_bThinFontText;			//是否为细笔划字体
	bool m_bLowBackgroundNoise;		//背景是否为低噪点 wjh-2019.11.28
	int m_nMonoForwardPixels;		//划分黑白点间的界限调整平衡系数,取值-0.5~0.5;0时对应最高频率像素前移20位
	CTempFileBuffer vmRawBytes;		//原始图像的虚拟内存
	CTempFileBuffer vmGreyBytes;	//灰度图及阈值图的虚拟内存
	double CalPdfRegionZoomScaleBySample(CImageDataRegion *pRegion);
	bool RetrievePdfRegionImageBits(CImageDataRegion *pRegion,CImageTransform &regionImage);
public:
	DWORD m_idSerial;
	CXhChar500 m_sPathFileName;
	CImageTransform image;
	CHashListEx<CImageDataRegion> hashRegions;
	struct BLCKPIXEL_WAVEINFO{
		WORD* blcks;
		int size;
		int nMaxBlcks,nMaxTroughBlcks;
		BLCKPIXEL_WAVEINFO(WORD* _blcks=NULL,int _size=0,int _nMaxBlcks=0,int _nMaxTroughBlcks=0){
			blcks=_blcks;
			size=_size;
			nMaxBlcks=_nMaxBlcks;
			nMaxTroughBlcks=_nMaxTroughBlcks;
		}
	};
	struct PRJSTAT_INFO{
		static const char STAT_TL = 0;	//左上角统计
		static const char STAT_TR = 1;	//右上角统计
		static const char STAT_BR = 2;	//右下角统计
		static const char STAT_BL = 3;	//左下角统计
		char ciStatType;
		double dfPriorityOfRow;	//行线作为文本正交矫正基准的优先系数
		double dfPriorityOfCol;	//列线作为文本正交矫正基准的优先系数
		double dfImgSharpness;	//图像锐利度，即峰值前后的锐利度(自定义概念) wjh-2018.9.7
		UINT uiMaxBlckHoriPixels,yjHoriMaxBlckLine;
		UINT uiMaxBlckVertPixels,xiVertMaxBlckLine;
		PRJSTAT_INFO();
		bool Stat(WORD* rowblcks,int width,WORD* colblcks,int height);
		bool StatLine(WORD* blcks,int size,int nMaxBlcks,char ciDirectionH0V1,bool inverse=false);
	private:
		struct STAT_BLCKS{int indexOfPeakBlcks,nPeakBlcks;};
		bool RecognizePeakBlcks(WORD* xarrBlcks,int size,STAT_BLCKS* pStatBlcks,int iMidPosition,int nMaxTroughBlcks,BYTE cnMaxPeakHalfWidth=5);
		bool RecognizePeakBlcks(BLCKPIXEL_WAVEINFO& xarrblcks,STAT_BLCKS* pStatBlcks,int iMidPosition,BYTE cnMaxPeakHalfWidth=5);
	};
public:
	CImageDataFile(DWORD key=0);
	~CImageDataFile();
	//
	bool LoadRawImageBytesFromVM(char* lpImgBytes=NULL,UINT uiMaxSize=0);
	bool UnloadRawImageBytes(bool write2vm=true);
	bool LoadGreyBytes(bool* pbLoadFromVmFile=NULL);
	bool UnloadGreyBytes(bool write2vm=true);
	//bSnapPdfLiveRegionImg,true:从PDF中根据设定比例动态截取指定区域的图像（不影响父级CImageDataFile中图像精度）wjh-2018.8.27
	bool RetrieveRegionImageBits(CImageDataRegion *pRegion,bool bSnapPdfLiveRegionImg=true);
	bool RetrieveRectImageBits(const RECT& rcRegion,IMAGE_DATA* pImgData,TRANSFORM* pTransform=NULL);
	bool CalTextRotRevision(const RECT& rcRgn,const IMGROT& rotation,char ciDetectRule='X',PRJSTAT_INFO* pStatInfo=NULL);
	bool TestRotRevision(const POINT& xPickCenter,const POINT& xTopLeft2Pick,WORD wiDetectWidth,WORD wiDetectHeight,char ciDetectRule='X',PRJSTAT_INFO* pStatInfo=NULL);
	bool PreTestScanRegionGridLine(CImageDataRegion *pRegion);
public:
	bool ProjectRegionImage(const RECT& rcRgn,int yjRotRightPixel,PRJSTAT_INFO* pStatInfo=NULL,IMAGE_DATA* imgdata=NULL);
	bool ProjectRegionImage(const RECT& rcRgn,double dfEdgeWidthX,double yjRightTipRotPixel,PRJSTAT_INFO* pStatInfo=NULL,IMAGE_DATA* imgdata=NULL);
	//
	SIZE GetOffsetSize(DWORD dwRegionKey=0);
	void SetOffsetSize(SIZE offset,DWORD dwRegionKey=0);
	double GetDisplayRatio(DWORD dwRegionKey=0);
	BOOL IsCanZoom(DWORD dwRegionKey=0);
	void ToBuffer(CBuffer &buffer);
	void FromBuffer(CBuffer &buffer,long version);
public:
	virtual long GetSerial(){return m_idSerial;}
	virtual bool InitImageFileHeader(const char* imagefile);
	virtual bool InitImageFile(const char* imagefile,const char* file_path,bool update_file_path=true,PDF_FILE_CONFIG *pPDFConfig=NULL);
	virtual void SetPathFileName(const char* imagefilename);
	virtual void GetPathFileName(char* imagename,int maxStrBufLen=17);
	virtual int GetWidth();
	virtual int GetHeight();
	__declspec( property(get=GetWidth)) int Width;
	__declspec( property(get=GetHeight)) int Height;
	//背景是否为低噪点 wjh-2019.11.28
	virtual bool SetLowBackgroundNoise(bool blValue){return m_bLowBackgroundNoise=blValue;}
	virtual bool IsLowBackgroundNoise()const{return m_bLowBackgroundNoise;}
	__declspec( property(put=SetLowBackgroundNoise,get=IsLowBackgroundNoise)) bool blLowBackgroundNoise;
	//是否为细笔划字体
	virtual bool SetThinFontText(bool blValue){return m_bThinFontText=blValue;}
	virtual bool IsThinFontText()const{return m_bThinFontText;}
	__declspec( property(put=SetThinFontText,get=IsThinFontText)) bool blThinFontText;
	virtual BYTE GetRawFileType(){return m_ciRawImageFileType;}
	virtual bool IsSrcFromPdfFile(){return m_ciRawImageFileType==RAW_IMAGE_PDF||m_ciRawImageFileType==RAW_IMAGE_PDF_IMG;}
	//获取内部分配给m_lpRawBits内存大小
	UINT GetInternalAllocRawImageBytes(){return image.GetInternalAllocRawImageBytes();}
	virtual int Get24BitsImageEffWidth();
	virtual int Get24BitsImageData(IMAGE_DATA* imagedata);
	virtual int GetMonoImageData(IMAGE_DATA* imagedata);
	virtual bool TurnClockwise90();		//将文件中图像顺时针转90度，旋转不影响已提取区域，应在提取区域之前旋转
	virtual bool TurnAntiClockwise90();	//将文件中图像逆时针转90度，旋转不影响已提取区域，应在提取区域之前旋转
	virtual bool IsBlackPixel(int i,int j);
	virtual void InitImageShowPara(RECT showRect);
	virtual RECT GetImageDisplayRect(DWORD dwRegionKey=0);
	virtual bool DeleteImageRegion(long idRegion);
	virtual void SetScrOffset(SIZE offset);
	virtual SIZE GetScrOffset();
	virtual bool IsCanZoom(){return m_fDisplayRatio<=min(1,m_fDisplayAllRatio);}
	virtual double Zoom(short zDelta,POINT* pxScrPoint=NULL);
	virtual double ZoomRatio(){return m_fDisplayRatio;}	//缩放比例=屏幕尺寸/原始图像尺寸
	virtual bool FocusPoint(POINT xImgPoint,POINT* pxScrPoint=NULL,double scaleofScr2Img=2.0);
	virtual POINT MappingToScrOrg();
	virtual RECT MappingToScrRect();
	virtual bool IntelliRecogCornerPoint(const POINT& pick,POINT* pCornerPoint,
				 char ciType_TL0_TR1_BR2_BL3=0,int *pyjHoriTipOffset=NULL,UINT uiTestWidth=300,UINT uiTestHeight=200);
	virtual IImageRegion* AddImageRegion();
	virtual IImageRegion* AddImageRegion(POINT topLeft,POINT btmLeft,POINT btmRight,POINT topRight);
	virtual IImageRegion* EnumFirstRegion();
	virtual IImageRegion* EnumNextRegion();
	virtual void UpdateImageRegions();
	virtual BYTE ImageCompressType();
	virtual bool SetImageRawFileData(char* imagedata,UINT uiMaxDataSize,BYTE biFileType,PDF_FILE_CONFIG *pPDFConfig=NULL);
	//virtual UINT GetJpegImageData(char* imagedata,UINT uiMaxDataSize=0);
	virtual bool SetCompressImageData(char* imagedata,UINT uiMaxDataSize);
	virtual UINT GetCompressImageData(char* imagedata,UINT uiMaxDataSize=0);
	virtual double SetMonoThresholdBalanceCoef(double balance=0,bool bUpdateMonoImages=false);
	virtual double GetMonoThresholdBalanceCoef();
	//balancecoef 划分黑白点间的界限调整平衡系数,取值-0.5~0.5;0时对应最高频率像素前移20位
	static double CalMonoThresholdBalanceCoef(int monoforwardpixels=20);
};
class CAlphabets : public IAlphabets
{
public:
	struct FONTINFO{
		WORD wFontSerial;	//模板序列号
		int m_nTemplateH;	//模板字体高度
		double m_fHtoW;		//高宽比
		double m_fLowestSimilarity;
		BOOL m_bPreSplitChar;	//预拆分特殊字符，针对某些字体需要根据字符匹配模式拆分出特殊字符 wht 18-07-04
		void SetKey(DWORD key){wFontSerial=(WORD)key;}
		void ToBuffer(CBuffer &buffer,DWORD dwVersion=0,BOOL bIncSerial=TRUE);
		void FromBuffer(CBuffer &buffer,DWORD dwVersion=0,BOOL bIncSerial=TRUE);
		FONTINFO();
	};//basicfont;	//默认字体
private:
	FONTINFO basicfont;
	int m_iActiveFontSerial;
	char m_sFontRootPath[MAX_PATH];
protected:	//自适应字体识别
	struct FONT_PRACTICE : public ICompareClass{
		WORD wiFontSerial;
		UINT uiHits;	//识别时命中次数
		double prefer;	//之前命中的累积匹配度
		FONT_PRACTICE(WORD _wiFontSerial=0){prefer=uiHits=0;wiFontSerial=_wiFontSerial;}
		void AddMatchHits(double matchcoef){
			uiHits++;
			prefer+=matchcoef;
		}
		virtual int Compare(const ICompareClass *pCompareObj)const{
			FONT_PRACTICE* pPractice=(FONT_PRACTICE*)pCompareObj;
			if(prefer>pPractice->prefer)
				return -1;
			else if(prefer<pPractice->prefer)
				return 1;
			else
				return 0;
		}
	};
	PRESET_ARRAY16<FONT_PRACTICE> xarrFontPreferGrade;
	PRESET_ARRAY16<WORD> xarrFontPreferOrder;	//按照动态智能匹配进行字体优先度排序的字体顺序
	UINT m_uiSumMatchHits,m_uiLastOptOrderHits;
	void ClearAdaptiveFonts();
	FONT_PRACTICE* GetFontPractice(WORD wiFontSerial);
	void PracticeFontRecogChance(WORD wiFontSerial,double matchcoef);
	int GetRecogTmplChars(CImageChar* pHeadTmplChar,CImageCharPtr* ppTmplChars,int nMaxTmplCharCount=0);
public:
	bool m_bPreferStrokeFeatureRecog;
	double m_fMinWtoHcoefOfChar1;	//字体中‘1’字符最少字宽像素数与高度的比值，0.表示系统默认值 wjh-2019.11.28
	CHashListEx<FONTINFO> hashFonts;
	CHashListEx<CImageChar> listChars;	//模板字符列表
	CHashListEx<CStrokeFeature> hashStrokeFeatures;	//字符特征
	CImageChar *pCharX,*pCharX2,*pCharL,*pCharQ,*pCharPoint,*pCharZero;
	virtual bool InitFromFolder(const char* charimagefile_folder,WORD font_serial=0,bool append_mode=false);
	virtual void InitFromImageFile(const WORD wChar, const char* image_path, const WORD wiFontSerial=0);
	virtual int GetCharMonoImageData(const WORD wChar, IMAGE_DATA* imagedata,WORD fontserial=0);	//每位代表1个像素，0表示白点,1表示黑点
	virtual void SetLowestRecogSimilarity(WORD fontserial,double fSimilarity);
	virtual double LowestRecogSimilarity(WORD fontserial=0);
	virtual void FilterRecogState(BYTE cbCellTypeFlag=0,DWORD dwParam=0);
	virtual void SetPreSplitChar(WORD fontserial,BOOL bPreSplitChar);
	virtual BOOL IsPreSplitChar(WORD fontserial=0);
	virtual void SetFontLibRootPath(const char* root_path);
	virtual UINT InitStrokeFeatureByFiles(const char* szFolderPath);
	bool ImportFontsFile(int iFontsLibId);
public:
	struct MATCHCHAR
	{
		XHWCHAR wChar;
		CImageChar* pTemplChar;
		char ciOffsetX,ciOffsetY;
		bool bReExtended;
		MATCHCOEF matchcoef;
		MATCHCHAR(){ciOffsetX=ciOffsetY=0;pTemplChar=NULL;bReExtended=false;}
	};
public:
	CAlphabets();
	virtual UINT ToBLOB(char* blob,UINT uBlobBufSize,DWORD dwVersion=0);
	virtual void FromBLOB(char* blob,UINT uBlobSize,DWORD dwVersion=0);
	virtual bool ImportFontsFile(const char* fontsfilename);
	virtual bool ExportFontsFile(const char* fontsfilename);
	int GetTemplateH(int template_serial=0);
	void RetrieveImageTextByTemplChar(CImageChar &safeText,CImageChar *pTemplChar,CImageCellRegion *pCellRegion,int iStartPos);
	BOOL RecognizeData(CImageChar* pText,double minMatchCoef=0,MATCHCOEF* pMachCoef=NULL);
	BOOL EarlyRecognizeChar(CImageChar* pText,double minMatchCoef=0,MATCHCOEF* pMachCoef=NULL,CImageCellRegion *pCellRegion=NULL,int iStartPos=0);
};
