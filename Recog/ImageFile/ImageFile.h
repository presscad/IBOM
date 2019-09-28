// ImageFile.h: interface for the CImageFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IMAGEFILE_H__01662188_548F_4D9A_B515_99F84A94B1D1__INCLUDED_)
#define AFX_IMAGEFILE_H__01662188_548F_4D9A_B515_99F84A94B1D1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CxImage supported formats
#define CXIMAGE_SUPPORT_BMP 0
#define CXIMAGE_SUPPORT_GIF 0
#define CXIMAGE_SUPPORT_JPG 1
#define CXIMAGE_SUPPORT_PNG 1
#define CXIMAGE_SUPPORT_ICO 0
#define CXIMAGE_SUPPORT_TIF 0
#define CXIMAGE_SUPPORT_TGA 0
#define CXIMAGE_SUPPORT_PCX 0
#define CXIMAGE_SUPPORT_WBMP 0
#define CXIMAGE_SUPPORT_WMF 0

#define CXIMAGE_SUPPORT_JP2 0
#define CXIMAGE_SUPPORT_JPC 0
#define CXIMAGE_SUPPORT_PGX 0
#define CXIMAGE_SUPPORT_PNM 0
#define CXIMAGE_SUPPORT_RAS 0

#define CXIMAGE_SUPPORT_JBG 0		// GPL'd see ../jbig/copying.txt & ../jbig/patents.htm

#define CXIMAGE_SUPPORT_MNG 0
#define CXIMAGE_SUPPORT_SKA 0
#define CXIMAGE_SUPPORT_RAW 0
/////////////////////////////////////////////////////////////////////////////
#define	CXIMAGE_MAX_MEMORY 268435456

// CxImage formats enumerator
enum ENUM_CXIMAGE_FORMATS{
CXIMAGE_FORMAT_UNKNOWN = 0,
#if CXIMAGE_SUPPORT_BMP
CXIMAGE_FORMAT_BMP = 1,
#endif
#if CXIMAGE_SUPPORT_GIF
CXIMAGE_FORMAT_GIF = 2,
#endif
#if CXIMAGE_SUPPORT_JPG
CXIMAGE_FORMAT_JPG = 3,
#endif
#if CXIMAGE_SUPPORT_PNG
CXIMAGE_FORMAT_PNG = 4,
#endif
#if CXIMAGE_SUPPORT_ICO
CXIMAGE_FORMAT_ICO = 5,
#endif
#if CXIMAGE_SUPPORT_TIF
CXIMAGE_FORMAT_TIF = 6,
#endif
#if CXIMAGE_SUPPORT_TGA
CXIMAGE_FORMAT_TGA = 7,
#endif
#if CXIMAGE_SUPPORT_PCX
CXIMAGE_FORMAT_PCX = 8,
#endif
#if CXIMAGE_SUPPORT_WBMP
CXIMAGE_FORMAT_WBMP = 9,
#endif
#if CXIMAGE_SUPPORT_WMF
CXIMAGE_FORMAT_WMF = 10,
#endif
#if CXIMAGE_SUPPORT_JP2
CXIMAGE_FORMAT_JP2 = 11,
#endif
#if CXIMAGE_SUPPORT_JPC
CXIMAGE_FORMAT_JPC = 12,
#endif
#if CXIMAGE_SUPPORT_PGX
CXIMAGE_FORMAT_PGX = 13,
#endif
#if CXIMAGE_SUPPORT_PNM
CXIMAGE_FORMAT_PNM = 14,
#endif
#if CXIMAGE_SUPPORT_RAS
CXIMAGE_FORMAT_RAS = 15,
#endif
#if CXIMAGE_SUPPORT_JBG
CXIMAGE_FORMAT_JBG = 16,
#endif
#if CXIMAGE_SUPPORT_MNG
CXIMAGE_FORMAT_MNG = 17,
#endif
#if CXIMAGE_SUPPORT_SKA
CXIMAGE_FORMAT_SKA = 18,
#endif
#if CXIMAGE_SUPPORT_RAW
CXIMAGE_FORMAT_RAW = 19,
#endif
CMAX_IMAGE_FORMATS = CXIMAGE_SUPPORT_BMP + CXIMAGE_SUPPORT_GIF + CXIMAGE_SUPPORT_JPG +
					 CXIMAGE_SUPPORT_PNG + CXIMAGE_SUPPORT_MNG + CXIMAGE_SUPPORT_ICO +
					 CXIMAGE_SUPPORT_TIF + CXIMAGE_SUPPORT_TGA + CXIMAGE_SUPPORT_PCX +
					 CXIMAGE_SUPPORT_WBMP+ CXIMAGE_SUPPORT_WMF +
					 CXIMAGE_SUPPORT_JBG + CXIMAGE_SUPPORT_JP2 + CXIMAGE_SUPPORT_JPC +
					 CXIMAGE_SUPPORT_PGX + CXIMAGE_SUPPORT_PNM + CXIMAGE_SUPPORT_RAS +
					 CXIMAGE_SUPPORT_SKA + CXIMAGE_SUPPORT_RAW + 1
};

