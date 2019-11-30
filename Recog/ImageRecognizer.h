#pragma once

#include "list.h"
#include "f_ent_list.h"	//�Ժ�Ӧ��CXhSimpleList��ȫȡ��
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
//�����༰���ߺ���
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
static const BYTE DETECT_LEFT	= 0x01;	//X��С����
static const BYTE DETECT_RIGHT	= 0x02;	//X���ӷ���
static const BYTE DETECT_UP		= 0x04;	//Y��С����
static const BYTE DETECT_DOWN	= 0x08;	//Y���ӷ���
static const BYTE DETECT_X0		= 0x10;	//X���ֲ���
static const BYTE DETECT_Y0		= 0x20;	//Y���ֲ���
struct IImage{
	static const BYTE PIXEL_IS_BYTE = 0;//ÿ������ռ1��Byte
	static const BYTE PIXEL_IS_BIT  = 1;//ÿ������ռ1��Bit

	BYTE* m_lpBitMap;	//new BYTE[m_nEffWidth*m_nHeight]����ɨ��downward (Top->Bottom)
	int m_nEffWidth;	//ʵ����Ч���
	int m_nWidth,m_nHeight;
public:
	IImage(){m_lpBitMap=NULL;m_nEffWidth=m_nWidth=m_nHeight=0;}
	virtual const BYTE* GetImageMap(){return m_lpBitMap;}
	virtual bool IsPixelBitMode()=0;	//True:ÿ������ռһ���е�һ��bitλ;falseռһ��Byte�ֽ�
	virtual bool BlackBitIsTrue(){return true;}
	virtual bool IsBlackPixel(int i,int j);
	virtual bool SetPixelState(int i,int j,bool black=true);
	virtual int GetImageEffWidth(){return m_nEffWidth;}	//��ȡͼ��ÿ��������ռ�õ�ʵ���ֽ���
	virtual int GetImageWidth(){return m_nWidth;}		//��ȡͼ��ÿ�е���Ч������
	virtual int GetImageHeight(){return m_nHeight;}		//��ȡͼ��ÿ�е���Ч������
};
class DETECT_MAP{
	bool m_bExternalMemory;
	char* map;	//0.δ̽�⵽δ��ʼ������;'W'����̽�⵽�İ׵�;����ֵΪ��̽�⵽�ĺڵ�
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
	bool m_bExternalMemory;	//����dataָ����������Ƿ�Ϊ�ⲿ������ڴ档���ǲ��������ڲ��ͷ�
	struct ROW_DATA{
		TYPE defaultErrorObj;
		TYPE* rowdata;	//ָ��data�ĺ�׺�����ڴ棬����Ҫ����������ͷ��ڴ�
		long liStartAddrIndex,liRowDataLength;
		ROW_DATA(BYTE* imgrowdata=NULL,long liBaseAddrIndex=0,long liRowLength=0)
		{
			rowdata=(TYPE*)imgrowdata;
			liStartAddrIndex=liBaseAddrIndex;
			liRowDataLength=liRowLength;
		}

