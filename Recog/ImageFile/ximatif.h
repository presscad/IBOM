/*
 * File:	ximatif.h
 * Purpose:	TIFF Image Class Loader and Writer
 */
/* ==========================================================
 * CxImageTIF (c) 07/Aug/2001 Davide Pizzolato - www.xdp.it
 * For conditions of distribution and use, see copyright notice in ximage.h
 *
 * Special thanks to Troels Knakkergaard for new features, enhancements and bugfixes
 *
 * Special thanks to Abe <God(dot)bless(at)marihuana(dot)com> for MultiPageTIFF code.
 *
 * LibTIFF is:
 * Copyright (c) 1988-1997 Sam Leffler
 * Copyright (c) 1991-1997 Silicon Graphics, Inc.
 * ==========================================================
 */

#if !defined(__ximatif_h)
#define __ximatif_h

#include "ImageFile.h"	//#include "ximage.h"

#if CXIMAGE_SUPPORT_TIF

#include "../tiff/tiffio.h"

class CImageFileTif: public CImageFile
{
public:
	CImageFileTif();
	~CImageFileTif();

	//TIFF* TIFFOpenEx(CxFile * hFile);
	void  TIFFCloseEx(TIFF* tif);
	bool ReadImageFile(FILE *fp, BYTE* lpExterRawBitsBuff = NULL, UINT uiBitsBuffSize = 0);
	bool ReadImageFile(const char* szTiffFileName);
	bool Decode(int niPageIndex,bool blConvertMonoImg=false);//CxIOFile * hFile)
	//bool Decode(FILE *hFile) { CxIOFile file(hFile); return Decode(&file); }

#if CXIMAGE_SUPPORT_ENCODE
	bool Encode(CxFile * hFile, bool bAppend=false);
	bool Encode(CxFile * hFile, CxImage ** pImages, int32_t pagecount);
	bool Encode(FILE *hFile, bool bAppend=false) { CxIOFile file(hFile); return Encode(&file,bAppend); }
	bool Encode(FILE *hFile, CxImage ** pImages, int32_t pagecount)
				{ CxIOFile file(hFile); return Encode(&file, pImages, pagecount); }
#endif // CXIMAGE_SUPPORT_ENCODE

protected:
	void TileToStrip(BYTE* out, BYTE* in,	UINT rows, UINT cols, int outskew, int inskew);
	bool EncodeBody(TIFF *m_tif, bool multipage=false, int page=0, int pagecount=0);
	TIFF *m_tif1, *m_tif2;	//m_tif1用于解码读取, m_tif2用于编码写入
	bool m_multipage;
	int  m_pages;
	void MoveBits( BYTE* dest, BYTE* from, int count, int bpp );
	void MoveBitsPal( BYTE* dest, BYTE*from, int count, int bpp, RGBQUAD* pal );
public:
	int get_niPageCount() const{return m_pages;}
	__declspec(property(get=get_niPageCount)) int niPageCount;
};

#endif

#endif
