// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 RECOG_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何其他项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// RECOG_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。
#ifdef APP_EMBEDDED_MODULE_ENCRYPT
#define RECOG_API	//内嵌时RECOG_API什么都不代表
#else
#ifdef RECOG_EXPORTS
#define RECOG_API __declspec(dllexport)
#else
#define RECOG_API __declspec(dllimport)
#endif
#endif
#pragma once

// 此类是从 Recog.dll 导出的
struct PDF_FILE_CONFIG{
	int page_no;
	int rotation;
	double zoom_scale;
	static const BYTE  LINE_MODE_GENERAL	= 0;	//正常模式
	static const BYTE  LINE_MODE_THIN		= 1;	//细字体
	static const BYTE  LINE_MODE_THICK		= 2;	//粗字体
	static const BYTE  LINE_MODE_RANGE		= 3;	//宽度范围
	static const BYTE  LINE_MODE_NONE		= 255;	//未初始化
	BYTE m_ciPdfLineMode;	//0.正常模式|1.细字体|2.粗字体|3.自定义|255.未初始化(随配置文件)
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
	BYTE biBitCount;	//单像素所占用比特位数
	char* imagedata;	//需要在调用方分配和释放内存(行扫描形式为Downward, top->bottom)
	IMAGE_DATA(){imagedata=NULL;nEffWidth=nWidth=nHeight=0;biBitCount=24;}
};
union XHWCHAR{
	WORD wHz;
	char chars[2];	//首字母加下划线'_'是为了写代码时与Label()函数智能拼写混 wjh-2014.8.19
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
	UINT uiPreAllocSize;	//imagedata!=NULL时表示预分配的内存空间大小
	char* imagedata;		//需要在调用方分配和释放内存(行扫描形式为Downward, top->bottom)
	WORD wPreAllocCharCount;//pPreAllocCharSet中预分配的字符对象数量
	struct CHAR_IMAGE{
		WORD wChar;	//可表示汉字
		WORD wiStartPos,wiEndPos;	//当前字符的起始与结束位置
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
	virtual char SetLiveStateInChar(WORD wChar,char ciLiveState)=0;	//该特征在指定字符中的存在状态,0:无约定;>0在字符指定部位;<0不允许在该字符出现
	virtual char LiveStateInChar(WORD wChar)=0;	//该特征在指定字符中的存在状态,0:无约定;>0在字符指定部位;<0不允许在该字符出现
	virtual void AddSearchRegion(RECT& rcSearch)=0;
	virtual bool FromBmpFile(const char* szStrokeBmpFile)=0;
	virtual UINT ToBLOB(char* blob,UINT uBlobBufSize,DWORD dwVersion=0)=0;
	virtual void FromBLOB(char* blob,UINT uBlobSize,DWORD dwVersion=0)=0;
};
//识别机器人字母知识库
struct IAlphabets
{
	virtual bool InitFromFolder(const char* charimagefile_folder,WORD font_serial,bool append_mode)=0;
	virtual void InitFromImageFile(const WORD wChar,const char* imagefile, const WORD wiFontSerial=0)=0;
	virtual int GetCharMonoImageData(const WORD wChar, IMAGE_DATA* imagedata,WORD fontserial=0)=0;	//每位代表1个像素，0表示白点,1表示黑点
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
		static const BYTE ANGLE = 2;//角钢
		static const BYTE PLATE = 3;//钢板
		static const BYTE TUBE  = 4;//钢管
		static const BYTE FLAT  = 5;//扁钢
		static const BYTE SLOT  = 6;//槽钢
		static const BYTE ROUND = 7;//圆钢
		static const BYTE ACCESSORY = 8;//附件
		WORD wPartType;		//构件类型
		//构件子类型
		const static BYTE SUB_TYPE_TUBE_MAIN	= 1;	//主管
		const static BYTE SUB_TYPE_TUBE_BRANCH	= 2;	//支管
		const static BYTE SUB_TYPE_PLATE_C		= 3;	//槽型插板
		const static BYTE SUB_TYPE_PLATE_U		= 4;	//U型插板
		const static BYTE SUB_TYPE_PLATE_X		= 5;	//十字插板
		const static BYTE SUB_TYPE_PLATE_FL		= 6;	//法兰
		const static BYTE SUB_TYPE_PLATE_WATER	= 7;	//遮水板
		const static BYTE SUB_TYPE_FOOT_FL		= 8;	//底脚法兰
		const static BYTE SUB_TYPE_ROD_Z		= 9;	//主材
		const static BYTE SUB_TYPE_ROD_F		= 10;	//辅材
		const static BYTE SUB_TYPE_ANGLE_SHORT	= 11;	//短角钢
		const static BYTE SUB_TYPE_PLATE_FLD	= 12;	//对焊法兰
		const static BYTE SUB_TYPE_PLATE_FLP	= 13;	//平焊法兰
		const static BYTE SUB_TYPE_PLATE_FLG	= 14;	//刚性法兰
		const static BYTE SUB_TYPE_PLATE_FLR	= 15;	//柔性法兰
		const static BYTE SUB_TYPE_BOLT_PAD		= 16;	//螺栓垫板
		const static BYTE SUB_TYPE_TUBE_WIRE	= 17;	//套管
		const static BYTE SUB_TYPE_STEEL_GRATING= 18;	//钢格栅
		const static BYTE SUB_TYPE_STAY_ROPE	= 19;	//拉索构件
		const static BYTE SUB_TYPE_LADDER		= 20;	//爬梯
		
