// ���� ifdef ���Ǵ���ʹ�� DLL �������򵥵�
// ��ı�׼�������� DLL �е������ļ��������������϶���� RECOG_EXPORTS
// ���ű���ġ���ʹ�ô� DLL ��
// �κ�������Ŀ�ϲ�Ӧ����˷��š�������Դ�ļ��а������ļ����κ�������Ŀ���Ὣ
// RECOG_API ������Ϊ�Ǵ� DLL ����ģ����� DLL ���ô˺궨���
// ������Ϊ�Ǳ������ġ�
#ifdef APP_EMBEDDED_MODULE_ENCRYPT
#define RECOG_API	//��ǶʱRECOG_APIʲô��������
#else
#ifdef RECOG_EXPORTS
#define RECOG_API __declspec(dllexport)
#else
#define RECOG_API __declspec(dllimport)
#endif
#endif
#pragma once

// �����Ǵ� Recog.dll ������
struct PDF_FILE_CONFIG{
	int page_no;
	int rotation;
	double zoom_scale;
	static const BYTE  LINE_MODE_GENERAL	= 0;	//����ģʽ
	static const BYTE  LINE_MODE_THIN		= 1;	//ϸ����
	static const BYTE  LINE_MODE_THICK		= 2;	//������
	static const BYTE  LINE_MODE_RANGE		= 3;	//��ȷ�Χ
	static const BYTE  LINE_MODE_NONE		= 255;	//δ��ʼ��
	BYTE m_ciPdfLineMode;	//0.����ģʽ|1.ϸ����|2.������|3.�Զ���|255.δ��ʼ��(�������ļ�)
	double min_line_weight;
	double min_line_flatness;
	double max_line_weight;
	double max_line_flatness;
	PDF_FILE_CONFIG(){
		page_no=1;
		rotation=0;
		zoom_scale=0;
		min_line_weight=0.3f;
		min_line_flatness=0.3f;
		max_line_weight=1.2f;
		max_line_flatness=1.2f;
		m_ciPdfLineMode=0xFF;
	}
};
struct IMAGE_DATA{
	int nEffWidth,nWidth,nHeight;
	BYTE biBitCount;	//��������ռ�ñ���λ��
	char* imagedata;	//��Ҫ�ڵ��÷�������ͷ��ڴ�(��ɨ����ʽΪDownward, top->bottom)
	IMAGE_DATA(){imagedata=NULL;nEffWidth=nWidth=nHeight=0;biBitCount=24;}
};
union XHWCHAR{
	WORD wHz;
	char chars[2];	//����ĸ���»���'_'��Ϊ��д����ʱ��Label()��������ƴд�� wjh-2014.8.19
public:
	XHWCHAR(){wHz=0;}
	XHWCHAR(const char* wc){
		chars[0]=wc[0];
		chars[1]=wc[1];
	}
	operator WORD(){return wHz;}
	operator char(){return chars[0];}
};
struct IMAGE_CHARSET_DATA{
	UINT nEffWidth,nWidth,nHeight;
	UINT uiPreAllocSize;	//imagedata!=NULLʱ��ʾԤ������ڴ�ռ��С
	char* imagedata;		//��Ҫ�ڵ��÷�������ͷ��ڴ�(��ɨ����ʽΪDownward, top->bottom)
	WORD wPreAllocCharCount;//pPreAllocCharSet��Ԥ������ַ���������
	struct CHAR_IMAGE{
		WORD wChar;	//�ɱ�ʾ����
		WORD wiStartPos,wiEndPos;	//��ǰ�ַ�����ʼ�����λ��
	}* pPreAllocCharSet;
	IMAGE_CHARSET_DATA(){
		imagedata=NULL;
		pPreAllocCharSet=NULL;
		wPreAllocCharCount=0;
		nEffWidth=nWidth=nHeight=uiPreAllocSize=0;
	}
};
struct IStrokeFeature
{
	virtual UINT GetId()=0;
	virtual char SetLiveStateInChar(WORD wChar,char ciLiveState)=0;	//��������ָ���ַ��еĴ���״̬,0:��Լ��;>0���ַ�ָ����λ;<0�������ڸ��ַ�����
	virtual char LiveStateInChar(WORD wChar)=0;	//��������ָ���ַ��еĴ���״̬,0:��Լ��;>0���ַ�ָ����λ;<0�������ڸ��ַ�����
	virtual void AddSearchRegion(RECT& rcSearch)=0;
	virtual bool FromBmpFile(const char* szStrokeBmpFile)=0;
	virtual UINT ToBLOB(char* blob,UINT uBlobBufSize,DWORD dwVersion=0)=0;
	virtual void FromBLOB(char* blob,UINT uBlobSize,DWORD dwVersion=0)=0;
};
//ʶ���������ĸ֪ʶ��
struct IAlphabets
{
	virtual bool InitFromFolder(const char* charimagefile_folder,WORD font_serial,bool append_mode)=0;
	virtual void InitFromImageFile(const WORD wChar,const char* imagefile, const WORD wiFontSerial=0)=0;
	virtual int GetCharMonoImageData(const WORD wChar, IMAGE_DATA* imagedata,WORD fontserial=0)=0;	//ÿλ����1�����أ�0��ʾ�׵�,1��ʾ�ڵ�
	virtual void SetLowestRecogSimilarity(WORD fontserial,double fSimilarity)=0;
	virtual double LowestRecogSimilarity(WORD fontserial)=0;
	virtual void SetPreSplitChar(WORD fontserial,BOOL bPreSplitChar)=0;
	virtual BOOL IsPreSplitChar(WORD fontserial)=0;
	virtual UINT ToBLOB(char* blob,UINT uBlobBufSize,DWORD dwVersion=0)=0;
	virtual void FromBLOB(char* blob,UINT uBlobSize,DWORD dwVersion=0)=0;
	virtual bool ImportFontsFile(const char* fontsfilename)=0;
	virtual bool ExportFontsFile(const char* fontsfilename)=0;
	virtual void SetFontLibRootPath(const char* root_path)=0;
	virtual UINT InitStrokeFeatureByFiles(const char* szFolderPath)=0;
	//BOOL RecognizeData(CImageChar* pText,double minMatchCoef=0.7,MATCHCOEF* pMachCoef=NULL);
};
struct IRecoginizer
{
	struct BOMPART{
		UINT id;
		int iSeg;
		static const BYTE ANGLE = 2;//�Ǹ�
		static const BYTE PLATE = 3;//�ְ�
		static const BYTE TUBE  = 4;//�ֹ�
		static const BYTE FLAT  = 5;//���
		static const BYTE SLOT  = 6;//�۸�
		static const BYTE ROUND = 7;//Բ��
		static const BYTE ACCESSORY = 8;//����
		WORD wPartType;		//��������
		//����������
		const static BYTE SUB_TYPE_TUBE_MAIN	= 1;	//����
		const static BYTE SUB_TYPE_TUBE_BRANCH	= 2;	//֧��
		const static BYTE SUB_TYPE_PLATE_C		= 3;	//���Ͳ��
		const static BYTE SUB_TYPE_PLATE_U		= 4;	//U�Ͳ��
		const static BYTE SUB_TYPE_PLATE_X		= 5;	//ʮ�ֲ��
		const static BYTE SUB_TYPE_PLATE_FL		= 6;	//����
		const static BYTE SUB_TYPE_PLATE_WATER	= 7;	//��ˮ��
		const static BYTE SUB_TYPE_FOOT_FL		= 8;	//�׽ŷ���
		const static BYTE SUB_TYPE_ROD_Z		= 9;	//����
		const static BYTE SUB_TYPE_ROD_F		= 10;	//����
		const static BYTE SUB_TYPE_ANGLE_SHORT	= 11;	//�̽Ǹ�
		const static BYTE SUB_TYPE_PLATE_FLD	= 12;	//�Ժ�����
		const static BYTE SUB_TYPE_PLATE_FLP	= 13;	//ƽ������
		const static BYTE SUB_TYPE_PLATE_FLG	= 14;	//���Է���
		const static BYTE SUB_TYPE_PLATE_FLR	= 15;	//���Է���
		const static BYTE SUB_TYPE_BOLT_PAD		= 16;	//��˨���
		const static BYTE SUB_TYPE_TUBE_WIRE	= 17;	//�׹�
		const static BYTE SUB_TYPE_STEEL_GRATING= 18;	//�ָ�դ
		const static BYTE SUB_TYPE_STAY_ROPE	= 19;	//��������
		const static BYTE SUB_TYPE_LADDER		= 20;	//����
		
