
// IBOMDoc.cpp : CIBOMDoc 类的实现
//

#include "stdafx.h"
// SHARED_HANDLERS 可以在实现预览、缩略图和搜索筛选器句柄的
// ATL 项目中进行定义，并允许与该项目共享文档代码。
#ifndef SHARED_HANDLERS
#include "IBOM.h"
#endif
#include "IBOMDoc.h"
#include <propkey.h>
#include "DataModel.h"
#include "ArrayList.h"
#include "AutoSaveParamDlg.h"
#include "XhLicAgent.h"
#include "CryptBuffer.h"
#include "Tool.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CIBOMDoc

IMPLEMENT_DYNCREATE(CIBOMDoc, CDocument)

BEGIN_MESSAGE_MAP(CIBOMDoc, CDocument)
	ON_COMMAND(ID_FONTS_LIBRARY, &CIBOMDoc::OnFontsLibrary)
END_MESSAGE_MAP()


// CIBOMDoc 构造/析构

CIBOMDoc::CIBOMDoc()
{
	// TODO: 在此添加一次性构造代码
	m_nTimer=0;
	m_iMaxBakSerial=0;
	m_bAutoBakSave=false;
}

CIBOMDoc::~CIBOMDoc()
{
}

BOOL CIBOMDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: 在此添加重新初始化代码
	// (SDI 文档将重用该文档)

	return TRUE;
}

void ClearTemporaryFile(const char* dir_path);
void CIBOMDoc::DeleteContents()
{
	dataModel.Empty();
	ClearTemporaryFile(NULL);
	CIBOMView *pView=theApp.GetIBomView();
	CDisplayDataPage *pPage=pView?pView->GetDataPage():NULL;
	if(pPage)
		pPage->ClearImage();
	CDocument::DeleteContents();
}

// CIBOMDoc 序列化

void CIBOMDoc::Serialize(CArchive& ar)
{
	DWORD cursor_pipeline_no=DogSerialNo();
	BeginWaitCursor();
	CLogErrorLife loglife;
	try{
		char file_version[20],product_version[20];
		theApp.GetAppVersion(file_version,product_version);
		if (ar.IsStoring())
		{
			int nZeroLen=0;
			ar<<nZeroLen;	//为兼容之前版本文件读取，在新版本开头写入一个整数0 wht 18-07-20
			ar<<CString("IBOM");
			CString sFileVersion=file_version;
			ar << sFileVersion;
			ar << dataModel.m_uiOriginalDogKey;
			ar << cursor_pipeline_no;
			//int version=10006;	//V1.0.5
			long version=FromStringVersion(file_version);
			CBuffer buffer(10000000);
			buffer.WriteString("IBOM");
			buffer.WriteInteger(version);
			dataModel.ParseEmbedImageFileToTempFile(m_strPathName);
			dataModel.ToBuffer(buffer,version,m_bAutoBakSave);
			dataModel.DeleteTempImageFile();
			if(version>=1000701)
				EncryptBuffer(buffer,2);
			else
				EncryptBuffer(buffer,true);
			DWORD dwFileLen = buffer.GetLength();
			ar.Write(&dwFileLen,sizeof(DWORD));
			ar.Write(buffer.GetBufferPtr(),dwFileLen);
			ResetAutoSaveTimer();
		}
		else
		{
			DWORD buffer_len=0;
			CString sDocTypeName,sFileVersion;
			char ciEncryptMode=0;
			ar>>buffer_len;
			if(buffer_len==0)
			{	//新版本文件
				ar >> sDocTypeName;
				ar >> sFileVersion;
				ar >> dataModel.m_uiOriginalDogKey;
				ar >> dataModel.m_uiLastSaveDogKey;
				if(compareVersion(sFileVersion,file_version)>0)
#ifdef AFX_TARG_ENU_ENGLISH
					throw "The file's version number is too high or too low,so it can't read directly";
#else 
					throw "此文件版本号太高,不能直接读取此文件";
#endif
				ciEncryptMode=(compareVersion(sFileVersion,"1.0.7.1")>=0)?2:true;
				ar>>buffer_len;
				dataModel.m_sFileVersion.Copy(sFileVersion);
			}
			CBuffer buffer(buffer_len);
			buffer.Write(NULL,buffer_len);
			ar.Read(buffer.GetBufferPtr(),buffer_len);
			if(ciEncryptMode>0)
				DecryptBuffer(buffer,ciEncryptMode,cursor_pipeline_no);
			buffer.SeekToBegin();
			int version=0;
			CXhChar50 szDocType;
			buffer.ReadString(szDocType,51);
			buffer.ReadInteger(&version);
			if(version==0)	//最开始未存储文件版本号
				version=10000;
			dataModel.FromBuffer(buffer,version);
			dataModel.ComplementSegItemHashList();
		}
	}
	catch(char* sError)
	{
		AfxMessageBox(sError);
		DisplayProcess(100,NULL);
	}
	EndWaitCursor();
}