		short siSubType;	//构件子类型
		char sLabel[16];	//构件编号
		char sSizeStr[16];	//如L300*20
		char materialStr[8];//材质
		short count;		//单基件数
		double width,thick;
		double length;		//长度,mm
		double calWeight;
		double calSumWeight;
		double weight;		//单重(kg)
		double sumWeight;	//总重
		static const int MODIFY_LABEL	= 0x01;
		static const int MODIFY_MATERIAL= 0x02;
		static const int MODIFY_SPEC	= 0x04;
		static const int MODIFY_LENGTH	= 0x08;
		static const int MODIFY_COUNT	= 0x10;
		static const int MODIFY_WEIGHT	= 0x20;
		static const int MODIFY_SUM_WEIGHT = 0x40;
		BYTE arrItemWarningLevel[7];//0.件号;1.材质&规格;2.长度;3.单基数;4.单重;5.总重;6.备注 列的警示信息，越高越需要警示
		BYTE ciSrcFromMode;	//0.图片识别，每一位代表一个属性的修改状态;
		RECT rc;			//只有在自动识别时有效，该行构件在CImageDataRegion中对应的矩形区域
		bool bInput;		//手动输入
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
	virtual int GetMonoImageData(IMAGE_DATA* imagedata)=0;	//每位代表1个像素，0表示白点,1表示黑点
	virtual int GetRectMonoImageData(RECT* rc,IMAGE_DATA* imagedata)=0;		//每位代表1个像素，0表示白点,1表示黑点
	virtual int GetImageColumnWidth(int* col_width_arr=NULL)=0;		//返回列数
	virtual int GetHeight()=0;
	virtual int GetWidth()=0;
	virtual void SetScrOffset(SIZE offset)=0;
	virtual SIZE GetScrOffset()=0;
	virtual bool IsCanZoom()=0;
	virtual double Zoom(short zDelta,POINT* pxScrPoint=NULL)=0;
	virtual double ZoomRatio()=0;	//缩放比例=屏幕尺寸/原始图像尺寸
	virtual POINT MappingToScrOrg()=0;
	virtual RECT  MappingToScrRect()=0;
#ifdef _DEBUG
	virtual int Recognize(char modeOfAuto0BomPart1Summ2=0,int iCellRowStart=-1,int iCellRowEnd=-1,int iCellCol=-1)=0;	//-1表示全部,>=0表示指定行或列 wjh-2018.3.14
#else
	virtual int Recognize(char modeOfAuto0BomPart1Summ2=0)=0;
#endif
	virtual bool RetrieveCharImageData(int iCellRow,int iCellCol,IMAGE_CHARSET_DATA* imagechardata)=0;
	virtual bool Destroy()=0;	//BelongImageData()->DeleteImageRegion(GetSerial())
	virtual bool EnumFirstBomPart(IRecoginizer::BOMPART* pBomPart)=0;
	virtual bool EnumNextBomPart(IRecoginizer::BOMPART* pBomPart)=0;
	virtual UINT GetBomPartNum()=0;
	virtual bool GetBomPartRect(UINT idPart,RECT* rc)=0;
public:	//调整灰度图转为黑白图分界线阈值的相关变量及函数
	//balancecoef 划分黑白点间的界限调整平衡系数,取值-1.0~1.0
	virtual double SetMonoThresholdBalanceCoef(double mono_balance_coef=0)=0;
	virtual double GetMonoThresholdBalanceCoef()=0;
	virtual int get_MonoForwardPixels()=0;
	__declspec(property(get=get_MonoForwardPixels)) int MonoForwardPixels;
};
struct IImageFile{
	static const BYTE RAW_IMAGE_NONE	= 0;	//按灰度图存储
	static const BYTE RAW_IMAGE_JPG		= 1;	//JPG
	static const BYTE RAW_IMAGE_PNG		= 2;	//PNG
	static const BYTE RAW_IMAGE_PDF		= 3;	//PDF形式存储的矢量笔画图
	static const BYTE RAW_IMAGE_PDF_IMG	= 4;	//PDF形式存储的图像
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
	virtual bool TurnClockwise90()=0;		//将文件中图像顺时针转90度，旋转不影响已提取区域，应在提取区域之前旋转
	virtual bool TurnAntiClockwise90()=0;	//将文件中图像逆时针转90度，旋转不影响已提取区域，应在提取区域之前旋转
	virtual bool IsBlackPixel(int i,int j)=0;
	virtual void InitImageShowPara(RECT showRect)=0;
	virtual RECT GetImageDisplayRect(DWORD dwRegionKey=0)=0;
	virtual bool DeleteImageRegion(long idRegion)=0;
	virtual void SetScrOffset(SIZE offset)=0;
	virtual SIZE GetScrOffset()=0;
	virtual bool IsCanZoom()=0;
	virtual double Zoom(short zDelta,POINT* pxScrPoint=NULL)=0;
	virtual double ZoomRatio()=0;	//缩放比例=屏幕尺寸/原始图像尺寸
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
	virtual long GetImageWidth()=0;		//获取图像每行的有效象素数
	virtual long GetImageHeight()=0;	//获取图像每列的有效象素数
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
	//获取识别机器人字母知识库
	static IAlphabets* GetAlphabetKnowledge();
	//原始待识别BOM图像文件函数
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