		short siSubType;	//����������
		char sLabel[16];	//�������
		char sSizeStr[16];	//��L300*20
		char materialStr[8];//����
		short count;		//��������
		double width,thick;
		double length;		//����,mm
		double calWeight;
		double calSumWeight;
		double weight;		//����(kg)
		double sumWeight;	//����
		static const int MODIFY_LABEL	= 0x01;
		static const int MODIFY_MATERIAL= 0x02;
		static const int MODIFY_SPEC	= 0x04;
		static const int MODIFY_LENGTH	= 0x08;
		static const int MODIFY_COUNT	= 0x10;
		static const int MODIFY_WEIGHT	= 0x20;
		static const int MODIFY_SUM_WEIGHT = 0x40;
		BYTE arrItemWarningLevel[7];//0.����;1.����&���;2.����;3.������;4.����;5.����;6.��ע �еľ�ʾ��Ϣ��Խ��Խ��Ҫ��ʾ
		BYTE ciSrcFromMode;	//0.ͼƬʶ��ÿһλ����һ�����Ե��޸�״̬;
		RECT rc;			//ֻ�����Զ�ʶ��ʱ��Ч�����й�����CImageDataRegion�ж�Ӧ�ľ�������
		bool bInput;		//�ֶ�����
		void Clear(){memset(this,0,sizeof(BOMPART));}
		BOMPART(){Clear();}
	};
public:
	virtual void FromBLOB(char* blob,UINT uBlobSize)=0;
	virtual int GetBomParts(BOMPART* parts)=0;
	virtual void Recoginize(long idImageRegion);
};
struct IImageFile;
struct IImageRegion{
	virtual long GetSerial()=0;
	virtual IImageFile* BelongImageData() const =0;
	virtual POINT TopLeft(bool bRealRegion=false)=0;
	virtual POINT TopRight(bool bRealRegion=false)=0;
	virtual POINT BtmLeft(bool bRealRegion=false)=0;
	virtual POINT BtmRight(bool bRealRegion=false)=0;
	virtual bool SetRegion(POINT topLeft,POINT btmLeft,POINT btmRight,POINT topRight,bool bRealRegion=false,bool bRetrieveRegion=true)=0;
	virtual void UpdateImageRegion()=0;
	virtual void InitImageShowPara(RECT showRect)=0;
	virtual int GetMonoImageData(IMAGE_DATA* imagedata)=0;	//ÿλ����1�����أ�0��ʾ�׵�,1��ʾ�ڵ�
	virtual int GetRectMonoImageData(RECT* rc,IMAGE_DATA* imagedata)=0;		//ÿλ����1�����أ�0��ʾ�׵�,1��ʾ�ڵ�
	virtual int GetImageColumnWidth(int* col_width_arr=NULL)=0;		//��������
	virtual int GetHeight()=0;
	virtual int GetWidth()=0;
	virtual void SetScrOffset(SIZE offset)=0;
	virtual SIZE GetScrOffset()=0;
	virtual bool IsCanZoom()=0;
	virtual double Zoom(short zDelta,POINT* pxScrPoint=NULL)=0;
	virtual double ZoomRatio()=0;	//���ű���=��Ļ�ߴ�/ԭʼͼ��ߴ�
	virtual POINT MappingToScrOrg()=0;
	virtual RECT  MappingToScrRect()=0;
#ifdef _DEBUG
	virtual int Recognize(char modeOfAuto0BomPart1Summ2=0,int iCellRowStart=-1,int iCellRowEnd=-1,int iCellCol=-1)=0;	//-1��ʾȫ��,>=0��ʾָ���л��� wjh-2018.3.14
#else
	virtual int Recognize(char modeOfAuto0BomPart1Summ2=0)=0;
#endif
	virtual bool RetrieveCharImageData(int iCellRow,int iCellCol,IMAGE_CHARSET_DATA* imagechardata)=0;
	virtual bool Destroy()=0;	//BelongImageData()->DeleteImageRegion(GetSerial())
	virtual bool EnumFirstBomPart(IRecoginizer::BOMPART* pBomPart)=0;
	virtual bool EnumNextBomPart(IRecoginizer::BOMPART* pBomPart)=0;
	virtual UINT GetBomPartNum()=0;
	virtual bool GetBomPartRect(UINT idPart,RECT* rc)=0;
public:	//�����Ҷ�ͼתΪ�ڰ�ͼ�ֽ�����ֵ����ر���������
	//balancecoef ���ֺڰ׵��Ľ��޵���ƽ��ϵ��,ȡֵ-1.0~1.0
	virtual double SetMonoThresholdBalanceCoef(double mono_balance_coef=0)=0;
	virtual double GetMonoThresholdBalanceCoef()=0;
	virtual int get_MonoForwardPixels()=0;
	__declspec(property(get=get_MonoForwardPixels)) int MonoForwardPixels;
};
struct IImageFile{
	static const BYTE RAW_IMAGE_NONE	= 0;	//���Ҷ�ͼ�洢
	static const BYTE RAW_IMAGE_JPG		= 1;	//JPG
	static const BYTE RAW_IMAGE_PNG		= 2;	//PNG
	static const BYTE RAW_IMAGE_PDF		= 3;	//PDF��ʽ�洢��ʸ���ʻ�ͼ
	static const BYTE RAW_IMAGE_PDF_IMG	= 4;	//PDF��ʽ�洢��ͼ��
	virtual long GetSerial()=0;
	virtual bool InitImageFileHeader(const char* imagefile)=0;
	virtual bool InitImageFile(const char* imagefile,const char* file_path,bool update_file_path=true,PDF_FILE_CONFIG *pPDFConfig=NULL)=0;
	virtual void GetPathFileName(char* imagename,int maxStrBufLen=17)=0;
	virtual BYTE ImageCompressType()=0;
	virtual bool SetImageRawFileData(char* imagedata,UINT uiMaxDataSize,BYTE biFileType,PDF_FILE_CONFIG *pPDFConfig=NULL)=0;
	virtual bool SetCompressImageData(char* imagedata,UINT uiMaxDataSize)=0;
	virtual UINT GetCompressImageData(char* imagedata,UINT uiMaxDataSize=0)=0;
	virtual double SetMonoThresholdBalanceCoef(double balance=0,bool bUpdateMonoImages=false)=0;
	virtual double GetMonoThresholdBalanceCoef()=0;
	virtual int GetHeight()=0;
	virtual int GetWidth()=0;
	virtual BYTE GetRawFileType()=0;
	virtual bool IsSrcFromPdfFile()=0;
	virtual int Get24BitsImageEffWidth()=0;
	virtual int Get24BitsImageData(IMAGE_DATA* imagedata)=0;
	virtual int GetMonoImageData(IMAGE_DATA* imagedata)=0;
	virtual bool TurnClockwise90()=0;		//���ļ���ͼ��˳ʱ��ת90�ȣ���ת��Ӱ������ȡ����Ӧ����ȡ����֮ǰ��ת
	virtual bool TurnAntiClockwise90()=0;	//���ļ���ͼ����ʱ��ת90�ȣ���ת��Ӱ������ȡ����Ӧ����ȡ����֮ǰ��ת
	virtual bool IsBlackPixel(int i,int j)=0;
	virtual void InitImageShowPara(RECT showRect)=0;
	virtual RECT GetImageDisplayRect(DWORD dwRegionKey=0)=0;
	virtual bool DeleteImageRegion(long idRegion)=0;
	virtual void SetScrOffset(SIZE offset)=0;
	virtual SIZE GetScrOffset()=0;
	virtual bool IsCanZoom()=0;
	virtual double Zoom(short zDelta,POINT* pxScrPoint=NULL)=0;
	virtual double ZoomRatio()=0;	//���ű���=��Ļ�ߴ�/ԭʼͼ��ߴ�
	virtual bool FocusPoint(POINT xImgPoint,POINT* pxScrPoint=NULL,double scaleofScr2Img=2.0)=0;
	virtual POINT MappingToScrOrg()=0;
	virtual RECT  MappingToScrRect()=0;
	virtual bool IntelliRecogCornerPoint(const POINT& pick,POINT* pCornerPoint,
				 char ciType_LT0_RT1_RB2_LB3=0,int *pyjHoriTipOffset=NULL,UINT uiTestWidth=300,UINT uiTestHeight=200)=0;
	virtual IImageRegion* AddImageRegion()=0;
	virtual IImageRegion* AddImageRegion(POINT topLeft,POINT btmLeft,POINT btmRight,POINT topRight)=0;
	virtual IImageRegion* EnumFirstRegion()=0;
	virtual IImageRegion* EnumNextRegion()=0;
	virtual void UpdateImageRegions()=0;
};
struct RECOG_API IMonoImage{
	virtual long Serial()=0;
	virtual bool IsNoisePixel(int i,int j,RECT* pValidRgn=NULL,int maxIslandBlackPixels=5)=0;
	virtual long RemoveNoisePixels()=0;
	virtual bool InitImageSize(int width=0,int height=0)=0;
	virtual bool IsBlackPixel(int i,int j)=0;
	virtual bool SetPixelState(int i,int j,bool black=true)=0;
	virtual long GetImageWidth()=0;		//��ȡͼ��ÿ�е���Ч������
	virtual long GetImageHeight()=0;	//��ȡͼ��ÿ�е���Ч������
	static IMonoImage* CreateObject();
	static BOOL RemoveObject(long serial);
	static IMonoImage* PresetObject(int presetObjSerialMax4=1);
};
class RECOG_API IMindRobot {
public:
	static BYTE GetMaxWaringMatchCoef();
	static BYTE SetMaxWaringMatchCoef(BYTE ciWarningPercent=80);
	static void SetWorkDirectory(const char* szWorkFilePath);
	static void GetWorkDirectory(char* szWorkFilePath,int maxLen);
	//��ȡʶ���������ĸ֪ʶ��
	static IAlphabets* GetAlphabetKnowledge();
	//ԭʼ��ʶ��BOMͼ���ļ�����
	static IImageFile* AddImageFile(const char* imagefile,bool loadFileInstant=true);
	static IImageFile* FromImageFileSerial(long serial);
	static IImageFile* EnumFirstImage();
	static IImageFile* EnumNextImage();
	static BOOL DestroyImageFile(long serial);
};

class RECOG_API IPdfReader {
public:
	struct PDF_PAGE_INFO{
		int iPageNo;
		char sPageName[200];
		PDF_PAGE_INFO(){iPageNo=1;strcpy(sPageName,"");}
	};
public:
	virtual bool OpenPdfFile(const char* file_path)=0;
	virtual PDF_PAGE_INFO *EnumFirstPage()=0;
	virtual PDF_PAGE_INFO *EnumNextPage()=0;
	virtual int PageCount()=0;
};

class RECOG_API CPdfReaderInstance
{
	IPdfReader *m_pPdfReader;
public:
	CPdfReaderInstance();
	~CPdfReaderInstance();
	IPdfReader *GetPdfReader(){ return m_pPdfReader;}
};