		TYPE& operator[](long xI)	//indexΪ���ص�������ֵ����(X,Y)�����е�X���꣬����x,y)=[y][x]
		{
			if(xI<this->liStartAddrIndex||xI>=liStartAddrIndex+liRowDataLength)
				return defaultErrorObj;
			else
				return rowdata[xI-liStartAddrIndex];
		}
	};
	ROW_DATA* rowheader;
	virtual ROW_DATA GetRowData(long index)
	{	//indexΪ���ص�������ֵ����(X,Y)�����е�Y���꣬����x,y)=[y][x]
		if(index<0||index>=m_nRows)
			return ROW_DATA();
		return rowheader[index];
	}
public:
	//uiExterDataBytesΪexterdataָ����ⲿ�洢�ռ���ֽ���
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
	//indexΪ���ص�������ֵ����(X,Y)�����е�Y���꣬����x,y)=[y][x]
	ROW_DATA operator[](long index){return GetRowData(index);}
	READONLY_PROPERTY(long,ActualElemCount);	//���ض�ά�������ܼƴ洢��TYPE���͵Ķ���Ԫ����
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
	//maxIslandBlackPixels��(x=i,y=j)��Ϊ�µ�ʶ�����������ڵ�������
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
	virtual bool IsPixelBitMode(){return true;}	//True:ÿ������ռһ���е�һ��bitλ;falseռһ��Byte�ֽ�
	virtual bool IsBlackPixel(int i,int j);
	virtual void SetClipRect(const RECT& rc){clip=rc;}
	virtual RECT GetClipRect(){return clip;}
protected:
	RECT clip;	//��Ч�Ĳü�����
	bool m_bBlackPixelIsTrue;	//���ر���λΪTrue��ʾ�ڵ�
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
	virtual long GetImageWidth();	//��ȡͼ��ÿ�е���Ч������
	virtual long GetImageHeight();	//��ȡͼ��ÿ�е���Ч������
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
	double original;	//����ǰԭʼ��Ȩƥ��ϵ��
	double weighting;	//��������ֱʻ�����ͳ��δ���Ǻڵ�������ƥ��ϵ�� wjh-2018.3.22
	MATCHCOEF(double originalCoef=0.0,double weightingCoef=0.0);
	int CompareWith(const MATCHCOEF& coef);
};
//�ʻ�λͼ,��(0x00)��ʾ��ɫ,��(0x01)��ʾ���ڱʻ�����ͳ�ƻ�������
//��չ�ʻ�λͼ,��ɫ(0x03)����ͳ�����أ���ɫ(0x02)��ʾ����ͳ�����أ���������ʻ����ػ���
class CImageChar;
struct STROKELINE{
	BYTE cnBitCount;
	union DATA{
		BYTE bytes[8];
		DWORD dword;
		__int64 i64;
	}bits;
	//cnStatBits���ɸ�λ���λͳ�Ƶ�bitλ��
	static BYTE StatTrueBitCount(BYTE ciByte,short siStatBits=8);
	STROKELINE(const BYTE* pBytes=NULL,BYTE _ciBitOffset=0,BYTE _cnBitCount=0);
	void InitBits(const BYTE* pBytes,BYTE _ciBitOffset,BYTE _cnBitCount);
	BYTE StatMatchBitCountWith(STROKELINE& strokeline);
	bool SetPixelState(int i,bool state=true);
	bool GetPixelState(int i);
};
class CStrokeFeature : public IImage, public IStrokeFeature
{	//�ʻ�λͼ,��(0x00)��ʾ��ɫ,��(0x01)��ʾ���ڱʻ�����ͳ�ƻ�������
	UINT m_id;
protected:
	BYTE* m_lpBitMap;	//new BYTE[m_nEffWidth*m_nHeight]����ɨ��downward (Top->Bottom)
	int m_nEffWidth;	//ʵ����Ч���
	int m_nWidth,m_nHeight;
	PRESET_ARRAY32<STROKELINE> blckrows;	//ÿλΪ1ʱ��ʾΪ�ڻ�ҵ㣨���ַ�ͼ���н�������ͳ�Ƶĺڵ�����
	PRESET_ARRAY32<STROKELINE> redrows;	//ÿλΪ1ʱ��ʾΪ��㣨���ַ�ͼ���н�ֹΪ�ڵ������
	static int PixelRowBytesToBits(const BYTE* rowbytes,BYTE* rowbits,int width);
public:
	CStrokeFeature();
	~CStrokeFeature();
	virtual const BYTE* GetImageMap(){return m_lpBitMap;}
	virtual BYTE PixelBitCount(){return 2;}	//True:ÿ������ռһ���е�һ��bitλ;falseռһ��Byte�ֽ�
	virtual bool BlackBitIsTrue(){return true;}
	virtual bool IsBlackPixel(int i,int j);
	virtual bool SetPixelState(int i,int j,bool black=true);
	virtual bool IsPixelBitMode(void){return false;}
	virtual int GetImageEffWidth(){return m_nEffWidth;}	//��ȡͼ��ÿ��������ռ�õ�ʵ���ֽ���
	virtual int GetImageWidth(){return m_nWidth;}		//��ȡͼ��ÿ�е���Ч������
	virtual int GetImageHeight(){return m_nHeight;}		//��ȡͼ��ÿ�е���Ч������
public:
	WORD m_wiSumBlackPixels;	//�ʻ��к�ɫ���ص�ͳ����(��)��
	RECT rcSearch;		//������������Χ�������40*40���ص��ַ���
	static const char STATE_UPPER  =0x01;	//�ϰ벿
	static const char STATE_DOWNER =0x02;	//�°벿
	static const char STATE_MIDDLE =0x03;	//�м�
	static const char STATE_EXCLUDE=  -1;	//��������
	struct LIVESTATE{
		WORD wChar;
		char ciState;	//��������ָ���ַ��еĴ���״̬,0:��Լ��;>0���ַ�ָ����λ;<0�������ڸ��ַ�����
		void SetKey(DWORD key){wChar=(WORD)key;}
	};
	CHashListEx<LIVESTATE> hashStateInChars;
	void SetKey(DWORD key){m_id=key;}
	struct CLR{
		static const BYTE WHITE = 0x00;	//0:��ɫ;
		static const BYTE BLACK = 0x01;	//0x01:��ɫ;
		static const BYTE RED   = 0x02;	//0x02:��ɫ(��ֹɫ);
		static const BYTE GRAY  = 0x03;	//0x03:��ɫ(����ͳ��ɫ)
	};
	bool SetPixelColor(int i,int j,BYTE ciPixelClrIndex=1);//0:��ɫ;0x01:��ɫ;0x02:��ɫ(��ֹɫ);0x03:��ɫ(����ͳ��ɫ)
	BYTE GetPixelColor(int i,int j);
	bool IsGreyPixel(int i,int j);	//��Ϊ��ɫ��������ͳ�Ƶ�����
	bool IsRedPixel(int i,int j);	//��ֹΪ��ɫ������
	bool InitTurboRecogInfo();
	bool RecogInImageChar(CImageChar* pImgChar,BYTE ciMinPercent=70);
	void ToBuffer(CBuffer &buffer,DWORD dwVersion=0,BOOL bIncSerial=TRUE);
	void FromBuffer(CBuffer &buffer,DWORD dwVersion=0,BOOL bIncSerial=TRUE);

