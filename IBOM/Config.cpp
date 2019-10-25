#include "StdAfx.h"
#include "Config.h"
#include "XhCharString.h"
#include "HashTable.h"
#include "imm.h"
#include "DataModel.h"
#include "Tool.h"

CHashList<CXhChar100> CConfig::hashTextByKey;
CHashStrList<CXhChar16> CConfig::hashAngleSpecByKey;
CXhChar100 CConfig::KEY_Q235=CXhChar16("R");
CXhChar100 CConfig::KEY_Q345=CXhChar16("T");
CXhChar100 CConfig::KEY_Q390=CXhChar16("Y");
CXhChar100 CConfig::KEY_Q420=CXhChar16("U");
CXhChar100 CConfig::KEY_REPEAT=CXhChar16(".");
CXhChar100 CConfig::KEY_LEFT=CXhChar16("S");
CXhChar100 CConfig::KEY_RIGHT=CXhChar16("F");
CXhChar100 CConfig::KEY_UP=CXhChar16("E");
CXhChar100 CConfig::KEY_DOWN=CXhChar16("D");
CXhChar100 CConfig::KEY_RETURN=CXhChar16("Enter");
CXhChar100 CConfig::KEY_HOME=CXhChar16("Home");
CXhChar100 CConfig::KEY_END=CXhChar16("End");
CXhChar100 CConfig::KEY_BIG_FAI=CXhChar16("O");
CXhChar100 CConfig::KEY_LITTLE_FAI=CXhChar16("P");
BOOL CConfig::m_bImmerseMode=FALSE;
BOOL CConfig::m_bTypewriterMode=TRUE;
BYTE CConfig::m_ciWaringMatchCoef=70;
COLORREF CConfig::MODIFY_COLOR=RGB(200,255,255);
COLORREF CConfig::DRAWING_ERROR_COLOR=RGB(106,106,255);
COLORREF CConfig::warningLevelClrArr[3]={0,RGB(200,150,0),RGB(245,185,19)};	//RGB(245,185,19)
BYTE CConfig::m_ciEditModel=1;				//0.单击 1.双击
BOOL CConfig::m_bDisplayPromptMsg=FALSE;	//显示提示信息
double CConfig::m_fInitPDFZoomScale=2.083333333333333;
int CConfig::m_iPdfLineMode=0;
double CConfig::m_fMinPdfLineWeight=0.3f;
double CConfig::m_fMinPdfLineFlatness=0.3f;
double CConfig::m_fMaxPdfLineWeight=1.2f;
double CConfig::m_fMaxPdfLineFlatness=1.2f;
int CConfig::m_iFontsLibSerial=0;
BOOL CConfig::m_bAutoSelectFontLib=FALSE;
BOOL CConfig::m_bListenScanner=TRUE;
UINT CConfig::m_nAutoSaveTime=120000;	//两分钟
UINT CConfig::m_iAutoSaveType=0;
BOOL CConfig::m_bRecogPieceWeight=FALSE;
BOOL CConfig::m_bRecogSumWeight=FALSE;
CXhChar100 CConfig::m_sOperator;	//操作员（制表人）
CXhChar100 CConfig::m_sAuditor;		//审核人
CXhChar100 CConfig::m_sCritic;		//评审人
double CConfig::m_fWeightEPS=0.5;	//重量对别误差值

CConfig::CConfig(void)
{
	m_bImmerseMode=FALSE;
	m_bTypewriterMode=TRUE;
}

CConfig::~CConfig(void)
{
}

