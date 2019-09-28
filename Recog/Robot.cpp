#include "stdafx.h"
#include "Hashtable.h"
#include "LogFile.h"
#include ".\Robot.h"
#include "XhCharString.h"

#ifdef APP_EMBEDDED_MODULE_ENCRYPT
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
#endif


CMindRobot robot;

CMindRobot::CMindRobot()
{
	strcpy(szTempFilePath,"c:\\");;
}
BYTE IMindRobot::GetMaxWaringMatchCoef(){return WARNING_LEVEL::MAX_WARNING_WEIGHT_MATCH_COEF;}
BYTE IMindRobot::SetMaxWaringMatchCoef(BYTE ciWarningPercent/*=80*/)
{
	if(ciWarningPercent<60)
		ciWarningPercent=60;
	else if(ciWarningPercent>100)
		ciWarningPercent=100;

	return WARNING_LEVEL::MAX_WARNING_WEIGHT_MATCH_COEF=ciWarningPercent;
}
void IMindRobot::SetWorkDirectory(const char* szWorkFilePath)
{
	if(szWorkFilePath!=NULL)
		StrCopy(robot.szTempFilePath,szWorkFilePath,201);
}
void IMindRobot::GetWorkDirectory(char* szWorkFilePath,int maxLen)
{
	if(szWorkFilePath!=NULL)
		StrCopy(szWorkFilePath,robot.szTempFilePath,min(201,maxLen));
}
	//获取识别机器人字母知识库
IAlphabets* IMindRobot::GetAlphabetKnowledge()
{
	return &robot.knowledge;
}
static DWORD MAX_IMGFILE_MEMORY_POOL_SIZE=0x100000*200;	//0x100000=1M Bytes
//原始待识别BOM图像文件函数
IImageFile* IMindRobot::AddImageFile(const char* imagefile,bool loadFileInstant/*=true*/)
{
	if(robot.listImageFiles.GetNodeNum()==0)
		CMindRobot::InitCryptData();	//添加第1张图像文件时初始化加密函数
	CImageDataFile* pImageFile;
	UINT uiAllocImageBytes=0;
	for(pImageFile=robot.listImageFiles.GetTail();pImageFile;pImageFile=robot.listImageFiles.GetPrev())
	{
		uiAllocImageBytes+=pImageFile->GetInternalAllocRawImageBytes();
		if(uiAllocImageBytes>=MAX_IMGFILE_MEMORY_POOL_SIZE)
			pImageFile->image.ReleaseRawImageBits();
	}
	pImageFile=robot.listImageFiles.Add(0);
	if(!loadFileInstant)
		pImageFile->InitImageFileHeader(imagefile);
	else if(!pImageFile->InitImageFile(imagefile,NULL))
	{
		robot.listImageFiles.DeleteNode(pImageFile->m_idSerial);
		return NULL;
	}
	return pImageFile;
}
IImageFile* IMindRobot::FromImageFileSerial(long serial)
{
	return robot.listImageFiles.GetValue(serial);
}
IImageFile* IMindRobot::EnumFirstImage()
{
	return robot.listImageFiles.GetFirst();
}
IImageFile* IMindRobot::EnumNextImage()
{
	return robot.listImageFiles.GetNext();
}
BOOL IMindRobot::DestroyImageFile(long serial)
{
	BOOL retcode= robot.listImageFiles.DeleteNode(serial);
	robot.listImageFiles.Clean();
	return retcode;
}
