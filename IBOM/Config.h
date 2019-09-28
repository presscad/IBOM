#pragma once
#include "HashTable.h"
#include "XhCharString.h"
#include "DataModel.h"

#pragma comment(lib, "Imm32.lib")  

class CConfig
{
	static CHashList<CXhChar100> hashTextByKey;
	static CHashStrList<CXhChar16> hashAngleSpecByKey;
public:
	CConfig(void);
	~CConfig(void);
	static const WORD CFG_VK_UP			= VK_UP;
	static const WORD CFG_VK_DOWN		= VK_DOWN;
	static const WORD CFG_VK_LEFT		= VK_LEFT;
	static const WORD CFG_VK_RIGHT		= VK_RIGHT;
	static const WORD CFG_VK_HOME		= VK_HOME;		//HOME
	static const WORD CFG_VK_END		= VK_END;		//END
	static const WORD CFG_VK_RETURN		= VK_RETURN;	//确认键
	static const WORD CFG_VK_SPACE		= VK_SPACE;		//空格键
	static const WORD CFG_VK_Q235		= 0xFFF0;
	static const WORD CFG_VK_Q345		= 0xFFF1;
	static const WORD CFG_VK_Q390		= 0xFFF2;
	static const WORD CFG_VK_Q420		= 0xFFF3;
	static const WORD CFG_VK_REPEAT		= 0xFFF5;	//重复上一次
	
public:
	static CXhChar100 KEY_Q235;
	static CXhChar100 KEY_Q345;
	static CXhChar100 KEY_Q390;
	static CXhChar100 KEY_Q420;
	static CXhChar100 KEY_LEFT;
	static CXhChar100 KEY_RIGHT;
	static CXhChar100 KEY_UP;
	static CXhChar100 KEY_DOWN;
	static CXhChar100 KEY_REPEAT;
	static CXhChar100 KEY_RETURN;
	static CXhChar100 KEY_HOME;
	static CXhChar100 KEY_END;
	static CXhChar100 KEY_BIG_FAI;		//钢管直径Φ快捷键
	static CXhChar100 KEY_LITTLE_FAI;	//钢管直径φ快捷键

	static BOOL m_bImmerseMode;
	static BOOL m_bTypewriterMode;
	static COLORREF MODIFY_COLOR;
	static COLORREF DRAWING_ERROR_COLOR;
	static COLORREF warningLevelClrArr[3];
	static BYTE m_ciWaringMatchCoef; 
	static BYTE m_ciEditModel;			//0.单击 1.双击
	static BOOL m_bDisplayPromptMsg;	//显示提示信息
	static double m_fInitPDFZoomScale;
	static int m_iPdfLineMode;			//0.正常|1.加粗|2.描细|3.自适应
	static double m_fMinPdfLineWeight;
	static double m_fMinPdfLineFlatness;
	static double m_fMaxPdfLineWeight;
	static double m_fMaxPdfLineFlatness;
	static int m_iFontsLibSerial;
	static BOOL m_bAutoSelectFontLib;
	static BOOL m_bListenScanner;
	static UINT m_nAutoSaveTime;	//单位：ms
	static UINT m_iAutoSaveType;
	static CXhChar100 m_sOperator;		//操作员（制表人）
	static CXhChar100 m_sAuditor;		//审核人
	static CXhChar100 m_sCritic;		//评审人
	static BOOL m_bRecogPieceWeight;	//识别单重
	static BOOL m_bRecogSumWeight;		//识别总重
	static double m_fWeightEPS;

	static void SaveToFile(const char* file_path);
	static void LoadFromFile(const char* file_path);
	static void InitKeyText();
	static int GetCfgVKCode(int wKeyCode,HWND hWnd);
	static int IsInputKey(int wKeyCode);
	static CXhChar16 GetAngleSpecByKey(const char* key);
	static void ReadSysParaFromReg();
	static void WriteSysParaToReg();
	static CXhChar500 GetShortcutPromptStr();
	static void InitDefaultSetting();
};