void CConfig::SaveToFile(const char* file_path)
{
	FILE *fp;
	CString sFileName;
	char file_name[MAX_PATH];
	if(file_path==NULL)
	{
		GetAppPath(file_name);
		strcat(file_name,"IBOM.cfg");
	}
	else
		strcpy(file_name,file_path);
	if(file_name==NULL)
		return;
	if((fp=fopen(file_name,"wt"))==NULL)
	{
		return;
	}
	//颜色
	fprintf(fp,"警告系数及警告色\n");
	fprintf(fp,"DrawingErrorColor=%d ;图纸数据错误标识色\n",DRAWING_ERROR_COLOR);
	fprintf(fp,"ModifyColor=%d ;单元格修改之后的背景色\n",MODIFY_COLOR);
	fprintf(fp,"WarningLevel1Color=%d ;警告级别1背景色\n",warningLevelClrArr[1]);
	fprintf(fp,"WarningLevel2Color=%d ;警告级别2背景色\n",warningLevelClrArr[2]);
	fprintf(fp,"WaringMatchCoef=%d ;警告匹配系数\n",m_ciWaringMatchCoef);
	//
	fprintf(fp,"材质快捷键\n");
	fprintf(fp,"Q235=%s ;Q235快捷键\n",(char*)KEY_Q235);
	fprintf(fp,"Q345=%s ;Q345快捷键\n",(char*)KEY_Q345);
	fprintf(fp,"Q390=%s ;Q390快捷键\n",(char*)KEY_Q390);
	fprintf(fp,"Q420=%s ;Q420快捷键\n",(char*)KEY_Q420);
	//
	fprintf(fp,"材质快捷键\n");
	fprintf(fp,"Up=%s ;上快捷键\n",(char*)KEY_UP);
	fprintf(fp,"Down=%s ;下快捷键\n",(char*)KEY_DOWN);
	fprintf(fp,"Left=%s ;左快捷键\n",(char*)KEY_LEFT);
	fprintf(fp,"Right=%s ;右快捷键\n",(char*)KEY_RIGHT);
	fprintf(fp,"Repeat=%s ;重复快捷键\n",(char*)KEY_REPEAT);
	//
	fprintf(fp,"特殊字符快捷键\n");
	fprintf(fp,"BigFai=%s ;钢管直径Φ\n",(char*)KEY_BIG_FAI);
	fprintf(fp,"LittleFai=%s ;钢管直径φ\n",(char*)KEY_LITTLE_FAI);
	//
	fprintf(fp,"EditModel=%d	;编辑模式\n",m_ciEditModel);
	fprintf(fp,"AutoSelectFontLib=%d	;自动选择字体库\n",m_bAutoSelectFontLib);
	fprintf(fp,"FontsLibSerial=%d	;字体库序列\n",m_iFontsLibSerial);
	fprintf(fp,"DisplayPromptMsg=%d	;显示提示信息\n",m_bDisplayPromptMsg);
	fprintf(fp,"InitPDFZoomScale=%lf	;PDF默认缩放比例\n",m_fInitPDFZoomScale);
	fprintf(fp,"PdfLineMode=%d	;PDF线宽模式\n",m_iPdfLineMode);
	fprintf(fp,"MinPdfLineWeight=%lf	;PDF最小线宽\n",m_fMinPdfLineWeight);
	fprintf(fp,"MinPdfLineFlatness=%lf	;PDF最小厚度\n",m_fMinPdfLineFlatness);
	fprintf(fp,"MaxPdfLineWeight=%lf	;PDF最大线宽\n",m_fMaxPdfLineWeight);
	fprintf(fp,"MaxPdfLineFlatness=%lf	;PDF最大厚度\n",m_fMaxPdfLineFlatness);
	fprintf(fp,"ListenScanner=%d	;监听扫描仪\n",m_bListenScanner);
	fprintf(fp,"AutoSaveTime=%d	;自动保存时间，单位ms\n",m_nAutoSaveTime);
	fprintf(fp,"AutoSaveType=%d	;自动备份方式\n",m_iAutoSaveType);
	fprintf(fp,"RecogPieceWeight=%d	;识别单重\n",m_bRecogPieceWeight);
	fprintf(fp,"RecogSumWeight=%d	;识别总重\n",m_bRecogSumWeight);
	fprintf(fp,"WeightEPS=%.1f	;重量对比误差值\n",m_fWeightEPS);
	fclose(fp);
}

