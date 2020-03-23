
// IBOMDoc.h : CIBOMDoc ��Ľӿ�
//


#pragma once


class CIBOMDoc : public CDocument
{
	bool m_bAutoBakSave;
protected: // �������л�����
	CIBOMDoc();
	DECLARE_DYNCREATE(CIBOMDoc)

// ����
public:
	UINT_PTR m_nTimer;
	int m_iMaxBakSerial;//��ǰ���ļ��洢��� Ĭ��ֵΪ0
// ����
public:

// ��д
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif // SHARED_HANDLERS

// ʵ��
public:
	virtual ~CIBOMDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	CView* GetView(const CRuntimeClass *pClass);
	CString GetFilePath(){return m_strPathName;}
	void AutoSaveBakFile();
	void ResetAutoSaveTimer();
protected:

// ���ɵ���Ϣӳ�亯��
protected:
	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	// ����Ϊ����������������������ݵ� Helper ����
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS
public:
	virtual void DeleteContents();
	afx_msg void OnFontsLibrary();
};