#ifdef SHARED_HANDLERS

// 缩略图的支持
void CIBOMDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// 修改此代码以绘制文档数据
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// 搜索处理程序的支持
void CIBOMDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// 从文档数据设置搜索内容。
	// 内容部分应由“;”分隔

	// 例如:  strSearchContent = _T("point;rectangle;circle;ole object;")；
	SetSearchContent(strSearchContent);
}

void CIBOMDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = NULL;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != NULL)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// CIBOMDoc 诊断

#ifdef _DEBUG
void CIBOMDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CIBOMDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CIBOMDoc 命令
CView* CIBOMDoc::GetView(const CRuntimeClass *pClass)
{
	CView *pView;
	POSITION position;
	position = GetFirstViewPosition();
	for(;;)
	{
		if(position==NULL)
		{
			pView = NULL;
			break;
		}
		pView = GetNextView(position);
		if(pView->IsKindOf(pClass))
			break;
	}
	return pView;
}

#include "XhLicAgent.h"
#include "FontsLibraryDlg.h"
void CIBOMDoc::OnFontsLibrary()
{
	CXhChar200 fontsfile("%sibom.fonts",theApp.execute_path);
	IAlphabets *pAlphabets=IMindRobot::GetAlphabetKnowledge();
	pAlphabets->ExportFontsFile(fontsfile);
	CFontsLibraryDlg fontsdlg;
	fontsdlg.DoModal();
}


void CALLBACK EXPORT AutoSaveProc(HWND hWnd, UINT nMsg, UINT_PTR nIDEvent, DWORD dwTime)
{
	if(nIDEvent==theApp.GetDocument()->m_nTimer)
		theApp.GetDocument()->AutoSaveBakFile();
}

char* SearchChar(char* srcStr,char ch,bool reverseOrder=false)
{
	if(!reverseOrder)
		return strchr(srcStr,ch);
	else
	{
		int len=strlen(srcStr);
		for(int i=len-1;i>=0;i--)
		{
			if(srcStr[i]==ch)
				return &srcStr[i];
		}
	}
	return NULL;
}

static CAutoSaveParamDlg dlg;
void CIBOMDoc::AutoSaveBakFile()
{
	CXhChar200 ss("%s",GetPathName());
	char* separator=SearchChar(ss,'.',true);
	if(separator==NULL)
	{
		if(dlg.GetSafeHwnd()!=NULL&&dlg.IsWindowVisible())
			return;
		if(dlg.DoModal()==IDOK)
		{
			CConfig::m_nAutoSaveTime=atoi(dlg.m_sAutoSaveTime)*60000;
			CConfig::m_iAutoSaveType=dlg.m_iAutoSaveType;
			OnFileSave();
			ResetAutoSaveTimer();
		}
		return;
	}
	*separator=0;
	if(CConfig::m_iAutoSaveType==0)
	{
		ss.Append(".bak");
		m_bAutoBakSave=true;
		OnSaveDocument(ss);
		m_bAutoBakSave=false;
	}
	else if(CConfig::m_iAutoSaveType==1)
	{
		CString filename="";
		CFileFind fileFind;
		do
		{
			filename="";
			m_iMaxBakSerial++;
			filename.Format("%s(%d).bak",(char*)ss,m_iMaxBakSerial);
		}while(fileFind.FindFile(filename));
		m_bAutoBakSave=true;
		OnSaveDocument(filename);
		m_bAutoBakSave=false;
	}
}

void CIBOMDoc::ResetAutoSaveTimer()
{
	if(CConfig::m_nAutoSaveTime==0)
	{
		if(m_nTimer)
			AfxGetMainWnd()->KillTimer(m_nTimer);
		m_nTimer=0;
		return;
	}
	if(m_nTimer)
	{
		AfxGetMainWnd()->KillTimer(m_nTimer);
		m_nTimer=AfxGetMainWnd()->SetTimer(2,CConfig::m_nAutoSaveTime,AutoSaveProc);
	}
	else
		m_nTimer=AfxGetMainWnd()->SetTimer(2,CConfig::m_nAutoSaveTime,AutoSaveProc);
}