	virtual UINT GetId(){return m_id;}
	__declspec(property(get=GetId)) UINT Id;
	virtual char SetLiveStateInChar(WORD wChar,char ciLiveState);	//��������ָ���ַ��еĴ���״̬,0:��Լ��;>0���ַ�ָ����λ;<0�������ڸ��ַ�����
	virtual char LiveStateInChar(WORD wChar);	//��������ָ���ַ��еĴ���״̬,0:��Լ��;>0���ַ�ָ����λ;<0�������ڸ��ַ�����
	virtual void AddSearchRegion(RECT& rcSearch);
	virtual bool FromBmpFile(const char* szStrokeBmpFile);
	virtual UINT ToBLOB(char* blob,UINT uBlobBufSize,DWORD dwVersion=0);
	virtual void FromBLOB(char* blob,UINT uBlobSize,DWORD dwVersion=0);
};
class CImageChar : public IImageZoomTrans, public IImage
{
protected:	//ʶ�������һЩ�ַ��ؼ�����
	WORD wTopBlackPixels;	//����20%�ĺڵ�����������ʶ��'7',�ų�'1'
	WORD wBtmBlackPixels;	//�ײ�20%�ĺڵ�����������ʶ��'2',�ų�'7'
	RECT rcFeatureActual;	//�ڵ��ʵ�ʰ��������������ʶ��'1','-','_','.'
public:
	RECT get_rcActual(){return rcFeatureActual;}
	__declspec(property(get=get_rcActual)) RECT rcActual;
	struct ISLAND{
		//bool boundary;
		WORD maxy,miny;
		double x,y,count;
		ISLAND(){Clear();}
		void Clear(){x=y=count=maxy=miny=0;}//boundary=false;}
	};	//�ڲ��հ׹µ���������
	struct PIXEL{
		BYTE *pcbPixel;
		PIXEL(BYTE* _pcbImgPixel=0){pcbPixel=_pcbImgPixel;}
		bool get_Black();
		bool set_Black(bool black);
		char set_Connected(char connected);		//�������Ƿ�Ϊ��ͨ���������
		char get_Connected();		//�������Ƿ�Ϊ��ͨ���������
		const static char NONEVISIT	=0;	//��δ���ʵ�������
		const static char CONNECTED	=1;	//��߽���ͨ������
		const static char UNDECIDED	=2;	//�Ѽ�⵽�����أ���״̬δ�ж�
		__declspec(property(put=set_Black,get=get_Black)) bool Black;
		__declspec(property(put=set_Connected,get=get_Connected)) char Connected;
	};
	int DetectIslands(CXhSimpleList<ISLAND>* listIslands);
	WORD StatLocalBlackPixelFeatures();
protected:
	int m_nPixels;
	//BYTE* m_lpBitMap;	//��m_nPixels����ÿ���ֽڱ�ʾ1������, 0��ʾ�׵�;1��ʾ�ڵ�
	int _nDestValidWidth;	//����ת����Ŀ��ͼ������Ч���
	bool IsDetect(int i,int j);
	bool SetDetect(int xi,int yi,bool detect=true);
	void ClearDetectMark();
	bool StatNearWhitePixels(int i,int j,ISLAND* pIsland,PRESET_ARRAY2048<PIXEL>* listStatPixels,BYTE cDetectFlag=0x0f,CLogFile* pLogFile=NULL);
public:
	bool m_bTemplEnabled;
	XHWCHAR wChar;
	int m_nBlckPixels;	//�ַ��ڵ�������Localize()��Standardize()��StandardLocalize()�����г�ʼ��
	int xiImgLeft,xiImgRight;	//�õ�Ԫ���ַ�λ��ԭʼ��Ԫ����ȡ����������ͼ�����ʼ���ؼ���ֹ����
	void SetKey(DWORD key){wChar.wHz=(WORD)key;}
	bool IsCharX(){return wChar.wHz=='x'||wChar.wHz=='X';}
	static const WORD CHAR_FAI=0xb5a6,CHAR_FAI2=0xd5a6;	//��(GB2312:0xb5a6,46502)��(GB2312:0xd5a6,54694)
	bool IsCharFAI(){return wChar.wHz==CHAR_FAI||wChar.wHz==CHAR_FAI2;}
	bool IsSoltTypeChar(){return wChar.wHz=='['||wChar.wHz=='C';}
	bool IsPlateTypeChar(){return wChar.wHz=='-'||wChar.wHz=='_';}
	bool IsAngleTypeChar(){return wChar.wHz=='L';}
	bool IsPartTypeChar(){return IsCharFAI()||IsSoltTypeChar()||IsPlateTypeChar()||IsAngleTypeChar();}
	bool IsMaterialChar(){ return CImageChar::IsMaterialChar(wChar.wHz);}
	static bool IsMaterialChar(WORD wChar){ return wChar=='S'||wChar=='H'||wChar=='P'||wChar=='E';}
	bool IsPartLabelPostfix(){return wChar.wHz=='A'||wChar.wHz=='B'||wChar.wHz=='C';}
	//RECT rect;	//��ʼ��ʱԭͼ���еĽ�ȡ����(����rect.right��rect.bottom��)
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
	//������������������ַ�
protected:
	WORD _serial;	//�����ʶ���
public:
	void set_FontSerial(WORD fontserial){_serial=fontserial;}
	WORD get_FontSerial(){return _serial;}	//�����ʶ���
	__declspec(property(get=get_FontSerial)) WORD FontSerial;
	CImageChar* linknext;
	CImageChar* AppendFontChar(WORD fontserial=0);	//fontserial=0��ʾ�Զ��ۼ�һ����ͬ�������ʶ������׷��ָ�������ʶ
	bool RemoveChild();
	CImageChar* FontChar(WORD fontserial);
public:
	CImageChar(WORD fontserial=0);
	~CImageChar(); 
	virtual bool IsPixelBitMode(){return false;}	//True:ÿ������ռһ���е�һ��bitλ;falseռһ��Byte�ֽ�
	virtual bool IsBlackPixel(int i,int j);
	void CopyImageChar(CImageChar* pChar);
	void InitTemplate(BYTE* lpBits,int eff_width,int width,RECT rect);
	void Localize(BYTE* lpBits,int eff_width,RECT rect);
	void Standardize(WORD wStdWidth,WORD wStdHeight);
	void StandardLocalize(BYTE* lpBits,bool blackbit_is_true,int eff_width,RECT& rc,int wStdHeight,bool onlyHighestBand=true);
	int CheckFeatures(CXhSimpleList<ISLAND>* pIslands=NULL);	//-1��ȷ��������������0.����ȷ���Ƿ����������1.ȷ����������
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
	static const WORD ABNORMAL_CHAR_EXIST = 0x01;	//���ڲ���ʶ����쳣�ַ�
	static const WORD ABNORMAL_TXT_REGION = 0x02;	//��ȡ�ĵ�Ԫ���ı�������쳣
	static const WORD LOWRECOG_CHAR_EXIST = 0x04;	//����ƥ��Ƚϵ͵��ַ�
	static BYTE MAX_WARNING_ORIGIN_MATCH_COEF/*=65*/;
	static BYTE MAX_WARNING_WEIGHT_MATCH_COEF/*=80*/;
	struct{
		BYTE matchLevel[2];	//ƥ��ʶ���:vfOrgMatchCoef=matchLevel[0]*0.01,vfWeightMatchCoef=matchLevel[1]*0.01;
		WORD wWarnFlag;		//ʶ������е��쳣���
	};
	BYTE WaringLevelNumber();//0~255,Խ�߱�ʾ��ʾ�̶�Խ��
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
	IMAGE_CHARSET_DATA::CHAR_IMAGE preSplitZeroCharArr[4];	//��ǰʶ����ַ����������4��0��Q420L100X10�� wht 18-07-01
	bool ProcessBriefMaTemplChar(CImageChar *pTemplChar,CImageChar *pPartTypeChar,
								 CImageChar *pFirstWidth,int nWidthCharCount,
								 CImageChar *pFirstPlateThickChar,int nThickCharCount);
public:
	struct CHAR_SECT{
		BYTE ciMatchPercentW;	//ƥ��Ȱٷ���(��Ȩ��ƥ��ȣ�
		BYTE ciMatchPercentO;	//ƥ��Ȱٷ�����ԭʼƥ��ȣ�
		BYTE cnLeftBlckPixels,cnRightBlckPixels;	//�ҷֽ��ߺ�ɫ������
		WORD wChar;	//�ɱ�ʾ����
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
	bool m_bStrokeVectorImage;			//�ʻ�ʸ��ͼ���ɵ�ͼ������ͼ�񼸺�������ʧ�棬�����̶ȱ����˸��ַ���������Ϣ������������ʶ������ wjh-2018.8.31
	int m_nDataType;
	int m_nLeftMargin,m_nRightMargin;	//��Ч�����뵥Ԫ����\�Ҳ�߽��߾��룬������������ѹ������ wht 18-07-30
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
	virtual bool IsPixelBitMode(){return true;}	//True:ÿ������ռһ���е�һ��bitλ;falseռһ��Byte�ֽ�
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
	long nPart;		//һ�������о��еĵ�ǰ��ŵ��Σ�������������
public:
	static const BYTE OTHER = 0;//����ǳ��湹��
	static const BYTE BOLT  = 1;//��˨
	static const BYTE ANGLE = 2;//�Ǹ�
	static const BYTE PLATE = 3;//�ְ�
	static const BYTE TUBE  = 4;//�ֹ�
	static const BYTE FLAT  = 5;//���
	static const BYTE SLOT  = 6;//�۸�
	static const BYTE ROUND = 7;//Բ��
	static const BYTE FITTINGS = 8;//����
	//����������
	const static BYTE SUB_TYPE_TUBE_MAIN	= 1;	//����
	const static BYTE SUB_TYPE_TUBE_BRANCH	= 2;	//֧��
	const static BYTE SUB_TYPE_PLATE_C		= 3;	//���Ͳ��
	const static BYTE SUB_TYPE_PLATE_U		= 4;	//U�Ͳ��
	const static BYTE SUB_TYPE_PLATE_X		= 5;	//ʮ�ֲ��
	const static BYTE SUB_TYPE_PLATE_FL		= 6;	//����
	const static BYTE SUB_TYPE_PLATE_WATER	= 7;	//��ˮ��
	const static BYTE SUB_TYPE_FOOT_FL		= 8;	//�׽ŷ���
	const static BYTE SUB_TYPE_ROD_Z		= 9;	//����
	const static BYTE SUB_TYPE_ROD_F		= 10;	//����
	const static BYTE SUB_TYPE_ANGLE_SHORT	= 11;	//�̽Ǹ�
	const static BYTE SUB_TYPE_PLATE_FLD	= 12;	//�Ժ�����
	const static BYTE SUB_TYPE_PLATE_FLP	= 13;	//ƽ������
	const static BYTE SUB_TYPE_PLATE_FLG	= 14;	//���Է���
	const static BYTE SUB_TYPE_PLATE_FLR	= 15;	//���Է���
	const static BYTE SUB_TYPE_BOLT_PAD		= 16;	//��˨���

