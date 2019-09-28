#include "stdafx.h"
#include "PdfReader.h"
#include "WinUtil.h"
#include "PdfEngine.h"

CPdfReader::CPdfReader()
{
	m_pPdfEngine=NULL;
}

int G2U(const char* gb2312,char* utf8_str);
bool CPdfReader::OpenPdfFile(const char* file_path)
{
	char utf_file_path[1024]={0};
	if(G2U(file_path,utf_file_path)<=0)
		return false;
	AutoFreeW dstPath(str::conv::FromUtf8(utf_file_path));
	m_pPdfEngine=PdfEngine::CreateFromFile(dstPath);
	if(m_pPdfEngine)
	{
		DocTocItem* pRootItem=m_pPdfEngine->GetTocTree();
		DocTocItem* pDocItem=pRootItem;
		if(pRootItem)
		{
			int i=1;
			while(pDocItem)
			{
				PDF_PAGE_INFO *pPageInfo=m_pagetInfoList.append();
				pPageInfo->iPageNo=i++;//pDocItem->pageNo;
				AutoFreeW title(pDocItem->title);			
				strcpy(pPageInfo->sPageName,str::conv::ToUtf8(title).data);
				pDocItem=pDocItem->next;
			}
			delete pDocItem;
		}
		else
		{
			for(int i=0;i<m_pPdfEngine->PageCount();i++)
			{
				PDF_PAGE_INFO *pPageInfo=m_pagetInfoList.append();
				pPageInfo->iPageNo=i+1;
				sprintf(pPageInfo->sPageName,"%d",i+1);
			}
		}
	}
	return true;
}

IPdfReader::PDF_PAGE_INFO *CPdfReader::EnumFirstPage()
{
	return m_pagetInfoList.GetFirst();
}

IPdfReader::PDF_PAGE_INFO *CPdfReader::EnumNextPage()
{
	return m_pagetInfoList.GetNext();
}

int CPdfReader::PageCount()
{
	return m_pagetInfoList.GetNodeNum();
}
CPdfReader::~CPdfReader()
{
	if(m_pPdfEngine)
	{
		delete m_pPdfEngine;
		m_pPdfEngine=NULL;
	}
}

//////////////////////////////////////////////////////////////////////////
//
CPdfReaderInstance::CPdfReaderInstance()
{
	m_pPdfReader=new CPdfReader();
}
CPdfReaderInstance::~CPdfReaderInstance()
{
	if(m_pPdfReader)
	{
		delete m_pPdfReader;
		m_pPdfReader=NULL;
	}
}