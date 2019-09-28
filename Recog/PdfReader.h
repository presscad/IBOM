#ifndef __MUPDF_PDF_FILE_H__
#define __MUPDF_PDF_FILE_H__

#include "Recog.h"
#include "f_ent_list.h"
#include "BaseEngine.h"

class CPdfReader : public IPdfReader {
	BaseEngine *m_pPdfEngine;
	ATOM_LIST<PDF_PAGE_INFO> m_pagetInfoList;
public:
	CPdfReader();
	~CPdfReader();
	virtual bool OpenPdfFile(const char* file_path);
	virtual PDF_PAGE_INFO *EnumFirstPage();
	virtual PDF_PAGE_INFO *EnumNextPage();
	virtual int PageCount();
};

#endif