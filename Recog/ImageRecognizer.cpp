#include "stdafx.h"
#include "LogFile.h"
#include "ImageRecognizer.h"
#include "zlib.h"
#include "Robot.h"
#include "XhLicAgent.h"
#include "PdfEngine.h"
#include "SegI.h"

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#ifdef APP_EMBEDDED_MODULE_ENCRYPT
	#ifdef _DEBUG
	#undef THIS_FILE
	static char THIS_FILE[]=__FILE__;
	#define new DEBUG_NEW
	#endif
#endif

#ifdef _DEBUG
#ifdef _TIMER_COUNT
#include ".\TimerCount.h"
CTimerCount timer;
#endif
#endif
static int f2i(double fVal)
{
	if(fVal>0)
		return (int)(fVal+0.5);
	else
		return (int)(fVal-0.5);
}
char QuerySteelBriefMatMark(const char* material_str)
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
char* QuerySteelMaterialStr(char cBriefMaterial)
{
	if(cBriefMaterial=='S')
		return "Q235";
	else if(cBriefMaterial=='H')
		return "Q345";
	else if(cBriefMaterial=='G')
		return "Q390";
	else if(cBriefMaterial=='P'||cBriefMaterial=='E')
		return "Q420";
	else if(cBriefMaterial=='T')
		return "Q460";
	else
		return 0;
}
RECT RECT_C(int l, int t, int r, int b)
{
	RECT rc;
	rc.left=l;
	rc.top=t;
	rc.right=r;
	rc.bottom=b;
	return rc;
}

const BYTE BIT2BYTE[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
struct FUNC_ProjectRegionImage_BUFF{
	//行列黑点统计缓存
	WORD row_blck_pixels[800];
	WORD col_blck_pixels[200];
	//图像缓存
	static const WORD img_buff_length=10240;
	char img_buff_bytes[10240];//10k缓存可存最大800*100像素
}_local_func_ProjectRegionImage_buff_object;
//////////////////////////////////////////////////////////////////////////
//CBitImage
static void SetByteBitState(BYTE* pImageByte,int bitindex,bool bState)
{
	if(bState)
		*pImageByte|=BIT2BYTE[7-bitindex];
	else
		*pImageByte&=(0XFF-BIT2BYTE[7-bitindex]);
}
bool IImage::SetPixelState(int i,int j,bool black/*=true*/)
{
	if(i<0||j<0||i>=m_nWidth||j>=m_nHeight)
		return false;
	bool pixelstate=BlackBitIsTrue()?black:!black;
	if(IsPixelBitMode())
	{
		int iByte=i/8,iBit=i%8;
		if(pixelstate)
			m_lpBitMap[j*m_nEffWidth+iByte]|=BIT2BYTE[7-iBit];
		else
			m_lpBitMap[j*m_nEffWidth+iByte]&=(0XFF-BIT2BYTE[7-iBit]);
	}
	else
		m_lpBitMap[j*m_nEffWidth+i]=pixelstate?1:0;
	return black;
}
bool IImage::IsBlackPixel(int i,int j)
{
	if( i<0||i>=m_nWidth || j<0||j>=m_nHeight)
		return false;
	if(IsPixelBitMode())
	{
		int iByte=i/8,iBit=i%8;
		int gray=m_lpBitMap[j*m_nEffWidth+iByte]&BIT2BYTE[7-iBit];	//获取该点的灰度值
		//如果这个象素是黑色的
		return BlackBitIsTrue() ? (gray==BIT2BYTE[7-iBit]) : (gray!=BIT2BYTE[7-iBit]);
	}
	else
		return BlackBitIsTrue() ? m_lpBitMap[j*m_nEffWidth+i]!=0 : m_lpBitMap[j*m_nEffWidth+i]==0;
}
CBitImage::CBitImage(int width/*=0*/,int height/*=0*/,bool blackbit_is_true/*=false*/)
{
	m_bBlackPixelIsTrue=blackbit_is_true;
	m_lpBitMap=NULL;
	m_bExternalData=false;
	InitBitImage(NULL,width,height,0);
}
CBitImage::CBitImage(BYTE* lpBitMap,int width,int height,int nEffWidth,bool blackbit_is_true/*=false*/)
{
	m_bBlackPixelIsTrue=blackbit_is_true;
	m_lpBitMap=NULL;
	m_bExternalData=false;
	InitBitImage(lpBitMap,width,height,nEffWidth);
}
CBitImage::CBitImage(IMAGE_DATA& imgdata,bool blackbit_is_true/*=false*/)
{
	m_bBlackPixelIsTrue=blackbit_is_true;
	m_lpBitMap=NULL;
	m_bExternalData=false;
	InitBitImage((BYTE*)imgdata.imagedata,imgdata.nWidth,imgdata.nHeight,imgdata.nEffWidth);
}
CBitImage::~CBitImage()
{
	if(m_lpBitMap&&!m_bExternalData)
	{
		delete []m_lpBitMap;
		m_lpBitMap=NULL;
	}
}
void CBitImage::InitBitImage(BYTE* lpBitMap,int width,int height,int nEffWidth)
{
	if(!m_bExternalData&&m_lpBitMap!=NULL)
		delete []m_lpBitMap;
	m_lpBitMap=lpBitMap;
	m_bExternalData=m_lpBitMap!=NULL;
	m_nWidth=width;
	m_nHeight=height;
	if(nEffWidth==0)
		m_nEffWidth=m_nWidth%8>0 ? m_nWidth/8+1 : m_nWidth/8;
	else
		m_nEffWidth=nEffWidth;
	//m_nEffWidth=m_nWidth%16>0?(m_nWidth/16)*2+2:m_nWidth/8;
	if(m_lpBitMap==NULL&&m_nEffWidth>0&&m_nHeight>0)
	{
		int pixels=m_nEffWidth*m_nHeight;
		m_lpBitMap=new BYTE[pixels];
		memset(m_lpBitMap,0,pixels);
	}
	clip.left=clip.top=0;
	clip.right=width-1;
	clip.bottom=height-1;
}
bool CBitImage::IsBlackPixel(int i,int j)
{
	if( i<clip.left || i>clip.right|| j<clip.top|| j>clip.bottom ||
		i>=m_nWidth || j>=m_nHeight)
		return false;
	return IImage::IsBlackPixel(i,j);
}
//////////////////////////////////////////////////////////////////////////
//CMonoImage
static CHashListEx<CMonoImage> _localHashMonoImages;
static CMonoImage _localMonoImage1(1),_localMonoImage2(2),_localMonoImage3(3),_localMonoImage4(4);
IMonoImage* IMonoImage::CreateObject()
{
	_localHashMonoImages.SetMinmalId(5);
	return _localHashMonoImages.Append();
}
BOOL IMonoImage::RemoveObject(long serial)
{
	return _localHashMonoImages.DeleteNode(serial);
}
RECOG_API IMonoImage* IMonoImage::PresetObject(int presetObjSerialMax4/*=1*/)
{
	if(presetObjSerialMax4==1)
		return &_localMonoImage1;
	else if(presetObjSerialMax4==2)
		return &_localMonoImage2;
	else if(presetObjSerialMax4==3)
		return &_localMonoImage3;
	else if(presetObjSerialMax4==4)
		return &_localMonoImage4;
	else
		return NULL;
}
CMonoImage::CMonoImage(long serial/*=0*/)
{
	m_idSerial=serial;
}
CMonoImage::~CMonoImage()
{
}
long CMonoImage::Serial()
{
	return m_idSerial;
}
bool CMonoImage::InitImageSize(int width,int height)
{
	image.InitBitImage(NULL,width,height,0);
	return width>0&&height>0;
}
bool CMonoImage::IsNoisePixel(int i,int j,RECT* pValidRgn/*=NULL*/,int maxIslandBlackPixels/*=5*/)
{
	return image.IsNoisePixel(i,j,pValidRgn,maxIslandBlackPixels);
}
long CMonoImage::RemoveNoisePixels()
{
	return image.RemoveNoisePixels();
}
bool CMonoImage::IsBlackPixel(int i,int j)
{
	return image.IsBlackPixel(i,j);
}
bool CMonoImage::SetPixelState(int i,int j,bool black/*=true*/)
{
	return image.SetPixelState(i,j,black);
}
long CMonoImage::GetImageWidth()
{	//获取图像每行的有效象素数
	return image.GetImageWidth();
}
long CMonoImage::GetImageHeight()
{	//获取图像每列的有效象素数
	return image.GetImageHeight();
}
//////////////////////////////////////////////////////////////////////////
//IImageZoomTrans
bool IImageZoomTrans::InitImageZoomTransData(const RECT& src,const RECT& dest)
{
	_src=src;
	_dest=dest;
	xCoefDest2Src=dest.right-dest.left;
	yCoefDest2Src=dest.bottom-dest.top;
	if(src.right-src.left<=0 || src.bottom-src.top<=0)
		return false;
	xCoefDest2Src/=src.right-src.left;
	yCoefDest2Src/=src.bottom-src.top;
	xOffsetOrg=yOffsetOrg=0.0;
	if(xCoefDest2Src<=yCoefDest2Src)
		yOffsetOrg=0.5*((_dest.bottom-_dest.top)/xCoefDest2Src-(_src.bottom-_src.top));
	else if(xCoefDest2Src>yCoefDest2Src)
		xOffsetOrg=0.5*((_dest.right-_dest.left)/yCoefDest2Src-(_src.right-_src.left));
	minCoefDest2Src=xCoefDest2Src<yCoefDest2Src?xCoefDest2Src:yCoefDest2Src;
	return true;
}
bool IImageZoomTrans::InitImageZoomTransDataByCenter(const RECT& src,const RECT& dest)
{
	UINT wStdWidth =dest.right-dest.left;
	UINT wStdHeight=dest.bottom-dest.top;
	UINT m_nWidth =src.right-src.left;
	UINT m_nHeight=src.bottom-src.top;
	double scaleofX=wStdWidth/(double)m_nWidth;
	double scaleofY=wStdHeight/(double)m_nHeight;
	if(scaleofX>=scaleofY)
	{
		double gap_x=wStdWidth-m_nWidth*scaleofY;
		int offset_x=f2i(gap_x*0.5);
		int offset_rx=f2i(gap_x-offset_x);
		int offset_y=0;
		return InitImageZoomTransData(src,RECT_C(offset_x,offset_y,wStdWidth-offset_rx,wStdHeight-offset_y));
	}
	else
	{
		int offset_x=0;
		double gap_y=wStdHeight-m_nHeight*scaleofX;
		int offset_y=f2i(gap_y*0.5);
		int offset_ry=f2i(gap_y-offset_y);
		return InitImageZoomTransData(src,RECT_C(offset_x,offset_y,wStdWidth-offset_x,wStdHeight-offset_ry));
	}
}
POINT IImageZoomTrans::TransFromDestImage(int xiDest,int yjDest)
{
	POINT srcpt;
	srcpt.x=f2i(_src.left-xOffsetOrg+(xiDest-_dest.left)/minCoefDest2Src);
	srcpt.y=f2i(_src.top -yOffsetOrg+(yjDest-_dest.top )/minCoefDest2Src);
	return srcpt;
}
bool IImageZoomTrans::IsBlackPixelAtDest(int xiDest,int yjDest,IImage* pSrcImage)
{
	//double minCoefDest2Src=xCoefDest2Src<yCoefDest2Src?xCoefDest2Src:yCoefDest2Src;
	double xifSrcX=_src.left-xOffsetOrg+(xiDest-_dest.left)/minCoefDest2Src;
	double yjfSrcY=_src.top -yOffsetOrg+(yjDest-_dest.top )/minCoefDest2Src;
	//以后应计算四边形面积内的黑白区域比例来精确计算是否为黑点
	double increment=1/minCoefDest2Src;
	double area=increment*increment,blackarea=0;
	int lowi=(int)floor(xifSrcX),highi=(int)ceil(xifSrcX+increment);
	int lowj=(int)floor(yjfSrcY),highj=(int)ceil(yjfSrcY+increment);
	int starti=f2i(xifSrcX),endi=f2i(xifSrcX+increment);
	int startj=f2i(yjfSrcY),endj=f2i(yjfSrcY+increment);
	endi=max(starti+1,endi);
	endj=max(startj+1,endj);
	bool hasPivotBlackPixel=false;
	for(int i=lowi;i<highi;i++)
	{
		if(pSrcImage->IsBlackPixel(i,lowj))
			blackarea-=yjfSrcY-lowj;	//扣除顶部多算黑色区域
		if(pSrcImage->IsBlackPixel(i,highj-1))
			blackarea-=highj-yjfSrcY-increment;	//扣除底部多算黑色区域
		for(int j=lowj;j<highj;j++)
		{
			if(pSrcImage->IsBlackPixel(i,j))
			{
				hasPivotBlackPixel|=(i>=starti&&i<endi&&j>=startj&&j<endj);
				blackarea+=1.0;
			}
		}
	}
	if(blackarea<EPS)
		return false;
	if(hasPivotBlackPixel)
		return true;	//保证不因缩小图像而丢失一些关键黑点像素（笔划较细时，这一点很关键）
	for(int j=lowj;j<highj;j++)
	{
		if(pSrcImage->IsBlackPixel(lowi,j))
			blackarea-=xifSrcX-lowi;	//扣除左侧多算黑色区域
		if(pSrcImage->IsBlackPixel(highi-1,j))
			blackarea-=highi-xifSrcX-increment;	//扣除右侧多算黑色区域
	}
	if(pSrcImage->IsBlackPixel(lowi,lowj))
		blackarea+=(xifSrcX-lowi)*(yjfSrcY-lowj);
	if(pSrcImage->IsBlackPixel(highi-1,lowj))
		blackarea+=(highi-xifSrcX-increment)*(yjfSrcY-lowj);
	if(pSrcImage->IsBlackPixel(highi-1,highj-1))
		blackarea+=(highi-xifSrcX-increment)*(highj-yjfSrcY-increment);
	if(pSrcImage->IsBlackPixel(lowi,highj-1))
		blackarea+=(xifSrcX-lowi)*(highj-yjfSrcY-increment);
	return blackarea/area>=0.5;
	//int xiSrcX=f2i(xifSrcX);
	//int yjSrcY=f2i(yjfSrcY);
	//return pSrcImage->IsBlackPixel(xiSrcX,yjSrcY);
}
//////////////////////////////////////////////////////////////////////////
//STROKELINE
//cnStatBits：由高位向低位统计的bit位数
BYTE STROKELINE::StatTrueBitCount(BYTE ciByte,short siStatBits/*=8*/)
{
	BYTE count=0;
	if(ciByte==0)
		return 0;
	if(ciByte&0x80&&siStatBits>=1)
		count++;
	else if(siStatBits<=1)
		return count;
	if(ciByte&0x40&&siStatBits>=2)
		count++;
	else if(siStatBits<=2)
		return count;
	if(ciByte&0x20&&siStatBits>=3)
		count++;
	else if(siStatBits<=3)
		return count;
	if(ciByte&0x10&&siStatBits>=4)
		count++;
	else if(siStatBits<=4)
		return count;
	if(ciByte&0x08&&siStatBits>=5)
		count++;
	else if(siStatBits<=5)
		return count;
	if(ciByte&0x04&&siStatBits>=6)
		count++;
	else if(siStatBits<=6)
		return count;
	if(ciByte&0x02&&siStatBits>=7)
		count++;
	else if(siStatBits<=7)
		return count;
	if(ciByte&0x01&&siStatBits>=8)
		count++;
	return count;
}
STROKELINE::STROKELINE(const BYTE* pBytes/*=NULL*/,BYTE _ciBitOffset/*=0*/,BYTE _cnBitCount/*=0*/)
{
	cnBitCount=min(_cnBitCount,64);
	if(pBytes==0||_cnBitCount==0)
		bits.i64=0;
	else
		InitBits(pBytes,_ciBitOffset,_cnBitCount);
}
void STROKELINE::InitBits(const BYTE* pBytes,BYTE _ciBitOffset,BYTE _cnBitCount)
{
	int nCopyBytes=_cnBitCount/8;
	int odd=_cnBitCount%8;
	if(odd+_ciBitOffset>0)
		nCopyBytes++;
	if(odd+_ciBitOffset>8)
		nCopyBytes++;
	bits.i64=0;
	memcpy(bits.bytes,pBytes,min(nCopyBytes,8));
	if(_ciBitOffset>0)
		bits.i64<<=_ciBitOffset;
	cnBitCount=min(_cnBitCount,64);
}
BYTE STROKELINE::StatMatchBitCountWith(STROKELINE& strokeline)
{
	__int64 dword=bits.i64&strokeline.bits.i64;
	BYTE* pBytes=(BYTE*)&dword;
	short siBitCount=cnBitCount;
	int match_bits=StatTrueBitCount(*pBytes,cnBitCount);
	match_bits+=StatTrueBitCount(*(pBytes+1),siBitCount-8);
	match_bits+=StatTrueBitCount(*(pBytes+1),siBitCount-16);
	match_bits+=StatTrueBitCount(*(pBytes+1),siBitCount-24);
	if(cnBitCount>32)
	{	
		//memcpy(&pBytes,&bits.bytes[4],4);
		match_bits+=StatTrueBitCount(*(pBytes+0),siBitCount-32);
		match_bits+=StatTrueBitCount(*(pBytes+1),siBitCount-40);
		match_bits+=StatTrueBitCount(*(pBytes+1),siBitCount-48);
		match_bits+=StatTrueBitCount(*(pBytes+1),siBitCount-56);
	}
	return match_bits;
}
static const BYTE byteConstArrH2L[8]={0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};
bool STROKELINE::SetPixelState(int i,bool state/*=true*/)
{
	if(i>=cnBitCount)
		return false;
	int ibyte=i/8,ibit=i%8;
	if(state)
		bits.bytes[ibyte]|=byteConstArrH2L[ibit];
	else
		bits.bytes[ibyte]&=0xff-byteConstArrH2L[ibit];
	return true;
}
bool STROKELINE::GetPixelState(int i)
{
	if(i>=cnBitCount)
		return false;
	int ibyte=i/8,ibit=i%8;
	return (bits.bytes[ibyte]&byteConstArrH2L[ibit])>0;
}
//////////////////////////////////////////////////////////////////////////
//CStrokeFeature
//笔划位图,白(0x00)表示底色,黑(0x01)表示用于笔划像素统计基础像素
CStrokeFeature::CStrokeFeature()
{
	m_lpBitMap=NULL;m_nEffWidth=m_nWidth=m_nHeight=m_id=0;
	m_wiSumBlackPixels=0;	//笔划中黑色像素的统计总(基)数
	rcSearch.left=rcSearch.top=0;		//特征的搜索范围（相对于40*40像素的字符）
	rcSearch.right=40;
	rcSearch.bottom=20;
}
CStrokeFeature::~CStrokeFeature()
{
	if(m_lpBitMap)
		delete []m_lpBitMap;
}
static const BYTE DUALBIT2BYTE[4]={0x01,0x04,0x10,0x40};
static const BYTE _xarrImgDualBit2Byte[4]={0xc0,0x30,0x0c,0x03};
bool CStrokeFeature::SetPixelColor(int i,int j,BYTE ciPixelClrIndex/*=1*/)//0:白色;0x01:黑色;0x02:红色(禁止色);0x03:灰色(正向统计色)
{
	if( i<0||i>=m_nWidth || j<0||j>=m_nHeight)
		return false;
	int iByte=i/4,iBit=i%4;
	BYTE& cbCurrByte=m_lpBitMap[j*m_nEffWidth+iByte];
	cbCurrByte&=(0xff^_xarrImgDualBit2Byte[iBit]);	//清零相应像素位
	ciPixelClrIndex&=0x03;
	ciPixelClrIndex<<=(2*(3-iBit));				//将像素位颜色转换为字节
	cbCurrByte|=ciPixelClrIndex;			//设定相应像素位颜色值
	return (cbCurrByte&ciPixelClrIndex)>0;//如果这个象素是黑色的
}
BYTE CStrokeFeature::GetPixelColor(int i,int j)
{
	if( i<0||i>=m_nWidth || j<0||j>=m_nHeight)
		return 0xff;	//图像越界出错
	int iByte=i/4,iBit=i%4;
	BYTE gray=m_lpBitMap[j*m_nEffWidth+iByte]>>(2*(3-iBit));
	return gray&0x03;
}
bool CStrokeFeature::IsBlackPixel(int i,int j)
{
	BYTE crPixelColor=GetPixelColor(i,j);
	if( crPixelColor>0x03)
		return false;
	return crPixelColor==0x01;
}
bool CStrokeFeature::SetPixelState(int i,int j,bool black/*=true*/)
{
	return SetPixelColor(i,j,black?1:0);
}
bool CStrokeFeature::IsGreyPixel(int i,int j)	//若为黑色进行正向统计的像素
{
	BYTE crPixelColor=GetPixelColor(i,j);
	if( crPixelColor>0x03)
		return false;
	return crPixelColor==0x03;
}
bool CStrokeFeature::IsRedPixel(int i,int j)	//禁止为黑色的像素
{
	BYTE crPixelColor=GetPixelColor(i,j);
	if( crPixelColor>0x03)
		return false;
	return crPixelColor==0x02;
}
bool CStrokeFeature::InitTurboRecogInfo()
{
	for(int i=0;i<this->m_nHeight;i++)
	{
		blckrows.Set(i,0);
		redrows.Set(i,0);
		blckrows[i].cnBitCount=m_nWidth;
		redrows[i].cnBitCount =m_nWidth;
	}
	for(int yJ=0;yJ<m_nHeight;yJ++)
	{
		BYTE* pPixelRow=&m_lpBitMap[yJ*this->m_nEffWidth];
		int maxFeatureWidth=min(32,this->m_nWidth);
		for(int xI=0;xI<maxFeatureWidth;xI++)
		{
			int iByte=xI/4,iBit=xI%4;
			BYTE cbGray=*(pPixelRow+iByte);
			cbGray>>=(3-iBit)*2;
			cbGray&=0x03;
			if(cbGray==CLR::BLACK||cbGray==CLR::GRAY)
				blckrows[yJ].SetPixelState(xI);
			if(cbGray==CLR::RED)
				redrows[yJ].SetPixelState(xI);
		}
	}
	/*CLogFile logf(CXhChar16("d:\\stroke\\%d.txt",this->m_id));
	CLogErrorLife life(&logf);
	for(WORD yj=0;yj<blckrows.Count;yj++)
	{
		for(int i=0;i<this->m_nWidth;i++)
		{
			bool black=blckrows[yj].GetPixelState(i);
			bool red  =redrows[yj].GetPixelState(i);
			if(black&&red)
				logf.LogString("*",false);
			else if(black)
				logf.LogString("B",false);
			else if(red)
				logf.LogString("R",false);
			else
				logf.LogString(" ",false);
		}
		logf.LogString("\n",false);
	}*/
	return true;
}
int CStrokeFeature::PixelRowBytesToBits(const BYTE* rowbytes,BYTE* rowbits,int width)
{
	BYTE byteConstArr[8]={0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};
	int indexOfBytes=0,indexOfBits=0;
	for(int i=0;i<width;i++)
	{
		indexOfBytes=i/8;
		indexOfBits =i%8;
		if(indexOfBits==0)
			rowbits[indexOfBytes]=0;
		if(rowbytes[i]>0)
			rowbits[indexOfBytes]|=byteConstArr[indexOfBits];
	}
	return indexOfBits>0?indexOfBytes+2:indexOfBytes+1;
}
bool CStrokeFeature::RecogInImageChar(CImageChar* pImgChar,BYTE ciMinPercent/*=70*/)
{
	int xiStart=rcSearch.left,xiEnd=rcSearch.right-m_nWidth;
	int yjStart=rcSearch.top,yjEnd=rcSearch.bottom-m_nHeight;
	const BYTE* pCharBytes=pImgChar->GetImageMap();
	POINT xOrgOffset,mostMatchOrg;
	float siMaxMatchHits=0,blckhits=0,redhits=0;
	PRESET_ARRAY64<BYTE>rowbits;
	rowbits.ZeroPresetMemory();
	for(int xI=xiStart;xI<=xiEnd;xI++)
	{
		xOrgOffset.x=xI;
		for(int yJ=yjStart;yJ<yjEnd;yJ++)
		{
			xOrgOffset.y=yJ;
			blckhits=0,redhits=0;
			for(int y=0;y<m_nHeight;y++)
			{
				//TODO:CImageChar每个像素占1个字节而不是1位
				const BYTE* pRowBits=pCharBytes+pImgChar->GetImageEffWidth()*(y+xOrgOffset.y);
				PixelRowBytesToBits(pRowBits,rowbits.Data(),pImgChar->GetImageWidth());
				const BYTE* pBytes=rowbits.Data()+xOrgOffset.x/8;
				int bitOffset=xOrgOffset.x%8;
				blckhits+=blckrows[yJ-yjStart].StatMatchBitCountWith(STROKELINE(pBytes,bitOffset,m_nWidth));
				redhits +=redrows[yJ-yjStart].StatMatchBitCountWith(STROKELINE(pBytes,bitOffset,m_nWidth));
			}
			blckhits-=redhits*1.5f;	//1.5是经验惩罚系数
			if(blckhits>siMaxMatchHits)
			{
				mostMatchOrg=xOrgOffset;
				siMaxMatchHits=blckhits;
			}
		}
	}
	return siMaxMatchHits*100>=m_wiSumBlackPixels*ciMinPercent;//>0.65;
}
char CStrokeFeature::SetLiveStateInChar(WORD wChar,char ciLiveState)
{	//该特征在指定字符中的存在状态,0:无约定;>0在字符指定部位;<0不允许在该字符出现
	LIVESTATE* pState=hashStateInChars.Add(wChar);
	pState->ciState=ciLiveState;
	return ciLiveState;
}
char CStrokeFeature::LiveStateInChar(WORD wChar)
{	//该特征在指定字符中的存在状态,0:无约定;>0在字符指定部位;<0不允许在该字符出现
	LIVESTATE* pState=hashStateInChars.GetValue(wChar);
	if(pState==NULL)
		return 0;
	return pState->ciState;
}
void CStrokeFeature::AddSearchRegion(RECT& _rcSearch)
{
	rcSearch=_rcSearch;
}
bool CStrokeFeature::FromBmpFile(const char* szStrokeBmpFile)
{
	CImageTransform image;
	if(!image.ReadImageFile(szStrokeBmpFile))
		return false;
	m_nWidth=image.GetWidth();
	m_nHeight=image.GetHeight();
	m_nEffWidth=m_nWidth/4;
	if(m_nWidth%4>0)
		m_nEffWidth+=1;
	if(m_lpBitMap)
		delete []m_lpBitMap;
	m_wiSumBlackPixels=0;
	WORD wiSumRedPixels=0;
	int nSize=m_nEffWidth*m_nHeight;
	m_lpBitMap=new BYTE[nSize];
	memset(m_lpBitMap,0,nSize);
	BYTE* lpImgBits=image.GetRawBits();
	for(int j=0;j<m_nHeight;j++)
	{
		BYTE* lpImageRow=&lpImgBits[j*image.GetEffWidth()];
		for(int i=0;i<m_nWidth;i++)
		{
			BYTE r=*(lpImageRow+i*3+2);
			BYTE g=*(lpImageRow+i*3+1);
			BYTE b=*(lpImageRow+i*3+0);
			//BYTE ciGray=(b*114+g*587+r*299)/1000;
			if(r+g+b<10)	//黑色ciGray
			{
				this->SetPixelColor(i,j,CLR::BLACK);
				m_wiSumBlackPixels++;
			}
			else if(r>245&&g+b<10)
			{
				this->SetPixelColor(i,j,CLR::RED);
				wiSumRedPixels++;
			}
			else if(r>245&&g>245&&b>245)	//ciGray
				this->SetPixelColor(i,j,CLR::WHITE);
			else //if(r>245&&g>245&&b>245)
				this->SetPixelColor(i,j,CLR::GRAY);
		}
	}
	return InitTurboRecogInfo();
}
void CStrokeFeature::ToBuffer(CBuffer &buffer,DWORD dwVersion/*=0*/,BOOL bIncSerial/*=TRUE*/)
{
	if(bIncSerial)
		buffer.WriteInteger(m_id);
	buffer.WriteInteger(rcSearch.top);
	buffer.WriteInteger(rcSearch.left);
	buffer.WriteInteger(rcSearch.bottom);
	buffer.WriteInteger(rcSearch.right);
	buffer.WriteInteger(this->m_nWidth);
	buffer.WriteInteger(this->m_nEffWidth);
	buffer.WriteInteger(this->m_nHeight);	//4*8=32
	buffer.WriteWord(m_wiSumBlackPixels);		//34
	buffer.Write(this->m_lpBitMap,m_nEffWidth*m_nHeight);
	buffer.WriteWord((WORD)hashStateInChars.GetNodeNum());
	for(LIVESTATE* pState=hashStateInChars.GetFirst();pState;pState=hashStateInChars.GetNext())
	{
		buffer.WriteWord(pState->wChar);
		buffer.WriteByte(pState->ciState);
	}
}
UINT CStrokeFeature::ToBLOB(char* blob,UINT uBlobBufSize,DWORD dwVersion/*=0*/)
{
	//buffer.WriteInteger(rcSearch.top);
	//buffer.WriteInteger(rcSearch.left);
	//buffer.WriteInteger(rcSearch.bottom);
	//buffer.WriteInteger(rcSearch.right);
	//buffer.WriteInteger(this->m_nWidth);
	//buffer.WriteInteger(this->m_nEffWidth);
	//buffer.WriteInteger(this->m_nHeight);
	//buffer.WriteWord(wiSumBlackPixels);
	//4*7+2=30;
	UINT nSize=30+m_nEffWidth*m_nHeight+2+hashStateInChars.Count*3;
	if(blob==NULL)
		return nSize;
	CBuffer buff(blob,uBlobBufSize);
	ToBuffer(buff,dwVersion,false);
	return min(uBlobBufSize,nSize);
}
void CStrokeFeature::FromBuffer(CBuffer &buffer,DWORD dwVersion/*=0*/,BOOL bIncSerial/*=TRUE*/)
{
	if(bIncSerial)
		buffer.ReadInteger(&m_id);
	buffer.ReadInteger(&rcSearch.top);
	buffer.ReadInteger(&rcSearch.left);
	buffer.ReadInteger(&rcSearch.bottom);
	buffer.ReadInteger(&rcSearch.right);
	buffer.ReadInteger(&this->m_nWidth);
	buffer.ReadInteger(&this->m_nEffWidth);
	buffer.ReadInteger(&this->m_nHeight);
	buffer.ReadWord(&m_wiSumBlackPixels);
	UINT size=m_nEffWidth*m_nHeight;
	if(m_lpBitMap)
		delete []m_lpBitMap;
	m_lpBitMap=new BYTE[size];
	buffer.Read(this->m_lpBitMap,size);
	WORD wChar,i,count=0;
	buffer.ReadWord(&count);
	hashStateInChars.Empty();
	for(i=0;i<count;i++)
	{
		buffer.ReadWord(&wChar);
		LIVESTATE* pState=hashStateInChars.Add(wChar);
		buffer.ReadByte(&pState->ciState);
	}
};
void CStrokeFeature::FromBLOB(char* blob,UINT uBlobSize,DWORD dwVersion/*=0*/)
{
	if(blob==NULL)
		return;// false;
	CBuffer buff(blob,uBlobSize);
	FromBuffer(buff,dwVersion,false);
}
//////////////////////////////////////////////////////////////////////////
//CImageChar
CImageChar::CImageChar(WORD fontserial/*=0*/)
{
	_nDestValidWidth=0;
	m_nBlckPixels=m_nPixels=wTopBlackPixels=wBtmBlackPixels=0;
	xiImgLeft=xiImgRight=0;
	m_lpBitMap=NULL;
	wChar.wHz=0;
	m_bTemplEnabled=true;
	linknext=NULL;
	_serial=fontserial;
	//IsPixelBitMode(){return false;}	字符类对象默认每个字节代表一个像素。
	//BlackBitIsTrue(){return true;}	字符类对象默认黑点用True或1表示
}
CImageChar::~CImageChar()
{
	if(m_lpBitMap)
		delete []m_lpBitMap;
	m_lpBitMap=NULL;
	RemoveChild();
}
CImageChar* CImageChar::AppendFontChar(WORD fontserial/*=0*/)
{
	WORD _fontserial=fontserial&0x7fff;
	WORD _src_fontserial=(fontserial&0x8000)>0?0:_fontserial;
	if(_serial!=0&&_serial==_fontserial)
		return this;
	else if(linknext==NULL)
	{
		if(_src_fontserial==0)
			fontserial=max(_fontserial,_serial)+1;
		linknext=new CImageChar(fontserial);
		linknext->wChar=wChar;
		return linknext;
	}
	else
	{
		if(_src_fontserial==0)
		{
			_fontserial=max(_fontserial,_serial);
			fontserial=_fontserial|0x8000;
		}
		return linknext->AppendFontChar(fontserial);
	}
}
bool CImageChar::RemoveChild()
{
	if(linknext==NULL)
		return true;
	delete linknext;
	linknext=NULL;
	return true;
}
CImageChar* CImageChar::FontChar(WORD fontserial)
{
	if(fontserial==_serial)
		return this;
	else if(linknext!=NULL)
		return linknext->FontChar(fontserial);
	else
		return NULL;
}
bool CImageChar::IsBlackPixel(int i,int j)
{
	if(i<0||j<0||i>=m_nWidth||j>=m_nHeight)
		return false;
	//字节低4位表示像素为黑或白，高4位表示一些附加信息（如孤岛探测时的边界连通特征）
	return (m_lpBitMap[j*m_nWidth+i]&0x0f)!=0;
}
void CImageChar::CopyImageChar(CImageChar* pChar)
{
	int nOldPixels=m_nPixels;
	m_nEffWidth=m_nWidth=pChar->GetImageWidth();
	m_nHeight=pChar->GetImageHeight();
	xiImgLeft=pChar->xiImgLeft;
	xiImgRight=pChar->xiImgRight;
	m_nPixels=m_nWidth*m_nHeight;
	if(m_lpBitMap!=NULL&&nOldPixels!=m_nPixels)
	{
		delete []m_lpBitMap;
		m_lpBitMap=NULL;
	}
	if(m_lpBitMap==NULL)	//需要重新分配内存
		m_lpBitMap=new BYTE[m_nPixels];
	memcpy(m_lpBitMap,pChar->GetImageMap(),m_nPixels);
	m_nBlckPixels=pChar->m_nBlckPixels;
	_nDestValidWidth=pChar->m_nDestValidWidth;
	wChar=pChar->wChar;
}
void CImageChar::InitTemplate(BYTE* lpBits,int eff_width,int width,RECT tem_rect)
{
	bool bHasBlack=false,bBlackPixelAppeared=false;
	CBitImage monoimg(lpBits,width,tem_rect.bottom,eff_width,false);
	RECT rect=RECT_C();
	for(int i=tem_rect.left;i<tem_rect.right;i++)
	{
		bHasBlack=false;
		for(int j=tem_rect.top;j<tem_rect.bottom;j++)
		{
			if(monoimg.IsBlackPixel(i,j))	
			{
				bHasBlack=true;
				if(bBlackPixelAppeared==false)
				{
					rect.left=i;
					bBlackPixelAppeared=true;
				}
				else
					break;
			}
		}
		if(bBlackPixelAppeared==true&&bHasBlack==false)
		{   
			rect.right=i;
			break;
		}
		if(i==tem_rect.right-1)
		{
			rect.right=tem_rect.right;
			break;
		}
	}
	//
	if(wChar.wHz=='-'||wChar.wHz=='_'||wChar.wHz=='.'||wChar.wHz=='x')
	{	//这三个字符上下位置有特殊要求，不能随便定位
		rect.top=tem_rect.top;
		rect.bottom=tem_rect.bottom;
	}
	else
	{
		bBlackPixelAppeared=false;
		for(int j=tem_rect.top;j<tem_rect.bottom;j++)
		{
			bHasBlack=false;
			for(int i=tem_rect.left;i<tem_rect.right;i++)
			{
				if(monoimg.IsBlackPixel(i,j))	
				{
					bHasBlack=true;
					if(bBlackPixelAppeared==false)
					{
						rect.top=j;
						bBlackPixelAppeared=true;
					}
					else
						break;
				}
			}
			if(bBlackPixelAppeared==true&&bHasBlack==false)
			{   
				rect.bottom=j;
				break;
			}
			if(j==tem_rect.bottom-1)
			{
				rect.bottom=tem_rect.bottom;
				break;
			}
		}
	}
	Localize(lpBits,eff_width,rect);
}
void CImageChar::Localize(BYTE* lpBits,int eff_width,RECT rect)
{
	m_nHeight=max(0,rect.bottom-rect.top);
	m_nEffWidth=m_nWidth=(rect.right-rect.left);
	m_nPixels=m_nWidth*m_nHeight;
	if(m_lpBitMap)
		delete []m_lpBitMap;
	m_lpBitMap=new BYTE[m_nPixels];
	memset(m_lpBitMap,0,m_nPixels);
	m_nBlckPixels=0;
	CBitImage monoimg(lpBits,eff_width*8,rect.bottom,eff_width,false);
	for(int i=rect.left;i<rect.right;i++)
	{
		for(int j=rect.top;j<rect.bottom;j++)
		{
			if(monoimg.IsBlackPixel(i,j))	
			{
				int xI1=i-rect.left,yJ1=j-rect.top;
				m_lpBitMap[yJ1*m_nWidth+xI1]=1;
				m_nBlckPixels++;
			}
		}
	}
}
void CImageChar::Standardize(WORD wStdWidth,WORD wStdHeight)
{
	RECT rc;
	rc.left=rc.top=0;
	rc.right=m_nWidth;//rect.right-rect.left;
	rc.bottom=m_nHeight;//rect.bottom-rect.top;
	InitImageZoomTransDataByCenter(rc,RECT_C(0,0,wStdWidth,wStdHeight));
	const int POOLSIZE=4096;
	static BYTE datapool[POOLSIZE];	//64*64=4096,预存静态变量避免频繁用new-delete分配释放内存 wjh-2018.4.11
	memset(datapool,0,POOLSIZE);
	UINT pixels_count=wStdWidth*wStdHeight;
	IMGPIXELS<BYTE> pixels(wStdWidth,wStdHeight,datapool,POOLSIZE);
	//BYTE_ARRAY bitsmap(wStdWidth*wStdHeight);
	m_nBlckPixels=0;
	for(int i=0;i<(int)wStdWidth;i++)
	{
		for(int j=0;j<(int)wStdHeight;j++)
		{
			if(IsBlackPixelAtDest(i,j,this))
			{
				pixels[j][i]=1;//[j*wStdWidth+i]=1;
				m_nBlckPixels++;
			}
		}
	}
	if(pixels_count>(DWORD)m_nPixels)
	{
		if(m_lpBitMap!=NULL)
			delete []m_lpBitMap;
		m_lpBitMap=new BYTE[pixels_count];
		m_nPixels=pixels_count;
	}
	m_nEffWidth=m_nWidth=wStdWidth;
	m_nHeight=wStdHeight;
	memcpy(m_lpBitMap,datapool,pixels_count);
}
struct PREFER_BAND{
	int yStart,yEnd;
	bool initStart,initEnd;
	void Clear(){
		yStart=yEnd=0;
		initStart=initEnd=false;
	}
	void VerifyPixelY(int iY){
		if(!initStart)
		{
			yStart=yEnd=iY;
			initStart=initEnd=true;
		}
		else if(iY>yEnd)
			yEnd=iY;
	};
	int get_Height(){
		if(initStart)
			return yEnd-yStart+1;
		else
			return 0;
	}
	__declspec(property(get=get_Height)) int Height;
	PREFER_BAND(){Clear();}
};
//对字符进行规则化处理
void CImageChar::StandardLocalize(BYTE* lpBits,bool blackbit_is_true,int eff_width,RECT& rc,int wStdHeight,bool onlyHighestBand/*=true*/)
{
	//rect=rc;
	CBitImage bitmap(lpBits,eff_width*8,rc.bottom,eff_width,blackbit_is_true);
	PREFER_BAND band,preferband;
	int bandcount=0;
	int height=rc.bottom-rc.top+1;
	int mid_band_starty=rc.top+f2i(height*0.3);
	int mid_band_endy=rc.bottom-f2i(height*0.3);
	for(int yJ=rc.top;yJ<=rc.bottom;yJ++)
	{
		//中间部位不过滤,该函数仅用于剔除上下边界干扰 wjh-2018.4.18
		bool hasBlckPixels=(yJ>=mid_band_starty||yJ<=mid_band_endy);	//是否有黑点
		for(int xI=rc.left;!hasBlckPixels&&xI<=rc.right;xI++)
		{
			if(bitmap.IsBlackPixel(xI,yJ))
				hasBlckPixels=true;
		}
		if(hasBlckPixels)
		{
			if(!band.initStart)
				bandcount++;
			band.VerifyPixelY(yJ);
		}
		else
		{
			if(preferband.Height<band.Height)
				preferband=band;
			band.Clear();
		}
	}
	if(preferband.Height>0&&bandcount>1)
	{	//剔除'-'或'_'类字符特殊情况
		rc.top=preferband.yStart;
		rc.bottom=preferband.yEnd;
	}
	bitmap.SetClipRect(rc);
	xiImgLeft=rc.left;
	xiImgRight=rc.right;
	m_nEffWidth=m_nWidth=m_nHeight=wStdHeight;
	int nNewPixels=wStdHeight*wStdHeight;
	if(m_lpBitMap&&m_nPixels!=nNewPixels)
	{
		delete []m_lpBitMap;
		m_lpBitMap=NULL;
	}
	if(m_lpBitMap==NULL)
		m_lpBitMap=new BYTE[nNewPixels];
	m_nPixels=nNewPixels;
	InitImageZoomTransDataByCenter(rc,RECT_C(0,0,wStdHeight,wStdHeight));
	m_nBlckPixels=0;
	_nDestValidWidth=0;
	int left=m_nMaxTextH,right=0;
	bool BLACK_BIT=BlackBitIsTrue();
	bool WHITE_BIT=!BLACK_BIT;
	for(int i=0;i<m_nWidth;i++)
	{
		for(int j=0;j<m_nHeight;j++)
		{
			m_lpBitMap[j*m_nWidth+i]=WHITE_BIT;	//默认为白色像素位
			//POINT pt=TransFromDestImage(i,j);
			//if(pt.x<rc.left || pt.x>=rc.right || pt.y<rc.top|| pt.y>=rc.bottom)
			//	continue;
			//int iByte=pt.x/8,iBit=pt.x%8;
			//int gray=lpBits[pt.y*eff_width+iByte]&BIT2BYTE[7-iBit];	//获取该点的灰度值
			//if(gray!=BIT2BYTE[7-iBit])	//如果这个象素是黑色的
			if(IsBlackPixelAtDest(i,j,&bitmap))
			{
				left =i<left ? i : left;
				right=i>right? i : right;
				m_lpBitMap[j*m_nWidth+i]=BLACK_BIT;	//
				m_nBlckPixels++;
			}
		}
	}
	_nDestValidWidth=right-left+1;
	return;
	//计算规则化区域
	/*int w=0,w1=0,w2=0;
	w=m_nMaxTextH-(rect.right-rect.left);
	if(w>1)
	{
		w1=w/2;
		w2=w-w1;
	}
	else
		w1=w;
	int h=0,h1=0,h2=0;
	h=m_nMaxTextH-(rect.bottom-rect.top);
	if(h>1)
	{
		h1=h/2;
		h2=h-h1;
	}
	else
		h1=h;
	//初始化像素特征值
	for(int i=0;i<m_nMaxTextH;i++)
	{
		for(int j=0;j<m_nMaxTextH;j++)
		{
			m_lpBitMap[j*m_nMaxTextH+i]=0;
			if(i<w1 || i>w1+(rect.right-rect.left))
				continue;
			if(j<h1 || j>h1+(rect.bottom-rect.top))
				continue;
			int x=rect.left+i-w1;
			int y=rect.top+j-h1;
			int iByte=x/8,iBit=x%8;
			int gray=lpBits[y*eff_width+iByte]&BIT2BYTE[7-iBit];	//获取该点的灰度值
			if(gray!=BIT2BYTE[7-iBit])	//如果这个象素是黑色的
			{
				m_lpBitMap[j*m_nMaxTextH+i]=1;
				m_nBlckPixels++;
			}
		}
	}*/
}
MATCHCOEF::MATCHCOEF(double originalCoef/*=0.0*/,double weightingCoef/*=0.0*/)
{
	original=originalCoef;
	weighting=weightingCoef;
}
int MATCHCOEF::CompareWith(const MATCHCOEF& coef)
{	//比对原则：以修正后为基准，但修正后差别不大时，以修正前为基准
	double scale=coef.weighting/weighting;
	if(scale>1.05)
		return -1;
	else if(scale<0.952381)
		return 1;
	double scale2=coef.original/original;
	if(scale2>1.05)
		return -1;
	else if(scale2<0.952381)
		return 1;
	double scale12=fabs(scale-1)<fabs(scale2-1)?scale2:scale;
	if(scale12>1)
		return -1;
	else
		return 1;
}
bool CImageChar::IsDetect(int i,int j)
{
	return (0x80&m_lpBitMap[j*m_nWidth+i])>0;
}
bool CImageChar::SetDetect(int i,int j,bool detect/*=true*/)
{
	BYTE* pxyPixel=&m_lpBitMap[j*m_nMaxTextH+i];
	if(detect)
		(*pxyPixel)|=0x80;
	else if((*pxyPixel)>=0x80)
		(*pxyPixel)-=0x80;
	return ((*pxyPixel)&0x80)!=0;
}
void CImageChar::ClearDetectMark()
{
	for(int i=0;i<m_nWidth;i++)
		for(int j=0;j<m_nHeight;j++)
			m_lpBitMap[j*m_nWidth+i]&=0x0f;
}
bool CImageChar::PIXEL::get_Black()
{
	return pcbPixel==NULL?false:((*pcbPixel)&0x0f)>0;
}
bool CImageChar::PIXEL::set_Black(bool black)
{
	if(pcbPixel&&black)
		*pcbPixel|=0x01;
	return pcbPixel!=NULL&&black;
}
char CImageChar::PIXEL::set_Connected(char connected)		//该像素是否为连通区域的像素
{
	if(pcbPixel==NULL)
		return NONEVISIT;
	if(connected==NONEVISIT)
	{
		(*pcbPixel)&=0x0f;
		return connected;
	}
	else if(connected==CONNECTED)
		(*pcbPixel)|=0xc0;	//0xc0=0x80|0x40
	else if(connected==UNDECIDED)
		(*pcbPixel)|=0x80;
	else
		(*pcbPixel)&=0x0f;
	return connected;
}
char CImageChar::PIXEL::get_Connected()
{
	if(pcbPixel==NULL)
		return NONEVISIT;
	BYTE cbFlag=(*pcbPixel);
	if(!(cbFlag&0x80))
		return NONEVISIT;
	if(cbFlag&0x40)
		return CONNECTED;
	else
		return UNDECIDED;
}
//该像素是否为连通区域的像素
int island_id=1;
//实践证明listStatPixels采用预置数组比CXhSimpleList内存分配效率更高 wjh-2018.4.12
bool CImageChar::StatNearWhitePixels(int i,int j,ISLAND* pIsland,PRESET_ARRAY2048<PIXEL>* listStatPixels,BYTE cDetectFlag/*=0x0f*/,CLogFile* pLogFile/*=NULL*/)
{
	BYTE* pxyPixel=&m_lpBitMap[j*m_nWidth+i];
	PIXEL pixel(pxyPixel);
	if(i==0||j==0||i==m_nWidth-1||j==m_nHeight-1)
	{
		if(j==0)
			StatNearWhitePixels(i,1,pIsland,listStatPixels,cDetectFlag);
		if(pLogFile)
			pLogFile->Log("%d#(%2d,%2d)=false",island_id,i,j);
		return pixel.Black;//false;	//白色像素与边界连通在一起，无法再形成白色孤岛
	}
	if(pixel.Black)
		return true;	//当前为黑点停止统计搜索
	if(pixel.Connected==PIXEL::CONNECTED)
		return false;	//已检测过并确认为与边界连通的白点
	if(pixel.Connected==PIXEL::UNDECIDED)
		return true;	//已检测过的节点
	pixel.Connected=PIXEL::UNDECIDED;	//*pxyPixel|=0x80;	设置已检测标记
	if(listStatPixels)
		listStatPixels->Append(pixel);//AttachObject(pixel);
	if(pLogFile)
		pLogFile->Log("(%2d,%2d)=%d",i,j,island_id);
	pIsland->x+=i;		//累积形心计算参数值
	pIsland->y+=j;
	if(pIsland->count==0)
		pIsland->maxy=pIsland->miny=j;
	else
	{
		pIsland->maxy=max(pIsland->maxy,j);
		pIsland->miny=min(pIsland->miny,j);
	}
	pIsland->count+=1;
	bool canconinue=true;
	for(int movJ=-1;movJ<=1;movJ++)
	{
		int jj=movJ+j;
		if(jj<0||jj>=m_nHeight)
			continue;
		if(jj<j&&!(cDetectFlag&DETECT_UP)||jj>j&&!(cDetectFlag&DETECT_DOWN)||jj==j&&!(cDetectFlag&DETECT_Y0))
			continue;
		for(int movI=-1;movI<=1;movI++)
		{
			int ii=movI+i;
			//移除abs(movJ)==abs(movI)情况，因为可能存在白点孤岛个别边界角为黑点对角白点也对角情况 wjh-2018.3.28
			if(ii<0||ii>=m_nWidth||(ii==i&&jj==j)||abs(movJ)==abs(movI))
				continue;
			if(ii<i&&!(cDetectFlag&DETECT_LEFT)||ii>i&&!(cDetectFlag&DETECT_RIGHT)||ii==i&&!(cDetectFlag&DETECT_X0))
				continue;
			if(!(canconinue=StatNearWhitePixels(ii,jj,pIsland,listStatPixels,cDetectFlag)))
				return false;
		}
	}
	return canconinue;
}
WORD CImageChar::StatLocalBlackPixelFeatures()
{
	int pixels=0, height2=f2i(m_nHeight*0.2);
	wTopBlackPixels=0;	//顶部20%区域，取行内最多的黑点数
	rcFeatureActual.top=m_nHeight;
	rcFeatureActual.left=m_nWidth;
	rcFeatureActual.right=rcFeatureActual.bottom=0;
	for(int j=0;j<m_nHeight;j++)
	{
		pixels=0;
		for(int i=0;i<m_nWidth;i++)
		{
			if(IsBlackPixel(i,j))
			{
				pixels=robot.Shell_AddLong(pixels,1);//pixels++;
				if(rcFeatureActual.left>i)  rcFeatureActual.left	=i;
				if(rcFeatureActual.right<i) rcFeatureActual.right	=i;
				if(rcFeatureActual.bottom<j)rcFeatureActual.bottom	=j;
				if(rcFeatureActual.top>j)	rcFeatureActual.top 	=j;
			}
		}
		if(j<=height2)	//取行内最多的黑点数
			wTopBlackPixels=max(wTopBlackPixels,pixels);
	}
	wBtmBlackPixels=0;
	for(int j=m_nHeight-1;j>=m_nHeight-height2;j--)
	{
		pixels=0;
		for(int i=0;i<m_nWidth;i++)
		{
			if(IsBlackPixel(i,j))
				pixels=robot.Shell_AddLong(pixels,1);//++;
		}
		wBtmBlackPixels=max(wBtmBlackPixels,pixels);
	}
	return wBtmBlackPixels;
}
int CImageChar::DetectIslands(CXhSimpleList<ISLAND>* listIslands)
{
	int count=0;
	int rowstarti=0;
	//CLogFile logfile(CXhChar50("c:/%s_detect.txt",wChar.chars));
	//CLogErrorLife life(&logfile);
	ClearDetectMark();
	island_id=1;
	for(int j=0;j<m_nHeight;j++)
	{
		ISLAND island;
		bool blackpixelStarted=false,blackpixelEnded=false;
		bool whitepixelStarted=false;
		for(int i=0;i<m_nWidth;i++)
		{
			if(m_lpBitMap[rowstarti+i]==1)
			{
				blackpixelStarted=true;
				if(whitepixelStarted)
					blackpixelEnded=true;
			}
			else if(blackpixelStarted&&m_lpBitMap[rowstarti+i]==0)
			{
				whitepixelStarted=true;
				PRESET_ARRAY2048<PIXEL> listStatPixels;
				island.Clear();
				//不能添加DETECT_X0|DETECT_Y0，因为可能存在白点孤岛个别边界角为黑点对角白点也对角情况 wjh-2018.3.28
				if(StatNearWhitePixels(i,j,&island,&listStatPixels,DETECT_UP|DETECT_Y0|DETECT_DOWN|DETECT_LEFT|DETECT_X0|DETECT_RIGHT))//,&logfile))
				{	//查到的白点集合为孤岛
					listIslands->AttachObject(island);
					//logfile.Log("%d#island finded @%2d-%2d",island_id,i,j);
					island_id++;
					count++;
				}
				else
				{	//查到的白点集合实际与边界连通，需设定边界标记
					for(UINT ii=0;ii<listStatPixels.Count;ii++)
					{
						PIXEL* pPixel=&listStatPixels.At(ii);
						pPixel->Connected=PIXEL::CONNECTED;
					}
				}
			}
			else if((m_lpBitMap[rowstarti+i]&0x7f)==0)
			{
				PRESET_ARRAY2048<PIXEL> listStatPixels;
				StatNearWhitePixels(i,j,&island,&listStatPixels,DETECT_UP|DETECT_Y0|DETECT_DOWN|DETECT_LEFT|DETECT_X0|DETECT_RIGHT);//,&logfile);
				for(UINT ii=0;ii<listStatPixels.Count;ii++)
				{
					PIXEL* pPixel=&listStatPixels.At(ii);
					pPixel->Connected=PIXEL::CONNECTED;
				}
				island.Clear();
			}
			//if(blackpixelStarted&&whitepixelStarted&&blackpixelEnded)
		}
		rowstarti+=m_nWidth;
	}
	ClearDetectMark();
	return count;
}
int CImageChar::CheckFeatures(CXhSimpleList<ISLAND>* pIslands/*=NULL*/)
{
	CXhSimpleList<ISLAND> islands;
	int count=0;
	ISLAND* pIsland=NULL,*pFirstValidIsland=NULL,*pSecValidIsland=NULL;
	if(pIslands==NULL)
	{
		pIslands=&islands;
		count=DetectIslands(&islands);
	}
	double MIN_ISLAND_PIXELS=3*m_nHeight/17.0;	//最小白点孤岛的有效像素数，用于避免因标准化时缩小尺寸额外产生的白点孤岛 wjh-2018.3.28
	for(pIsland=pIslands->EnumObjectFirst();pIsland;pIsland=pIslands->EnumObjectNext())
	{
		if(pIsland->count<MIN_ISLAND_PIXELS)
			continue;
		if(pFirstValidIsland==NULL)
			pFirstValidIsland=pIsland;
		else if(pSecValidIsland==NULL)
			pSecValidIsland=pIsland;
		count++;
	}
	double core_y=0;
	if(pIsland=pFirstValidIsland)
		core_y=pIsland->y/pIsland->count;
	double vfScaleCoef=m_nHeight/17.0;
	if(wChar.wHz=='0')
	{
		if(count==0)	//个别细笔划字体在标准化时缩小后会出现不闭合情况，此时中间白色孤岛会丢失，需要后结仔细判断 wjh-2018.4.2
			return -2;
		if(count!=1)
			return -1;
		else if(pIsland->miny>m_nHeight*0.4||pIsland->maxy<m_nHeight*0.7)	//'0'孤岛上点有时也偏下，故上值取0.4较0.3更合理
			return -1;	//孤岛的上下顶点不在合理区域内（用于区分0与6　wjh-2018.3.18
		else if(core_y<=m_nHeight*0.4&&core_y>=m_nHeight*0.6)
			return -1;
		else if(pIsland->count>4)
			return 0;//1;
	}
	else if(wChar.wHz=='1')
	{
		if(pIsland!=NULL)
			return -1;	//'1'字符不可能有孤岛存在
		if(m_nDestValidWidth>m_nWidth/2)
			return -1;	//1字符过宽可能是精度不高的'4'字符　wjh-2018.3.18
		if(wTopBlackPixels>f2i(0.35*m_nWidth))	//'1顶部有时候会向左突出一部分宽度，故取值0.35较好 wjh-2018.4.19
			return -1;	//'1'顶部黑点过多，可能是字符'7'
		int yiStart=(int)(0.3*m_nHeight);
		int yiEnd=m_nHeight-yiStart+1;
		int irow,icol,iPrevBlackPixelStart=0,iPrevBlackPixelEnd=0;
		for(irow=yiStart;irow<=yiEnd;irow++)
		{
			int iBlackPixelStart=-1,iBlackPixelEnd=0;
			for(icol=0;icol<m_nWidth;icol++)
			{
				if(IsBlackPixel(icol,irow))
				{
					if(iBlackPixelStart<0)
						iBlackPixelStart=icol;
					iBlackPixelEnd=max(iBlackPixelEnd,icol);
				}
				else if(iBlackPixelStart>=0)
					break;	//黑点已结束
			}
			if(irow==yiStart)
			{
				iPrevBlackPixelStart=iBlackPixelStart;
				iPrevBlackPixelEnd  =iBlackPixelEnd;
				continue;
			}
			int minPixelX=max(iPrevBlackPixelStart,iBlackPixelStart);
			int maxPixelX=max(iPrevBlackPixelEnd,  iBlackPixelEnd);
			if(maxPixelX-minPixelX+1<1)
				return -1;	//'1'字符中间不能出现中断或黑点过少现象
			iPrevBlackPixelStart=iBlackPixelStart;
			iPrevBlackPixelEnd  =iBlackPixelEnd;
		}
	}
	else if(wChar.wHz=='2')
	{
		if(pIsland!=NULL)
			return -1;	//'2'字符不可能有孤岛存在
		if(wBtmBlackPixels<f2i(0.3*m_nWidth))
			return -1;	//'2'底部黑点过少，可能是字符'7'
		//1.目前暂时统计字符右下30/40~32/40之间的区域，黑点像素数不能大于5 wjh-2018.3.27
		//发现个别字体笔划较粗，2字符底线较粗，故抬高检测区下限Y值 wjh-2018.11.16
		int yiStart=30,yiEnd=32;
		int rightblacks=0,xiMid=m_nWidth/2;
		int moveright=f2i(2*vfScaleCoef);
		int maxblackpixels=f2i(2*vfScaleCoef);
		for(int irow=yiStart;irow<=yiEnd;irow++)
		{
			for(int icol=xiMid+moveright;icol<m_nWidth;icol++)
			{
				if(IsBlackPixel(icol,irow))
					rightblacks++;
				if(rightblacks>maxblackpixels)
					return -1;	//右上侧开放区域黑点过多超出误差噪点范围，不认为是'2'，可能是'3'
			}
		}
	}
	else if(wChar.wHz=='3')
	{
		if(pIsland!=NULL)
			return -1;	//'3'字符不可能有孤岛存在
		int yiStart,yiEnd,irow,maxblackpixels,icol,xiMid=m_nWidth/2;
		//1.目前暂时统计字符左下8/17~11/17之间的区域，黑点像素数不能大于4 wjh-2018.3.27
		yiStart=(int)f2i(0.4705*m_nHeight);	//0.4705=8/17	,部分PDF字体中线下移故调为8/17更合适
		yiEnd=(int)f2i(0.6471*m_nHeight);	//0.6471=11/17
		int iPrevBlackPixelStart=0,iPrevBlackPixelEnd=0;
		int leftblacks=0,moveleft=f2i(2*vfScaleCoef);	//由于'3'字符右下角可能存在粘连故起始统计像素x左移
		maxblackpixels=f2i(3*m_nHeight/17.0);
		for(irow=yiStart;irow<=yiEnd;irow++)
		{
			for(icol=0;icol<xiMid-moveleft;icol++)
			{
				if(IsBlackPixel(icol,irow))
					leftblacks++;
				if(leftblacks>=maxblackpixels)
					return -1;	//左下侧开放区域黑点过多超出误差噪点范围，不认为是'3'
			}
		}
		//2.目前暂时统计字符右上3/17~4/17之间的区域，黑点像素数不能少于3 wjh-2018.8.31
		yiStart=(int)f2i(0.1765*m_nHeight);	//0.1765=3/17
		yiEnd  =(int)f2i(0.2353*m_nHeight);	//0.2353=4/17
		int moveright=f2i(1.5*vfScaleCoef);	//由于'5'字符左上角可能存在粘连故起始统计像素x右移
		int rightblacks=0,minblackpixels=f2i(2*m_nHeight/17.0);
		for(irow=yiStart;irow<=yiEnd;irow++)
		{
			for(icol=xiMid+moveright;icol<m_nWidth;icol++)
			{
				if(IsBlackPixel(icol,irow))
					rightblacks++;
				if(rightblacks>=minblackpixels+1)
					break;
			}
		}
		if(rightblacks<minblackpixels)
			return -1;	//右上侧开放区域黑点过少，缺少'3'字符必要特征，不认为是'3'
	}
	else if(wChar.wHz=='4')
	{
		bool hasBtmIsland=false;
		double splitY=f2i(m_nHeight*0.6);
		for(ISLAND* pIsland=pIslands->EnumObjectFirst();pIsland;pIsland=pIslands->EnumObjectNext())
		{
			if(pIsland->count>=MIN_ISLAND_PIXELS&&(pIsland->maxy+pIsland->miny)/2>splitY)
				return -1;	//无论如何字符'4'的空白孤岛也不可能出现在字符下半部
		}
		//if(count>1)	//由于4字符中间空白孤岛较小存在蜕普通为全黑或分隔为多个空白孤岛可能  wjh-2018.3.27
		//	return -1;
		if(count<=0)
		{
			//1.为了区分'1'与'4'字符，'4'字符至少占6/17个像素宽度
			//2.由于个别情况４字符处理灰度->黑白图时导致字符减小，故在原有半格基础上再减1个像素 wjh-2016.5.25
			int MIN_CHAR4_WIDTH=f2i(0.3529*m_nHeight);	//0.3529=6/17
			if(m_nDestValidWidth<MIN_CHAR4_WIDTH)
				return -1;
			return 0;	//由于图像转换，可能导致中间的孤岛在转为黑白图时丢失
		}
		else if(count==1&&(core_y>m_nHeight*0.6||core_y<m_nHeight*0.4))
			return -1;	//'4'字符的孤岛应该在中间范围,以示与'6'及'9'等字符区分
		else
			return 0;
		//if(pIsland!=NULL)
		//	return -1;	//'1'字符不可能有孤岛存在
	}
	else if(wChar.wHz=='5')
	{
		int yiStart,yiEnd,irow,maxblackpixels,icol,xiMid=m_nWidth/2;
		if(count>1)
			return -1;
		else if(count==1)
		{
			//'5'字符原则上不可能有孤岛存在，但部分情况如305分割字符时，由于0与5粘连在一块可能导致一个错误孤岛，故设为孤岛>2为肯定不合理
			//目前看字符分割问题导致的识别错误，可以通过提高图像精度及，功能数字特征识别来解决,但６与５字符由于目前字库问题确实存在匹配度不容易区分问题　wjh-2018.3.18
			//目前暂时统计字符左下9/17~12/17之间的区域，黑点像素数不能大于5 wjh-2018.3.27
			yiStart=(int)f2i(0.5294*m_nHeight);	//0.5294=9/17
			yiEnd=(int)f2i(0.7059*m_nHeight);	//0.7059=12/17
			int iPrevBlackPixelStart=0,iPrevBlackPixelEnd=0;
			int leftblacks=0;
			maxblackpixels=f2i(5*vfScaleCoef);
			for(irow=yiStart;irow<=yiEnd;irow++)
			{
				for(icol=0;icol<xiMid;icol++)
				{
					if(IsBlackPixel(icol,irow))
						leftblacks++;
					if(leftblacks>maxblackpixels)
						return -1;	//左下侧开放区域黑点过多超出误差噪点范围，不认为是'5',可能是'6'
				}
			}
		}
		//目前暂时统计字符右上3/17~4/17之间的区域，黑点像素数不能大于2 wjh-2018.3.27
		yiStart=(int)f2i(0.1765*m_nHeight);	//0.1765=3/17
		yiEnd  =(int)f2i(0.2353*m_nHeight);	//0.2353=4/17
		int rightblacks=0;
		int moveright=f2i(2*vfScaleCoef);	//由于'5'字符左上角可能存在粘连故起始统计像素x右移
		maxblackpixels=f2i(2*vfScaleCoef);
		for(irow=yiStart;irow<=yiEnd;irow++)
		{
			for(icol=xiMid+moveright;icol<m_nWidth;icol++)
			{
				if(IsBlackPixel(icol,irow))
					rightblacks++;
				if(rightblacks>maxblackpixels)
					return -1;	//右上侧开放区域黑点过多超出误差噪点范围，不认为是'5'，可能是'9'或'3'
			}
		}
	}
	else if(wChar.wHz=='7')
	{
		if(pIsland!=NULL)
			return -1;	//'7'字符不可能有孤岛存在
		if(wTopBlackPixels<f2i(0.3*m_nWidth))
			return -1;	//'7'顶部黑点过少，可能是字符'1'
		if(wBtmBlackPixels>=f2i(0.3*m_nWidth))
			return -1;	//'7'字符底部行内黑点数过多就可能是'2'
	}
	//else if(wChar.wHz=='H')
	else if(wChar.wHz=='-')
	{
		if(pIsland!=NULL)
			return -1;	//'-'字符不可能有孤岛存在
		int yjStart30Percent=f2i(m_nHeight*0.3);
		int yjStart70Percent=f2i(m_nHeight*0.7);
		if(rcFeatureActual.top>=yjStart30Percent&&rcFeatureActual.bottom<=yjStart70Percent)
		{
			double w2hscale=(rcFeatureActual.right-rcFeatureActual.left+1)/(rcFeatureActual.bottom-rcFeatureActual.top+1.0);
			if(w2hscale>=2.0)
				return 1;	//确定为'-'字符
			else if(w2hscale<1.5)
				return 0;
		}
		else	//有时候上下侧也会存在噪点影响，故不能直接排除 wjh-2018.4.4
			return 0;
	}
	else if(wChar.wHz=='_')
	{
		if(pIsland!=NULL)
			return -1;	//'-'字符不可能有孤岛存在
		int yjStart60Percent=f2i(m_nHeight*0.6);
		if(rcFeatureActual.top>=yjStart60Percent&&rcFeatureActual.bottom>=rcFeatureActual.top)
		{
			double w2hscale=(rcFeatureActual.right-rcFeatureActual.left+1)/(rcFeatureActual.bottom-rcFeatureActual.top+1.0);
			if(w2hscale>=2.0)
				return 1;	//确定为'_'字符
			else if(w2hscale<1.5)
				return 0;
		}
		else	//有时候上侧也会存在噪点影响，故不能直接排除 wjh-2018.4.4
			return 0;
	}
	else if(wChar.wHz=='.')
	{
		if(pIsland!=NULL)
			return -1;	//'.'字符不可能有孤岛存在
		int yjStart60Percent=f2i(m_nHeight*0.6);
		if(rcFeatureActual.top>=yjStart60Percent&&rcFeatureActual.bottom>=rcFeatureActual.top)
		{
			double w2hscale=(rcFeatureActual.right-rcFeatureActual.left+1)/(rcFeatureActual.bottom-rcFeatureActual.top+1.0);
			if(w2hscale>=0.66&&w2hscale<=1.5)
				return 1;	//确定为'.'字符
			else
				return -1;
		}
	}
	else if(wChar.wHz=='6')	//下半层有一个白色孤岛
	{
		if(count!=1)
			return -1;
		if(pIsland->count>=2&&core_y>m_nHeight*0.5)
			return 0;//1;
		else
			return -1;
	}
	else if(wChar.wHz=='8')
	{
		if(count!=2)
			return -1;
		ISLAND* pIslandUp=pIsland;
		ISLAND* pIslandDown=pSecValidIsland;
		if(pIslandUp==NULL||pIslandDown==NULL)
			return -1;
		if(pIslandUp->y>pIslandDown->y)
		{
			pIslandUp=pIslandDown;
			pIslandDown=pIsland;
		}
		if((pIslandUp->y/pIslandUp->count<m_nHeight*0.5)&&(pIslandDown->y/pIslandDown->count>m_nHeight*0.5))
		{
			//暂时不开启'8'字符的确定特征识别, 待将来测试成熟再开放 wjh-2018.4.4
			//if(pIslandUp->count>=2&&pIslandDown->count>=3)
			//	return 1;
			//else
				return 0;
		}
		else
			return -1;
	}
	else if(wChar.wHz=='9')
	{
		if(count!=1)
			return -1;
		if(core_y>2&&core_y<m_nHeight*0.40)//TODO:0.45应该调小
			return 0;//1;暂时不开启'8'字符的确定特征识别, 待将来测试成熟再开放 wjh-2018.4.4
		else
			return -1;
	}
	/*else if(wChar.wHz=='X')
	else if(wChar.wHz=='A')
	else if(wChar.wHz=='B')
	else if(wChar.wHz=='C')*/
	/*else if(wChar.wHz=='Q')
	{
		if(count!=1)
			return -1;
		if(core_y<m_nMaxTextH*0.25||core_y>m_nMaxTextH*0.75)
			return -1;
		else
			return 1;
	}
	else if(wChar.wHz=='P')
	{
		if(count!=1)
			return -1;
		if(core_y>2&&core_y<m_nMaxTextH/2)
			return 1;
		else
			return -1;
	}
	else if(wChar.wHz!=4&&pIsland&&pIsland->count>4)
		return -1;*/
	return 0;
}
//比较两字符是否相同
MATCHCOEF CImageChar::CalMatchCoef(CImageChar* pText,char* pciOffsetX/*=NULL*/,char* pciOffsetY/*=NULL*/)
{
	MATCHCOEF matchcoef;
	int i,j,k,matchCountArr[7]={0};
	bool turbomode=m_nHeight>30;	//此时向左右各扩3个像素
	for(i=0;i<m_nMaxTextH;i++)
	{
		for(j=0;j<m_nMaxTextH;j++)
		{
			if(!pText->IsBlackPixel(i,j))
				continue;
			bool black_l3,black_l2,black_l1,black_0,black_r1,black_r2,black_r3;
			if(wChar.wHz=='-'||wChar.wHz=='_'||wChar.wHz=='x')
			{
				black_l2=j<2?false:IsBlackPixel(i,j-2);
				black_l1=j<1?false:IsBlackPixel(i,j-1);
				black_0=IsBlackPixel(i,j);
				black_r1=(j>=m_nMaxTextH-1)?false:IsBlackPixel(i,j+1);
				black_r2=(j>=m_nMaxTextH-2)?false:IsBlackPixel(i,j+2);
				if(turbomode)
				{
					black_l3=j<3?false:IsBlackPixel(i,j-3);
					black_r3=(j>=m_nMaxTextH-3)?false:IsBlackPixel(i,j+3);
				}
			}
			else
			{
				if(turbomode)
				{
					black_l3=i<3?false:IsBlackPixel(i-3,j);
					black_r3=(i>=m_nMaxTextH-3)?false:IsBlackPixel(i+3,j);
				}
				black_l2=i<2?false:IsBlackPixel(i-2,j);
				black_l1=i<1?false:IsBlackPixel(i-1,j);
				black_0=IsBlackPixel(i,j);
				black_r1=(i>=m_nMaxTextH-1)?false:IsBlackPixel(i+1,j);
				black_r2=(i>=m_nMaxTextH-2)?false:IsBlackPixel(i+2,j);
			}
			if(i>=2&&black_l2)
				matchCountArr[0]++;
			if(i>=1&&black_l1)
				matchCountArr[1]++;
			if(black_0)
				matchCountArr[2]++;
			if(i<m_nMaxTextH-1&&black_r1)
				matchCountArr[3]++;
			if(i<m_nMaxTextH-2&&black_r2)
				matchCountArr[4]++;
			if(turbomode)
			{
				if(i>=3&&black_l3)
					matchCountArr[5]++;
				if(i<m_nMaxTextH-3&&black_r3)
					matchCountArr[6]++;
			}
		}
	}
	double uMaxMatchNum=0;
	int offset_x=0,offset_y=0;
	for(k=0;k<7;k++)
	{
		if(matchCountArr[k]>uMaxMatchNum)
		{
			uMaxMatchNum=matchCountArr[k];
			int xy=0;
			if(k<5)
				xy=k-2;
			else if(k==5)
				xy=-3;
			else if(k==6)
				xy=3;
			if(wChar.wHz=='-'||wChar.wHz=='_'||wChar.wHz=='x')
				offset_y=xy;//k-2;
			else
				offset_x=xy;//k-2;
			if(pciOffsetX)
				*pciOffsetX=offset_x;
			if(pciOffsetY)
				*pciOffsetY=offset_y;
		}
	}
	double increment=0;
	double bigger=max(m_nBlckPixels,pText->m_nBlckPixels);
	double smaller=min(m_nBlckPixels,pText->m_nBlckPixels);
	if(smaller==0)
	{
		matchcoef.original=matchcoef.weighting=0;
		return matchcoef;
	}
	double diffcoef=bigger/smaller;//因后续已考虑了渐近描粗的问题，故此差异度由原来的1.1降为1.05 wjh-2018.3.28
	if(uMaxMatchNum/smaller>0.6&&diffcoef>1.05)
	{	//描粗笔画，加大重叠区域
		int nBufMapSize=m_nMaxTextH*m_nMaxTextH;
		BYTE_ARRAY bufmap(nBufMapSize);
		memset(bufmap,0,nBufMapSize);
		if(m_nBlckPixels<pText->m_nBlckPixels)
		{
			int nSize=pText->GetImageEffWidth()*pText->GetImageHeight();
			if(nSize==0)
				nSize=pText->GetImageWidth()*pText->GetImageHeight();
			memcpy(bufmap,pText->GetImageMap(),(nSize>0&&nSize<nBufMapSize)?nSize:nBufMapSize);
		}
		else
		{
			int nSize=GetImageEffWidth()*GetImageHeight();
			if(nSize==0)
				nSize=GetImageWidth()*GetImageHeight();
			memcpy(bufmap,GetImageMap(),(nSize>0&&nSize<nBufMapSize)?nSize:nBufMapSize);
		}
		bool bEnableVertTrace=true;	//是否允许Y向描粗笔划
		if(wChar.wHz=='5'||wChar.wHz=='3')	//这些字符上下描粗后容易
			bEnableVertTrace=false;
		for(i=0;i<m_nWidth;i++)
		{
			for(j=0;j<m_nHeight;j++)
			{
				if(!pText->IsBlackPixel(i,j)||!IsBlackPixel(i+offset_x,j+offset_y))
					continue;
				int deitax=m_nBlckPixels>pText->m_nBlckPixels?offset_x:0;
				int deitay=m_nBlckPixels>pText->m_nBlckPixels?offset_y:0;
				int i2=i+deitax,j2=j+deitay;
				if(i+deitax<0)
				{	//借位
					j2-=1;
					i2=m_nWidth+i+deitax;
				}
				else if(i+deitax>=m_nWidth)
				{
					j2+=1;
					i2=i+deitax-m_nWidth;
				}
				BYTE* pCursor=&bufmap[j2*m_nWidth+i2];
				*pCursor=0;	//将当前重叠为黑点像素设为白点
				//将当前重叠黑点的上下左右点描黑为重叠点，并设为白点 wjh-2018.3.22
				if(i2>0)
					*(pCursor-1)=0;
				if(i2<m_nWidth-1)
					*(pCursor+1)=0;
				if(bEnableVertTrace&&j2>0)
					*(pCursor-m_nWidth)=0;
				if(bEnableVertTrace&&j2<m_nHeight-1)
					*(pCursor+m_nWidth)=0;
			}
		}
		for(j=0;j<m_nHeight;j++)
		{
			BYTE* pCursor=&bufmap[j*m_nWidth];
			for(i=0;i<m_nWidth;i++,pCursor++)
			{
				if(((*pCursor)&0x0f)>0)
					increment+=1;
			}
		}
		//计算描粗后乘加权系数后的有效匹配像素增加数(此举的目的在于解决描粗与否间的匹配系数跳跃问题） wjh-2018.3.28
		double weight_incre_coef=min((diffcoef-1)*5,1.0);	//此处5为经验值，相当于big/small=1.2时，权重系数为1
		if(weight_incre_coef>0)
		{
			int weight_match_pixels_increment=f2i((bigger-increment-uMaxMatchNum)*weight_incre_coef);
			increment=bigger-weight_match_pixels_increment-uMaxMatchNum;
		}
	}
	else
		increment=bigger-uMaxMatchNum;
	//double FACTORtoSMALL=0.382,FACTORtoBIG=0.618;
	double FACTORtoSMALL=robot.MATCHFACTORtoSMALL;
	double FACTORtoBIG  =robot.MATCHFACTORtoBIG;
	if(wChar.wHz=='1'||wChar.wHz=='7')
	{
		FACTORtoSMALL=0.618;
		FACTORtoBIG=0.382;
	}
	else if(wChar.wHz=='3')
	{
		FACTORtoSMALL=0.5;
		FACTORtoBIG=0.5;
	}
	double bigcoef  =min(1.0,uMaxMatchNum/smaller);
	matchcoef.original = 0.382*bigcoef+0.618*uMaxMatchNum/bigger;
	matchcoef.weighting= 0.382*bigcoef+0.618*(1-increment/bigger);
	//if(wChar.wHz=='1'&&matchcoef.original<matchcoef.weighting)
	//	matchcoef.weighting=matchcoef.original;
	//else if(wChar.wHz=='1'&&matchcoef.weighting<matchcoef.original)
	//	matchcoef.original=matchcoef.weighting;
	return matchcoef;
}
void CImageChar::ToBuffer(CBuffer &buff)
{
	buff.WriteInteger(m_nWidth);//m_nEffWidth=m_nWidth=pImageChar->GetImageWidth();
	buff.WriteInteger(m_nHeight);//m_nHeight=pImageChar->GetImageHeight();
	UINT pixcels=m_nWidth*m_nHeight;
	buff.WriteInteger(pixcels);
	buff.Write(m_lpBitMap,pixcels);
	buff.WriteInteger(m_nBlckPixels);
	buff.WriteInteger(_nDestValidWidth);
	//buffer.WriteInteger(rect.top);
	//buffer.WriteInteger(rect.left);
	//buffer.WriteInteger(rect.bottom);
	//buffer.WriteInteger(rect.right);
	//buffer.WriteWord(wChar.wHz);
	//buffer.WriteInteger(m_nMaxTextH);
}
void CImageChar::FromBuffer(CBuffer &buff,long version)
{
	buff.ReadInteger(&m_nWidth);//m_nEffWidth=m_nWidth=pImageChar->GetImageWidth();
	buff.ReadInteger(&m_nHeight);//m_nHeight=pImageChar->GetImageHeight();
	UINT pixels;//,nBlckPixels;//=pImageChar->GetImageWidth()*pImageChar->GetImageHeight();
	buff.ReadInteger(&pixels);
	if(m_lpBitMap)
		delete[] m_lpBitMap;
	m_lpBitMap=new BYTE[pixels];
	buff.Read(m_lpBitMap,pixels);
	buff.ReadInteger(&m_nBlckPixels);
	buff.ReadInteger(&_nDestValidWidth);
	//buffer.ReadInteger(&rect.top);
	//buffer.ReadInteger(&rect.left);
	//buffer.ReadInteger(&rect.bottom);
	//buffer.ReadInteger(&rect.right);
	//buffer.ReadWord(&wChar.wHz);
	//buffer.ReadInteger(&m_nMaxTextH);
}
//////////////////////////////////////////////////////////////////////////
//CImageCellRegion
BYTE WARNING_LEVEL::MAX_WARNING_ORIGIN_MATCH_COEF=65;
BYTE WARNING_LEVEL::MAX_WARNING_WEIGHT_MATCH_COEF=75;
BYTE WARNING_LEVEL::WaringLevelNumber()//0~255,越高表示警示程度越高
{
	if(wWarnFlag==ABNORMAL_CHAR_EXIST)		//识别过程中的异常情况
		return 2;	//存在不可识别的异常字符
	else if(wWarnFlag==ABNORMAL_TXT_REGION)
		return 2;	//提取的单元格文本区宽高异常
	else if(wWarnFlag==LOWRECOG_CHAR_EXIST)
	{	//存在匹配度较低的字符
		if(matchLevel[1]<MAX_WARNING_WEIGHT_MATCH_COEF)//||matchLevel[0]<MAX_WARNING_ORIGIN_MATCH_COEF
			return 1;
		else
			return 0;
	}
	return 0;
}
void WARNING_LEVEL::VerifyMatchCoef(const MATCHCOEF& matchcoef)
{
	if(wWarnFlag==0)
	{
		wWarnFlag=WARNING_LEVEL::LOWRECOG_CHAR_EXIST;
		matchLevel[0]=f2i(matchcoef.original*100);
		matchLevel[1]=f2i(matchcoef.weighting*100);
	}
	else if(wWarnFlag==WARNING_LEVEL::LOWRECOG_CHAR_EXIST)
	{
		MATCHCOEF self(matchLevel[0]*0.01,matchLevel[1]*0.01);
		if(self.CompareWith(matchcoef)<=0)
			return;
		matchLevel[0]=f2i(matchcoef.original*100);
		matchLevel[1]=f2i(matchcoef.weighting*100);
	}
}
CImageCellRegion::CImageCellRegion()
{
	m_lpBitMap=NULL;
	m_nEffWidth=0;
	m_nWidth=0;
	m_nHeight=0;
	m_nDataType=1;
	m_nIterDepth=0;
	m_bStrokeVectorImage=false;
	m_bBlackPixelIsTrue=true;
	m_biStdPartType=0;
	m_nMaxCharCout=0;
	m_nLeftMargin=0;
	m_nRightMargin=0;
	ClearPreSplitChar();
}

void CImageCellRegion::ClearPreSplitChar()
{
	m_nPreSplitZeroCount=0;
	preSplitXChar.wChar=preSplitQChar.wChar=preSplitLChar.wChar='0';
	preSplitXChar.wiStartPos=preSplitXChar.wiEndPos=0;
	preSplitQChar.wiStartPos=preSplitQChar.wiEndPos=0;
	preSplitLChar.wiStartPos=preSplitLChar.wiEndPos=0;
	memset(preSplitZeroCharArr,0,sizeof(IMAGE_CHARSET_DATA::CHAR_IMAGE)*4);
}

CImageCellRegion::~CImageCellRegion()
{
	if(m_lpBitMap)
	{
		delete []m_lpBitMap;
		m_lpBitMap=NULL;
	}
}
CAlphabets* CImageCellRegion::get_Alphabets() const
{
	return (CAlphabets*)IMindRobot::GetAlphabetKnowledge();
}
CLIP_PIXELS::CLIP_PIXELS(RECT* prcClip/*=NULL*/,BYTE* exterdata/*=NULL*/,UINT uiExterDataBytes/*=0*/)
{
	m_nRows=0;
	m_nCols=0;
	data=NULL;
	rowheader=NULL;
	m_bExternalMemory=false;
	if(prcClip)
	{
		rcClip=*prcClip;
		Resize(rcClip,exterdata,uiExterDataBytes);
	}
}
CLIP_PIXELS::CLIP_PIXELS(int width,int height,BYTE* exterdata/*=NULL*/,UINT uiExterDataSize/*=0*/)
{
	m_nRows=0;
	m_nCols=0;
	data=NULL;
	rowheader=NULL;
	m_bExternalMemory=false;
	rcClip=RECT_C(0,0,width-1,height-1);
	Resize(rcClip,exterdata,uiExterDataSize);
}
CLIP_PIXELS::~CLIP_PIXELS()
{
	if(data&&!m_bExternalMemory)
		delete []data;
	data=NULL;
}
UINT CLIP_PIXELS::CalAllocSize(int width,int height)
{
	return width*height+sizeof(ROW_DATA)*height;
}
void CLIP_PIXELS::Resize(const RECT& rcClip,BYTE* exterdata/*=NULL*/,UINT uiExterDataSize/*=0*/)
{
	m_nRows=rcClip.bottom-rcClip.top+1;
	m_nCols=rcClip.right-rcClip.left+1;
	if(m_nRows<0)
		m_nRows=0;
	if(m_nCols<0)
		m_nCols=0;
	if((m_nActualElemSize=m_nRows*m_nCols)>0)
	{
		if(data!=NULL&&!m_bExternalMemory)
			delete []data;
		UINT uiAllocDataSize=m_nActualElemSize;//*sizeof(BYTE);
		UINT uiAllocSize=m_nActualElemSize/* *sizeof(BYTE) */+sizeof(ROW_DATA)*m_nRows;
		if(exterdata!=NULL&&uiExterDataSize>=uiAllocSize)
		{
			data=exterdata;
			this->m_bExternalMemory=true;
		}
		else
		{
			data = new BYTE[uiAllocSize];
			this->m_bExternalMemory=false;
		}
		rowheader=(ROW_DATA*)(data+uiAllocDataSize);
		rowheader[0]=ROW_DATA(data,rcClip.left,m_nCols);
		for(int j=1;j<m_nRows;j++)
		{
			rowheader[j]=rowheader[j-1];
			rowheader[j].rowdata+=m_nCols;
		}
	}
}
CLIP_PIXELS::ROW_DATA CLIP_PIXELS::GetRowData(long index)
{	//index为像素的行索引值，即(X,Y)坐标中的Y坐标，即（x,y)=[y][x]
	if(index<rcClip.top||index>rcClip.bottom)
		return ROW_DATA();
	return rowheader[index-rcClip.top];
}
int IImageNoiseDetect::StatNearBlackPixels(int i,int j,DETECT_MAP* pDetectMap/*=NULL*/,RECT* pValidRgn/*=NULL*/,
	PRESET_ARRAY16<PIXEL_XY>* listStatPixels/*=NULL*/,int maxBlackPixels/*=5*/,BYTE cDetectFlag/*=0x0f*/)
{
	if(pValidRgn&&(i<pValidRgn->left||i>pValidRgn->right||j<pValidRgn->top||j>pValidRgn->bottom))
		return 0;	//超出预设有效边界范围

	bool blackpixel=IsBlackPixel(i,j);
	//char ciPixel=0;
	//pDetectMap->IsDetect(i,j,&ciPixel);
	pDetectMap->SetDetect(i,j,blackpixel?1:'W');
	int blackPixels=blackpixel?1:0;
	if(blackPixels==0)
		return 0;
	if(listStatPixels!=NULL)
		listStatPixels->Append(PIXEL_XY(i,j));
	if( blackPixels<maxBlackPixels&&(cDetectFlag&DETECT_RIGHT)>0 &&
		!pDetectMap->IsDetect(i+1,j))
		blackPixels+=StatNearBlackPixels(i+1,j,pDetectMap,pValidRgn,listStatPixels,maxBlackPixels-blackPixels);
	//孤岛检测时添加对斜对角连通的情况，如'1'字符的顶部斜线有可能是仅斜连通 wjh-2019.11.28
	if( blackPixels<maxBlackPixels&&((cDetectFlag&DETECT_RIGHT)>0||(cDetectFlag&DETECT_UP)>0) && 
		!pDetectMap->IsDetect(i+1,j+1))
		blackPixels+=StatNearBlackPixels(i+1,j+1,pDetectMap,pValidRgn,listStatPixels,maxBlackPixels-blackPixels);
	if( blackPixels<maxBlackPixels&&((cDetectFlag&DETECT_RIGHT)>0||(cDetectFlag&DETECT_DOWN)>0)
		&& !pDetectMap->IsDetect(i+1,j-1))
		blackPixels+=StatNearBlackPixels(i+1,j-1,pDetectMap,pValidRgn,listStatPixels,maxBlackPixels-blackPixels);
	if( blackPixels<maxBlackPixels&&(cDetectFlag&DETECT_UP)>0 &&
		!pDetectMap->IsDetect(i  ,j+1))
		blackPixels+=StatNearBlackPixels(i  ,j+1,pDetectMap,pValidRgn,listStatPixels,maxBlackPixels-blackPixels);
	if( blackPixels<maxBlackPixels&&(cDetectFlag&DETECT_DOWN)>0 &&
		!pDetectMap->IsDetect(i  ,j-1))
		blackPixels+=StatNearBlackPixels(i  ,j-1,pDetectMap,pValidRgn,listStatPixels,maxBlackPixels-blackPixels);
	if( blackPixels<maxBlackPixels&&(cDetectFlag&DETECT_LEFT)>0 && 
		!pDetectMap->IsDetect(i-1,j))
		blackPixels+=StatNearBlackPixels(i-1,j,pDetectMap,pValidRgn,listStatPixels,maxBlackPixels-blackPixels);
	if( blackPixels<maxBlackPixels&&((cDetectFlag&DETECT_LEFT)>0||(cDetectFlag&DETECT_UP)>0) &&
		!pDetectMap->IsDetect(i-1,j+1))
		blackPixels+=StatNearBlackPixels(i-1,j+1,pDetectMap,pValidRgn,listStatPixels,maxBlackPixels-blackPixels);
	if( blackPixels<maxBlackPixels&&((cDetectFlag&DETECT_LEFT)>0||(cDetectFlag&DETECT_DOWN)>0) && 
		!pDetectMap->IsDetect(i-1,j-1))
		blackPixels+=StatNearBlackPixels(i-1,j-1,pDetectMap,pValidRgn,listStatPixels,maxBlackPixels-blackPixels);
	return blackPixels;
}
bool IImageNoiseDetect::IsNoisePixel(int i,int j,RECT* pValidRgn/*=NULL*/,int maxIslandBlackPixels/*=5*/,BYTE cDetectFlag/*=0x0f*/,DETECT_MAP* pDetectMap/*=NULL*/)
{
	const int POOLSIZE=4096;	//4096=64*64
	static char BYTES_POOL[POOLSIZE];	//避免重复调用new-delete分配与释放内存
	DETECT_MAP detect(maxIslandBlackPixels,i,j,BYTES_POOL,POOLSIZE);
	if(pDetectMap==NULL)
		pDetectMap=&detect;
	bool blackpixel=IsBlackPixel(i,j);
	pDetectMap->SetDetect(i,j,blackpixel?1:'W');
	if(!blackpixel)
		return false;	//本身即为白点，绝不是黑点孤岛
	PRESET_ARRAY16<PIXEL_XY> listStatPixels;
	int blackpixels=StatNearBlackPixels(i,j,pDetectMap,pValidRgn,&listStatPixels,maxIslandBlackPixels,cDetectFlag);
	if(blackpixels>=maxIslandBlackPixels)
		return false;
	//检测排除'-'字符
	int min_y,max_y,min_x,max_x;
	for(UINT ii=0;ii<listStatPixels.Count;ii++)
	{
		PIXEL_XY* pPixel=&listStatPixels.At(ii);
		if(ii==0)
		{
			min_x=max_x=pPixel->x;
			min_y=max_y=pPixel->y;
		}
		else
		{
			if(min_x>pPixel->x)
				min_x=pPixel->x;
			else if(max_x<pPixel->x)
				max_x=pPixel->x;
			if(min_y>pPixel->y)
				min_y=pPixel->y;
			else if(max_y<pPixel->y)
				max_y=pPixel->y;
		}
	}
	int width=max_x-min_x+1;
	int height=max_y-min_y+1;
	if(height<=2&&width>=3&&listStatPixels.Count>5)	//
		return false;
	return true;

}
long IImageNoiseDetect::RemoveNoisePixels(RECT* pValidRgn/*=NULL*/,int maxIslandBlackPixels/*=5*/)
{
	int i,j,hits=0;
	int left=0,top=0,right=m_nWidth-1,bottom=m_nHeight-1;
	RECT clipper;
	if(pValidRgn)
	{
		left=pValidRgn->left;
		right=pValidRgn->right;
		top=pValidRgn->top;
		bottom=pValidRgn->bottom;
		clipper=*pValidRgn;
	}
	else
	{
		clipper.left=clipper.top=0;
		clipper.right =m_nWidth-1;
		clipper.bottom=m_nHeight-1;
	}
	CLIP_PIXELS states(&clipper);
#ifdef _TIMER_COUNT
	timer.Relay();
#endif
	for(i=left;i<=right;i++)
	{
		for(j=top;j<=bottom;j++)
			states[j][i]=IsNoisePixel(i,j,pValidRgn,maxIslandBlackPixels);
	}
#ifdef _TIMER_COUNT
	timer.Relay(40022);
#endif
	for(i=left;i<=right;i++)
	{
		for(j=top;j<=bottom;j++)
		{
			if(!states[j][i])
				continue;
			SetPixelState(i,j,false);
			hits++;
		}
	}
#ifdef _TIMER_COUNT
	timer.Relay(40023);
#endif
	return hits;
}
/*bool CImageCellRegion::IsBlackPixel(int i,int j)
{
	WORD iBytePos=i/8;
	WORD iBitPosInByte=i%8;
	BYTE gray=m_lpBitMap[j*m_nEffWidth+iBytePos]&BIT2BYTE[7-iBitPosInByte];
	if(gray!=BIT2BYTE[7-iBitPosInByte])
		return TRUE;	//黑点
	else
		return FALSE;	//白点
}*/
BOOL CImageCellRegion::IsSplitter(WORD movewnd[5],double lessThanSplitPixels/*=2*/)
{
	int leftbigger =max(movewnd[0],movewnd[1]);
	int rightbigger=max(movewnd[3],movewnd[4]);
	int leftsmall =min(movewnd[0],movewnd[1]);
	int rightsmall=min(movewnd[3],movewnd[4]);
	if(movewnd[2]==0)
		return TRUE;
	else if(movewnd[2]<lessThanSplitPixels&&
		movewnd[1]>=movewnd[2]&&leftbigger >=lessThanSplitPixels&&leftsmall >=movewnd[2]&&
		movewnd[3]>=movewnd[2]&&rightbigger>=lessThanSplitPixels&&rightsmall>=movewnd[2])
		return TRUE;
	else
		return FALSE;
}
CXhChar50 CImageCellRegion::SpellCellStrValue()
{
	CXhChar50 sValue;
	for(CImageChar* pText=listChars.GetFirst();pText;pText=listChars.GetNext())
	{
		if(pText->wChar.wHz>0)
		{
			if(pText->wChar.wHz<127)
				sValue.Append(pText->wChar.chars[0]);
			else	//汉字
			{
				sValue.Append(pText->wChar.chars[0]);
				sValue.Append(pText->wChar.chars[1]);
			}
		}
		else
			sValue.Append('?');
	}
	return sValue;
}

int CImageCellRegion::RetrieveMatchChar(WORD iBitStart,WORD& wChar,MATCHCOEF* pMatchCoef/*=NULL*/,double min_recogmatch_coef/*=0.6*/)
{
	WORD wPerfectChar=0;
	MATCHCOEF maxMactchCoef;
	int splitter=0;
	CImageChar character,matchchar;
	int stdHeight=Alphabets->GetTemplateH();
	MATCHCOEF matchCoef=0;
	if(!Alphabets->EarlyRecognizeChar(&character,min_recogmatch_coef,&matchCoef,this,iBitStart))
		return 0;
	if(character.wChar.wHz>0&&matchCoef.CompareWith(maxMactchCoef)>0)
	{
		wPerfectChar=character.wChar.wHz;
		matchchar.CopyImageChar(&character);
		maxMactchCoef=matchCoef;
		splitter=iBitStart+(character.xiImgRight-character.xiImgLeft);
	}
	if(wPerfectChar>0)
	{
		wChar=wPerfectChar;
		CImageChar* pText=listChars.append();
		pText->CopyImageChar(&matchchar);
		if(pMatchCoef)
			*pMatchCoef=maxMactchCoef;
		return splitter;
	}
	else
		return 0;
}

int CImageCellRegion::RetrieveMatchChar(WORD iBitStart,WORD iSplitStart,WORD iSplitEnd,WORD& wChar,
									    MATCHCOEF* pMatchCoef/*=NULL*/,double min_recogmatch_coef/*=0.6*/)
{
	if(Alphabets->IsPreSplitChar())
		return RetrieveMatchChar(iBitStart,wChar,pMatchCoef,min_recogmatch_coef);
	WORD wPerfectChar=0;
	MATCHCOEF maxMactchCoef;
	int splitter=0;
	CImageChar character,matchchar;
	for(WORD iSplit=iSplitStart;iSplit<=iSplitEnd;iSplit++)
	{
		int stdHeight=Alphabets->GetTemplateH();
		if(m_nWidth>m_nHeight||m_nHeight<5)
			stdHeight=m_nHeight;	//高宽比大于1，不需要拉伸，如“-” wht 18-07-25
		character.StandardLocalize(m_lpBitMap,m_bBlackPixelIsTrue,m_nEffWidth,RECT_C(iBitStart,0,iSplit,m_nHeight),stdHeight);
		MATCHCOEF matchCoef=0;
		if(!Alphabets->RecognizeData(&character,min_recogmatch_coef,&matchCoef))
			continue;
		if(character.wChar.wHz>0&&matchCoef.CompareWith(maxMactchCoef)>0)
		{
			wPerfectChar=character.wChar.wHz;
			matchchar.CopyImageChar(&character);
			maxMactchCoef=matchCoef;
			splitter=iBitStart+(character.xiImgRight-character.xiImgLeft);
			splitter=iSplit;
			//if(maxMactchCoef.weighting>0.9&&maxMactchCoef.original>0.7)
			//	break;
		}
	}
	if(wPerfectChar>0)
	{
		wChar=wPerfectChar;
		CImageChar* pText=listChars.append();
		pText->CopyImageChar(&matchchar);
		if(pMatchCoef)
			*pMatchCoef=maxMactchCoef;
		return splitter;
	}
	else
		return 0;
}
#ifdef _DEBUG
void CImageCellRegion::PrintCharMark()
{
	int i,from=0,iPrevSplitter=-1;
	bool jumpCharStart=false;
	for(CImageChar* pText=listChars.GetFirst();pText;)
	{
		for(i=from;i<pText->xiImgLeft;i++)
			logerr.LogString(" ",false);
		if(!jumpCharStart)
			logerr.LogString("<",false);	//pText->rect.left
		for(i=pText->xiImgLeft+1;i<pText->xiImgRight;i++)
			logerr.LogString(".",false);
		CImageChar* pNextChar=listChars.GetNext();
		if(pNextChar&&pText->xiImgRight==pNextChar->xiImgLeft)
		{
			logerr.LogString("|",false);	//pText->rect.left
			from=pText->xiImgRight;
			jumpCharStart=true;
		}
		else
		{
			logerr.LogString(">",false);	//pText->rect.right
			from=pText->xiImgRight+1;
			jumpCharStart=false;
		}
		pText=pNextChar;
	}
}
#endif
#include "LifeObj.h"
//设置是否启用材质简化字符
bool CImageCellRegion::ProcessBriefMaTemplChar(CImageChar *pTemplChar,CImageChar *pPartTypeChar,
											   CImageChar *pFirstWidth,int nWidthCharCount,
											   CImageChar *pFirstPlateThickChar,int nThickCharCount)
{
	if(pTemplChar==NULL||!pTemplChar->IsMaterialChar())
		return false;
	if(pPartTypeChar&&pPartTypeChar->IsPlateTypeChar())
	{	//钢板
		if(pFirstWidth)
		{	//简化材质字符再钢板宽度之后（-10X184H）
			if(pFirstWidth->wChar.wHz<'4')	//宽度为三位数
				pTemplChar->m_bTemplEnabled=(nWidthCharCount>=3);
			else
				pTemplChar->m_bTemplEnabled=(nWidthCharCount>=2);
		}
		else if(pFirstPlateThickChar)
		{	//简化材质字符在钢板规格之后（-10HX184）wht 19-01-15
			if(pFirstPlateThickChar->wChar.wHz<'4')	//最小厚度为
				pTemplChar->m_bTemplEnabled=(nThickCharCount>=2);
			else
				pTemplChar->m_bTemplEnabled=(nThickCharCount>=1);
		}
		else
			pTemplChar->m_bTemplEnabled=false;
	}
	else if(pFirstWidth&&pPartTypeChar&&(pPartTypeChar->IsAngleTypeChar()||pPartTypeChar->IsCharFAI()))
	{	//角钢或钢管
		int nAngleThickCharCount=nWidthCharCount;
		int nAngleWidthCharCount=listChars.GetNodeNum()-nAngleThickCharCount-2;	//除去乘号和构件类型字符
		if( (nAngleWidthCharCount>2&&pFirstWidth->wChar.wHz<='3')||		//角钢肢宽>100，最小肢厚为4,小于3肢厚大于等于2位数
			(nAngleWidthCharCount<=2&&pFirstWidth->wChar.wHz<='2'))		//角钢肢宽<100,最大肢宽不超过20,小于2之后大于等于2位数
			pTemplChar->m_bTemplEnabled=(nAngleThickCharCount>=2);
		else 
			pTemplChar->m_bTemplEnabled=(nAngleThickCharCount>=1);
	}
	else if(nWidthCharCount>1)
		pTemplChar->m_bTemplEnabled=true;
	else
		pTemplChar->m_bTemplEnabled=false;
	return true;
};
void CImageCellRegion::UpdateFontTemplCharState()
{
	if(m_nDataType==CImageCellRegion::PARTNO)
	{
		int nRecogCharCount=listChars.GetNodeNum();
		CImageChar *pHeadChar=listChars.GetFirst();
		if(nRecogCharCount==0||(nRecogCharCount<=2&&pHeadChar!=NULL&&pHeadChar->wChar.wHz>='0'&&pHeadChar->wChar.wHz<='9'))
		{	//件号第一位为数字时，前三位只能为数字 wht 18-06-28
			for(CImageChar* pTemplChar=Alphabets->listChars.GetFirst();pTemplChar;pTemplChar=Alphabets->listChars.GetNext())
			{
				if((pTemplChar->wChar.wHz>='0'&&pTemplChar->wChar.wHz<='9')||(nRecogCharCount>0&&pTemplChar->wChar.wHz=='-'))
					pTemplChar->m_bTemplEnabled=true;
				else
					pTemplChar->m_bTemplEnabled=false;
			}
		}
		else
		{
			int i=0,iSeparate=-1;
			for(CImageChar* pIterChar=listChars.GetFirst();pIterChar;pIterChar=listChars.GetNext())
			{
				if(pIterChar->wChar.wHz=='-')
				{
					iSeparate=i;
					break;
				}
				i++;
			}
			for(CImageChar* pTemplChar=Alphabets->listChars.GetFirst();pTemplChar;pTemplChar=Alphabets->listChars.GetNext())
			{
				/*if( pTemplChar->wChar.wHz=='_'||pTemplChar->wChar.wHz>128||	//暂时停止下划线的识别
					pTemplChar->IsPartTypeChar()||
					pTemplChar->wChar.wHz=='Q'||pTemplChar->IsCharX()||pTemplChar->wChar.wHz=='D')
					pTemplChar->m_bTemplEnabled=false;
				else
					pTemplChar->m_bTemplEnabled=true;*/
				if((pTemplChar->wChar.wHz>='0'&&pTemplChar->wChar.wHz<='9')||(iSeparate&&pTemplChar->wChar.wHz=='-'))
					pTemplChar->m_bTemplEnabled=true;
				else if(nRecogCharCount>=(iSeparate+1)+3&&pTemplChar->IsPartLabelPostfix())
					pTemplChar->m_bTemplEnabled=true;	//无分隔符从第4个字母开始可能出现字母，有分隔符从分隔符后第4个字母开始可能出现字母 wht 18-07-25
				else
					pTemplChar->m_bTemplEnabled=false;
			}
		}
		//CImageChar* pTmplDeductor=Alphabets->listChars.GetValue('-');
		//if(pTmplDeductor)
		//	pTmplDeductor->m_bTemplEnabled=true;
	}
	else if(m_nDataType==CImageCellRegion::GUIGE)
	{
		int nRecogCharCount=listChars.GetNodeNum();
		for(CImageChar* pIterChar=Alphabets->listChars.GetFirst();pIterChar;pIterChar=Alphabets->listChars.GetNext())
			pIterChar->m_bTemplEnabled=pIterChar->wChar.wHz<128;	//规格列中不会存在汉字字符
		BOOL bEndStdPart=(m_biStdPartType==1);
		if(m_biStdPartType>0)
		{
			int nRecogCharCount=listChars.GetNodeNum();
			CImageChar *pFirstChar=listChars.GetFirst(),*pSecondChar=listChars.GetNext();
			if(nRecogCharCount==0)
			{
				if(m_biStdPartType==1)
				{
					if(m_nMaxCharCout>=6)
					{	//法兰插板
						for(CImageChar* pTemplChar=Alphabets->listChars.GetFirst();pTemplChar;pTemplChar=Alphabets->listChars.GetNext())
						{
							if(	pTemplChar->wChar.wHz=='C'||pTemplChar->IsCharX()||	//插板
								pTemplChar->wChar.wHz=='F')							//法兰
								pTemplChar->m_bTemplEnabled=true;
							else
								pTemplChar->m_bTemplEnabled=false;
						}
					}
					else
					{
						for(CImageChar* pTemplChar=Alphabets->listChars.GetFirst();pTemplChar;pTemplChar=Alphabets->listChars.GetNext())
						{
							if( pTemplChar->wChar.wHz=='U'||pTemplChar->wChar.wHz=='N'||	//NUT-2,U-10,NX-2.1
								pTemplChar->wChar.wHz=='J'||								//JK-2
								pTemplChar->wChar.wHz=='T'||pTemplChar->wChar.wHz=='P')		//爬梯
								pTemplChar->m_bTemplEnabled=true;
							else
								pTemplChar->m_bTemplEnabled=false;
						}
					}
				}
				else
				{
					for(CImageChar* pTemplChar=Alphabets->listChars.GetFirst();pTemplChar;pTemplChar=Alphabets->listChars.GetNext())
					{
						if(//(pTemplChar->wChar.wHz>='0'&&pTemplChar->wChar.wHz<='9')||
							pTemplChar->wChar.wHz=='G')	//GJ-50
							pTemplChar->m_bTemplEnabled=true;
						else
							pTemplChar->m_bTemplEnabled=false;
					}
				}
			}
			else if(nRecogCharCount==1&&pFirstChar->wChar.wHz=='F')
			{	//当前为法兰第二个字符
				for(CImageChar* pTemplChar=Alphabets->listChars.GetFirst();pTemplChar;pTemplChar=Alphabets->listChars.GetNext())
				{
					if( pTemplChar->wChar.wHz=='D'||pTemplChar->wChar.wHz=='R'||
						pTemplChar->wChar.wHz=='G'||pTemplChar->wChar.wHz=='P')
						pTemplChar->m_bTemplEnabled=true;
					else
						pTemplChar->m_bTemplEnabled=false;
				}
			}
			else
			{
				bool bFL=false,bInsertPlate=false,bHasMinus=false,bGangSuo=false;
				if( pFirstChar&&pSecondChar&&pFirstChar->wChar.wHz=='F'&&
					(pSecondChar->wChar.wHz=='D'||pSecondChar->wChar.wHz=='R'||
					 pSecondChar->wChar.wHz=='G'||pSecondChar->wChar.wHz=='P'))
					bFL=true;
				else if(pFirstChar->wChar.wHz=='C'||pFirstChar->IsCharX())//||pFirstChar->wChar.wHz=='U')
					bInsertPlate=true;
				else if(pFirstChar->wChar.wHz=='U'||pFirstChar->wChar.wHz=='N'||	//NUT-2,U-10,NX-2.1
						pFirstChar->wChar.wHz=='G'||pFirstChar->wChar.wHz=='J')		//GJ-50,JK-2
					bGangSuo=true;
				if(bGangSuo)
				{
					for(CImageChar* pIterChar=listChars.GetFirst();pIterChar;pIterChar=listChars.GetNext())
					{
						if(pIterChar->wChar.wHz=='_'||pIterChar->wChar.wHz=='-')
						{
							bHasMinus=true;
							break;
						}
					}
				}
				if(bFL||bInsertPlate)
				{	//法兰第3-6、8个字符是数字，插板第2-5、7个字符是数字
					if( (bFL&&(nRecogCharCount<6||nRecogCharCount==7))||
						(bInsertPlate&&(nRecogCharCount<5||nRecogCharCount==6)))
					{
						for(CImageChar* pTemplChar=Alphabets->listChars.GetFirst();pTemplChar;pTemplChar=Alphabets->listChars.GetNext())
						{
							if(pTemplChar->wChar.wHz>='0'&&pTemplChar->wChar.wHz<='9')
								pTemplChar->m_bTemplEnabled=true;
							else
								pTemplChar->m_bTemplEnabled=false;
						}
					}
					else if((bFL&&nRecogCharCount==6)||			//法兰第7个字符为A或B
							(bInsertPlate&&nRecogCharCount==7))	//插板第8个字符为A或B
					{	
						for(CImageChar* pTemplChar=Alphabets->listChars.GetFirst();pTemplChar;pTemplChar=Alphabets->listChars.GetNext())
						{
							if(pTemplChar->wChar.wHz=='A'||pTemplChar->wChar.wHz=='B')
								pTemplChar->m_bTemplEnabled=true;
							else
								pTemplChar->m_bTemplEnabled=false;
						}
					}
					else 
					{	//法兰第9个字符，插板第6个字符为材质字符
						for(CImageChar* pTemplChar=Alphabets->listChars.GetFirst();pTemplChar;pTemplChar=Alphabets->listChars.GetNext())
						{
							if( pTemplChar->wChar.wHz=='S'||pTemplChar->wChar.wHz=='H'||
								(bFL&&pTemplChar->wChar.wHz=='D')||(!bFL&&pTemplChar->wChar.wHz=='P'))
								//||pTemplChar->wChar.wHz=='G'||pTemplChar->wChar.wHz=='T') //标准库中暂时未发现390\460材质 wht 18-07-21
								pTemplChar->m_bTemplEnabled=true;
							else
								pTemplChar->m_bTemplEnabled=false;
						}
					}
				}
				else if(bGangSuo)
				{	//NUT-2,U-10,NX-2.1,GJ-50,JK-2
					if(!bHasMinus)
					{
						if(pFirstChar->wChar.wHz=='N'||pFirstChar->wChar.wHz=='U')
						{	//NUT-2,NX-2.1 U-10
							for(CImageChar* pTemplChar=Alphabets->listChars.GetFirst();pTemplChar;pTemplChar=Alphabets->listChars.GetNext())
							{
								if( pTemplChar->wChar.wHz=='X'||pTemplChar->wChar.wHz=='U'||
									pTemplChar->wChar.wHz=='T'||pTemplChar->wChar.wHz=='-'||
									pTemplChar->wChar.wHz=='_'||pTemplChar->wChar.wHz=='.')
									pTemplChar->m_bTemplEnabled=true;
								else
									pTemplChar->m_bTemplEnabled=false;
							}
						}
						else 
						{	//GJ-50,JK-2
							for(CImageChar* pTemplChar=Alphabets->listChars.GetFirst();pTemplChar;pTemplChar=Alphabets->listChars.GetNext())
							{
								if( pTemplChar->wChar.wHz=='J'||pTemplChar->wChar.wHz=='K'||
									pTemplChar->wChar.wHz=='-'||pTemplChar->wChar.wHz=='_')
									pTemplChar->m_bTemplEnabled=true;
								else
									pTemplChar->m_bTemplEnabled=false;
							}
						}
					}
					else 
					{
						for(CImageChar* pTemplChar=Alphabets->listChars.GetFirst();pTemplChar;pTemplChar=Alphabets->listChars.GetNext())
						{
							if(pTemplChar->wChar.wHz>='0'&&pTemplChar->wChar.wHz<='9')
								pTemplChar->m_bTemplEnabled=true;
							else
								pTemplChar->m_bTemplEnabled=false;
						}
					}
				}
				else
				{
					for(CImageChar* pTemplChar=Alphabets->listChars.GetFirst();pTemplChar;pTemplChar=Alphabets->listChars.GetNext())
					{
						if(pTemplChar->wChar.wHz>='0'&&pTemplChar->wChar.wHz<='9')
							pTemplChar->m_bTemplEnabled=true;
						else
							pTemplChar->m_bTemplEnabled=false;
					}
				}
				Alphabets->pCharPoint->m_bTemplEnabled=true;	//钢管塔规格列支持识别"." wht 19-06-09
			}
			if(nRecogCharCount>0&&this->m_bStrokeVectorImage&&Alphabets->pCharPoint)
				Alphabets->pCharPoint->m_bTemplEnabled=true;
		}
		else
		{
			if(nRecogCharCount==0)
			{
				if(m_nMaxCharCout<=3)
				{	//字符数小于等于3只能为槽钢或圆钢(C5、[5、φ20)
					for(CImageChar* pTemplChar=Alphabets->listChars.GetFirst();pTemplChar;pTemplChar=Alphabets->listChars.GetNext())
					{
						if( pTemplChar->wChar.wHz=='['||pTemplChar->wChar.wHz=='C'||//槽钢
							pTemplChar->IsCharFAI())	//圆钢
							pTemplChar->m_bTemplEnabled=true;
						else
							pTemplChar->m_bTemplEnabled=false;
					}
				}
				else
				{
					for(CImageChar* pTemplChar=Alphabets->listChars.GetFirst();pTemplChar;pTemplChar=Alphabets->listChars.GetNext())
					{
						if( (pTemplChar->IsPartTypeChar()&&!pTemplChar->IsSoltTypeChar())||pTemplChar->wChar.wHz=='Q')
							pTemplChar->m_bTemplEnabled=true;
						//else if( pTemplChar->wChar.wHz>128||(	//规格列中不会存在汉字字符
						//	pTemplChar->wChar.wHz>='0'&&pTemplChar->wChar.wHz<='9'))
						//	pTemplChar->m_bTemplEnabled=false;
						else
							pTemplChar->m_bTemplEnabled=false;
					}
				}
				if(Alphabets->pCharX)
					Alphabets->pCharX->m_bTemplEnabled=false;
				if(Alphabets->pCharX2)
					Alphabets->pCharX2->m_bTemplEnabled=false;
				if(Alphabets->pCharPoint)
					Alphabets->pCharPoint->m_bTemplEnabled=false;
			}
			else
			{
				bool bRecognizedCharX=false;
				bool bSplashEndable=false;
				bool bHasBerifMatChar=false;	//已经出现过简化材质字符 wht 19-01-15
				int nWidthCharCount=0,nPlateThickCharCount=0;
				CImageChar *pPartTypeChar=NULL,*pFirstWidth=NULL,*pFirstThickChar=NULL;
				for(CImageChar* pIterChar=listChars.GetFirst();pIterChar;pIterChar=listChars.GetNext())
				{
					if(pIterChar->wChar.wHz=='_')
						pIterChar->wChar.wHz='-';	//个别情况规格字符串中'-'容易识别为'_' wjh-2018.3.30
					else if(pIterChar->wChar.wHz=='x'||pIterChar->wChar.wHz=='X')
					{
						bRecognizedCharX=true;
						nWidthCharCount=listChars.GetNodeNum()-(listChars.GetCurrentIndex()+1);
						pFirstWidth=listChars.GetNext();	//乘号之后的第一个字符
						break;
					}
					if( pIterChar->IsPartTypeChar())
					{
						pPartTypeChar=pIterChar;
						bSplashEndable=!pIterChar->IsCharFAI();	//套管需要/字符
					}
					else if(pPartTypeChar&&pPartTypeChar->IsPlateTypeChar())
					{	//统计钢板厚度字符 wht 19-01-15
						if(nPlateThickCharCount==0)
							pFirstThickChar=pIterChar;
						nPlateThickCharCount++;
					}
					if(pIterChar->IsMaterialChar())
						bHasBerifMatChar=true;
				}
				bool bProcessedBriefMaTemplChar=false;
				CImageChar *pChar,*pTemplChar,*pHeadChar=listChars.GetFirst(),*pLastChar=listChars.GetTail();
				Alphabets->pCharX->m_bTemplEnabled=true;
				if(Alphabets->pCharX2)
					Alphabets->pCharX2->m_bTemplEnabled=true;
				bool bIsPlate=(pPartTypeChar&&pPartTypeChar->IsPartTypeChar());
				BOOL bBriefMatChar=(pHeadChar&&pHeadChar->wChar.wHz!='Q')&&!bHasBerifMatChar;	//可能存在简化材质字符
				if(pHeadChar&&pHeadChar->wChar.wHz=='Q'&&nRecogCharCount<=4)
				{
					for(pTemplChar=Alphabets->listChars.GetFirst();pTemplChar;pTemplChar=Alphabets->listChars.GetNext())
					{
						if(pTemplChar->wChar.wHz>='0'&&pTemplChar->wChar.wHz<='9')
							pTemplChar->m_bTemplEnabled=true;
						if( nRecogCharCount==1&&pTemplChar->wChar.wHz!='2'&&
							pTemplChar->wChar.wHz!='3'&&pTemplChar->wChar.wHz!='4')
							pTemplChar->m_bTemplEnabled=false;	//第一个字母为'Q'，后续字母只能为'2':Q235,'3':Q345,'4':Q420
						if( nRecogCharCount==2&&pTemplChar->wChar.wHz!='3'&&pTemplChar->wChar.wHz!='4'&&
							pTemplChar->wChar.wHz!='2'&&pTemplChar->wChar.wHz!='6')
							pTemplChar->m_bTemplEnabled=false;	//第一个字母为'Q'，后续字母只能为'3':Q345,'2':Q420,'6':Q460
						if( nRecogCharCount==3&&pTemplChar->wChar.wHz!='0'&&pTemplChar->wChar.wHz!='5')
							pTemplChar->m_bTemplEnabled=false;	//第一个字母为'Q'，后续字母只能为'5':Q345,'0':Q420,Q460
						if( nRecogCharCount==4)
						{
							if( pTemplChar->wChar.wHz!='L'&&pTemplChar->wChar.wHz!='-'&&
								!pTemplChar->IsCharFAI())
								pTemplChar->m_bTemplEnabled=false;	//第一个字母为'Q'，后续4th字母只能为'L','-','Φ'
							else
								pTemplChar->m_bTemplEnabled=true;
						}
					}
				}
				else if(pLastChar&&
						(pLastChar->IsPartTypeChar()||
						pLastChar->wChar.wHz=='X'||pLastChar->wChar.wHz=='x'))
				{
					for(pTemplChar=Alphabets->listChars.GetFirst();pTemplChar;pTemplChar=Alphabets->listChars.GetNext())
					{
						if(pTemplChar->wChar.wHz>='1'&&pTemplChar->wChar.wHz<='9')
							pTemplChar->m_bTemplEnabled=true;
						else
							pTemplChar->m_bTemplEnabled=false;
					}
				}
				else
				{
					for(pChar=listChars.GetFirst();pChar;pChar=listChars.GetNext())
					{
						if(pChar->wChar.wHz=='Q')
							Alphabets->pCharQ->m_bTemplEnabled=false;
						else if(pChar->wChar.wHz=='L')
							Alphabets->pCharQ->m_bTemplEnabled=Alphabets->pCharL->m_bTemplEnabled=false;
						else if(pChar->IsCharX())//wChar.wHz=='X')
						{
							Alphabets->pCharQ->m_bTemplEnabled=Alphabets->pCharL->m_bTemplEnabled=Alphabets->pCharX->m_bTemplEnabled=false;
							if(Alphabets->pCharX2)
								Alphabets->pCharX->m_bTemplEnabled=false;
						}
					}
					for(pTemplChar=Alphabets->listChars.GetFirst();pTemplChar;pTemplChar=Alphabets->listChars.GetNext())
					{	//钢板材质字符可能在X之前（如：-10HX256）wht 19-01-15
						if((bIsPlate||bRecognizedCharX)&&(pTemplChar->wChar.wHz<'0'||pTemplChar->wChar.wHz>'9'))
						{
							if(bBriefMatChar&&pTemplChar->IsMaterialChar())
								bProcessedBriefMaTemplChar=ProcessBriefMaTemplChar(pTemplChar,pPartTypeChar,pFirstWidth,nWidthCharCount,pFirstThickChar,nPlateThickCharCount);
							else
							{
								if(!bRecognizedCharX&&pTemplChar->IsCharX())
									pTemplChar->m_bTemplEnabled=true;	//未出现X字符，应启用X字符 wht 19-01-15
								else
									pTemplChar->m_bTemplEnabled=false;	//'x'之后规格字符一定是数字
							}
						}
						if(pTemplChar->wChar.wHz>='0'&&pTemplChar->wChar.wHz<='9')
							pTemplChar->m_bTemplEnabled=true;
						if(pPartTypeChar&&pTemplChar->IsPartTypeChar())
						{	//存在构件类型字符，除套管外不可能再出现类型字符 wht 18-07-22
							if(pPartTypeChar->IsCharFAI()&&pTemplChar->IsCharFAI())
								pTemplChar->m_bTemplEnabled=true;	//套管可能存在两个φ
							else
								pTemplChar->m_bTemplEnabled=false;
						}
					}
				}
				//规格列处Q、X之外其余字母都应屏蔽，槽钢规格中可能存在字母(在此处统一屏蔽否则字库变化可能影响提取效果) wht 18-06-07
				for(CImageChar* pTemplChar=Alphabets->listChars.GetFirst();pTemplChar;pTemplChar=Alphabets->listChars.GetNext())
				{	//
					if( pTemplChar->wChar.wHz>='A'&&pTemplChar->wChar.wHz<='Z'&&!pTemplChar->IsCharX()&&pTemplChar->wChar.wHz!='L')
					{
						if((bIsPlate||bRecognizedCharX)&&bBriefMatChar&&pTemplChar->IsMaterialChar())
						{
							if(!bProcessedBriefMaTemplChar)
								ProcessBriefMaTemplChar(pTemplChar,pPartTypeChar,pFirstWidth,nWidthCharCount,pFirstThickChar,nPlateThickCharCount);
						}
						else
						{
							if(!bRecognizedCharX&&pTemplChar->IsCharX())
								pTemplChar->m_bTemplEnabled=true;	//未出现X字符，应启用X字符 wht 19-01-15
							else
								pTemplChar->m_bTemplEnabled=false;	//'x'之后规格字符一定是数字
						}
					}
					if(pTemplChar->wChar.wHz=='/')
					{
						if(!bSplashEndable)
							pTemplChar->m_bTemplEnabled=false;
					}
				}
				if(this->m_bStrokeVectorImage&&Alphabets->pCharPoint)
					Alphabets->pCharPoint->m_bTemplEnabled=true;
			}
		}
	}
	else if(m_nDataType==CImageCellRegion::LENGTH||m_nDataType==CImageCellRegion::NUM)
	{
		for(CImageChar* pTemplChar=Alphabets->listChars.GetFirst();pTemplChar;pTemplChar=Alphabets->listChars.GetNext())
		{	//暂时停止下划线的识别
			if( pTemplChar->wChar.wHz<'0'||pTemplChar->wChar.wHz>'9')
				pTemplChar->m_bTemplEnabled=false;
			else
				pTemplChar->m_bTemplEnabled=true;
		}
	}
	if((m_nDataType==CImageCellRegion::PIECE_W||m_nDataType==CImageCellRegion::SUM_W)&&Alphabets->pCharPoint)
	{
		for(CImageChar* pIterChar=Alphabets->listChars.GetFirst();pIterChar;pIterChar=Alphabets->listChars.GetNext())
		{
			if((pIterChar->wChar.wHz>='0'&&pIterChar->wChar.wHz<='9')||pIterChar->wChar.wHz=='.')
				pIterChar->m_bTemplEnabled=true;
			else
				pIterChar->m_bTemplEnabled=false;
		}
		Alphabets->pCharPoint->m_bTemplEnabled=true;
	}
	else if(!m_bStrokeVectorImage&&Alphabets->pCharPoint)
		Alphabets->pCharPoint->m_bTemplEnabled=false;
}

void PrintImageChar(CImageChar *pChar)
{
	for(int j=0;j<pChar->m_nImageH;j++)
	{
		for(int i=0;i<pChar->m_nImageW;i++)
		{
			if(pChar->IsBlackPixel(i,j))
				logerr.LogString(".",false);
			else
				logerr.LogString("B",false);
		}
		logerr.Log("\n");
	}
}
static BYTE byteConstArr[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
CELLTYPE_FLAG::CELLTYPE_FLAG(BYTE textTypeIdOrFlag,bool byTextTypeId)
{
	if(!byTextTypeId)
		cbFlag=textTypeIdOrFlag;
	else
	{
		cbFlag=0;
		AddCellTextType(textTypeIdOrFlag);
	}
}
BYTE CELLTYPE_FLAG::AddCellTextType(int idCellTextType/*=0*/)
{
	if(idCellTextType>=1&&idCellTextType<=8)
		cbFlag|=byteConstArr[idCellTextType-1];
	return cbFlag;
}
bool CELLTYPE_FLAG::IsHasCellTextType(int idCellTextType)
{
	if(idCellTextType>=1&&idCellTextType<=8)
		return (cbFlag&byteConstArr[idCellTextType-1])>0;
	else
		return false;
}
struct PRESET_PIXELS160 {static const int COUNT=160;};
struct PRESET_PIXELS1024{static const int COUNT=1024;};
//函数作用：1.提前统计总字符数，便于定位倒数第2、3位小数点
//			2.提前将边界清晰的字符优先提取出来
//			3.预知整体字符串中是否有关键字符
int CImageCellRegion::ParseImgCharRects(BYTE cbTextTypeFlag/*=0*/,IXhList<CHAR_SECT>* pCharSectList/*=NULL*/,
		WORD iBitStart/*=0*/,WORD iBitEnd/*=0xffff*/)
{
	ClearPreSplitChar();
	CELLTYPE_FLAG flag(cbTextTypeFlag);
	if(cbTextTypeFlag==0)
		flag.AddCellTextType(CImageCellRegion::GUIGE);
	//if(m_nDataType!=CImageCellRegion::GUIGE||Alphabets->pCharL==NULL||Alphabets->pCharZero==NULL)
	if(flag.IsHasCellTextType(CImageCellRegion::GUIGE)&&(Alphabets->pCharL==NULL||Alphabets->pCharZero==NULL))
		return -1;
	if(flag.IsHasCellTextType(CImageCellRegion::SUM_W)&&Alphabets->pCharPoint==NULL)
		return -1;
	int STDFONT_HEIGHT=Alphabets->GetTemplateH();
	CHAR_SECT* pCharSect;
	CXhSimpleList<CHAR_SECT> listCharSects;
	if(pCharSectList==NULL)
		pCharSectList=&listCharSects;
	//1.检测单元格是否为空单元格
	double dfMinWtoHcoefOfChar1=Alphabets->m_fMinWtoHcoefOfChar1>0?Alphabets->m_fMinWtoHcoefOfChar1:1.0/7;
	WORD wMinCharWidth=f2i(STDFONT_HEIGHT*dfMinWtoHcoefOfChar1);//robot.MIN_CHAR_WIDTH_SCALE);//0.15),2/10;
	//4的左半部分宽约10像素（字高40），故设定10范围内要求全分断时才视为字符分断。 wjh-2019.11.29
	WORD wMinCharWidth2=f2i(STDFONT_HEIGHT*0.25);//robot.MIN_CHAR_WIDTH_SCALE);//0.15)
	WORD wMaxCharWidth=f2i(STDFONT_HEIGHT*robot.MAX_CHAR_WIDTH_SCALE);//8/10;
	WORD wMinCharAllWidth=f2i(STDFONT_HEIGHT*0.33);	//全宽字符的最小宽度，如'Q','X','0',2~9字符等
	WORD wPreferCharWidth=f2i(STDFONT_HEIGHT*robot.PREFER_CHAR_WIDTH_SCALE);//6/10;
	if(m_nHeight<10&&this->m_nWidth<4)//STDFONT_HEIGHT/2||this->m_nWidth<wMinCharWidth)
		return 0;	//空单元格
	//Alphabets->FilterRecogState(cbTextTypeFlag);
	//2.拆分单元格
	MATCHCOEF matchcoef;
	if(wMinCharWidth<m_nWidth&&m_nWidth<=wMinCharWidth*2)
	{	//剩余为一个字符
		CHAR_SECT* pCharSect=pCharSectList->Append(CHAR_SECT(0,m_nWidth));
		/*CImageChar* pText=listChars.append();
		pText->xiImgLeft=0;
		pText->xiImgRight=m_nWidth;
		pText->StandardLocalize(m_lpBitMap,m_bBlackPixelIsTrue,m_nEffWidth,RECT_C(0,0,m_nWidth,m_nHeight),STDFONT_HEIGHT);
		UINT count=listChars.GetNodeNum();
		if(Alphabets->RecognizeData(pText,0.6,&matchcoef))
		{
			pCharSect->wChar=pText->wChar;
			pCharSect->ciMatchPercentW=(BYTE)(matchcoef.weighting*100+0.5);
			pCharSect->ciMatchPercentO=(BYTE)(matchcoef.original *100+0.5);
		}*/
		return 1;
	}
	if(m_nHeight>=PRESET_PIXELS160::COUNT)
	{
		logerr.Log("单元格的竖向分辨率像素过高(=%d)",m_nHeight);
		return -1;
	}
	if(m_nWidth>=PRESET_PIXELS1024::COUNT)
	{
		logerr.Log("单元格横向分辨率像素过高(=%d)",m_nWidth);
		return -1;
	}
	WORD maxSearchStep=wMaxCharWidth+2; //2是因为移动窗口宽5，中点右侧还有2个像素
	WORD wSplitColConnBlackPixels=0;		//分隔列中与左侧文本黑点水平连通的点数
	WORD movewnd[5]={0},wiSplitPos=0,i=0,j=0;
	PRESET_ARRAY<bool,PRESET_PIXELS160> arrayRightColPixels;
	PRESET_ARRAY<WORD,PRESET_PIXELS1024> xarrColBlckPixels;
	memset((bool*)arrayRightColPixels.Data(),0,sizeof(bool)*PRESET_PIXELS160::COUNT);
	memset((WORD*)xarrColBlckPixels.Data(),0,sizeof(WORD)*PRESET_PIXELS1024::COUNT);
	arrayRightColPixels.Clear();
	WORD xiPixel,yjPixel;
	if(iBitEnd>m_nWidth)
		iBitEnd=m_nWidth-1;
	for(yjPixel=0;yjPixel<m_nHeight;yjPixel++)
	{
		bool blckpixel=IsBlackPixel(iBitStart+wMinCharWidth,yjPixel);
		arrayRightColPixels.Set(yjPixel,blckpixel);
	}
	for(i=iBitStart+wMinCharWidth;i<=iBitEnd;i++)
	{
		xiPixel=i;
		WORD wColMaxBlackHits=0;
		WORD wColVertConnBlackHits=0,wColRightConnBlackHits=0;	//竖向及横向连通黑点数
		bool prevblack=false,blackpixel=false,rightpixel=false;
		for(yjPixel=0;yjPixel<=m_nHeight;yjPixel++)
		{
			blackpixel=arrayRightColPixels.At(yjPixel);
			rightpixel=IsBlackPixel(xiPixel+1,yjPixel);
			arrayRightColPixels.Set(yjPixel,rightpixel);
			if(blackpixel)
			{
				if(rightpixel)
					wColRightConnBlackHits++;	//与左侧文本区黑点的连通数量
				if(prevblack)
					wColVertConnBlackHits++;
				else
				{
					prevblack=true;
					wColVertConnBlackHits=1;
				}
			}
			else
			{
				wColMaxBlackHits=max(wColMaxBlackHits,wColVertConnBlackHits);
				prevblack=false;
			}
		}
		wColMaxBlackHits=max(wColMaxBlackHits,wColVertConnBlackHits);
		if(i>iBitStart+wMinCharWidth2)//极限超窄范围内分割字符要求完全分割（斜角连通情况视为连通，如'1'左上撇) wjh-2019.11.28
			wColMaxBlackHits=min(wColMaxBlackHits,wColRightConnBlackHits);
		xarrColBlckPixels.Set(xiPixel,wColMaxBlackHits);
	}
	xarrColBlckPixels.Set(iBitStart+wMinCharWidth-1,100);	//预置一较大黑点数
	xarrColBlckPixels.Set(iBitStart+wMinCharWidth-2,100);	//预置一较大黑点数
	WORD wiPrevSplitPos=iBitStart,xiScanStart=iBitStart+max(2,wMinCharWidth),xiScanEnd=iBitStart+wMaxCharWidth;
	do{
		bool findSplitPos=false;
		WORD wnBlckPixels=0xffff;
		//在细字体时发现ciTolerancePixels=2容易把一个字符如'L'，给截断 wjh-2019.11.28
		//for(BYTE ciTolerancePixels=2;!findSplitPos&&ciTolerancePixels<=6;ciTolerancePixels++)
		for(BYTE ciTolerancePixels=1;!findSplitPos&&ciTolerancePixels<=6;ciTolerancePixels++)
		{
			findSplitPos=false;
			for(i=xiScanStart;i<xiScanEnd&&i<iBitEnd-1;i++)
			{
				if(IsSplitter(xarrColBlckPixels.GetAt(i-2),ciTolerancePixels))
				{
					wiSplitPos=i;
					findSplitPos=true;
					wnBlckPixels=xarrColBlckPixels[i];
					break;
				}
			}
			if(i>=iBitEnd-1)
			{	//末尾字符
				wiSplitPos=iBitEnd;
				findSplitPos=true;
				wnBlckPixels=0;
				break;
			}
		}
		if(findSplitPos)
		{
			pCharSect=listCharSects.Append(CHAR_SECT(wiPrevSplitPos,wiSplitPos));
			pCharSect->cnRightBlckPixels=(BYTE)wnBlckPixels;
			//跳过字符间空白间隔，查找下一字符起点
			for(i=wiSplitPos+1;xarrColBlckPixels[i]==0&&i<=iBitEnd;i++);
			wiPrevSplitPos=i;
			xiScanStart=wiPrevSplitPos+wMinCharWidth;
			xiScanEnd=wiPrevSplitPos+wMaxCharWidth;
		}
		else
		{	//未找到合理分割间隙时继续查找下一分割间隙（此时当前分割区间可能存在多个字符
			xiScanStart=i;
			xiScanEnd=xiScanStart+wMinCharWidth;
		}
	}while(wiPrevSplitPos<iBitEnd-1);
	//预先识别窄字符，重新计算最小宽字符宽度
	int sumWidth=0,sumCount=0;
	for(pCharSect=listCharSects.EnumObjectFirst();pCharSect;pCharSect=listCharSects.EnumObjectNext())
	{
		WORD wiCharWidth=pCharSect->xiEnd-pCharSect->xiStart+1;
		if(wiCharWidth>wMinCharAllWidth)
			continue;	//仅识别窄字符(如：1)
		CImageChar text;
		text.StandardLocalize(m_lpBitMap,m_bBlackPixelIsTrue,m_nEffWidth,RECT_C(pCharSect->xiStart,0,pCharSect->xiEnd,m_nHeight),STDFONT_HEIGHT);
		if(Alphabets->RecognizeData(&text,0.65,&matchcoef))
		{
			pCharSect->wChar=text.wChar;
			pCharSect->ciMatchPercentW=(BYTE)(matchcoef.weighting*100+0.5);
			pCharSect->ciMatchPercentO=(BYTE)(matchcoef.original *100+0.5);
			sumWidth+=wiCharWidth;
			sumCount++;
		}
	}
	if(sumCount>0)
	{
		WORD wAvgWidth=f2i(sumWidth/sumCount);
		if(wAvgWidth<wMinCharAllWidth)
			wMinCharAllWidth=wAvgWidth+1;	//更新全字符最小宽度
	}
	//统计初步分隔后的字符宽高信息
	int hitscount=0,stathits=0;
	PRESET_ARRAY<BYTE,PRESET_PIXELS160> xarrHitsStat;
	xarrHitsStat.ZeroPresetMemory();
	CHAR_SECT* xarrFinalCharSects[3]={NULL,NULL,NULL};
	int ACTUAL_MAX_WIDTH=0;
	for(pCharSect=listCharSects.EnumObjectFirst();pCharSect;pCharSect=listCharSects.EnumObjectNext())
	{
		if(xarrFinalCharSects[0]==NULL)
			xarrFinalCharSects[0]=pCharSect;
		else if(xarrFinalCharSects[1]==NULL)
			xarrFinalCharSects[1]=pCharSect;
		else if(xarrFinalCharSects[2]==NULL)
			xarrFinalCharSects[2]=pCharSect;
		else
		{
			memmove(xarrFinalCharSects,&xarrFinalCharSects[1],8);
			xarrFinalCharSects[2]=pCharSect;
		}
		if(pCharSect->wChar=='1'||pCharSect->wChar=='.')
			continue;	//排除已识别的1、.等窄字符
		WORD wiCharWidth=pCharSect->xiEnd-pCharSect->xiStart+1;
		if(wiCharWidth<wMinCharAllWidth)
			continue;
		if(wiCharWidth<wMaxCharWidth&&wiCharWidth>ACTUAL_MAX_WIDTH)
			ACTUAL_MAX_WIDTH=wiCharWidth;
		xarrHitsStat[wiCharWidth]++;
		hitscount++;
	}
	//根据众数原则来确定默认字符宽度
	//取宽度较大的众数，避免(1515)误取窄字符宽度 wht 18-11-22
	int MID_WIDTH=f2i(hitscount*0.5);
	for(i=wMaxCharWidth-1;i>=wMinCharAllWidth;i--)
	{
		stathits+=xarrHitsStat[i];
		if(stathits>=MID_WIDTH)
		{
			wPreferCharWidth=i;
			break;
		}
	}
	if(wMaxCharWidth>wPreferCharWidth+wMinCharWidth+2)	//个别字体中的'-'符可能较宽，如wPreferCharWidth=25,wMinCharWidth=6,而'-'宽度则达到31 wjh-2018.11.16
		wMaxCharWidth=wPreferCharWidth+wMinCharWidth+2;
	//查找之前分割失败的字符区间
	CXhSimpleList<CHAR_SECT> listFinalCharSects;
	for(pCharSect=listCharSects.EnumObjectFirst();pCharSect;pCharSect=listCharSects.EnumObjectNext())
	{
		double dfCharWidth=pCharSect->xiEnd-pCharSect->xiStart+1;
		if(dfCharWidth<=wMaxCharWidth)
		{
			listFinalCharSects.Append(*pCharSect);
			continue;
		}
		int count=f2i(dfCharWidth/wPreferCharWidth);
		if(count==1)
		{	//可能是一个全宽字符+半宽字符（如'1','.','-'等）组合
			if(this->m_nDataType==CImageCellRegion::SUM_W||m_nDataType==CImageCellRegion::PIECE_W)
			{	//一般倒数第2或3位带一个小数点字符'.'
				if(xarrFinalCharSects[2]==pCharSect&&(xarrFinalCharSects[2]->xiEnd-xarrFinalCharSects[2]->xiStart+1)>wMaxCharWidth)
				{
					WORD wiSplitPos=xarrFinalCharSects[2]->xiEnd-wPreferCharWidth;
					CHAR_SECT *pInsCharPoint=listFinalCharSects.Append(CHAR_SECT(xarrFinalCharSects[2]->xiStart,wiSplitPos));
					CHAR_SECT *pInsCharNumbr=listFinalCharSects.Append(CHAR_SECT(wiSplitPos+1,xarrFinalCharSects[2]->xiEnd));
				}
				else if(xarrFinalCharSects[1]==pCharSect&&(xarrFinalCharSects[1]->xiEnd-xarrFinalCharSects[1]->xiStart+1)>wMaxCharWidth)
				{
					WORD wiSplitPos=xarrFinalCharSects[1]->xiStart+wPreferCharWidth-1;
					CHAR_SECT *pInsCharNumbr=listFinalCharSects.Append(CHAR_SECT(xarrFinalCharSects[1]->xiStart,wiSplitPos));
					CHAR_SECT *pInsCharPoint=listFinalCharSects.Append(CHAR_SECT(wiSplitPos+1,xarrFinalCharSects[1]->xiEnd));
				}
				else if(xarrFinalCharSects[0]==pCharSect&&(xarrFinalCharSects[0]->xiEnd-xarrFinalCharSects[0]->xiStart+1)>wMaxCharWidth)
				{
					WORD wiSplitPos=xarrFinalCharSects[0]->xiStart+wPreferCharWidth-1;
					CHAR_SECT *pInsCharNumbr=listFinalCharSects.Append(CHAR_SECT(xarrFinalCharSects[0]->xiStart,wiSplitPos));
					CHAR_SECT *pInsCharPoint=listFinalCharSects.Append(CHAR_SECT(wiSplitPos+1,xarrFinalCharSects[0]->xiEnd));
				}
			}
			else
			{
				wiSplitPos=pCharSect->xiStart+wMinCharWidth;
				WORD wMinBlckPixels=xarrColBlckPixels[pCharSect->xiStart+wMinCharWidth];
				for(i=pCharSect->xiStart+wMinCharWidth+1;i<=pCharSect->xiEnd-wMinCharWidth;i++)
				{
					if(xarrColBlckPixels[i]<wMinBlckPixels)
					{
						wMinBlckPixels=xarrColBlckPixels[i];
						wiSplitPos=i;
					}
				}
				CHAR_SECT *pInsChar1=listFinalCharSects.Append(CHAR_SECT(pCharSect->xiStart,wiSplitPos));
				CHAR_SECT *pInsChar2=listFinalCharSects.Append(CHAR_SECT(wiSplitPos+1,pCharSect->xiEnd));
			}
		}
		else
		{	//根据均分法分割字符
			double dfAvgWidth=dfCharWidth/count;
			short hits=1,iStart=pCharSect->xiStart,iEnd=pCharSect->xiStart+f2i(dfAvgWidth);
			while(iEnd<pCharSect->xiEnd){
				//在均分法基础上进行相对精细的分割定位
				wiSplitPos=iEnd-2;
				WORD wMinBlckPixels=xarrColBlckPixels[iEnd-2];
				for(i=iEnd-1;i<=iEnd+2;i++)
				{
					if(xarrColBlckPixels[i]<wMinBlckPixels)
					{
						wMinBlckPixels=xarrColBlckPixels[i];
						wiSplitPos=i;
					}
					else if(xarrColBlckPixels[i]==wMinBlckPixels&&wiSplitPos<iEnd&&(i<iEnd||i-iEnd<iEnd-wiSplitPos))
						wiSplitPos=i;
				}
				iEnd=wiSplitPos;
				CHAR_SECT *pInsChar=listFinalCharSects.Append(CHAR_SECT(iStart,iEnd));
				hits++;
				iStart=iEnd+1;
				iEnd=pCharSect->xiStart+f2i(dfAvgWidth*hits);
				if(iEnd>pCharSect->xiEnd||pCharSect->xiEnd-iEnd<wMinCharWidth)
				{
					iEnd=pCharSect->xiEnd;
					pInsChar=listFinalCharSects.Append(CHAR_SECT(iStart,iEnd));
				}
			};
		}
	}
	listCharSects.Empty();
	for(pCharSect=listFinalCharSects.EnumObjectFirst();pCharSect;pCharSect=listFinalCharSects.EnumObjectNext()){
		pCharSectList->Append(*pCharSect);
	}
	return pCharSectList->Count;
}

void CImageCellRegion::SplitImageCharSet(WORD iBitStart/*=0*/,WORD wGuessCharWidth/*=0*/,
	bool bIgnoreRepeatFlag/*=false*/,IMAGE_CHARSET_DATA* imagedata/*=NULL*/,BYTE cbTextTypeFlag/*=0*/)
{
	WORD wMinCharWidth=f2i(m_nHeight*robot.MIN_CHAR_WIDTH_SCALE);//0.15),2/10;
	WORD wMaxCharWidth=f2i(m_nHeight*robot.MAX_CHAR_WIDTH_SCALE);//8/10;
	WORD movewnd[5]={0},wSplitPos=0;
	if(bIgnoreRepeatFlag)
		m_nIterDepth=0;
	CELLTYPE_FLAG flag=cbTextTypeFlag;
#ifdef _TIMER_COUNT
	timer.Relay();
#endif
	if(m_nHeight>=PRESET_PIXELS160::COUNT)
	{
		logerr.Log("表格单行的分辨率像素过高(=%d)",m_nHeight);
		return;
	}
	//笔画矢量图生成的图像，这种图像几乎不存在失真，故最大程度保留了各字符的特征信息，适用于特征识别优先 wjh-2018.8.31
	Alphabets->m_bPreferStrokeFeatureRecog=m_bStrokeVectorImage;
	//预分割字符
	CXhSimpleList<CHAR_SECT> listCharSects;
	ParseImgCharRects(0,&listCharSects);

	double MIN_RECOG_MATCHCOEF=0.6;
	if(flag.IsHasCellTextType(CImageCellRegion::GUIGE)&&listChars.GetNodeNum()==4)
	{
		CImageChar *pFirstChar=listChars.GetFirst();
		if(pFirstChar->wChar.wHz=='Q')	//由于部分情况'L'字符与'-'字符识别度较低，此时需要降低识别度
			MIN_RECOG_MATCHCOEF=0.3;
	}
	//if(flag.IsHasCellTextType(CImageCellRegion::PARTNO&&listCharSects.Count>=5
	CDepthCounter<int> visitor(&m_nIterDepth);
	if(m_nIterDepth>robot.MAX_SPLIT_IMG_ITER_DEPTH)//>15
	{		//可能会出现Q345-8X358之类的规格字符串，故防死循环检测深度设为15
		logerr.Log("SplitImageCharSet fall into death lock loop");
		return;
	}
	int stdHeight=Alphabets->GetTemplateH();
	for(CHAR_SECT* pCharSect=listCharSects.EnumObjectFirst();pCharSect;pCharSect=listCharSects.EnumObjectNext())
	{
		if(flag.IsHasCellTextType(CImageCellRegion::GUIGE)||flag.IsHasCellTextType(CImageCellRegion::PARTNO))
			UpdateFontTemplCharState();	//规格列中需要根据当前识别的字符数量实时设定模版字符的识别状态
		MATCHCOEF matchcoef;
		CImageChar* pText=listChars.append();
		pText->xiImgLeft=pCharSect->xiStart;
		pText->xiImgRight=pCharSect->xiEnd;
		pText->StandardLocalize(m_lpBitMap,m_bBlackPixelIsTrue,m_nEffWidth,RECT_C(pCharSect->xiStart,0,pCharSect->xiEnd,m_nHeight),stdHeight);
		//UINT count=listChars.GetNodeNum();
		if(Alphabets->RecognizeData(pText,0.6,&matchcoef))
		{
			/*if(imagedata&&imagedata->uiPreAllocSize>=count)
			{
				imagedata->pPreAllocCharSet[count-1].wChar=pText->wChar;
				imagedata->pPreAllocCharSet[count-1].wiStartPos=iBitStart;
				imagedata->pPreAllocCharSet[count-1].wiEndPos=m_nWidth;
			}*/
			xWarningLevel.VerifyMatchCoef(matchcoef);
		}
		else
			xWarningLevel.wWarnFlag=WARNING_LEVEL::ABNORMAL_CHAR_EXIST;
	}
	/*XHWCHAR wChar;
	WORD maxSearchStep=wMaxCharWidth+2; //2是因为移动窗口宽5，中点右侧还有2个像素
	WORD wSplitColConnBlackPixels=0;		//分隔列中与左侧文本黑点水平连通的点数
	PRESET_ARRAY<bool,PRESET_PIXELS160> arrayPrevColPixels;
	memset((bool*)arrayPrevColPixels,0,sizeof(bool)*PRESET_PIXELS160::COUNT);
	arrayPrevColPixels.Clear();
	BOOL bSplitBySpace=TRUE;
	BOOL bPreSplitL=FALSE,bPreSplitZero=FALSE,bPreSplitQ=FALSE,bPreSplitX=FALSE;
	if(Alphabets->IsPreSplitChar())
	{
		WORD iTestPos=iBitStart+wMinCharWidth+1;
		if( preSplitQChar.wiEndPos>preSplitQChar.wiStartPos&&
			iTestPos>preSplitQChar.wiStartPos&&iTestPos<preSplitQChar.wiEndPos)
		{
			iBitStart=preSplitQChar.wiStartPos;
			wSplitPos=preSplitQChar.wiEndPos;
			bPreSplitQ=TRUE;
		}
		else if(preSplitXChar.wiEndPos>preSplitXChar.wiStartPos&&
				iTestPos>preSplitXChar.wiStartPos&&iTestPos<preSplitXChar.wiEndPos)
		{
			iBitStart=preSplitXChar.wiStartPos;
			wSplitPos=preSplitXChar.wiEndPos;
			bPreSplitX=TRUE;
		}
		else if(preSplitLChar.wiEndPos>preSplitLChar.wiStartPos&&
				iTestPos>preSplitLChar.wiStartPos&&iTestPos<preSplitLChar.wiEndPos)
		{	//是否为预处理L字符
			iBitStart=preSplitLChar.wiStartPos;
			wSplitPos=preSplitLChar.wiEndPos;
			bPreSplitL=TRUE;
		}
		else if(m_nPreSplitZeroCount>0)
		{	//是否为预处理0字符
			for(int j=0;j<m_nPreSplitZeroCount;j++)
			{
				if( preSplitZeroCharArr[j].wiEndPos>preSplitZeroCharArr[j].wiStartPos&&
					iTestPos>preSplitZeroCharArr[j].wiStartPos&&iTestPos<preSplitZeroCharArr[j].wiEndPos)
				{
					iBitStart=preSplitZeroCharArr[j].wiStartPos;
					wSplitPos=preSplitZeroCharArr[j].wiEndPos;
					bPreSplitZero=TRUE;
					break;
				}
			}
		}
		bSplitBySpace=!(bPreSplitL||bPreSplitQ||bPreSplitX||bPreSplitZero);
		//TODO:这里是不是有问题？
		if(preSplitQChar.wiEndPos>preSplitQChar.wiStartPos&&preSplitLChar.wiEndPos>preSplitLChar.wiStartPos)
		{	//Q与L之前存在3个字符（Q345、Q420、Q460)
			int nSumLen=preSplitLChar.wiStartPos-preSplitQChar.wiEndPos;
			if(nSumLen<maxSearchStep*3)
				bSplitBySpace=false;
		}
		bSplitBySpace=FALSE;
	}
	if(bSplitBySpace)
	{
		WORD i,xiPixel,yjPixel;
		xiPixel=wMinCharWidth+iBitStart;
		for(yjPixel=0;yjPixel<m_nHeight;yjPixel++)
		{
			if(IsBlackPixel(xiPixel,yjPixel))
				arrayPrevColPixels.Set(yjPixel,true);
			else
				arrayPrevColPixels.Set(yjPixel,false);
		}
		for(i=wMinCharWidth;i<=maxSearchStep;i++)
		{	
			xiPixel=i+iBitStart;
			WORD wColContinueBlackHits=0,wColBlackHits=0,wColConnBlackHits=0;
			bool prevblack=false,blackpixel=false,rightpixel=false;
			for(yjPixel=0;yjPixel<m_nHeight;yjPixel++)
			{
				blackpixel=arrayPrevColPixels.At(yjPixel);
				rightpixel=IsBlackPixel(xiPixel+1,yjPixel);
				arrayPrevColPixels.Set(yjPixel,rightpixel);
				if(blackpixel)
				{
					if(rightpixel)
						wColConnBlackHits++;	//与右侧文本区黑点的连通数量
					if(prevblack)
						wColContinueBlackHits++;
					else
					{
						prevblack=true;
						wColContinueBlackHits=1;
					}
				}
				else
				{
					wColBlackHits=max(wColBlackHits,wColContinueBlackHits);
					prevblack=false;
				}
			}
			wColBlackHits=max(wColBlackHits,wColContinueBlackHits);
			wColBlackHits=min(wColBlackHits,wColConnBlackHits);
			if(i<wMinCharWidth+4)
			{	//i-wMinCharWidth=0,1,2,3
				movewnd[i-wMinCharWidth]=wColBlackHits;
				if(i-wMinCharWidth<2 && (wColBlackHits==0))//||wColConnBlackHits==0))
				{	//防止漏判i-wMinCharWidth=0, 1处的分隔列
					wSplitPos=xiPixel;
					break;
				}
			}
			else
			{	//移位一个像素点
				if(i>wMinCharWidth+4)
					memmove(movewnd,&movewnd[1],8);
				movewnd[4]=wColBlackHits;
				//if(wColConnBlackHits==0)
				//{
				//	wSplitPos=xiPixel-1;
				//	wSplitColConnBlackPixels=wColConnBlackHits;
				//	break;
				//}
				//else
				if(IsSplitter(movewnd))
				{
					if(wColBlackHits>0)
						wSplitPos=xiPixel-2;
					else
						wSplitPos=xiPixel-3;
					wSplitColConnBlackPixels=wColConnBlackHits;
					break;
				}
			}
		}
	}
#ifdef _TIMER_COUNT
	timer.Relay(4020);
#endif
	bool bInvalidChar=false;
	const int MIN_MARGIN=3;
	if(flag.IsHasCellTextType(CImageCellRegion::GUIGE)&&iBitStart==0&&m_nLeftMargin<=MIN_MARGIN)
		bInvalidChar=true;
	if(wSplitPos!=0&&!bInvalidChar)
	{	//按空白像素列分割法找到字符分隔列
		//BYTE biRecogMode=Alphabets->GetRecogMode();
		//int stdHeight=(biRecogMode==1)?m_nHeight:Alphabets->GetTemplateH();
		int stdHeight=Alphabets->GetTemplateH();
		CImageChar* pText=listChars.append();
		pText->StandardLocalize(m_lpBitMap,m_bBlackPixelIsTrue,m_nEffWidth,RECT_C(iBitStart,0,wSplitPos,m_nHeight),stdHeight);
#ifdef _TIMER_COUNT
		DWORD dwStart4022Tick=timer.Relay(4021);
#endif
		if( bPreSplitX||bPreSplitQ||bPreSplitL||bPreSplitZero||
			Alphabets->RecognizeData(pText,MIN_RECOG_MATCHCOEF,&matchcoef))
		{
			if(bPreSplitX||bPreSplitQ||bPreSplitL||bPreSplitZero)
			{
				matchcoef.original=matchcoef.weighting=1.0;
				if(bPreSplitQ)
					pText->wChar.wHz='Q';
				else if(bPreSplitX)
					pText->wChar.wHz='X';
				else 
					pText->wChar.wHz=bPreSplitL?'L':'0';
			}
			xWarningLevel.VerifyMatchCoef(matchcoef);
			UINT count=listChars.GetNodeNum();
			if(imagedata&&imagedata->uiPreAllocSize>=count)
			{
				imagedata->pPreAllocCharSet[count-1].wChar=pText->wChar;
				imagedata->pPreAllocCharSet[count-1].wiStartPos=iBitStart;
				imagedata->pPreAllocCharSet[count-1].wiEndPos=wSplitPos;
			}
#ifdef _DEBUG
			int b=3;//调试设断点用
#endif
		}
		else
		{
			xWarningLevel.wWarnFlag=WARNING_LEVEL::ABNORMAL_CHAR_EXIST;
			if(m_nIterDepth==1)
				listChars.DeleteCursor();
		}
#ifdef _TIMER_COUNT
		timer.Relay(4022,dwStart4022Tick);
#endif
	}
	else if(wSplitPos==0)
	{	//未能按空白像素列分割法找到字符分隔列,此时转为逐步识别最匹配字符方法
		WORD xiPixel=0;
		do{
			if((xiPixel=RetrieveMatchChar(iBitStart,iBitStart+wMinCharWidth,iBitStart+wMaxCharWidth,wChar.wHz,&matchcoef,MIN_RECOG_MATCHCOEF))>0)
				wSplitPos=xiPixel;
			else
				iBitStart+=1;
			if(iBitStart+wMinCharWidth>=m_nWidth)
			{
				xWarningLevel.SetAbnormalCharExist();	//存在不可识别的字符
				return;	//全部字符都不匹配，有可能剩余部分为噪音点组成的斑块
			}
		}while(xiPixel==0);
		if(bInvalidChar)
		{	//第一个字符为无效字符，清空字符链表 wht 18-07-30
			listChars.Empty();
		}
		else
		{
			xWarningLevel.VerifyMatchCoef(matchcoef);
			UINT count=listChars.GetNodeNum();
			if(imagedata&&imagedata->uiPreAllocSize>=count)
			{
				CImageChar* pText=listChars.GetTail();
				imagedata->pPreAllocCharSet[count-1].wChar=pText->wChar;
				imagedata->pPreAllocCharSet[count-1].wiStartPos=iBitStart;
				imagedata->pPreAllocCharSet[count-1].wiEndPos=wSplitPos;
			}
		}
#ifdef _TIMER_COUNT
		timer.Relay(4023);
#endif
	}
	//计算下一个字符开始像素
	iBitStart=wSplitPos+1;
	for(WORD i=wSplitPos+1;i<=m_nWidth;i++)
	{
		BOOL bBalck=FALSE;
		for(WORD j=0;j<m_nHeight;j++)
		{
			if(IsBlackPixel(i,j))
			{
				bBalck=TRUE;
				break;
			}
		}
		if(bBalck || i==m_nWidth-1)
		{
			iBitStart=i;
			break;
		}
	}
	SplitImageCharSet(iBitStart,wGuessCharWidth,false,imagedata,cbTextTypeFlag);
	if(m_nIterDepth==1)
		Alphabets->m_bPreferStrokeFeatureRecog=false;*/
}
//////////////////////////////////////////////////////////////////////////
//CImageDataRegion
#ifdef _DEBUG
bool CImageDataRegion::EXPORT_CHAR_RECOGNITION=false;
#endif
int CImageDataRegion::m_nInitMonoForwardPixels=20;	//记录前一次区域平衡值，新建区域时用来初始化新区域的平衡值 wht 18-07-22
CImageDataRegion::CImageDataRegion(DWORD key/*=0*/)
{
	m_dwKey=key;
	m_nEffWidth=0;
	m_nWidth=0;
	m_nHeight=0;
	m_dwKey=NULL;
	m_pImageData=NULL;
	m_lpBitMap=NULL;
	m_fDisplayAllRatio=0;
	m_fDisplayRatio=1;
	m_nOffsetX=0;
	m_nOffsetY=0;
	//同一批图纸明亮度一般一致或接近，使用静态变量初始化，避免对于一张图纸重复调整明暗度 wht 18-07-22
	m_nMonoForwardPixels=m_nInitMonoForwardPixels;
}
CImageDataRegion::~CImageDataRegion()
{
	if(m_lpBitMap)
		delete []m_lpBitMap;
}
static double CalFunc(double X)
{
	double M=14.66667,A=30,B=35.33333,C=20;
	double X2=X*X;
	return M*X2*X+A*X2+B*X+C;
}
static double CalTangentFunc(double X)
{
	double M=14.66667,A=30,B=35.33333,C=20;
	return 3*M*X*X+2*A*X+B;
}
static double CalXByAntiFunc(double Y)
{
	double X=0;
	double diff=Y-20;
	while(fabs(diff)>0.1)
	{
		double K=CalTangentFunc(X);
		double XC=diff/K;
		X+=XC;
		double y=CalFunc(X);
		diff=Y-y;
	}
	return X;
}
//balancecoef 划分黑白点间的界限调整平衡系数,取值-0.5~0.5;0时对应最高频率像素前移20位
double CImageDataRegion::SetMonoThresholdBalanceCoef(double mono_balance_coef/*=0*/)
{	//保证mono_balance_coef= 0  时mono_balance_coef= 20，
	//    mono_balance_coef=-1.0时mono_balance_coef=  0，
	//    mono_balance_coef= 1.0时mono_balance_coef=100，
	if( mono_balance_coef<-1.0)
		mono_balance_coef=-1.0;
	if( mono_balance_coef>1.0)
		mono_balance_coef=1.0;
	int nOldMonoForwardPixels=m_nMonoForwardPixels;
	m_nMonoForwardPixels=f2i(CalFunc(mono_balance_coef));
	m_nInitMonoForwardPixels=m_nMonoForwardPixels;
	if(nOldMonoForwardPixels!=m_nMonoForwardPixels)
		UpdateImageRegion();
	return mono_balance_coef;
}
double CImageDataRegion::GetMonoThresholdBalanceCoef()
{
	return CImageDataFile::CalMonoThresholdBalanceCoef(m_nMonoForwardPixels);
}
bool CImageDataRegion::IsBlackPixel(int i,int j)
{
	bool *pbState=NULL;
	IJ ij(i,j);
	DWORD key=ij;
	if(pbState=hashImgShieldLayer.GetValue(ij))
		return *pbState;
	return IsBlackPixel(i,j,NULL,NULL);
}
bool CImageDataRegion::IsBlackPixel(int i,int j,BYTE* lpBits/*=NULL*/,WORD wBitWidth/*=0*/)
{
	if(i<0||j<0||i>=m_nWidth||j>=m_nHeight)
		return false;
	WORD iBytePos=i/8;
	WORD iBitPosInByte=i%8;
	BYTE gray=0;
	if(lpBits)
		gray=lpBits[j*wBitWidth+iBytePos]&BIT2BYTE[7-iBitPosInByte];
	else
		gray=m_lpBitMap[j*m_nEffWidth+iBytePos]&BIT2BYTE[7-iBitPosInByte];
	if(gray!=BIT2BYTE[7-iBitPosInByte])
		return true;	//黑点
	else
		return false;	//白点
}
bool CImageDataRegion::SetPixelState(int i,int j,bool black/*=true*/)
{
	//hashImgShieldLayer.SetValue(IJ(i,j),black);
	IImage::SetPixelState(i,j,black);
	return black;
}
long CImageDataRegion::FillRgnPixel(int i,int j,RECT& rgn,DWORD crColorFrom,DWORD crColorTo)
{
	if(i<rgn.left||j<rgn.top||i>rgn.right||j>rgn.bottom)
		return 0;
	bool blackstate=crColorFrom==0;
	bool deststate=crColorTo==0;
	if(IsBlackPixel(i,j)!=blackstate)
		return 0;
	hashImgShieldLayer.SetValue(IJ(i,j),deststate);
	//SetPixelState(i,j,deststate);
	long setpixels=1;
	setpixels+=FillRgnPixel(i-1,j  ,rgn,crColorFrom,crColorTo);
	setpixels+=FillRgnPixel(i-1,j+1,rgn,crColorFrom,crColorTo);
	setpixels+=FillRgnPixel(i-1,j-1,rgn,crColorFrom,crColorTo);
	setpixels+=FillRgnPixel(i  ,j+1,rgn,crColorFrom,crColorTo);
	setpixels+=FillRgnPixel(i  ,j-1,rgn,crColorFrom,crColorTo);
	setpixels+=FillRgnPixel(i+1,j  ,rgn,crColorFrom,crColorTo);
	setpixels+=FillRgnPixel(i+1,j+1,rgn,crColorFrom,crColorTo);
	setpixels+=FillRgnPixel(i+1,j-1,rgn,crColorFrom,crColorTo);
	return setpixels;
}
int CImageDataRegion::CalMaxPixelsIndex(int start,int end,BOOL bLevel,IMAGE_DATA *pImageData/*=NULL*/)
{
	int nPixelsSum=0;
	if(pImageData)
	{
		if(bLevel)
			nPixelsSum=pImageData->nWidth;
		else
			nPixelsSum=pImageData->nHeight;
	}
	else
	{
		if(bLevel)
			nPixelsSum=m_nWidth;
		else
			nPixelsSum=m_nHeight;
	}
	ATOM_LIST<PIXELLINE> pixellList; 
	for(int i=start;i<=end;i++)
	{
		PIXELLINE* pPixelLine=pixellList.append();
		pPixelLine->iStart=i;
		for(int j=0;j<nPixelsSum;j++)
		{
			if(pImageData)
			{
				if( (!bLevel && IsBlackPixel(i,j,(BYTE*)pImageData->imagedata,pImageData->nWidth))||
					(bLevel && IsBlackPixel(j,i,(BYTE*)pImageData->imagedata,pImageData->nWidth)))
					pPixelLine->nPixell=robot.Shell_AddLong(pPixelLine->nPixell,1);//++;
			}
			else
			{
				if(!bLevel && IsBlackPixel(i,j))
					pPixelLine->nPixell=robot.Shell_AddLong(pPixelLine->nPixell,1);//++;
				else if(bLevel && IsBlackPixel(j,i))
					pPixelLine->nPixell=robot.Shell_AddLong(pPixelLine->nPixell,1);//++;	
			}
		}
	}
	int nMaxPixell=0;
	for(PIXELLINE* pPixelLine=pixellList.GetFirst();pPixelLine;pPixelLine=pixellList.GetNext())
	{
		if(pPixelLine->nPixell>nMaxPixell)
		{
			nMaxPixell=pPixelLine->nPixell;
			start=pPixelLine->iStart;
		}
	}
	return start;
}

bool CImageDataRegion::RecognizeSampleCell(IMAGE_DATA *pImageData,RECT* prcText)
{
	//1、提取数据区域的行数和列数
	//行
	ATOM_LIST<int>	levelStartList;
	RecognizeLines(levelStartList,TRUE,pImageData);
	//列
	ATOM_LIST<int> verticalStartList;
	RecognizeLines(verticalStartList,FALSE,pImageData);
	//2.从行列线中找出第一个单元格的四个顶点
	if(levelStartList.GetNodeNum()>=2&&verticalStartList.GetNodeNum()>=2)
	{
		int *pCellLeft=verticalStartList.GetFirst();
		int *pCellRight=verticalStartList.GetNext();
		int *pCellTopY=levelStartList.GetFirst();
		int *pCellBtmY=levelStartList.GetNext();
		if(prcText&&pCellLeft&&pCellRight&&pCellTopY&&pCellBtmY)
			*prcText=RECT_C(*pCellLeft,*pCellTopY,*pCellRight,*pCellBtmY);	//TRUE
		else if(prcText)
		{
			*prcText=RECT_C();
			return false;
		}
		for(;pCellRight;pCellRight=verticalStartList.GetNext())
		{
			for(;pCellBtmY;pCellBtmY=levelStartList.GetNext())
			{
				//计算文本的最小有效区域
				LONG xStart=0,xEnd=0,yStart=0,yEnd=0;
				RECT rcCell=RECT_C(*pCellLeft,*pCellTopY,*pCellRight,*pCellBtmY);
				CalCharValidRect(rcCell,CImageCellRegion::PARTNO,xStart,yStart,xEnd,yEnd);
				if(xEnd-xStart>0&&yEnd-yStart>4)	//找到单元格内的有效文本区域
				{
					if(prcText)
						*prcText=RECT_C(xStart,yStart,xEnd,yEnd);	//TRUE
					return true;
				}
				pCellTopY=pCellBtmY;
			}
			pCellLeft=pCellRight;
		}
	}
	return false;	//FALSE
}

//直接坐标系转换公式
// x'=x*cosa+y*sina
// y'=y*cosa-x*sina
//
// x=(x'-y*sina)/cosa
// y=(y'+x*sina)/cosa
POINT TransPoint(POINT pt,double sina,double cosa,bool bForward)
{
	POINT newPt;
	newPt.x=newPt.y=0;
	if(bForward)
	{
		//newPt.x=pt.x*cosa+pt.y*sina;
		//newPt.y=pt.y*cosa;
	}
	return newPt;
}
BOOL CImageDataRegion::AnalyzeSampleImage(int sample_width,int sample_height,int &rowHeight)
{
	//1、截取分析样本
	RECT rc;
	rc.top=0;
	rc.left=0;
	rc.right=sample_width;
	rc.bottom=sample_height;
	IMAGE_DATA data,prev_data;
	GetRectMonoImageData(&rc,&data);
	POINT ptArr[4];
	ptArr[0].x=0;ptArr[0].y=0;
	ptArr[1].x=sample_width;ptArr[1].y=0;
	ptArr[2].x=sample_width;ptArr[2].y=sample_height;
	ptArr[3].x=0;ptArr[3].y=sample_height;
	//2、计算旋转范围
	int ADJUST_RANG=(int)(0.2*sample_height);
	double c=sqrt((double)(1+sample_width*sample_width));
	double sina=1/c,cosa=sample_width/c;
	//1、根据四个顶点计算旋转后的矩形框尺寸
	for(int i=0;i<ADJUST_RANG;i++)
	{
		
	}
	for(int i=0;i<ADJUST_RANG;i++)
	{

	}
	
	//1、获取左上角第一个交叉点

	//2、根据第一个交叉点向下查找第二个交叉点
	//3、根据前两个交叉点计算行高，根据行高与列宽比例计算列宽像素宽度
	//4、查找第一列所有的交叉点，如遇到断开处，按行高计算
	//5、从第一个交叉点向右横向查找所有交叉点
	//6、相邻四个点为一个单元格
	//7、将单元格摆正

	return FALSE;
}

//
struct STATBLCKPIXELS{long num,num2;};	//num是当前列精确黑点数，num2是考虑到线宽后当前列前后一定范围内的黑点数
MOVEWND_POOL64<STATBLCKPIXELS> statpool;	//统计当前行或列的黑点数量
MOVEWND_POOL64<bool> bandpixels;
BOOL CImageDataRegion::AnalyzeLinePixels(int& start,int& end,BOOL bLevel/*=TRUE*/,IMAGE_DATA *pImageData/*=NULL*/)
{
	int nBlackNum=0,nBlackSum=0;
	if(pImageData)
	{
		if(bLevel)
			nBlackSum=pImageData->nWidth;
		else
			nBlackSum=pImageData->nHeight;
	}
	else
	{
		if(bLevel)
			nBlackSum=m_nWidth;
		else
			nBlackSum=m_nHeight;
	}

	statpool.Clear();
	statpool.wiStartIndex=bandpixels.wiStartIndex=start;
	statpool.wiEndIndex  =bandpixels.wiEndIndex  =end;
	//计算作为表格线最多缺失的空白区段长度（像素数）
	int i,j;
	int MAX_LOSS_BLCK_PIXELS=f2i(0.7*nBlackSum);//f2i((100-robot.FRAMELINE_INTEGRITY_PERCENT_LOW)*nBlackSum*0.01);
	for(i=0;i<nBlackSum;i++)
	{
		bandpixels.Clear();
		for(j=start;j<=end;j++)
		{
			if(pImageData)
				bandpixels[j]= bLevel?IsBlackPixel(i,j,(BYTE*)pImageData->imagedata,pImageData->nWidth):IsBlackPixel(j,i,(BYTE*)pImageData->imagedata,pImageData->nWidth);
			else
				bandpixels[j]= bLevel?IsBlackPixel(i,j):IsBlackPixel(j,i);
			bool blackpixel=bandpixels[j];
			if(i-statpool[j].num>=MAX_LOSS_BLCK_PIXELS)
				continue;	//该行（列）缺失的黑点数已超出表格线缺失上限，被判别为非表格线行（列）
			if(blackpixel)
				statpool[j].num=robot.Shell_AddLong(statpool[j].num,1);	//statpool[j]++;
		}
		for(j=start;j<=end;j++)
		{
			if(i-statpool[j].num>=MAX_LOSS_BLCK_PIXELS)
				continue;	//该行（列）缺失的黑点数已超出表格线缺失上限，被判别为非表格线行（列）
			if(bandpixels[j])
				statpool[j].num2=robot.Shell_AddLong(statpool[j].num2,1);	//statpool[j]++;
			else if(j-start-1>0&&bandpixels[j-1])
				statpool[j].num2=robot.Shell_AddLong(statpool[j].num2,1);	//statpool[j]++;
			else if(j-start-2>0&&bandpixels[j-2])
				statpool[j].num2=robot.Shell_AddLong(statpool[j].num2,1);	//statpool[j]++;
			else if(j-start+1<=end&&bandpixels[j+1])
				statpool[j].num2=robot.Shell_AddLong(statpool[j].num2,1);	//statpool[j]++;
			else if(j-start+2<=end&&bandpixels[j+2])
				statpool[j].num2=robot.Shell_AddLong(statpool[j].num2,1);	//statpool[j]++;
		}
	}
	int nMostBlackNum=0,iMostBlackIndex=start;
	for(j=statpool.wiStartIndex;j<=statpool.wiEndIndex;j++)
	{
		if(nMostBlackNum<statpool[j].num)
		{
			nMostBlackNum=statpool[j].num;
			iMostBlackIndex=j;
		}
	}
	int nMostBlackNum2=0,iMostBlackIndex2=start;
	for(j=statpool.wiStartIndex;j<=statpool.wiEndIndex;j++)
	{
		if(nMostBlackNum2<statpool[j].num2)
		{
			nMostBlackNum2=statpool[j].num2;
			iMostBlackIndex2=j;
		}
	}
	double fMatchCoef =robot.Shell_DivFloat(nMostBlackNum ,nBlackSum);//(double)nBlackNum/nBlackSum;
	double fMatchCoef2=robot.Shell_DivFloat(nMostBlackNum2,nBlackSum);//(double)nBlackNum/nBlackSum;
	if(fMatchCoef<robot.FRAMELINE_INTEGRITY_PERCENT_LOW*0.01)	//个别图纸由于折痕导致竖线可能局部扭曲0.95->0.8
		return FALSE;
	if(fMatchCoef2<robot.FRAMELINE_INTEGRITY_PERCENT_HIGH*0.01)	//个别图纸由于折痕导致竖线可能局部扭曲0.95->0.8
		return FALSE;
	if(iMostBlackIndex>end-5&&(nMostBlackNum<nBlackSum||nMostBlackNum2<nBlackSum))
	{	//可能存在end之后的近邻列为表格竖线情况，此时左路左侧单元格文本充满紧邻当前竖线就会识别错误 wjh-2018.11.20
		statpool.wiStartIndex=bandpixels.wiStartIndex=end+1;
		statpool.wiEndIndex  =bandpixels.wiEndIndex  =end+5;
		bandpixels.Clear();
		statpool.Clear();
		for(i=0;i<nBlackSum;i++)
		{
			bandpixels.Clear();
			for(j=end+1;j<=end+5;j++)
			{
				if(pImageData)
					bandpixels[j]= bLevel?IsBlackPixel(i,j,(BYTE*)pImageData->imagedata,pImageData->nWidth):IsBlackPixel(j,i,(BYTE*)pImageData->imagedata,pImageData->nWidth);
				else
					bandpixels[j]= bLevel?IsBlackPixel(i,j):IsBlackPixel(j,i);
				bool blackpixel=bandpixels[j];
				if(i-statpool[j].num>=MAX_LOSS_BLCK_PIXELS)
					continue;	//该行（列）缺失的黑点数已超出表格线缺失上限，被判别为非表格线行（列）
				if(blackpixel)
					statpool[j].num=robot.Shell_AddLong(statpool[j].num,1);	//statpool[j]++;
			}
		}
		for(j=statpool.wiStartIndex;j<statpool.wiEndIndex;j++)
		{
			if(nMostBlackNum<statpool[j].num)
			{
				nMostBlackNum=statpool[j].num;
				iMostBlackIndex=j;
				if(nMostBlackNum==nBlackSum)
					break;
			}
		}
		for(j=statpool.wiStartIndex;j<statpool.wiEndIndex;j++)
		{
			if(nMostBlackNum2<statpool[j].num2)
			{
				nMostBlackNum2=statpool[j].num2;
				iMostBlackIndex2=j;
				if(nMostBlackNum2==nBlackSum)
					break;
			}
		}
		fMatchCoef =robot.Shell_DivFloat(nMostBlackNum ,nBlackSum);//(double)nBlackNum/nBlackSum;
		fMatchCoef2=robot.Shell_DivFloat(nMostBlackNum2,nBlackSum);//(double)nBlackNum/nBlackSum;
	}
	if(iMostBlackIndex!=iMostBlackIndex2&&fMatchCoef2/fMatchCoef>1.2)
		start=iMostBlackIndex2;
	else
		start=iMostBlackIndex;	//计算最优起始点
	return TRUE;
}
bool CImageDataRegion::IntelliRecogCornerPoint(const POINT& xPickPoint,const POINT& xTopLeft2Pick,POINT* pCornerPoint,WORD wiDetectWidth/*=100*/,WORD wiDetectHeight/*=100*/,char ciDetectRule/*='Y'*/)
{
	RECT rcRgn;
	rcRgn.left=xPickPoint.x-xTopLeft2Pick.x;
	rcRgn.top=xPickPoint.y-xTopLeft2Pick.y;
	rcRgn.right=rcRgn.left+wiDetectWidth;
	rcRgn.bottom=rcRgn.top+wiDetectHeight;
	int MAJOR_LENGTH=wiDetectWidth,MAJOR_DEPTH=wiDetectHeight;
	CImageDataFile::PRJSTAT_INFO stat;
	stat.ciStatType=CImageDataFile::PRJSTAT_INFO::STAT_TL;
	if(ciDetectRule=='Y'||ciDetectRule=='V')
	{
		ciDetectRule='Y';
		stat.dfPriorityOfRow=0;
		stat.dfPriorityOfCol=1.0;
		MAJOR_LENGTH=wiDetectHeight;
		MAJOR_DEPTH =wiDetectWidth;
	}
	else //if(ciDetectRule=='X'||ciDetectRule=='H')
	{
		ciDetectRule='X';
		stat.dfPriorityOfRow=1.0;
		stat.dfPriorityOfCol=0;
		MAJOR_LENGTH=wiDetectWidth;
		MAJOR_DEPTH =wiDetectHeight;
	}
	FUNC_ProjectRegionImage_BUFF* pBufObj=&_local_func_ProjectRegionImage_buff_object;
	DYN_ARRAY<WORD> lineblcks(MAJOR_LENGTH,true,pBufObj->row_blck_pixels,800);
	int minBlckStartI=-1,maxBlckStartI=-1,VALID_DEPTH=0;
	for(int xI=0;xI<(int)wiDetectWidth;xI++)
	{
		for(int yJ=0;yJ<(int)wiDetectHeight;yJ++)
		{
			POINT ptR=XHPOINT(xI+rcRgn.left,yJ+rcRgn.top);
			bool black=this->IsBlackPixel(ptR.x,ptR.y);
			if(black)
			{
				int index=ciDetectRule=='X'?xI:yJ;
				lineblcks[index]++;
				//统计黑点有效区间宽度
				int currI=ciDetectRule=='Y'?xI:yJ;
				if(minBlckStartI<0)
					minBlckStartI=currI;
				else
					minBlckStartI=min(minBlckStartI,currI);
				if(maxBlckStartI<0)
					maxBlckStartI=currI;
				else
					maxBlckStartI=max(maxBlckStartI,currI);
			}
		}
	}
	VALID_DEPTH=maxBlckStartI-minBlckStartI+1;
	bool successed=stat.StatLine(lineblcks,MAJOR_LENGTH,VALID_DEPTH,ciDetectRule=='Y'?0:1);
	if(successed)
	{
		if(ciDetectRule=='Y')
			pCornerPoint->y=stat.yjHoriMaxBlckLine+rcRgn.top;
		else
			pCornerPoint->x=stat.xiVertMaxBlckLine+rcRgn.left;
	}
	return successed;
}
//识别图片的中的行列
int CImageDataRegion::RecognizeLines(ATOM_LIST<int> &lineStartList,BOOL bLevel,IMAGE_DATA *pImageData/*=NULL*/)
{
	int offset=15,start=0,end=0;
	int i,nLen=0;
	if(bLevel)	//识别行数
		nLen=m_nHeight;
	else		//识别列数
		nLen=m_nWidth;
	int rowHeight=0,colWidth=0;
	for(i=0;i<nLen-1;)
	{	
		start=i;
		end=i+offset;
		if(end>=nLen)
			end=nLen-1;
		if(AnalyzeLinePixels(start,end,bLevel,pImageData))
			lineStartList.append(start);
		i=start+offset;
	}
	if(lineStartList.GetNodeNum()<=0)
		return 0;
	int *piFirstLineXorY=lineStartList.GetFirst();
	if(bLevel)
	{	//水平行线识别时，应根据各行等高及众数原则判定行高并，反向修正行线，以免漏线、重线
		//计算行高平均值dfAvH
		int i,niRows=max(0,lineStartList.GetNodeNum()-1);	//行数最小值不能为负数 wht 19-06-26
		double dfSumHeight=m_nHeight;
		double dfAverageHeight = (niRows > 0) ? dfSumHeight / niRows : dfSumHeight;
		double ROWHEIGHT_TOLERANCE = (niRows > 0) ? dfAverageHeight / (2 * niRows) : dfAverageHeight / 2;
		PRESET_ARRAY128<WORD> xarrStatHeightHits;
		xarrStatHeightHits.ZeroPresetMemory();
		xarrStatHeightHits.ReSize(128);
		int *pPrevPosY=lineStartList.GetFirst(),*pPosY=NULL;
		for(pPosY=lineStartList.GetNext();pPosY;pPosY=lineStartList.GetNext())
		{
			int niRowHeight=*pPosY-*pPrevPosY;
			WORD* pwiRowHeightHits=NULL;
			if((pwiRowHeightHits=xarrStatHeightHits.GetAt(niRowHeight))!=NULL)
				*pwiRowHeightHits+=1;
			else
				xarrStatHeightHits.Set(niRowHeight,1);
			pPrevPosY=pPosY;
		}
		//计算行高众数及中位数高度值
		int niMidPosHeight=0,indexMidPosHeight=(niRows+1)/2;	//中位数索引位置
		int niMajorHeight=0,niMajorHeightHits=0;
		int indexCurrentAccumHits=0;
		for(i=0;i<(int)xarrStatHeightHits.Count;i++)
		{
			WORD* pwiHits=xarrStatHeightHits.GetAt(i);
			if(pwiHits==0||*pwiHits==0)
				continue;
			if( niMidPosHeight==0&&	//中位数行高值为0表示未初始化
				indexCurrentAccumHits<indexMidPosHeight&&indexCurrentAccumHits+*pwiHits>=indexMidPosHeight)
				niMidPosHeight=i;	//找到中位数索引位置
			indexCurrentAccumHits+=*pwiHits;
			if(*pwiHits>niMajorHeightHits)
			{	//查找找到行高众数值
				niMajorHeightHits=*pwiHits;
				niMajorHeight=i;
			}
			if(indexCurrentAccumHits>=niRows)
				break;
		}
		int niGuessRowHeight=niMidPosHeight;	//根据中位数原则预测实际正确行高值，也可以根据众数原则预测行高值
		if(fabs(dfAverageHeight-niGuessRowHeight)>ROWHEIGHT_TOLERANCE)
		{
			int niActualRows=f2i(dfSumHeight/niGuessRowHeight);
			if(niActualRows!=niRows)
			{
				niRows=niActualRows;
				dfAverageHeight=dfSumHeight/niActualRows;
			}

			PRESET_ARRAY128<int>xarrLevelLinePosY; 
			xarrLevelLinePosY.ZeroPresetMemory();
			pPrevPosY=lineStartList.GetFirst();
			POINT xPickPoint,xCornerPoint,xTopLeft2Pick;
			double HALF_ROWHEIGHT=dfAverageHeight/2;
			//确保行高最小值，避免因行高计算错误进入while死循环 wht 19-06-26
			HALF_ROWHEIGHT = max(1, HALF_ROWHEIGHT);
			if(*pPrevPosY>=HALF_ROWHEIGHT)
			{	//首行位置识别错误
				xPickPoint.x=xPickPoint.y=xCornerPoint.x=xCornerPoint.y=0;
				xTopLeft2Pick.x=xTopLeft2Pick.y=20;
				if(IntelliRecogCornerPoint(xPickPoint,xTopLeft2Pick,&xCornerPoint,100,100,'Y'))
				{
					if(abs(xCornerPoint.y-dfAverageHeight)<4)	//此处4是经验容差数，3根据统计实践偏小了，改为4 wjh-2019.1.18
						xarrLevelLinePosY.Append(0);//第一行有可能因为裁剪原因丢失表格线
					if(*pPrevPosY-xCornerPoint.y>HALF_ROWHEIGHT)
						xarrLevelLinePosY.Append(xCornerPoint.y);
					double tolerance=*pPrevPosY-xCornerPoint.y-dfAverageHeight;
					int iPrevPosY=xCornerPoint.y;
					while(tolerance>HALF_ROWHEIGHT)
					{	//须加行
						int iNewLineY=f2i(iPrevPosY+dfAverageHeight);
						xarrLevelLinePosY.Append(iNewLineY);
						tolerance-=dfAverageHeight;
						iPrevPosY=iNewLineY;
					}
				}
				else
				{	//倒推行线
					PRESET_ARRAY128<int> xarrTempPosY;
					int iPrevPosY=*pPrevPosY;
					double tolerance=*pPrevPosY;
					while(tolerance>HALF_ROWHEIGHT)
					{
						int iNewLineY=f2i(iPrevPosY-dfAverageHeight);
						xarrTempPosY.Append(iNewLineY);
						tolerance-=dfAverageHeight;
						iPrevPosY=iNewLineY;
					}
					for(i=xarrTempPosY.Count-1;i<(int)xarrTempPosY.Count;i--)
						xarrLevelLinePosY.Append(xarrTempPosY.At(i));
				}
			}
			xarrLevelLinePosY.Append(*pPrevPosY);
			int* pTailPosY=lineStartList.GetTail();
			if(m_nHeight-*pTailPosY>=HALF_ROWHEIGHT)
			{	//末尾行位置识别错误
				xPickPoint.x=xCornerPoint.x=0;
				xTopLeft2Pick.x=xTopLeft2Pick.y=20;
				int iPrevPosY=*pTailPosY;
				double tolerance=m_nHeight-*pTailPosY;
				while(tolerance>HALF_ROWHEIGHT){
					xPickPoint.y=f2i(iPrevPosY+dfAverageHeight);
					if(IntelliRecogCornerPoint(xPickPoint,xTopLeft2Pick,&xCornerPoint,100,100,'Y')&&abs(xPickPoint.y-xCornerPoint.y)<5)
						xPickPoint.y=xCornerPoint.y;
					if(xPickPoint.y>=m_nHeight)
						xPickPoint.y=m_nHeight-1;
					lineStartList.append(xPickPoint.y);
					tolerance-=(xPickPoint.y-iPrevPosY);
					iPrevPosY=xPickPoint.y;
				}
			}
			pPrevPosY=lineStartList.GetFirst();
			for(pPosY=lineStartList.GetNext();pPosY;pPosY=lineStartList.GetNext())
			{
				double tolerance=*pPosY-*pPrevPosY-dfAverageHeight;
				if(tolerance>HALF_ROWHEIGHT)
				{	//须加行
					int iPrevPosY=*pPrevPosY;
					do{
						int iNewLineY=f2i(iPrevPosY+dfAverageHeight);
						xarrLevelLinePosY.Append(iNewLineY);
						tolerance-=dfAverageHeight;
						iPrevPosY=iNewLineY;
					}while(tolerance>HALF_ROWHEIGHT);
					xarrLevelLinePosY.Append(*pPosY);
				}
				else if(tolerance<-HALF_ROWHEIGHT)
				{
					int yiPerfectLine=CalMaxPixelsIndex(*pPrevPosY,*pPosY,true);
					*pPrevPosY=yiPerfectLine;
					lineStartList.DeleteCursor();
					continue;
				}
				else
					xarrLevelLinePosY.Append(*pPosY);
				pPrevPosY=pPosY;
			}
			lineStartList.Empty();
			for(i=0;i<(int)xarrLevelLinePosY.Count;i++)
				lineStartList.append(xarrLevelLinePosY.At(i));
		}
		lineStartList.Clean();
	}
	else
	{
		if(*piFirstLineXorY>10)				//首行/列检查			
			lineStartList.insert(0,0);
		bool bStrokeVectorImage=BelongImageData()!=NULL?(BelongImageData()->GetRawFileType()==CImageDataFile::RAW_IMAGE_PDF):FALSE;
		if(!bStrokeVectorImage)
		{	//读取矢量pdf时一般不需要修正 wjh-2018.11.15
			POINT xPickPoint,xCornerPoint,xTopLeft2Pick;
			//初步粗略分析列线,xCornerPoint,xTopLeft2Pick
			xPickPoint=XHPOINT(0,0);
			xTopLeft2Pick.x=20;
			xTopLeft2Pick.y=0;
			bool successed=false;
			PRESET_ARRAY32<int>xarrVertLinePosX;
			xarrVertLinePosX.ZeroPresetMemory();
			do{
				xCornerPoint=xPickPoint;
				if( bStrokeVectorImage||	//未处理横向单元格较长时中间不存在竖线情况 wjh-2018.11.15
					(successed=IntelliRecogCornerPoint(xPickPoint,xTopLeft2Pick,&xCornerPoint,500,100,'X')))
				{	//找到列线
					if(xCornerPoint.x>30&&xarrVertLinePosX.Count==0)
						xarrVertLinePosX.Append(0);
					xarrVertLinePosX.Append(xCornerPoint.x);
					xPickPoint.x=xCornerPoint.x+20;
				}
				else if(xarrVertLinePosX.Count==0)
					xarrVertLinePosX.Append(*piFirstLineXorY>10?0:*piFirstLineXorY);
				else
					xPickPoint.x+=100;
				xTopLeft2Pick.x=0;
			}while(xPickPoint.x<=this->m_nWidth);
			if(lineStartList.GetNodeNum()!=xarrVertLinePosX.Count)
			{
				lineStartList.Empty();
				for(i=0;i<(int)xarrVertLinePosX.Count;i++)
					lineStartList.append(xarrVertLinePosX.At(i));
			}
		}
	}
	return lineStartList.GetNodeNum();
}

//计算单元格内剔除边框线后的内部区域
void CImageDataRegion::CalCellInternalRect(RECT& data_rect,double vfScaleCoef,LONG& xStart,LONG& yStart,LONG& xEnd,LONG& yEnd)
{
	int nBlack=0;
	int FRAMELINE_TOLERANCE=max(3,f2i(3*vfScaleCoef));	//识别时锚定角点距离表格线的搜索容差范围
	int MINPIXELS_OF_FRAMELINE=f2i(15*vfScaleCoef);		//表格线（横线或竖线）最少像素数(因后续有检测空白间隙代码故放在此值7->10以区分文本压框情部况)
	int MINPIXELS_OF_LEFT_CONNECT=MINPIXELS_OF_FRAMELINE/2;
	//int MAXPIXELS_OF_BLANKLINE=f2i(1*vfScaleCoef);		//单元格内横线或竖线识别为空白的最多像素数
	//提取失败时应防止无初始值
	xStart=data_rect.left;
	xEnd  =data_rect.right;
	yStart=data_rect.top;
	yEnd  =data_rect.bottom;
	//水平左侧起始位置
	bool bFindFrameline=false;
	for(int i=data_rect.left;i<data_rect.right;i++)
	{
		nBlack=0;
		for(int j=data_rect.top;j<=data_rect.bottom;j++)
		{
			if(IsBlackPixel(i,j))
				nBlack++;
			if(nBlack>=MINPIXELS_OF_FRAMELINE)
			{
				bFindFrameline=true;
				break;
			}
		}
		//此处必须为<MINPIXELS_OF_FRAMELINE，而不能为<=MAXPIXELS_OF_BLANKLINE,因为要考虑边框线黑点因素影响
		if(nBlack<MINPIXELS_OF_FRAMELINE)
		{
			if(bFindFrameline)
				xStart=i;
			else if((i-data_rect.left+1)>=FRAMELINE_TOLERANCE)
				xStart=data_rect.left;//超出搜索容差范围，则视为已跳过单元格框线
			else
				continue;
			break;
		}
	}
	//水平右侧终止位置
	bFindFrameline=false;
	int nLeftConnBlackPixels=0;
	for(int i=data_rect.right;i>data_rect.left;i--)
	{
		nBlack=nLeftConnBlackPixels=0;
		for(int j=data_rect.top;j<=data_rect.bottom;j++)
		{
			bool blackpixel=IsBlackPixel(i,j);
			bool nearleft=IsBlackPixel(i-1,j);
			if(blackpixel)
				nBlack++;
			if(blackpixel&&nearleft)
				nLeftConnBlackPixels++;
			if(nBlack>=MINPIXELS_OF_FRAMELINE)
			{
				bFindFrameline=true;
				break;
			}
		}
		bFindFrameline|=(data_rect.right-i+1)>=FRAMELINE_TOLERANCE;	//超出搜索容差范围，则视为已跳过单元格框线
		if(bFindFrameline&&(nBlack<MINPIXELS_OF_FRAMELINE||nLeftConnBlackPixels<MINPIXELS_OF_LEFT_CONNECT))
		{
			if(nBlack<MINPIXELS_OF_FRAMELINE)
				xEnd=i;
			else
				xEnd=i-1;
			break;
		}
	}
	//计算横向表格线最少像素数
	double vfVertCoef=(xEnd-xStart)/(double)(data_rect.bottom-data_rect.top);
	int MINPIXELS_OF_VERTFRAMELINE=max(MINPIXELS_OF_FRAMELINE,f2i(MINPIXELS_OF_FRAMELINE*vfVertCoef));
	int xWidth=xEnd-xStart;
	//竖直起始位置
	bFindFrameline=false;
	for(int j=data_rect.top;j<data_rect.bottom;j++)
	{
		nBlack=0;
		int iFirstBlackIndex=0;
		for(int i=xStart;i<=xEnd;i++)
		{
			if(IsBlackPixel(i,j))
				nBlack++;
			if(nBlack>=MINPIXELS_OF_VERTFRAMELINE)
			{
				bFindFrameline=true;
				break;
			}
		}
		bFindFrameline|=(j-data_rect.top+1)>=FRAMELINE_TOLERANCE;	//超出搜索容差范围，则视为已跳过单元格框线
		if(nBlack<MINPIXELS_OF_VERTFRAMELINE&&bFindFrameline)
		{
			yStart=j;
			break;
		}
	}
	//竖直终止位置
	bFindFrameline=false;
	for(int j=data_rect.bottom;j>data_rect.top;j--)
	{
		nBlack=0;
		for(int i=xStart;i<=xEnd;i++)
		{
			if(IsBlackPixel(i,j))
				nBlack++;
			if(nBlack>=MINPIXELS_OF_VERTFRAMELINE)
			{
				bFindFrameline=true;
				break;
			}
		}
		bFindFrameline|=(data_rect.bottom-j+1)>=FRAMELINE_TOLERANCE;	//超出搜索容差范围，则视为已跳过单元格框线
		if(nBlack<MINPIXELS_OF_VERTFRAMELINE&&bFindFrameline)
		{
			yEnd=j;
			break;
		}
	}
}
int CImageDataRegion::DetectConnBorderX(int xi,int xiDetectDepth,int yjTop,int yjBtm,bool* xarrColState/*=NULL*/,bool bToLeftTrueRightFalse/*=true*/)
{
	PRESET_ARRAY<bool,PRESET_PIXELS160> xarrColBlckPixels;
	DYN_ARRAY<bool> xarrDynColPixels(0);
	bool *parrCurrColBlckPixels=NULL;
	if(yjBtm-yjTop+1<PRESET_PIXELS160::COUNT)
	{
		xarrColBlckPixels.ZeroPresetMemory();
		parrCurrColBlckPixels = xarrColBlckPixels.Data();
	}
	else
	{
		xarrDynColPixels.Resize(yjBtm-yjTop+1,true);
		parrCurrColBlckPixels=xarrDynColPixels;
	}
	int j,xiNextCol=bToLeftTrueRightFalse?xi-1:xi+1;
	for(j=yjTop;j<=yjBtm;j++)
	{
		bool blckpixel=(xarrColState!=NULL)?xarrColState[j-yjTop]:IsBlackPixel(xi,j);
		if(!blckpixel)
			continue;
		if(IsBlackPixel(xiNextCol,j))
			parrCurrColBlckPixels[j-yjTop]=true;
		if(j>yjTop&&!parrCurrColBlckPixels[j-yjTop]&&IsBlackPixel(xiNextCol,j-1))
			parrCurrColBlckPixels[j-yjTop-1]=true;
		if(j<yjBtm&&IsBlackPixel(xiNextCol,j+1))
			parrCurrColBlckPixels[j-yjTop+1]=true;
	}
	bool hasConnBlckPixel=false;
	for(j=yjTop;!hasConnBlckPixel&&j<=yjBtm;j++)
		hasConnBlckPixel|=parrCurrColBlckPixels[j-yjTop];
	if(xiDetectDepth<=1)
		return hasConnBlckPixel?xiNextCol:xi;
	xiDetectDepth--;
	if(hasConnBlckPixel)
		return this->DetectConnBorderX(xiNextCol,xiDetectDepth,yjTop,yjBtm,parrCurrColBlckPixels,bToLeftTrueRightFalse);
	else//if(!hasConnBlckPixel)
		return xi;
}
void CImageDataRegion::CalCellTextRect(RECT& rcCell,double vfScaleCoef,LONG& xStart,LONG& yStart,LONG& xEnd,LONG& yEnd,int iDataType)
{
	int i,j,nBlack=0;
	int FRAMELINE_TOLERANCE=max(2,f2i(1.5*vfScaleCoef));	//识别时锚定角点距离表格线的搜索容差范围
	int MINPIXELS_OF_TEXTVERTLINE=max(1,f2i(1*vfScaleCoef));	//单元格内横线或竖线识别为空白的最多像素数，既文本区域竖线中最少像素数
	int MARGIN=f2i(3*vfScaleCoef);	//防止因上下边框线引起的噪音点误判为文本区横向起止位置 wjh-2018.3.29
	if (this->BelongImageData() && this->BelongImageData()->IsLowBackgroundNoise())
		MINPIXELS_OF_TEXTVERTLINE=1;	//低噪点背景下可不考虑噪声点影响，不考虑列最少有效黑点数 wjh-2019.11.28
	//因为单元格中仅有一行文字，故先判断文本区域左侧起始位置
	bool bFindBlankMargin=false;
	int minBlackColI=rcCell.left,minBlackPixels=m_nHeight;
	bool bInitStartX=false,bInitEndX=false;
	for(i=rcCell.left;i<=rcCell.right;i++)
	{
		nBlack=0;
		int nNextBlack=0;
		for(j=rcCell.top+MARGIN;j<=rcCell.bottom-MARGIN;j++)
		{
			if(nBlack<MINPIXELS_OF_TEXTVERTLINE&&IsBlackPixel(i,j))
				nBlack++;
			if(nNextBlack<MINPIXELS_OF_TEXTVERTLINE&&IsBlackPixel(i+1,j))
				nNextBlack++;
			if(nBlack>=MINPIXELS_OF_TEXTVERTLINE&&nNextBlack>=MINPIXELS_OF_TEXTVERTLINE&&i-rcCell.left>=FRAMELINE_TOLERANCE)
				break;
		}
		if(nBlack<MINPIXELS_OF_TEXTVERTLINE)
			bFindBlankMargin=true;
		else if(nBlack<minBlackPixels)
		{
			minBlackColI=i;
			minBlackPixels=nBlack;
		}
		if(nBlack>=MINPIXELS_OF_TEXTVERTLINE && nNextBlack>=MINPIXELS_OF_TEXTVERTLINE)
		{  
			if(bFindBlankMargin)
				xStart=i;				//已发现并跳过空白区域，再遇到的集中黑点区域则视为文区域
			else if(i-rcCell.left>=FRAMELINE_TOLERANCE)
				xStart=rcCell.left;		//超出搜索容差范围，则视为已跳过单元格框线
			else
				continue;
			bInitStartX=true;
			break;
		}
	}
	if(!bInitStartX)
	{
		xStart=xEnd=rcCell.left;
		yStart=yEnd=rcCell.top;
		return;
	}
	//因为单元格中仅有一行文字，故先判断文本区域右侧起始位置
	bFindBlankMargin=false;
	minBlackColI=rcCell.right,minBlackPixels=m_nHeight;
	for(int i=rcCell.right;i>=rcCell.left;i--)
	{
		nBlack=0;
		int nPreBlack=0;
		for(int j=rcCell.top+MARGIN;j<=rcCell.bottom-MARGIN;j++)
		{
			if(nBlack<MINPIXELS_OF_TEXTVERTLINE&&IsBlackPixel(i,j))
				nBlack++;
			if(nPreBlack<MINPIXELS_OF_TEXTVERTLINE&&IsBlackPixel(i-1,j))
				nPreBlack++;
			if(nBlack>=MINPIXELS_OF_TEXTVERTLINE&&nPreBlack>=MINPIXELS_OF_TEXTVERTLINE&&rcCell.right-i>=FRAMELINE_TOLERANCE)
				break;
		}
		if(nBlack<MINPIXELS_OF_TEXTVERTLINE)
			bFindBlankMargin=true;
		else if(nBlack<minBlackPixels)
		{
			minBlackPixels=nBlack;
			minBlackColI=i;
		}
		if(nBlack>=MINPIXELS_OF_TEXTVERTLINE && nPreBlack>=MINPIXELS_OF_TEXTVERTLINE)
		{  
			if(bFindBlankMargin)
				xEnd=i;				//已发现并跳过空白区域，再遇到的集中黑点区域则视为文区域
			else if(rcCell.right-i>=FRAMELINE_TOLERANCE)
				xEnd=minBlackColI;	//超出搜索容差范围，则视为已跳过单元格框线,以过往搜索到黑点最少的列为边界
			else
				continue;
			bInitEndX=true;
			break;
		}
	}
	if(!bInitEndX)
	{
		xStart=xEnd=rcCell.left;
		yStart=yEnd=rcCell.top;
		return;
	}
	//文本区域横线中最少像素数12=0.7*17,极端情况一些字体较细可能出现f2i((xEnd-xStart+1)/12.0)=0 wjh-2018.3.30
	int MINPIXELS_OF_TEXTHORZLINE=max(1,f2i((xEnd-xStart+1)/12.0));
	if (this->BelongImageData() && this->BelongImageData()->IsLowBackgroundNoise())
		MINPIXELS_OF_TEXTHORZLINE=1;	//低噪点背景下可不考虑噪声点影响，不考虑行最少有效黑点数 wjh-2019.11.28
	PREFER_BAND prefer,band;
	static bool poolbytes[1024],poolbytes2[1024];
	DYN_ARRAY<bool>prevrow(xEnd+1,false,poolbytes,1024),nextrow(xEnd+1,false,poolbytes,1024);
	for(int j=rcCell.top;j<=rcCell.bottom;j++)
	{
		nBlack=0;
		int nNextBlack=0;
		bool bConnectWithUpper=false;
		for(int i=xStart;i<=xEnd;i++)
		{	
			bool currpixel=j>rcCell.top?nextrow[i]:IsBlackPixel(i,j);
			if(currpixel)//IsBlackPixel(i,j))
				nBlack++;
			if(j>rcCell.top&&(currpixel&&prevrow[i]))
				bConnectWithUpper=true;
			prevrow[i]=currpixel;
			//if(nBlack>=MINPIXELS_OF_TEXTHORZLINE)
			//	break;
		}
		for(int i=xStart;i<=xEnd;i++)
		{	
			bool black=IsBlackPixel(i,j+1);
			if(nextrow[i]=black)
				nNextBlack++;
			//if(nNextBlack>=MINPIXELS_OF_TEXTHORZLINE)
			//	break;
		}
		if( bConnectWithUpper||(
			nBlack>=MINPIXELS_OF_TEXTHORZLINE && nNextBlack>=MINPIXELS_OF_TEXTHORZLINE))
		{
			if(!band.initStart)
				band.VerifyPixelY(j);//band.yStart=j;band.initStart=true;
			else
				band.yEnd=bConnectWithUpper?j:j+1;
		}
		else
		{	//遇到空白行
			if(band.initEnd&&band.Height>prefer.Height)
				prefer=band;
			band.Clear();
		}
	}
	if(!prefer.initStart&&band.initStart)
		prefer=band;	//用唯一个黑点带初始化
	else if(band.initEnd&&band.Height>prefer.Height)
		prefer=band;
	if(prefer.initStart)
		yStart=prefer.yStart;
	if(prefer.initEnd)
		yEnd=prefer.yEnd<=rcCell.bottom?prefer.yEnd:rcCell.bottom;
	//因为Y向有效范围发生变化，有可能影响到文本有效区的左右侧起止位置，故需要复查一下
	xStart=this->DetectConnBorderX(xStart,10,yStart,yEnd,NULL,true);
	xEnd  =this->DetectConnBorderX(xEnd,10,yStart,yEnd,NULL,false);
	for(int i=xStart;i<=xEnd;i++)
	{
		nBlack=0;
		int nNextBlack=0;
		for(int j=yStart;j<=yEnd;j++)
		{
			if(nBlack<MINPIXELS_OF_TEXTVERTLINE&&IsBlackPixel(i,j))
				nBlack++;
			if(nNextBlack<MINPIXELS_OF_TEXTVERTLINE&&IsBlackPixel(i+1,j))
				nNextBlack++;
			if(nBlack>=MINPIXELS_OF_TEXTVERTLINE&&nNextBlack>=MINPIXELS_OF_TEXTVERTLINE)
				break;
		}
		if(nBlack>=MINPIXELS_OF_TEXTVERTLINE && nNextBlack>=MINPIXELS_OF_TEXTVERTLINE)
		{
			xStart=i;
			break;
		}
	}
	for(int i=xEnd;i>=xStart;i--)
	{
		nBlack=0;
		int nPreBlack=0;
		for(int j=yStart;j<=yEnd;j++)
		{
			if(nBlack<MINPIXELS_OF_TEXTVERTLINE&&IsBlackPixel(i,j))
				nBlack++;
			if(nPreBlack<MINPIXELS_OF_TEXTVERTLINE&&IsBlackPixel(i-1,j))
				nPreBlack++;
			if(nBlack>=MINPIXELS_OF_TEXTVERTLINE&&nPreBlack>=MINPIXELS_OF_TEXTVERTLINE)
				break;
		}
		if(nBlack>=MINPIXELS_OF_TEXTVERTLINE && nPreBlack>=MINPIXELS_OF_TEXTVERTLINE)
		{
			xEnd=i;				//已发现并跳过空白区域，再遇到的集中黑点区域则视为文区域
			break;
		}
	}
	//统计每行黑点数，删除噪音点
	if(iDataType==CImageCellRegion::PARTNO||iDataType==CImageCellRegion::GUIGE)
	{
		int nWidth=xEnd-xStart+1;
		for(int j=yStart;j<=yEnd;j++)
		{	
			nBlack=0;	//统计每行的黑点数
			int iFirstBlack=0,iEndBlack=0;
			for(int i=xStart;i<=xEnd;i++)
			{
				if(IsBlackPixel(i,j))
				{
					if(iFirstBlack==0)
						iFirstBlack=i;
					iEndBlack=i;
					nBlack++;
				}
			}
			//顶行黑点的起止区间占文本整宽的比例值(高噪声时影响真实文本Y向区域限制） wjh-2019.11.28
			double TOPLINE_BLCKPIXELS_MINSCALECOEF=0.7;
			if (this->BelongImageData() && this->BelongImageData()->IsLowBackgroundNoise())
				TOPLINE_BLCKPIXELS_MINSCALECOEF=0;
			double fScopeScale=(iEndBlack-iFirstBlack+1)/(double)nWidth;
			if(nBlack>MINPIXELS_OF_TEXTHORZLINE&&fScopeScale>TOPLINE_BLCKPIXELS_MINSCALECOEF)
			{
				yStart=j;
				break;
			}
		}
	}
}
//计算文本的最小有效区域,xEnd,yEnd分别为右侧及底部含黑点的边界
void CImageDataRegion::CalCharValidRect(RECT data_rect,int iDataType,LONG& xStart,LONG& yStart,LONG& xEnd,LONG& yEnd)
{
	//单元格高度按两个字高计算，初始参数统计时以字高17像素为基准，故此处以单元格高35像素为基准 wjh-2018.3.26
	double vfScaleCoef=(data_rect.bottom-data_rect.top)/35.0;
	//Step 1. 第一次计算文本在矩形框中的空白起始位置及终止位置
	RECT rcCell;
#ifdef _TIMER_COUNT
	timer.Relay();
#endif
	CalCellInternalRect(data_rect,vfScaleCoef,rcCell.left,rcCell.top,rcCell.right,rcCell.bottom);
#ifdef _TIMER_COUNT
	timer.Relay(4001);
#endif
	//Step 2. 移除单元格内的噪声点
	//TODO:未考虑小数点字符像素数较少情况　wjh-2018.3.19
	int MAX_ISLAND_PIXELS=max(10,f2i(5*vfScaleCoef));
	//if(iDataType==CImageCellRegion::GUIGE)	//'-'字符像素较少
	if(iDataType==CImageCellRegion::NUM)
		MAX_ISLAND_PIXELS=max(15,f2i(15*vfScaleCoef));
	long liNoisePixels=this->RemoveNoisePixels(&rcCell,MAX_ISLAND_PIXELS);
	if (liNoisePixels<10 && BelongImageData())	//自动设定为低噪声背景状态 wjh-2019.11.28
		BelongImageData()->SetLowBackgroundNoise(true);
#ifdef _TIMER_COUNT
	timer.Relay(4002);
#endif
	//Step 3. 收缩矩形到有效文本区域（判断条件从四周跳过空白区域）
	CalCellTextRect(rcCell,vfScaleCoef,xStart,yStart,xEnd,yEnd,iDataType);
#ifdef _TIMER_COUNT
	timer.Relay(4003);
#endif
}
//初始化单元文本区域信息
void CImageDataRegion::RetrieveCellRgn(CImageCellRegion& cell,RECT rect)
{
	cell.InitBitImage(NULL,rect.right-rect.left+1,rect.bottom-rect.top+1,0);
	//unitRgn.m_nWidth=rect.right-rect.left;
	//unitRgn.m_nHeight=rect.bottom-rect.top;
	//unitRgn.m_nWidth%16>0?unitRgn.m_nEffWidth=(unitRgn.m_nWidth/16)*2+2:unitRgn.m_nEffWidth=unitRgn.m_nWidth/8;
	//unitRgn.m_lpBitMap=new BYTE[unitRgn.m_nEffWidth*unitRgn.m_nHeight];
	//memset(unitRgn.m_lpBits,0,unitRgn.m_nEffWidth*unitRgn.m_nHeight);
	for(int i=rect.left;i<=rect.right;i++)
	{
		for(int j=rect.top;j<=rect.bottom;j++)
		{
			if(IsBlackPixel(i,j))	//默认为白点，只有黑点时需要设定
				cell.SetPixelState(i-rect.left,j-rect.top,true);
			//if(!IsBlackPixel(i,j))
			//{
			//	int x=i-rect.left;
			//	int y=j-rect.top;
			//	int iByte=x/8,iBit=x%8;
			//	unitRgn.m_lpBits[y*unitRgn.m_nEffWidth+iByte]|=BIT2BYTE[7-iBit];
			//}
		}
	}
}
//识别数据内容
#ifdef _DEBUG
CXhChar50 CImageDataRegion::RecognizeDatas(CELL_RECT &data_rect,int idCellTextType,IMAGE_CHARSET_DATA* imagedata/*=NULL*/,BYTE* pcbWarningLevel/*=NULL*/,bool bExportImgLogFile/*=false*/,BYTE biStdPart/*=0*/)
#else
CXhChar50 CImageDataRegion::RecognizeDatas(CELL_RECT &data_rect,int idCellTextType,IMAGE_CHARSET_DATA* imagedata/*=NULL*/,BYTE* pcbWarningLevel/*=NULL*/,BYTE biStdPart/*=0*/)
#endif
{
	CXhChar50 sValue;
	//计算文本的最小有效区域
	LONG xStart=0,xEnd=0;
	LONG yStart=0,yEnd=0;
	int i,j;
#ifdef _DEBUG
	CLogErrorLife life;
	if(bExportImgLogFile)
	{
	logerr.Log("-----------原始单元格区域-------------\n");
	for(j=data_rect.top;j<=data_rect.bottom;j++)
	{
		for(i=data_rect.left;i<=data_rect.right;i++)
		{
			if(IsBlackPixel(i,j))
				logerr.LogString(".",false);
			else
				logerr.LogString(" ",false);
		}
		logerr.Log("\n");
	}
	}
#endif
	RECT rcCell=data_rect.rect;
	if(idCellTextType==CImageCellRegion::NUM)
	{	//通过设定一个左右横向边距去除因边框线造成粗判时横向有效区不准进而导致
		//纵向有效区黑点像素数判断因子MINPIXELS_OF_TEXTHORZLINE失准 wjh-2018.3.29
		int width=data_rect.right-data_rect.left;
		int height=data_rect.bottom-data_rect.top;
		double vfScaleCoef=height/35.0;
		//存在数量列文本不居中现像，故此横向边距项也不宜取值太大
		int HORI_MARGIN=robot.GetHoriMargin(width,vfScaleCoef);//min(10,width-f2i(35*vfScaleCoef));
		rcCell.left	+=HORI_MARGIN;
		rcCell.right-=HORI_MARGIN;
	}
#ifdef _TIMER_COUNT
	DWORD dwStart400Tick=timer.Relay();
#endif
	CalCharValidRect(rcCell,idCellTextType,xStart,yStart,xEnd,yEnd);
	if(xEnd-xStart<2)
		return sValue="";	//返回空单元格
	double fRatioOfActualHeight=(yEnd-yStart+1.0)/(data_rect.bottom-data_rect.top+1.0);
	if(pcbWarningLevel)	//清零警示信息
		*pcbWarningLevel=0;
	if(fRatioOfActualHeight<0.4||fRatioOfActualHeight>0.85)
		*pcbWarningLevel=WARNING_LEVEL::ABNORMAL_TXT_REGION;
	if(imagedata!=NULL)
	{	//返回单元格区域图像，以便从单元格中提取字体
		imagedata->nWidth=xEnd-xStart+1;
		imagedata->nHeight=yEnd-yStart+1;
		imagedata->nEffWidth=imagedata->nWidth%8>0?imagedata->nWidth/8+1:imagedata->nWidth/8;
		if(imagedata->uiPreAllocSize>=imagedata->nEffWidth*imagedata->nHeight)
		{
			for(j=yStart;j<=yEnd;j++)
			{
				for(i=xStart;i<=xEnd;i++)
				{
					bool black=IsBlackPixel(i,j);
					int iByte=(i-xStart)/8,iBit=(i-xStart)%8;
					BYTE* pImageByte=(BYTE*)&imagedata->imagedata[(j-yStart)*imagedata->nEffWidth+iByte];
					SetByteBitState(pImageByte,iBit,!black);
				}
			}
		}
	}
#ifdef _TIMER_COUNT
	timer.Relay(400,dwStart400Tick);
#endif
#ifdef _DEBUG
	if(bExportImgLogFile)
	{
	logerr.Log("-----------有效区域-------------\n");
	for(j=data_rect.top;j<=data_rect.bottom;j++)
	{
		for(i=data_rect.left;i<=data_rect.right;i++)
		{
			if(j<yStart||j>yEnd||i<xStart||i>xEnd)
				logerr.LogString("B",false);
			else if(IsBlackPixel(i,j))
				logerr.LogString(".",false);
			else
				logerr.LogString(" ",false);
		}
		logerr.Log("\n");
	}
	for(i=data_rect.left;i<xStart;i++)
		logerr.LogString(" ",false);
	}
#endif
	if(xEnd<=xStart || yEnd<=yStart)
		return sValue;
	data_rect.left=xStart;
	data_rect.right=xEnd;
	data_rect.top=yStart;
	data_rect.bottom=yEnd;
	//初始化文本单元,拆分识别字符
	CImageCellRegion cell;
	//笔画矢量图生成的图像，这种图像几乎不存在失真，故最大程度保留了各字符的特征信息，适用于特征识别优先 wjh-2018.8.31
	cell.m_bStrokeVectorImage=(BelongImageData() && BelongImageData()->GetRawFileType()==CImageDataFile::RAW_IMAGE_PDF);
	//static const int NUM		= 4;
	//static const int PIECE_W	= 5;
	//static const int SUM_W		= 6;
	//static const int NOTE		= 7;
	RetrieveCellRgn(cell,data_rect.rect);
	cell.m_nDataType=idCellTextType;
	cell.m_biStdPartType=biStdPart;
	cell.m_nMaxCharCout=(int)(cell.m_nWidth/(0.6*cell.m_nHeight));
	cell.m_nLeftMargin=xStart-rcCell.left;	//有效区域与单元格左侧边界线距离，用来处理表格线压字问题 wht 18-07-30
	cell.m_nRightMargin=rcCell.right-xEnd;		//有效区域与单元格右侧边界线距离
	if(idCellTextType!=CImageCellRegion::GUIGE&&idCellTextType!=CImageCellRegion::PARTNO)
		cell.UpdateFontTemplCharState();	//规格列中需要根据当前识别的字符数量实时设定模版字符的识别状态
#ifdef _DEBUG
	if(bExportImgLogFile)
	{
		logerr.Log("-----------单元格最终有效内容-------------\n");
		for(j=data_rect.top;j<=data_rect.bottom;j++)
		{
			for(i=data_rect.left;i<=data_rect.right;i++)
			{
				if(IsBlackPixel(i,j))
					logerr.LogString(".",false);
				else
					logerr.LogString(" ",false);
			}
			logerr.Log("\n");
		}
	}
#endif
#ifdef _TIMER_COUNT
	DWORD dwStart402Tick=timer.Relay(401);
#endif
	//TODO:将来应根据cell的有效区域智能识别ABNORMAL_TXT_REGION异常 wjh-18.4.16
	CAlphabets* pAlphabets=(CAlphabets*)IMindRobot::GetAlphabetKnowledge();
	double dfNewStackValue=pAlphabets->m_fMinWtoHcoefOfChar1;
	if (this->BelongImageData() && this->BelongImageData()->IsLowBackgroundNoise())//IsThinFontText())
		dfNewStackValue=0.1;	//细字体时，以标准40高字体，横向‘1'字符最宽处最少应具备4个像素 wjh-2019.11.28
	CStackVariant<double> stackvar(&pAlphabets->m_fMinWtoHcoefOfChar1,dfNewStackValue);
	cell.xWarningLevel.uiWarning=0;
	cell.SplitImageCharSet(0,0,true,imagedata,CELLTYPE_FLAG(idCellTextType,true));		//拆分识别字符
	if(pcbWarningLevel)
		*pcbWarningLevel=cell.xWarningLevel.WaringLevelNumber();
#ifdef _TIMER_COUNT
	timer.Relay(402,dwStart402Tick);
#endif
	if(cell.SpellCellStrValue().GetLength()>0)
		sValue=cell.SpellCellStrValue();
#ifdef _DEBUG
	if(bExportImgLogFile)
		cell.PrintCharMark();
#endif
	return sValue;
}
void CImageDataRegion::ParseDataValue(CXhChar50 sText,int data_type,char* sValue1
									,char* sValue2/*=NULL*/,int cls_id/*=0*/)
{
	if(data_type==CImageCellRegion::GUIGE)
	{
		if(strstr(sText,"L"))
			cls_id=BOM_PART::ANGLE;
		else if(strstr(sText,"-"))
			cls_id=BOM_PART::PLATE;
		else if(strstr(sText,"φ")||strstr(sText,"Φ"))
			cls_id=BOM_PART::TUBE;
		else if(strstr(sText,"[")||(sText.Length<4&&strstr(sText,"C")))
			cls_id=BOM_PART::SLOT;
		else
			cls_id=0;
		strcpy(sValue1,"");
		CXhChar16 sMat;
		if(strstr(sText,"Q"))
		{	//"Q345","Q390","Q420","Q460"
			sText.Replace("L"," ");
			sText.Replace("-"," ");
			int nLittle=sText.Replace("φ"," ");
			int nBig=sText.Replace("Φ"," ");
			sscanf(sText,"%s%s",(char*)sMat,sValue1);
			if(sMat[2]=='4')
			{
				sMat[1]='3';
				sMat[3]='5';
			}
			else if(sMat[2]=='2')
			{
				sMat[1]='4';
				sMat[3]='0';
			}
			//提取材质后将类型字符添加会规格字符中
			CXhChar100 sSpec;
			if(cls_id==BOM_PART::ANGLE)
				sSpec.Append("L");
			else if(cls_id==BOM_PART::PLATE)
				sSpec.Append("-");
			else if(cls_id==BOM_PART::TUBE)
			{
				if(nBig>0)
					sSpec.Append("Φ");
				else
					sSpec.Append("φ");
			}
			sSpec.Append(sValue1);
			strcpy(sValue1,sSpec);
		}
		else
		{
			sText.Remove(' ');
			int len=strlen(sText);
			char cBriefMat=len>1?sText.At(len-1):' ';
			bool bBriefMatInMiddle=false;
			if(!CImageChar::IsMaterialChar(cBriefMat)&&BOM_PART::PLATE==cls_id)
			{	//钢板简化材质字符可能在厚度之后X之前 wht 19-01-15
				char cMat=0;
				for(int i=0;i<sText.GetLength();i++)
				{
					char cCur=sText.At(i);
					if(cCur=='X'||cCur=='x')
						break;
					cMat=cCur;
				}
				if(CImageChar::IsMaterialChar(cMat))
				{
					cBriefMat=cMat;
					bBriefMatInMiddle=true;
				}
			}
			if(CImageChar::IsMaterialChar(cBriefMat))
			{
				sMat=QuerySteelMaterialStr(cBriefMat);
				if(bBriefMatInMiddle)		//简化材质字符在钢板厚度之后X之前 wht 19-01-15
					sText.Remove(cBriefMat);//移除材质厚长度-1
				strncpy(sValue1,sText,len-1);
			}
			else 
			{
				strcpy(sMat,"Q235");
				strcpy(sValue1,sText);
			}
		}	
		if(strlen(sValue1)==0)
			strcpy(sValue1,sText);
		if(sValue2)
			strcpy(sValue2,sMat);
	}
	else if(data_type==CImageCellRegion::PIECE_W)
	{	//单重倒数第三位为小数点
		strcpy(sValue1,sText);
		int len=strlen(sValue1);
		if(len>3)
			sValue1[len-3]='.';
	}
	else if(data_type==CImageCellRegion::SUM_W)
	{	//总重倒数第二位为小数点
		strcpy(sValue1,sText);
		int len=strlen(sValue1);
		if(len>2)
			sValue1[len-2]='.';
	}
	else
		strcpy(sValue1,sText);
}
IImageFile* CImageDataRegion::BelongImageData() const
{
	return m_pImageData;
}
//TODO:Part I
void CImageDataRegion::InitImageShowPara(RECT showRect)
{
	if(m_fDisplayAllRatio==0)
	{
		double scale_h=(double)(showRect.right-showRect.left)/m_nWidth;
		double scale_v=(double)(showRect.bottom-showRect.top)/m_nHeight;
		m_nOffsetX=m_nOffsetY=0;
		m_fDisplayAllRatio=min(1,scale_h>scale_v?scale_v:scale_h);
		//m_fDisplayRatio=m_fDisplayAllRatio;	//默认全显
		m_fDisplayRatio=min(1,scale_h>scale_v?scale_h:scale_v);
	}
}
int CImageDataRegion::GetMonoImageData(IMAGE_DATA* imagedata)	//每位代表1个像素，0表示白点,1表示黑点
{
	imagedata->nHeight=m_nHeight;
	imagedata->nWidth=m_nWidth;
	imagedata->nEffWidth=m_nEffWidth;//m_nWidth%16>0 ? (m_nWidth/16)*2+2 : m_nWidth/8;
	int count=imagedata->nEffWidth*m_nHeight;
	if(imagedata->imagedata)
		memcpy(imagedata->imagedata,m_lpBitMap,count);
	return count;
}
int CImageDataRegion::GetRectMonoImageData(RECT* rc,IMAGE_DATA* imagedata)		//每位代表1个像素，0表示白点,1表示黑点
{	//不包含rc->right列及rc->bottom行
	if(rc==NULL||imagedata==NULL)
		return 0;
	imagedata->nHeight=rc->bottom-rc->top+1;
	imagedata->nWidth=rc->right-rc->left+1;
	imagedata->nEffWidth=(imagedata->nWidth/8);
	if(imagedata->nEffWidth%2>0)	//原始单行图像数据为单数，无论如何补1个字节对齐
		imagedata->nEffWidth+=1;
	else if(imagedata->nWidth%8>0)	//原始单行图像数据为双数
		imagedata->nEffWidth+=2;	//单色图像数据每行以2Byte对齐
	//else if(imagedata->nWidth%16==0)	//整好单行像素数为16的倍数, 即可保证2字节对齐
	if(imagedata->imagedata==NULL)
		return 0;
	else
	{
		//if(imagedata->nWidth>=6000||imagedata->nHeight>=6000)
		//{
		//	logerr.Log("error image memory copy");
		//	return 0;
		//}
		if(rc->left==0&&imagedata->nEffWidth==m_nEffWidth)
			memcpy(imagedata->imagedata,&m_lpBitMap[m_nEffWidth*rc->top+rc->left],imagedata->nHeight*imagedata->nEffWidth);
		else
		{
			long dwByteSumCount=m_nEffWidth*m_nHeight;
			//多出来两个字节是为移位等因素带来的多拷贝数据带来的裕度空间
			CHAR_ARRAY imgdata_pool(imagedata->nEffWidth+2);
			BYTE lowMaskBytes [8]={0x00,0x80,0xc0,0xe0,0xf0,0xf8,0xfc,0xfe};
			BYTE highMaskBytes[8]={0xff,0xfe,0xfc,0xf8,0xf0,0xe0,0xc0,0x80};
			for(int rowi=rc->top;rowi<=rc->bottom;rowi++)
			{
				char* imagerow_data=imagedata->imagedata+imagedata->nEffWidth*(rowi-rc->top);
				int moveleft=rc->left%8,iStartCopyAddr=rc->left/8;
				int nReadBytes=min(imagedata->nEffWidth+2,m_nEffWidth-iStartCopyAddr);
				int iStartPos=(m_nEffWidth*rowi+iStartCopyAddr);
				if(nReadBytes<0||(iStartPos+nReadBytes)>dwByteSumCount)
				{
					break;	//最后一行时内容可能会溢出导致异常 wht 18-07-12
				}
				memcpy((char*)imgdata_pool,&m_lpBitMap[m_nEffWidth*rowi+iStartCopyAddr],nReadBytes);
				char* prevbyte=(char*)imgdata_pool,*currbyte=NULL;
				for(DWORD i=1;i<imgdata_pool.Size();i++)
				{
					currbyte=prevbyte+1;
					(*prevbyte)<<=moveleft;
					WORD wch=(*currbyte)&lowMaskBytes[moveleft];	//升为双字节，同时清掉低位字节（左移时不会移到前一字节的像素位)
					wch<<=moveleft;	//左移相应有效位至高位字节
					wch>>=8;		//右移一字节，以获取当前字节向上一字节的移位内容
					(*prevbyte)&=highMaskBytes[moveleft];
					(*prevbyte)|=*((char*)&wch);	//将当前字节中相应高位左移至上一字节的低位
					prevbyte=currbyte;
				}
				(*prevbyte)<<=moveleft;
				//将进行移位处理后的行像素拷贝至目标图像数据的目标行
				memcpy(imagerow_data,(char*)imgdata_pool,imagedata->nEffWidth);
			}
		}
		return imagedata->nHeight*imagedata->nEffWidth;
	}
}
int CImageDataRegion::GetImageColumnWidth(int* col_width_arr/*=NULL*/)		//返回列数
{
	if(col_width_arr!=NULL)
		memcpy(col_width_arr,m_arrColCellsWidth.m_pData,m_arrColCellsWidth.GetSize()*4);
	return m_arrColCellsWidth.GetSize();
}
void CImageDataRegion::SetScrOffset(SIZE offset)
{
	m_nOffsetX=offset.cx;
	m_nOffsetY=offset.cy;
}
SIZE CImageDataRegion::GetScrOffset()
{
	SIZE size;
	size.cx=m_nOffsetX;
	size.cy=m_nOffsetY;
	return size;
}
POINT CImageDataRegion::MappingToScrOrg()
{
	POINT org;
	org.x=m_nOffsetX;
	org.y=m_nOffsetY;
	return org;
}
RECT CImageDataRegion::MappingToScrRect()
{
	RECT rc;
	rc.left=m_nOffsetX;
	rc.top=m_nOffsetY;
	rc.right=f2i(m_nWidth*m_fDisplayRatio)+m_nOffsetX;
	rc.bottom=f2i(m_nHeight*m_fDisplayRatio)+m_nOffsetY;
	return rc;
}
#define      FIXED_SCLAE	  1.8
double CImageDataRegion::Zoom(short zDelta,POINT* pxScrPoint/*=NULL*/)
{
	double HALF_SCALE_OF_ZOOMALL=m_fDisplayAllRatio/2;
	double fMinDisplayRatio=min(0.5,HALF_SCALE_OF_ZOOMALL);
	double fOldRatioScr2Img=m_fDisplayRatio;
	if(zDelta==0)		//全显
		m_fDisplayRatio=m_fDisplayAllRatio;
	else if(zDelta>0)	//放大
		m_fDisplayRatio*=FIXED_SCLAE;
	else if(m_fDisplayRatio>fMinDisplayRatio)	//缩小
		m_fDisplayRatio=max(fMinDisplayRatio,m_fDisplayRatio/FIXED_SCLAE);
	if(pxScrPoint)
	{	//以指定屏幕位置为当前缩放中心
		int offset_x=this->m_nOffsetX-pxScrPoint->x;
		int offset_y=this->m_nOffsetY-pxScrPoint->y;
		double realOffsetX=offset_x/fOldRatioScr2Img;
		double realOffsetY=offset_y/fOldRatioScr2Img;
		m_nOffsetX=pxScrPoint->x+f2i((m_nOffsetX-pxScrPoint->x)*m_fDisplayRatio/fOldRatioScr2Img);
		m_nOffsetY=pxScrPoint->y+f2i((m_nOffsetY-pxScrPoint->y)*m_fDisplayRatio/fOldRatioScr2Img);
	}
	return m_fDisplayRatio;
}
POINT CImageDataFile::MappingToScrOrg()
{
	POINT org;
	org.x=m_nOffsetX;
	org.y=m_nOffsetY;
	return org;
}
RECT  CImageDataFile::MappingToScrRect()
{
	RECT rc;
	rc.left=m_nOffsetX;
	rc.top=m_nOffsetY;
	rc.right=f2i(Width *m_fDisplayRatio)+m_nOffsetX;
	rc.bottom=f2i(Height*m_fDisplayRatio)+m_nOffsetY;
	return rc;
}
POINT CImageDataRegion::TopLeft(bool bRealRegion/*=false*/)
{
	return bRealRegion?realLeftUp:leftUp;
}
POINT CImageDataRegion::TopRight(bool bRealRegion/*=false*/)
{
	return bRealRegion?realRightUp:rightUp;
}
POINT CImageDataRegion::BtmLeft(bool bRealRegion/*=false*/)
{
	return bRealRegion?realLeftDown:leftDown;
}
POINT CImageDataRegion::BtmRight(bool bRealRegion/*=false*/)
{
	return bRealRegion?realRightDown:rightDown;
}
bool CImageDataRegion::SetRegion(POINT topLeft,POINT btmLeft,POINT btmRight,POINT topRight,bool bRealRegion/*=false*/,bool bRetrieveRegion/*=true*/)
{
	if(bRealRegion)
	{
		realLeftDown=btmLeft;
		realLeftUp=topLeft;
		realRightDown=btmRight;
		realRightUp=topRight;
	}
	else
	{
		leftDown=btmLeft;
		leftUp=topLeft;
		rightDown=btmRight;
		rightUp=topRight;
	}
	//通过选取四个点确定数据区域
	if(bRetrieveRegion)
		return m_pImageData->RetrieveRegionImageBits(this);
	else
		return true;
	//m_pImageData->InitImageShowPara(rect,GetSerial());
	//return true;
}
void CImageDataRegion::UpdateImageRegion()
{
	m_pImageData->RetrieveRegionImageBits(this);
}
bool CImageDataRegion::Destroy()	//BelongImageData()->DeleteImageRegion(GetSerial())
{
	if (BelongImageData())
		return BelongImageData()->DeleteImageRegion(m_dwKey);
	else
		return false;
}
bool CImageDataRegion::EnumFirstBomPart(IRecoginizer::BOMPART* pRawBomPart)
{
	BOM_PART* pBomPart=hashBomParts.GetFirst();
	if(pBomPart==NULL)
		return NULL;
	pRawBomPart->id=pBomPart->id;
	StrCopy(pRawBomPart->sLabel,pBomPart->sPartNo,16);
	StrCopy(pRawBomPart->sSizeStr,pBomPart->sSpec,16);
	StrCopy(pRawBomPart->materialStr,QuerySteelMaterialStr(pBomPart->cMaterial),8);
	pRawBomPart->ciSrcFromMode=1;
	pRawBomPart->rc=pBomPart->rc;
	pRawBomPart->count=(short)pBomPart->GetPartNum();
	pRawBomPart->weight=pBomPart->fPieceWeight;
	pRawBomPart->sumWeight=pBomPart->fSumWeight;
	pRawBomPart->length=pBomPart->length;
	memcpy(pRawBomPart->arrItemWarningLevel,pBomPart->arrItemWarningLevel,IRecoginizer::BOMPART::WARNING_ARR_LEN);
	return true;
}
bool CImageDataRegion::EnumNextBomPart(IRecoginizer::BOMPART* pRawBomPart)
{
	BOM_PART* pBomPart=hashBomParts.GetNext();
	if(pBomPart==NULL)
		return NULL;
	pRawBomPart->id=pBomPart->id;
	StrCopy(pRawBomPart->sLabel,pBomPart->sPartNo,16);
	StrCopy(pRawBomPart->sSizeStr,pBomPart->sSpec,16);
	StrCopy(pRawBomPart->materialStr,QuerySteelMaterialStr(pBomPart->cMaterial),8);
	pRawBomPart->ciSrcFromMode=1;
	pRawBomPart->rc=pBomPart->rc;
	pRawBomPart->count=(short)pBomPart->GetPartNum();
	pRawBomPart->weight=pBomPart->fPieceWeight;
	pRawBomPart->sumWeight=pBomPart->fSumWeight;
	pRawBomPart->length=pBomPart->length;
	memcpy(pRawBomPart->arrItemWarningLevel,pBomPart->arrItemWarningLevel,7);
	return true;
}
UINT CImageDataRegion::GetBomPartNum()
{
	return hashBomParts.GetNodeNum();
}
bool CImageDataRegion::GetBomPartRect(UINT idPart,RECT* rc)
{
	if(rc==NULL)
		return false;
	BOM_PART* pBomPart=hashBomParts.GetValue(idPart);
	if(pBomPart)
		*rc=pBomPart->rc;
	return pBomPart!=NULL;
}
#include "ParseSizeSpecStr.h"
static bool RestoreSpec(const char* spec,float *width,float *thick,char *matStr=NULL)
{
	PARTSIZE parser;
	if(parser.ParseSizeStr(spec))
	{
		if(width)
			*width=parser.sfWidth;
		if(thick)
			*thick=parser.sfThick;
		if(matStr)
			StrCopy(matStr,parser.szMaterial,5);
		return true;
	}
	else
		return false;
	//以下代码以后废弃 wjh-2018.9.7
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
		sSpec.Replace("?"," ");
		sscanf(sSpec,"%f%f",width,thick);
	}
	else if (strstr(spec,"-"))
		sscanf(spec,"%c%f",sMat,thick);
	//else if(spec,"φ")
	//sscanf(spec,"%c%d%c%d",sMat,)
	if(matStr)
		strcpy(matStr,sMat);
	return true;
}
//提取数据
bool ISDIGIT(char ch){return ch>='0'&&ch<='9';}
bool IsClassicAngleSize(int anglesize_number);	//defined in PartLibrary.cpp

typedef struct tagPART_LABEL 
{
	BOM_PART *m_pPart;
	SEGI segI;
	SEGI serialNo;
	tagPART_LABEL(BOM_PART *pPart=NULL){
		Init(pPart);
	}
	void Init(BOM_PART *pPart){
		m_pPart=pPart;
		if(m_pPart)
		{
			CXhChar50 sSerialNo;
			ParsePartNo(pPart->sPartNo,&segI,sSerialNo);
			serialNo=SEGI(sSerialNo);
		}
		else
		{
			segI.iSeg=0;
			serialNo=SEGI((long)0);
		}
	}
}PART_LABEL;
void CheckPartLabel(CImageDataRegion *pRegion)
{
	ARRAY_LIST<PART_LABEL> partLabelArr(pRegion->hashBomParts.GetNodeNum());
	BOM_PART *pBomPart=NULL;
	for(pBomPart=pRegion->hashBomParts.GetFirst();pBomPart;pBomPart=pRegion->hashBomParts.GetNext())
	{
		PART_LABEL *pPartLabel=partLabelArr.append();
		pPartLabel->Init(pBomPart);
	}
	DWORD nPartNum=pRegion->hashBomParts.GetNodeNum();
	for(DWORD i=0;i<partLabelArr.Size();i++)
	{
		if(partLabelArr[i].m_pPart==NULL)
			continue;
		PART_LABEL *pPrevPart=i>0?&partLabelArr[(i-1)]:NULL;
		PART_LABEL *pNextPart=i<nPartNum-1?&partLabelArr[i+1]:NULL;
		bool bTubeStdPartPart=false;
		if(strlen(partLabelArr[i].m_pPart->sSpec)>0&&!partLabelArr[i].m_pPart->bStdPart)
		{
			if(pPrevPart&&pPrevPart->m_pPart&&pPrevPart->m_pPart->arrItemWarningLevel[0]==0)
			{	
				if(partLabelArr[i].segI!=pPrevPart->segI)
					partLabelArr[i].m_pPart->arrItemWarningLevel[0]=2;
				else if(partLabelArr[i].serialNo.Suffix().Length<=0&&pPrevPart->serialNo.Suffix().Length<=0&&
					partLabelArr[i].serialNo!=pPrevPart->serialNo+1)
					partLabelArr[i].m_pPart->arrItemWarningLevel[0]=2;				
			}
			if(pNextPart&&pNextPart->m_pPart&&pNextPart->m_pPart->arrItemWarningLevel[0]==0)
			{	
				if(partLabelArr[i].segI!=pNextPart->segI)
					partLabelArr[i].m_pPart->arrItemWarningLevel[0]=2;
				else if(partLabelArr[i].serialNo.Suffix().Length<=0&&pNextPart->serialNo.Suffix().Length<=0&&
					partLabelArr[i].serialNo!=pNextPart->serialNo+1)
					partLabelArr[i].m_pPart->arrItemWarningLevel[0]=2;
			}
		}
	}
}
CLogFile bomlog("D:\\summbom.txt");
#ifdef _DEBUG
int CImageDataRegion::Recognize(char modeOfAuto0BomPart1Summ2/*=0*/,int iCellRowStart/*=-1*/,int iCellRowEnd/*=-1*/,int iCellCol/*=-1*/)	//-1表示全部,>=0表示指定行或列 wjh-2018.3.14
#else
int CImageDataRegion::Recognize(char modeOfAuto0BomPart1Summ2/*=0*/)
#endif
{
	hashBomParts.Empty();
	hashImgShieldLayer.Empty();
#ifdef _TIMER_COUNT
	timer.Start();
#endif
	//1、提取数据区域的行数和列数
	//行
	ATOM_LIST<int>	levelStartList;
	RecognizeLines(levelStartList,TRUE);
	if(levelStartList.GetNodeNum()<=0)
	{
		logerr.Log("分行失败");
		return false;
	}
#ifdef _TIMER_COUNT
	timer.Relay(1);
#endif
	//列
	ATOM_LIST<int> verticalStartList;
	RecognizeLines(verticalStartList,FALSE);
	if(verticalStartList.GetNodeNum()<=0)
	{
		logerr.Log("分列失败");
		return false;
	}
#ifdef _TIMER_COUNT
	timer.Relay(2);
#endif
	m_arrColCellsWidth.SetSize(0,verticalStartList.GetNodeNum()-1);
	int* pPrevColSplitterX=verticalStartList.GetFirst();
	for(int* pColSplitterX=verticalStartList.GetNext();pColSplitterX;pColSplitterX=verticalStartList.GetNext())
	{
		m_arrColCellsWidth.append(*pColSplitterX-*pPrevColSplitterX);
		pPrevColSplitterX=pColSplitterX;
	}
#ifdef _TIMER_COUNT
	timer.Relay(3);
#endif
	//2、将数据区域分成若干个矩形区域
#ifdef _DEBUG
	EXPORT_CHAR_RECOGNITION=(iCellRowStart>=0&&iCellRowEnd-iCellRowStart>=0)||iCellCol>=0;
#endif
	CAlphabets *pAlphabets=(CAlphabets*)IMindRobot::GetAlphabetKnowledge();
	DYN_ARRAY<CELL_RECT> rowCells(verticalStartList.GetNodeNum()-1);
	/*if(levelStartList.GetNodeNum()>0&&pAlphabets->IsAutoSelectFontLib())
	{	//去第一行进行识别测试，自动选择当前字库
		int iOldSerial=pAlphabets->GetActiveFontSerial();
		int iMinSerial=0,nMinWarning=10000;
		int nTestRowCount=min(10,levelStartList.GetNodeNum()-1);
		for(int i=0;i<10;i++)
		{
			if(!pAlphabets->ImportFontsFile(i))
				break;
			pAlphabets->SetActiveFontSerial(i);
			hashBomParts.Empty();
			for(int j=0;j<nTestRowCount-1;j++)
			{
				RECT rect;
				rect.top=levelStartList[j];
				rect.bottom=levelStartList[j+1];
				for(int k=0;k<verticalStartList.GetNodeNum()-1;k++)
				{
					rect.left=verticalStartList[k];
					rect.right=verticalStartList[k+1];
					rowCells[k]=rect;
				}
				RecognizeSingleBomPart(rowCells,TRUE,NULL);
			}
			CheckPartLabel(this);
			int nWarning=0;
			for(BOM_PART *pBomPart=hashBomParts.GetFirst();pBomPart;pBomPart=hashBomParts.GetNext())
			{
				for(int i=0;i<7;i++)
					nWarning+=pBomPart->arrItemWarningLevel[i];
			}
			if(nWarning<=nMinWarning)
			{
				nMinWarning=nWarning;
				iMinSerial=i;
				if(nMinWarning==0)
					break;
			}
		}
		if(iMinSerial!=iOldSerial)
		{
			pAlphabets->ImportFontsFile(iMinSerial);
			pAlphabets->SetActiveFontSerial(iMinSerial);
		}
		else
		{
			pAlphabets->ImportFontsFile(iOldSerial);
			pAlphabets->SetActiveFontSerial(iOldSerial);
		}
	}*/
	if(modeOfAuto0BomPart1Summ2==0)	//材料汇总表模式
		modeOfAuto0BomPart1Summ2=rowCells.Size()<=7?1:2;
	hashBomParts.Empty();
	CLogErrorLife life(&bomlog);
	for(int i=0;i<levelStartList.GetNodeNum()-1;i++)	//
	{
#ifdef _DEBUG
		if(iCellRowStart>=0&&iCellRowEnd-iCellRowStart>=0&&(i<iCellRowStart||i>iCellRowEnd))
			continue;
#endif
		RECT rect;
		rect.top=levelStartList[i];
		rect.bottom=levelStartList[i+1];
		int rightest=0;
		for(int j=0;j<verticalStartList.GetNodeNum()-1;j++)
		{
			rect.left=verticalStartList[j];
			rect.right=rightest=verticalStartList[j+1];
			rowCells[j].rect=rect;
		}
		if(modeOfAuto0BomPart1Summ2!=2)
		{	//3、识别每行构件明细区域的内容
#ifdef _DEBUG
			RecognizeSingleBomPart(rowCells,FALSE,NULL,iCellCol,EXPORT_CHAR_RECOGNITION);
#else
			RecognizeSingleBomPart(rowCells,FALSE,NULL);
#endif
		}
		else	//识别材料汇总表内容
			RecognizeSummaryBOM(rowCells);
	}
#ifdef _TIMER_COUNT
	CLogErrorLife logtimer(&logerr);
	for(DWORD* pdwTickTime=timer.hashTicks.GetFirst();pdwTickTime;pdwTickTime=timer.hashTicks.GetNext())
		logerr.Log("Id=%3d, cost %d ms",timer.hashTicks.GetCursorKey(),*pdwTickTime);
#endif
	logerr.ShowToScreen();
	return modeOfAuto0BomPart1Summ2;
}

static BOOL ParseSpec(char* spec_str,int* thick,int* width,int* cls_id,int *sub_type=NULL,bool bPartLabelStr=false)
{
	if(spec_str==NULL||strlen(spec_str)<=0)
		return FALSE;
	CXhChar50 sSpec(spec_str);
	char* validstr=NULL;
	CXhChar16 material;
	int inter_width=0,inter_thick=0,inter_cls_id=0,inter_sub_type=0;
	char first_char=spec_str[0];
	char sec_char=strlen(spec_str)>1?spec_str[1]:' ';
	char third_char=strlen(spec_str)>2?spec_str[2]:' ';
	if(bPartLabelStr)
	{
		if( strstr(spec_str,"-")==NULL&&
			(first_char=='F'||first_char=='X'||first_char=='C'||first_char=='X'||first_char=='U'))
		{
			inter_cls_id=IRecoginizer::BOMPART::PLATE;
			if(first_char=='F')
			{
				inter_sub_type=IRecoginizer::BOMPART::SUB_TYPE_PLATE_FL;
				if(sec_char=='D')
					inter_sub_type=IRecoginizer::BOMPART::SUB_TYPE_PLATE_FLD;
				else if(sec_char=='P')
					inter_sub_type=IRecoginizer::BOMPART::SUB_TYPE_PLATE_FLP;
				else if(sec_char=='G')
					inter_sub_type=IRecoginizer::BOMPART::SUB_TYPE_PLATE_FLG;
				else if(sec_char=='R')
					inter_sub_type=IRecoginizer::BOMPART::SUB_TYPE_PLATE_FLR;
			}
			else if(first_char=='X')
				inter_sub_type=IRecoginizer::BOMPART::SUB_TYPE_PLATE_X;
			else if(first_char=='C')
				inter_sub_type=IRecoginizer::BOMPART::SUB_TYPE_PLATE_C;
			else if(first_char=='U')
				inter_sub_type=IRecoginizer::BOMPART::SUB_TYPE_PLATE_U;
		}
		else if( (first_char=='G'&&sec_char=='J'&&third_char=='-')||	//GJ-50 钢绞线
			(first_char=='N'&&sec_char=='U'&&third_char=='T')||	//UT型线夹
			(first_char=='U'&&sec_char=='-')||						//U型挂环
			(first_char=='J'&&sec_char=='K'&&third_char=='-')||	//钢线夹子
			(first_char=='N'&&sec_char=='X'&&third_char=='-')||	//楔形线夹
			first_char=='T')
			inter_cls_id=IRecoginizer::BOMPART::ACCESSORY;
	}
	else if(((validstr=strstr(spec_str,"L"))!=NULL)||(validstr=strstr(spec_str,"∠"))!=NULL)
	{
		int materialstr_len=validstr-spec_str;
		material.NCopy(spec_str,materialstr_len);
		memmove((char*)sSpec,validstr,sSpec.Length-materialstr_len+1);
		inter_cls_id=IRecoginizer::BOMPART::ANGLE;	//角钢
		sSpec.Replace("L","");
		sSpec.Replace("∠","");
		sSpec.Replace("×"," ");
		sSpec.Replace("X"," ");
		sSpec.Replace("x"," ");
		sSpec.Replace("*"," ");
		sSpec.Replace("?"," ");	//调试状态下识别错误时系统默认待处理字符
		validstr++;
		sscanf(sSpec,"%d%d",&inter_width,&inter_thick);
	}
	else if((validstr=strstr(spec_str,"-"))!=NULL&&(validstr==spec_str||(validstr-spec_str)==4))
	{
		inter_cls_id=IRecoginizer::BOMPART::PLATE;	//钢板
		memmove((char*)sSpec,validstr+1,sSpec.Length);
		if(strstr(sSpec,"×")||strstr(sSpec,"x")||strstr(sSpec,"*")||strstr(sSpec,"X"))
		{
			sSpec.Replace("×"," ");
			sSpec.Replace("X"," ");
			sSpec.Replace("x"," ");
			sSpec.Replace("*"," ");
			sSpec.Replace("?"," ");	//调试状态下识别错误时系统默认待处理字符
			sscanf(sSpec,"%d%d",&inter_thick,&inter_width);
		}
		else
			inter_thick=atoi(sSpec);
	}
	else if(((validstr=strstr(spec_str,"φ"))!=NULL)||(validstr=strstr(spec_str,"Φ"))!=NULL)
	{
		int hits=sSpec.Replace("φ"," ");
		hits+=sSpec.Replace("Φ"," ");
		sSpec.Replace("/"," ");
		sSpec.Replace("\\"," ");
		inter_cls_id=IRecoginizer::BOMPART::TUBE;	//钢管
		if(hits==2)
		{
			inter_cls_id=IRecoginizer::BOMPART::SUB_TYPE_TUBE_WIRE;	//套管
			sscanf(sSpec,"%d%d",&inter_thick,&inter_width);
		}
		else
		{
			int materialstr_len=validstr-spec_str;
			material.NCopy(spec_str,materialstr_len);
			sSpec.Copy(validstr);
			sSpec.Replace("φ"," ");
			sSpec.Replace("Φ"," ");
			int nXCount=sSpec.Replace("×"," ");
			nXCount+=sSpec.Replace("X"," ");
			nXCount+=sSpec.Replace("x"," ");
			nXCount+=sSpec.Replace("*"," ");
			sSpec.Replace("?"," ");	//调试状态下识别错误时系统默认待处理字符
			sscanf(sSpec,"%d%d",&inter_width,&inter_thick);
			if(nXCount<=0)	//圆钢
				inter_cls_id=IRecoginizer::BOMPART::ROUND;
		}
	}
	else if(((validstr=strstr(spec_str,"["))!=NULL)||(strlen(spec_str)==2&&spec_str[0]=='C'))
	{	//槽钢
		inter_cls_id=IRecoginizer::BOMPART::SLOT;
	}
	else
		return FALSE;
	if(width!=NULL)
		*width=inter_width;
	if(thick!=NULL)
		*thick=inter_thick;
	if(cls_id!=NULL)
		*cls_id=inter_cls_id;
	if(sub_type!=NULL)
		*sub_type=inter_sub_type;
	return TRUE;
}

static void CorrectBomPartProp(BOM_PART *pBomPart,BOM_PART *pPrevPart)
{
	//1.修正件号
	if(pPrevPart&&pBomPart->sPartNo.Length==pPrevPart->sPartNo.Length)
	{	//修正将B错误识别为8的件号
		int nLen=pBomPart->sPartNo.Length;
		if(pBomPart->sPartNo[nLen-1]=='8'&&pPrevPart->sPartNo[nLen-1]=='A')
			pBomPart->sPartNo[nLen-1]='B';
	}
	if(pPrevPart&&pPrevPart->arrItemWarningLevel[0]==0&&pBomPart->arrItemWarningLevel[0]>0)
	{	//根据前一个构件件号修订当前构件件号
		SEGI segI1,segI2;
		CXhChar16 serial1,serial2;
		char separator1=0,separator2=0;
		if( ParsePartNo(pPrevPart->sPartNo,&segI1,serial1,NULL,&separator1)&&
			ParsePartNo(pBomPart->sPartNo,&segI2,serial2,NULL,&separator2))
		{
			int nSerial1=atoi(serial1);
			int nSerial2=atoi(serial2);
			serial2.Printf("%d",nSerial2);
			if(nSerial2<10)
				serial2.InsertBefore('0');
			if(segI1!=segI2)
			{
				if(separator1>0)
					pBomPart->sPartNo.Printf("%s%c%s",(char*)segI1.ToString(),separator1,(char*)serial2);
				else
					pBomPart->sPartNo.Printf("%s%s",(char*)segI1.ToString(),(char*)serial2);
			}
			else if(serial2!=(serial1+1))
			{
				if(separator1>0)
					pBomPart->sPartNo.Printf("%s%c%s",(char*)segI1.ToString(),separator1,(char*)serial2);
				else
					pBomPart->sPartNo.Printf("%s%s",(char*)segI1.ToString(),(char*)serial2);
			}
		}
	}
	else if(pPrevPart&&pPrevPart->arrItemWarningLevel[0]>0&&pBomPart->arrItemWarningLevel[0]==0)
	{	//根据当前件号修订前一个件号
		SEGI segI1,segI2;
		CXhChar16 serial1,serial2;
		char separator1=0,separator2=0;
		if( ParsePartNo(pPrevPart->sPartNo,&segI1,serial1,NULL,&separator1)&&
			ParsePartNo(pBomPart->sPartNo,&segI2,serial2,NULL,&separator2))
		{
			int nSerial1=atoi(serial1);
			int nSerial2=atoi(serial2);
			if(segI1!=segI2)
			{
				if(separator2>0)
					pPrevPart->sPartNo.Printf("%s%c%s",(char*)segI2.ToString(),separator2,(char*)serial1);
				else
					pPrevPart->sPartNo.Printf("%s%s",(char*)segI2.ToString(),(char*)serial1);
			}
			else if(nSerial2!=nSerial1+1)
			{
				nSerial1=nSerial2-1;
				serial1.Printf("%d",nSerial1);
				if(nSerial1<10)
					serial1.InsertBefore('0');
				if(separator2>0)
					pPrevPart->sPartNo.Printf("%s%c%s",(char*)segI2.ToString(),separator2,(char*)serial1);
				else
					pPrevPart->sPartNo.Printf("%s%s",(char*)segI2.ToString(),(char*)serial1);
			}
		}
	}
	//2.修正规格
	if(pBomPart->sSpec.EqualNoCase("C5"))
		pBomPart->sSpec.Copy("[5");	//统一将C5修订为[5 wht 18-07-28
	if(pBomPart->arrItemWarningLevel[2]>0)
	{
		int thick=0,width=0,cls_id=0;
		if(ParseSpec(pBomPart->sSpec,&thick,&width,&cls_id))
		{
			if((cls_id==IRecoginizer::BOMPART::TUBE||cls_id==IRecoginizer::BOMPART::ANGLE)&&width<thick)
				pBomPart->sSpec.Printf("-%dX%d",width,thick);
		}
	}
}
int CImageDataRegion::RecognizeSummaryBOM(DYN_ARRAY<CELL_RECT> &rowCells,BYTE *pWarningLevel/*=NULL*/)
{	
	//3、识别每个区域的内容
	CELL_RECT rect;
	CXhChar50 sText;
	CXhChar50 sSpec,sWeight,sSumW;
	int idBomPartClsType=0;
	BYTE arrWarningLevelBytes[7]={0};
	for(WORD k=0;k<rowCells.Size();k++)
	{
		rect=rowCells[k];
		BYTE* pcbWarningByte=k<1?&arrWarningLevelBytes[k]:&arrWarningLevelBytes[min(6,k+1)];
		//UINT idCellTexttype=CImageCellRegion::SUM_W;//(k==0)?CImageCellRegion::GUIGE:CImageCellRegion::SUM_W;
		UINT idCellTexttype=(k==0)?CImageCellRegion::GUIGE:CImageCellRegion::SUM_W;
		sText=RecognizeDatas(rect,idCellTexttype,NULL,pcbWarningByte);
		if(idCellTexttype==CImageCellRegion::GUIGE)	//材质|规格
		{
			if(sText.Length>0&&sText[0]=='['&&(strchr(sText,'X')>0||strchr(sText,'x')>0))
			{	//L错误识别成了[，在此处修正	wht 18-06-28
				sText[0]='L';
			}
			char* pszSizeNumber=NULL,*pszThick=NULL,*pszPlateFlag=NULL;
			if(pszSizeNumber=strchr(sText,'L'))
			{
				idBomPartClsType=BOM_PART::ANGLE;
				pszThick=strchr(pszSizeNumber,'X');
				if(pszThick==NULL)
					pszThick=strchr(pszSizeNumber,'x');
				if(pszThick==NULL)
				{	//未能识别出x,根据规格表预测
					int nLen=strlen(pszSizeNumber+1);
					char *pszSeparator=NULL;
					if(nLen==4)	//4位,L40X4
						pszSeparator=pszSizeNumber+1+2;
					else if(nLen==5)
					{	//五个字符，厚度可能为1位或2位，首字母大于'3'时肢宽小于100
						if(*(pszSizeNumber+1)>'3')
							pszSeparator=pszSizeNumber+1+2;
						else
							pszSeparator=pszSizeNumber+1+3;
					}
					else if(nLen>=6)
						pszSeparator=pszSizeNumber+1+3;
					if(pszSeparator!=NULL)
					{
						*pszSeparator='X';
						pszThick=pszSeparator;
					}
				}
			}
			else if(pszPlateFlag=strchr(sText,'-'))
			{
				idBomPartClsType=BOM_PART::PLATE;
				pszThick=strchr(pszPlateFlag,'X');
				if(pszThick==NULL)
					pszThick=strchr(pszPlateFlag,'x');
				if(pszThick==NULL)
				{	//未能识别出x,根据规格表预测
					int nLen=strlen(pszPlateFlag+1);
					char *pszSeparator=NULL;
					if(nLen==4)	//4位,-6X100
						pszSeparator=pszPlateFlag+1+1;
					else if(nLen==5)
					{	//五个字符，厚度可能为1位或2位，首字母小于'3'时厚度为2位
						if(*(pszPlateFlag+1)<'3')
							pszSeparator=pszPlateFlag+1+2;
						else
							pszSeparator=pszPlateFlag+1+1;
					}
					else if(nLen>=6)
						pszSeparator=pszPlateFlag+1+2;
					if(pszSeparator!=NULL)
					{
						*pszSeparator='X';
						pszThick=pszSeparator;
					}
				}
			}
			else if((pszSizeNumber=strstr(sText,"Φ"))!=NULL||(pszSizeNumber=strstr(sText,"φ"))!=NULL)
				idBomPartClsType=BOM_PART::TUBE;

			if(sText.Remove('?')>0||(pszSizeNumber&&pszThick))
			{
				int sizenumber=0;
				if(pszSizeNumber!=NULL&&pszThick!=NULL)
				{
					char cSpliter=*pszThick;
					*pszThick=0;
					int wide = atoi(pszSizeNumber+1);
					int thick= atoi(pszThick+1);
					*pszThick=cSpliter;
					if(!IsClassicAngleSize(wide*100+thick))
						*pcbWarningByte+=WARNING_LEVEL::ABNORMAL_CHAR_EXIST;
					else
						int b=3;
				}
				else if(pszSizeNumber==NULL||pszThick==NULL)
					*pcbWarningByte=WARNING_LEVEL::ABNORMAL_CHAR_EXIST;
			}
			ParseDataValue(sText,CImageCellRegion::GUIGE,sSpec);
			sSpec=sText;
			/////////////////////////////////////////////////////////
			//启用规格规则智能识别 wjh-2018.4.8
			int nSizeSpecLength=sSpec.GetLength();
			while(nSizeSpecLength>0&&sSpec[nSizeSpecLength-1]=='?')
			{
				sSpec[nSizeSpecLength-1]=0;
				nSizeSpecLength--;
			}
			sText=sSpec;
		}
		else if(idCellTexttype==CImageCellRegion::SUM_W)	//总重
		{
			ParseDataValue(sText,CImageCellRegion::SUM_W,sSumW);
			sText=sSumW;
		}
		sText.ResizeLength(6,' ',true);
		if(k>0)
			sText.InsertBefore(',');
		bomlog.LogString(sText,false);
	}
	bomlog.LogString("\n",false);
	int nSumWarningLevel=0;
	//if(pWarningLevel)
	//	memcpy(pWarningLevel,arrWarningLevelBytes,7);
	//for(int i=0;i<7;i++)
	//	nSumWarningLevel+=arrWarningLevelBytes[i];
	return nSumWarningLevel;
}
int CImageDataRegion::StatColLinePixeCount(int start,int end,int iCol)
{
	int nPixelCount=0;
	for(int i=start;i<end;i++)
	{
		if(IsBlackPixel(iCol,i))
			nPixelCount=robot.Shell_AddLong(nPixelCount,1);//++;
		if(IsBlackPixel(iCol+1,i))
			nPixelCount=robot.Shell_AddLong(nPixelCount,1);//++;
		if(IsBlackPixel(iCol-1,i))
			nPixelCount=robot.Shell_AddLong(nPixelCount,1);//++;
	}
	return nPixelCount;
}
#ifdef _DEBUG
int CImageDataRegion::RecognizeSingleBomPart(DYN_ARRAY<CELL_RECT> &rowCells,BOOL bTestRecog/*=FALSE*/,BYTE *pWarningLevel/*=NULL*/,
											  int iCellCol/*=-1*/,bool bExportLogFile/*=false*/)
#else
int CImageDataRegion::RecognizeSingleBomPart(DYN_ARRAY<CELL_RECT> &rowCells,BOOL bTestRecog/*=FALSE*/,BYTE *pWarningLevel/*=NULL*/)
#endif
{	
	//TODO:按定死比例去判断是否有列合并，存在隐患，但目前还不急迫,应参照CFormatTable处理单元格合并 wjh-2018.8.22
	//检查第1、2、3列是否合并，前四列宽度比例为：10、25、15、10
	const BYTE MERGE_COL1n2   = 2;
	const BYTE MERGE_COL1n2n3 = 3;
	BYTE biMergeLabelCount=0;
	if(rowCells.Size()>2)
	{
		double col_width1=rowCells[0].right-rowCells[0].left;//verticalStartList[1]-verticalStartList[0];
		double col_width2=rowCells[1].right-rowCells[1].left;//verticalStartList[2]-verticalStartList[1];
		//未合并时前两列正常比例大概为：2.5=25/10
		//前两列合并后，前两列比例应大概为：0.428=15/35
		//前三列合并后，前两列比例应大概为：0.2=10/50
		double scale=col_width2/col_width1;
		if(scale<0.3)
			biMergeLabelCount=MERGE_COL1n2n3;//3;
		else if(scale<0.5)
			biMergeLabelCount=MERGE_COL1n2;//2;
	}
	//修订单元格列分割线
	const int DETECTION_WIDTH=8;	//在分割线左右侧查找合适的分割线
	CELL_RECT *pPrevRect=NULL;
	//最后一个单元格，同时修订左右侧分割线，其余单元格修订左侧分割线(即前一单元格的右侧分割线)
	for(UINT i=0;i<rowCells.Size()+1;i++)
	{	
		bool bLastCell=(i==rowCells.Size());
		int iCellIndex=bLastCell?i-1:i;
		int iCurCol=bLastCell?rowCells[iCellIndex].right:rowCells[iCellIndex].left;
		int start=rowCells[iCellIndex].top,end=rowCells[iCellIndex].bottom;
		const int MIN_PIXE_COUNT=end-start;
		//1.判断当前分割线是否需要修订
		int nCount=StatColLinePixeCount(start,end,iCurCol);
		if(nCount<MIN_PIXE_COUNT)
		{	//2.在当前分割线左右侧探测新的分割线
			int iMaxColIndex=iCurCol;
			int nMaxPixelCount=nCount;
			for(int j=max(0,iCurCol-DETECTION_WIDTH);j<min(m_nWidth,iCurCol+DETECTION_WIDTH);j++)
			{
				nCount=StatColLinePixeCount(start,end,j);
				if(nCount>nMaxPixelCount)
				{
					iMaxColIndex=j;
					nMaxPixelCount=nCount;
				}
			}
			//3.修正分割线
			if(nMaxPixelCount>MIN_PIXE_COUNT)
			{
				if(bLastCell)
					rowCells[iCellIndex].right=iMaxColIndex;
				else
				{
					rowCells[iCellIndex].left=iMaxColIndex;
					if(pPrevRect)
						pPrevRect->rect.right=rowCells[iCellIndex].left;
				}
			}
		}
		pPrevRect=&rowCells[i];
	}
	//3、识别每个区域的内容
	static const BYTE PART_GENERAL			= 0;	//常规构件（角钢、钢板、钢管、扁钢、槽钢、圆钢）
	static const BYTE PART_STANDARD			= 1;	//标准构件（槽型插板、十字插板、U型插板、对焊法兰、平焊法兰、刚性法兰、柔性法兰）
	static const BYTE PART_FITTINGS			= 2;	//贴附件（如钢筋或钢绞线等无须件号的线缆型材料铁附件）
	static const BYTE PART_STEEL_GRATING	= 3;	//钢格栅(G235/400/100WFG)
	int idBomPartClsType=0;
	int cls_id=0;
	CXhChar50 sText;
	CXhChar50 sPartNo,sMaterial,sSpec,sLen,sNum,sWeight,sSumW,sNote;
#ifdef _TIMER_COUNT
	DWORD start4Tick=timer.Relay();
#endif
	CELL_RECT rect;
	BYTE arrWarningLevelBytes[7]={0};
	for(WORD k=0;k<6&&k<rowCells.Size();k++)
	{
		rect=rowCells[k];
#ifdef _TIMER_COUNT
		DWORD start40Tick=timer.Relay();
#endif
		/*WORD iCurCol=k;
		if(biMergeLabelCount>0)
		{	//前两列合并时为钢管塔标准件，不在此处识别第一列,长度列不识别 wht 18-06-11
			if(iCurCol==0||(biMergeLabelCount==2&&iCurCol==1))
				continue;
			else
				iCurCol=k+(biMergeLabelCount-1);
		}*/
		//if(bTestRecog&&!(k==0||k==1))
		//	continue;	//测试识别度时只识别件号与规格列 wht 18-07-14
		BYTE* pcbWarningByte=k<1?&arrWarningLevelBytes[k]:&arrWarningLevelBytes[min(6,k+1)];
		UINT idCellTexttype=k+1;
#ifdef _DEBUG
		if(iCellCol>=0&&iCellCol!=k)
			continue;
		sText=RecognizeDatas(rect,idCellTexttype,NULL,pcbWarningByte,bExportLogFile||iCellCol>=0);
#else
		sText=RecognizeDatas(rect,idCellTexttype,NULL,pcbWarningByte);
#endif
#ifdef _TIMER_COUNT
		timer.Relay(40,start40Tick);
#endif
		if(idCellTexttype==CImageCellRegion::PARTNO)		//件号
		{
			ParseDataValue(sText,CImageCellRegion::PARTNO,sPartNo);
		}
		else if(idCellTexttype==CImageCellRegion::GUIGE)	//材质|规格
		{
			if(sText.Length>0&&sText[0]=='['&&(strchr(sText,'X')>0||strchr(sText,'x')>0))
			{	//L错误识别成了[，在此处修正	wht 18-06-28
				sText[0]='L';
			}
			char* pszSizeNumber=NULL,*pszThick=NULL,*pszPlateFlag=NULL;
			if(pszSizeNumber=strchr(sText,'L'))
			{
				cls_id=BOM_PART::ANGLE;
				pszThick=strchr(pszSizeNumber,'X');
				if(pszThick==NULL)
					pszThick=strchr(pszSizeNumber,'x');
				if(pszThick==NULL)
				{	//未能识别出x,根据规格表预测
					int nLen=strlen(pszSizeNumber+1);
					char *pszSeparator=NULL;
					if(nLen==4)	//4位,L40X4
						pszSeparator=pszSizeNumber+1+2;
					else if(nLen==5)
					{	//五个字符，厚度可能为1位或2位，首字母大于'3'时肢宽小于100
						if(*(pszSizeNumber+1)>'3')
							pszSeparator=pszSizeNumber+1+2;
						else
							pszSeparator=pszSizeNumber+1+3;
					}
					else if(nLen>=6)
						pszSeparator=pszSizeNumber+1+3;
					if(pszSeparator!=NULL)
					{
						*pszSeparator='X';
						pszThick=pszSeparator;
					}
				}
			}
			else if(pszPlateFlag=strchr(sText,'-'))
			{
				cls_id=BOM_PART::PLATE;
				pszThick=strchr(pszPlateFlag,'X');
				if(pszThick==NULL)
					pszThick=strchr(pszPlateFlag,'x');
				if(pszThick==NULL)
				{	//未能识别出x,根据规格表预测
					int nLen=strlen(pszPlateFlag+1);
					char *pszSeparator=NULL;
					if(nLen==4)	//4位,-6X100
						pszSeparator=pszPlateFlag+1+1;
					else if(nLen==5)
					{	//五个字符，厚度可能为1位或2位，首字母小于'3'时厚度为2位
						if(*(pszPlateFlag+1)<'3')
							pszSeparator=pszPlateFlag+1+2;
						else
							pszSeparator=pszPlateFlag+1+1;
					}
					else if(nLen>=6)
						pszSeparator=pszPlateFlag+1+2;
					if(pszSeparator!=NULL)
					{
						*pszSeparator='X';
						pszThick=pszSeparator;
					}
				}
			}
			else if((pszSizeNumber=strstr(sText,"Φ"))!=NULL||(pszSizeNumber=strstr(sText,"φ"))!=NULL)
				cls_id=BOM_PART::TUBE;

			if(sText.Remove('?')>0||(pszSizeNumber&&pszThick))
			{
				int sizenumber=0;
				if(pszSizeNumber!=NULL&&pszThick!=NULL)
				{
					char cSpliter=*pszThick;
					*pszThick=0;
					int wide = atoi(pszSizeNumber+1);
					int thick= atoi(pszThick+1);
					*pszThick=cSpliter;
					if(!IsClassicAngleSize(wide*100+thick))
						*pcbWarningByte+=WARNING_LEVEL::ABNORMAL_CHAR_EXIST;
					else
						int b=3;
				}
				else if(pszSizeNumber==NULL||pszThick==NULL)
					*pcbWarningByte=WARNING_LEVEL::ABNORMAL_CHAR_EXIST;
			}
			ParseDataValue(sText,CImageCellRegion::GUIGE,sSpec,sMaterial);
			//sSpec=sText;
		}
		else if(idCellTexttype==CImageCellRegion::LENGTH)	//长度
		{
			if(sText.Remove('?')>0)
				*pcbWarningByte=WARNING_LEVEL::ABNORMAL_CHAR_EXIST;
			sLen=sText;//ParseDataValue(sText,CImageCellRegion::LENGHT,sLen);
		}
		else if(idCellTexttype==CImageCellRegion::NUM)	//数量
		{
			ParseDataValue(sText,CImageCellRegion::NUM,sNum);
			/** 目前数量列已将非数字字符屏蔽，暂不需要数字字符验证
			char* iterchar=(char*)sText;
			while(iterchar!=0)
			{
				if(*iterchar<'0'||*iterchar>'9')
				{
					*pcbWarningByte=2;
					break;
				}
				iterchar++;
			}
			*/
		}
		else if(idCellTexttype==CImageCellRegion::PIECE_W)	//单重
			ParseDataValue(sText,CImageCellRegion::PIECE_W,sWeight);
		else if(idCellTexttype==CImageCellRegion::SUM_W)	//总重
			ParseDataValue(sText,CImageCellRegion::SUM_W,sSumW);
		else if(idCellTexttype==CImageCellRegion::NOTE)	//备注
			ParseDataValue(sText,CImageCellRegion::NOTE,sNote);	
#ifdef _TIMER_COUNT
		timer.Relay(41);
#endif
	}
	if(biMergeLabelCount>0)
		idBomPartClsType=PART_STANDARD;//biStdPartType=1;		//1.如钢管塔中槽型插板标准件
	else
	{
		if(sPartNo.Length==0&&sSpec.Length>0&&sLen.Length==0)
			idBomPartClsType=PART_STANDARD;	//1.如钢管塔中槽型插板标准件
		else if(sPartNo.Length==0)
			idBomPartClsType=PART_FITTINGS;	//2.如钢筋或钢绞线等无须件号的线缆型材料铁附件
	}
	if(sPartNo.Length==0&&idBomPartClsType==0)
		sPartNo.Copy(sSpec);
	else if(idBomPartClsType>0)
	{
		if(idBomPartClsType==PART_STANDARD)
			cls_id=BOM_PART::PLATE;
		else if(idBomPartClsType==PART_FITTINGS)
			cls_id=BOM_PART::FITTINGS;
#ifdef _DEBUG
		sText=RecognizeDatas(rowCells[biMergeLabelCount>0?0:1],CImageCellRegion::GUIGE,NULL,&arrWarningLevelBytes[0],false,idBomPartClsType);
#else
		sText=RecognizeDatas(rowCells[biMergeLabelCount>0?0:1],CImageCellRegion::GUIGE,NULL,&arrWarningLevelBytes[0],idBomPartClsType);
#endif
		sPartNo.Copy(sText);
	}
	//if(!bTestRecog)
	{
		/////////////////////////////////////////////////////////
		//#ifdef _DEBUG
		sPartNo.Remove('?');
		//CXhChar16 szPrevLabel;
		//启用件号规则智能识别 wjh-2018.4.4
		/*char* pDoubtChar=NULL,*pPrevFinalChar=(char*)szPrevLabel,*pFinalChar=NULL;
		if(i>0&&(pDoubtChar=strchr(sPartNo,'?'))!=NULL&&szPrevLabel[0]!=0)
		{
			int nPrevLabelLength=szPrevLabel.Length();
			int nCurrLabelLength=sPartNo.GetLength();
			if(nPrevLabelLength>0&&!ISDIGIT(szPrevLabel[nPrevLabelLength-1]))
				nPrevLabelLength--;	//此时编号长度是去掉后缀的字符长度
			if(sPartNo[nCurrLabelLength-1]=='?')
			{
				sPartNo[nCurrLabelLength-1]=0;
				nCurrLabelLength--;
			}
			if(nCurrLabelLength>0&&!ISDIGIT(sPartNo[nCurrLabelLength-1]))
				nCurrLabelLength--;	//此时编号长度是去掉后缀的字符长度
			if(nPrevLabelLength<nCurrLabelLength)
				sPartNo.Remove('?');	//当前编号的'?'是由于噪点产生的冗余字符，应移除
			else if(szPrevLabel[0]==sPartNo[0])
			{	//说明前后编号的编号类型一致（排除FL***类编号与普通编号的相邻情况）
				int pos=pDoubtChar-(char*)sPartNo;
				char* pPrevMatchChar=pos+(char*)szPrevLabel;
				if(pos<nCurrLabelLength-1&&*(pDoubtChar+1)>'0')
					*pDoubtChar=*pPrevMatchChar;	//非末尾字符，且后续字符非'0'pDoubtChar不存在进位
				else if(pos>=nPrevLabelLength
				else if(*pPrevMatchChar=='9')
					*pDoubtChar='0';
				else //if(*pPrevMatchChar!='9')
					*pDoubtChar=*pPrevMatchChar+1;
			}
		}
		szPrevLabel=sPartNo;	//记录上一构件编号
		*/
		//启用规格规则智能识别 wjh-2018.4.8
		int nSizeSpecLength=sSpec.GetLength();
		while(nSizeSpecLength>0&&sSpec[nSizeSpecLength-1]=='?')
		{
			sSpec[nSizeSpecLength-1]=0;
			nSizeSpecLength--;
		}
		//启用长度规则智能识别 wjh-2018.4.8
		sLen.Remove('?');
		//#endif
		/////////////////////////////////////////////////////////
		//初始化哈希表
		BOM_PART* pPrevPart=hashBomParts.GetTail();
		BOM_PART* pBomPart=hashBomParts.Add(0);
		//构件矩形区域高度取行高值,方便外部绘制行区域 wht 19-01-18
		rect.top=rowCells[0].top;
		rect.bottom=rowCells[0].bottom;
		pBomPart->rc.top=rect.top;
		pBomPart->rc.bottom=rect.bottom;
		pBomPart->rc.left=rowCells[0].left;
		pBomPart->rc.right=rowCells[rowCells.Size()-1].right;
		pBomPart->cPartType=cls_id;
		pBomPart->AddPart(atoi(sNum));
		memcpy(pBomPart->arrItemWarningLevel,arrWarningLevelBytes,7);
		pBomPart->sPartNo.Copy(sPartNo);
		if(idBomPartClsType>0)
		{
			if(sLen.Length>0)
				pBomPart->length=atoi(sLen);
			else
			{
				pBomPart->length=0;
				pBomPart->arrItemWarningLevel[3]=0;	//长度
			}
			pBomPart->arrItemWarningLevel[2]=0;	//规格
		}
		else
		{
			pBomPart->sSpec.Copy(sSpec);
			pBomPart->cMaterial=QuerySteelBriefMatMark(sMaterial);
			pBomPart->length=atoi(sLen);
		}
		if(sWeight.GetLength()>0)
			pBomPart->fPieceWeight=atof(sWeight);
		if(sSumW.GetLength()>0)
			pBomPart->fSumWeight=atof(sSumW);
		float fWidth=0,fThick=0;
		RestoreSpec(sSpec,&fWidth,&fThick);
		pBomPart->wide=fWidth;
		pBomPart->thick=fThick;
		pBomPart->feature1=rect.top;
		pBomPart->feature2=rect.bottom;
		//修正明显不正确的构件属性
		CorrectBomPartProp(pBomPart,pPrevPart);
	}
	if(pWarningLevel)
		memcpy(pWarningLevel,arrWarningLevelBytes,7);
	int nSumWarningLevel=0;
	for(int i=0;i<7;i++)
		nSumWarningLevel+=arrWarningLevelBytes[i];
	return nSumWarningLevel;
}

bool CImageDataRegion::RetrieveCharImageData(int iCellRow,int iCellCol,IMAGE_CHARSET_DATA* imagedata)
{
	hashImgShieldLayer.Empty();
	//1、提取数据区域的行数和列数
	ATOM_LIST<int>	levelStartList,verticalStartList;
	if(RecognizeLines(levelStartList,TRUE)<=0)	//拆分行
	{
		logerr.Log("分行失败");
		return false;
	}
	if(RecognizeLines(verticalStartList,FALSE)<=0)//拆分列
	{
		logerr.Log("分列失败");
		return false;
	}
	//2、将数据区域分成若干个矩形区域
	if(iCellRow<0||iCellRow>=levelStartList.GetNodeNum()||iCellCol<0||iCellCol>=verticalStartList.GetNodeNum())
		return false;
	RECT rcCurrCell;
	rcCurrCell.top=levelStartList[iCellRow];
	rcCurrCell.bottom=levelStartList[iCellRow+1];
	rcCurrCell.left=verticalStartList[iCellCol];
	rcCurrCell.right=verticalStartList[iCellCol+1];
	//3、识别每个区域的内容
	CXhChar50 sText=RecognizeDatas(CELL_RECT(rcCurrCell),iCellCol+1,imagedata);
	return true;
}

void CImageDataRegion::ToBuffer(CBuffer &buffer)
{
	buffer.WriteInteger(leftUp.x);
	buffer.WriteInteger(leftUp.y);
	buffer.WriteInteger(leftDown.x);
	buffer.WriteInteger(leftDown.y);
	buffer.WriteInteger(rightUp.x);
	buffer.WriteInteger(rightUp.y);
	buffer.WriteInteger(rightDown.x);
	buffer.WriteInteger(rightDown.y);
	//图片
	BUFFERPOP stack(&buffer,hashBomParts.GetNodeNum());
	buffer.WriteInteger(hashBomParts.GetNodeNum());
	for(BOM_PART *pPart=hashBomParts.GetFirst();pPart;pPart=hashBomParts.GetNext())
	{
		buffer.WriteInteger(hashBomParts.GetCursorKey());
		buffer.WriteByte(pPart->cPartType);
		//保存构件信息
		buffer.WriteInteger(pPart->GetPartNum());
		buffer.WriteString(pPart->sPartNo);
		buffer.WriteInteger(pPart->iSeg);
		buffer.WriteByte(pPart->cMaterial);
		buffer.WriteDouble(pPart->wide);
		buffer.WriteDouble(pPart->thick);
		buffer.WriteDouble(pPart->wingWideY);
		buffer.WriteString(pPart->sSpec);
		buffer.WriteDouble(pPart->length);
		buffer.WriteDouble(pPart->fPieceWeight);
		buffer.WriteString(pPart->sNotes);
		buffer.WriteInteger(pPart->feature1);	//构件对应图片行顶部坐标
		buffer.WriteInteger(pPart->feature2);	//构件对应图片行底部坐标
		stack.Increment();
	}
	if(!stack.VerifyAndRestore())
#ifdef  AFX_TARG_ENU_ENGLISH
		logerr.Log("The number record of image part is wrong!");
#else 
		logerr.Log("图片识别构件数量错误!");
#endif
	buffer.WriteInteger(m_arrColCellsWidth.GetSize());
	buffer.Write(m_arrColCellsWidth.m_pData,sizeof(int)*m_arrColCellsWidth.GetSize());
}
void CImageDataRegion::FromBuffer(CBuffer &buffer,long version)
{
	buffer.ReadInteger(&leftUp.x);
	buffer.ReadInteger(&leftUp.y);
	buffer.ReadInteger(&leftDown.x);
	buffer.ReadInteger(&leftDown.y);
	buffer.ReadInteger(&rightUp.x);
	buffer.ReadInteger(&rightUp.y);
	buffer.ReadInteger(&rightDown.x);
	buffer.ReadInteger(&rightDown.y);
	//图片
	hashBomParts.Empty();
	int n=buffer.ReadInteger();
	for(int i=0;i<n;i++)
	{
		DWORD dwKey=buffer.ReadInteger();
		BYTE cPartType=0;
		buffer.ReadByte(&cPartType);
		BOM_PART *pPart=hashBomParts.Add(dwKey);
		pPart->AddPart(buffer.ReadInteger());
		buffer.ReadString(pPart->sPartNo);
		buffer.ReadInteger(&pPart->iSeg);
		buffer.ReadByte(&pPart->cMaterial);
		buffer.ReadDouble(&pPart->wide);
		buffer.ReadDouble(&pPart->thick);
		buffer.ReadDouble(&pPart->wingWideY);
		buffer.ReadString(pPart->sSpec);
		buffer.ReadDouble(&pPart->length);
		buffer.ReadDouble(&pPart->fPieceWeight);
		buffer.ReadString(pPart->sNotes);
		buffer.ReadInteger(&pPart->feature1);	//构件对应图片行顶部坐标
		buffer.ReadInteger(&pPart->feature2);	//构件对应图片行底部坐标
	}
	int nColSize=0;
	buffer.ReadInteger(&nColSize);
	m_arrColCellsWidth.SetSize(nColSize);
	buffer.Read(m_arrColCellsWidth.m_pData,sizeof(int)*nColSize);
}
//////////////////////////////////////////////////////////////////////////
//CImageDataFile
CImageDataFile::CImageDataFile(DWORD key/*=0*/)
{
	m_idSerial=key;
	m_nOffsetX=m_nOffsetY=0;
	m_fDisplayAllRatio=0;
	m_fDisplayRatio=1;
	m_ciRawImageFileType=0;
	m_nMonoForwardPixels=20;
	m_bLowBackgroundNoise=m_bThinFontText=false;
	m_nTurnCount = 0;
}
CImageDataFile::~CImageDataFile()
{
	
}
//balancecoef 划分黑白点间的界限调整平衡系数,取值-0.5~0.5;0时对应最高频率像素前移20位
//划分黑白点间的界限调整值(从区域最高频率像素的前移位数)
double CImageDataFile::SetMonoThresholdBalanceCoef(double balancecoef/*=0*/,bool bUpdateMonoImages/*=false*/)
{	//保证balancecoef= 0  时balancecoef= 20，
	//    balancecoef=-1.0时balancecoef=  0，
	//    balancecoef= 1.0时balancecoef=100，
	if( balancecoef<-1.0)
		balancecoef=-1.0;
	if( balancecoef>1.0)
		balancecoef=1.0;
	int nOldMonoForwardPixels=m_nMonoForwardPixels;
	m_nMonoForwardPixels=f2i(CalFunc(balancecoef));
	if(bUpdateMonoImages&&nOldMonoForwardPixels!=m_nMonoForwardPixels)
		UpdateImageRegions();
	return balancecoef;
}
double CImageDataFile::GetMonoThresholdBalanceCoef()
{
	return CalMonoThresholdBalanceCoef(m_nMonoForwardPixels);
}
static const double MONO_BALANCE_NUMBERS[101]={   -1.000175,
-0.942635,-0.888785,-0.827365,-0.762566,-0.695685,-0.628495,-0.562766,-0.499864,-0.440604,-0.385302,
-0.329113,-0.283392,-0.240357,-0.199844,-0.161679,-0.125687,-0.091690,-0.056604,-0.028302, 0.000000,
 0.028302, 0.056604, 0.079377, 0.103703, 0.127142, 0.149788, 0.171727, 0.193036, 0.213783, 0.232096,
 0.251182, 0.269723, 0.287755, 0.305310, 0.322418, 0.339105, 0.355398, 0.371318, 0.386889, 0.402129,
 0.417058, 0.431693, 0.446051, 0.460146, 0.473995, 0.487610, 0.501005, 0.514192, 0.525747, 0.538294,
 0.550633, 0.562774, 0.574724, 0.586490, 0.598078, 0.609496, 0.620749, 0.631843, 0.642783, 0.653574,
 0.664222, 0.674731, 0.685105, 0.695350, 0.705468, 0.715464, 0.725342, 0.735106, 0.744758, 0.754302,
 0.763742, 0.773080, 0.782320, 0.791464, 0.800515, 0.809476, 0.818350, 0.827139, 0.835846, 0.844472,
 0.853021, 0.861494, 0.869894, 0.877376, 0.885545, 0.893641, 0.901664, 0.909615, 0.917498, 0.925312,
 0.933060, 0.940742, 0.948361, 0.955917, 0.963412, 0.970846, 0.978222, 0.985540, 0.992801, 1.000000
};
double CImageDataFile::CalMonoThresholdBalanceCoef(int forward/*=20*/)
{
	if(forward<0)
		forward=0;
	else if(forward>100)
		forward=100;
	return MONO_BALANCE_NUMBERS[forward];
}
void CImageDataFile::InitImageShowPara(RECT showRect)
{
	int new_width=showRect.right-showRect.left;
	int new_height=showRect.bottom-showRect.top;
	int old_width=m_rcShow.right-showRect.left;
	int old_height=m_rcShow.bottom-m_rcShow.top;
	if(m_fDisplayAllRatio==0||new_width!=old_width||new_height!=old_height)
	{
		m_rcShow=showRect;
		int image_width=image.GetWidth(),image_height=image.GetHeight();
		double scale_h=(double)(showRect.right-showRect.left)/image_width;
		double scale_v=(double)(showRect.bottom-showRect.top)/image_height;
		m_nOffsetX=m_nOffsetY=0;
		m_fDisplayRatio=m_fDisplayAllRatio=min(1,scale_h>scale_v?scale_v:scale_h);
		//m_fDisplayRatio=min(1,scale_h>scale_v?scale_h:scale_v);
	}
}
bool CImageDataFile::RetrievePdfRegionImageBits(CImageDataRegion *pRegion,CImageTransform &regionImage)
{
	if(m_ciRawImageFileType!=RAW_IMAGE_PDF)
		return false;
	//1.计算字体放大到模板字高的缩放比例
	double zoom_scale=CalPdfRegionZoomScaleBySample(pRegion);
	//2.按计算比例获取待提取区域
	POINT leftUp=pRegion->TopLeft(),rightDown=pRegion->BtmRight();
	RectD rect=RectD::FromXY(leftUp.x,leftUp.y,rightDown.x,rightDown.y);
	rect=RectD::FromPoint(pRegion->TopLeft(),pRegion->TopRight(),pRegion->BtmLeft(),pRegion->BtmRight());
	if(image.RetrievePdfRegion(&rect,zoom_scale,regionImage))
	{
		RECT rc=rect.ToRECT();
		POINT leftU,leftD,rightU,rightD;
		leftU.x=leftD.x=rc.left;
		rightU.x=rightD.x=rc.right;
		leftU.y=rightU.y=rc.top;
		leftD.y=rightD.y=rc.bottom;
		pRegion->SetRegion(leftU,leftD,rightD,rightU,true,false);
		return true;
	}
	else
		return false;
}

double CImageDataFile::CalPdfRegionZoomScaleBySample(CImageDataRegion *pRegion)
{	
	if(m_ciRawImageFileType!=RAW_IMAGE_PDF)
		return 0;
	PDF_FILE_CONFIG pdfCfg=image.GetPdfConfig();
	double zoom_scale=pdfCfg.zoom_scale;
	//1.根据框选范围获取取样单元格
	CImageDataRegion sampleRegion;
	const int SAMPLE_WIDTH=350;
	const int SAMPLE_HEIGHT=80;
	POINT leftUp=pRegion->TopLeft(),leftDown,rightUp,rightDown;
	RectI rect(leftUp.x,leftUp.y,SAMPLE_WIDTH,SAMPLE_HEIGHT);
	PointI br=rect.BR();
	rightDown.x=br.x;
	rightDown.y=br.y;
	leftDown.x=leftUp.x; 
	leftDown.y=rightDown.y;
	rightUp.x=rightDown.x;
	rightUp.y=leftUp.y;
	sampleRegion.SetRegion(leftUp,leftDown,rightDown,rightUp,false,false);
	RetrieveRegionImageBits(&sampleRegion,false);
	//2.分析取样单元格，计算当前缩放比例下的字体高度
	RECT rcSampleCell=RECT_C();
	int nTextHeight,nTemplateH;
	if(sampleRegion.RecognizeSampleCell(NULL,&rcSampleCell))
	{	//3.根据模板字高计算最佳缩放比例
		CAlphabets *pAlphabets=(CAlphabets*)IMindRobot::GetAlphabetKnowledge();
		nTemplateH=pAlphabets->GetTemplateH();	//40
		nTextHeight=(rcSampleCell.bottom-rcSampleCell.top+1);
	}
	else
	{
		CAlphabets *pAlphabets=(CAlphabets*)IMindRobot::GetAlphabetKnowledge();
		nTemplateH=pAlphabets->GetTemplateH();	//40
		if(rcSampleCell.bottom-rcSampleCell.top>0)
		{
			nTextHeight=(int)((rcSampleCell.bottom-rcSampleCell.top+1)*0.65+0.5);
			logerr.LevelLog(CLogFile::WARNING_LEVEL1_IMPORTANT,"样本检测区未找到单元格文本信息!");
		}
		else
		{	//==0可能是还未导入图片信息
			nTextHeight=nTemplateH;
			if(image.GetWidth()>0&&image.GetHeight()>0)
				logerr.LevelLog(CLogFile::WARNING_LEVEL1_IMPORTANT,"样本检测区未找到表格信息!");
		}
	}
	if(nTemplateH>0&&nTextHeight>0)
	{
		zoom_scale=(1.0*nTemplateH)/nTextHeight;
		zoom_scale*=pdfCfg.zoom_scale;
		//zoom_scale*=0.8;	//字体字高取模板字高0.8倍,海涛也忘了当时为什么缩0.8了 wjh-2018.8.31
	}
	else
		zoom_scale=min(zoom_scale,3);
	return min(20,zoom_scale);	//最大支持放大8倍
}
//
bool CImageDataFile::RetrieveRegionImageBits(CImageDataRegion *pRegion,bool bSnapPdfLiveRegionImg/*=true*/)
{
	POINT leftDown=pRegion->BtmLeft(),leftUp=pRegion->TopLeft(),rightUp=pRegion->TopRight(),rightDown=pRegion->BtmRight();
	if(pRegion==NULL||(rightUp.x-leftUp.x<=0)||abs(leftDown.y-leftUp.y)==0)
	{
		logerr.Log("没有选中数据区域");
		return false;
	}
	bool bRetCode=false;
	bool loadfrom_virtualmemory=false;
	CImageTransform regionImage;
	CImageTransform *pImage=&image;
	if( bSnapPdfLiveRegionImg&&m_ciRawImageFileType==RAW_IMAGE_PDF&&
		RetrievePdfRegionImageBits(pRegion,regionImage))
	{
		pImage=&regionImage;
		PointD regionLeftTop=pRegion->TopLeft(true);
		PointD leftTop(leftUp),leftBtm(leftDown),rightTop(rightUp),rightBtm(rightDown);
		image.TransPdfRegionCornerPoint(regionImage.GetPdfConfig().zoom_scale,leftTop,leftBtm,rightTop,rightBtm);
		Sub_2dPnt(leftTop,leftTop,regionLeftTop);
		Sub_2dPnt(leftBtm,leftBtm,regionLeftTop);
		Sub_2dPnt(rightTop,rightTop,regionLeftTop);
		Sub_2dPnt(rightBtm,rightBtm,regionLeftTop);
		//
		Cpy_2dPnt(leftUp,leftTop.ToInt());
		Cpy_2dPnt(leftDown,leftBtm.ToInt());
		Cpy_2dPnt(rightUp,rightTop.ToInt());
		Cpy_2dPnt(rightDown,rightBtm.ToInt());
	}
	else
	{
		if(image.GetGrayBits()==NULL)
			LoadGreyBytes(&loadfrom_virtualmemory);
		if(image.GetGrayBits()==NULL)
			return false;
	}
	pImage->LocateDrawingRgnRect(leftUp,leftDown,rightUp,rightDown);
	int xiTopLeft=min(leftUp.x,leftDown.x);
	int yjTopLeft=min(leftUp.y,rightUp.y);
	int nRgnWidth=max(rightUp.x,rightDown.x)-xiTopLeft+1;
	int nRgnHeight=max(leftDown.y,rightDown.y)-yjTopLeft+1;
	if(m_ciRawImageFileType!=RAW_IMAGE_PDF)
	{	//通过前几行预测明细表的行高及各列宽度
		//预测网格线及网格锚点
		//PreTestScanRegionGridLine(pRegion);
		//根据行高精细化背景单色分隔域值
		int FINE_GRID_WIDTH=100,FINE_GRID_HEIGHT=50;
		pImage->LocalFiningMonoPixelThreshold(xiTopLeft,yjTopLeft,nRgnWidth,nRgnHeight,FINE_GRID_WIDTH,FINE_GRID_HEIGHT);
		loadfrom_virtualmemory=false;
		//进行网格锚点的正交化精细矫正
	}

	pRegion->m_nWidth=pImage->GetDrawingWidth()+1;
	pRegion->m_nHeight=pImage->GetDrawingHeight()+1;
	pRegion->m_nWidth%16>0?pRegion->m_nEffWidth=(pRegion->m_nWidth/16)*2+2:pRegion->m_nEffWidth=pRegion->m_nWidth/8;
	if(pRegion->m_lpBitMap)
		delete []pRegion->m_lpBitMap;
	pRegion->m_lpBitMap=new BYTE[pRegion->m_nEffWidth*pRegion->m_nHeight];
	memset(pRegion->m_lpBitMap,0,pRegion->m_nEffWidth*pRegion->m_nHeight);
	POINT p;
	for(int i=0;i<pRegion->m_nWidth;i++)
	{
		for(int j=0;j<pRegion->m_nHeight;j++)
		{
			p.x=i;
			//p.y=pRegion->m_nHeight-1-j;
			p.y=j;	//已从图像读取时统一为Downward行扫描形式
			//采用区域判断以避免原图较大时，一些黑点特征的丢失
			RECT rc=pImage->TransformRectToOriginalField(p);
			bool bBlack=false;
#ifdef _DEBUG
			bool bLTPixelState=false;
#endif
			for(int m=rc.left;!bBlack&&m<rc.right;m++)
			{
				for(int n=rc.top;!bBlack&&n<rc.bottom;n++)
				{
#ifndef _DEBUG
					bBlack|=pImage->IsBlackPixel(m,n,pRegion->MonoForwardPixels-m_nMonoForwardPixels);
#else
					bool black=pImage->IsBlackPixel(m,n,pRegion->MonoForwardPixels-m_nMonoForwardPixels);
					bBlack|=black;
					if(m==rc.left&&n==rc.top)
						bLTPixelState=black;
#endif
				}
			}
#ifdef _DEBUG
			if(bLTPixelState!=bBlack)
				int b=3;
#endif
			if(!bBlack)
			{
				int iByte=i/8,iBit=i%8;
				pRegion->m_lpBitMap[j*pRegion->m_nEffWidth+iByte]|=BIT2BYTE[7-iBit];
			}
		}
	}
	this->UnloadGreyBytes(!loadfrom_virtualmemory);
	return true;
}
bool CImageDataFile::RetrieveRectImageBits(const RECT& rcRegion,IMAGE_DATA* pImgData,TRANSFORM* pTransform/*=NULL*/)
{
	pImgData->nWidth=rcRegion.right-rcRegion.left;
	pImgData->nHeight=rcRegion.bottom-rcRegion.top;
	pImgData->nEffWidth=pImgData->nWidth%16>0?(pImgData->nWidth/16)*2+2:pImgData->nWidth/8;
	pImgData->biBitCount=1;
	if(pImgData->imagedata)
		memset(pImgData->imagedata,0,pImgData->nEffWidth*pImgData->nHeight);
	POINT p;
	int monobalance=pTransform?pTransform->m_iMonoBalance:0;
	for(int i=rcRegion.left;i<rcRegion.right;i++)
	{
		for(int j=rcRegion.top;j<rcRegion.bottom;j++)
		{
			p.x=i;
			//p.y=pRegion->m_nHeight-1-j;
			p.y=j;	//已从图像读取时统一为Downward行扫描形式
			//采用区域判断以避免原图较大时，一些黑点特征的丢失
			RECT rc=image.TransformRectToOriginalField(p,pTransform);
			bool bBlack=false;
#ifdef _DEBUG
			bool bLTPixelState=false;
#endif
			for(int m=rc.left;!bBlack&&m<rc.right;m++)
			{
				for(int n=rc.top;!bBlack&&n<rc.bottom;n++)
				{
#ifndef _DEBUG
					bBlack|=image.IsBlackPixel(m,n,monobalance);
#else
					bool black=image.IsBlackPixel(m,n,monobalance);
					bBlack|=black;
					if(m==rc.left&&n==rc.top)
						bLTPixelState=black;
#endif
				}
			}
			if(!bBlack&&pImgData->imagedata)
			{
				int iByte=i/8,iBit=i%8;
				pImgData->imagedata[j*pImgData->nEffWidth+iByte]|=BIT2BYTE[7-iBit];
			}
		}
	}
	return true;
}
////////////////////////////////////////////
//VVVV智能投影以获取当前明细表图像的偏转角度
CImageDataFile::PRJSTAT_INFO::PRJSTAT_INFO()
{
	ciStatType=STAT_TL;
	dfImgSharpness=0.0;
	dfPriorityOfRow=3.0;
	dfPriorityOfCol=1.0;
	uiMaxBlckHoriPixels=yjHoriMaxBlckLine=0;
	uiMaxBlckVertPixels=xiVertMaxBlckLine=0;
}
bool CImageDataFile::PRJSTAT_INFO::RecognizePeakBlcks(WORD* xarrBlcks,int size,STAT_BLCKS* pStatBlcks,int iMidPosition,int nMaxTroughBlcks,BYTE cnMaxPeakHalfWidth/*=5*/)
{
	BLCKPIXEL_WAVEINFO xarrblcks(xarrBlcks,size,-1,nMaxTroughBlcks);
	return RecognizePeakBlcks(xarrblcks,pStatBlcks,iMidPosition,cnMaxPeakHalfWidth);
}
bool CImageDataFile::PRJSTAT_INFO::RecognizePeakBlcks(BLCKPIXEL_WAVEINFO& xarrblcks,STAT_BLCKS* pStatBlcks,int iMidPosition,BYTE cnMaxPeakHalfWidth/*=5*/)
{
	WORD* xarrBlcks=xarrblcks.blcks;
	int size=xarrblcks.size;
	int i,SCANPIXELS=2,indexOfPeakBlcks=iMidPosition,nPeakBlcks=xarrBlcks[iMidPosition];
	bool hasPeakPoint=false,hasLeftThrough=false,hasRightThrough=false;
	int MAX_BLCKS=0;
	if(xarrblcks.nMaxBlcks>0)
		MAX_BLCKS=xarrblcks.nMaxBlcks;
	else
	{
		for(i=0;i<size;i++)
			MAX_BLCKS=max(MAX_BLCKS,xarrBlcks[i]);
	}
	int nMinPeakBlcks=f2i(0.9*MAX_BLCKS);	//根据最多黑色像素数来计算有效峰值可以避免一半为空白的区域图像 wjh-2018.9.11
	int nMaxTroughBlcks=max(f2i(nMinPeakBlcks*0.2),xarrblcks.nMaxTroughBlcks);
	for(i=iMidPosition-cnMaxPeakHalfWidth;i<=iMidPosition+cnMaxPeakHalfWidth;i++)
	{
		if(i<0)
			i=0;
		if(xarrBlcks[i]>nPeakBlcks)
		{
			nPeakBlcks=xarrBlcks[i];
			indexOfPeakBlcks=i;
		}
	}
	if(pStatBlcks)
	{
		pStatBlcks->nPeakBlcks=nPeakBlcks;
		pStatBlcks->indexOfPeakBlcks=indexOfPeakBlcks;
	}
	hasPeakPoint=(nPeakBlcks>nMinPeakBlcks);
	if(!hasPeakPoint)
		return false;	//不存在黑点统计数波峰
	if(nPeakBlcks/2>nMaxTroughBlcks)//波峰到波谷，必须至少跌路峰值的一半
		nMaxTroughBlcks=nPeakBlcks/2;
	int MIN_SCAN_POS=max(0,indexOfPeakBlcks-cnMaxPeakHalfWidth-SCANPIXELS-1);
	int iLeftScanStart=max(0,indexOfPeakBlcks-1),iRightScanStart=min(size-1,indexOfPeakBlcks+1);
	for(i=iLeftScanStart;!hasLeftThrough&&i>=MIN_SCAN_POS;i--)
	{
		if(xarrBlcks[i]>nPeakBlcks)
		{	//保证nPeakBlcks一定为左右波谷间的最高波峰值
			nPeakBlcks=xarrBlcks[i];
			indexOfPeakBlcks=i;
		}
		hasLeftThrough=(xarrBlcks[i]<nMaxTroughBlcks);
	}
	if(pStatBlcks)
	{
		pStatBlcks->nPeakBlcks=nPeakBlcks;
		pStatBlcks->indexOfPeakBlcks=indexOfPeakBlcks;
	}
	if(i>0&&!hasLeftThrough)
		return false;	//不存在黑点统计数左侧波谷
	int MAX_SCAN_POS=min(size,indexOfPeakBlcks+cnMaxPeakHalfWidth+SCANPIXELS);
	for(i=iRightScanStart;!hasRightThrough&&i<MAX_SCAN_POS;i++)
	{
		if(xarrBlcks[i]>nPeakBlcks)
		{	//保证nPeakBlcks一定为左右波谷间的最高波峰值
			nPeakBlcks=xarrBlcks[i];
			indexOfPeakBlcks=i;
		}
		hasRightThrough=(xarrBlcks[i]<nMaxTroughBlcks);
	}
	if(pStatBlcks)
	{
		pStatBlcks->nPeakBlcks=nPeakBlcks;
		pStatBlcks->indexOfPeakBlcks=indexOfPeakBlcks;
	}
	if(i<size&&!hasRightThrough)
		return false;	//不存在黑点统计数右侧波谷
	return true;
}
bool CImageDataFile::PRJSTAT_INFO::StatLine(WORD* xarrBlcks,int size,int nMaxBlcks,char ciDirectionH0V1,bool inverse/*=false*/)
{
	int nPeakBlcks=0;
	for(int i=0;i<size;i++)
		nPeakBlcks=max(nPeakBlcks,xarrBlcks[i]);
	int THRESHOLD_EDGE=nPeakBlcks/4;
	STAT_BLCKS statblcks;
	bool recognized=false;
	BLCKPIXEL_WAVEINFO waveinfo(xarrBlcks,size,nMaxBlcks,THRESHOLD_EDGE);
	if(!inverse)
	{
		for(int i=0;i<size-1;i++)
		{	//已出现超出总尺度70%的峰值,且由波峰跌落峰值50%且不少于THRESHOLD_EDGE以下识别为存在一个波峰
			if(recognized=RecognizePeakBlcks(waveinfo,&statblcks,i,5))
				break;
		}
	}
	else
	{
		for(int i=size-1;i>=0;i--)
		{	//已出现超出总尺度70%的峰值,且由波峰跌落峰值50%且不少于THRESHOLD_EDGE以下识别为存在一个波峰
			if(recognized=RecognizePeakBlcks(waveinfo,&statblcks,i,5))
				break;
		}
	}
	double dfRewardSharpness=0;
	if(statblcks.nPeakBlcks>=nMaxBlcks-2)
	{
		for(i=statblcks.indexOfPeakBlcks-1;i>=max(0,statblcks.indexOfPeakBlcks-4);i--)
		{
			if(xarrBlcks[i]>=nMaxBlcks-2)
				dfRewardSharpness+=xarrBlcks[i]*0.1/nMaxBlcks;
		}
		for(i=statblcks.indexOfPeakBlcks+1;i<min(size,statblcks.indexOfPeakBlcks+5);i++)
		{
			if(xarrBlcks[i]>=nMaxBlcks-2)
				dfRewardSharpness+=xarrBlcks[i]*0.1/nMaxBlcks;
		}
	}
	if(nMaxBlcks>0)
		this->dfImgSharpness=dfRewardSharpness+statblcks.nPeakBlcks/nMaxBlcks;
	else
		dfImgSharpness=0;
	if(ciDirectionH0V1==0)
	{
		this->uiMaxBlckHoriPixels=statblcks.nPeakBlcks;
		this->yjHoriMaxBlckLine=statblcks.indexOfPeakBlcks;
	}
	else
	{
		this->uiMaxBlckVertPixels=statblcks.nPeakBlcks;
		this->xiVertMaxBlckLine=statblcks.indexOfPeakBlcks;
	}
	return recognized;
}
bool CImageDataFile::PRJSTAT_INFO::Stat(WORD* rowblcks,int width,WORD* colblcks,int height)
{
	bool recognized_h=false,recognized_v=false;
	STAT_BLCKS statblcks;
	uiMaxBlckHoriPixels=uiMaxBlckVertPixels=0;
	int i,validwidth=width,validheight=height,start=-1;
	//统计有效宽度
	for(i=0;i<width&&start<0;i++)
	{
		if(rowblcks[i]>0&&start<0)
			start=i;
	}
	for(i=width-1;i>=0;i--)
	{
		if(rowblcks[i]>0&&start>=0)
		{
			validwidth=i-start+1;
			break;
		}
	}
	//统计有效高度
	start=-1;
	for(i=0;i<height&&start<0;i++)
	{
		if(colblcks[i]>0&&start<0)
			start=i;
	}
	for(i=height-1;i>=0;i--)
	{
		if(colblcks[i]>0&&start>=0)
		{
			validheight=i-start+1;
			break;
		}
	}
	int THRESHOLD_EDGE=validheight/4;
	double dfVertRewardCoef=1.0,dfHoriRewardCoef=1.0;
	if(dfPriorityOfRow>0)
	{
		if(this->ciStatType==PRJSTAT_INFO::STAT_TL||ciStatType==PRJSTAT_INFO::STAT_BL)
		{
			for(i=0;i<width-1;i++)
			{
				if(recognized_v=RecognizePeakBlcks(rowblcks,width,&statblcks,i,THRESHOLD_EDGE,4))
					break;
			}
		}
		else
		{
			for(i=width-1;i>=0;i--)
			{
				if(recognized_v=RecognizePeakBlcks(rowblcks,width,&statblcks,i,THRESHOLD_EDGE,4))
					break;
			}
		}
		this->uiMaxBlckVertPixels=statblcks.nPeakBlcks;
		this->xiVertMaxBlckLine=statblcks.indexOfPeakBlcks;
		if((int)this->uiMaxBlckVertPixels>=height-2)
		{
			int hits=1;
			for(i=statblcks.indexOfPeakBlcks-1;i>=max(0,statblcks.indexOfPeakBlcks-4);i--)
			{
				if(rowblcks[i]>=height-2)
					hits++;
			}
			for(i=statblcks.indexOfPeakBlcks+1;i<min(width,statblcks.indexOfPeakBlcks+5);i++)
			{
				if(rowblcks[i]>=height-2)
					hits++;
			}
			dfVertRewardCoef=1+(hits-1)*0.1;
		}
	}
	THRESHOLD_EDGE=validwidth/4;
	if(dfPriorityOfCol>0)
	{
		if(this->ciStatType==PRJSTAT_INFO::STAT_TL||ciStatType==PRJSTAT_INFO::STAT_TR)
		{
			for(i=0;i<height-1;i++)
			{
				if(recognized_h=RecognizePeakBlcks(colblcks,height,&statblcks,i,THRESHOLD_EDGE,4))
					break;
			}
		}
		else
		{
			for(i=height-1;i>=0;i--)
			{
				if(recognized_h=RecognizePeakBlcks(colblcks,height,&statblcks,i,THRESHOLD_EDGE,4))
					break;
			}
		}
		this->uiMaxBlckHoriPixels=statblcks.nPeakBlcks;
		this->yjHoriMaxBlckLine=statblcks.indexOfPeakBlcks;
		if((int)this->uiMaxBlckVertPixels>=height-2)
		{
			int hits=1;
			for(i=statblcks.indexOfPeakBlcks-1;i>=max(0,statblcks.indexOfPeakBlcks-4);i--)
			{
				if(colblcks[i]>=width-2)
					hits++;
			}
			for(i=statblcks.indexOfPeakBlcks+1;i<min(height,statblcks.indexOfPeakBlcks+5);i++)
			{
				if(colblcks[i]>=width-2)
					hits++;
			}
			dfHoriRewardCoef=1+(hits-1)*0.1;
		}
	}
	if(recognized_h&&recognized_v)
	{
		UINT threshold_peak_h=validwidth/2,threshold_peak_v=validheight/2;
		double dfv=this->uiMaxBlckHoriPixels/(double)validwidth;
		double dfh=this->uiMaxBlckVertPixels/(double)validheight;
		dfv*=dfVertRewardCoef;
		dfh*=dfHoriRewardCoef;
		double factor_v=dfPriorityOfRow/(double)(dfPriorityOfRow+dfPriorityOfCol);
		double factor_h=dfPriorityOfCol/(double)(dfPriorityOfRow+dfPriorityOfCol);
		double base=dfv*factor_v+dfh*factor_h;
		double diff=fabs(dfv-dfh)/base;
		if(diff<0.3)
			dfImgSharpness=base;
		else
			dfImgSharpness=max(dfv,dfh);	//取纵向或横向中的较大的锐利度
		return true;
	}
	else if(recognized_h)
		dfImgSharpness=dfHoriRewardCoef*uiMaxBlckHoriPixels/(double)validwidth;
	else if(recognized_v)
		dfImgSharpness=dfVertRewardCoef*uiMaxBlckVertPixels/(double)validheight;
	else
	{
		double BASE_WIDTH=11.0;
		double dfv=this->uiMaxBlckHoriPixels/(validwidth*BASE_WIDTH);
		double dfh=this->uiMaxBlckVertPixels/(validheight*BASE_WIDTH);
		dfImgSharpness=max(dfv,dfh);
		return false;
	}
	return true;
}
bool CImageDataFile::ProjectRegionImage(const RECT& rcRgn,int yjRightTipRotPixel,PRJSTAT_INFO* pStatInfo/*=NULL*/,IMAGE_DATA* imgdata/*=NULL*/)
{
	double dfEdgeWidthX=rcRgn.right-rcRgn.left;
	return ProjectRegionImage(rcRgn,dfEdgeWidthX,yjRightTipRotPixel,pStatInfo,imgdata);
}
bool CImageDataFile::ProjectRegionImage(const RECT& rcRgn,double dfEdgeWidthX,double yjRightTipRotPixel,
						PRJSTAT_INFO* pStatInfo/*=NULL*/,IMAGE_DATA* imgdata/*=NULL*/)
{
	CBitImage imgrc;
	IMAGE_DATA img;
	img.biBitCount=1;
	img.nWidth=rcRgn.right-rcRgn.left;
	img.nHeight=rcRgn.bottom-rcRgn.top;
	img.nEffWidth=img.nWidth%16>0 ? (img.nWidth/16)*2+2 : img.nWidth/8;
	if(imgdata)
	{
		imgdata->biBitCount=img.biBitCount;
		imgdata->nWidth=img.nWidth;
		imgdata->nHeight=img.nHeight;
		imgdata->nEffWidth=img.nEffWidth;
		memset(imgdata->imagedata,0,imgdata->nEffWidth*imgdata->nHeight);
		imgrc.InitBitImage((BYTE*)imgdata->imagedata,imgdata->nWidth,imgdata->nHeight,imgdata->nEffWidth);
	}

	FUNC_ProjectRegionImage_BUFF* pBufObj=&_local_func_ProjectRegionImage_buff_object;
	DYN_ARRAY<WORD> rowblcks(img.nWidth,true,pBufObj->row_blck_pixels,800);
	DYN_ARRAY<WORD> colblcks(img.nHeight,true,pBufObj->col_blck_pixels,200);
	IMGROT rotation(rcRgn.left,rcRgn.top,dfEdgeWidthX,yjRightTipRotPixel);
	for(int xI=0;xI<img.nWidth;xI++)
	{
		for(int yJ=0;yJ<img.nHeight;yJ++)
		{
			POINT ptR=rotation.Rotate(xI+rcRgn.left,yJ+rcRgn.top);
			bool black=this->IsBlackPixel(ptR.x,ptR.y);
			if(imgdata)
				imgrc.SetPixelState(xI,yJ,black);
			if(black)
			{
				rowblcks[xI]++;
				colblcks[yJ]++;
			}
		}
	}
	int i;
	double xarrVals[2]={0,0};
	xarrVals[0]=(rowblcks[0]+rowblcks[1]*0.5)/1.5;
	int MAXDIFF=img.nWidth/2;
	for(i=1;i<img.nWidth-1;i++)
	{
		double basenumber=1;
		if(rowblcks[i]-rowblcks[i-1]>=MAXDIFF)
			xarrVals[1]=rowblcks[i];
		else
		{
			basenumber+=0.5;
			xarrVals[1]=rowblcks[i-1]*0.5+rowblcks[i];
		}
		if(rowblcks[i]-rowblcks[i+1]<MAXDIFF)
		{
			basenumber+=0.5;
			xarrVals[1]+=rowblcks[i+1]*0.5;
		}
		xarrVals[1]/=basenumber;
		rowblcks[i-1]=(int)(xarrVals[0]+0.5);
		xarrVals[0]=xarrVals[1];
	}
	if(rowblcks[img.nWidth-1]-rowblcks[img.nWidth-2]>=MAXDIFF)
		xarrVals[1]=rowblcks[img.nWidth-2];
	else
		xarrVals[1]=(rowblcks[img.nWidth-2]*0.5+rowblcks[img.nWidth-1])/1.5;
	rowblcks[img.nWidth-2]=(int)(xarrVals[0]+0.5);;
	rowblcks[img.nWidth-1]=(int)(xarrVals[1]+0.5);;

	xarrVals[0]=(colblcks[0]+colblcks[1])*0.5;
	for(i=1;i<img.nHeight-1;i++)
	{
		double basenumber=1;
		if(colblcks[i]-colblcks[i-1]>=MAXDIFF)
			xarrVals[1]=colblcks[i];
		else
		{
			basenumber+=0.5;
			xarrVals[1]=colblcks[i-1]*0.5+colblcks[i];
		}
		if(colblcks[i]-colblcks[i+1]<MAXDIFF)
		{
			basenumber+=0.5;
			xarrVals[1]+=colblcks[i+1]*0.5;
		}
		xarrVals[1]/=basenumber;
		colblcks[i-1]=(int)(xarrVals[0]+0.5);
		xarrVals[0]=xarrVals[1];
	}
	if(colblcks[img.nHeight-1]-colblcks[img.nHeight-2]>=MAXDIFF)
		xarrVals[1]=colblcks[img.nHeight-2];
	else
		xarrVals[1]=(colblcks[img.nHeight-2]*0.5+colblcks[img.nHeight-1])/1.5;
	colblcks[img.nHeight-2]=(int)(xarrVals[0]+0.5);
	colblcks[img.nHeight-1]=(int)(xarrVals[1]+0.5);
/*#ifdef _DEBUG
	CXhChar500 linetxt;
	CLogFile logimgh("c:/prjimgh.txt"),logimgv("c:/prjimgv.txt");
	int j;
	bool hasblcks=true;
	for(j=0;j<200&&hasblcks;j++)
	{
		linetxt.Empty();
		hasblcks=false;
		for(i=0;i<img.nWidth;i++)
		{
			hasblcks|=rowblcks[i]>j;
			if(rowblcks[i]>j)
				linetxt.Append('.');
			else
				linetxt.Append(' ');
		}
		logimgh.LogString(linetxt);
	}
	logimgh.ShowToScreen();

	hasblcks=true;
	for(j=0;j<200&&hasblcks;j++)
	{
		linetxt.Empty();
		hasblcks|=colblcks[j]>0;
		for(i=0;i<colblcks[j];i++)
			linetxt.Append('.');
		logimgv.LogString(linetxt);
	}
	logimgv.ShowToScreen();
#endif*/
	PRJSTAT_INFO stat;
	if(pStatInfo==NULL)
		pStatInfo=&stat;
	pStatInfo->dfPriorityOfRow=3.0;
	pStatInfo->dfPriorityOfCol=1.0;
	return pStatInfo->Stat(rowblcks,img.nWidth,colblcks,img.nHeight);
}
#include "MonoBmpFile.h"
bool CImageDataFile::IntelliRecogCornerPoint(const POINT& pick,POINT* pCornerPoint,
		char ciType_TL0_TR1_BR2_BL3/*=0*/,int *pyjHoriTipOffset/*=NULL*/,UINT uiTestWidth/*=300*/,UINT uiTestHeight/*=200*/)
{
	RECT rcRgn;
	if(ciType_TL0_TR1_BR2_BL3==PRJSTAT_INFO::STAT_TL)
	{
		rcRgn.left=pick.x;
		rcRgn.right=pick.x+uiTestWidth;
		rcRgn.top =pick.y;
		rcRgn.bottom=pick.y+uiTestHeight;
	}
	else if(ciType_TL0_TR1_BR2_BL3==PRJSTAT_INFO::STAT_TR)
	{
		rcRgn.right=pick.x+1;
		rcRgn.left=rcRgn.right-uiTestWidth;
		rcRgn.top =pick.y;
		rcRgn.bottom=pick.y+uiTestHeight;
	}
	else if(ciType_TL0_TR1_BR2_BL3==PRJSTAT_INFO::STAT_BR)
	{
		rcRgn.left=pick.x-uiTestWidth;
		rcRgn.right=pick.x;
		rcRgn.bottom=pick.y;
		rcRgn.top =rcRgn.bottom-uiTestHeight;
	}
	else if(ciType_TL0_TR1_BR2_BL3==PRJSTAT_INFO::STAT_BL)
	{
		rcRgn.left=pick.x;
		rcRgn.right=pick.x+uiTestWidth;
		rcRgn.bottom=pick.y;
		rcRgn.top =rcRgn.bottom-uiTestHeight;
	}
	else
		return false;
	bool loadfrom_virtualmemory=false;
	if(image.GetGrayBits()==NULL)
		LoadGreyBytes(&loadfrom_virtualmemory);
	if(image.GetGrayBits()==NULL)
		return false;
	//偏移探测恰当的正交投影方向
	int j,MaxTipOffset=uiTestWidth/2,ERROR_TOLERANCE=4;
	FUNC_ProjectRegionImage_BUFF* pBufObj=&_local_func_ProjectRegionImage_buff_object;
	int size=uiTestWidth*uiTestHeight;
	CHAR_ARRAY bytes(size,true,pBufObj->img_buff_bytes,pBufObj->img_buff_length);
	IMAGE_DATA* pPrjImg=NULL;
#ifdef _DEBUG
	IMAGE_DATA prjimg;
	prjimg.imagedata=bytes;
	pPrjImg=&prjimg;
#endif
	PRJSTAT_INFO stat,perfect;
	perfect.ciStatType=stat.ciStatType=(ciType_TL0_TR1_BR2_BL3<0||ciType_TL0_TR1_BR2_BL3>3)?CImageDataFile::PRJSTAT_INFO::STAT_TL:ciType_TL0_TR1_BR2_BL3;
	ProjectRegionImage(rcRgn,0,&perfect,pPrjImg);
#ifdef _DEBUG
	CXhChar50 szFileName="c:\\Recog\\prjimgY.bmp";
	WriteMonoBmpFile(szFileName,prjimg.nWidth,prjimg.nEffWidth,prjimg.nHeight,(BYTE*)prjimg.imagedata);
#endif
	PRJSTAT_INFO statPrevYz=perfect,statPrevYf=perfect;
	int maxsize=max(uiTestWidth,uiTestHeight);
	//逆时针旋转（Y+正向探测）
	int inverse_hits=0,yjPerfectTipOffset=0,hits=0;
	for(j=1;j<MaxTipOffset;j++)
	{
		ProjectRegionImage(rcRgn,maxsize,j,&stat,pPrjImg);
#ifdef _DEBUG
		szFileName.Printf("c:\\Recog\\prjimgY+%d.bmp",j);
		WriteMonoBmpFile(szFileName,prjimg.nWidth,prjimg.nEffWidth,prjimg.nHeight,(BYTE*)prjimg.imagedata);
#endif
		if(stat.dfImgSharpness>statPrevYz.dfImgSharpness)
		{
			if(stat.dfImgSharpness>perfect.dfImgSharpness)
			{
				perfect=stat;
				yjPerfectTipOffset=j;
				hits=0;
			}
			else
			{
				hits++;
				if(stat.dfImgSharpness>0.8)
					inverse_hits=0;
				else if(stat.dfImgSharpness<0.1)
					inverse_hits++;
			}
		}
		else if(inverse_hits<ERROR_TOLERANCE&&hits<ERROR_TOLERANCE)
			inverse_hits++;
		else
			break;
		statPrevYz=stat;
	}
	//顺时针旋转（Y-正向探测）
	inverse_hits=0,hits=0;
	for(j=1;j<MaxTipOffset;j++)
	{
		ProjectRegionImage(rcRgn,maxsize,-j,&stat,pPrjImg);
#ifdef _DEBUG
		szFileName.Printf("c:\\Recog\\prjimgY-%d.bmp",j);
		WriteMonoBmpFile(szFileName,prjimg.nWidth,prjimg.nEffWidth,prjimg.nHeight,(BYTE*)prjimg.imagedata);
#endif
		if(stat.dfImgSharpness>statPrevYf.dfImgSharpness)
		{
			if(stat.dfImgSharpness>perfect.dfImgSharpness)
			{
				perfect=stat;
				yjPerfectTipOffset=-j;
			}
			else
			{
				hits++;
				if(stat.dfImgSharpness>1)
					inverse_hits=0;
				else if(stat.dfImgSharpness<0.1)
					inverse_hits++;
			}
		}
		else if(inverse_hits<ERROR_TOLERANCE&&hits<ERROR_TOLERANCE)
			inverse_hits++;
		else
			break;
		statPrevYf=stat;
	}
	if(pyjHoriTipOffset)
		*pyjHoriTipOffset=yjPerfectTipOffset;
	if(pCornerPoint)
	{
		IMGROT rotation(rcRgn.left,rcRgn.top,uiTestWidth,yjPerfectTipOffset);
		POINT point=pick;
		if(perfect.uiMaxBlckHoriPixels>0)
			point.y=rcRgn.top+perfect.yjHoriMaxBlckLine;
		if(perfect.uiMaxBlckVertPixels>0)
			point.x=rcRgn.left+perfect.xiVertMaxBlckLine;
		point=rotation.Rotate(point.x,point.y);
		pCornerPoint->x=point.x;
		pCornerPoint->y=point.y;
	}
	this->UnloadGreyBytes(!loadfrom_virtualmemory);
	return true;
}
bool CImageDataFile::CalTextRotRevision(const RECT& rcRgn,const IMGROT& rotation,char ciDetectRule/*='X'*/,PRJSTAT_INFO* pStatInfo/*=NULL*/)
{
	UINT uiDetectWidth =rcRgn.right-rcRgn.left;
	UINT uiDetectHeight=rcRgn.bottom-rcRgn.top;

	int MAJOR_LENGTH=uiDetectWidth,MAJOR_DEPTH=uiDetectHeight;
	PRJSTAT_INFO stat;
	if(pStatInfo==NULL)
		pStatInfo=&stat;
	pStatInfo->ciStatType=PRJSTAT_INFO::STAT_TL;
	if(ciDetectRule=='Y'||ciDetectRule=='V')
	{
		ciDetectRule='Y';
		pStatInfo->dfPriorityOfRow=0;
		pStatInfo->dfPriorityOfCol=1.0;
		MAJOR_LENGTH=uiDetectHeight;
		MAJOR_DEPTH =uiDetectWidth;
	}
	else //if(ciDetectRule=='X'||ciDetectRule=='H')
	{
		ciDetectRule='X';
		pStatInfo->dfPriorityOfRow=1.0;
		pStatInfo->dfPriorityOfCol=0;
		MAJOR_LENGTH=uiDetectWidth;
		MAJOR_DEPTH =uiDetectHeight;
	}
	FUNC_ProjectRegionImage_BUFF* pBufObj=&_local_func_ProjectRegionImage_buff_object;
	DYN_ARRAY<WORD> lineblcks(MAJOR_LENGTH,true,pBufObj->row_blck_pixels,800);
	int blckStartI=-1,VALID_DEPTH=0;
	for(int xI=0;xI<(int)uiDetectWidth;xI++)
	{
		for(int yJ=0;yJ<(int)uiDetectHeight;yJ++)
		{
			POINT ptR=rotation.Rotate(xI+rcRgn.left,yJ+rcRgn.top);
			bool black=this->IsBlackPixel(ptR.x,ptR.y);
			if(black)
			{
				int index=ciDetectRule=='X'?xI:yJ;
				lineblcks[index]++;
				//统计黑点有效区间宽度
				int currI=ciDetectRule=='Y'?xI:yJ;
				if(blckStartI<0)
					blckStartI=currI;
				else
					VALID_DEPTH=currI-blckStartI+1;
			}
		}
	}
	bool successed=pStatInfo->StatLine(lineblcks,MAJOR_LENGTH,VALID_DEPTH,ciDetectRule=='Y'?0:1);
	if(ciDetectRule=='Y')
		pStatInfo->yjHoriMaxBlckLine+=rcRgn.top;
	else
		pStatInfo->xiVertMaxBlckLine+=rcRgn.left;
	return successed;
}
bool CImageDataFile::TestRotRevision(const POINT& xPickPoint,const POINT& xTopLeft2Pick,WORD wiDetectWidth,WORD wiDetectHeight,char ciDetectRule/*='X'*/,PRJSTAT_INFO* pStatInfo/*=NULL*/)
{
	RECT rcRgn;
	rcRgn.left=xPickPoint.x-xTopLeft2Pick.x;
	rcRgn.top=xPickPoint.y-xTopLeft2Pick.y;
	rcRgn.right=rcRgn.left+wiDetectWidth;
	rcRgn.bottom=rcRgn.top+wiDetectHeight;
	//偏移探测恰当的正交投影方向
	int j,MaxTipOffset=wiDetectWidth/2,ERROR_TOLERANCE=4;
	/*FUNC_ProjectRegionImage_BUFF* pBufObj=&_local_func_ProjectRegionImage_buff_object;
	int size=wiDetectWidth*wiDetectHeight;
	CHAR_ARRAY bytes(size,true,pBufObj->img_buff_bytes,pBufObj->img_buff_length);
	IMAGE_DATA* pPrjImg=NULL;
#ifdef _DEBUG
	IMAGE_DATA prjimg;
	prjimg.imagedata=bytes;
	pPrjImg=&prjimg;
#endif*/
	PRJSTAT_INFO stat,perfect;
	if(pStatInfo==NULL)
		pStatInfo=&perfect;
	int maxsize=max(wiDetectWidth,wiDetectHeight);
	IMGROT rotation(xPickPoint.x,xPickPoint.y,maxsize,0);
	CalTextRotRevision(rcRgn,rotation,ciDetectRule,pStatInfo);
#ifdef _DEBUG
	//CXhChar50 szFileName="c:\\Recog\\prjimgY.bmp";
	//WriteMonoBmpFile(szFileName,prjimg.nWidth,prjimg.nEffWidth,prjimg.nHeight,(BYTE*)prjimg.imagedata);
#endif
	PRJSTAT_INFO statPrevYz=perfect,statPrevYf=perfect;
	//逆时针旋转（Y+正向探测）
	int inverse_hits=0,yjPerfectTipOffset=0,hits=0;
	for(j=1;j<MaxTipOffset;j++)
	{
		rotation.Init(xPickPoint.x,xPickPoint.y,maxsize,j);
		CalTextRotRevision(rcRgn,rotation,ciDetectRule,&stat);
#ifdef _DEBUG
		//szFileName.Printf("c:\\Recog\\prjimgY+%d.bmp",j);
		//WriteMonoBmpFile(szFileName,prjimg.nWidth,prjimg.nEffWidth,prjimg.nHeight,(BYTE*)prjimg.imagedata);
#endif
		if(stat.dfImgSharpness>statPrevYz.dfImgSharpness)
		{
			if(stat.dfImgSharpness>pStatInfo->dfImgSharpness)
			{
				*pStatInfo=stat;
				yjPerfectTipOffset=j;
				hits=0;
			}
			else
			{
				hits++;
				if(stat.dfImgSharpness>0.8)
					inverse_hits=0;
				else if(stat.dfImgSharpness<0.1)
					inverse_hits++;
			}
		}
		else if(inverse_hits<ERROR_TOLERANCE&&hits<ERROR_TOLERANCE)
			inverse_hits++;
		else
			break;
		statPrevYz=stat;
	}
	//顺时针旋转（Y-正向探测）
	inverse_hits=0,hits=0;
	for(j=1;j<MaxTipOffset;j++)
	{
		rotation.Init(xPickPoint.x,xPickPoint.y,maxsize,-j);
		CalTextRotRevision(rcRgn,rotation,ciDetectRule,&stat);
#ifdef _DEBUG
		//szFileName.Printf("c:\\Recog\\prjimgY-%d.bmp",j);
		//WriteMonoBmpFile(szFileName,prjimg.nWidth,prjimg.nEffWidth,prjimg.nHeight,(BYTE*)prjimg.imagedata);
#endif
		if(stat.dfImgSharpness>=statPrevYf.dfImgSharpness)
		{
			if(stat.dfImgSharpness>pStatInfo->dfImgSharpness)
			{
				*pStatInfo=stat;
				yjPerfectTipOffset=-j;
			}
			else
			{
				hits++;
				if(stat.dfImgSharpness>1)
					inverse_hits=0;
				else if(stat.dfImgSharpness<0.1)
					inverse_hits++;
			}
		}
		else if(inverse_hits<ERROR_TOLERANCE&&hits<ERROR_TOLERANCE)
			inverse_hits++;
		else
			break;
		statPrevYf=stat;
	}
	return true;
}
bool CImageDataFile::PreTestScanRegionGridLine(CImageDataRegion *pRegion)
{
	PRJSTAT_INFO stat;
	int index=0;//,STEP_X=300,STEP_Y=100;
	//初步粗略分析行线
	POINT xTopLeft=pRegion->TopLeft(),xBtmRight=pRegion->BtmRight();
	POINT xPick=xTopLeft,xOffsetOrg2Pick;
	xOffsetOrg2Pick.x=0;
	xOffsetOrg2Pick.y=20;
	bool successed=false;
	WORD* pStatHits;
	PRESET_ARRAY128<WORD> xarrStatHeightHits;
	xarrStatHeightHits.ZeroPresetMemory();
	xarrStatHeightHits.ReSize(128);
	int nHoriLines;
	do{
		if(successed=TestRotRevision(xPick,xOffsetOrg2Pick,200,index==0?40:200,'Y',&stat))
		{	//找到行线
			pRegion->m_xarrBtmLinePosYOfRow.append(stat.yjHoriMaxBlckLine);
			xPick.y=stat.yjHoriMaxBlckLine+20;
			if((nHoriLines=(int)pRegion->m_xarrBtmLinePosYOfRow.Size())>1)
			{
				int niHeight=pRegion->m_xarrBtmLinePosYOfRow[nHoriLines-1]-pRegion->m_xarrBtmLinePosYOfRow[nHoriLines-2];
				if((pStatHits=xarrStatHeightHits.GetAt(niHeight))!=NULL)
					*pStatHits+=1;
				else
					xarrStatHeightHits.Set(niHeight,1,true);
			}
		}
		else
			xPick.y+=200;
		xOffsetOrg2Pick.y=0;
		index++;
	}while(xPick.y<=xBtmRight.y);
	//计算行高平均值dfAvH
	int i,niRows=pRegion->m_xarrBtmLinePosYOfRow.GetSize()-1;
	double dfSumHeight=pRegion->m_xarrBtmLinePosYOfRow[niRows]-pRegion->m_xarrBtmLinePosYOfRow[0];
	double dfAverageHeight=dfSumHeight/niRows;
	double ROWHEIGHT_TOLERANCE=dfAverageHeight/(2*niRows);
	//计算行高众数及中位数高度值
	int niMidPosHeight=0,indexMidPosHeight=(niRows+1)/2;	//中位数索引位置
	int niMajorHeight=0,niMajorHeightHits=0;
	int indexCurrentAccumHits=0;
	for(i=0;i<(int)xarrStatHeightHits.Count;i++)
	{
		int hits=xarrStatHeightHits[i];
		if(hits==0)
			continue;
		if( niMidPosHeight==0&&	//中位数行高值为0表示未初始化
			indexCurrentAccumHits<indexMidPosHeight&&indexCurrentAccumHits+hits>=indexMidPosHeight)
			niMidPosHeight=i;	//找到中位数索引位置
		indexCurrentAccumHits+=hits;
		if(hits>niMajorHeightHits)
		{	//查找找到行高众数值
			niMajorHeightHits=hits;
			niMajorHeight=i;
		}
		if(indexCurrentAccumHits>=niRows)
			break;
	}
	int niGuessRowHeight=niMidPosHeight;	//根据中位数原则预测实际正确行高值，也可以根据众数原则预测行高值
	if(fabs(dfAverageHeight-niGuessRowHeight)>ROWHEIGHT_TOLERANCE)
	{
		int niActualRows=f2i(dfSumHeight/niGuessRowHeight);
		if(niActualRows!=niRows)
		{
			niRows=niActualRows;
			dfAverageHeight=dfSumHeight/niActualRows;
		}
	}
	long yjTopLinePos=pRegion->m_xarrBtmLinePosYOfRow[0];
	pRegion->m_xarrBtmLinePosYOfRow.SetSize(niRows+1);
	for(i=1;i<=niRows;i++)	//根据平均行高修订行线
		pRegion->m_xarrBtmLinePosYOfRow[i]=yjTopLinePos+f2i(dfAverageHeight*i);
	//初步粗略分析列线
	xPick=xTopLeft;
	xOffsetOrg2Pick.x=20;
	xOffsetOrg2Pick.y=0;
	do{
		if(successed=TestRotRevision(xPick,xOffsetOrg2Pick,index==0?40:200,200,'X',&stat))
		{	//找到行线
			pRegion->m_xarrRightLinePosXOfCol.append(stat.xiVertMaxBlckLine);
			xPick.x=stat.xiVertMaxBlckLine+20;
		}
		else
			xPick.x+=200;
		xOffsetOrg2Pick.x=0;
		index++;
	}while(xPick.x<=xBtmRight.x);
	//XHPOINT point;point.x
	//pRegion->transform.gridmap.Init(
	return true;
}
//^^^^智能投影以获取当前明细表图像的偏转角度
////////////////////////////////////////////

SIZE CImageDataFile::GetOffsetSize(DWORD dwRegionKey/*=0*/)
{
	CImageDataRegion *pRegion=hashRegions.GetValue(dwRegionKey);
	SIZE size1,size2;
	size1.cx=pRegion->m_nOffsetX;
	size1.cy=pRegion->m_nOffsetY;
	size2.cx=m_nOffsetX;
	size2.cy=m_nOffsetY;
	return pRegion!=NULL?size1:size2;
}
void CImageDataFile::SetOffsetSize(SIZE offset,DWORD dwRegionKey/*=0*/)
{
	CImageDataRegion *pRegion=hashRegions.GetValue(dwRegionKey);
	if(pRegion)
	{
		pRegion->m_nOffsetX=offset.cx;
		pRegion->m_nOffsetY=offset.cy;
	}
	else
	{
		m_nOffsetX=offset.cx;
		m_nOffsetY=offset.cy;
	}
}
double CImageDataFile::GetDisplayRatio(DWORD dwRegionKey/*=0*/)
{
	CImageDataRegion *pRegion=hashRegions.GetValue(dwRegionKey);
	return pRegion!=NULL?pRegion->m_fDisplayRatio:m_fDisplayRatio;
}
BOOL CImageDataFile::IsCanZoom(DWORD dwRegionKey/*=0*/)
{
	CImageDataRegion *pRegion=hashRegions.GetValue(dwRegionKey);
	if(pRegion)
		return pRegion->m_fDisplayRatio<=min(1,pRegion->m_fDisplayAllRatio);
	else
		return m_fDisplayRatio<=min(1,m_fDisplayAllRatio);
}
double CImageDataFile::Zoom(short zDelta,POINT* pxScrPoint/*=NULL*/)
{
	double HALF_SCALE_OF_ZOOMALL=m_fDisplayAllRatio/2;
	//加载一些A0整张扫描图时，需要缩放到较小的比例
	double fMinDisplayRatio=min(0.05,HALF_SCALE_OF_ZOOMALL);
	double fOldRatioScr2Img=m_fDisplayRatio;
	if(zDelta==0)		//全显
		m_fDisplayRatio=m_fDisplayAllRatio;
	else if(zDelta>0)	//放大
		m_fDisplayRatio*=FIXED_SCLAE;
	else if(m_fDisplayRatio>fMinDisplayRatio)	//缩小
		m_fDisplayRatio=max(fMinDisplayRatio,m_fDisplayRatio/FIXED_SCLAE);
	if(pxScrPoint)
	{	//以指定屏幕位置为当前缩放中心
		int offset_x=this->m_nOffsetX-pxScrPoint->x;
		int offset_y=this->m_nOffsetY-pxScrPoint->y;
		double realOffsetX=offset_x/fOldRatioScr2Img;
		double realOffsetY=offset_y/fOldRatioScr2Img;
		m_nOffsetX=pxScrPoint->x+f2i((m_nOffsetX-pxScrPoint->x)*m_fDisplayRatio/fOldRatioScr2Img);
		m_nOffsetY=pxScrPoint->y+f2i((m_nOffsetY-pxScrPoint->y)*m_fDisplayRatio/fOldRatioScr2Img);
	}
	return m_fDisplayRatio;
}
bool CImageDataFile::FocusPoint(POINT xImgPoint,POINT* pxScrPoint/*=NULL*/,double scaleofScr2Img/*=2.0*/)
{
	POINT xScrFocusCenter;//,xScrOrg2ImgCenterOffset;
	if(pxScrPoint==NULL)
	{	//未指定屏幕上的缩放中心点时，自动以图像左对齐全显状态下的图像中心对应屏幕点为缩放中心
		/*double HALF_SCALE_OF_ZOOMALL=m_fDisplayAllRatio/2;
		xScrOrg2ImgCenterOffset.x=f2i(HALF_SCALE_OF_ZOOMALL*Width);
		xScrOrg2ImgCenterOffset.y=f2i(HALF_SCALE_OF_ZOOMALL*Height);
		xScrFocusCenter=xScrOrg2ImgCenterOffset;*/
		//xScrFocusCenter=xImgPoint;
		xScrFocusCenter.x=f2i((m_rcShow.left+m_rcShow.right)*0.5);
		xScrFocusCenter.y=f2i((m_rcShow.top+m_rcShow.bottom)*0.5);
	}
	else
		xScrFocusCenter=*pxScrPoint;
	//以指定屏幕点为指定图像的像素点的缩放中心
	m_fDisplayRatio=scaleofScr2Img;//>1.0?scaleofScr2Img:1.0;	//指定缩放比例
	m_nOffsetX=xScrFocusCenter.x-f2i(xImgPoint.x*m_fDisplayRatio);
	m_nOffsetY=xScrFocusCenter.y-f2i(xImgPoint.y*m_fDisplayRatio);
	return true;
}
RECT CImageDataFile::GetImageDisplayRect(DWORD dwRegionKey/*=0*/)
{
	CImageDataRegion *pRegion=hashRegions.GetValue(dwRegionKey);
	RECT rect = RECT_C(0,0,0,0);
	if(pRegion)
	{
		rect.left=pRegion->m_nOffsetX;
		rect.top=pRegion->m_nOffsetY;
		rect.right=rect.left+(int)(pRegion->m_nWidth*pRegion->m_fDisplayRatio);
		rect.bottom=rect.top+(int)(pRegion->m_nHeight*pRegion->m_fDisplayRatio);
	}
	else
	{
		rect.left=m_nOffsetX;
		rect.top=m_nOffsetY;
		rect.right=rect.left+(int)(image.GetWidth()*m_fDisplayRatio);
		rect.bottom=rect.top+(int)(image.GetHeight()*m_fDisplayRatio);
	}
	return rect;
}
void CImageDataFile::ToBuffer(CBuffer &buffer)
{
	buffer.WriteString(m_sPathFileName);
	//图片区域
	BUFFERPOP stack(&buffer,hashRegions.GetNodeNum());
	buffer.WriteInteger(hashRegions.GetNodeNum());
	for(CImageDataRegion *pRegion=hashRegions.GetFirst();pRegion;pRegion=hashRegions.GetNext())
	{
		buffer.WriteInteger(pRegion->GetSerial());
		pRegion->ToBuffer(buffer);
		stack.Increment();
	}
	if(!stack.VerifyAndRestore())
#ifdef  AFX_TARG_ENU_ENGLISH
		logerr.Log("The number record of image region is wrong!");
#else 
		logerr.Log("图片区域数量错误!");
#endif
}
void CImageDataFile::FromBuffer(CBuffer &buffer,long version)
{
	buffer.ReadString(m_sPathFileName);
	bool bRetCode=InitImageFile(m_sPathFileName,NULL);
	//图片区域
	hashRegions.Empty();
	int n=buffer.ReadInteger();
	for(int i=0;i<n;i++)
	{
		int key=buffer.ReadInteger();
		CImageDataRegion *pRegion=hashRegions.Add(key);
		pRegion->SetBelongImageData(this);
		pRegion->FromBuffer(buffer,version);
		if(bRetCode)
			RetrieveRegionImageBits(pRegion);
	}
}
void CImageDataFile::GetPathFileName(char* imagename,int maxStrBufLen/*=17*/)
{
	StrCopy(imagename,m_sPathFileName,maxStrBufLen);
}
void CImageDataFile::SetPathFileName(const char* imagefilename)
{
	m_sPathFileName.Copy(imagefilename);
}
bool CImageDataFile::LoadRawImageBytesFromVM(char* lpImgBytes/*=NULL*/,UINT uiMaxSize/*=0*/)
{
	if(lpImgBytes==NULL)
		return image.LoadRawBytesFromVirtualMemBuff(&vmRawBytes);
	vmRawBytes.SeekToBegin();
	UINT uiRawImgSize=image.GetEffWidth()*image.GetHeight();
	UINT uiReadRawImgSize=0;
	vmRawBytes.ReadInteger(&uiReadRawImgSize);
	if(uiReadRawImgSize!=uiRawImgSize||uiRawImgSize==0)
		return false;
	if(uiRawImgSize>uiReadRawImgSize)
		uiRawImgSize=uiReadRawImgSize;
	uiReadRawImgSize=uiMaxSize==0?uiRawImgSize:min(uiRawImgSize,uiMaxSize);
	vmRawBytes.Read(lpImgBytes,uiReadRawImgSize);
	return true;

}
bool CImageDataFile::UnloadRawImageBytes(bool write2vm/*=true*/)
{
	if(!vmRawBytes.IsValidFile())
	{
		//获取当进程的操作系统级全局唯一Id
		HANDLE hCurrProcess=GetCurrentProcess();
		DWORD processid=GetProcessId(hCurrProcess);
		CXhChar200 szTempFile("%sprocess-%d#vm_raw_%d.vm",robot.szTempFilePath,processid,this->GetSerial());
		vmRawBytes.CreateFile(szTempFile,0xa00000);	//缓存大小为10M
	}
	if(!vmRawBytes.IsValidFile()||image.GetRawBits()==NULL)
		return false;
	image.UnloadRawBytesToVirtualMemBuff(&vmRawBytes,write2vm);
	return true;
}
bool CImageDataFile::LoadGreyBytes(bool* pbLoadFromVmFile/*=NULL*/)
{
	bool loadfrom_virtualmemory=image.LoadGreyBytesFromVirtualMemBuff(&vmGreyBytes);
	if(pbLoadFromVmFile)
		*pbLoadFromVmFile=loadfrom_virtualmemory;
	if(!loadfrom_virtualmemory)
	{
		bool loadRawBitsFromVM=false;
		if(image.GetRawBits()==NULL)
			loadRawBitsFromVM=image.LoadRawBytesFromVirtualMemBuff(&vmRawBytes);
		image.UpdateGreyImageBits(true,m_nMonoForwardPixels);
		if(loadRawBitsFromVM)
			image.UnloadRawBytesToVirtualMemBuff(&vmRawBytes,false);
	}
	return image.GetGrayBits()!=NULL;
}
bool CImageDataFile::UnloadGreyBytes(bool write2vm/*=true*/)
{
	if(!vmGreyBytes.IsValidFile())
	{
		//获取当进程的操作系统级全局唯一Id
		HANDLE hCurrProcess=GetCurrentProcess();
		DWORD processid=GetProcessId(hCurrProcess);
		CXhChar200 szTempFile("%sprocess-%d#vm_grey_%d.vm",robot.szTempFilePath,processid,this->GetSerial());
		vmGreyBytes.CreateFile(szTempFile,0xa00000);	//缓存大小为10M
	}
	if(!vmGreyBytes.IsValidFile()||image.GetGrayBits()==NULL)
		return false;
	image.UnloadGreyBytesToVirtualMemBuff(&vmGreyBytes,write2vm);
	return true;
}
bool CImageDataFile::InitImageFileHeader(const char* imagefile)
{
	if(imagefile!=NULL)
		m_sPathFileName.Copy(imagefile);
	int len=m_sPathFileName.GetLength();
	char* ext=((char*)m_sPathFileName)+(len-4);
	if(stricmp(ext,".jpg")==0)
		this->m_ciRawImageFileType=1;
	else if(stricmp(ext,".png")==0)
		this->m_ciRawImageFileType=2;
	else if(stricmp(ext,".pdf")==0)
		this->m_ciRawImageFileType=3;
	else if (stricmp(ext, ".tif") == 0)
		this->m_ciRawImageFileType = 4;
	else
		this->m_ciRawImageFileType=0;
	BYTE exterbyte;
	return image.ReadImageFile(m_sPathFileName,&exterbyte,1,m_nMonoForwardPixels);
}
bool CImageDataFile::InitImageFile(const char* imagefile,const char* file_path,bool update_file_path/*=true*/,PDF_FILE_CONFIG *pPDFConfig/*=NULL*/)
{
	CXhChar500 sPathFileName;
	if(imagefile!=NULL)
		sPathFileName.Copy(imagefile);
	else
		sPathFileName.Copy(m_sPathFileName);
	if(imagefile!=NULL&&update_file_path)
		m_sPathFileName.Copy(imagefile);
	int len=sPathFileName.GetLength();
	char* ext=((char*)sPathFileName)+(len-4);
	if (stricmp(ext, ".jpg") == 0)
		this->m_ciRawImageFileType = RAW_IMAGE_JPG;
	else if (stricmp(ext, ".png") == 0)
		this->m_ciRawImageFileType = RAW_IMAGE_PNG;
	else if (stricmp(ext, ".pdf") == 0)
		this->m_ciRawImageFileType = RAW_IMAGE_PDF;
	else if (stricmp(ext, "*.tif") == 0)
		this->m_ciRawImageFileType = RAW_IMAGE_TIF;
	else
		this->m_ciRawImageFileType=RAW_IMAGE_NONE;
	bool retcode=false;
	if(m_ciRawImageFileType== RAW_IMAGE_PDF)
	{
		if(pPDFConfig)
			retcode=image.ReadPdfFile(sPathFileName,*pPDFConfig,m_nMonoForwardPixels);
		if(image.ciRawImageFileType!=RAW_IMAGE_PDF)
			m_ciRawImageFileType=RAW_IMAGE_PDF_IMG;
	}
	else
		retcode=image.ReadImageFile(sPathFileName,NULL,NULL,m_nMonoForwardPixels);
	if(image.GetEffWidth()*image.GetHeight()>=0x2000000)
	{
		this->UnloadRawImageBytes();
		this->UnloadGreyBytes();
		image.ReleaseRawImageBits();	//原始展开位图大小超过32M的一律不常驻内存 wjh-2018.4.16
	}
	return retcode;
}
void CImageDataFile::UpdateImageRegions()
{
	bool loadfromVM=false;
	if(image.GetGrayBits()==NULL)
		LoadGreyBytes(&loadfromVM);
	image.IntelliRecogMonoPixelThreshold(m_nMonoForwardPixels);
	for(IImageRegion* pRegionImg=EnumFirstRegion();pRegionImg;pRegionImg=EnumNextRegion())
		pRegionImg->UpdateImageRegion();
	if(loadfromVM)
		UnloadGreyBytes();
}
BYTE CImageDataFile::ImageCompressType()
{
	return m_ciRawImageFileType;
}

bool CImageDataFile::SetImageRawFileData(char* imagedata,UINT uiMaxDataSize,BYTE biFileType,PDF_FILE_CONFIG *pPDFConfig/*=NULL*/)
{
	//if(image.GetWidth()*image.GetEffWidth()>=0x2000000)
	//	return false;	//原始展开位图大小超过32M的一律不常驻内存
	CXhChar200 sTempDir;
	IMindRobot::GetWorkDirectory(sTempDir,201);
	CXhChar500 szTempFile("%s_laksdfjasdlfjasf_temp_jpg_file_%d.jpg",(char*)sTempDir,GetSerial());
	if(biFileType==2)
		szTempFile.Printf("%s_laksdfjasdlfjasf_temp_pdf_file_%d.pdf",(char*)sTempDir,GetSerial());
	FILE* fp=fopen(szTempFile,"w+b");
	fwrite(imagedata,uiMaxDataSize,1,fp);
	fseek(fp,0,SEEK_SET);
	fclose(fp);
	bool successed=InitImageFile(szTempFile,NULL,false,pPDFConfig);
	DeleteFile(szTempFile);
	return successed;
}

static UINT ReadImageFile(const char* file_path,char* imageData,UINT uiMaxDataSize)
{
	if(file_path==NULL)
		return 0;
	char extname[9]={0};
	_splitpath(file_path,NULL,NULL,NULL,extname);
	if(stricmp(extname,".jpg")!=0&&stricmp(extname,".jpeg")!=0)
		return 0;

	UINT uiImageSize=0;
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
	return buff.GetLength();
}

//UINT CImageDataFile::GetJpegImageData(char* imagedata,UINT uiMaxDataSize/*=0*/)
//{
//	return ReadImageFile(m_sPathFileName,imagedata,uiMaxDataSize);
//}
bool CImageDataFile::SetCompressImageData(char* imagedata,UINT uiMaxDataSize)
{
	//示意图数据
	CBuffer pool(imagedata,uiMaxDataSize);
	bool bHasImage=0;
	pool.ReadBooleanThin(&bHasImage);
	if(!bHasImage)
	{
		image.ClearImage();
		return false;
	}
	else
	{
		BITMAP bmp;
		LPBITMAPINFOHEADER lpBMIH=image.GetBitmapInfoHeader();
		long bmWidthBytes=image.GetEffWidth();
		pool.ReadInteger(&bmp.bmType);	//image.bmType=0
		pool.ReadInteger(&bmp.bmWidth);
		pool.ReadInteger(&bmp.bmHeight);
		pool.ReadInteger(&bmp.bmWidthBytes);
		pool.ReadWord(&bmp.bmPlanes);
		pool.ReadWord(&bmp.bmBitsPixel);	//bmBitsPixel

		//将原位图文件压缩成PNG格式存储
		//将原位图文件压缩成PNG格式存储
		ULONG comp_len;
		ULONG pLen = bmp.bmHeight*bmp.bmWidthBytes;
		if(pLen>=0x2000000)
			return false;	//原始展开位图大小超过32M的一律不常驻内存 wjh-2018.4.16
		pool.Read(&comp_len,sizeof(ULONG));
		BYTE_ARRAY compdata(comp_len);
		pool.Read(compdata,comp_len);
		BYTE_ARRAY imagebits(pLen);
		bmp.bmBits = imagebits;//new BYTE[pLen];
		if(Z_OK != uncompress((Byte*)bmp.bmBits,&pLen,compdata,comp_len))
			return false;//delete []compdata;
		image.SetBitMap(bmp);
		//delete []compdata;
		return true;
	}
}
UINT CImageDataFile::GetCompressImageData(char* imagedata,UINT uiMaxDataSize/*=0*/)
{
	//示意图数据
	CBuffer pool(0x100000);
	LPBITMAPINFOHEADER lpBMIH=image.GetBitmapInfoHeader();
	bool loadFromVmPool=false;
	if(image.GetGrayBits()==NULL)
		LoadGreyBytes(&loadFromVmPool);
	if(image.GetHeight()==0||image.GetGrayBits()==NULL)
	{
		pool.WriteBooleanThin(false);
		return 0;
	}
	else
	{
		pool.WriteBooleanThin(true);
		pool.Write(0,sizeof(LONG));	//image.bmType=0
		pool.WriteInteger(image.GetWidth());
		pool.WriteInteger(image.GetHeight());
		long bmWidthBytes=image.GetWidth();//image.GetEffWidth();	灰度图不需要行字节对齐
		pool.Write(&bmWidthBytes,sizeof(LONG));
		WORD wiBitCount=8;	//只存储灰度图
		WORD biPlanes=1;
		pool.Write(&biPlanes,sizeof(WORD));
		pool.WriteWord(wiBitCount);	//lpBMIH->biBitCount or 

		//将原位图文件压缩存储
		BYTE *buffer=NULL;
		ULONG pLen = image.GetHeight()*bmWidthBytes;
		ULONG comp_len = compressBound(pLen);
		BYTE_ARRAY compdata(comp_len);
		if(Z_OK !=compress(compdata,&comp_len,(Byte*)image.GetGrayBits(),pLen))
		{
			if(loadFromVmPool)
				UnloadGreyBytes(false);
			return false;
		}
		else if(loadFromVmPool)
			UnloadGreyBytes(false);
		pool.Write(&comp_len,sizeof(ULONG));
		pool.Write(compdata,comp_len);
		if(imagedata)
			memcpy(imagedata,pool.GetBufferPtr(),pool.GetLength());
		//delete [] compdata;
		return pool.GetLength();
	}
}
int  CImageDataFile::GetHeight()
{
	return image.GetHeight();
}
int  CImageDataFile::GetWidth()
{
	return image.GetWidth();
}
int CImageDataFile::Get24BitsImageEffWidth()
{
	return image.GetEffWidth();
}
int CImageDataFile::Get24BitsImageData(IMAGE_DATA* imagedata)
{
	bool loadrawdata_from_vm=false;
	int count=image.GetEffWidth()*image.GetHeight();
	int nHeight = image.GetHeight();
	int nWidth = image.GetWidth();
	int nEffWidth = image.GetEffWidth();
	if (IsNeedTurnImage())
	{	//根据图片翻转信息重新计算宽度高度 wht 19-11-30
		int nTurnCount = abs(GetTurnCount());
		bool bClockwise = GetTurnCount() > 0;
		for (int i = 0; i < nTurnCount; i++)
		{
			int W1 = nHeight;
			int H1 = nWidth;
			nEffWidth = ((W1 * 3 + 3) / 4) * 4;
			nWidth = W1;
			nHeight = H1;
		}
		count = nEffWidth*nHeight;
	}
	if(imagedata->imagedata&&(imagedata->nEffWidth*imagedata->nHeight)>=count)
	{
		imagedata->nHeight=nHeight;
		imagedata->nWidth=nWidth;
		imagedata->nEffWidth=nEffWidth;
		if(image.GetRawBits()!=NULL)
		{
			memcpy(imagedata->imagedata,image.GetRawBits(),count);
			if(nEffWidth*nHeight>=0x2000000)
			{
				UnloadRawImageBytes();
				UnloadGreyBytes();
			}
			return count;
		}
		if(LoadRawImageBytesFromVM(imagedata->imagedata))
			return count;
		//优先从物理文件加载文件	&&image.GetGrayBits()!=NULL)
		bool loadgreydata_from_vm=false;
		if(image.ReadImageFile(m_sPathFileName))
		{
			if (IsNeedTurnImage())
			{	//根据图片翻转信息重新计算宽度高度 wht 19-11-30
				int nTurnCount = abs(GetTurnCount());
				bool bClockwise = GetTurnCount() > 0;
				for (int i = 0; i < nTurnCount; i++)
					image.Turn90(bClockwise);
			}
			memcpy(imagedata->imagedata,image.GetRawBits(),count);
			if(nEffWidth*nHeight >=0x2000000)
			{
				UnloadGreyBytes();
				if(!loadrawdata_from_vm)
					UnloadRawImageBytes();
				else
					image.ReleaseRawImageBits();	//用完释放，否则遇到大图片多打开几张就死定了 wjh-2018.4.16
			}
			return count;
		}
		else if(image.GetGrayBits()==NULL)
			LoadGreyBytes(&loadgreydata_from_vm);
		if(image.GetGrayBits()!=NULL)
		{	//从数据文件的灰度图中动态生成图片
			//TODO:完成从灰度图向真彩图的转换 wjh-2018.2.8
			BYTE* pGrayBits=image.GetGrayBits();
			for(int i=0;i<imagedata->nWidth;i++)
			{
				for(int j=0;j<imagedata->nHeight;j++)
				{
					BYTE grayness=pGrayBits[j*imagedata->nWidth+i];
					imagedata->imagedata[j*imagedata->nEffWidth+i*3+2]=grayness;
					imagedata->imagedata[j*imagedata->nEffWidth+i*3+1]=grayness;
					imagedata->imagedata[j*imagedata->nEffWidth+i*3+0]=grayness;
				}
			}
			if(!loadgreydata_from_vm)
				UnloadGreyBytes();
			else
				image.ReleaseGreyImageBits();
			return count;
		}
		return 0;
	}
	else
	{
		imagedata->nHeight=nHeight;
		imagedata->nWidth=nWidth;
		imagedata->nEffWidth=nEffWidth;
		
	}
	return count;
}
int CImageDataFile::GetMonoImageData(IMAGE_DATA* imagedata)
{
	int nMonoEffWidth=image.GetWidth()%16>0 ? (image.GetWidth()/16)*2+2 : image.GetWidth()/8;
	int count=nMonoEffWidth*image.GetHeight();
	if(imagedata->imagedata&&(imagedata->nEffWidth*imagedata->nHeight)>=count)
	{
		imagedata->biBitCount=1;
		imagedata->nHeight=image.GetHeight();
		imagedata->nWidth=image.GetWidth();
		imagedata->nEffWidth=nMonoEffWidth;
		bool loadfrom_virtualmemory=false;
		if(image.GetGrayBits()==NULL)
			LoadGreyBytes(&loadfrom_virtualmemory);
		if(image.GetGrayBits()==NULL)
			return false;
		const BYTE bytesConstArr[8]={0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};
		for(int yJ=0;yJ<image.GetHeight();yJ++)
		{
			char* pRowPixels=imagedata->imagedata+yJ*nMonoEffWidth;
			memset(pRowPixels,0,nMonoEffWidth);
			for(int xI=0;xI<image.GetWidth();xI++)
			{
				int iBytePos=xI/8,iBitPos=xI%8;
				if(!IsBlackPixel(xI,yJ))
					pRowPixels[iBytePos]|=bytesConstArr[iBitPos];
			}
		}
		this->UnloadGreyBytes(!loadfrom_virtualmemory);
	}
	else
	{
		imagedata->nHeight=image.GetHeight();
		imagedata->nWidth=image.GetWidth();
		imagedata->nEffWidth=nMonoEffWidth;//imagedata->nWidth%16>0 ? (imagedata->nWidth/16)*2+2 : imagedata->nWidth/8;
		imagedata->biBitCount=1;
	}
	return count;
}
bool CImageDataFile::TurnClockwise90()		//将文件中图像顺时针转90度，旋转不影响已提取区域，应在提取区域之前旋转
{
	bool loadRawImgFromVM=false,loadGreyFromVM=false;
	if(image.GetRawBits()==NULL)
	{
		loadRawImgFromVM=LoadRawImageBytesFromVM();
		LoadGreyBytes(&loadGreyFromVM);
	}
	bool retcode=image.Turn90(true);
	//旋转之后更新m_nTurnCount,保证旋转角度与旋转次数一致，否则会触发JPG图像文件内部旋转 wht 19-11-30
	m_nTurnCount = image.GetPdfConfig().rotation / 90;
	if(loadRawImgFromVM)
		UnloadRawImageBytes();
	if(loadGreyFromVM)
		UnloadGreyBytes();
	return retcode;
}
bool CImageDataFile::TurnAntiClockwise90()	//将文件中图像逆时针转90度，旋转不影响已提取区域，应在提取区域之前旋转
{
	bool loadRawImgFromVM=false,loadGreyFromVM=false;
	if(image.GetRawBits()==NULL)
	{
		loadRawImgFromVM=LoadRawImageBytesFromVM();
		LoadGreyBytes(&loadGreyFromVM);
	}
	bool retcode=image.Turn90(false);
	//旋转之后更新m_nTurnCount,保证旋转角度与旋转次数一致，否则会触发JPG图像文件内部旋转 wht 19-11-30
	m_nTurnCount = image.GetPdfConfig().rotation / 90;
	if(loadRawImgFromVM)
		UnloadRawImageBytes();
	if(loadGreyFromVM)
		UnloadGreyBytes();
	return retcode;
}

bool CImageDataFile::IsNeedTurnImage()
{
	PDF_FILE_CONFIG cfg = image.GetPdfConfig();
	int nTurnCount = cfg.rotation / 90;
	if (nTurnCount != m_nTurnCount)
		return true;
	else
		return false;
}

bool CImageDataFile::IsBlackPixel(int i,int j)
{
	if(i<0||j<0||i>=Width||j>=Height)
		return false;
	return image.IsBlackPixel(i,j);
}
bool CImageDataFile::DeleteImageRegion(long idRegion)
{
	return hashRegions.DeleteNode(idRegion)!=FALSE;
}
IImageRegion* CImageDataFile::AddImageRegion()
{
	CImageDataRegion* pRegion=hashRegions.Add(0);
	pRegion->SetBelongImageData(this);
	return pRegion;
}
IImageRegion* CImageDataFile::AddImageRegion(POINT topLeft,POINT btmLeft,POINT btmRight,POINT topRight)
{
	CImageDataRegion* pRegion=hashRegions.Add(0);
	pRegion->SetBelongImageData(this);
	pRegion->SetRegion(topLeft,btmLeft,btmRight,topRight);
	return pRegion;
}
void CImageDataFile::SetScrOffset(SIZE offset)
{
	m_nOffsetX=offset.cx;
	m_nOffsetY=offset.cy;
}
SIZE CImageDataFile::GetScrOffset()
{
	SIZE size;
	size.cx=m_nOffsetX;
	size.cy=m_nOffsetY;
	return size;
}
IImageRegion* CImageDataFile::EnumFirstRegion()
{
	return hashRegions.GetFirst();
}
IImageRegion* CImageDataFile::EnumNextRegion()
{
	return hashRegions.GetNext();
}
//////////////////////////////////////////////////////////////////////////
// CAlphabets
CAlphabets::FONTINFO::FONTINFO()
{
	wFontSerial=0;
	m_nTemplateH=0;
	m_fHtoW=0;
	m_fLowestSimilarity=0.7;
	m_bPreSplitChar=FALSE;
}
void CAlphabets::FONTINFO::ToBuffer(CBuffer &buff,DWORD dwVersion/*=0*/,BOOL bIncSerial/*=TRUE*/)
{
	XHVERSION xhver(dwVersion);
	if(bIncSerial)
		buff.WriteWord(wFontSerial);
	buff.WriteInteger(m_nTemplateH);
	buff.WriteDouble(m_fHtoW);
	if(dwVersion==0||xhver.Compare("0.0.0.3")>=0)
	{
		buff.WriteInteger(m_bPreSplitChar);
		buff.WriteByte(0);//m_biRecogMode);
	}

}
void CAlphabets::FONTINFO::FromBuffer(CBuffer &buff,DWORD dwVersion/*=0*/,BOOL bIncSerial/*=TRUE*/)
{
	XHVERSION xhver(dwVersion);
	if(bIncSerial)
		buff.ReadWord(&wFontSerial);
	buff.ReadInteger(&m_nTemplateH);
	buff.ReadDouble(&m_fHtoW);
	if(dwVersion==0||xhver.Compare("0.0.0.3")>=0)
	{
		buff.ReadInteger(&m_bPreSplitChar);
		BYTE m_biRecogMode;
		buff.ReadByte(&m_biRecogMode);
	}
}

CAlphabets::CAlphabets()
{
	pCharX=pCharX2=pCharL=pCharQ=pCharPoint=pCharZero=NULL;
	ClearAdaptiveFonts();
}
bool ValidateWChar(char* psWChar)
{
	if(stricmp(psWChar,"SLASH")==0)
	{
		*psWChar='/';
		*(psWChar+1)=0;
	}
	else if(psWChar[1]!=0&&psWChar[0]>='a'&&psWChar[0]<='z')
		psWChar[1]=0;
	else if(psWChar[1]!=0&&psWChar[0]>='A'&&psWChar[0]<='Z')
	{
		psWChar[0]+='a'-'A';
		psWChar[1]=0;
	}
	return true;
}
bool CAlphabets::InitFromFolder(const char* folder_path,WORD font_serial/*=0*/,bool append_mode/*=false*/)
{
	CXhChar100 sName;	
	WIN32_FIND_DATA FindFileData;
	char fn[MAX_PATH];
	sprintf(fn,"%s*.bmp",(char*)folder_path);
	HANDLE hFindFile = FindFirstFile(fn, &FindFileData);
	if (hFindFile ==INVALID_HANDLE_VALUE)
	{
		sprintf(fn,"%s*.jpg",(char*)folder_path);
		hFindFile = FindFirstFile(fn, &FindFileData);
	}
	if (hFindFile ==INVALID_HANDLE_VALUE)
	{
		//logerr.Log("未找到字符模板文件*.jpg!\n");
		return false;
	}
	CMindRobot::InitCryptData();	//添加第1张图像文件时初始化加密函数
	if(!append_mode)
	{
		hashFonts.Empty();
		listChars.Empty();
	}
	do{
		CXhChar200 sFileName("%s%s",(char*)folder_path, FindFileData.cFileName);
		_splitpath(sFileName,NULL,NULL,sName,NULL);
		char* szSplitter=strchr(sName,'#');
		WORD fontserial=0;
		if(font_serial>0)
		{
			fontserial=font_serial;
			if(szSplitter)
				continue;	//外部指定字体序号时，不支持#添加字体库 wht 18-07-05
		}
		else if(szSplitter)
		{
			fontserial=atoi(szSplitter+1);
			*szSplitter=0;
		}
		ValidateWChar(sName);
		InitFromImageFile(XHWCHAR(sName),sFileName,fontserial);
		if(fontserial>0)
			hashFonts.Add(fontserial);
	} while (FindNextFile(hFindFile,&FindFileData));
	//以下应该是读取完所有字符模板图像后的最终规范化操作
	CImageChar* pAlphabet;
	FONTINFO* pFont=&basicfont;
	if(font_serial>0)
		pFont=hashFonts.GetValue(font_serial);
	while(pFont)
	{
		BYTE cSampleCounts=0;
		float fStdHeight=0,fStdWidth=0;
		//优先根据字符'Q','L','2','8'统计字体信息，在没有找到时启用其它普通字符
		BYTE cSampleCounts2=0;
		float fStdHeight2=0,fStdWidth2=0;
		WORD fontserial=(pFont==&basicfont)?0:pFont->wFontSerial;
		for(pAlphabet=listChars.GetFirst();pAlphabet;pAlphabet=listChars.GetNext())
		{
			while(pAlphabet&&pAlphabet->FontSerial!=fontserial)
				pAlphabet=pAlphabet->linknext;
			if(pAlphabet==NULL)
				continue;
			if(pAlphabet->wChar.wHz=='Q'||pAlphabet->wChar.wHz=='L'||pAlphabet->wChar.wHz=='2'||pAlphabet->wChar.wHz=='8')
			{
				fStdHeight+=pAlphabet->m_nHeight;	//wMaxValidHeight-wMinValidHeight+1;
				fStdWidth +=pAlphabet->m_nWidth;	//wMaxValidWidth -wMinValidWidth +1;
				cSampleCounts++;
			}
			else if(cSampleCounts==0)
			{
				fStdHeight2+=pAlphabet->m_nHeight;	//wMaxValidHeight-wMinValidHeight+1;
				fStdWidth2 +=pAlphabet->m_nWidth;	//wMaxValidWidth -wMinValidWidth +1;
				cSampleCounts2++;
			}
		}
		if(cSampleCounts>0)
		{
			WORD wStdWidth =f2i(fStdWidth /cSampleCounts);
			WORD wStdHeight=f2i(fStdHeight/cSampleCounts);
			pFont->m_nTemplateH=max(wStdWidth,wStdHeight);
		}
		else
		{
			WORD wStdWidth =f2i(fStdWidth2 /cSampleCounts2);
			WORD wStdHeight=f2i(fStdHeight2/cSampleCounts2);
			pFont->m_nTemplateH=max(wStdWidth,wStdHeight);
		}
		if(font_serial>0)
			break;
		if(pFont==&basicfont)
			pFont=hashFonts.GetFirst();
		else
			pFont=hashFonts.GetNext();
	}
	//规则化字体库中各字体的标准高与宽
	for(CImageChar* pText=listChars.GetFirst();pText;pText=listChars.GetNext())
	{
		if(pText->wChar.wHz=='L')
			pCharL=pText;
		else if(pText->wChar.wHz=='Q')
			pCharQ=pText;
		else if(pText->wChar.wHz=='X')
			pCharX=pText;
		else if(pText->wChar.wHz=='x')
			pCharX2=pText;
		else if(pText->wChar.wHz=='.')
			pCharPoint=pText;
		else if(pText->wChar.wHz=='0')
			pCharZero=pText;
		pFont=NULL;
		do
		{
			if(pText->FontSerial==0)
				pFont=&basicfont;
			else
				pFont=pText!=NULL?hashFonts.GetValue(pText->FontSerial):NULL;
			if(font_serial==0||pFont->wFontSerial==font_serial)
			{
				int height=pFont!=NULL?pFont->m_nTemplateH:basicfont.m_nTemplateH;
				pText->Standardize(height,height);
			}
			pText=pText->linknext;
		}while(pText!=NULL);
	}
	return true;
}
//TODO:Part III
void CAlphabets::InitFromImageFile(const WORD wChar, const char* imagefile, const WORD wiFontSerial/*=0*/)
{
	CImageTransform image;
	image.ReadImageFile(imagefile);
	RECT rect;
	rect.left=rect.top=0;
	rect.right=image.GetWidth();
	rect.bottom=image.GetHeight();
	int nEffWidth=rect.right%16>0 ? (rect.right/16)*2+2 : rect.right/8;
	BYTE_ARRAY bitsmap(4*24);
	bitsmap.Resize(nEffWidth*rect.bottom);
	bitsmap.Clear();
	for(int i=0;i<rect.right;i++)
	{
		for(int j=0;j<rect.bottom;j++)
		{
			int px=i;
			//int py=rect.bottom-1-j;
			int py=j;	//已从图像读取时统一为Downward行扫描形式
			if(!image.IsBlackPixel(px,py))
			{
				int iByte=i/8,iBit=i%8;
				bitsmap[j*nEffWidth+iByte]|=BIT2BYTE[7-iBit];
			}
		}
	}
	bool existed=listChars.GetValue(wChar)!=NULL;
	CImageChar* pText=listChars.Add(wChar);
	if(wiFontSerial>0)
	{
		if(existed)
			pText=pText->AppendFontChar(wiFontSerial);
		else
			pText->set_FontSerial(wiFontSerial);
	}
	pText->InitTemplate(bitsmap,nEffWidth,image.GetWidth(),rect);
	/*if(pText->wChar.wHz=='6'||pText->wChar.wHz=='8'||pText->wChar.wHz=='9')
	{
		CXhSimpleList<CImageChar::ISLAND> islands;
		int count=pText->DetectIslands(&islands);
		count=0;
	}*/
}
int CAlphabets::GetCharMonoImageData(const WORD wChar, IMAGE_DATA* imagedata,WORD fontserial/*=0*/)	//每位代表1个像素，0表示白点,1表示黑点
{
	CImageChar* pImageChar= listChars.GetValue(wChar);
	if(pImageChar==NULL)
		return 0;
	if(fontserial>0)
	{
		CImageChar* pSerialChar=pImageChar;
		while(pSerialChar!=NULL&&pSerialChar->FontSerial!=fontserial)
			pSerialChar=pSerialChar->linknext;
		if(pSerialChar!=NULL)
			pImageChar=pSerialChar;
	}
	if(imagedata==NULL)
		return pImageChar->m_nEffWidth*pImageChar->m_nHeight;
	imagedata->nHeight=pImageChar->GetImageHeight();
	imagedata->nWidth=pImageChar->GetImageWidth();
	imagedata->nEffWidth=pImageChar->GetImageEffWidth();//m_nWidth%16>0 ? (m_nWidth/16)*2+2 : m_nWidth/8;
	int count=pImageChar->m_nEffWidth*pImageChar->m_nHeight;
	if(imagedata->imagedata)
		memcpy(imagedata->imagedata,pImageChar->m_lpBitMap,count);
	return count;
}
void CAlphabets::SetLowestRecogSimilarity(WORD fontserial,double fSimilarity)
{
	if(fontserial==0)
		basicfont.m_fLowestSimilarity=fSimilarity;
	else
	{
		FONTINFO* pFontInfo=hashFonts.Add(fontserial);
		pFontInfo->m_fLowestSimilarity=fSimilarity;
	}
}
double CAlphabets::LowestRecogSimilarity(WORD fontserial/*=0*/)
{
	//if(fontserial==0)
	//	fontserial=GetActiveFontSerial();
	FONTINFO* pFontInfo=fontserial>0?hashFonts.GetValue(fontserial):NULL;
	if(pFontInfo)
		return pFontInfo->m_fLowestSimilarity;
	else //if(fontserial==0)
		return basicfont.m_fLowestSimilarity;
}
void CAlphabets::FilterRecogState(BYTE cbCellTypeFlag/*=0*/,DWORD dwParam/*=0*/)
{
	CELLTYPE_FLAG flag=cbCellTypeFlag;
	for(CImageChar* pTmplChar=listChars.GetFirst();pTmplChar;pTmplChar=listChars.GetNext())
	{
		if(cbCellTypeFlag==0)
		{
			pTmplChar->m_bTemplEnabled=true;
			continue;
		}
		if(pTmplChar->wChar.wHz>='0'&&pTmplChar->wChar.wHz<='9')
			pTmplChar->m_bTemplEnabled=true;	//任何类型单元格都有可能存在数字
		else if((pTmplChar->IsCharX()||pTmplChar->IsCharFAI()||pTmplChar->wChar.wHz=='-')&&
			flag.IsHasCellTextType(CImageCellRegion::GUIGE))
		{
			pTmplChar->m_bTemplEnabled=true;
		}
		if( pTmplChar->wChar.wHz>'A'&&pTmplChar->wChar.wHz<='Z'&&(
			flag.IsHasCellTextType(CImageCellRegion::PARTNO)||flag.IsHasCellTextType(CImageCellRegion::GUIGE)))
		{	//暂时把字母也全部开放给编号与规格
			pTmplChar->m_bTemplEnabled=true;
		}
		else if(pTmplChar->wChar.wHz=='.'&&(
			flag.IsHasCellTextType(CImageCellRegion::GUIGE)||
			flag.IsHasCellTextType(CImageCellRegion::PIECE_W)||
			flag.IsHasCellTextType(CImageCellRegion::SUM_W)))
		{	//小数点仅开放给规格、重量列
			pTmplChar->m_bTemplEnabled=true;
		}
		else if(pTmplChar->wChar.wHz>=128&&flag.IsHasCellTextType(CImageCellRegion::NOTE))
			pTmplChar->m_bTemplEnabled=true;
		else
			pTmplChar->m_bTemplEnabled=false;
	}
}
void CAlphabets::SetPreSplitChar(WORD fontserial,BOOL bPreSplitChar)
{
	if(fontserial==0)
		basicfont.m_bPreSplitChar=bPreSplitChar;
	else
	{
		FONTINFO* pFontInfo=hashFonts.Add(fontserial);
		pFontInfo->m_bPreSplitChar=bPreSplitChar;
	}
}
BOOL CAlphabets::IsPreSplitChar(WORD fontserial/*=0*/)
{
	//if(fontserial==0)
	//	fontserial=GetActiveFontSerial();
	FONTINFO* pFontInfo=fontserial>0?hashFonts.GetValue(fontserial):NULL;
	if(pFontInfo)
		return pFontInfo->m_bPreSplitChar;
	else //if(fontserial==0)
		return basicfont.m_bPreSplitChar;
}
void InitStrokeConfig(CStrokeFeature* pStroke)
{	//这部分代码未来应从strokes.ini中读取
	switch(pStroke->Id)	//
	{
	case 1:	// '2','3'字符上部
		pStroke->AddSearchRegion(RECT_C(0,0,40,20));
		pStroke->SetLiveStateInChar('2',1);
		pStroke->SetLiveStateInChar('3',1);
		break;
	case 2:	// '4'字符上部
		pStroke->AddSearchRegion(RECT_C(0,0,40,20));
		pStroke->SetLiveStateInChar('4',1);
		break;
	case 3:	// '5'字符上部
		pStroke->AddSearchRegion(RECT_C(0,0,40,20));
		pStroke->SetLiveStateInChar('5',1);
		break;
	case 4:	// '7'字符上部
		pStroke->AddSearchRegion(RECT_C(0,0,40,20));
		pStroke->SetLiveStateInChar('7',1);
		break;
	case 5:	// '9'字符上部
		pStroke->AddSearchRegion(RECT_C(0,0,40,20));
		pStroke->SetLiveStateInChar('9',CStrokeFeature::STATE_UPPER);
		break;
	case 6:	// '6'字符下部
		pStroke->AddSearchRegion(RECT_C(0,20,40,40));
		pStroke->SetLiveStateInChar('6',CStrokeFeature::STATE_DOWNER);
		break;
	case 7:	// '3'、'5'字符下部
		pStroke->AddSearchRegion(RECT_C(0,20,40,40));
		pStroke->SetLiveStateInChar('3',CStrokeFeature::STATE_DOWNER);
		pStroke->SetLiveStateInChar('5',CStrokeFeature::STATE_DOWNER);
		break;
	case 8:	// 'L'字符下部
		pStroke->AddSearchRegion(RECT_C(0,20,40,40));
		pStroke->SetLiveStateInChar('L',CStrokeFeature::STATE_DOWNER);
		break;
	case 9:	//'X'字符中部; 'Q'字符下部,
		pStroke->AddSearchRegion(RECT_C(10,10,40,40));
		pStroke->SetLiveStateInChar('X',CStrokeFeature::STATE_MIDDLE);
		pStroke->SetLiveStateInChar('Q',CStrokeFeature::STATE_DOWNER);
		break;
	case 10://'8'字符上部;
		pStroke->AddSearchRegion(RECT_C(0,0,40,20));
		pStroke->SetLiveStateInChar('8',CStrokeFeature::STATE_UPPER);
		break;
	case 11:	// '6','8'字符下部
		pStroke->AddSearchRegion(RECT_C(0,20,40,40));
		pStroke->SetLiveStateInChar('6',CStrokeFeature::STATE_DOWNER);
		pStroke->SetLiveStateInChar('8',CStrokeFeature::STATE_DOWNER);
		break;
	default:
		break;
	}
}
UINT CAlphabets::InitStrokeFeatureByFiles(const char* szFolderPath)
{
	CMindRobot::InitCryptData();
	WIN32_FIND_DATA FindFileData;
	char fn[MAX_PATH];
	sprintf(fn,"%s*.bmp",(char*)szFolderPath);
	HANDLE hFindFile = FindFirstFile(fn, &FindFileData);
	if (hFindFile ==INVALID_HANDLE_VALUE)
		return 0;//logerr.Log("未找到任字符特征文件*.bmp!\n");
	CXhChar16 idstr,sName;
	CStrokeFeature* pStroke;
	hashStrokeFeatures.Empty();
	do{
		CXhChar200 sFileName("%s%s",szFolderPath, FindFileData.cFileName);
		_splitpath(sFileName,NULL,NULL,sName,NULL);
		char* szSplitter=strchr(sName,'@');
		if(szSplitter==NULL)
			continue;
		idstr.NCopy(sName,szSplitter-(char*)sName);
		UINT idStroke=atoi(idstr);
		pStroke=hashStrokeFeatures.Add(idStroke);
		pStroke->FromBmpFile(sFileName);
		InitStrokeConfig(pStroke);
	} while (FindNextFile(hFindFile,&FindFileData));
	//读入strokes.ini配置文件
	//TODO:可由InitStrokeConfig暂时代替执行其实始化笔划信息设定功能 wjh-2018.11.6
	return true;
}
void CAlphabets::SetFontLibRootPath(const char* root_path)
{
	if(root_path!=NULL)
		strcpy(m_sFontRootPath,root_path);
}
bool CAlphabets::ImportFontsFile(int iFontsLibId)
{
	if(m_sFontRootPath==NULL||strlen(m_sFontRootPath)<=0)
		return false;
	CXhChar500 sFontsFilePath;
	sFontsFilePath.Printf("%s\\ibom.fonts",m_sFontRootPath);
	if(iFontsLibId>0)
		sFontsFilePath.Printf("%s\\ibom.fonts-%d",m_sFontRootPath,iFontsLibId);
	return ImportFontsFile(sFontsFilePath);
}
UINT CAlphabets::ToBLOB(char* blob,UINT uBlobBufSize,DWORD dwVersion/*=0*/)
{
	XHVERSION xhver(dwVersion);
	CBuffer buff(blob,uBlobBufSize);
	buff.ClearContents();
	if(xhver.Compare("0.0.0.3")>=0)
	{
		basicfont.ToBuffer(buff,dwVersion);
		buff.WriteInteger(hashFonts.GetNodeNum());
		for(FONTINFO *pFontInfo=hashFonts.GetFirst();pFontInfo;pFontInfo=hashFonts.GetNext())
		{
			buff.WriteInteger(pFontInfo->wFontSerial);
			pFontInfo->ToBuffer(buff,dwVersion,FALSE);
		}
	}
	else
	{
		buff.WriteWord(basicfont.wFontSerial);
		buff.WriteInteger(basicfont.m_nTemplateH);
		buff.WriteDouble(basicfont.m_fHtoW);	
	}
	CImageChar* pImageChar;
	WORD wCharCount=0;
	UINT uiPushPosition=buff.GetCursorPosition();
	buff.WriteWord(wCharCount);
	for(pImageChar=listChars.GetFirst();pImageChar;pImageChar=listChars.GetNext())
	{
		wCharCount++;
		buff.WriteWord(pImageChar->wChar);
		pImageChar->ToBuffer(buff);
		if(xhver.Compare("0.0.0.2")>=0)
		{	//写入后续字体
			BUFFERPOP stackfont(&buff,0);
			buff.WriteInteger(0);
			CImageChar *pChildText=pImageChar->linknext;
			while(pChildText)
			{
				buff.WriteWord(pChildText->FontSerial);
				pChildText->ToBuffer(buff);
				stackfont.Increment();
				pChildText=pChildText->linknext;
			}
			stackfont.VerifyAndRestore();
		}
	}
	UINT uiNowPosition=buff.GetCursorPosition();
	buff.SeekPosition(uiPushPosition);
	buff.WriteWord(wCharCount);
	buff.SeekPosition(uiNowPosition);

	buff.SeekToBegin();
	if(blob&&buff.GetLength()>uBlobBufSize)
		buff.Read(blob,uBlobBufSize);
	return min(buff.GetLength(),uBlobBufSize);
}
void CAlphabets::FromBLOB(char* blob,UINT uBlobSize,DWORD dwVersion/*=0*/)
{
	CBuffer buff(blob,uBlobSize);
	XHVERSION xhver(dwVersion);
	WORD i,j,wFontCount=1,wCharCount=0,wChar=0;
	if(xhver.Compare("0.0.0.3")<0)
	{
		if(xhver.Compare("0.0.0.2")<0)
			buff.ReadWord(&wFontCount);
		if(xhver.Compare("0.0.0.2")<0)
		{
			basicfont.wFontSerial=(WORD)buff.ReadInteger();
			CXhChar16 fontname;
			buff.ReadString(fontname);
		}
		else
			buff.ReadWord(&basicfont.wFontSerial);
		buff.ReadInteger(&basicfont.m_nTemplateH);
		buff.ReadDouble(&basicfont.m_fHtoW);
	}
	else
	{
		basicfont.FromBuffer(buff,dwVersion);
		int nCount=buff.ReadInteger();
		hashFonts.Empty();
		for(int i=0;i<nCount;i++)
		{
			int wFontSerial=buff.ReadInteger();
			FONTINFO *pFontInfo=hashFonts.Add(wFontSerial);
			pFontInfo->FromBuffer(buff,dwVersion,FALSE);
		}
	}
	buff.ReadWord(&wCharCount);
	for(i=0;i<wCharCount;i++)
	{
		CImageChar* pImageChar;
		buff.ReadWord(&wChar);
		pImageChar=listChars.Add(wChar);
		pImageChar->FromBuffer(buff,0);
		pImageChar->StatLocalBlackPixelFeatures();
		if(pImageChar->wChar.wHz=='L')
			pCharL=pImageChar;
		else if(pImageChar->wChar.wHz=='Q')
			pCharQ=pImageChar;
		else if(pImageChar->wChar.wHz=='X')
			pCharX=pImageChar;
		else if(pImageChar->wChar.wHz=='x')
			pCharX2=pImageChar;
		else if(pImageChar->wChar.wHz=='.')
			pCharPoint=pImageChar;
		else if(pImageChar->wChar.wHz=='0')
			pCharZero=pImageChar;
		if(xhver.Compare("0.0.0.2")>=0)
		{	//写入后续字体
			int fontchar_count=0;
			buff.ReadInteger(&fontchar_count);
			CImageChar *pChildText=pImageChar;
			for(j=0;j<fontchar_count;j++)
			{
				WORD fontserial=0;
				buff.ReadWord(&fontserial);
				pChildText=pChildText->AppendFontChar(fontserial);
				pChildText->FromBuffer(buff,dwVersion);
			}
		}
	}
}
bool CAlphabets::ImportFontsFile(const char* fontsfilename)
{
	FILE* fp=fopen(fontsfilename,"rb");
	if(fp==NULL)
	{
		//AfxMessageBox("用户主服务器授权文件打开失败!");
		return false;
	}
	//CWaitCursor waitcursorobj;
	XHVERSION fontsfile_version("0.0.0.2");
	//1.读取authentication文件头信息
	UINT uDocType=2837543355;	//2834534455为一随机数表示信狐公司 授权文件
	fread(&uDocType,4,1,fp);	//doc_type, 
	if(uDocType!=2837543355)
	{
		//AfxMessageBox("用户主服务器授权文件类型验证失败!");
		fclose(fp);
		return false;//-2;
	}
	fread(&fontsfile_version,4,1,fp);// 
	//(fontsfile_version.Compare("0.0.0.2")>=0)
	//2.读取实质内容信息
	CMindRobot::InitCryptData();
	UINT uiBlobSize=0;
	fread(&uiBlobSize,4,1,fp);
	CHAR_ARRAY blob(uiBlobSize);
	fread(blob,uiBlobSize,1,fp);
	fclose(fp);
	IAlphabets *pAlphabets=IMindRobot::GetAlphabetKnowledge();
	pAlphabets->FromBLOB(blob,uiBlobSize,fontsfile_version.dwVersion);
	return true;
}
bool CAlphabets::ExportFontsFile(const char* fontsfilename)
{
	FILE* fp=fopen(fontsfilename,"wb");
	if(fp==NULL)
		return false;
	XHVERSION fontsfile_version("0.0.0.3");
	CBuffer buff;
	CHAR_ARRAY blob(0x80000);
	IAlphabets *pAlphabets=IMindRobot::GetAlphabetKnowledge();
	UINT uiBlobSize=pAlphabets->ToBLOB(blob,blob.Size(),fontsfile_version.dwVersion);
	if(uiBlobSize>0x80000)
	{
		blob.Resize(uiBlobSize);
		uiBlobSize=pAlphabets->ToBLOB(blob,blob.Size(),fontsfile_version.dwVersion);//blob.si
	}
	//CWaitCursor waitcursorobj;
	//1.写入fonts文件头信息
	UINT uDocType=2837543355;	//2837543355为一随机数表示信狐公司 授权文件
	fwrite(&uDocType,4,1,fp);	//doc_type, 
	fwrite(&fontsfile_version,4,1,fp);// 
	//2.写入实质内容信息
	DWORD dwContentsLength_s=0,dwParam=0;
	fwrite(&uiBlobSize,4,1,fp);
	fwrite(blob,uiBlobSize,1,fp);
	fclose(fp);
	return true;
}
int CAlphabets::GetTemplateH(int template_serial/*=0*/)
{
	//if(template_serial==0)
	//	template_serial=GetActiveFontSerial();
	FONTINFO *pSerial=template_serial>0?hashFonts.GetValue(template_serial):NULL;
	if(pSerial)
		return pSerial->m_nTemplateH;
	else
		return basicfont.m_nTemplateH;
}

void CAlphabets::RetrieveImageTextByTemplChar(CImageChar &safeText,CImageChar *pTemplChar,CImageCellRegion *pCellRegion,int iStartPos)
{
	int stdHeight=GetTemplateH();
	RECT rect=pCellRegion->GetClipRect();
	int cellTextHeight=rect.bottom-rect.top;
	double scale=(1.0*cellTextHeight)/stdHeight;
	pTemplChar->StatLocalBlackPixelFeatures();
	int cellWidthChar=(int)((pTemplChar->rcActual.right-pTemplChar->rcActual.left)*scale);
	safeText.StandardLocalize(pCellRegion->m_lpBitMap,pCellRegion->BlackBitIsTrue(),pCellRegion->m_nEffWidth,RECT_C(iStartPos,rect.top,iStartPos+cellWidthChar,rect.bottom),stdHeight);
	safeText.StatLocalBlackPixelFeatures();
}

void CAlphabets::ClearAdaptiveFonts()
{
	m_uiSumMatchHits=m_uiLastOptOrderHits=0;
	xarrFontPreferGrade.Clear();
	xarrFontPreferOrder.Clear();
	FONT_PRACTICE defaultfont;
	xarrFontPreferGrade.Set(0,defaultfont);
	xarrFontPreferOrder.Set(0,0);
}
CAlphabets::FONT_PRACTICE* CAlphabets::GetFontPractice(WORD wiFontSerial)
{
	FONT_PRACTICE* pPractice;
	if(wiFontSerial<=xarrFontPreferOrder.Count)
	{
		if((pPractice=xarrFontPreferGrade.GetAt(xarrFontPreferOrder[wiFontSerial]))!=NULL&&
			pPractice->wiFontSerial==wiFontSerial)
			return pPractice;
	}
	for(UINT i=0;i<xarrFontPreferGrade.Count;i++)
	{
		if(xarrFontPreferGrade[i].wiFontSerial==wiFontSerial)
			return &xarrFontPreferGrade[i];
	}
	return NULL;
}
void CAlphabets::PracticeFontRecogChance(WORD wiFontSerial,double matchcoef)
{
	if(m_uiSumMatchHits>1000)
		return;	//超过1000个字符就不再优化更新字体库优先顺序
	FONT_PRACTICE* pPractice=GetFontPractice(wiFontSerial);
	if(pPractice)
		pPractice->AddMatchHits(matchcoef);
	else
	{
		pPractice=xarrFontPreferGrade.Append(FONT_PRACTICE(wiFontSerial));
		xarrFontPreferOrder.Set(wiFontSerial,xarrFontPreferGrade.Count-1);
	}
	m_uiSumMatchHits++;
	if(m_uiSumMatchHits-m_uiLastOptOrderHits>5)
	{
		m_uiLastOptOrderHits=m_uiSumMatchHits;
		CHeapSort<FONT_PRACTICE>::HeapSortClassic(xarrFontPreferGrade.Data(),xarrFontPreferGrade.Count);
		for(UINT i=0;i<xarrFontPreferGrade.Count;i++)
			xarrFontPreferOrder.Set(xarrFontPreferGrade[i].wiFontSerial,i);
	}
}
int CAlphabets::GetRecogTmplChars(CImageChar* pHeadTmplChar,CImageCharPtr* ppTmplChars,int nMaxTmplCharCount/*=0*/)
{
	PRESET_ARRAY8<CImageChar*> chars;
	CImageChar* pTmplChar=pHeadTmplChar;
	UINT i,hits=0,max_active_font_count=2,actfont=0;
	for(i=0;i<xarrFontPreferGrade.Count;i++)
	{
		hits+=xarrFontPreferGrade[i].uiHits;
		actfont++;
		if(hits>20)	//识别超过20个字符后就不在全字库匹配
			break;
	}
	if(hits>20)
		max_active_font_count=min(3,xarrFontPreferGrade.Count);
	else
		max_active_font_count=0;	//全部字库匹配
	do
	{
		if(pTmplChar->FontSerial==0||max_active_font_count==0)
			chars.Append(pTmplChar);
		else
		{
			for(i=0;i<max_active_font_count;i++)
			{
				if(pTmplChar->FontSerial==xarrFontPreferGrade[i].wiFontSerial)
					chars.Append(pTmplChar);
			}
		}
		pTmplChar=pTmplChar->linknext;
	}while(pTmplChar!=NULL);
	int validcount=chars.Count;
	if(nMaxTmplCharCount>0&&nMaxTmplCharCount<validcount)
		validcount=nMaxTmplCharCount;
	if(ppTmplChars)
		memcpy(ppTmplChars,chars.Data(),sizeof(CImageChar*)*validcount);
	return chars.Count;
}
//
BOOL CAlphabets::RecognizeData(CImageChar* pText,double minMatchCoef/*=0.0*/,MATCHCOEF* pMachCoef/*=NULL*/)
{
#ifdef _TIMER_COUNT
	timer.Relay();
#endif
	CXhSimpleList<CImageChar::ISLAND> islands;
	pText->DetectIslands(&islands);
#ifdef _TIMER_COUNT
	timer.Relay(40220);
#endif
	MATCHCHAR* pMatch;
	CXhSimpleList<MATCHCHAR> listMatches;
	pText->StatLocalBlackPixelFeatures();
	int side_mingap=max(2,f2i(pText->m_nHeight*0.05));
	int side_maxgap=max(2,f2i(pText->m_nHeight*0.2));
	bool needextend=pText->rcActual.top<side_maxgap&&(pText->m_nHeight-pText->rcActual.bottom-1)<side_maxgap&&
		(pText->rcActual.top>=side_mingap||(pText->m_nHeight-pText->rcActual.bottom-1)>=side_mingap);
	if(pText->m_nWidth>pText->m_nHeight||pText->m_nHeight<5)
		needextend=false;	//高宽比大于1，不需要拉伸，如“-” wht 18-07-25

	CImageChar safetext,*pSafeText=pText;
#ifdef _TIMER_COUNT
	timer.Relay(40221);
#endif
	if(minMatchCoef==0)
		minMatchCoef=LowestRecogSimilarity();
	double dfActualMinWtoHcoefOfChar1=m_fMinWtoHcoefOfChar1;
	if (dfActualMinWtoHcoefOfChar1<=0)
		dfActualMinWtoHcoefOfChar1=1.0/7;	//0.143
	int nMinTextW_CHAR_1=max(1,f2i(GetTemplateH()*dfActualMinWtoHcoefOfChar1));	//40字高存在宽为6的'1'字符
	for(CImageChar* pTemplChar=listChars.GetFirst();pTemplChar;pTemplChar=listChars.GetNext())
	{
		if(!pTemplChar->m_bTemplEnabled)
			continue;
		if(pTemplChar->wChar.wHz=='1'&&pText->m_nDestValidWidth<nMinTextW_CHAR_1)
			continue;
		if( needextend&&pTemplChar->wChar.wHz!='.'&&pTemplChar->wChar.wHz!='_'&&
			pTemplChar->wChar.wHz!='-'&&pTemplChar->wChar.wHz!='x')
		{	//需要重新延展至最大的非全高字符
			if(safetext.wChar.wHz==0)	//第一次需要初始化延展
			{
				CBitImage bitmap(NULL,pText->m_nWidth,pText->m_nHeight,0,true);
				for(int yJ=0;yJ<pText->m_nHeight;yJ++)
				{
					BYTE* rowdata=pText->m_lpBitMap+yJ*pText->m_nEffWidth;
					for(int xI=0;xI<pText->m_nWidth;xI++)
						bitmap.SetPixelState(xI,yJ,(*(rowdata+xI)!=0));
				}
				safetext.StandardLocalize(bitmap.m_lpBitMap,bitmap.BlackBitIsTrue(),bitmap.m_nEffWidth,pText->rcActual,GetTemplateH(),false);
				safetext.StatLocalBlackPixelFeatures();
			}
			pSafeText=&safetext;
		}
		pSafeText->wChar=pTemplChar->wChar;
		/*bool impossible=false;
		for(CStrokeFeature* pStroke=this->hashStrokeFeatures.GetFirst();pStroke&&!impossible;pStroke=this->hashStrokeFeatures.GetNext())
		{
			//该特征在指定字符中的存在状态,0:无约定;>0在字符指定部位;<0不允许在该字符出现
			char ciLiveState=pStroke->LiveStateInChar(pTemplChar->wChar);
			if(ciLiveState==0)
				continue;
			bool existed=pStroke->RecogInImageChar(pSafeText);
			if((ciLiveState>0&&!existed)||(ciLiveState<0&&existed))
				impossible=true;
		}
		if(impossible)
			continue;	//不可能匹配上的字符*/

		int checkfeature=pSafeText->CheckFeatures(&islands);
		bool bRaiseMatchCoef=(checkfeature==-2);	//如'0'字符存在局部像素缺失导致无孤岛情况存在
		double deductcoef=0;
		/*if(!m_bPreferStrokeFeatureRecog)	//图像可能存在失真
			deductcoef=0.2;
		else*/ 
			if(checkfeature==-1)
			continue;	//确定不符合特征
		pMatch=listMatches.AttachObject();
		pMatch->bReExtended=(pSafeText!=pText);
		pMatch->wChar=pTemplChar->wChar;
		pMatch->pTemplChar=pTemplChar;
#ifdef _TIMER_COUNT
		timer.Relay();
#endif
		if(checkfeature==1)
		{
			pMatch->matchcoef.original=pMatch->matchcoef.weighting=1.0;	//确定符合特征
			break;
		}
		else //if(checkfeature==0)
		{
			PRESET_ARRAY8<CImageChar*> tmplchars;
			int charcount=GetRecogTmplChars(pTemplChar,tmplchars.Data(),8);
			tmplchars.ReSize(charcount);
			pTemplChar=tmplchars[0];
			pMatch->matchcoef=pTemplChar->CalMatchCoef(pSafeText,&pMatch->ciOffsetX,&pMatch->ciOffsetY);
			MATCHCOEF mincoef(minMatchCoef,minMatchCoef);
			if(pMatch->matchcoef.CompareWith(mincoef)<0)
			{
				for(WORD j=1;j<tmplchars.Count;j++)
				{
					CImageChar* pChildTmplChar=tmplchars.At(j);
					MATCHCOEF matchcoef=pChildTmplChar->CalMatchCoef(pSafeText,&pMatch->ciOffsetX,&pMatch->ciOffsetY);
					if(pMatch->matchcoef.CompareWith(matchcoef)<0)
					{
						pMatch->matchcoef=matchcoef;
						pMatch->pTemplChar=pChildTmplChar;
					}
					if(pMachCoef->CompareWith(mincoef)>=0)
						break;	//已找到合适字体的匹配字符
				}
			}
			pMatch->matchcoef.weighting-=deductcoef;
		}
#ifdef _TIMER_COUNT
		timer.Relay(40224);
#endif
		if(bRaiseMatchCoef)
		{	//可能因彩图转黑白时丢失特征导致特征匹配失败，此时如果原始匹配系数较大，依然可以视同成功 wjh-2018.3.30
			if( pSafeText->wChar.wHz=='0'&&pMatch->matchcoef.original>=0.8||(
				pSafeText->m_nBlckPixels<pTemplChar->m_nBlckPixels&&pMatch->matchcoef.weighting>=0.9))
				continue;	//该字符匹配度较高应该可以不受字符典型特征限制
			else
				listMatches.DeleteCursor();
		}
	}
	//
	MATCHCOEF maxcoef(minMatchCoef,minMatchCoef),maxcoef2(0.2,0.2);
	pText->wChar.wHz=0;
	MATCHCHAR* pLastMatch=NULL,*pMostMatch=NULL;
	for(MATCHCHAR* pMatch=listMatches.EnumObjectFirst();pMatch;pMatch=listMatches.EnumObjectNext())
	{
		if(pMatch->matchcoef.CompareWith(maxcoef)>0)
		{
			maxcoef=pMatch->matchcoef;
			pText->wChar=pMatch->wChar;
			pLastMatch=pMatch;
		}
		//取最匹配的一项，避免丢字符 wht 18-07-14
		if(pText->wChar.wHz==0&&pMatch->matchcoef.CompareWith(maxcoef2)>0)
		{
			maxcoef2=pMatch->matchcoef;
			pMostMatch=pMatch;
		}
	}
	if(pLastMatch==NULL&&pMostMatch)
	{
		pText->wChar=pMostMatch->wChar;
		pLastMatch=pMostMatch;
	}
	
	if(pText->wChar.wHz==0&&pCharPoint&&pCharPoint->m_bTemplEnabled)
	{
		pText->wChar.wHz='.';
		if(pText->CheckFeatures(NULL)!=1)
			pText->wChar.wHz=0;
	}
#ifdef _DEBUG
	if(CImageDataRegion::EXPORT_CHAR_RECOGNITION&&pText->wChar.wHz>0&&pLastMatch&&pLastMatch->pTemplChar)
	{
		CImageChar* pTempText=pLastMatch->pTemplChar;
		char wch[3]={pTempText->wChar.chars[0],pTempText->wChar.chars[1],0};
		CXhChar50 charname("D:\\Recog\\%s",wch);
		if(pTempText->FontSerial>0)
			charname.Append(CXhChar16("F%d",pTempText->FontSerial),'#');
		charname.Append(CXhChar16("%.2f.txt",pLastMatch->matchcoef.weighting),'#');
		CLogFile logchar(charname);
		CLogErrorLife life(&logchar);
		int i,j,blackcount=0,blackcount1=0,blackcount2=0;
		pSafeText=pLastMatch->bReExtended?&safetext:pText;
		for(j=0;j<pTempText->m_nMaxTextH;j++)
		{
			for(i=0;i<pTempText->m_nMaxTextH;i++)
			{
				bool black1=pTempText->IsBlackPixel(i,j);
				bool black2=pSafeText->IsBlackPixel(i-pLastMatch->ciOffsetX,j-pLastMatch->ciOffsetY);
				if(black1)
					blackcount1++;
				if(black2)
					blackcount2++;
				if(black1&&black2)
				{
					blackcount++;
					logchar.LogString("*",false);
				}
				else if(black1)
					logchar.LogString("1",false);
				else if(black2)
					logchar.LogString("2",false);
				else
					logchar.LogString(" ",false);
			}
			logchar.LogString("|",false);
			for(i=0;i<pTempText->m_nMaxTextH;i++)
			{
				bool black2=pSafeText->IsBlackPixel(i,j);
				if(black2)
					logchar.LogString("2",false);
				else
					logchar.LogString(" ",false);
			}
			logchar.LogString("|",false);
			for(int i=0;i<pTempText->m_nMaxTextH;i++)
			{
				bool black1=pTempText->IsBlackPixel(i,j);
				if(black1)
					logchar.LogString("1",false);
				else
					logchar.LogString(" ",false);
			}
			logchar.LogString("\n");
		}
		CXhChar50 countstr("%4d",blackcount);
		countstr.ResizeLength(pTempText->m_nMaxTextH);
		logchar.LogString(countstr,false);
		logchar.LogString("|",false);
		countstr.Printf("%4d",blackcount2);
		countstr.ResizeLength(pTempText->m_nMaxTextH);
		logchar.LogString(countstr,false);
		logchar.LogString("|",false);
		countstr.Printf("%4d(std char)",blackcount1);
		countstr.ResizeLength(pTempText->m_nMaxTextH);
		logchar.LogString(countstr,false);
		life.EnableShowScreen(false);
	}
#endif
	if(pText->wChar.wHz>0&&pMachCoef)
		*pMachCoef=maxcoef;
	if(pLastMatch&&pLastMatch->pTemplChar)
		PracticeFontRecogChance(pLastMatch->pTemplChar->FontSerial,maxcoef.weighting);
	return pText->wChar.wHz>0;
}
BOOL CAlphabets::EarlyRecognizeChar(CImageChar* pText,double minMatchCoef/*=0.0*/,MATCHCOEF* pMachCoef/*=NULL*/,CImageCellRegion *pCellRegion/*=NULL*/,int iStartPos/*=0*/)
{
	bool needextend=false;
	MATCHCHAR* pMatch=NULL;
	CXhSimpleList<MATCHCHAR> listMatches;
	CXhSimpleList<CImageChar::ISLAND> islands;
	CImageChar safetext,*pSafeText=pText;
	if(minMatchCoef==0)
		minMatchCoef=LowestRecogSimilarity();
	const int HEIGHT_DIFF=3;
	for(CImageChar* pTemplChar=listChars.GetFirst();pTemplChar;pTemplChar=listChars.GetNext())
	{
		if( !pTemplChar->m_bTemplEnabled)
			continue;
		RetrieveImageTextByTemplChar(safetext,pTemplChar,pCellRegion,iStartPos);
		pSafeText=&safetext;
		pSafeText->DetectIslands(&islands);
		pSafeText->wChar=pTemplChar->wChar;
		BOOL bCheckFeature=!IsPreSplitChar();
		int checkfeature=bCheckFeature?pSafeText->CheckFeatures(&islands):0;
		bool bRaiseMatchCoef=(checkfeature==-2);	//如'0'字符存在局部像素缺失导致无孤岛情况存在
		if(checkfeature==-1)
			continue;	//确定不符合特征
		pMatch=listMatches.AttachObject();
		pMatch->bReExtended=(pSafeText!=pText);
		pMatch->wChar=pTemplChar->wChar;
		pMatch->pTemplChar=pTemplChar;
		if(checkfeature==1)
		{
			pMatch->matchcoef.original=pMatch->matchcoef.weighting=1.0;	//确定符合特征
			break;
		}
		else //if(checkfeature==0)
		{
			pMatch->matchcoef=pTemplChar->CalMatchCoef(pSafeText,&pMatch->ciOffsetX,&pMatch->ciOffsetY);
			if(pMatch->matchcoef.original>0.7&&pMatch->matchcoef.weighting>0.9)
				break;
		}
		if(bRaiseMatchCoef)
		{	//可能因彩图转黑白时丢失特征导致特征匹配失败，此时如果原始匹配系数较大，依然可以视同成功 wjh-2018.3.30
			if( pSafeText->wChar.wHz=='0'&&pMatch->matchcoef.original>=0.8||(
				pSafeText->m_nBlckPixels<pTemplChar->m_nBlckPixels&&pMatch->matchcoef.weighting>=0.9))
				continue;	//该字符匹配度较高应该可以不受字符典型特征限制
			else
				listMatches.DeleteCursor();
		}
	}
	//
	MATCHCOEF maxcoef(minMatchCoef,minMatchCoef);
	pText->wChar.wHz=0;
	MATCHCHAR* pLastMatch=NULL;
	for(MATCHCHAR* pMatch=listMatches.EnumObjectFirst();pMatch;pMatch=listMatches.EnumObjectNext())
	{
		if(pMatch->matchcoef.CompareWith(maxcoef)>0)
		{
			maxcoef=pMatch->matchcoef;
			pText->wChar=pMatch->wChar;
			pLastMatch=pMatch;
		}
	}
	
	if(pText->wChar.wHz==0&&pCharPoint&&pCharPoint->m_bTemplEnabled)
	{
		pText->wChar.wHz='.';
		if(pText->CheckFeatures(NULL)!=1)
			pText->wChar.wHz=0;
	}
	if(pText->wChar.wHz>0&&pMachCoef)
		*pMachCoef=maxcoef;
	return pText->wChar.wHz>0;
}
