#include <stdafx.h>
#include "math.h"
#include "ImageFile.h"
#include "ximajpg.h"
#include "ximapng.h"
#ifdef _SUPPORT_TIFF_IMG_
#include "ximatif.h"
#endif
#include "ImageTransform.h"
#include "HashTable.h"
#include "LogFile.h"
#include "ArrayList.h"
#include "Robot.h"
#include "PdfEngine.h"

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
extern CTimerCount timer;
#endif
#endif

#define      Pi               3.14159265358979323846
#define      RADTODEG_COEF    0.01745329251994329577
#define      DEGTORAD_COEF    57.2957795130823208768
//////////////////////////////////////////////////////////////////////////
//IMGROT
#include <math.h>
static int f2i(double fval)
{
	if(fval>0)
		return (int)(fval+0.5);
	else if(fval==0)
		return 0;
	else
		return (int)(fval-0.5);
}
IMGROT::IMGROT(int xiOrg,int yjOrg,double dfEdgeWidthX,double yjRotPixels)
{
	Init(xiOrg,yjOrg,dfEdgeWidthX,yjRotPixels);
}
void IMGROT::Init(int xiOrg,int yjOrg,double dfEdgeWidthX,double yjRotPixels)
{
	xOrg.x=xiOrg;
	xOrg.y=yjOrg;
	if(dfEdgeWidthX<=0)
	{
		cosa=1;
		sina=0;
	}
	else
	{
		double gl=sqrt((double)(dfEdgeWidthX*dfEdgeWidthX+yjRotPixels*yjRotPixels));
		cosa=dfEdgeWidthX/gl;
		sina=yjRotPixels/gl;
	}
}
POINT IMGROT::Rotate(const POINT& point,bool inverse/*=false*/) const
{
	return Rotate(point.x,point.y,inverse);
}
POINT IMGROT::Rotate(int xiPoint,int yjPoint,bool inverse/*=false*/) const
{
	POINT pt;
	int dx=xiPoint-xOrg.x;
	int dy=yjPoint-xOrg.y;
	if(!inverse)
	{
		pt.x=xOrg.x+f2i(dx*cosa-dy*sina);
		pt.y=xOrg.y+f2i(dy*cosa+dx*sina);
	}
	else
	{
		pt.x=xOrg.x+f2i(dx*cosa+dy*sina);
		pt.y=xOrg.y+f2i(dy*cosa-dx*sina);
	}
	return pt;
}
//////////////////////////////////////////////////////////////////////////
//CSamplePixelLine
CSamplePixelLine::CSamplePixelLine()
{

}
CSamplePixelLine::~CSamplePixelLine()
{

}

bool CSamplePixelLine::AnalyzeLinePixels(bool* pixels,int nNum)
{
	bool prevGrayness = false;	//记录前一象素点颜色
	int maxPixelsOfValidSeg=30;	//有效黑色线段的最多象素数
	int minPixelsOfValidSeg=0;	//有效黑色线段的最少象素数

	m_nValidBlackSeg=0;
	for(int i = 0;i<nNum;i++)
	{
		if((prevGrayness&&!pixels[i]) || (prevGrayness&&pixels[i]&&i==nNum-1))
		{	//前一点为黑点当前点为白点，意味一个黑色线段的结束
			int seg_len=i-shiftSegArr[m_nValidBlackSeg-1][0];
			if(seg_len>maxPixelsOfValidSeg||seg_len<minPixelsOfValidSeg)
				m_nValidBlackSeg--;	//黑色线段象素数太多或太少，为不合理黑色线段，不进行统计回滚
			else
				shiftSegArr[m_nValidBlackSeg-1][1]=i-1;
		}
		else if(!prevGrayness && pixels[i])
		{	//前一点为白点当前点为黑点，意味一个黑色线段的开始
			m_nValidBlackSeg++;
			if(m_nValidBlackSeg>5)
				break;	//系统最多只检测靠前的５个黑色线段
			shiftSegArr[m_nValidBlackSeg-1][0]=i;
			if(m_nValidBlackSeg>5)
			{
				m_nValidBlackSeg=5;
				break;
			}
		}
		prevGrayness=pixels[i];
	}
	return m_nValidBlackSeg>0;
}
bool CSamplePixelLine::
	FindBlackSegInSpecScope(int scope_start,int scope_end,int* seg_start, int* seg_end)
{
	for(int i=0;i<m_nValidBlackSeg;i++)
	{
		if(shiftSegArr[i][1]<scope_start)
			continue;	//当前黑色线段在检测范围之前
		else if(shiftSegArr[i][0]>scope_end)
			continue;	//当前黑色线段在检测范围之后
		else			//当前黑色线段与检测范围有重叠区域
		{
			if(seg_start)
				*seg_start=shiftSegArr[i][0];
			if(seg_end)
				*seg_end=shiftSegArr[i][1];
			return true;
		}
	}
	return false;
}
//////////////////////////////////////////////////////////////////////////
//CImageTransForm
CImageTransform::CImageTransform()
{
	//m_lpBMIH=NULL;
	m_ciRawImageFileType=0;
	m_uiExterRawBitsuffSize=0;
	m_bInternalRawBitsBuff=true;
	m_lpRawBits=m_lpGrayBitsMap=m_lpPixelGrayThreshold=NULL;
	m_nWidth=m_nEffByteWidth=m_nHeight=0;
	m_ubGreynessThreshold=85;
	m_nRectColNum=m_nRectRowNum=0;
	m_nRectWidth=m_nRectHeight=0;
	m_bExternalPdfEngine=FALSE;
	m_pPdfEngine=NULL;
	m_nMonoForward=20;
}

