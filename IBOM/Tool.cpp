#include "stdafx.h"
#include "Tool.h"
#include "XhCharString.h"
#include "ProcBarDlg.h"

char APP_PATH[MAX_PATH];
void GetAppPath(char* App_Path,const char *src_path/*=NULL*/,char *file_name/*=NULL*/,char *extension/*=NULL*/)
{
	char drive[4];
	char dir[MAX_PATH];
	char fname[MAX_PATH];
	char ext[MAX_PATH];
	if(src_path)
		_splitpath(src_path,drive,dir,fname,ext);
	else
		_splitpath(__argv[0],drive,dir,fname,ext);
	if(App_Path)
	{
		strcpy(App_Path,drive);
		strcat(App_Path,dir);
	}
	if(file_name)
		strcpy(file_name,fname);
	if(extension)
		strcpy(extension,ext);
}
LONG SetRegKey(HKEY key,const char* subkey,const char* sEntry,const char* sValue)
{
	HKEY hkey;
	LONG retval=RegOpenKeyEx(key,subkey,0,KEY_WRITE, &hkey);
	if(retval == ERROR_SUCCESS) 
	{
		DWORD dwLength=strlen(sValue);
		retval=RegSetValueEx(hkey,_T(sEntry),NULL,REG_SZ,(BYTE*)&sValue[0],dwLength);
		RegCloseKey(hkey);
	}
	return retval;
}
LONG GetRegKey(HKEY key,const char* subkey,const char* sEntry,char* retdata)
{
	HKEY hkey;
	LONG retval=RegOpenKeyEx(key,subkey,0,KEY_READ, &hkey);
	if(retval == ERROR_SUCCESS) 
	{
		char data[MAX_PATH]="";
		DWORD dwType,dwCount = MAX_PATH;
		retval=RegQueryValueEx(hkey,_T(sEntry),NULL,&dwType,(BYTE*)&data[0], &dwCount);
		if(retval==ERROR_SUCCESS)
			strcpy(retdata,data);
		RegCloseKey(hkey);
	}
	return retval;
}

BOOL GetCadPathFromReg(const char* cad_version,char *cad_path)
{
	char sCurVer[MAX_PATH]="",sSubKey[MAX_PATH]="";
	sprintf(sSubKey,"Software\\Autodesk\\AutoCAD\\%s",cad_version);
	if(GetRegKey(HKEY_CURRENT_USER,sSubKey,"CurVer",sCurVer)!=ERROR_SUCCESS)
		return FALSE;
	sprintf(sSubKey,"SOFTWARE\\Autodesk\\AutoCAD\\%s\\%s",cad_version,sCurVer);
	if(GetRegKey(HKEY_LOCAL_MACHINE,sSubKey,"AcadLocation",cad_path)!=ERROR_SUCCESS)
		return FALSE;
	if(strlen(cad_path)>1)
	{
		strcat(cad_path,"\\");
		return TRUE;
	}
	else
		return FALSE;
}

