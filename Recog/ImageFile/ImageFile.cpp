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
	m_bInternalRawBitsBuff=true;
}

CImageFile::~CImageFile()
{
	if(pDib&&m_bInternalRawBitsBuff)
		delete []pDib;
}
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
/**
 * Initializes or rebuilds the image.
 * \param dwWidth: width
 * \param dwHeight: height
 * \param wBpp: bit per pixel, can be 1, 4, 8, 24
 * \param imagetype: (optional) set the image format, see ENUM_CXIMAGE_FORMATS
 * \return pointer to the internal pDib object; NULL if an error occurs.
 */
void* CImageFile::Create(DWORD dwWidth, DWORD dwHeight, DWORD wBpp, DWORD imagetype,BYTE* lpExterRawBitsBuff/*=NULL*/,UINT uiBitsBuffSize/*=0*/)
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

	if(lpExterRawBitsBuff!=NULL&&uiBitsBuffSize>=(UINT)GetSize())
	{
		pDib=lpExterRawBitsBuff;
		m_bInternalRawBitsBuff=false;
	}
	else if(lpExterRawBitsBuff!=NULL)
		return NULL;	//Ô¤ÁôÍâ»º´æ²»×ã
	else
	{
		m_bInternalRawBitsBuff=true;
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
		if (pDib&&m_bInternalRawBitsBuff) {free(pDib); pDib=0;}
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
DWORD CImageFile::GetBitmapBits(DWORD dwCount, BYTE* lpBits)
{
	DWORD dwActualRead;
	if(dwCount<=GetEffWidth()*GetHeight())
		dwActualRead=dwCount;
	else
		dwActualRead=GetEffWidth()*GetHeight();
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