	char cPartType;
public:
	UINT id;			//��ʶ��
	RECT rc;			//���й�����CImageDataRegion�ж�Ӧ�ľ�������
	CXhChar16 sPartNo;	//�������
	int iSeg;			//�κ�
	short siSubType;	//
	char cMaterial;		//���ʼ��ַ����磺'S','H','G'
	double wide;		//������Ȳ���
	double thick;		//������Ȳ���
	double wingWideY;	//�Ǹ�Y֫���
	CXhChar16 sSpec;	//���
	double length;		//�������Ȳ���
	double fPieceWeight;//������(kg)�����ڸְ��Ϊ��С���ݾ�������
	double fSumWeight;	//����
	short siZhiWan;		//>0�����������<=0����Ҫ����
	BOOL bWeldPart;		//�Ƿ���Ҫ����
	BOOL bLegPart;		//�Ƿ�Ϊ�Ȳ�����
	short nM16Ls;	//M16��˨����
	short nM20Ls;	//M20��˨����
	short nM24Ls;	//M24��˨����
	short nMXLs;	//����ֱ����˨����
	short nMSumLs;	//��˨������
	char sNotes[50];	//������ע˵��
	long feature1;		//��������1����ʱʹ��
	long feature2;		//��������2����ʱʹ��
	bool bStdPart;		//��׼��
	BYTE arrItemWarningLevel[7];//0.����;1.����&���;2.����;3.������;4.����;5.����;6.��ע �еľ�ʾ��Ϣ��Խ��Խ��Ҫ��ʾ
	//��˨��Ϣ
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
	virtual int GetLsHoleStr(char *hole_str,BOOL bIncUnit=TRUE);	//��˨���ַ���
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
	static int m_nInitMonoForwardPixels;//=20;	//��¼ǰһ������ƽ��ֵ���½�����ʱ������ʼ���������ƽ��ֵ wht 18-07-22
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
	int m_nMonoForwardPixels;		//��CImageDataFile���������ĳһ����Ļ��ֺڰ׵��Ľ��޵���ƽ�����ֵ
	POINT leftUp,leftDown,rightUp,rightDown;					//��ͼֽ�Ͽ�ѡ������
	POINT realLeftUp,realLeftDown,realRightUp,realRightDown;	//�Ŵ���PDF������ο� wht 18-07-10
	virtual bool IsPixelBitMode(){return true;}	//True:ÿ������ռһ���е�һ��bitλ;falseռһ��Byte�ֽ�
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
	//�����ϻ��ܱ�ģʽ��ȡ	 wjh-2018.11.13
	int RecognizeSummaryBOM(DYN_ARRAY<CELL_RECT> &rowCells,BYTE *pWarningLevel=NULL);
	//���ֶι�����ϸ��ģʽ��ȡ
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
public:	//�����������д�����ʾ״̬��ص�����
	double m_fDisplayAllRatio;		//��¼ͼ��ȫ�Ա�����������ʾʱ֧���϶�
	double m_fDisplayRatio;			//��¼ͼ������ϵ��������ͼƬʱʹ��
	int m_nOffsetX,m_nOffsetY;		//��¼ͼƬƫ��λ�ã�����ͼƬʱʹ��
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
public:	//�����Ҷ�ͼתΪ�ڰ�ͼ�ֽ�����ֵ����ر���������
	virtual double SetMonoThresholdBalanceCoef(double mono_balance_coef=0);
	virtual double GetMonoThresholdBalanceCoef();
	int get_MonoForwardPixels(){return this->m_nMonoForwardPixels;}
	//__declspec(property(get=get_MonoForwardPixels)) int MonoForwardPixels;
	//balancecoef ���ֺڰ׵��Ľ��޵���ƽ��ϵ��,ȡֵ-0.5~0.5;0ʱ��Ӧ���Ƶ������ǰ��20λ
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
	virtual int GetMonoImageData(IMAGE_DATA* imagedata);	//ÿλ����1�����أ�0��ʾ�׵�,1��ʾ�ڵ�
	virtual int GetRectMonoImageData(RECT* rc,IMAGE_DATA* imagedata);	//������rc->right�м�rc->bottom��
	virtual int GetImageColumnWidth(int* col_width_arr=NULL);		//��������
	virtual int GetHeight(){return m_nHeight;}
	virtual int GetWidth(){return m_nWidth;}
	virtual void SetScrOffset(SIZE offset);
	virtual SIZE GetScrOffset();
	virtual bool IsCanZoom(){return m_fDisplayRatio<=min(1,m_fDisplayAllRatio);}
	virtual double Zoom(short zDelta,POINT* pxScrPoint=NULL);
	virtual double ZoomRatio(){return m_fDisplayRatio;}	//���ű���=��Ļ�ߴ�/ԭʼͼ��ߴ�
	virtual POINT MappingToScrOrg();
	virtual RECT MappingToScrRect();
#ifdef _DEBUG
	virtual int Recognize(char modeOfAuto0BomPart1Summ2=0,int iCellRowStart=-1,int iCellRowEnd=-1,int iCellCol=-1);	//-1��ʾȫ��,>=0��ʾָ���л��� wjh-2018.3.14
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
	double m_fDisplayAllRatio;		//��¼ͼ��ȫ�Ա�����������ʾʱ֧���϶�
	double m_fDisplayRatio;			//��¼ͼ������ϵ��������ͼƬʱʹ��
	int m_nOffsetX,m_nOffsetY;		//��¼ͼƬƫ��λ�ã�����ͼƬʱʹ��
	BYTE m_ciRawImageFileType;		//0.��Jpeg�ļ������Ҷ�ͼ�洢��1.Jpeg�ļ���ԭʼ�ļ��洢
	bool m_bThinFontText;			//�Ƿ�Ϊϸ�ʻ�����
	bool m_bLowBackgroundNoise;		//�����Ƿ�Ϊ����� wjh-2019.11.28
	int m_nMonoForwardPixels;		//���ֺڰ׵��Ľ��޵���ƽ��ϵ��,ȡֵ-0.5~0.5;0ʱ��Ӧ���Ƶ������ǰ��20λ
	CTempFileBuffer vmRawBytes;		//ԭʼͼ��������ڴ�
	CTempFileBuffer vmGreyBytes;	//�Ҷ�ͼ����ֵͼ�������ڴ�
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
		static const char STAT_TL = 0;	//���Ͻ�ͳ��
		static const char STAT_TR = 1;	//���Ͻ�ͳ��
		static const char STAT_BR = 2;	//���½�ͳ��
		static const char STAT_BL = 3;	//���½�ͳ��
		char ciStatType;
		double dfPriorityOfRow;	//������Ϊ�ı�����������׼������ϵ��
		double dfPriorityOfCol;	//������Ϊ�ı�����������׼������ϵ��
		double dfImgSharpness;	//ͼ�������ȣ�����ֵǰ���������(�Զ������) wjh-2018.9.7
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
	//bSnapPdfLiveRegionImg,true:��PDF�и����趨������̬��ȡָ�������ͼ�񣨲�Ӱ�츸��CImageDataFile��ͼ�񾫶ȣ�wjh-2018.8.27
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
	//�����Ƿ�Ϊ����� wjh-2019.11.28
	virtual bool SetLowBackgroundNoise(bool blValue){return m_bLowBackgroundNoise=blValue;}
	virtual bool IsLowBackgroundNoise()const{return m_bLowBackgroundNoise;}
	__declspec( property(put=SetLowBackgroundNoise,get=IsLowBackgroundNoise)) bool blLowBackgroundNoise;
	//�Ƿ�Ϊϸ�ʻ�����
	virtual bool SetThinFontText(bool blValue){return m_bThinFontText=blValue;}
	virtual bool IsThinFontText()const{return m_bThinFontText;}
	__declspec( property(put=SetThinFontText,get=IsThinFontText)) bool blThinFontText;
	virtual BYTE GetRawFileType(){return m_ciRawImageFileType;}
	virtual bool IsSrcFromPdfFile(){return m_ciRawImageFileType==RAW_IMAGE_PDF||m_ciRawImageFileType==RAW_IMAGE_PDF_IMG;}
	//��ȡ�ڲ������m_lpRawBits�ڴ��С
	UINT GetInternalAllocRawImageBytes(){return image.GetInternalAllocRawImageBytes();}
	virtual int Get24BitsImageEffWidth();
	virtual int Get24BitsImageData(IMAGE_DATA* imagedata);
	virtual int GetMonoImageData(IMAGE_DATA* imagedata);
	virtual bool TurnClockwise90();		//���ļ���ͼ��˳ʱ��ת90�ȣ���ת��Ӱ������ȡ����Ӧ����ȡ����֮ǰ��ת
	virtual bool TurnAntiClockwise90();	//���ļ���ͼ����ʱ��ת90�ȣ���ת��Ӱ������ȡ����Ӧ����ȡ����֮ǰ��ת
	virtual bool IsBlackPixel(int i,int j);
	virtual void InitImageShowPara(RECT showRect);
	virtual RECT GetImageDisplayRect(DWORD dwRegionKey=0);
	virtual bool DeleteImageRegion(long idRegion);
	virtual void SetScrOffset(SIZE offset);
	virtual SIZE GetScrOffset();
	virtual bool IsCanZoom(){return m_fDisplayRatio<=min(1,m_fDisplayAllRatio);}
	virtual double Zoom(short zDelta,POINT* pxScrPoint=NULL);
	virtual double ZoomRatio(){return m_fDisplayRatio;}	//���ű���=��Ļ�ߴ�/ԭʼͼ��ߴ�
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
	//balancecoef ���ֺڰ׵��Ľ��޵���ƽ��ϵ��,ȡֵ-0.5~0.5;0ʱ��Ӧ���Ƶ������ǰ��20λ
	static double CalMonoThresholdBalanceCoef(int monoforwardpixels=20);
};
class CAlphabets : public IAlphabets
{
public:
	struct FONTINFO{
		WORD wFontSerial;	//ģ�����к�
		int m_nTemplateH;	//ģ������߶�
		double m_fHtoW;		//�߿��
		double m_fLowestSimilarity;
		BOOL m_bPreSplitChar;	//Ԥ��������ַ������ĳЩ������Ҫ�����ַ�ƥ��ģʽ��ֳ������ַ� wht 18-07-04
		void SetKey(DWORD key){wFontSerial=(WORD)key;}
		void ToBuffer(CBuffer &buffer,DWORD dwVersion=0,BOOL bIncSerial=TRUE);
		void FromBuffer(CBuffer &buffer,DWORD dwVersion=0,BOOL bIncSerial=TRUE);
		FONTINFO();
	};//basicfont;	//Ĭ������
private:
	FONTINFO basicfont;
	int m_iActiveFontSerial;
	char m_sFontRootPath[MAX_PATH];
protected:	//����Ӧ����ʶ��
	struct FONT_PRACTICE : public ICompareClass{
		WORD wiFontSerial;
		UINT uiHits;	//ʶ��ʱ���д���
		double prefer;	//֮ǰ���е��ۻ�ƥ���
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
	PRESET_ARRAY16<WORD> xarrFontPreferOrder;	//���ն�̬����ƥ������������ȶ����������˳��
	UINT m_uiSumMatchHits,m_uiLastOptOrderHits;
	void ClearAdaptiveFonts();
	FONT_PRACTICE* GetFontPractice(WORD wiFontSerial);
	void PracticeFontRecogChance(WORD wiFontSerial,double matchcoef);
	int GetRecogTmplChars(CImageChar* pHeadTmplChar,CImageCharPtr* ppTmplChars,int nMaxTmplCharCount=0);
public:
	bool m_bPreferStrokeFeatureRecog;
	double m_fMinWtoHcoefOfChar1;	//�����С�1���ַ������ֿ���������߶ȵı�ֵ��0.��ʾϵͳĬ��ֵ wjh-2019.11.28
	CHashListEx<FONTINFO> hashFonts;
	CHashListEx<CImageChar> listChars;	//ģ���ַ��б�
	CHashListEx<CStrokeFeature> hashStrokeFeatures;	//�ַ�����
	CImageChar *pCharX,*pCharX2,*pCharL,*pCharQ,*pCharPoint,*pCharZero;
	virtual bool InitFromFolder(const char* charimagefile_folder,WORD font_serial=0,bool append_mode=false);
	virtual void InitFromImageFile(const WORD wChar, const char* image_path, const WORD wiFontSerial=0);
	virtual int GetCharMonoImageData(const WORD wChar, IMAGE_DATA* imagedata,WORD fontserial=0);	//ÿλ����1�����أ�0��ʾ�׵�,1��ʾ�ڵ�
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