BOOL ReadCadPath(char* cad_path)
{
	//优先从CAD_PATH中读取用户设定的CAD路径
	char sCurVer[MAX_PATH]="",sSubKey[MAX_PATH]="";
	strcpy(sSubKey,"Software\\Xerofox\\IBOM\\Settings");
	GetRegKey(HKEY_CURRENT_USER,sSubKey,"CAD_PATH",cad_path);
	if(strlen(cad_path)<=0)
	{	//从注册表中自动查询CAD的安装路径
		BOOL bRetCode=FALSE;
		for(int i=0;i<3;i++)
		{
			int main_verison=16+i;
			for(int j=0;j<10;j++)
			{
				CXhChar16 sCadVersion("R%d.%d",main_verison,j);
				bRetCode=GetCadPathFromReg(sCadVersion,cad_path);
				if(bRetCode)
					break;
			}
			if(bRetCode)
				break;
		}
		return bRetCode;
	}
	return TRUE;
}
BOOL ParseSpec(char* spec_str,double* thick,double* width,int* cls_id,int *sub_type/*=NULL*/,bool bPartLabelStr/*=false*/,char *real_spec/*=NULL*/,char* part_material/*=NULL*/)
{
	if(spec_str==NULL||strlen(spec_str)<=0)
		return FALSE;
	CXhChar50 sSpec(spec_str);
	char* validstr=NULL;
	CXhChar16 material;
	int inter_cls_id=0,inter_sub_type=0;
	double inter_width=0,inter_thick=0;
	char first_char=spec_str[0];
	char sec_char=strlen(spec_str)>1?spec_str[1]:' ';
	char third_char=strlen(spec_str)>2?spec_str[2]:' ';
	if(bPartLabelStr)
	{
		if( strstr(spec_str,"-")==NULL&&
			(first_char=='F'||first_char=='X'||first_char=='C'||first_char=='X'||first_char=='U'))
		{
			inter_cls_id=IRecoginizer::BOMPART::PLATE;
			if(first_char=='F')
			{
				inter_sub_type=IRecoginizer::BOMPART::SUB_TYPE_PLATE_FL;
				if(sec_char=='D')
					inter_sub_type=IRecoginizer::BOMPART::SUB_TYPE_PLATE_FLD;
				else if(sec_char=='P')
					inter_sub_type=IRecoginizer::BOMPART::SUB_TYPE_PLATE_FLP;
				else if(sec_char=='G')
					inter_sub_type=IRecoginizer::BOMPART::SUB_TYPE_PLATE_FLG;
				else if(sec_char=='R')
					inter_sub_type=IRecoginizer::BOMPART::SUB_TYPE_PLATE_FLR;
			}
			else if(first_char=='X')
				inter_sub_type=IRecoginizer::BOMPART::SUB_TYPE_PLATE_X;
			else if(first_char=='C')
				inter_sub_type=IRecoginizer::BOMPART::SUB_TYPE_PLATE_C;
			else if(first_char=='U')
				inter_sub_type=IRecoginizer::BOMPART::SUB_TYPE_PLATE_U;
		}
		else if( (first_char=='G'&&sec_char=='J'&&third_char=='-')||	//GJ-50 钢绞线
				 (first_char=='N'&&sec_char=='U'&&third_char=='T')||	//UT型线夹
				 (first_char=='U'&&sec_char=='-')||						//U型挂环
				 (first_char=='J'&&sec_char=='K'&&third_char=='-')||	//钢线夹子
				 (first_char=='N'&&sec_char=='X'&&third_char=='-')||	//楔形线夹
				 first_char=='T')
		{
			inter_cls_id=IRecoginizer::BOMPART::ACCESSORY;
			if(first_char=='T')
				inter_sub_type=IRecoginizer::BOMPART::SUB_TYPE_LADDER;	//爬梯
			else
				inter_sub_type=IRecoginizer::BOMPART::SUB_TYPE_STAY_ROPE;	//拉索构件
		}
	}
	else if(((validstr=strstr(spec_str,"L"))!=NULL)||(validstr=strstr(spec_str,"∠"))!=NULL)
	{
		int materialstr_len=validstr-spec_str;
		if(materialstr_len>0)
			material.NCopy((char*)sSpec,materialstr_len);
		memmove((char*)sSpec,validstr,sSpec.Length-materialstr_len+1);
		if(real_spec!=NULL)
			strcpy(real_spec,sSpec);
		inter_cls_id=IRecoginizer::BOMPART::ANGLE;	//角钢
		sSpec.Replace("L","");
		sSpec.Replace("∠","");
		sSpec.Replace("×"," ");
		sSpec.Replace("X"," ");
		sSpec.Replace("x"," ");
		sSpec.Replace("*"," ");
		sSpec.Replace("?"," ");	//调试状态下识别错误时系统默认待处理字符
		validstr++;
		sscanf(sSpec,"%lf%lf",&inter_width,&inter_thick);
	}
	else if((validstr=strstr(spec_str,"-"))!=NULL&&(validstr==spec_str||(validstr-spec_str)==4))
	{
		int materialstr_len=validstr-spec_str;
		if(materialstr_len>0)
			material.NCopy((char*)sSpec,materialstr_len);
		inter_cls_id=IRecoginizer::BOMPART::PLATE;	//钢板
		memmove((char*)sSpec,validstr+1,sSpec.Length);
		if(real_spec!=NULL)
			strcpy(real_spec,sSpec);
		if(strstr(sSpec,"×")||strstr(sSpec,"x")||strstr(sSpec,"*")||strstr(sSpec,"X"))
		{
			sSpec.Replace("×"," ");
			sSpec.Replace("X"," ");
			sSpec.Replace("x"," ");
			sSpec.Replace("*"," ");
			sSpec.Replace("?"," ");	//调试状态下识别错误时系统默认待处理字符
			sscanf(sSpec,"%lf%lf",&inter_thick,&inter_width);
		}
		else
			inter_thick=atof(sSpec);
	}
	else if(((validstr=strstr(spec_str,"φ"))!=NULL)||(validstr=strstr(spec_str,"Φ"))!=NULL)
	{
		if(real_spec!=NULL)
			strcpy(real_spec,validstr);
		int materialstr_len=validstr-spec_str;
		if(materialstr_len>0)
			material.NCopy((char*)sSpec,materialstr_len);
		int hits=sSpec.Replace("φ"," ");
		hits+=sSpec.Replace("Φ"," ");
		sSpec.Replace("/"," ");
		sSpec.Replace("\\"," ");
		inter_cls_id=IRecoginizer::BOMPART::TUBE;	//钢管
		if(hits==2)
		{
			inter_cls_id=IRecoginizer::BOMPART::SUB_TYPE_TUBE_WIRE;	//套管
			sscanf(sSpec,"%lf%lf",&inter_thick,&inter_width);
		}
		else
		{
			sSpec.Copy(validstr);
			sSpec.Replace("φ"," ");
			sSpec.Replace("Φ"," ");
			int nXCount=sSpec.Replace("×"," ");
			nXCount+=sSpec.Replace("X"," ");
			nXCount+=sSpec.Replace("x"," ");
			nXCount+=sSpec.Replace("*"," ");
			sSpec.Replace("?"," ");	//调试状态下识别错误时系统默认待处理字符
			sscanf(sSpec,"%lf%lf",&inter_width,&inter_thick);
			if(nXCount<=0)	//圆钢
				inter_cls_id=IRecoginizer::BOMPART::ROUND;
		}
	}
	else if(((validstr=strstr(spec_str,"["))!=NULL)||(strlen(spec_str)==2&&spec_str[0]=='C'))
	{	//槽钢
		int materialstr_len=validstr-spec_str;
		if(materialstr_len>0)
			material.NCopy((char*)sSpec,materialstr_len);
		if(real_spec!=NULL)
			strcpy(real_spec,validstr);
		inter_cls_id=IRecoginizer::BOMPART::SLOT;
	}
	else
		return FALSE;
	if(width!=NULL)
		*width=inter_width;
	if(thick!=NULL)
		*thick=inter_thick;
	if(cls_id!=NULL)
		*cls_id=inter_cls_id;
	if(sub_type!=NULL)
		*sub_type=inter_sub_type;
	if(part_material)
		strcpy(part_material,material);
	return TRUE;
}