CImageTransform::~CImageTransform()
{
	if(m_lpRawBits&&m_bInternalRawBitsBuff)
		delete []m_lpRawBits;
	if(m_lpGrayBitsMap)
		delete []m_lpGrayBitsMap;
	if(m_lpPixelGrayThreshold)
		delete []m_lpPixelGrayThreshold;
	if(m_pPdfEngine&&!m_bExternalPdfEngine)
		delete m_pPdfEngine;
}
void CImageTransform::Clear()
{
	if(m_lpRawBits&&m_bInternalRawBitsBuff)
		delete []m_lpRawBits;
	if(m_lpGrayBitsMap)
		delete []m_lpGrayBitsMap;
	if(m_lpPixelGrayThreshold)
		delete []m_lpPixelGrayThreshold;
	m_bInternalRawBitsBuff=false;
	m_lpRawBits=m_lpGrayBitsMap=m_lpPixelGrayThreshold=NULL;
	m_nWidth=m_nEffByteWidth=m_nHeight=0;
	m_ubGreynessThreshold=85;
	m_nRectColNum=m_nRectRowNum=0;
	m_nRectWidth=m_nRectHeight=0;
}
void CImageTransform::UnloadRawBytesToVirtualMemBuff(BUFFER_IO* pIO,bool write2vm/*=true*/)
{
	pIO->SeekToBegin();
	UINT uiRawImgSize=m_nEffByteWidth*m_nHeight;
	if(m_lpRawBits!=NULL)
	{
		if(write2vm)
		{
			pIO->WriteInteger(uiRawImgSize);
			pIO->Write(m_lpRawBits,uiRawImgSize);
		}
		delete[] m_lpRawBits;
		m_lpRawBits=NULL;
	}
	else
		pIO->WriteInteger(0);
}
bool CImageTransform::LoadRawBytesFromVirtualMemBuff(BUFFER_IO* pIO)
{
	pIO->SeekToBegin();
	UINT uiReadRawImgSize=0,uiRawImgSize=m_nEffByteWidth*m_nHeight;
	pIO->ReadInteger(&uiReadRawImgSize);
	if(uiReadRawImgSize!=uiRawImgSize||uiRawImgSize==0)
		return false;
	if(m_lpRawBits==NULL&&uiReadRawImgSize>0)
		m_lpRawBits=new BYTE[uiReadRawImgSize];
	if(m_lpRawBits)
		pIO->Read(m_lpRawBits,uiReadRawImgSize);
	return true;
}
void CImageTransform::UnloadGreyBytesToVirtualMemBuff(BUFFER_IO* pIO,bool write2vm/*=true*/)
{
	pIO->SeekToBegin();
	UINT uiGreyImgSize=m_nWidth*m_nHeight;
	if(m_lpGrayBitsMap!=NULL)
	{
		if(write2vm)
		{
			pIO->WriteInteger(uiGreyImgSize);
			pIO->Write(m_lpGrayBitsMap,uiGreyImgSize);
		}
		delete[] m_lpGrayBitsMap;
		m_lpGrayBitsMap=NULL;
	}
	else
		pIO->WriteInteger(0);
	if(m_lpPixelGrayThreshold!=NULL)
	{
		if(write2vm)
		{
			pIO->WriteInteger(uiGreyImgSize);
			pIO->Write(m_lpPixelGrayThreshold,uiGreyImgSize);
		}
		delete[] m_lpPixelGrayThreshold;
		m_lpPixelGrayThreshold=NULL;
	}
	else
		pIO->WriteInteger(0);
}
bool CImageTransform::LoadGreyBytesFromVirtualMemBuff(BUFFER_IO* pIO)
{
	pIO->SeekToBegin();
	UINT uiReadGreyImgSize=0,uiGreyImgSize=m_nWidth*m_nHeight;
	pIO->ReadInteger(&uiReadGreyImgSize);
	if(uiReadGreyImgSize!=uiGreyImgSize||uiReadGreyImgSize==0)
		return false;
	if(m_lpGrayBitsMap==NULL&&uiReadGreyImgSize>0)
		m_lpGrayBitsMap=new BYTE[uiGreyImgSize];
	if(m_lpGrayBitsMap)
		pIO->Read(m_lpGrayBitsMap,uiGreyImgSize);

	pIO->ReadInteger(&uiReadGreyImgSize);
	if(uiReadGreyImgSize!=uiGreyImgSize)
		return false;
	if(m_lpPixelGrayThreshold==NULL&&uiReadGreyImgSize>0)
		m_lpPixelGrayThreshold=new BYTE[uiGreyImgSize];
	if(m_lpPixelGrayThreshold)
		pIO->Read(m_lpPixelGrayThreshold,uiGreyImgSize);
	return true;
}
void CImageTransform::ReleaseRawImageBits()
{
	if(m_lpRawBits&&m_bInternalRawBitsBuff)
	{
		delete []m_lpRawBits;
		m_lpRawBits=NULL;
	}
	else
		m_lpRawBits=NULL;
}
void CImageTransform::ReleaseGreyImageBits()
{
	if(m_lpGrayBitsMap!=NULL)
	{
		delete[] m_lpGrayBitsMap;
		m_lpGrayBitsMap=NULL;
	}
	if(m_lpPixelGrayThreshold!=NULL)
	{
		delete[] m_lpPixelGrayThreshold;
		m_lpPixelGrayThreshold=NULL;
	}
}
UINT CImageTransform::GetInternalAllocRawImageBytes()	//获取内部分配给m_lpRawBits内存大小
{
	UINT uiInternalAllocSize=0;
	if(m_lpRawBits!=NULL&&m_bInternalRawBitsBuff)
		uiInternalAllocSize+=m_nEffByteWidth*m_nHeight;
	if(m_lpGrayBitsMap!=NULL)
		uiInternalAllocSize+=m_nWidth*m_nHeight;
	if(m_lpGrayBitsMap!=NULL)
		uiInternalAllocSize+=m_nWidth*m_nHeight;
	return uiInternalAllocSize;
}
UINT CImageTransform::get_uiExterRawBitsBuffSize()
{
	return m_bInternalRawBitsBuff?0:uiExterRawBitsBuffSize;
}
//BMP格式图片读写
//static BOOL GetBitMap(const char* bmp_file,CBitmap& xBitMap)
DWORD GetSingleWord(int serial)	//serial is 1 based index 
{
	DWORD constDwordArr[32]={
		0x00000001,0x00000002,0x00000004,0x00000008,0x00000010,0x00000020,0x00000040,0x00000080,
		0x00000100,0x00000200,0x00000400,0x00000800,0x00001000,0x00002000,0x00004000,0x00008000,
		0x00010000,0x00020000,0x00040000,0x00080000,0x00100000,0x00200000,0x00400000,0x00800000,
		0x01000000,0x02000000,0x04000000,0x08000000,0x10000000,0x20000000,0x40000000,0x80000000};
	return constDwordArr[serial-1];
}
void CImageTransform::SetBitMap(BITMAP& bmp)
{
	//m_nWidth=m_nEffByteWidth=m_nHeight=0;
	//m_ubGreynessThreshold=85;
	//m_lpPixelGrayThreshold=NULL;
	//m_nRectColNum=m_nRectRowNum=0;
	//m_nRectWidth=m_nRectHeight=0;
	Clear();
	if(bmp.bmBitsPixel==8)	//灰度图
	{
		m_nWidth=bmp.bmWidth;			//图片的宽度和高度(像素单位)
		m_nHeight=bmp.bmHeight;			//图片的宽度和高度(像素单位)
		m_nEffByteWidth=((m_nWidth*3+3)/4)*4;
		ULONG count=bmp.bmWidth*bmp.bmHeight;
		m_lpGrayBitsMap=new BYTE[count];
		memcpy(m_lpGrayBitsMap,bmp.bmBits,count);
	}
	else if(bmp.bmBitsPixel>=24)//彩色图
	{

	}
	//else if(bmp.bmBitsPixel==1)	//黑白图
	//	;
}
bool CImageTransform::ReadBmpFile(FILE* fp,BYTE* lpExterRawBitsBuff/*=NULL*/,UINT uiBitsBuffSize/*=0*/)
{
	CTempFileBuffer tempBuffer(fp);
	bool bRetCode=ReadBmpFileByTempFile(tempBuffer,lpExterRawBitsBuff,uiBitsBuffSize);
	tempBuffer.CloseFile();
	return bRetCode;
}
bool CImageTransform::ReadBmpFileByTempFile(CTempFileBuffer &tempBuffer,BYTE* lpExterRawBitsBuff/*=NULL*/,UINT uiBitsBuffSize/*=0*/)
{
	//if(fp==NULL)
	//	return false;
	BITMAPFILEHEADER fileHeader;
	BITMAPINFOHEADER infoHeader;
	tempBuffer.SeekToBegin();
	tempBuffer.Read(&fileHeader,sizeof(BITMAPFILEHEADER));
	tempBuffer.Read(&infoHeader,sizeof(BITMAPINFOHEADER));
	//fread(&fileHeader,1,sizeof(BITMAPFILEHEADER),fp);
	//fread(&infoHeader,1,sizeof(BITMAPINFOHEADER),fp);
	int i,nPalette = infoHeader.biClrUsed;
	if(nPalette==0&&infoHeader.biBitCount<=8)
		nPalette=GetSingleWord(infoHeader.biBitCount+1);
	DYN_ARRAY<RGBQUAD> palette(nPalette);
	for(i=0;i<nPalette;i++)
	{
		tempBuffer.Read(&palette[i].rgbBlue,1);
		tempBuffer.Read(&palette[i].rgbGreen,1);
		tempBuffer.Read(&palette[i].rgbRed,1);
		tempBuffer.Read(&palette[i].rgbReserved,1);
		//fread(&palette[i].rgbBlue,1,1,fp);
		//fread(&palette[i].rgbGreen,1,1,fp);
		//fread(&palette[i].rgbRed,1,1,fp);
		//fread(&palette[i].rgbReserved,1,1,fp);
	}
	BITMAP image;
	image.bmType=0;
	image.bmPlanes=1;
	image.bmWidth=infoHeader.biWidth;
	image.bmBitsPixel=infoHeader.biBitCount;
	image.bmHeight=infoHeader.biHeight;
	int widthBits = infoHeader.biWidth*infoHeader.biBitCount;
	image.bmWidthBytes=((widthBits+31)/32)*4;
	if(lpExterRawBitsBuff!=NULL&&(long)uiBitsBuffSize<infoHeader.biHeight*image.bmWidthBytes)
	{
		//fclose(fp);
		return false;
	}
	BYTE_ARRAY fileBytes(infoHeader.biHeight*image.bmWidthBytes);
	image.bmBits = fileBytes;
	//fread(image.bmBits,1,infoHeader.biHeight*image.bmWidthBytes,fp);
	tempBuffer.Read(image.bmBits,infoHeader.biHeight*image.bmWidthBytes);
	//fclose(fp);
	return InitFromBITMAP(image,palette,(BYTE*)image.bmBits,lpExterRawBitsBuff,uiExterRawBitsBuffSize);
}
bool CImageTransform::InitFromBITMAP(BITMAP &image, RGBQUAD *pPalette/*=NULL*/, BYTE *lpBmBits/*=NULL*/, BYTE* lpExterRawBitsBuff/*=NULL*/,UINT uiBitsBuffSize/*=0*/)
{
	//填写图像数据
	m_nWidth=image.bmWidth;
	m_nHeight=image.bmHeight;
	this->m_uBitcount=24;
	m_nEffByteWidth=((m_nWidth*3+3)/4)*4;
	DWORD dwCount=m_nEffByteWidth*m_nHeight;
	//获取原始图像位图内存池m_lpRawBits
	ReleaseRawImageBits();
	if(lpExterRawBitsBuff&&uiBitsBuffSize>dwCount)
	{
		m_lpRawBits=lpExterRawBitsBuff;
		m_uiExterRawBitsuffSize=uiBitsBuffSize;
		m_bInternalRawBitsBuff=false;
	}
	else
	{
		m_lpRawBits=new BYTE[dwCount];
		m_bInternalRawBitsBuff=true;
	}
	//
	RGBQUAD crPixelColor;
	crPixelColor.rgbReserved=0;
	BYTE byteConstArr[8]={0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};
	if(lpBmBits==NULL)
		lpBmBits=(BYTE*)image.bmBits;
	if((image.bmBitsPixel<24&&pPalette==NULL)||(image.bmBitsPixel<24&&lpBmBits==NULL))
		return false;
	for(int rowi=0;rowi<image.bmHeight;rowi++)
	{
		int row=image.bmHeight-rowi-1;
		for(int i=0;i<image.bmWidth;i++)
		{
			int colorindex=0;
			RGBQUAD crPixelColor;
			crPixelColor.rgbReserved=0;
			if(image.bmBitsPixel==1)
			{	//单色图
				int ibyte=row*image.bmWidthBytes+i/8;
				if (lpBmBits[ibyte]&byteConstArr[i%8])
					crPixelColor=pPalette[1];//crPixelColor.rgbBlue=crPixelColor.rgbGreen=crPixelColor.rgbRed=255;
				else
					crPixelColor=pPalette[0];
			}
			else if(image.bmBitsPixel==4)
			{	//16色图
				int ibyte=row*image.bmWidthBytes+i/2;
				if(i%2==0)
					colorindex=(lpBmBits[ibyte]&0xf0)>>4;
				else
					colorindex=lpBmBits[ibyte]&0x0f;
				crPixelColor=pPalette[colorindex];
			}
			else if(image.bmBitsPixel==8)
			{	//256色图
				int ibyte=row*image.bmWidthBytes+i;
				colorindex=lpBmBits[ibyte];
				crPixelColor=pPalette[colorindex];
			}
			else if(image.bmBitsPixel==24||image.bmBitsPixel==32)
			{	//真彩24位或32位
				BYTE ciPixelBytes=image.bmBitsPixel/8;
				int ibyte=row*image.bmWidthBytes+i*ciPixelBytes;
				memcpy(&crPixelColor,&lpBmBits[ibyte],ciPixelBytes);
			}
			memcpy(&m_lpRawBits[rowi*m_nEffByteWidth+i*3],&crPixelColor,3);
		}
	}
	return true;
}
bool CImageTransform::WriteBmpFile(FILE* fp)
{
	BITMAPINFOHEADER bmpInfoHeader;
	BITMAPFILEHEADER bmpFileHeader;
	unsigned int width=GetWidth();
	unsigned int height=GetHeight();
	unsigned int row, column;
	unsigned int extrabytes, bytesize;
	unsigned char *paddedImage = NULL, *paddedImagePtr, *imagePtr;
	BYTE* image=GetRawBits();

	/* The .bmp format requires that the image data is aligned on a 4 byte boundary.  For 24 - bit bitmaps,
	   this means that the width of the bitmap  * 3 must be a multiple of 4. This code determines
	   the extra padding needed to meet this requirement. */
	extrabytes = (4 - (width * 3) % 4) % 4;

	// This is the size of the padded bitmap
	bytesize = (width * 3 + extrabytes) * height;

	// Fill the bitmap file header structure
	bmpFileHeader.bfType = 'MB';   // Bitmap header
	bmpFileHeader.bfSize = 0;      // This can be 0 for BI_RGB bitmaps
	bmpFileHeader.bfReserved1 = 0;
	bmpFileHeader.bfReserved2 = 0;
	bmpFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	// Fill the bitmap info structure
	bmpInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpInfoHeader.biWidth = width;
	bmpInfoHeader.biHeight = height;
	bmpInfoHeader.biPlanes = 1;
	bmpInfoHeader.biBitCount = 24;            // 8 - bit bitmap
	bmpInfoHeader.biCompression = BI_RGB;
	bmpInfoHeader.biSizeImage = bytesize;     // includes padding for 4 byte alignment
	bmpInfoHeader.biXPelsPerMeter = 2952;
	bmpInfoHeader.biYPelsPerMeter = 2952;
	bmpInfoHeader.biClrUsed = 0;
	bmpInfoHeader.biClrImportant = 0;

	// Write bmp file header
	if (fwrite(&bmpFileHeader, 1, sizeof(BITMAPFILEHEADER), fp) < sizeof(BITMAPFILEHEADER)) {
		//TODO:AfxMessageBox("Error writing bitmap file header");
		return false;
	}

	// Write bmp info header
	if (fwrite(&bmpInfoHeader, 1, sizeof(BITMAPINFOHEADER), fp) < sizeof(BITMAPINFOHEADER)) {
		//TODO:AfxMessageBox("Error writing bitmap info header");
		return false;
	}

	// Allocate memory for some temporary storage
	paddedImage = (unsigned char *)calloc(sizeof(unsigned char), bytesize);
	if (paddedImage == NULL) {
		//TODO:AfxMessageBox("Error allocating memory");
		return false;
	}

	/*	This code does three things.  First, it flips the image data upside down, as the .bmp
		format requires an upside down image.  Second, it pads the image data with extrabytes 
		number of bytes so that the width in bytes of the image data that is written to the
		file is a multiple of 4.  Finally, it swaps (r, g, b) for (b, g, r).  This is another
		quirk of the .bmp file format. */
	for (row = 0; row < height; row++) {
		//imagePtr = image + (height - 1 - row) * width * 3;
		imagePtr = image + row * m_nEffByteWidth;
		paddedImagePtr = paddedImage + row * (width * 3 + extrabytes);
		for (column = 0; column < width; column++) {
			*paddedImagePtr = *(imagePtr + 2);
			*(paddedImagePtr + 1) = *(imagePtr + 1);
			*(paddedImagePtr + 2) = *(imagePtr);
			imagePtr += 3;
			paddedImagePtr += 3;
		}
	}

	// Write bmp data
	if (fwrite(paddedImage, 1, bytesize, fp) < bytesize) {
		//TODO:AfxMessageBox("Error writing bitmap data");
		free(paddedImage);
		return false;
	}

	// Close file
	free(paddedImage);
	return true;
}
bool CImageTransform::ReadImageFileHeader(FILE* fp,char ciBmp0Jpeg1Png2tif3)
{
	BYTE exterbyte=0;	//仅获取文件头，不需要实际分配空间
	if(ciBmp0Jpeg1Png2tif3==0)
		return ReadBmpFile(fp,&exterbyte,1);
	CImageFileJpeg jpgFile;
	CImageFilePng  pngFile;
#ifdef _SUPPORT_TIFF_IMG_
	CImageFileTif  tifFile;
#endif
	CImageFile* pImageFile=&pngFile;
	if(ciBmp0Jpeg1Png2tif3==1)
		pImageFile=&jpgFile;
#ifdef _SUPPORT_TIFF_IMG_
	else if (ciBmp0Jpeg1Png2tif3==3)
		pImageFile=&tifFile;
#endif
	pImageFile->ReadImageFile(fp,&exterbyte,1);
	m_nWidth=pImageFile->GetWidth();
	m_nEffByteWidth=pImageFile->GetEffWidth();
	m_nHeight=pImageFile->GetHeight();
	m_uBitcount=pImageFile->GetBpp();
	return true;
}
//JPG格式图片的读写
bool CImageTransform::ReadImageFile(FILE* fp,char ciBmp0Jpeg1Png2Tif3,BYTE* lpExterRawBitsBuff/*=NULL*/,UINT uiBitsBuffSize/*=0*/,
			int nMonoForward/*=20*/)
{
	if(ciBmp0Jpeg1Png2Tif3==0)
	{
		if(!ReadBmpFile(fp,lpExterRawBitsBuff,uiBitsBuffSize))
			return false;
		return UpdateGreyImageBits(true,nMonoForward);
	}
	CImageFileJpeg jpgFile;
	CImageFilePng  pngFile;
#ifdef _SUPPORT_TIFF_IMG_
	CImageFileTif  tifFile;
#endif
	CImageFile* pImageFile=&pngFile;
	if (ciBmp0Jpeg1Png2Tif3 == 1)
		pImageFile = &jpgFile;
	else if (ciBmp0Jpeg1Png2Tif3 == 2)
		pImageFile = &pngFile;
#ifdef _SUPPORT_TIFF_IMG_
	else if (ciBmp0Jpeg1Png2Tif3 == 3)
		pImageFile = &tifFile;
#endif
	else
		return false;
	bool readimgdata=pImageFile->ReadImageFile(fp,lpExterRawBitsBuff,uiBitsBuffSize);
	
	m_nWidth = pImageFile->GetWidth();
	m_nEffByteWidth = pImageFile->GetEffWidth();
	m_nHeight = pImageFile->GetHeight();
	m_uBitcount = pImageFile->GetBpp();
	/*
	m_nWidth=pImageFile->GetWidth();
	m_nEffByteWidth= m_nWidth*3+(4-(m_nWidth*3)%4);
	m_nHeight=pImageFile->GetHeight();
	m_uBitcount=24;*/
	if(!readimgdata)
		return false;
	DWORD dwCount=m_nEffByteWidth*m_nHeight;
	//获取原始图像位图内存池m_lpRawBits
	ReleaseRawImageBits();
	if(lpExterRawBitsBuff&&uiBitsBuffSize>dwCount)
	{
		m_lpRawBits=lpExterRawBitsBuff;
		m_uiExterRawBitsuffSize=uiBitsBuffSize;
		m_bInternalRawBitsBuff=false;
	}
	else
	{
		m_lpRawBits=new BYTE[dwCount];
		m_bInternalRawBitsBuff=true;
	}
	/*if (pImageFile->GetBpp() == 1)
	{
		const BYTE xarrConstBitBytes[8] = { 0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80 };
		for (int row = 0; row < m_nHeight; row++)
		{
			BYTE* pFrom = pImageFile->GetBits(m_nHeight - row - 1);	//info.pImage
			BYTE* pTo = m_lpRawBits+row* m_nEffByteWidth;
			for (int xi = 0; xi < m_nWidth; xi++)
			{
				int ibyte = xi / 8, ibit = xi % 8;
				BYTE cbByte = *(pFrom + ibyte);
				if (cbByte&xarrConstBitBytes[ibit])	//通过异或操作得出dwFlag的补码
					*(pTo + 2) = *(pTo + 1) = *pTo = 255;
				else
					*(pTo + 2) = *(pTo + 1) = *pTo = 0;
				pTo += 3;
			}
		}
	}
	else*/
	{	//Jpeg与Png格式的默认行扫描模式为自下至上upward,应统一为自上至下downward模式
		for (int i = 0; i < m_nHeight; i++)
		{
			int rowi = pImageFile->head.biHeight - 1 - i;
			BYTE* pImgRowData = pImageFile->GetBits(i);
			memcpy(&m_lpRawBits[rowi*m_nEffByteWidth], pImgRowData, m_nEffByteWidth);
		}
	}
	//
	return UpdateGreyImageBits(true,nMonoForward);
}
//GB2312到UTF-8的转换,utf8需要在调用处释放
int G2U(const char* gb2312,char* utf8_str)
{
	int len = MultiByteToWideChar(CP_ACP, 0, gb2312, -1, NULL, 0);
	if(len<=0)
		return 0;
	wchar_t* wstr = new wchar_t[len+1];
	memset(wstr, 0, len+1);
	MultiByteToWideChar(CP_ACP, 0, gb2312, -1, wstr, len);
	len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
	if(len>0)
	{
		char* utf8 = new char[len+1];
		memset(utf8, 0, len+1);
		WideCharToMultiByte(CP_UTF8, 0, wstr, -1, utf8, len, NULL, NULL);
		if(utf8_str!=NULL)
			strcpy(utf8_str,utf8);
		if(utf8!=NULL)	
			delete[] utf8;
	}
	if(wstr) 
		delete[] wstr;
	return len;
}
#include "WinUtil.h"
bool CImageTransform::ReadPdfFile(char* file_path,const PDF_FILE_CONFIG& pdfConfig,int nMonoForward/*=20*/,RectD *pRect/*=NULL*/,BOOL bInitGrayBitMap/*=FALSE*/)
{
	int page_no=pdfConfig.page_no;
	int rotation=pdfConfig.rotation;
	double zoom_scale=pdfConfig.zoom_scale;
	double min_line_weight=pdfConfig.min_line_weight;
	double min_line_flatness=pdfConfig.min_line_flatness;
	double max_line_weight=pdfConfig.max_line_weight;
	double max_line_flatness=pdfConfig.max_line_flatness;
	PdfEngine::SetPdfStrokeLineMinWeight(min_line_weight);
	PdfEngine::SetPdfStrokeLineMinFlatness(min_line_flatness);
	PdfEngine::SetPdfStrokeLineMaxWeight(max_line_weight);
	PdfEngine::SetPdfStrokeLineMaxFlatness(max_line_flatness);
	m_nMonoForward=nMonoForward;
	if(page_no<=0)
		return false;
	m_xPdfCfg=pdfConfig;
	if(m_pPdfEngine==NULL)
	{
		char utf_file_path[1024]={0};
		if(G2U(file_path,utf_file_path)<=0)
			return false;
		AutoFreeW dstPath(str::conv::FromUtf8(utf_file_path));
		m_pPdfEngine=PdfEngine::CreateFromFile(dstPath);
	}
	if(m_pPdfEngine==NULL)
		return false;
	//float dpi=m_pPdfEngine->GetFileDPI();
	//const float STD_DPI=150;
	//float zoom_scale=STD_DPI/dpi;
	/*RenderedBitmap* pBMP = pPdfEngine?pPdfEngine->RenderBitmap(page_no, (float)zoom_scale, rotation, NULL):NULL;
	if(pBMP)
	{
		HBITMAP hBitMap=NULL;
		if(pBMP&&(hBitMap=pBMP->GetBitmap())!=NULL)
		{
			//CopyImageToClipboard(hBitMap,false);
			CTempFileBuffer tempFile;
			//获取当进程的操作系统级全局唯一Id
			HANDLE hCurrProcess=GetCurrentProcess();
			DWORD processid=GetProcessId(hCurrProcess);
			CXhChar200 szTempFile("%sprocess-%d#pdf_raw_bmp.bmp",robot.szTempFilePath,processid);
			tempFile.CreateFile(szTempFile,0xF00000);	//缓存大小为10M
			size_t len=0;
			BYTE * byteArr=SerializeBitmap(hBitMap,&len);
			tempFile.Write(byteArr,len);
			ReadBmpFileByTempFile(tempFile);
			delete pBMP;
			delete pPdfEngine;
			//
			UpdateGreyImageBits();
			return UpdateMonoImageBits(nMonoForward);
		}
		else
			return false;
	}
	else
		return false;*/
#ifdef _TIMER_COUNT
	timer.Start();
#endif
	if(m_pPdfEngine->GetPageType(page_no)==BaseEngine::PAGE_TYPE_PDF_STROKE)
		m_ciRawImageFileType=3;	//RAW_IMAGE_PDF
	else
		m_ciRawImageFileType=4;	//RAW_IMAGE_PDF_IMG
	bool bRetCode=false;
	RenderedBitmapEx *pBitMap=m_pPdfEngine?m_pPdfEngine->RenderBitmapEx(page_no,(float)zoom_scale,rotation,pRect):NULL;
	if(pBitMap)
	{
#ifdef _TIMER_COUNT
		timer.Relay();
#endif
		ReleaseRawImageBits();
		m_nWidth=pBitMap->m_nWidth;
		m_nHeight=pBitMap->m_nHeight;
		m_nEffByteWidth=pBitMap->m_nEffByteWidth;
		m_uBitcount=pBitMap->m_uBitcount;
		if(pBitMap->m_dwRawBitsSize>0&&pBitMap->m_lpRawBits!=NULL)
		{
			m_lpRawBits = new BYTE[pBitMap->m_dwRawBitsSize];
			memcpy(m_lpRawBits,pBitMap->m_lpRawBits,pBitMap->m_dwRawBitsSize);
		}
		delete pBitMap;
		bRetCode=true;
	}
	else 
	{
		RenderedBitmap* pBMP = m_pPdfEngine?m_pPdfEngine->RenderBitmap(page_no, (float)zoom_scale, rotation, pRect):NULL;
		if(pBMP)
		{
#ifdef _TIMER_COUNT
			timer.Relay();
#endif
			HBITMAP hBitMap=NULL;
			if(pBMP&&(hBitMap=pBMP->GetBitmap())!=NULL)
			{
#ifdef _TIMER_COUNT
				timer.Relay(20001);
#endif
				//CopyImageToClipboard(hBitMap,false);
				ReleaseRawImageBits();
				CTempFileBuffer tempFile;
				//获取当进程的操作系统级全局唯一Id
				HANDLE hCurrProcess=GetCurrentProcess();
				DWORD processid=GetProcessId(hCurrProcess);
				CXhChar200 szTempFile("%sprocess-%d#pdf_raw_bmp.bmp",robot.szTempFilePath,processid);
				bool bCreateVirtualFile=false;
				if(!(bCreateVirtualFile=tempFile.CreateFile(szTempFile,0xF00000)))	//缓存大小为10M
					logerr.LevelLog(CLogFile::WARNING_LEVEL1_IMPORTANT,"虚拟内存文件创建失败，请核实程序是否以管理员身份运行!");
				size_t len=0;
				BYTE * byteArr=SerializeBitmap(hBitMap,&len);
				if(bCreateVirtualFile)
					tempFile.Write(byteArr,len);
				if(byteArr)
					free(byteArr);
				if(bCreateVirtualFile)
					ReadBmpFileByTempFile(tempFile);
				else
					;//TODO: 未完待续
				delete pBMP;
				bRetCode=true;
			}
			else
				bRetCode=false;
		}
		else
			bRetCode=false;
	}
	if(bRetCode&&bInitGrayBitMap)
	{
#ifdef _TIMER_COUNT
		timer.Relay(10001);
#endif
		UpdateGreyImageBits(nMonoForward);
#ifdef _TIMER_COUNT
		timer.Relay(10002);
#endif
		bRetCode=IntelliRecogMonoPixelThreshold(nMonoForward);
#ifdef _TIMER_COUNT
		timer.Relay(10003);
#endif
	}
#ifdef _TIMER_COUNT
	CLogErrorLife logtimer(&logerr);
	for(DWORD* pdwTickTime=timer.hashTicks.GetFirst();pdwTickTime;pdwTickTime=timer.hashTicks.GetNext())
		logerr.Log("Id=%3d, cost %d ms",timer.hashTicks.GetCursorKey(),*pdwTickTime);
#endif
	return bRetCode;
}