void CConfig::LoadFromFile(const char* file_path)
{
	char file_name[MAX_PATH];
	if(file_path==NULL)
	{
		GetAppPath(file_name);
		strcat(file_name,"IBOM.cfg");
	}
	else
		strcpy(file_name,file_path);
	if(file_name==NULL)
		return;
	FILE *fp=NULL;
	if((fp=fopen(file_name,"rt"))==NULL)
		return;

	CString sFileName,sLine;
	char line_txt[MAX_PATH],key_word[100];
	while(!feof(fp))
	{
		if(fgets(line_txt,MAX_PATH,fp)==NULL)
			break;
		char sText[MAX_PATH];
		strcpy(sText,line_txt);
		sLine=sText;
		sLine.Replace('=',' ');
		sprintf(line_txt,"%s",sLine);
		char *skey=strtok((char*)sText,"=,;");
		strncpy(key_word,skey,100);
		//常规设置
		if(_stricmp(key_word,"Left")==0)
			sscanf(line_txt,"%s %s",&key_word,&KEY_LEFT);
		else if(_stricmp(key_word,"Right")==0)
			sscanf(line_txt,"%s %s",&key_word,&KEY_RIGHT);
		else if(_stricmp(key_word,"Up")==0)
			sscanf(line_txt,"%s %s",&key_word,&KEY_UP);
		else if(_stricmp(key_word,"Down")==0)
			sscanf(line_txt,"%s %s",&key_word,&KEY_DOWN);
		else if(_stricmp(key_word,"Repeat")==0)
			sscanf(line_txt,"%s %s",&key_word,&KEY_REPEAT);
		else if(_stricmp(key_word,"Q235")==0)
			sscanf(line_txt,"%s %s",&key_word,&KEY_Q235);
		else if(_stricmp(key_word,"Q345")==0)
			sscanf(line_txt,"%s %s",&key_word,&KEY_Q345);
		else if(_stricmp(key_word,"Q390")==0)
			sscanf(line_txt,"%s %s",&key_word,&KEY_Q390);
		else if(_stricmp(key_word,"Q420")==0)
			sscanf(line_txt,"%s %s",&key_word,&KEY_Q420);
		else if(_stricmp(key_word,"DrawingErrorColor")==0)
			sscanf(line_txt,"%s %d",&key_word,&DRAWING_ERROR_COLOR);
		else if(_stricmp(key_word,"ModifyColor")==0)
			sscanf(line_txt,"%s %d",&key_word,&MODIFY_COLOR);
		else if(_stricmp(key_word,"WarningLevel1Color")==0)
			sscanf(line_txt,"%s %d",&key_word,&warningLevelClrArr[1]);
		else if(_stricmp(key_word,"WarningLevel2Color")==0)
			sscanf(line_txt,"%s %d",&key_word,&warningLevelClrArr[2]);
		else if(_stricmp(key_word,"WaringMatchCoef")==0)
			sscanf(line_txt,"%s %d",&key_word,&m_ciWaringMatchCoef);
		else if(_stricmp(key_word,"BigFai")==0)
			sscanf(line_txt,"%s %s",&key_word,&KEY_BIG_FAI);
		else if(_stricmp(key_word,"LittleFai")==0)
			sscanf(line_txt,"%s %s",&key_word,&KEY_LITTLE_FAI);
		else if(_stricmp(key_word,"EditModel")==0)
			sscanf(line_txt,"%s %d",&key_word,&m_ciEditModel);
		else if(_stricmp(key_word,"AutoSelectFontLib")==0)
			sscanf(line_txt,"%s %d",&key_word,&m_bAutoSelectFontLib);
		else if(_stricmp(key_word,"FontsLibSerial")==0)
			sscanf(line_txt,"%s %d",&key_word,&m_iFontsLibSerial);
		else if(_stricmp(key_word,"DisplayPromptMsg")==0)
			sscanf(line_txt,"%s %d",&key_word,&m_bDisplayPromptMsg);
		else if(_stricmp(key_word,"InitPDFZoomScale")==0)
			sscanf(line_txt,"%s %lf",&key_word,&m_fInitPDFZoomScale);
		else if(_stricmp(key_word,"PdfLineMode")==0)
			sscanf(line_txt,"%s %d",&key_word,&m_iPdfLineMode);
		else if(_stricmp(key_word,"MinPdfLineWeight")==0)
			sscanf(line_txt,"%s %lf",&key_word,&m_fMinPdfLineWeight);
		else if(_stricmp(key_word,"MinPdfLineFlatness")==0)
			sscanf(line_txt,"%s %lf",&key_word,&m_fMinPdfLineFlatness);
		else if(_stricmp(key_word,"MaxPdfLineWeight")==0)
			sscanf(line_txt,"%s %lf",&key_word,&m_fMaxPdfLineWeight);
		else if(_stricmp(key_word,"MaxPdfLineFlatness")==0)
			sscanf(line_txt,"%s %lf",&key_word,&m_fMaxPdfLineFlatness);
		else if(_stricmp(key_word,"ListenScanner")==0)
			sscanf(line_txt,"%s %d",&key_word,&m_bListenScanner);
		else if(_stricmp(key_word,"AutoSaveType")==0)
			sscanf(line_txt,"%s %d",&key_word,&m_iAutoSaveType);
		else if(_stricmp(key_word,"AutoSaveTime")==0)
			sscanf(line_txt,"%s %d",&key_word,&m_nAutoSaveTime);
		else if(_stricmp(key_word,"RecogPieceWeight")==0)
			sscanf(line_txt,"%s %d",&key_word,&m_bRecogPieceWeight);
		else if(_stricmp(key_word,"RecogSumWeight")==0)
			sscanf(line_txt,"%s %d",&key_word,&m_bRecogSumWeight);
		else if (_stricmp(key_word, "WeightEPS") == 0)
		{
			sscanf(line_txt, "%s %lf", &key_word, &m_fWeightEPS);
			AfxGetApp()->WriteProfileString(_T("Settings"), _T("WeightEPS"), CXhChar16(m_fWeightEPS));
		}
	}
	fclose(fp);
}

