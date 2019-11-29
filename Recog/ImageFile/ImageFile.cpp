// ImageFile.cpp: implementation of the CImageFile class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ImageFile.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CImageFile::CImageFile()
{
	//init pointers
	pDib = NULL;
	pAlpha=NULL;
	//pSelection = pAlpha = NULL;
	//ppLayers = ppFrames = NULL;
	//init structures
	memset(&head,0,sizeof(BITMAPINFOHEADER));
	memset(&info,0,sizeof(CIMAGEINFO));
	//init default attributes
    //info.dwType = imagetype;
	info.fQuality = 90.0f;
	info.nAlphaMax = 255;
	info.nBkgndIndex = -1;
	info.bEnabled = true;
	SetXDPI(96);//CXIMAGE_DEFAULT_DPI);
	SetYDPI(96);//CXIMAGE_DEFAULT_DPI);

	short test = 1;
	info.bLittleEndianHost = (*((char *) &test) == 1);
	m_bInternalRawBitsBuff = true;
}

CImageFile::~CImageFile()
{
	if (pDib&&m_bInternalRawBitsBuff)
		Destroy();
}
#if CXIMAGE_SUPPORT_ALPHA
////////////////////////////////////////////////////////////////////////////////
/**
 * Sets the alpha level for the whole image.
 * \param level : from 0 (transparent) to 255 (opaque)
 */