bool CImageTransform::RetrievePdfRegion(RectD *pRect,double zoom_scale,CImageTransform &image)
{
	if(m_pPdfEngine==NULL||pRect==NULL)
		return false;
	RectD rcInPdfPage=m_pPdfEngine->Transform(*pRect,m_xPdfCfg.page_no,(float)m_xPdfCfg.zoom_scale,m_xPdfCfg.rotation,true);
	zoom_scale=max(zoom_scale,m_xPdfCfg.zoom_scale);
	image.SetPdfEngine(m_pPdfEngine);
	PDF_FILE_CONFIG pdfConfig=m_xPdfCfg;
	pdfConfig.zoom_scale=zoom_scale;
	if(image.ReadPdfFile(NULL,pdfConfig,m_nMonoForward,&rcInPdfPage,TRUE))
	{
		RectD rcInUserImg=m_pPdfEngine->Transform(rcInPdfPage,m_xPdfCfg.page_no,(float)zoom_scale,m_xPdfCfg.rotation,false);
		*pRect=rcInUserImg;
		return true;
	}
	else
		return false;
}

bool CImageTransform::TransPdfRegionCornerPoint(double zoom_scale,PointD &leftTop,PointD &leftBtm,PointD &rightTop,PointD &rightBtm)
{
	if(m_pPdfEngine==NULL)
		return false;
	leftTop=m_pPdfEngine->Transform(leftTop,m_xPdfCfg.page_no,(float)m_xPdfCfg.zoom_scale,m_xPdfCfg.rotation,true);
	leftBtm=m_pPdfEngine->Transform(leftBtm,m_xPdfCfg.page_no,(float)m_xPdfCfg.zoom_scale,m_xPdfCfg.rotation,true);
	rightTop=m_pPdfEngine->Transform(rightTop,m_xPdfCfg.page_no,(float)m_xPdfCfg.zoom_scale,m_xPdfCfg.rotation,true);
	rightBtm=m_pPdfEngine->Transform(rightBtm,m_xPdfCfg.page_no,(float)m_xPdfCfg.zoom_scale,m_xPdfCfg.rotation,true);
	//
	leftTop=m_pPdfEngine->Transform(leftTop,m_xPdfCfg.page_no,(float)zoom_scale,m_xPdfCfg.rotation,false);
	leftBtm=m_pPdfEngine->Transform(leftBtm,m_xPdfCfg.page_no,(float)zoom_scale,m_xPdfCfg.rotation,false);
	rightTop=m_pPdfEngine->Transform(rightTop,m_xPdfCfg.page_no,(float)zoom_scale,m_xPdfCfg.rotation,false);
	rightBtm=m_pPdfEngine->Transform(rightBtm,m_xPdfCfg.page_no,(float)zoom_scale,m_xPdfCfg.rotation,false);
	return true;
}

