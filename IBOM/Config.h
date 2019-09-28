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
	static const WORD CFG_VK_RETURN		= VK_RETURN;	//ȷ�ϼ�
	static const WORD CFG_VK_SPACE		= VK_SPACE;		//�ո��
	static const WORD CFG_VK_Q235		= 0xFFF0;
	static const WORD CFG_VK_Q345		= 0xFFF1;
	static const WORD CFG_VK_Q390		= 0xFFF2;
	static const WORD CFG_VK_Q420		= 0xFFF3;
	static const WORD CFG_VK_REPEAT		= 0xFFF5;	//�ظ���һ��
	
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
	static CXhChar100 KEY_BIG_FAI;		//�ֹ�ֱ������ݼ�
	static CXhChar100 KEY_LITTLE_FAI;	//�ֹ�ֱ���տ�ݼ�

	static BOOL m_bImmerseMode;
	static BOOL m_bTypewriterMode;
	static COLORREF MODIFY_COLOR;
	static COLORREF DRAWING_ERROR_COLOR;
	static COLORREF warningLevelClrArr[3];
	static BYTE m_ciWaringMatchCoef; 
	static BYTE m_ciEditModel;			//0.���� 1.˫��
	static BOOL m_bDisplayPromptMsg;	//��ʾ��ʾ��Ϣ
	static double m_fInitPDFZoomScale;
	static int m_iPdfLineMode;			//0.����|1.�Ӵ�|2.��ϸ|3.����Ӧ
	static double m_fMinPdfLineWeight;
	static double m_fMinPdfLineFlatness;
	static double m_fMaxPdfLineWeight;
	static double m_fMaxPdfLineFlatness;
	static int m_iFontsLibSerial;
	static BOOL m_bAutoSelectFontLib;
	static BOOL m_bListenScanner;
	static UINT m_nAutoSaveTime;	//��λ��ms
	static UINT m_iAutoSaveType;
	static CXhChar100 m_sOperator;		//����Ա���Ʊ��ˣ�
	static CXhChar100 m_sAuditor;		//�����
	static CXhChar100 m_sCritic;		//������
	static BOOL m_bRecogPieceWeight;	//ʶ����
	static BOOL m_bRecogSumWeight;		//ʶ������
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