void CImageFile::AlphaSet(BYTE level)
{
	if (pAlpha)	memset(pAlpha,level,head.biWidth * head.biHeight);
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Sets the alpha level for a single pixel 
 */
void CImageFile::AlphaSet(const int x,const int y,const int level)
{
	if (pAlpha && IsInside(x,y)) pAlpha[x+y*head.biWidth]=level;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Gets the alpha level for a single pixel 
 */
BYTE CImageFile::AlphaGet(const int x,const int y)
{
	if (pAlpha && IsInside(x,y)) return pAlpha[x+y*head.biWidth];
	return 0;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Allocates an empty (opaque) alpha channel.
 */
bool CImageFile::AlphaCreate()
{
	if (pAlpha==NULL) {
		pAlpha = (BYTE*)malloc(head.biWidth * head.biHeight);
		if (pAlpha) memset(pAlpha,255,head.biWidth * head.biHeight);
	}
	return (pAlpha!=0);
}
////////////////////////////////////////////////////////////////////////////////
void CImageFile::AlphaDelete()
{
	if (pAlpha) { free(pAlpha); pAlpha=0; }
}
////////////////////////////////////////////////////////////////////////////////
bool CImageFile::AlphaFlip()
{
	if (!pAlpha) return false;

	BYTE *buff = (BYTE*)malloc(head.biWidth);
	if (!buff) return false;

	BYTE *iSrc,*iDst;
	iSrc = pAlpha + (head.biHeight-1)*head.biWidth;
	iDst = pAlpha;
	for (int i=0; i<(head.biHeight/2); ++i)
	{
		memcpy(buff, iSrc, head.biWidth);
		memcpy(iSrc, iDst, head.biWidth);
		memcpy(iDst, buff, head.biWidth);
		iSrc-=head.biWidth;
		iDst+=head.biWidth;
	}

	free(buff);

	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Get alpha value without boundscheck (a bit faster). Pixel must be inside the image.
 *
 * \author ***bd*** 2.2004
 */
BYTE CImageFile::BlindAlphaGet(const int x,const int y)
{
#ifdef _DEBUG
	if (!IsInside(x,y) || (pAlpha==0))
  #if CXIMAGE_SUPPORT_EXCEPTION_HANDLING
		throw 0;
  #else
		return 0;
  #endif
#endif
	return pAlpha[x+y*head.biWidth];
}
#endif
////////////////////////////////////////////////////////////////////////////////
/**
 * swaps the blue and red components (for RGB images)
 * \param buffer : pointer to the pixels
 * \param length : number of bytes to swap. lenght may not exceed the scan line.
 */
void CImageFile::RGBtoBGR(BYTE *buffer, int length)
{
	if (buffer && (head.biClrUsed==0)){
		BYTE temp;
		length = min(length,(int)info.dwEffWidth);
		length = min(length,(int)(3*head.biWidth));
		for (int i=0;i<length;i+=3){
			temp = buffer[i]; buffer[i] = buffer[i+2]; buffer[i+2] = temp;
		}
	}
}
////////////////////////////////////////////////////////////////////////////////
RGBQUAD CImageFile::RGBtoRGBQUAD(COLORREF cr)
{
	RGBQUAD c;
	c.rgbRed = GetRValue(cr);	/* get R, G, and B out of UINT */
	c.rgbGreen = GetGValue(cr);
	c.rgbBlue = GetBValue(cr);
	c.rgbReserved=0;
	return c;
}
////////////////////////////////////////////////////////////////////////////////
COLORREF CImageFile::RGBQUADtoRGB (RGBQUAD c)
{
	return RGB(c.rgbRed,c.rgbGreen,c.rgbBlue);
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Initializes or rebuilds the image.
 * \param dwWidth: width
 * \param dwHeight: height
 * \param wBpp: bit per pixel, can be 1, 4, 8, 24
 * \param imagetype: (optional) set the image format, see ENUM_CXIMAGE_FORMATS
 * \return pointer to the internal pDib object; NULL if an error occurs.
 */
void* CImageFile::Create(DWORD dwWidth, DWORD dwHeight, DWORD wBpp, DWORD imagetype, BYTE* lpExterRawBitsBuff/*=NULL*/, UINT uiBitsBuffSize/*=0*/)
{
	// destroy the existing image (if any)
	if (!Destroy())
		return NULL;

	// prevent further actions if width or height are not vaild <Balabasnia>
	if ((dwWidth == 0) || (dwHeight == 0)){
		strcpy(info.szLastError,"CxImage::Create : width and height must be greater than zero");
		return NULL;
	}

    // Make sure bits per pixel is valid
    if		(wBpp <= 1)	wBpp = 1;
    else if (wBpp <= 4)	wBpp = 4;
    else if (wBpp <= 8)	wBpp = 8;
    else				wBpp = 24;

	// limit memory requirements (and also a check for bad parameters)
	if (((dwWidth*dwHeight*wBpp)>>3) > CXIMAGE_MAX_MEMORY ||
		((dwWidth*dwHeight*wBpp)/wBpp) != (dwWidth*dwHeight))
	{
		strcpy(info.szLastError,"CXIMAGE_MAX_MEMORY exceeded");
		return NULL;
	}

	// set the correct bpp value
    switch (wBpp){
        case 1:
            head.biClrUsed = 2;	break;
        case 4:
            head.biClrUsed = 16; break;
        case 8:
            head.biClrUsed = 256; break;
        default:
            head.biClrUsed = 0;
    }

	//set the common image informations
    info.dwEffWidth = ((((wBpp * dwWidth) + 31) / 32) * 4);
    info.dwType = imagetype;

    // initialize BITMAPINFOHEADER
	head.biSize = sizeof(BITMAPINFOHEADER); //<ralphw>
    head.biWidth = dwWidth;		// fill in width from parameter
    head.biHeight = dwHeight;	// fill in height from parameter
    head.biPlanes = 1;			// must be 1
    head.biBitCount = (WORD)wBpp;		// from parameter
    head.biCompression = BI_RGB;    
    head.biSizeImage = info.dwEffWidth * dwHeight;
//    head.biXPelsPerMeter = 0; See SetXDPI
//    head.biYPelsPerMeter = 0; See SetYDPI
//    head.biClrImportant = 0;  See SetClrImportant

	if (lpExterRawBitsBuff != NULL && uiBitsBuffSize >= (UINT)GetSize())
	{
		pDib = lpExterRawBitsBuff;
		m_bInternalRawBitsBuff = false;
	}
	else if (lpExterRawBitsBuff != NULL)
		return NULL;	//预留外缓存不足
	else
	{
		m_bInternalRawBitsBuff = true;
		pDib = malloc(GetSize()); // alloc memory block to store our bitmap
	}
    if (!pDib){
		strcpy(info.szLastError,"CxImage::Create can't allocate memory");
		return NULL;
	}

	//clear the palette
	RGBQUAD* pal=GetPalette();
	if (pal) memset(pal,0,GetPaletteSize());
	//Destroy the existing selection
#if CXIMAGE_SUPPORT_SELECTION
	if (pSelection) SelectionDelete();
#endif //CXIMAGE_SUPPORT_SELECTION
	//Destroy the existing alpha channel
#if CXIMAGE_SUPPORT_ALPHA
	if (pAlpha) AlphaDelete();
#endif //CXIMAGE_SUPPORT_ALPHA

    // use our bitmap info structure to fill in first part of
    // our DIB with the BITMAPINFOHEADER
    BITMAPINFOHEADER*  lpbi;
	lpbi = (BITMAPINFOHEADER*)(pDib);
    *lpbi = head;

	info.pImage=GetBits();

    return pDib; //return handle to the DIB
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Call this function to destroy image pixels, alpha channel, selection and sub layers.
 * - Attributes are not erased, but IsValid returns false.
 *
 * \return true if everything is freed, false if the image is a Ghost
 */
bool CImageFile::Destroy()
{
	//free this only if it's valid and it's not a ghost
	if (info.pGhost==NULL){
		/* (ppLayers) { 
			for(long n=0; n<info.nNumLayers;n++){ delete ppLayers[n]; }
			delete [] ppLayers; ppLayers=0; info.nNumLayers = 0;
		}*/
		// (pSelection) {free(pSelection); pSelection=0;}
		// (pAlpha) {free(pAlpha); pAlpha=0;}
		if (pDib&&m_bInternalRawBitsBuff) { free(pDib); pDib = 0; }
		return true;
	}
	return false;
}
////////////////////////////////////////////////////////////////////////////////
DWORD CImageFile::GetTypeIndexFromId(const DWORD id)
{
	DWORD n;

	n=0; if (id == CXIMAGE_FORMAT_UNKNOWN) return n;
#if CXIMAGE_SUPPORT_BMP
	n++; if (id == CXIMAGE_FORMAT_BMP) return n;
#endif
#if CXIMAGE_SUPPORT_GIF
	n++; if (id == CXIMAGE_FORMAT_GIF) return n;
#endif
#if CXIMAGE_SUPPORT_JPG
	n++; if (id == CXIMAGE_FORMAT_JPG) return n;
#endif
#if CXIMAGE_SUPPORT_PNG
	n++; if (id == CXIMAGE_FORMAT_PNG) return n;
#endif
#if CXIMAGE_SUPPORT_ICO
	n++; if (id == CXIMAGE_FORMAT_ICO) return n;
#endif
#if CXIMAGE_SUPPORT_TIF
	n++; if (id == CXIMAGE_FORMAT_TIF) return n;
#endif
#if CXIMAGE_SUPPORT_TGA
	n++; if (id == CXIMAGE_FORMAT_TGA) return n;
#endif
#if CXIMAGE_SUPPORT_PCX
	n++; if (id == CXIMAGE_FORMAT_PCX) return n;
#endif
#if CXIMAGE_SUPPORT_WBMP
	n++; if (id == CXIMAGE_FORMAT_WBMP) return n;
#endif
#if CXIMAGE_SUPPORT_WMF
	n++; if (id == CXIMAGE_FORMAT_WMF) return n;
#endif
#if CXIMAGE_SUPPORT_JP2
	n++; if (id == CXIMAGE_FORMAT_JP2) return n;
#endif
#if CXIMAGE_SUPPORT_JPC
	n++; if (id == CXIMAGE_FORMAT_JPC) return n;
#endif
#if CXIMAGE_SUPPORT_PGX
	n++; if (id == CXIMAGE_FORMAT_PGX) return n;
#endif
#if CXIMAGE_SUPPORT_PNM
	n++; if (id == CXIMAGE_FORMAT_PNM) return n;
#endif
#if CXIMAGE_SUPPORT_RAS
	n++; if (id == CXIMAGE_FORMAT_RAS) return n;
#endif
#if CXIMAGE_SUPPORT_JBG
	n++; if (id == CXIMAGE_FORMAT_JBG) return n;
#endif
#if CXIMAGE_SUPPORT_MNG
	n++; if (id == CXIMAGE_FORMAT_MNG) return n;
#endif
#if CXIMAGE_SUPPORT_SKA
	n++; if (id == CXIMAGE_FORMAT_SKA) return n;
#endif
#if CXIMAGE_SUPPORT_RAW
	n++; if (id == CXIMAGE_FORMAT_RAW) return n;
#endif

	return 0;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * \sa SetClrImportant
 */
DWORD CImageFile::GetClrImportant() const
{
	return head.biClrImportant;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * sets the maximum number of colors that some functions like
 * DecreaseBpp() or GetNearestIndex() will use on indexed images
 * \param ncolors should be less than 2^bpp,
 * or 0 if all the colors are important.
 */
void CImageFile::SetClrImportant(DWORD ncolors)
{
	if (ncolors==0 || ncolors>256) {
		head.biClrImportant = 0;
		return;
	}

	switch(head.biBitCount){
	case 1:
		head.biClrImportant = min(ncolors,2);
		break;
	case 4:
		head.biClrImportant = min(ncolors,16);
		break;
	case 8:
		head.biClrImportant = ncolors;
		break;
	}
	return;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Transfers the image from an existing source image. The source becomes empty.
 * \return true if everything is ok
 */
bool CImageFile::Transfer(CImageFile &from)//, bool bTransferFrames /*=true*/)
{
	if (!Destroy())
		return false;

	memcpy(&head,&from.head,sizeof(BITMAPINFOHEADER));
	memcpy(&info,&from.info,sizeof(CIMAGEINFO));

	pDib = from.pDib;
	pAlpha = from.pAlpha;
	//pSelection = from.pSelection;
	//ppLayers = from.ppLayers;

	memset(&from.head,0,sizeof(BITMAPINFOHEADER));
	memset(&from.info,0,sizeof(CIMAGEINFO));
	from.pDib = from.pAlpha = NULL;
	//from.pSelection = NULL;
	//from.ppLayers = NULL;

	/*if (bTransferFrames){
		DestroyFrames();
		ppFrames = from.ppFrames;
		from.ppFrames = NULL;
	}*/

	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * \return pointer to the image pixels. <b> USE CAREFULLY </b>
 */
BYTE* CImageFile::GetBits(DWORD row)
{ 
	if (pDib){
		if (row) {
			if (row<(DWORD)head.biHeight){
				return ((BYTE*)pDib + *(DWORD*)pDib + GetPaletteSize() + (info.dwEffWidth * row));
			} else {
				return NULL;
			}
		} else {
			return ((BYTE*)pDib + *(DWORD*)pDib + GetPaletteSize());
		}
	}
	return NULL;
}
DWORD CImageFile::GetBitmap(BITMAP* bitmap)
{
	BYTE* pTo=(BYTE*)bitmap->bmBits;
	////////////////////////////////
	//初始化BITMAP
	if(pTo)
		delete []pTo;
	memset(bitmap,0,sizeof(BITMAP));
	bitmap->bmType=0;
	bitmap->bmPlanes=1;
	bitmap->bmBitsPixel=32;
	bitmap->bmWidth=head.biWidth;
	bitmap->bmHeight=head.biHeight;
	bitmap->bmWidthBytes=head.biWidth*4;
	UINT niBytesCount=bitmap->bmWidthBytes*bitmap->bmHeight;
	bitmap->bmBits=pTo=new BYTE[niBytesCount];
	////////////////////////////////
	const BYTE xarrConstBitBytes[8]={ 0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80 };
	for (int row=0;row<head.biHeight;row++)
	{
		BYTE* pFrom=GetBits(head.biHeight-row-1);	//info.pImage
		for (int xi=0;xi<head.biWidth;xi++)
		{
			if (head.biBitCount==32)
				memcpy(pTo,pFrom+xi*4,4);
			else if (head.biBitCount==24)
			{
				memcpy(pTo,pFrom+xi*3,3);
				*(pTo+3)=255;
			}
			else if (head.biBitCount==1)
			{
				int ibyte=xi/8,ibit=xi%8;
				BYTE cbByte=*(pFrom+ibyte);
				if (cbByte&xarrConstBitBytes[ibit])	//通过异或操作得出dwFlag的补码
					*(pTo+3)=*(pTo+2)=*(pTo+1)=*pTo=255;
				else
					*(pTo+3)=*(pTo+2)=*(pTo+1)=*pTo=0;
			}
			else
				return 0;	//logerr.Log("error");
			pTo+=4;
		}
	}
	return niBytesCount;
}
DWORD CImageFile::GetBitmapBits(DWORD dwCount, BYTE* lpBits)
{
	DWORD dwSummBytes=GetEffWidth()*GetHeight();
	DWORD dwActualRead=dwCount<=dwSummBytes?dwCount:dwSummBytes;
	memcpy(lpBits,GetBits(),dwActualRead);
	return dwActualRead;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * \return the color used for transparency, and/or for background color
 */
RGBQUAD	CImageFile::GetTransColor()
{
	if (head.biBitCount<24 && info.nBkgndIndex>=0) return GetPaletteColor((BYTE)info.nBkgndIndex);
	return info.nBkgndColor;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * returns the pointer to the first palette index
 */
RGBQUAD* CImageFile::GetPalette() const
{
	if ((pDib)&&(head.biClrUsed))
		return (RGBQUAD*)((BYTE*)pDib + sizeof(BITMAPINFOHEADER));
	return NULL;
}
////////////////////////////////////////////////////////////////////////////////
void CImageFile::SetPalette(DWORD n, BYTE *r, BYTE *g, BYTE *b)
{
	if ((!r)||(pDib==NULL)||(head.biClrUsed==0)) return;
	if (!g) g = r;
	if (!b) b = g;
	RGBQUAD* ppal=GetPalette();
	DWORD m=min(n,head.biClrUsed);
	for (DWORD i=0; i<m;i++){
		ppal[i].rgbRed=r[i];
		ppal[i].rgbGreen=g[i];
		ppal[i].rgbBlue=b[i];
	}
	info.last_c_isvalid = false;
}
////////////////////////////////////////////////////////////////////////////////
void CImageFile::SetPalette(rgb_color *rgb,DWORD nColors)
{
	if ((!rgb)||(pDib==NULL)||(head.biClrUsed==0)) return;
	RGBQUAD* ppal=GetPalette();
	DWORD m=min(nColors,head.biClrUsed);
	for (DWORD i=0; i<m;i++){
		ppal[i].rgbRed=rgb[i].r;
		ppal[i].rgbGreen=rgb[i].g;
		ppal[i].rgbBlue=rgb[i].b;
	}
	info.last_c_isvalid = false;
}
////////////////////////////////////////////////////////////////////////////////
void CImageFile::SetPaletteColor(BYTE idx, RGBQUAD c)
{
	if ((pDib)&&(head.biClrUsed)){
		BYTE* iDst = (BYTE*)(pDib) + sizeof(BITMAPINFOHEADER);
		if (idx<head.biClrUsed){
			long ldx=idx*sizeof(RGBQUAD);
			iDst[ldx++] = (BYTE) c.rgbBlue;
			iDst[ldx++] = (BYTE) c.rgbGreen;
			iDst[ldx++] = (BYTE) c.rgbRed;
			iDst[ldx] = (BYTE) c.rgbReserved;
			info.last_c_isvalid = false;
		}
	}
}
////////////////////////////////////////////////////////////////////////////////
void CImageFile::SetPaletteColor(BYTE idx, COLORREF cr)
{
	if ((pDib)&&(head.biClrUsed)){
		BYTE* iDst = (BYTE*)(pDib) + sizeof(BITMAPINFOHEADER);
		if (idx<head.biClrUsed){
			int ldx=idx*sizeof(RGBQUAD);
			iDst[ldx++] = (BYTE) GetBValue(cr);
			iDst[ldx++] = (BYTE) GetGValue(cr);
			iDst[ldx++] = (BYTE) GetRValue(cr);
			iDst[ldx] = (BYTE) 0;
			info.last_c_isvalid = false;
		}
	}
}
////////////////////////////////////////////////////////////////////////////////
void CImageFile::SetPaletteColor(BYTE idx, BYTE r, BYTE g, BYTE b, BYTE alpha)
{
	if ((pDib)&&(head.biClrUsed)){
		BYTE* iDst = (BYTE*)(pDib) + sizeof(BITMAPINFOHEADER);
		if (idx<head.biClrUsed){
			long ldx=idx*sizeof(RGBQUAD);
			iDst[ldx++] = (BYTE) b;
			iDst[ldx++] = (BYTE) g;
			iDst[ldx++] = (BYTE) r;
			iDst[ldx] = (BYTE) alpha;
			info.last_c_isvalid = false;
		}
	}
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Returns the color of the specified index.
 * \param i = palette index
 * \param r, g, b = output color channels
 */
bool CImageFile::GetPaletteColor(BYTE i, BYTE* r, BYTE* g, BYTE* b)
{
	RGBQUAD* ppal=GetPalette();
	if (ppal) {
		*r = ppal[i].rgbRed;
		*g = ppal[i].rgbGreen;
		*b = ppal[i].rgbBlue; 
		return true;
	}
	return false;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Returns the color of the specified index.
 */
RGBQUAD CImageFile::GetPaletteColor(BYTE idx)
{
	RGBQUAD rgb = {0,0,0,0};
	if ((pDib)&&(head.biClrUsed)){
		BYTE* iDst = (BYTE*)(pDib) + sizeof(BITMAPINFOHEADER);
		if (idx<head.biClrUsed){
			long ldx=idx*sizeof(RGBQUAD);
			rgb.rgbBlue = iDst[ldx++];
			rgb.rgbGreen=iDst[ldx++];
			rgb.rgbRed =iDst[ldx++];
			rgb.rgbReserved = iDst[ldx];
		}
	}
	return rgb;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Sets (or replaces) the palette to gray scale palette.
 * The function doesn't change the pixels; for standard
 * gray scale conversion use GrayScale().
 */
void CImageFile::SetGrayPalette()
{
	if ((pDib==NULL)||(head.biClrUsed==0)) return;
	RGBQUAD* pal=GetPalette();
	for (DWORD ni=0;ni<head.biClrUsed;ni++)
		pal[ni].rgbBlue=pal[ni].rgbGreen = pal[ni].rgbRed = (BYTE)(ni*(255/(head.biClrUsed-1)));
}
////////////////////////////////////////////////////////////////////////////////
/**
 * \sa SetCodecOption
 */
DWORD CImageFile::GetCodecOption(DWORD imagetype)
{
	imagetype = GetTypeIndexFromId(imagetype);
	if (imagetype==0){
		imagetype = GetTypeIndexFromId(GetType());
	}
	return info.dwCodecOpt[imagetype];
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Encode option for GIF, TIF and JPG.
 * - GIF : 0 = LZW (default), 1 = none, 2 = RLE.
 * - TIF : 0 = automatic (default), or a valid compression code as defined in "tiff.h" (COMPRESSION_NONE = 1, COMPRESSION_CCITTRLE = 2, ...)
 * - JPG : valid values stored in enum CODEC_OPTION ( ENCODE_BASELINE = 0x01, ENCODE_PROGRESSIVE = 0x10, ...)
 * - RAW : valid values stored in enum CODEC_OPTION ( DECODE_QUALITY_LIN = 0x00, DECODE_QUALITY_VNG = 0x01, ...)
 *
 * \return true if everything is ok
 */
bool CImageFile::SetCodecOption(DWORD opt, DWORD imagetype)
{
	imagetype = GetTypeIndexFromId(imagetype);
	if (imagetype==0){
		imagetype = GetTypeIndexFromId(GetType());
	}
	info.dwCodecOpt[imagetype] = opt;
	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Returns the best palette index that matches a specified color.
 */
BYTE CImageFile::GetNearestIndex(RGBQUAD c)
{
	if ((pDib==NULL)||(head.biClrUsed==0)) return 0;

	// <RJ> check matching with the previous result
	if (info.last_c_isvalid && (*(int*)&info.last_c == *(int*)&c)) return info.last_c_index;
	info.last_c = c;
	info.last_c_isvalid = true;

	BYTE* iDst = (BYTE*)(pDib) + sizeof(BITMAPINFOHEADER);
	int distance=200000;
	int i,j = 0;
	int k,l;
	int m = (int)(head.biClrImportant==0 ? head.biClrUsed : head.biClrImportant);
	for(i=0,l=0;i<m;i++,l+=sizeof(RGBQUAD)){
		k = (iDst[l]-c.rgbBlue)*(iDst[l]-c.rgbBlue)+
			(iDst[l+1]-c.rgbGreen)*(iDst[l+1]-c.rgbGreen)+
			(iDst[l+2]-c.rgbRed)*(iDst[l+2]-c.rgbRed);
//		k = abs(iDst[l]-c.rgbBlue)+abs(iDst[l+1]-c.rgbGreen)+abs(iDst[l+2]-c.rgbRed);
		if (k==0){
			j=i;
			break;
		}
		if (k<distance){
			distance=k;
			j=i;
		}
	}
	info.last_c_index = (BYTE)j;
	return (BYTE)j;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Returns the palette index of the specified pixel.
 */
BYTE CImageFile::GetPixelIndex(int x,int y)
{
	if ((pDib==NULL)||(head.biClrUsed==0)) return 0;

	if ((x<0)||(y<0)||(x>=head.biWidth)||(y>=head.biHeight)) {
		if (info.nBkgndIndex >= 0)	return (BYTE)info.nBkgndIndex;
		else return *info.pImage;
	}
	if (head.biBitCount==8){
		return info.pImage[y*info.dwEffWidth + x];
	} else {
		BYTE pos;
		BYTE iDst= info.pImage[y*info.dwEffWidth + (x*head.biBitCount >> 3)];
		if (head.biBitCount==4){
			pos = (BYTE)(4*(1-x%2));
			iDst &= (0x0F<<pos);
			return (BYTE)(iDst >> pos);
		} else if (head.biBitCount==1){
			pos = (BYTE)(7-x%8);
			iDst &= (0x01<<pos);
			return (BYTE)(iDst >> pos);
		}
	}
	return 0;
}
void CImageFile::SetPixelIndex(int x,int y,BYTE i)
{
	if ((pDib==NULL)||(head.biClrUsed==0)||
		(x<0)||(y<0)||(x>=head.biWidth)||(y>=head.biHeight)) return ;

	if (head.biBitCount==8){
		info.pImage[y*info.dwEffWidth + x]=i;
		return;
	}
	else {
		BYTE pos;
		BYTE* iDst= info.pImage + y*info.dwEffWidth + (x*head.biBitCount >> 3);
		if (head.biBitCount==4){
			pos = (BYTE)(4*(1-x%2));
			*iDst &= ~(0x0F<<pos);
			*iDst |= ((i & 0x0F)<<pos);
			return;
		} else if (head.biBitCount==1){
			pos = (BYTE)(7-x%8);
			*iDst &= ~(0x01<<pos);
			*iDst |= ((i & 0x01)<<pos);
			return;
		}
	}
}
void CImageFile::SetPixelColor(int x,int y,RGBQUAD c,bool bSetAlpha/* = false*/)
{
	if ((pDib==NULL)||(x<0)||(y<0)||
		(x>=head.biWidth)||(y>=head.biHeight)) return;
	if (head.biClrUsed)
		BlindSetPixelIndex(x,y,GetNearestIndex(c));
	else {
		BYTE* iDst = info.pImage + y*info.dwEffWidth + x*3;
		*iDst++ = c.rgbBlue;
		*iDst++ = c.rgbGreen;
		*iDst   = c.rgbRed;
	}
#if CImageFile_SUPPORT_ALPHA
	if (bSetAlpha) AlphaSet(x,y,c.rgbReserved);
#endif //CImageFile_SUPPORT_ALPHA
}
void CImageFile::SetPixelColor(int x,int y,COLORREF cr)
{
	SetPixelColor(x,y,RGBtoRGBQUAD(cr));
}
bool CImageFile::EncodeSafeCheck(FILE *fp)
{
	if (fp==NULL) {
		strcpy(info.szLastError,"CXIMAGE_ERR_NOFILE");
		return true;
	}

	if (pDib==NULL){
		strcpy(info.szLastError,"CXIMAGE_ERR_NOIMAGE");
		return true;
	}
	return false;
}
////////////////////////////////////////////////////////////////////////////////
bool CImageFile::GrayScale()
{
	/*if (!pDib) return false;
	if (head.biBitCount<=8){
		RGBQUAD* ppal=GetPalette();
		int gray;
		//converts the colors to gray, use the blue channel only
		for(UINT i=0;i<head.biClrUsed;i++){
			gray=(int)RGB2GRAY(ppal[i].rgbRed,ppal[i].rgbGreen,ppal[i].rgbBlue);
			ppal[i].rgbBlue = (BYTE)gray;
		}
		// preserve transparency
		if (info.nBkgndIndex >= 0) info.nBkgndIndex = ppal[info.nBkgndIndex].rgbBlue;
		//create a "real" 8 bit gray scale image
		if (head.biBitCount==8){
			BYTE *img=info.pImage;
			for(UINT i=0;i<head.biSizeImage;i++) img[i]=ppal[img[i]].rgbBlue;
			SetGrayPalette();
		}
		//transform to 8 bit gray scale
		if (head.biBitCount==4 || head.biBitCount==1){
			CxImage ima;
			ima.CopyInfo(*this);
			if (!ima.Create(head.biWidth,head.biHeight,8,info.dwType)) return false;
			ima.SetGrayPalette();
#if CXIMAGE_SUPPORT_SELECTION
			ima.SelectionCopy(*this);
#endif //CXIMAGE_SUPPORT_SELECTION
#if CXIMAGE_SUPPORT_ALPHA
			ima.AlphaCopy(*this);
#endif //CXIMAGE_SUPPORT_ALPHA
			for (int y=0;y<head.biHeight;y++){
				BYTE *iDst = ima.GetBits(y);
				BYTE *iSrc = GetBits(y);
				for (int x=0;x<head.biWidth; x++){
					//iDst[x]=ppal[BlindGetPixelIndex(x,y)].rgbBlue;
					if (head.biBitCount==4){
						BYTE pos = (BYTE)(4*(1-x%2));
						iDst[x]= ppal[(BYTE)((iSrc[x >> 1]&((BYTE)0x0F<<pos)) >> pos)].rgbBlue;
					} else {
						BYTE pos = (BYTE)(7-x%8);
						iDst[x]= ppal[(BYTE)((iSrc[x >> 3]&((BYTE)0x01<<pos)) >> pos)].rgbBlue;
					}
				}
			}
			Transfer(ima);
		}
	} else { //from RGB to 8 bit gray scale
		BYTE *iSrc=info.pImage;
		CxImage ima;
		ima.CopyInfo(*this);
		if (!ima.Create(head.biWidth,head.biHeight,8,info.dwType)) return false;
		ima.SetGrayPalette();
		if (GetTransIndex()>=0){
			RGBQUAD c = GetTransColor();
			ima.SetTransIndex((BYTE)RGB2GRAY(c.rgbRed,c.rgbGreen,c.rgbBlue));
		}
#if CXIMAGE_SUPPORT_SELECTION
		ima.SelectionCopy(*this);
#endif //CXIMAGE_SUPPORT_SELECTION
#if CXIMAGE_SUPPORT_ALPHA
		ima.AlphaCopy(*this);
#endif //CXIMAGE_SUPPORT_ALPHA
		BYTE *img=ima.GetBits();
		int l8=ima.GetEffWidth();
		int l=head.biWidth * 3;
		for(int y=0; y < head.biHeight; y++) {
			for(int x=0,x8=0; x < l; x+=3,x8++) {
				img[x8+y*l8]=(BYTE)RGB2GRAY(*(iSrc+x+2),*(iSrc+x+1),*(iSrc+x+0));
			}
			iSrc+=info.dwEffWidth;
		}
		Transfer(ima);
	}*/
	return true;
}
bool CImageFile::Flip(bool bFlipSelection/* = false*/,bool bFlipAlpha/* = true*/)
{
	if (!pDib) return false;

	BYTE *buff = (BYTE*)malloc(info.dwEffWidth);
	if (!buff) return false;

	BYTE *iSrc,*iDst;
	iSrc = GetBits(head.biHeight-1);
	iDst = GetBits(0);
	for (int i=0; i<(head.biHeight/2); ++i)
	{
		memcpy(buff, iSrc, info.dwEffWidth);
		memcpy(iSrc, iDst, info.dwEffWidth);
		memcpy(iDst, buff, info.dwEffWidth);
		iSrc-=info.dwEffWidth;
		iDst+=info.dwEffWidth;
	}

	free(buff);

	if (bFlipSelection){
#if CXIMAGE_SUPPORT_SELECTION
		SelectionFlip();
#endif //CXIMAGE_SUPPORT_SELECTION
	}

	if (bFlipAlpha){
#if CXIMAGE_SUPPORT_ALPHA
		AlphaFlip();
#endif //CXIMAGE_SUPPORT_ALPHA
	}

	return true;
}
bool CImageFile::Mirror(bool bMirrorSelection/* = false*/,bool bMirrorAlpha/* = true*/)
{
#ifdef __FINISHED_
	if (!pDib) return false;

	CImageFile* imatmp = new CImageFile(*this,false,true,true);
	if (!imatmp) return false;
	if (!imatmp->IsValid()){
		delete imatmp;
		return false;
	}

	BYTE *iSrc,*iDst;
	int wdt=(head.biWidth-1) * (head.biBitCount==24 ? 3:1);
	iSrc=info.pImage + wdt;
	iDst=imatmp->info.pImage;
	int x,y;
	switch (head.biBitCount){
	case 24:
		for(y=0; y < head.biHeight; y++){
			for(x=0; x <= wdt; x+=3){
				*(iDst+x)=*(iSrc-x);
				*(iDst+x+1)=*(iSrc-x+1);
				*(iDst+x+2)=*(iSrc-x+2);
			}
			iSrc+=info.dwEffWidth;
			iDst+=info.dwEffWidth;
		}
		break;
	case 8:
		for(y=0; y < head.biHeight; y++){
			for(x=0; x <= wdt; x++)
				*(iDst+x)=*(iSrc-x);
			iSrc+=info.dwEffWidth;
			iDst+=info.dwEffWidth;
		}
		break;
	default:
		for(y=0; y < head.biHeight; y++){
			for(x=0; x <= wdt; x++)
				imatmp->SetPixelIndex(x,y,GetPixelIndex(wdt-x,y));
		}
	}

	if (bMirrorSelection){
#if CXIMAGE_SUPPORT_SELECTION
		imatmp->SelectionMirror();
#endif //CXIMAGE_SUPPORT_SELECTION
	}

	if (bMirrorAlpha){
#if CXIMAGE_SUPPORT_ALPHA
		imatmp->AlphaMirror();
#endif //CXIMAGE_SUPPORT_ALPHA
	}

	Transfer(*imatmp);
	delete imatmp;
#endif
	return true;
}
bool CImageFile::Negative()
{
#ifdef __FINISHED_
	if (!pDib) return false;

	if (head.biBitCount<=8){
		if (IsGrayScale())
		{ //GRAYSCALE, selection
			/*if (pSelection){
				for(int y=info.rSelectionBox.bottom; y<info.rSelectionBox.top; y++){
					for(int x=info.rSelectionBox.left; x<info.rSelectionBox.right; x++){
#if CXIMAGE_SUPPORT_SELECTION
						if (BlindSelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
						{
							BlindSetPixelIndex(x,y,(BYTE)(255-BlindGetPixelIndex(x,y)));
						}
					}
				}
			} 
			else */
			{
				BYTE *iSrc=info.pImage;
				for(UINT i=0; i < head.biSizeImage; i++){
					*iSrc=(BYTE)~(*(iSrc));
					iSrc++;
				}
			}
		} else { //PALETTE, full image
			RGBQUAD* ppal=GetPalette();
			for(UINT i=0;i<head.biClrUsed;i++){
				ppal[i].rgbBlue =(BYTE)(255-ppal[i].rgbBlue);
				ppal[i].rgbGreen =(BYTE)(255-ppal[i].rgbGreen);
				ppal[i].rgbRed =(BYTE)(255-ppal[i].rgbRed);
			}
		}
	}
	else
	{
		/*if (pSelection==NULL){ //RGB, full image
			BYTE *iSrc=info.pImage;
			for(UINT i=0; i < head.biSizeImage; i++){
				*iSrc=(BYTE)~(*(iSrc));
				iSrc++;
			}
		} 
		else */
		{ // RGB with selection
			RGBQUAD color;
			for(int y=info.rSelectionBox.bottom; y<info.rSelectionBox.top; y++){
				for(int x=info.rSelectionBox.left; x<info.rSelectionBox.right; x++){
#if CXIMAGE_SUPPORT_SELECTION
					if (BlindSelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
					{
						color = BlindGetPixelColor(x,y);
						color.rgbRed = (BYTE)(255-color.rgbRed);
						color.rgbGreen = (BYTE)(255-color.rgbGreen);
						color.rgbBlue = (BYTE)(255-color.rgbBlue);
						BlindSetPixelColor(x,y,color);
					}
				}
			}
		}
		//<DP> invert transparent color too
		info.nBkgndColor.rgbBlue = (BYTE)(255-info.nBkgndColor.rgbBlue);
		info.nBkgndColor.rgbGreen = (BYTE)(255-info.nBkgndColor.rgbGreen);
		info.nBkgndColor.rgbRed = (BYTE)(255-info.nBkgndColor.rgbRed);
	}
#endif
	return true;
}
bool CImageFile::RotateLeft(CImageFile* iDst/* = NULL*/)
{
#ifdef __FINISHED_
	if (!pDib) return false;

	int newWidth = GetHeight();
	int newHeight = GetWidth();

	CImageFile imgDest;
	imgDest.CopyInfo(*this);
	imgDest.Create(newWidth,newHeight,GetBpp(),GetType());
	imgDest.SetPalette(GetPalette());

#if CXIMAGE_SUPPORT_ALPHA
	if (AlphaIsValid()) imgDest.AlphaCreate();
#endif

#if CXIMAGE_SUPPORT_SELECTION
	if (SelectionIsValid()) imgDest.SelectionCreate();
#endif

	int x,x2,y,dlineup;
	
	// Speedy rotate for BW images <Robert Abram>
	if (head.biBitCount == 1) {
	
		BYTE *sbits, *dbits, *dbitsmax, bitpos, *nrow,*srcdisp;
		ldiv_t div_r;

		BYTE *bsrc = GetBits(), *bdest = imgDest.GetBits();
		dbitsmax = bdest + imgDest.head.biSizeImage - 1;
		dlineup = 8 * imgDest.info.dwEffWidth - imgDest.head.biWidth;

		imgDest.Clear(0);
		for (y = 0; y < head.biHeight; y++) {
			// Figure out the Column we are going to be copying to
			div_r = ldiv(y + dlineup, (int)8);
			// set bit pos of src column byte				
			bitpos = (BYTE)(1 << div_r.rem);
			srcdisp = bsrc + y * info.dwEffWidth;
			for (x = 0; x < (int)info.dwEffWidth; x++) {
				// Get Source Bits
				sbits = srcdisp + x;
				// Get destination column
				nrow = bdest + (x * 8) * imgDest.info.dwEffWidth + imgDest.info.dwEffWidth - 1 - div_r.quot;
				for (int z = 0; z < 8; z++) {
				   // Get Destination Byte
					dbits = nrow + z * imgDest.info.dwEffWidth;
					if ((dbits < bdest) || (dbits > dbitsmax)) break;
					if (*sbits & (128 >> z)) *dbits |= bitpos;
				}
			}
		}//for y

#if CXIMAGE_SUPPORT_ALPHA
		if (AlphaIsValid()) {
			for (x = 0; x < newWidth; x++){
				x2=newWidth-x-1;
				for (y = 0; y < newHeight; y++){
					imgDest.AlphaSet(x,y,BlindAlphaGet(y, x2));
				}//for y
			}//for x
		}
#endif //CXIMAGE_SUPPORT_ALPHA

#if CXIMAGE_SUPPORT_SELECTION
		if (SelectionIsValid()) {
			imgDest.info.rSelectionBox.left = newWidth-info.rSelectionBox.top;
			imgDest.info.rSelectionBox.right = newWidth-info.rSelectionBox.bottom;
			imgDest.info.rSelectionBox.bottom = info.rSelectionBox.left;
			imgDest.info.rSelectionBox.top = info.rSelectionBox.right;
			for (x = 0; x < newWidth; x++){
				x2=newWidth-x-1;
				for (y = 0; y < newHeight; y++){
					imgDest.SelectionSet(x,y,BlindSelectionGet(y, x2));
				}//for y
			}//for x
		}
#endif //CXIMAGE_SUPPORT_SELECTION

	} else {
	//anything other than BW:
	//bd, 10. 2004: This optimized version of rotation rotates image by smaller blocks. It is quite
	//a bit faster than obvious algorithm, because it produces much less CPU cache misses.
	//This optimization can be tuned by changing block size (RBLOCK). 96 is good value for current
	//CPUs (tested on Athlon XP and Celeron D). Larger value (if CPU has enough cache) will increase
	//speed somehow, but once you drop out of CPU's cache, things will slow down drastically.
	//For older CPUs with less cache, lower value would yield better results.
		
		BYTE *srcPtr, *dstPtr;                        //source and destionation for 24-bit version
		int xs, ys;                                   //x-segment and y-segment
		for (xs = 0; xs < newWidth; xs+=RBLOCK) {       //for all image blocks of RBLOCK*RBLOCK pixels
			for (ys = 0; ys < newHeight; ys+=RBLOCK) {
				if (head.biBitCount==24) {
					//RGB24 optimized pixel access:
					for (x = xs; x < min(newWidth, xs+RBLOCK); x++){    //do rotation
						info.nProgress = (int)(100*x/newWidth);
						x2=newWidth-x-1;
						dstPtr = (BYTE*) imgDest.BlindGetPixelPointer(x,ys);
						srcPtr = (BYTE*) BlindGetPixelPointer(ys, x2);
						for (y = ys; y < min(newHeight, ys+RBLOCK); y++){
							//imgDest.SetPixelColor(x, y, GetPixelColor(y, x2));
							*(dstPtr) = *(srcPtr);
							*(dstPtr+1) = *(srcPtr+1);
							*(dstPtr+2) = *(srcPtr+2);
							srcPtr += 3;
							dstPtr += imgDest.info.dwEffWidth;
						}//for y
					}//for x
				} else {
					//anything else than 24bpp (and 1bpp): palette
					for (x = xs; x < min(newWidth, xs+RBLOCK); x++){
						info.nProgress = (int)(100*x/newWidth); //<Anatoly Ivasyuk>
						x2=newWidth-x-1;
						for (y = ys; y < min(newHeight, ys+RBLOCK); y++){
							imgDest.SetPixelIndex(x, y, BlindGetPixelIndex(y, x2));
						}//for y
					}//for x
				}//if (version selection)
#if CXIMAGE_SUPPORT_ALPHA
				if (AlphaIsValid()) {
					for (x = xs; x < min(newWidth, xs+RBLOCK); x++){
						x2=newWidth-x-1;
						for (y = ys; y < min(newHeight, ys+RBLOCK); y++){
							imgDest.AlphaSet(x,y,BlindAlphaGet(y, x2));
						}//for y
					}//for x
				}//if (alpha channel)
#endif //CXIMAGE_SUPPORT_ALPHA

#if CXIMAGE_SUPPORT_SELECTION
				if (SelectionIsValid()) {
					imgDest.info.rSelectionBox.left = newWidth-info.rSelectionBox.top;
					imgDest.info.rSelectionBox.right = newWidth-info.rSelectionBox.bottom;
					imgDest.info.rSelectionBox.bottom = info.rSelectionBox.left;
					imgDest.info.rSelectionBox.top = info.rSelectionBox.right;
					for (x = xs; x < min(newWidth, xs+RBLOCK); x++){
						x2=newWidth-x-1;
						for (y = ys; y < min(newHeight, ys+RBLOCK); y++){
							imgDest.SelectionSet(x,y,BlindSelectionGet(y, x2));
						}//for y
					}//for x
				}//if (selection)
#endif //CXIMAGE_SUPPORT_SELECTION
			}//for ys
		}//for xs
	}//if

	//select the destination
	if (iDst) iDst->Transfer(imgDest);
	else Transfer(imgDest);
#endif
	return true;
}
bool CImageFile::RotateRight(CImageFile* iDst/* = NULL*/)
{
#ifdef __FINISHED_
	if (!pDib) return false;

	int newWidth = GetHeight();
	int newHeight = GetWidth();

	CImageFile imgDest;
	imgDest.CopyInfo(*this);
	imgDest.Create(newWidth,newHeight,GetBpp(),GetType());
	imgDest.SetPalette(GetPalette());

#if CXIMAGE_SUPPORT_ALPHA
	if (AlphaIsValid()) imgDest.AlphaCreate();
#endif

#if CXIMAGE_SUPPORT_SELECTION
	if (SelectionIsValid()) imgDest.SelectionCreate();
#endif

	int x,y,y2;
	// Speedy rotate for BW images <Robert Abram>
	if (head.biBitCount == 1) {
	
		BYTE *sbits, *dbits, *dbitsmax, bitpos, *nrow,*srcdisp;
		ldiv_t div_r;

		BYTE *bsrc = GetBits(), *bdest = imgDest.GetBits();
		dbitsmax = bdest + imgDest.head.biSizeImage - 1;

		imgDest.Clear(0);
		for (y = 0; y < head.biHeight; y++) {
			// Figure out the Column we are going to be copying to
			div_r = ldiv(y, (int)8);
			// set bit pos of src column byte				
			bitpos = (BYTE)(128 >> div_r.rem);
			srcdisp = bsrc + y * info.dwEffWidth;
			for (x = 0; x < (int)info.dwEffWidth; x++) {
				// Get Source Bits
				sbits = srcdisp + x;
				// Get destination column
				nrow = bdest + (imgDest.head.biHeight-1-(x*8)) * imgDest.info.dwEffWidth + div_r.quot;
				for (int z = 0; z < 8; z++) {
				   // Get Destination Byte
					dbits = nrow - z * imgDest.info.dwEffWidth;
					if ((dbits < bdest) || (dbits > dbitsmax)) break;
					if (*sbits & (128 >> z)) *dbits |= bitpos;
				}
			}
		}

#if CXIMAGE_SUPPORT_ALPHA
		if (AlphaIsValid()){
			for (y = 0; y < newHeight; y++){
				y2=newHeight-y-1;
				for (x = 0; x < newWidth; x++){
					imgDest.AlphaSet(x,y,BlindAlphaGet(y2, x));
				}
			}
		}
#endif //CXIMAGE_SUPPORT_ALPHA

#if CXIMAGE_SUPPORT_SELECTION
		if (SelectionIsValid()){
			imgDest.info.rSelectionBox.left = info.rSelectionBox.bottom;
			imgDest.info.rSelectionBox.right = info.rSelectionBox.top;
			imgDest.info.rSelectionBox.bottom = newHeight-info.rSelectionBox.right;
			imgDest.info.rSelectionBox.top = newHeight-info.rSelectionBox.left;
			for (y = 0; y < newHeight; y++){
				y2=newHeight-y-1;
				for (x = 0; x < newWidth; x++){
					imgDest.SelectionSet(x,y,BlindSelectionGet(y2, x));
				}
			}
		}
#endif //CXIMAGE_SUPPORT_SELECTION

	} else {
		//anything else but BW
		BYTE *srcPtr, *dstPtr;                        //source and destionation for 24-bit version
		int xs, ys;                                   //x-segment and y-segment
		for (xs = 0; xs < newWidth; xs+=RBLOCK) {
			for (ys = 0; ys < newHeight; ys+=RBLOCK) {
				if (head.biBitCount==24) {
					//RGB24 optimized pixel access:
					for (y = ys; y < min(newHeight, ys+RBLOCK); y++){
						info.nProgress = (int)(100*y/newHeight); //<Anatoly Ivasyuk>
						y2=newHeight-y-1;
						dstPtr = (BYTE*) imgDest.BlindGetPixelPointer(xs,y);
						srcPtr = (BYTE*) BlindGetPixelPointer(y2, xs);
						for (x = xs; x < min(newWidth, xs+RBLOCK); x++){
							//imgDest.SetPixelColor(x, y, GetPixelColor(y2, x));
							*(dstPtr) = *(srcPtr);
							*(dstPtr+1) = *(srcPtr+1);
							*(dstPtr+2) = *(srcPtr+2);
							dstPtr += 3;
							srcPtr += info.dwEffWidth;
						}//for x
					}//for y
				} else {
					//anything else than BW & RGB24: palette
					for (y = ys; y < min(newHeight, ys+RBLOCK); y++){
						info.nProgress = (int)(100*y/newHeight); //<Anatoly Ivasyuk>
						y2=newHeight-y-1;
						for (x = xs; x < min(newWidth, xs+RBLOCK); x++){
							imgDest.SetPixelIndex(x, y, BlindGetPixelIndex(y2, x));
						}//for x
					}//for y
				}//if
#if CXIMAGE_SUPPORT_ALPHA
				if (AlphaIsValid()){
					for (y = ys; y < min(newHeight, ys+RBLOCK); y++){
						y2=newHeight-y-1;
						for (x = xs; x < min(newWidth, xs+RBLOCK); x++){
							imgDest.AlphaSet(x,y,BlindAlphaGet(y2, x));
						}//for x
					}//for y
				}//if (has alpha)
#endif //CXIMAGE_SUPPORT_ALPHA

#if CXIMAGE_SUPPORT_SELECTION
				if (SelectionIsValid()){
					imgDest.info.rSelectionBox.left = info.rSelectionBox.bottom;
					imgDest.info.rSelectionBox.right = info.rSelectionBox.top;
					imgDest.info.rSelectionBox.bottom = newHeight-info.rSelectionBox.right;
					imgDest.info.rSelectionBox.top = newHeight-info.rSelectionBox.left;
					for (y = ys; y < min(newHeight, ys+RBLOCK); y++){
						y2=newHeight-y-1;
						for (x = xs; x < min(newWidth, xs+RBLOCK); x++){
							imgDest.SelectionSet(x,y,BlindSelectionGet(y2, x));
						}//for x
					}//for y
				}//if (has alpha)
#endif //CXIMAGE_SUPPORT_SELECTION
			}//for ys
		}//for xs
	}//if

	//select the destination
	if (iDst) iDst->Transfer(imgDest);
	else Transfer(imgDest);
#endif
	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Returns true if the image has 256 colors and a linear grey scale palette.
 */
bool CImageFile::IsGrayScale()
{
	RGBQUAD* ppal=GetPalette();
	if(!(pDib && ppal && head.biClrUsed)) return false;
	for(DWORD i=0;i<head.biClrUsed;i++){
		if (ppal[i].rgbBlue!=i || ppal[i].rgbGreen!=i || ppal[i].rgbRed!=i) return false;
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
RGBQUAD CImageFile::GetPixelColor(int x,int y, bool bGetAlpha)
{
//	RGBQUAD rgb={0,0,0,0};
	RGBQUAD rgb=info.nBkgndColor; //<mpwolski>
	if ((pDib==NULL)||(x<0)||(y<0)||
		(x>=head.biWidth)||(y>=head.biHeight))
	{
		if (info.nBkgndIndex >= 0)
		{
			if (head.biBitCount<24)
				return GetPaletteColor((BYTE)info.nBkgndIndex);
			else
				return info.nBkgndColor;
		}
		else if (pDib)
			return GetPixelColor(0,0,bGetAlpha);
		return rgb;
	}

	if (head.biClrUsed){
		rgb = GetPaletteColor(BlindGetPixelIndex(x,y));
	}
	else {
		BYTE* iDst  = info.pImage + y*info.dwEffWidth + x*3;
		rgb.rgbBlue = *iDst++;
		rgb.rgbGreen= *iDst++;
		rgb.rgbRed  = *iDst;
	}
#if CImageFile_SUPPORT_ALPHA
	if (pAlpha && bGetAlpha) rgb.rgbReserved = BlindAlphaGet(x,y);
#else
	rgb.rgbReserved = 0;
#endif //CImageFile_SUPPORT_ALPHA
	return rgb;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * swap two indexes in the image and their colors in the palette
 */
void CImageFile::SwapIndex(BYTE idx1, BYTE idx2)
{
	RGBQUAD* ppal=GetPalette();
	if(!(pDib && ppal)) return;
	//swap the colors
	RGBQUAD tempRGB=GetPaletteColor(idx1);
	SetPaletteColor(idx1,GetPaletteColor(idx2));
	SetPaletteColor(idx2,tempRGB);
	//swap the pixels
	BYTE idx;
	for(long y=0; y < head.biHeight; y++){
		for(long x=0; x < head.biWidth; x++){
			idx=BlindGetPixelIndex(x,y);
			if (idx==idx1) BlindSetPixelIndex(x,y,idx2);
			if (idx==idx2) BlindSetPixelIndex(x,y,idx1);
		}
	}
}
////////////////////////////////////////////////////////////////////////////////
BYTE CImageFile::BlindGetPixelIndex(const long x,const long y)
{
#ifdef _DEBUG
	if ((pDib==NULL) || (head.biClrUsed==0) || !IsInside(x,y))
  #if CXIMAGE_SUPPORT_EXCEPTION_HANDLING
		throw 0;
  #else
		return 0;
  #endif
#endif

	if (head.biBitCount==8){
		return info.pImage[y*info.dwEffWidth + x];
	} else {
		BYTE pos;
		BYTE iDst= info.pImage[y*info.dwEffWidth + (x*head.biBitCount >> 3)];
		if (head.biBitCount==4){
			pos = (BYTE)(4*(1-x%2));
			iDst &= (0x0F<<pos);
			return (BYTE)(iDst >> pos);
		} else if (head.biBitCount==1){
			pos = (BYTE)(7-x%8);
			iDst &= (0x01<<pos);
			return (BYTE)(iDst >> pos);
		}
	}
	return 0;
}
////////////////////////////////////////////////////////////////////////////////
void CImageFile::BlindSetPixelIndex(long x,long y,BYTE i)
{
#ifdef _DEBUG
	if ((pDib==NULL)||(head.biClrUsed==0)||
		(x<0)||(y<0)||(x>=head.biWidth)||(y>=head.biHeight))
  #if CXIMAGE_SUPPORT_EXCEPTION_HANDLING
		throw 0;
  #else
		return;
  #endif
#endif

	if (head.biBitCount==8){
		info.pImage[y*info.dwEffWidth + x]=i;
		return;
	} else {
		BYTE pos;
		BYTE* iDst= info.pImage + y*info.dwEffWidth + (x*head.biBitCount >> 3);
		if (head.biBitCount==4){
			pos = (BYTE)(4*(1-x%2));
			*iDst &= ~(0x0F<<pos);
			*iDst |= ((i & 0x0F)<<pos);
			return;
		} else if (head.biBitCount==1){
			pos = (BYTE)(7-x%8);
			*iDst &= ~(0x01<<pos);
			*iDst |= ((i & 0x01)<<pos);
			return;
		}
	}
}
////////////////////////////////////////////////////////////////////////////////
/**
 * This is (a bit) faster version of GetPixelColor. 
 * It tests bounds only in debug mode (_DEBUG defined).
 * 
 * It is an error to request out-of-borders pixel with this method. 
 * In DEBUG mode an exception will be thrown, and data will be violated in non-DEBUG mode. 
 * \author ***bd*** 2.2004
 */
RGBQUAD CImageFile::BlindGetPixelColor(const long x,const long y, bool bGetAlpha)
{
  RGBQUAD rgb;
#ifdef _DEBUG
	if ((pDib==NULL) || !IsInside(x,y))
  #if CXIMAGE_SUPPORT_EXCEPTION_HANDLING
		throw 0;
  #else
		{rgb.rgbReserved = 0; return rgb;}
  #endif
#endif

	if (head.biClrUsed){
		rgb = GetPaletteColor(BlindGetPixelIndex(x,y));
	} else {
		BYTE* iDst  = info.pImage + y*info.dwEffWidth + x*3;
		rgb.rgbBlue = *iDst++;
		rgb.rgbGreen= *iDst++;
		rgb.rgbRed  = *iDst;
		rgb.rgbReserved = 0; //needed for images without alpha layer
	}
#if CXIMAGE_SUPPORT_ALPHA
	if (pAlpha && bGetAlpha) rgb.rgbReserved = BlindAlphaGet(x,y);
#else
	rgb.rgbReserved = 0;
#endif //CXIMAGE_SUPPORT_ALPHA
	return rgb;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Gets the alpha level for a single pixel 
 */
BYTE CImageFile::AlphaGet(const long x,const long y)
{
	if (pAlpha && IsInside(x,y)) return pAlpha[x+y*head.biWidth];
	return 0;
}