void CImageTransform::SetPdfEngine(BaseEngine *pPdfEngine)
{
	m_bExternalPdfEngine=TRUE;
	m_pPdfEngine=pPdfEngine;
}

bool CImageTransform::WriteJpegFile(FILE* fp)
{
	return false;
}
//PNG格式图片的读写
bool CImageTransform::WritePngFile(FILE* fp)
{
	return false;
}
void CImageTransform::ClearImage()
{
	m_nWidth=m_nEffByteWidth=m_nHeight=m_uBitcount=0;
	//记录原始Bits
	if(m_lpRawBits)
		delete []m_lpRawBits;
	if(m_lpGrayBitsMap)
		delete []m_lpGrayBitsMap;
	if(m_lpPixelGrayThreshold)
		delete []m_lpPixelGrayThreshold;
	m_lpRawBits=m_lpGrayBitsMap=m_lpPixelGrayThreshold=NULL;
}
//图片的读与写
bool CImageTransform::ReadImageFile(const char* file_path,BYTE* lpExterRawBitsBuff/*=NULL*/,UINT uiBitsBuffSize/*=0*/,
			int nMonoForward/*=20*/)
{
	if(file_path==NULL)
		return false;
	FILE* fp=fopen(file_path,"rb");
	if(fp==NULL)
		return false;
	//CFileLifeObject filelife(fp);
	char drive[4],dir[MAX_PATH],fname[MAX_PATH],ext[MAX_PATH];
	_splitpath(file_path,drive,dir,fname,ext);
	char ciBmp0JpgPng2Tif3=0;
	bool retvalue=false;
	if(stricmp(ext,".bmp")==0)
		ciBmp0JpgPng2Tif3=0;
	else if(stricmp(ext,".jpg")==0)
		ciBmp0JpgPng2Tif3=1;
	else if(stricmp(ext,".png")==0)
		ciBmp0JpgPng2Tif3=2;
	else if (stricmp(ext, ".tif") == 0)
		ciBmp0JpgPng2Tif3 = 3;
	else
		return false;
	m_ciRawImageFileType=ciBmp0JpgPng2Tif3;
	return ReadImageFile(fp,ciBmp0JpgPng2Tif3,lpExterRawBitsBuff,uiBitsBuffSize);
}
bool CImageTransform::WriteImageFile(const char* file_path)
{
	if(file_path)
	{
		FILE* fp=fopen(file_path,"wb");
		if(fp==NULL)
			return false;
		char drive[4],dir[MAX_PATH],fname[MAX_PATH],ext[MAX_PATH];
		_splitpath(file_path,drive,dir,fname,ext);
		bool retvalue=false;
		if(stricmp(ext,".bmp")==0)
			retvalue=WriteBmpFile(fp);
		else if(stricmp(ext,".jpg")==0)
			retvalue=WriteJpegFile(fp);
		else if(stricmp(ext,".png")==0)
			retvalue=WritePngFile(fp);
		fclose(fp);
		return retvalue;
	}
	else
		return false;
}
//图像灰度值处理
BYTE CImageTransform::GetPixelGrayness(int x,int y)
{
	int index=y*m_nWidth+x;
	if(index<0||index>=m_nHeight*m_nWidth)
		return 0;
	if(m_lpGrayBitsMap)
		return m_lpGrayBitsMap[index];
	else
		return 0;
}
//图像灰度值处理
BYTE CImageTransform::GetPixelGraynessThresold(int x,int y)
{
	int index=y*m_nWidth+x;
	if(index<0||index>=m_nHeight*m_nWidth)
		return 0;
	if(m_lpPixelGrayThreshold)
		return m_lpPixelGrayThreshold[index];
	else
		return 0;
}
static bool _LocalTurnIJ(int W0,int H0,int xI0,int yJ0,int *pxI1,int *pyJ1,bool clockwise)
{
	if(clockwise)
	{	//像素索引号转换 i1=H0-1-j0,j1=i0;
		*pxI1=H0-1-yJ0;
		*pyJ1=xI0;
	}
	else
	{	//像素索引号转换 i1=j0,j1=W0-1-i0;
		*pxI1=yJ0;
		*pyJ1=W0-1-xI0;
	}
	return true;
}
bool CImageTransform::Turn90(bool byClockwise)		//将文件中图像顺时针转90度
{
	int xI0,yJ0,xI1,yJ1,W0,H0,W1,H1;
	//宽度W0,高度H0;H0->W1,W0->H1;
	W1=H0=this->GetHeight();
	H1=W0=this->GetWidth();
	if(m_lpRawBits!=NULL)
	{
		//转换真彩图像数据m_lpRawBits，行扫描(Downward top->bottom)形式的位图字节数据(真彩24位格式），处理前图像数据,m_nEffByteWidth*m_nHeight
		int nEffByteWidth=((W1*3+3)/4)*4;
		BYTE* lpNewRawBits=new BYTE[nEffByteWidth*H1];
		//像素索引号转换 i1=H0-1-j0,j1=i0;
		for(yJ0=0;yJ0<H0;yJ0++)
		{
			BYTE* rowdata=m_lpRawBits+m_nEffByteWidth*yJ0;
			for(xI0=0;xI0<W0;xI0++)
			{
				BYTE* pcbPixel=rowdata+xI0*3;
				_LocalTurnIJ(W0,H0,xI0,yJ0,&xI1,&yJ1,byClockwise);
				BYTE* pcbNewPixel=&lpNewRawBits[yJ1*nEffByteWidth+xI1*3];
				memcpy(pcbNewPixel,pcbPixel,3);
			}
		}
		delete []m_lpRawBits;
		m_lpRawBits=lpNewRawBits;
	}
	//转换单字节灰度型的图像像素数据表m_lpGrayBitsMap
	BYTE* lpGrayBitsMap=m_lpGrayBitsMap!=NULL?new BYTE[W1*H1]:NULL;
	BYTE* lpPixelGrayThreshold=m_lpPixelGrayThreshold!=NULL?new BYTE[W1*H1]:NULL;
	bool bRetCode=false;
	if(lpGrayBitsMap!=NULL||lpPixelGrayThreshold!=NULL)
	{
		for(yJ0=0;yJ0<H0;yJ0++)
		{
			BYTE* rowdata =m_lpGrayBitsMap+W0*yJ0;
			BYTE* rowdata2=m_lpPixelGrayThreshold+W0*yJ0;
			for(xI0=0;xI0<W0;xI0++)
			{
				_LocalTurnIJ(W0,H0,xI0,yJ0,&xI1,&yJ1,byClockwise);
				if(m_lpGrayBitsMap)
				{
					BYTE* pcbPixel=rowdata+xI0;
					BYTE* pcbNewPixel=&lpGrayBitsMap[yJ1*W1+xI1];
					*pcbNewPixel=*pcbPixel;
				}
				if(m_lpPixelGrayThreshold)
				{	//转换黑白点的划分阈值
					BYTE* pcbPixel=rowdata2+xI0;
					BYTE* pcbNewPixel=&lpPixelGrayThreshold[yJ1*W1+xI1];
					*pcbNewPixel=*pcbPixel;
				}
			}
		}
		if(m_lpGrayBitsMap!=NULL)
			delete []m_lpGrayBitsMap;
		m_lpGrayBitsMap=lpGrayBitsMap;
		if(m_lpPixelGrayThreshold!=NULL)
			delete []m_lpPixelGrayThreshold;
		m_lpPixelGrayThreshold=lpPixelGrayThreshold;
	}
	this->m_nWidth=W1;
	this->m_nHeight=H1;
	m_nEffByteWidth=((W1*3+3)/4)*4;
	//
	if (byClockwise)
		m_xPdfCfg.rotation += 90;
	else
		m_xPdfCfg.rotation -= 90;
	m_xPdfCfg.rotation = m_xPdfCfg.rotation % 360;
	return m_lpRawBits!=NULL;
}
BYTE CImageTransform::CalGreynessThreshold()
{
	int i,j;
	BYTE minGreyness=255,maxGreyness=0;
	for(i=0;i<m_nWidth;i++)
	{
		for(j=0;j<m_nHeight;j++)
		{
			if(GetPixelGrayness(i,j)>maxGreyness)
				maxGreyness=GetPixelGrayness(i,j);
			if(GetPixelGrayness(i,j)<minGreyness)
				minGreyness=GetPixelGrayness(i,j);
		}
	}
	m_ubGreynessThreshold=(int)((maxGreyness-minGreyness)/2.5)+minGreyness;
	return m_ubGreynessThreshold;
}
bool CImageTransform::IsBlackPixel(int x,int y,int nMonoForwardBalance/*=0*/)
{
	int index=y*m_nWidth+x;
	if(index<0||index>=m_nHeight*m_nWidth)
		return 0;
	BYTE greyness =m_lpGrayBitsMap?m_lpGrayBitsMap[index]:0;
	BYTE threshold=m_lpPixelGrayThreshold?m_lpPixelGrayThreshold[index]:0;
	return greyness<threshold-nMonoForwardBalance;
}
bool CImageTransform::UpdateRowRectPixelGrayThreshold(int xiTopLeft,int yjTopLeft,int niRowRectWidth,int niRowRectHeight,int nMonoForward/*=20*/)
{
	const int RGB_SIZE=256;
	int arrRGBSumPixelNum[RGB_SIZE]={0};
	int GRID_SIZE=niRowRectHeight;
	//GRID_WIDTH滑动窗口宽度值(取经验值20)，取值过大后折痕部分会成为黑块 wht 19-01-18
	int GRID_WIDTH=20,HALF_GRIDWIDTH=GRID_WIDTH/2;
	int xiRight=min(m_nWidth,xiTopLeft+niRowRectWidth);	//每行的右边界(不含本身)
	for(int xiCurrScan=0;xiCurrScan<niRowRectWidth;xiCurrScan++)
	{
		DWORD iPixelStartPos=yjTopLeft*m_nWidth+xiTopLeft+xiCurrScan;
		char ciMoveWndType='L';	//'L'表示在左边界，'R'表示在右边界，其它如'M'表示中间，中间滑动窗口只更新第xiCurrScan+HALF_GRIDWIDTH列单色分割阈值
		if(xiCurrScan<GRID_WIDTH)
			ciMoveWndType='L';
		else
		{
			if(xiCurrScan<m_nWidth-1)
				ciMoveWndType='M';
			else //if(xiCurrScan==m_nWidth-1)
				ciMoveWndType='R';
			int xiLeftElapsedIndex=iPixelStartPos-GRID_WIDTH;
			for (int iy=0;iy<GRID_SIZE;iy++)
			{
				arrRGBSumPixelNum[m_lpGrayBitsMap[xiLeftElapsedIndex]]--;
				xiLeftElapsedIndex+=m_nWidth;
			}
		}
		int xiCurrScanPixelIndex=iPixelStartPos;
		for (int iy=0;iy<GRID_SIZE;iy++)
		{
			arrRGBSumPixelNum[m_lpGrayBitsMap[xiCurrScanPixelIndex]]++;
			xiCurrScanPixelIndex+=m_nWidth;
		}
		if(xiCurrScan<GRID_WIDTH-1&&xiCurrScan<m_nWidth-1)
			continue;//	还不够指定的的统计样本宽度
		int nCursorWidth =5, unitRGBSumByArr[RGB_SIZE]={0};
		for(int i=0;i<RGB_SIZE;i++)
		{
			for(int	m=0;m<nCursorWidth;m++)
			{
				if(i-m>=0)
					unitRGBSumByArr[i]+=arrRGBSumPixelNum[i-m];
				if(i+m<RGB_SIZE)
					unitRGBSumByArr[i]+=arrRGBSumPixelNum[i+m];
			}
			//unitRGBSumByArr[i]=(int)(unitRGBSumByArr[i]/nCursorWidth);
		}

		int iMaxValueIndex=0,nMaxValue=0;
		//背景色阈值取经验值80，从大于背景色阈值的像素中找到频率最高的像素点
		//避免一个滑动窗口内黑色像素点过多，误将黑点设置为背景色 wht 19-01-18
		const int BACKGROUND_THRESHOLD=80;	
		for (int i=BACKGROUND_THRESHOLD;i<RGB_SIZE;i++ ) //找出最大
		{
			if(unitRGBSumByArr[i]>nMaxValue)
			{
				iMaxValueIndex=i;
				nMaxValue=unitRGBSumByArr[i];
			}
		}
		int niMonoThreshold=(BYTE)max(0,iMaxValueIndex-nMonoForward);	//暂时以出现频率最高的像素点为基准向前推20,作为单元格灰度阈值
		int xiCurrSetPixelX=xiCurrScan,xiRightestSetPixelX=xiCurrScan+HALF_GRIDWIDTH;
		if(xiCurrScan==GRID_WIDTH-1)
			xiCurrSetPixelX=0;	//左侧第一个合格统计样本区应同时把样本方格的左半侧初始化赋值
		else
			xiCurrSetPixelX=xiCurrScan-HALF_GRIDWIDTH;
		if(ciMoveWndType!='R')
			xiRightestSetPixelX=xiCurrScan-HALF_GRIDWIDTH;
		else
			xiRightestSetPixelX=xiRight-1;
		if(m_lpPixelGrayThreshold!=NULL)
		{
			for(;xiCurrSetPixelX<=xiRightestSetPixelX;xiCurrSetPixelX++)
			{
				int xiCurrSetPixelIndex=iPixelStartPos+xiCurrSetPixelX-xiCurrScan;
				for (int iy=0;iy<GRID_SIZE;iy++)
				{
					this->m_lpPixelGrayThreshold[xiCurrSetPixelIndex]=niMonoThreshold;
					xiCurrSetPixelIndex+=m_nWidth;
				}
			}
		}
	}
	return true;
}
bool CImageTransform::LocalFiningMonoPixelThreshold(int xiTopLeft,int yjTopLeft,int nRgnWidth,int nRgnHeight,
	int nGridWidth/*=100*/,int nGridHeight/*=100*/)
{
	for(int yjCurr=yjTopLeft;yjCurr<yjTopLeft+nRgnHeight;yjCurr+=nGridHeight)
		UpdateRowRectPixelGrayThreshold(xiTopLeft,yjCurr,nRgnWidth,nGridHeight);
	return true;
}
void CImageTransform::CalRectPixelGrayThreshold(int iRow,int iCol,int RECT_WIDTH,int RECT_HEIGHT,int nMonoForward/*=20*/)
{
	const int RGB_SIZE=256;
	int arrRGBSumPixelNum [RGB_SIZE]={0};
	for (int iWidth=0;iWidth<RECT_WIDTH;iWidth++)//遍历每个单元(长)
	{
		if((iCol==m_nRectColNum-1)&&(iCol*RECT_WIDTH+iWidth)>m_nWidth)
			continue;	//最后一个单元格宽度可能小于RECT_WIDTH
		for (int iHeight=0;iHeight<RECT_HEIGHT;iHeight++)
		{	//取出rgb数字
			int ix=iCol*RECT_WIDTH+iWidth;
			int iy=iRow*RECT_HEIGHT+iHeight;
			if(ix>=m_nWidth||iy>=m_nHeight)
				break;	//超界
			//DWORD iPixelStartPos=iy*m_nEffByteWidth+ix*3;
			//BYTE r=m_lpRawBits[iPixelStartPos+2];
			//BYTE g=m_lpRawBits[iPixelStartPos+1];
			//BYTE b=m_lpRawBits[iPixelStartPos+0];
			//int rgbIndex=(b*114+g*587+r*299)/1000;
			//arrRGBSumPixelNum[rgbIndex]++;
			DWORD iPixelStartPos=iy*m_nWidth+ix;
			arrRGBSumPixelNum[m_lpGrayBitsMap[iPixelStartPos]]++;
		}
	}
	int nCursorWidth =5;
	int unitRGBSumByArr[RGB_SIZE]={0};
	for(int i=0;i<RGB_SIZE;i++)
	{
		for(int	m=0;m<nCursorWidth;m++)
		{
			if(i-m>=0)
				unitRGBSumByArr[i]+=arrRGBSumPixelNum[i-m];
			if(i+m<RGB_SIZE)
				unitRGBSumByArr[i]+=arrRGBSumPixelNum[i+m];
		}
		unitRGBSumByArr[i]=(int)(unitRGBSumByArr[i]/nCursorWidth);
	}
	/*logerr.Log("第%d行 第%d列",iRow,iCol);
	if(iRow==0&&(iCol==0||iCol==1))
	{
		for(int k=0;k<RGB_SIZE;k++)
			logerr.Log("%d:%d",k,unitRGBSumByArr[k]);
	}*/
	int iMaxValueIndex=0,nMaxValue=0;
	for (int i=0;i<RGB_SIZE;i++ ) //找出最大
	{
		if(unitRGBSumByArr[i]>nMaxValue)
		{
			iMaxValueIndex=i;
			nMaxValue=unitRGBSumByArr[i];
		}
	}
	iMaxValueIndex = max(10,iMaxValueIndex-nMonoForward);		//暂时以出现频率最高的像素点为基准向前推20,作为单元格灰度阈值
	for (int iWidth=0;iWidth<RECT_WIDTH;iWidth++)//遍历每个单元(长)
	{
		if((iCol==m_nRectColNum-1)&&(iCol*RECT_WIDTH+iWidth)>m_nWidth)
			continue;	//最后一个单元格宽度可能小于RECT_WIDTH
		for (int iHeight=0;iHeight<RECT_HEIGHT;iHeight++)
		{
			int ix=iCol*RECT_WIDTH+iWidth;
			int iy=iRow*RECT_HEIGHT+iHeight;
			if(ix>=m_nWidth||iy>=m_nHeight)
				break;	//超界
			//int iPixelPos=iy*m_nWidth+ix;
			//if(iPixelPos>=m_nWidth*m_nHeight)
			//	break;
			m_lpPixelGrayThreshold[iy*m_nWidth+ix]=iMaxValueIndex;
		}
	}
}
bool CImageTransform::IntelliRecogMonoPixelThreshold(int nMonoForward/*=20*/)
{
	CLogErrorLife log5;
	if(m_lpGrayBitsMap==NULL||m_nWidth==0||m_nHeight==0)
		return false;
	if(m_lpPixelGrayThreshold!=NULL)
		delete []m_lpPixelGrayThreshold;
	m_lpPixelGrayThreshold = new BYTE[m_nWidth*m_nHeight];
	memset(m_lpPixelGrayThreshold,0,sizeof(BYTE)*m_nWidth*m_nHeight);
	if(m_nWidth<64)	//字符模板不需分区计算
	{
		CalRectPixelGrayThreshold(0,0,m_nWidth,m_nHeight,nMonoForward);
		return true;
	}
	//1.计算矩形区域宽度和高度
	//TODO:此处的行数与列数未来应进一步优化，以便更为精细化的处理局部区域的黑白转换 wjh-2018.4.16
	m_nRectColNum=16,m_nRectRowNum=0;		//宽高等分数
	//1.1 计算矩形区域宽度(基底统计宽度不能太小，否则可能会误把黑色自身当成底色）
	m_nRectWidth=max(64,m_nWidth/m_nRectColNum), m_nRectHeight=0;
	int nWidthOdd=m_nWidth%m_nRectWidth;
	while(nWidthOdd<=0.5*m_nRectWidth)
	{
		m_nRectWidth++;
		nWidthOdd=m_nWidth%m_nRectWidth;
	}
	m_nRectColNum = m_nWidth/m_nRectWidth;
	if(nWidthOdd>0)
		m_nRectColNum+=1;
	//1.2 计算矩形高度,保证等分图片高度
	m_nRectHeight=m_nRectWidth;
	if(m_nRectHeight>m_nHeight)
		m_nRectHeight=m_nHeight;
	else 
	{
		int nOdd=m_nHeight%m_nRectHeight;
		while(nOdd!=0 && nOdd<0.5*m_nRectHeight)
		{
			m_nRectHeight--;
			nOdd=m_nHeight%m_nRectHeight;
		}
	}
	m_nRectRowNum = m_nHeight/m_nRectHeight;
	if(m_nHeight%m_nRectHeight>0)
		m_nRectRowNum+=1;
	//2.按矩形计算像素点灰度阈值
	for(int iCol=0;iCol<m_nRectColNum;iCol++)
	{
		for(int iRow=0;iRow<m_nRectRowNum;iRow++)
			CalRectPixelGrayThreshold(iRow,iCol,m_nRectWidth,m_nRectHeight,nMonoForward);	
	}
	return true;
}
//图像进行灰白化处理
bool CImageTransform::UpdateGreyImageBits(bool bUpdateMonoThreshold/*=false*/, int nMonoForward/*=20*/)
{
	if (m_lpRawBits == NULL || m_nWidth == 0 || m_nHeight == 0)
		return false;
	if (m_lpGrayBitsMap)
		delete[]m_lpGrayBitsMap;
	//if (m_uBitcount == 24)	//因过往代码不规范存在m_uBitcount未初始化情况所以暂不敢开放此参数 wjh-2019.11.30
	{
		m_lpGrayBitsMap = new BYTE[m_nWidth*m_nHeight];
		for (int i = 0; i < m_nWidth; i++)
		{
			for (int j = 0; j < m_nHeight; j++)
			{
#ifndef __ENCRYPT_VERSION_
				BYTE r = m_lpRawBits[j*m_nEffByteWidth + i * 3 + 2];
				BYTE g = m_lpRawBits[j*m_nEffByteWidth + i * 3 + 1];
				BYTE b = m_lpRawBits[j*m_nEffByteWidth + i * 3 + 0];
				//RGB按照300:300:400比例分配
				//字体颜色RGB(103,101,114)  综合值106.8
				//背景颜色RGB(165,166,171)  综合值150.6
				m_lpGrayBitsMap[j*m_nWidth + i] = (b * 114 + g * 587 + r * 299) / 1000;
#else		//替换为加密函数
				m_lpGrayBitsMap[j*m_nWidth + i] = CMindRobot::ToGreyPixelByte(&m_lpRawBits[j*m_nEffByteWidth + i * 3]);
#endif
			}
		}
	}
	/*else
	{
		DWORD dwCount = m_nEffByteWidth * m_nHeight;
		m_lpGrayBitsMap = new BYTE[dwCount];
		memcpy(m_lpGrayBitsMap, m_lpRawBits, dwCount);
	}*/
	if(bUpdateMonoThreshold)
		IntelliRecogMonoPixelThreshold(nMonoForward);
	return true;
}
//
bool CImageTransform::GapDetect(int start_x,int start_y,int maxGap,bool bScanByRow,int *gaplen)
{
	int i,step=1,absMaxGap=abs(maxGap);
	if(maxGap<0)
		step=-1;
	for(i=0;i<absMaxGap;i++)
	{
		if(bScanByRow)	//行扫描
			start_x+=step;
		else	//列扫描
			start_y+=step;
		if(IsBlackPixel(start_x,start_y))
		{
			*gaplen=i+1;
			return true;
		}
	}
	return false;
}
//
POINT CImageTransform::TransformPointToOriginalField(POINT p,TRANSFORM* pTransform/*=NULL*/)
{
	if(pTransform==NULL)
	{
#ifndef __ENCRYPT_VERSION_
		POINT point;
		double yitax=(double)p.x/m_nDrawingRgnWidth;	//ηx
		double yitay=(double)p.y/m_nDrawingRgnHeight;	//ηy
		point.x=(long)(0.5+(kesai[0]*p.x+m_leftUp.x)*(1-yitay)+(kesai[1]*p.x+m_leftDown.x)*yitay);
		point.y=(long)(0.5+(kesai[2]*p.y+m_leftUp.y)*(1-yitax)+(kesai[3]*p.y+m_rightUp.y)*yitax);
		return point;
#else
		POINT vertexes[4]={m_leftUp,m_leftDown,m_rightUp,m_rightDown};
		return CMindRobot::TransPointToOrginalCoord(p,kesai,vertexes,m_nDrawingRgnWidth,m_nDrawingRgnHeight);
#endif
	}
	else
		return pTransform->gridmap.Interpolate(p.y,p.x);
}
RECT CImageTransform::TransformRectToOriginalField(POINT p,TRANSFORM* pTransform/*=NULL*/)	//将指定区域点转换到原始图像的指定矩形区域
{
	POINT point,point2;
	RECT rcOriginal;
	if(kesai[0]+kesai[1]+kesai[2]+kesai[3]<=4)
	{
		point2=point=TransformPointToOriginalField(p,pTransform);
		rcOriginal.left=point.x;
		rcOriginal.top =point.y;
		rcOriginal.right=point.x+1;
		rcOriginal.bottom=point.y+1;

	}
	else
	{
		point=TransformPointToOriginalField(p,pTransform);
		point2.x=p.x+1;
		point2.y=p.y+1;
		point2=TransformPointToOriginalField(point2,pTransform);
		rcOriginal.left=point.x;
		int x1=point.x+1,y1=point.y+1;
		rcOriginal.right=x1>point2.x?x1:point2.x;//max(point.x+1,point2.x);
		rcOriginal.top=point.y;
		rcOriginal.bottom=y1>point2.y?y1:point2.y;//max(point.y+1,point2.y);
	}
	return rcOriginal;
}
//
const BYTE BIT2BYTE[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
bool CImageTransform::RetrieveSpecifyRegion(double pos_x_coef, double pos_y_coef, double width_coef, double height_coef,BYTE** outBitsMap,long *outWidth,long *effWidth,long *outHeight)
{
	POINT leftdown,p;
	leftdown.x=(int)(pos_x_coef*m_nDrawingRgnWidth);
	leftdown.y=(int)(pos_y_coef*m_nDrawingRgnHeight);
	int width=*outWidth=(int)(width_coef*m_nDrawingRgnWidth);
	int height=*outHeight=(int)(height_coef*m_nDrawingRgnHeight);
	int eff_width;
	width%16>0?eff_width=(width/16)*2+2:eff_width=width/8;
	*effWidth=eff_width;
	*outBitsMap = new BYTE[eff_width*height];
	memset(*outBitsMap,0,eff_width*height);
	for(int j=0;j<height;j++)
	{
		for(int i=0;i<width;i++)
		{
			p.x=i+leftdown.x;
			p.y=height-j-1+leftdown.y;
			p=TransformPointToOriginalField(p);
			if(!IsBlackPixel(p.x,p.y))//i+leftUp.x,GetHeight()-j-leftUp.y-1))
			{
				int iByte=i/8,iBit=i%8;
				(*outBitsMap)[j*eff_width+iByte]|=BIT2BYTE[7-iBit];
			}
		}
	}
	return true;
}
//
bool CImageTransform::CalDrawingRgnRect()
{
	int i,j;
	CSamplePixelLine sampleLineArr[5];
	int half_width=m_nWidth/2;
	bool *pixels=new bool[half_width];
	//选择五条象素取样行线，用来确定边缘线
	for(i=0;i<5;i++)
	{
		int offset=0;
		while(1)
		{
			sampleLineArr[i].m_iPosIndex=(int)((3+i)*m_nHeight/10);
			for(j=0;j<half_width;j++)
			{
				pixels[j]=IsBlackPixel(j,sampleLineArr[i].m_iPosIndex);
			}
			if(sampleLineArr[i].AnalyzeLinePixels(pixels,half_width))
				break;
			else
			{
				offset++;
				if(offset%2>0)	//奇数为负向移位
					sampleLineArr[i].m_iPosIndex-=offset/2+1;
				else			//偶数为正向移位
					sampleLineArr[i].m_iPosIndex+=offset/2;
				if(offset>0.1*m_nHeight)
					break;
			}
		}
	}
	delete []pixels;
	POINT leftUp,leftDown,rightUp,rightDown;
	leftUp.x=leftUp.y=leftDown.x=leftDown.y=0;
	double deviation_coef=tan(10*RADTODEG_COEF);
	for(i=0;i<sampleLineArr[0].m_nValidBlackSeg;i++)
	{
		int seg_start,seg_end;
		int mid=(sampleLineArr[0].shiftSegArr[i][0]+sampleLineArr[0].shiftSegArr[i][1])/2;
		leftUp.x=mid;
		for(j=1;j<5;j++)
		{
			int height=sampleLineArr[j].m_iPosIndex-sampleLineArr[j-1].m_iPosIndex;
			int deviation=(int)(height*deviation_coef);
			if(!sampleLineArr[j].FindBlackSegInSpecScope(mid-deviation,mid+deviation,&seg_start,&seg_end))
				break;	//没找到合适的黑色线段
			mid=(seg_start+seg_end)/2;
		}
		if(j==5)
		{
			leftDown.x=mid;
			break;
		}
	}
	if(i==sampleLineArr[0].m_nValidBlackSeg)
		return false;
	leftUp.y=sampleLineArr[0].m_iPosIndex;
	leftDown.y=sampleLineArr[4].m_iPosIndex;
	bool bBreakLoop=false;
	double fSlope = 1;
	if((leftUp.x-leftDown.x)!=0)
		fSlope = (leftUp.y - leftDown.y)/(leftUp.x-leftDown.x);
	else
		fSlope = 0;
	int gaplen;
	//查找左上顶点
	for(i=1;i<leftUp.y;i++)
	{
		if(IsBlackPixel(leftUp.x,leftUp.y-i))
			continue;
		else if(leftUp.x-1>0&&IsBlackPixel(leftUp.x-1,leftUp.y-i))
			leftUp.x--;
		else if(leftUp.x+1<m_nWidth&&IsBlackPixel(leftUp.x+1,leftUp.y-i))
			leftUp.x++;
		else if(GapDetect(leftUp.x,leftUp.y-i,-5,false,&gaplen))
			i+=gaplen-1;
		else if(leftUp.x-1>0&&GapDetect(leftUp.x-1,leftUp.y-i,-5,false,&gaplen))
		{
			i+=gaplen-1;
			leftUp.x--;
		}
		else if(leftUp.x+1<m_nWidth&&GapDetect(leftUp.x+1,leftUp.y-i,-5,false,&gaplen))
		{
			i+=gaplen-1;
			leftUp.x++;
		}
		else
		{
			leftUp.y -= i-1;
			bBreakLoop=true;
			break;
		}
	}
	if(!bBreakLoop)
		leftUp.y=0;
	//左下顶点
	bBreakLoop=false;
	for(i=1;i<m_nHeight-leftDown.y;i++)
	{
		if(IsBlackPixel(leftDown.x,leftDown.y+i))
			continue;
		else if(leftDown.x-1>0&&IsBlackPixel(leftDown.x-1,leftDown.y+i))
			leftDown.x--;
		else if(leftDown.x+1<m_nWidth&&IsBlackPixel(leftDown.x+1,leftDown.y+i))
			leftDown.x++;
		else if(GapDetect(leftDown.x,leftDown.y+i,5,false,&gaplen))
			i+=gaplen-1;
		else if(leftDown.x-1>0&&GapDetect(leftDown.x-1,leftDown.y+i,5,false,&gaplen))
		{
			i+=gaplen-1;
			leftDown.x--;
		}
		else if(leftDown.x+1<m_nWidth&&GapDetect(leftDown.x+1,leftDown.y+i,5,false,&gaplen))
		{
			i+=gaplen-1;
			leftDown.x++;
		}
		else
		{
			leftDown.y += i-1;
			bBreakLoop=true;
			break;
		}
	}
	if(!bBreakLoop)
		leftDown.y=m_nHeight-1;
	//查找右上顶点
	rightUp = leftUp;
	bBreakLoop=false;
	for(i=1;i<m_nWidth;i++)
	{
		if(IsBlackPixel(rightUp.x+i,rightUp.y))
			continue;
		else if(rightUp.y-1>0&&IsBlackPixel(rightUp.x+i,rightUp.y-1))
			rightUp.y--;
		else if(rightUp.y+1<m_nHeight&&IsBlackPixel(rightUp.x+i,rightUp.y+1))
			rightUp.y++;
		else if(GapDetect(rightUp.x+1,rightUp.y,5,true,&gaplen))
			i+=gaplen-1;
		else if(leftDown.x-1>0&&GapDetect(rightUp.x+i,rightUp.y-1,5,true,&gaplen))
		{
			i+=gaplen-1;
			rightUp.y--;
		}
		else if(leftDown.x+1<m_nWidth&&GapDetect(rightUp.x+i,rightUp.y+1,5,true,&gaplen))
		{
			i+=gaplen-1;
			rightUp.y++;
		}
		else
		{
			rightUp.x += i-1;
			bBreakLoop=true;
			break;
		}
	}
	if(!bBreakLoop)
		rightUp.x=m_nWidth;
	//查找右下定点
	rightDown = leftDown;
	bBreakLoop=false;
	for(i=1;i<m_nWidth;i++)
	{
		if(IsBlackPixel(rightDown.x+i,rightDown.y))
			continue;
		else if(rightDown.y-1>0&&IsBlackPixel(rightDown.x+i,rightDown.y-1))
			rightDown.y--;
		else if(rightDown.y+1<m_nHeight&&IsBlackPixel(rightDown.x+i,rightDown.y+1))
			rightDown.y++;
		else if(GapDetect(rightDown.x+1,rightDown.y+1,5,true,&gaplen))
			i+=gaplen-1;
		else if(leftDown.x-1>0&&GapDetect(rightDown.x+i,rightDown.y+1,5,true,&gaplen))
		{
			i+=gaplen-1;
			rightDown.y--;
		}
		else if(leftDown.x+1<m_nWidth&&GapDetect(rightDown.x+i,rightDown.y+1,5,true,&gaplen))
		{
			i+=gaplen-1;
			rightDown.y++;
		}
		else
		{
			rightDown.x += i-1;
			bBreakLoop=true;
			break;
		}
	}
	if(!bBreakLoop)
		rightDown.x=m_nWidth;
	//
	m_leftUp=leftUp;
	m_leftDown=leftDown;
	m_rightUp=rightUp;
	m_rightDown=rightDown;
	return true;
}
void CImageTransform::LocateDrawingRgnRect(POINT leftU,POINT leftD,POINT rightU,POINT rightD)
{
	m_leftUp.x=leftU.x;
	m_leftUp.y=leftU.y;
	m_leftDown.x=leftD.x;
	m_leftDown.y=leftD.y;
	m_rightUp.x=rightU.x;
	m_rightUp.y=rightU.y;
	m_rightDown.x=rightD.x;
	m_rightDown.y=rightD.y;
	m_nDrawingRgnWidth=(m_rightUp.x+m_rightDown.x-m_leftUp.x-m_leftDown.x)/2;
	m_nDrawingRgnHeight=(m_leftDown.y+m_rightDown.y-m_leftUp.y-m_rightUp.y)/2;
#ifndef __ENCRYPT_VERSION_
	kesai[0]=(double)(m_rightUp.x-m_leftUp.x)/m_nDrawingRgnWidth;
	kesai[1]=(double)(m_rightDown.x-m_leftDown.x)/m_nDrawingRgnWidth;
	kesai[2]=(double)(m_leftDown.y-m_leftUp.y)/m_nDrawingRgnHeight;
	kesai[3]=(double)(m_rightDown.y-m_rightUp.y)/m_nDrawingRgnHeight;
#else //以上四行被下面的加密函数取代
	POINT vertexes[4]={m_leftUp,m_leftDown,m_rightUp,m_rightDown};
	CMindRobot::CalRectPolyTransCoef(vertexes,kesai);
#endif
}
/*int CImageTransform::StretchBlt(HDC hDC,CRect imageShowRect,DWORD rop)
{
	return ::StretchDIBits( hDC,imageShowRect.left,imageShowRect.top,imageShowRect.Width(),imageShowRect.Height(),
		0,0,GetWidth(),GetHeight(),GetBits(),(LPBITMAPINFO)GetBitmapInfoHeader(),DIB_RGB_COLORS,rop);
}*/
