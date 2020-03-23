
// IBOMDoc.h : CIBOMDoc 类的接口
//


#pragma once


class CIBOMDoc : public CDocument
{
	bool m_bAutoBakSave;
protected: // 仅从序列化创建
	CIBOMDoc();
	DECLARE_DYNCREATE(CIBOMDoc)

// 特性
public:
	UINT_PTR m_nTimer;
	int m_iMaxBakSerial;//当前的文件存储序号 默认值为0
// 操作
public:

// 重写
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif // SHARED_HANDLERS

// 实现
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

// 生成的消息映射函数
protected:
	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	// 用于为搜索处理程序设置搜索内容的 Helper 函数
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS
public:
	virtual void DeleteContents();
	afx_msg void OnFontsLibrary();
};