void CConfig::InitKeyText()
{
	hashTextByKey.Empty();
	hashTextByKey.SetValue(VK_UP,CXhChar100("UP"));
	hashTextByKey.SetValue(VK_DOWN,CXhChar100("DOWN"));
	hashTextByKey.SetValue(VK_LEFT,CXhChar100("LEFT"));
	hashTextByKey.SetValue(VK_RIGHT,CXhChar100("RIGHT"));
	hashTextByKey.SetValue(VK_SPACE,CXhChar100("SPACE"));
	hashTextByKey.SetValue(VK_PRIOR,CXhChar100("PRIOR"));
	hashTextByKey.SetValue(VK_NEXT,CXhChar100("NEXT"));
	hashTextByKey.SetValue(VK_END,CXhChar100("END"));
	hashTextByKey.SetValue(VK_HOME,CXhChar100("HOME"));
	for(int i=0x41;i<=0x5A;i++)
		hashTextByKey.SetValue(i,CXhChar100("%C",i));
	for(int i=0x60;i<=0x69;i++)
		hashTextByKey.SetValue(i,CXhChar100("Num%d",i-0x60+1));
	hashTextByKey.SetValue(VK_MULTIPLY,CXhChar100("*"));
	hashTextByKey.SetValue(VK_ADD,CXhChar100("+"));
	hashTextByKey.SetValue(VK_SEPARATOR,CXhChar100("\\"));
	hashTextByKey.SetValue(VK_SUBTRACT,CXhChar100("-"));
	hashTextByKey.SetValue(VK_DECIMAL,CXhChar100("."));
	hashTextByKey.SetValue(VK_DIVIDE,CXhChar100("/"));
	for(int i=0x70;i<=0x67;i++)
		hashTextByKey.SetValue(i,CXhChar100("F%d",i-0x70+1));
	hashTextByKey.SetValue(VK_SPACE,CXhChar100("SPACE"));
	hashTextByKey.SetValue(VK_EXECUTE,CXhChar100("Enter"));
	hashTextByKey.SetValue(VK_OEM_1,CXhChar100(";"));
	hashTextByKey.SetValue(VK_OEM_PLUS,CXhChar100("+"));
	hashTextByKey.SetValue(VK_OEM_COMMA,CXhChar100(","));
	hashTextByKey.SetValue(VK_OEM_MINUS,CXhChar100("-"));
	hashTextByKey.SetValue(VK_OEM_PERIOD,CXhChar100("."));
	hashTextByKey.SetValue(VK_OEM_2,CXhChar100("/"));
	hashTextByKey.SetValue(VK_OEM_3,CXhChar100("`"));
	hashTextByKey.SetValue(VK_OEM_4,CXhChar100("["));	//  '[{' for US
	hashTextByKey.SetValue(VK_OEM_5,CXhChar100("\\"));	//  '\|' for US
	hashTextByKey.SetValue(VK_OEM_6,CXhChar100("]"));	//  ']}' for US
	hashTextByKey.SetValue(VK_OEM_7,CXhChar100("'"));	//  ''"' for US
	hashTextByKey.SetValue(VK_OEM_7,CXhChar100("'"));	//  ''"' for US
	//
	hashAngleSpecByKey.Empty();
	for(int i=0;i<jgguige_N;i++)
	{
		int wide=(int)jgguige_table[i].wing_wide,thick=(int)jgguige_table[i].wing_thick;
		hashAngleSpecByKey.SetValue(CXhChar16("%d%d",wide,thick),CXhChar16("L%dX%d",wide,thick));
	}
}