BOOL ParseSpec(IRecoginizer::BOMPART *pBomPart)
{
	if(pBomPart==NULL||pBomPart->sSizeStr==NULL)
		return FALSE;
	int cls_id=0,sub_type=0;
	double width=0,thick=0;
	if( ParseSpec(pBomPart->sSizeStr,&thick,&width,&cls_id,&sub_type)||
		(strlen(pBomPart->sSizeStr)==0&&ParseSpec(pBomPart->sLabel,&thick,&width,&cls_id,&sub_type,TRUE)))
	{
		pBomPart->width=width;
		pBomPart->thick=thick;
		pBomPart->wPartType=cls_id;
		pBomPart->siSubType=(short)sub_type;
		return TRUE;
	}
	else
		return FALSE;
}

CProcBarDlg *pProcDlg=NULL;
void DisplayProcess(int percent,const char *sTitle)
{
	if(percent>=100)
	{
		if(pProcDlg!=NULL)
		{
			pProcDlg->DestroyWindow();
			delete pProcDlg;
			pProcDlg=NULL;
		}
		return;
	}
	else if(pProcDlg==NULL)
		pProcDlg=new CProcBarDlg(NULL);
	if(pProcDlg->GetSafeHwnd()==NULL)
		pProcDlg->Create();
	static int prevPercent;
	if(percent!=0&&percent==prevPercent)
		return;	//跟上次进度一致不需要更新
	else
		prevPercent=percent;
	if(sTitle)
		pProcDlg->SetTitle(CString(sTitle));
	else
#ifdef AFX_TARG_ENU_ENGLISH
		pProcDlg->SetTitle("Process");
#else 
		pProcDlg->SetTitle("进度");
#endif
	pProcDlg->Refresh(percent);
}