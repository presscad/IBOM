#pragma once
#include "Recog.h"
#include "XhCharString.h"
#include "HashTable.h"
#include "MemberProperty.h"
#include "SegI.h"
#include "Buffer.h"
#include "ArrayList.h"
#include "TempFile.h"
#include "f_ent_list.h"

class CImageFileHost;
class CImageRegionHost;
struct IMAGE_DATA_EX : public IMAGE_DATA
{
	BYTE ciRelaObjType;//CDataCmpModel::DWG_IMAGE_DATA �� CDataCmpModel::IMAGE_REGION_DATA
	DWORD dwRelaObj;
	IMAGE_DATA_EX(){dwRelaObj=ciRelaObjType=0;}
};
class CImageRegionHost : public IImageRegion
{
	IImageRegion* m_pInternalRegion;
	DYN_ARRAY<int> m_arrColWidth;	//��¼�п�
private:
	DWORD m_dwKey;
	CImageFileHost* m_pBelongImageFile;
	CHashList<IRecoginizer::BOMPART> hashInputPartById;	//�ֶ�����Ĺ�������
	CHashStrList<IRecoginizer::BOMPART> hashParts;		//������¼����(�Զ���ȡ+�ֶ����)

	virtual bool EnumFirstBomPart(IRecoginizer::BOMPART* pBomPart);
	virtual bool EnumNextBomPart(IRecoginizer::BOMPART* pBomPart);
public:
	CXhChar50 m_sName;
	CImageRegionHost();
	~CImageRegionHost();
	void FromBuffer(CBuffer& buffer,long version=0);
	void ToBuffer(CBuffer& buffer,long version=0);
	void SetImageRegion(IImageRegion* pImageReg){m_pInternalRegion=pImageReg;}
	IImageRegion* GetImageRegion(){return m_pInternalRegion;}
	__declspec( property(get=GetWidth)) int Width;
	__declspec( property(get=GetHeight)) int Height;
	//���ݲ���
	void SetKey(DWORD key){m_dwKey=key;}
	DWORD GetKey(){return m_dwKey;}
	void SetBelongImageFile(CImageFileHost* pImage){m_pBelongImageFile=pImage;}
	CImageFileHost* BelongImageFile(){return m_pBelongImageFile;}
	BOOL SummarizePartInfo();
	BOOL SyncInputPartToSummaryParts();
	int GetPartNum(){return hashParts.GetNodeNum();}
	BOOL DeleteInputPart(int id){return hashInputPartById.DeleteNode(id);}
	int GetInputPartNum(){return hashInputPartById.GetNodeNum();}
	int GetRecogPartNum();
	IRecoginizer::BOMPART* AddInputPart(int id=0){return hashInputPartById.Add(id);}
	IRecoginizer::BOMPART* EnumFirstInputPart(){return hashInputPartById.GetFirst();}
	IRecoginizer::BOMPART* EnumNextInputPart(){return hashInputPartById.GetNext();}
	IRecoginizer::BOMPART* ReplaceKey(const char* sOldKey,const char* sNewKey){return hashParts.ModifyKeyStr(sOldKey,sNewKey);}
	IRecoginizer::BOMPART* FromPartNo(const char* sPartNo){return hashParts.GetValue(sPartNo);}
	IRecoginizer::BOMPART* AddBomPart(const char* sPartNo){return hashParts.Add(sPartNo);}
	IRecoginizer::BOMPART* EnumFirstPart(){return hashParts.GetFirst();}
	IRecoginizer::BOMPART* EnumNextPart(){return hashParts.GetNext();}
	IRecoginizer::BOMPART* EnumPrevPart(){return hashParts.GetPrev();}
	IRecoginizer::BOMPART* EnumTailPart(){return hashParts.GetTail();}
	void DeleteBomPart(const char* sPartNo){hashParts.DeleteNode(sPartNo);}
	//
	long GetSerial();
	IImageFile* BelongImageData() const;
	POINT TopLeft(bool bRealRegion=false);
	POINT TopRight(bool bRealRegion=false);
	POINT BtmLeft(bool bRealRegion=false);
	POINT BtmRight(bool bRealRegion=false);
	void InitImageShowPara(RECT showRect);
	virtual double Zoom(short zDelta,POINT* pxScrPoint=NULL){return m_pInternalRegion->Zoom(zDelta,pxScrPoint);}
	virtual double ZoomRatio(){return m_pInternalRegion->ZoomRatio();}
	virtual bool IsCanZoom(){return m_pInternalRegion->IsCanZoom();}
	virtual int GetWidth() {return m_pInternalRegion->GetWidth();}
	virtual int GetHeight(){return m_pInternalRegion->GetHeight();}
	virtual void SetScrOffset(SIZE offset){m_pInternalRegion->SetScrOffset(offset);}
	virtual SIZE GetScrOffset(){return m_pInternalRegion->GetScrOffset();}
	virtual POINT MappingToScrOrg(){return m_pInternalRegion->MappingToScrOrg();}
	virtual RECT  MappingToScrRect(){return m_pInternalRegion->MappingToScrRect();}
	virtual bool SetRegion(POINT topLeft,POINT btmLeft,POINT btmRight,POINT topRight,bool bRealRegion=false,bool bRetrieveRegion=true);
	virtual void UpdateImageRegion(){return m_pInternalRegion->UpdateImageRegion();}
	virtual int  GetMonoImageData(IMAGE_DATA* imagedata);	//ÿλ����1�����أ�0��ʾ�׵�,1��ʾ�ڵ�
	virtual int  GetRectMonoImageData(RECT* rc,IMAGE_DATA* imagedata);		//ÿλ����1�����أ�0��ʾ�׵�,1��ʾ�ڵ�
	virtual int  GetImageColumnWidth(int* col_width_arr=NULL);	//��������
	int  GetImageOrgColumnWidth(int* col_width_arr=NULL);		//��������
#ifdef _DEBUG
	virtual int Recognize(char modeOfAuto0BomPart1Summ2=0,int iCellRowStart=-1,int iCellRowEnd=-1,int iCellCol=-1);	//-1��ʾȫ��,>=0��ʾָ���л��� wjh-2018.3.14
#else
	virtual int Recognize(char modeOfAuto0BomPart1Summ2=0);
#endif
	virtual bool RetrieveCharImageData(int iCellRow,int iCellCol,IMAGE_CHARSET_DATA* imagedata);
	virtual bool Destroy();
	virtual UINT GetBomPartNum(){return m_pInternalRegion->GetBomPartNum();}
	virtual bool GetBomPartRect(UINT idPart,RECT* rc){return m_pInternalRegion->GetBomPartRect(idPart,rc);}
public:	//�����Ҷ�ͼתΪ�ڰ�ͼ�ֽ�����ֵ����ر���������
	//balancecoef ���ֺڰ׵��Ľ��޵���ƽ��ϵ��,ȡֵ-1.0~1.0;0
	virtual double SetMonoThresholdBalanceCoef(double mono_balance_coef=0);
	virtual double GetMonoThresholdBalanceCoef();
	virtual int get_MonoForwardPixels();
	__declspec(property(get=get_MonoForwardPixels)) int MonoForwardPixels;
};
struct POLYLINE_4PT{
	CPoint m_ptArr[4];	//���ϡ����ϡ����¡�����,��Ϊͼ������ϵ�е�����λ��
	char m_iActivePt;	//���������
	//
	static const BYTE STATE_NONE		= 0;
	static const BYTE STATE_WAITNEXTPT	= 1;
	static const BYTE STATE_VALID		= 2;
	BYTE m_cState;		//0.δ��ʼ�� 1.�ȴ�ѡ���2�� 2.��Ч
	void Init();
	POLYLINE_4PT(){Init();}
};
class CImageFileHost : public IImageFile
{
	IImageFile* m_pInternalImgFile;
	CHashListEx<CImageRegionHost> m_hashRegionById;
	CHashStrList<IRecoginizer::BOMPART> hashSumParts;	//������¼����
	CXhChar100 m_sFileName;
	CXhChar100 m_sAliasName;	//�������ļ������û�����ע����
	PDF_FILE_CONFIG m_pdfConfig;
	/*int m_iPageNo;
	int m_nRotationCount;		//ȡֵΪ-3��-2��-1��0��1��2��3
	double m_fPDFZoomScale;		//PDFתΪbmpͼƬʱ�ı�����Ϣ
	double m_fPDFMinLineWeight;
	double m_fPDFMinLineFlatness;*/
	//biType=0.��ʾ���Ҷ�ͼ��ѹ���洢;
	//		=1.��ʾ��ԭʼjpg�ļ��洢��ʵ��֤��ԭʼ��jpg�ļ�ͼ��ѹ���ʸ��߶��Ҳ�����IBOMת��ʧ�����⣩
	BYTE m_biImageSaveType;
	DWORD m_dwAddressJpgFile;
	DWORD m_dwSizeJpgFile;
	CTempFileBuffer m_tempFileBuffer;
public:
	SEGI m_iSeg;	//ͼƬ�ļ������κ�
	POLYLINE_4PT m_xPolyline;
	//CXhChar100 m_sPathFileName;
public:
	CImageFileHost();
	~CImageFileHost();
	void FromBuffer(CBuffer& buffer,long version=0);
	void ToBuffer(CBuffer& buffer,long version=0,DWORD address_file_pos=0,DWORD file_size=0,BYTE save_type=0,bool bAutoBakSave=false);
	void SetImageFile(IImageFile* pImageFile);
	IImageFile* GetImageFile(){return m_pInternalImgFile;}
	BOOL InitTempFileBuffer(CBuffer &buffer,const char* sTempPath);
	BOOL DeleteTempFile(){return m_tempFileBuffer.DeleteFile();}
	BOOL InitFileAddressAndSize(DWORD file_pos,DWORD file_size,BYTE save_type);
	//���ݲ���
	CImageRegionHost* AppendRegion();
	void DeleteRegion(DWORD dwRegionKey);
	void SummarizePartInfo();
	CImageRegionHost* EnumFirstRegionHost(){return m_hashRegionById.GetFirst();}
	CImageRegionHost* EnumNextRegionHost(){return m_hashRegionById.GetNext();}
	IRecoginizer::BOMPART* FromPartNo(const char* sPartNo){return hashSumParts.GetValue(sPartNo);}
	IRecoginizer::BOMPART* AddBomPart(const char* sPartNo){return hashSumParts.Add(sPartNo);}
	IRecoginizer::BOMPART* EnumFirstPart(){return hashSumParts.GetFirst();}
	IRecoginizer::BOMPART* EnumNextPart(){return hashSumParts.GetNext();}
public: //����
	CXhChar500 get_PathFileName();
	CXhChar100 get_FileName(){return m_sFileName;}
	CXhChar100 set_FileName(const char* _szFileName);
	CXhChar100 get_AliasName();
	CXhChar100 set_AliasName(const char* _szAliasName);
	int get_PageNo();
	int set_PageNo(int page_no);
	int get_RotDegAngle();
	int set_RotDegAngle(int rotation);
	double get_PDFZoomScale(){return m_pdfConfig.zoom_scale;}
	double set_PDFZoomScale(double zoom_scale){return (m_pdfConfig.zoom_scale=zoom_scale);}
	BYTE get_ImageSaveType(){return m_biImageSaveType;}
	DWORD get_FileAddressPos(){return m_dwAddressJpgFile;}
	DWORD get_FileSize(){return m_dwSizeJpgFile;}
	__declspec(property(get=get_PathFileName)) CXhChar500 szPathFileName;
	__declspec(property(put=set_FileName,get=get_FileName)) CXhChar100 szFileName;
	__declspec(property(put=set_AliasName,get=get_AliasName)) CXhChar100 szAliasName;
	__declspec(property(put=set_PageNo,get=get_PageNo)) int iPageNo;
	__declspec(property(put=set_RotDegAngle,get=get_RotDegAngle)) int niRotDegAngle;
	__declspec(property(put=set_PDFZoomScale,get=get_PDFZoomScale)) double fPDFZoomScale;
	__declspec(property(get=get_ImageSaveType)) BYTE biImageSaveType;
	__declspec(property(get=get_FileAddressPos)) DWORD dwFileAddressPos;
	__declspec(property(get=get_FileSize)) DWORD dwFileSize;
public:
	virtual long GetSerial();
	virtual bool InitImageFileHeader(const char* imagefile);
	virtual bool InitImageFile(const char* imagefile,const char* szIBomFilePath,bool update_file_path=true,PDF_FILE_CONFIG *pPDFConfig=NULL);
	virtual void GetPathFileName(char* imagefilename,int maxStrBufLen=17);
	virtual BYTE ImageCompressType();
	virtual bool SetImageRawFileData(char* imagedata,UINT uiMaxDataSize,BYTE biFileType,PDF_FILE_CONFIG *pPDFConfig=NULL);
	//virtual UINT GetJpegImageData(char* imagedata,UINT uiMaxDataSize=0);
	virtual bool SetCompressImageData(char* imagedata,UINT uiMaxDataSize);
	virtual UINT GetCompressImageData(char* imagedata,UINT uiMaxDataSize=0);
	virtual double SetMonoThresholdBalanceCoef(double balance=0,bool bUpdateMonoImages=false);
	virtual double GetMonoThresholdBalanceCoef();
	double get_PDFMinLineWeight(){return m_pdfConfig.min_line_weight;}
	double set_PDFMinLineWeight(double min_weight){return (m_pdfConfig.min_line_weight=min_weight);}
	double get_PDFMinLineFlatness(){return m_pdfConfig.min_line_flatness;}
	double set_PDFMinLineFlatness(double min_flatness){return (m_pdfConfig.min_line_flatness=min_flatness);}
	double get_PDFMaxLineWeight(){return m_pdfConfig.max_line_weight;}
	double set_PDFMaxLineWeight(double max_weight){return (m_pdfConfig.max_line_weight=max_weight);}
	double get_PDFMaxLineFlatness(){return m_pdfConfig.max_line_flatness;}
	double set_PDFMaxLineFlatness(double max_flatness){return (m_pdfConfig.max_line_flatness=max_flatness);}
	BYTE get_PDFLineMode(){return m_pdfConfig.m_ciPdfLineMode;}
	BYTE set_PDFLineMode(BYTE ciPdfLineMode){return (m_pdfConfig.m_ciPdfLineMode=ciPdfLineMode);}
	__declspec(property(put=set_PDFMinLineWeight,get=get_PDFMinLineWeight)) double fPDFMinLineWeight;
	__declspec(property(put=set_PDFMinLineFlatness,get=get_PDFMinLineFlatness)) double fPDFMinLineFlatness;
	__declspec(property(put=set_PDFMaxLineWeight,get=get_PDFMaxLineWeight)) double fPDFMaxLineWeight;
	__declspec(property(put=set_PDFMaxLineFlatness,get=get_PDFMaxLineFlatness)) double fPDFMaxLineFlatness;
	__declspec(property(put=set_PDFLineMode,get=get_PDFLineMode)) BYTE ciPDFLineMode;
	virtual int GetWidth();
	DECLARE_READONLY_PROPERTY(int, Width){return GetWidth();}
	virtual int GetHeight();
	DECLARE_READONLY_PROPERTY(int, Height){return GetHeight();}
	virtual BYTE GetRawFileType();
	DECLARE_READONLY_PROPERTY(BYTE, RawFileType){return GetRawFileType();}
	//�����Ƿ�Ϊ����� wjh-2019.11.28
	virtual bool SetLowBackgroundNoise(bool blValue);
	virtual bool IsLowBackgroundNoise()const;
	//�Ƿ�Ϊϸ�ʻ�����
	virtual bool SetThinFontText(bool blValue);
	virtual bool IsThinFontText()const;
	virtual bool IsSrcFromPdfFile();
	virtual int Get24BitsImageEffWidth();
	virtual int Get24BitsImageData(IMAGE_DATA* imagedata);
	virtual int GetMonoImageData(IMAGE_DATA* imagedata);
	virtual bool TurnClockwise90();		//���ļ���ͼ��˳ʱ��ת90�ȣ���ת��Ӱ������ȡ����Ӧ����ȡ����֮ǰ��ת
	virtual bool TurnAntiClockwise90();	//���ļ���ͼ����ʱ��ת90�ȣ���ת��Ӱ������ȡ����Ӧ����ȡ����֮ǰ��ת
	virtual bool IsBlackPixel(int i,int j);
	virtual RECT GetImageDisplayRect(DWORD dwRegionKey=0);
	virtual bool DeleteImageRegion(long idRegionSerial);
	virtual void InitImageShowPara(RECT showRect){
		if(m_pInternalImgFile)
			return m_pInternalImgFile->InitImageShowPara(showRect);
	}
	virtual double Zoom(short zDelta,POINT* pxScrPoint=NULL){
		if (m_pInternalImgFile)
			return m_pInternalImgFile->Zoom(zDelta, pxScrPoint);
		else
			return 1;
	}
	virtual double ZoomRatio(){
		if (m_pInternalImgFile)
			return m_pInternalImgFile->ZoomRatio();
		else
			return 1;
	}
	virtual bool FocusPoint(POINT xImgPoint,POINT* pxScrPoint=NULL,double scaleofScr2Img=2.0){
		if (m_pInternalImgFile)
			return m_pInternalImgFile->FocusPoint(xImgPoint, pxScrPoint, scaleofScr2Img);
		else
			return false;
	}
	virtual bool IsCanZoom()
	{
		if (m_pInternalImgFile)
			return m_pInternalImgFile->IsCanZoom();
		else
			return false;
	}
	virtual void SetScrOffset(SIZE offset){
		if (m_pInternalImgFile)
			m_pInternalImgFile->SetScrOffset(offset);
	}
	virtual SIZE GetScrOffset(){
		if (m_pInternalImgFile)
			return m_pInternalImgFile->GetScrOffset();
		else
			return SIZE();
	}
	virtual POINT MappingToScrOrg(){
		if (m_pInternalImgFile)
			return m_pInternalImgFile->MappingToScrOrg();
		else
			return POINT();
	}
	virtual RECT  MappingToScrRect(){
		if (m_pInternalImgFile)
			return m_pInternalImgFile->MappingToScrRect();
		else
			return RECT();
	}
	virtual bool IntelliRecogCornerPoint(const POINT& pick,POINT* pCornerPoint,
					char ciType_LT0_RT1_RB2_LB3=0,int *pyjHoriTipOffset=NULL,UINT uiTestWidth=300,UINT uiTestHeight=200);
	virtual IImageRegion* AddImageRegion();
	virtual IImageRegion* AddImageRegion(POINT topLeft,POINT btmLeft,POINT btmRight,POINT topRight);
	virtual IImageRegion* EnumFirstRegion();
	virtual IImageRegion* EnumNextRegion();
	virtual void UpdateImageRegions(){
		if(m_pInternalImgFile)
			m_pInternalImgFile->UpdateImageRegions();
	}
	virtual void SetTurnCount(int count) { 
		if(m_pInternalImgFile)
			m_pInternalImgFile->SetTurnCount(count); 
	}
	virtual int GetTurnCount() {
		if (m_pInternalImgFile)
			return m_pInternalImgFile->GetTurnCount();
		else
			return 0;
	}
};