int CConfig::GetCfgVKCode(int wKeyCode,HWND hWnd)
{
	CXhChar100 sKeyText;
	if(wKeyCode==VK_PROCESSKEY)
		wKeyCode=ImmGetVirtualKey(hWnd);
	CXhChar100 *pText=hashTextByKey.GetValue(wKeyCode);
	if(pText!=NULL)
	{
		sKeyText.Copy(*pText);
		if(stricmp(KEY_Q235,sKeyText)==0)
			return CFG_VK_Q235;
		else if(stricmp(KEY_Q345,sKeyText)==0)
			return CFG_VK_Q345;
		else if(stricmp(KEY_Q390,sKeyText)==0)
			return CFG_VK_Q390;
		else if(stricmp(KEY_Q420,sKeyText)==0)
			return CFG_VK_Q420;
		else if(stricmp(KEY_UP,sKeyText)==0)
			return CFG_VK_UP;
		else if(stricmp(KEY_DOWN,sKeyText)==0)
			return CFG_VK_DOWN;
		else if(stricmp(KEY_LEFT,sKeyText)==0)
			return CFG_VK_LEFT;
		else if(stricmp(KEY_RIGHT,sKeyText)==0)
			return CFG_VK_RIGHT;
		else if(stricmp(KEY_REPEAT,sKeyText)==0)
			return CFG_VK_REPEAT;
		else
			return wKeyCode;
	}
	else
		return wKeyCode;
}

int CConfig::IsInputKey(int wKeyCode)
{
	if( wKeyCode==VK_SHIFT||
		wKeyCode==VK_CONTROL||
		wKeyCode==VK_MENU||
		wKeyCode==VK_PAUSE||
		wKeyCode==VK_CAPITAL)
		return FALSE;
	else
		return TRUE;
}

CXhChar16 CConfig::GetAngleSpecByKey(const char* key)
{
	CXhChar16 *pSpec=hashAngleSpecByKey.GetValue(key);
	if(pSpec!=NULL)
		return *pSpec;
	else 
	{	//解析Key生成角钢规格
		return CXhChar16(key);
	}
}

CXhChar500 CConfig::GetShortcutPromptStr()
{
	CXhChar500 sPrompt;
	sPrompt.Append("材质快捷键\n");
	sPrompt.Append(CXhChar16("Q235: %s\n",(char*)KEY_Q235));
	sPrompt.Append(CXhChar16("Q345: %s\n",(char*)KEY_Q345));
	sPrompt.Append(CXhChar16("Q390: %s\n",(char*)KEY_Q390));
	sPrompt.Append(CXhChar16("Q420: %s\n",(char*)KEY_Q420));
	sPrompt.Append("\n");
	sPrompt.Append("操作快捷键\n");
	sPrompt.Append(CXhChar16("上: %s\n",(char*)KEY_UP));
	sPrompt.Append(CXhChar16("下: %s\n",(char*)KEY_DOWN));
	sPrompt.Append(CXhChar16("左: %s\n",(char*)KEY_LEFT));
	sPrompt.Append(CXhChar16("右: %s\n",(char*)KEY_RIGHT));
	sPrompt.Append(CXhChar16("重复: %s\n",(char*)KEY_REPEAT));
	sPrompt.Append("特殊字符快捷键\n");
	sPrompt.Append(CXhChar16("Φ: %s\n",(char*)KEY_BIG_FAI));
	sPrompt.Append(CXhChar16("φ: %s\n",(char*)KEY_LITTLE_FAI));
	return sPrompt;
}