class CImageFile  
{
	//extensible information collector
	typedef struct tagCImageInfo {
		DWORD	dwEffWidth;			///< DWORD aligned scan line width
		BYTE*	pImage;				///< THE IMAGE BITS
		CImageFile* pGhost;			///< if this is a ghost, pGhost points to the body
		CImageFile* pParent;			///< if this is a layer, pParent points to the body
		DWORD	dwType;				///< original image format
		char	szLastError[256];	///< debugging
		long	nProgress;			///< monitor
		long	nEscape;			///< escape
		long	nBkgndIndex;		///< used for GIF, PNG, MNG
		RGBQUAD nBkgndColor;		///< used for RGB transparency
		float	fQuality;			///< used for JPEG, JPEG2000 (0.0f ... 100.0f)
		BYTE	nJpegScale;			///< used for JPEG [ignacio]
		long	nFrame;				///< used for TIF, GIF, MNG : actual frame
		long	nNumFrames;			///< used for TIF, GIF, MNG : total number of frames
		DWORD	dwFrameDelay;		///< used for GIF, MNG
		long	xDPI;				///< horizontal resolution
		long	yDPI;				///< vertical resolution
		RECT	rSelectionBox;		///< bounding rectangle
		BYTE	nAlphaMax;			///< max opacity (fade)
		bool	bAlphaPaletteEnabled; ///< true if alpha values in the palette are enabled.
		bool	bEnabled;			///< enables the painting functions
		long	xOffset;
		long	yOffset;
		DWORD	dwCodecOpt[1];//CMAX_IMAGE_FORMATS];	///< for GIF, TIF : 0=def.1=unc,2=fax3,3=fax4,4=pack,5=jpg
		RGBQUAD last_c;				///< for GetNearestIndex optimization
		BYTE	last_c_index;
		bool	last_c_isvalid;
		long	nNumLayers;
		DWORD	dwFlags;			///< 0x??00000 = reserved, 0x00??0000 = blend mode, 0x0000???? = layer id - user flags
		BYTE	dispmeth;
		bool	bGetAllFrames;
		bool	bLittleEndianHost;

	} CIMAGEINFO;
protected:
	bool m_bInternalRawBitsBuff;
	void*				pDib; //contains the header, the palette, the pixels
	BYTE*				pAlpha; //alpha channel
public:
	//public structures
	struct rgb_color { BYTE r,g,b; };
    BITMAPINFOHEADER    head; //standard header
	CIMAGEINFO			info; //extended information
	virtual bool ReadImageFile(FILE* fp,BYTE* lpExterRawBitsBuff=NULL,UINT uiBitsBuffSize=0)=0;
	virtual bool WriteImageFile(FILE* fp)=0;
	CImageFile();
	virtual ~CImageFile();
	DWORD GetBitmapBits(DWORD dwCount, BYTE* lpBits);
	void*	Create(DWORD dwWidth, DWORD dwHeight, DWORD wBpp, DWORD imagetype = 0,BYTE* lpExterRawBitsBuff=NULL,UINT uiBitsBuffSize=0);
	bool	Destroy();
	static DWORD GetTypeIndexFromId(const DWORD id);

	BYTE	GetJpegQuality() const{return (BYTE)(info.fQuality + 0.5f);}
	void	SetJpegQuality(BYTE q){info.fQuality = (float)q;}
	BYTE	GetJpegScale() const{return info.nJpegScale;}
	void	SetJpegScale(BYTE q){info.nJpegScale = q;}

	DWORD	GetCodecOption(DWORD imagetype = 0);
	bool	SetCodecOption(DWORD opt, DWORD imagetype = 0);

	void RGBtoBGR(BYTE *buffer, int length);
	DWORD	GetHeight() const{return head.biHeight;}
	DWORD	GetWidth() const{return head.biWidth;}
	DWORD	GetEffWidth() const{return info.dwEffWidth;}
	DWORD	GetNumColors() const{return head.biClrUsed;}
	long	GetXDPI() const{return info.xDPI;}
	long	GetYDPI() const{return info.yDPI;}
	void	SetXDPI(long dpi){info.xDPI=dpi;}
	void	SetYDPI(long dpi){info.yDPI=dpi;}
	DWORD	GetClrImportant() const;
	void	SetClrImportant(DWORD ncolors = 0);
	BYTE*	GetBits(DWORD row = 0);
	long	GetSize(){return head.biSize + head.biSizeImage + GetPaletteSize();}
	WORD    GetBpp() const{return head.biBitCount;}
	DWORD	GetType() const{return info.dwType;}
	bool IsInside(long x, long y){return (0<=y && y<head.biHeight && 0<=x && x<head.biWidth);}

	DWORD	GetPaletteSize(){return (head.biClrUsed * sizeof(RGBQUAD));}
	RGBQUAD* GetPalette() const;
	void	SetPalette(DWORD n, BYTE *r, BYTE *g, BYTE *b);
	void	SetPalette(rgb_color *rgb,DWORD nColors=256);
	void	SetPaletteColor(BYTE idx, BYTE r, BYTE g, BYTE b, BYTE alpha=0);
	void	SetPaletteColor(BYTE idx, RGBQUAD c);
	RGBQUAD GetPaletteColor(BYTE idx);
	bool	GetPaletteColor(BYTE i, BYTE* r, BYTE* g, BYTE* b);
	void	SetGrayPalette();

	RGBQUAD	GetTransColor();
/** \addtogroup Palette
 * These functions have no effects on RGB images and in this case the returned value is always 0.
 * @{ */
	bool	IsGrayScale();
	void	SwapIndex(BYTE idx1, BYTE idx2);
protected:
/** \addtogroup Protected */ //@{
	bool EncodeSafeCheck(FILE *fp);
//@}
	BYTE BlindGetPixelIndex(const long x,const long y);
	void BlindSetPixelIndex(long x,long y,BYTE i);
	RGBQUAD BlindGetPixelColor(const long x,const long y, bool bGetAlpha = true);
	bool AlphaIsValid(){return pAlpha!=0;}
	BYTE AlphaGet(const long x,const long y);

};
#endif // !defined(AFX_IMAGEFILE_H__01662188_548F_4D9A_B515_99F84A94B1D1__INCLUDED_)