void CConfig::InitDefaultSetting()
{
	KEY_Q235=CXhChar16("R");
	KEY_Q345=CXhChar16("T");
	KEY_Q390=CXhChar16("Y");
	KEY_Q420=CXhChar16("U");
	KEY_REPEAT=CXhChar16(".");
	KEY_LEFT=CXhChar16("S");
	KEY_RIGHT=CXhChar16("F");
	KEY_UP=CXhChar16("E");
	KEY_DOWN=CXhChar16("D");
	KEY_RETURN=CXhChar16("Enter");
	KEY_HOME=CXhChar16("Home");
	KEY_END=CXhChar16("End");
	KEY_BIG_FAI=CXhChar16("O");
	KEY_LITTLE_FAI=CXhChar16("P");
	MODIFY_COLOR=RGB(200,255,255);
	DRAWING_ERROR_COLOR=RGB(106,106,255);
	warningLevelClrArr[1]=RGB(200,150,0);
	warningLevelClrArr[2]=RGB(245,185,19);//RGB(232,45,45);	//RGB(245,185,19)
	m_ciWaringMatchCoef=70;
	m_ciEditModel=1;
	m_bDisplayPromptMsg=FALSE;
	m_fInitPDFZoomScale=2.083333333333333;
	m_iPdfLineMode=0;
	m_fMinPdfLineWeight=0.3f;
	m_fMinPdfLineFlatness=0.3f;
	m_fMaxPdfLineWeight=1.2f;
	m_fMaxPdfLineFlatness=1.2f;
	m_nAutoSaveTime=120000;	//两分钟
	m_iAutoSaveType=0;
	m_bAutoSelectFontLib=FALSE;
	m_bRecogPieceWeight=FALSE;
	m_bRecogSumWeight=FALSE;
	m_fWeightEPS=0.5;
	AfxGetApp()->WriteProfileString(_T("Settings"), _T("WeightEPS"), CXhChar16(m_fWeightEPS));
}

void CConfig::ReadSysParaFromReg()
{
	m_bImmerseMode=AfxGetApp()->GetProfileInt(_T("Settings"),_T("ImmerseMode"),m_bImmerseMode);
	m_bTypewriterMode=AfxGetApp()->GetProfileInt(_T("Settings"),_T("TypewriterMode"),m_bTypewriterMode);
	m_sOperator.Copy(AfxGetApp()->GetProfileString(_T("Settings"),_T("Operator"),m_sOperator));
	m_sAuditor.Copy(AfxGetApp()->GetProfileString(_T("Settings"),_T("Auditor"),m_sAuditor));
	m_sCritic.Copy(AfxGetApp()->GetProfileString(_T("Settings"),_T("Critic"),m_sCritic));
}
void CConfig::WriteSysParaToReg()
{
	AfxGetApp()->WriteProfileInt(_T("Settings"),_T("ImmerseMode"),m_bImmerseMode);
	AfxGetApp()->WriteProfileInt(_T("Settings"),_T("TypewriterMode"),m_bTypewriterMode);
	AfxGetApp()->WriteProfileString(_T("Settings"),_T("Operator"),m_sOperator);
	AfxGetApp()->WriteProfileString(_T("Settings"),_T("Auditor"),m_sAuditor);
	AfxGetApp()->WriteProfileString(_T("Settings"),_T("Critic"),m_sCritic);
	AfxGetApp()->WriteProfileString(_T("Settings"), _T("WeightEPS"), CXhChar16(m_fWeightEPS));
}