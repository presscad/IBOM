#ifndef __DATA_BASE_H_
#define __DATA_BASE_H_
#include "PartLib.h"
//角钢接头相关参数及函数,暂时放在此处,以后移至PartLib.dll中 wht 11-01-25
/*
typedef struct tagJG_JOINT_PARA
{
	double fBaseJgWidth;		//基准角钢宽度
	double fBaseJgThick;		//基准角钢厚度
	double fInnerJgWidth;		//内包角钢宽度
	double fInnerJgThick;		//内包角钢厚度
	double fOuterPlateWidth;	//外包钢板宽度
	double fOuterPlateThick;	//外包钢板厚度
	double fOuterJgWidth;		//外包角钢宽度
	double fOuterJgThick;		//外包角钢厚度
}JG_JOINT_PARA;
extern int g_nJgJointRecordNum;
extern JG_JOINT_PARA jg_joint_table[];	//角钢接头对应表
extern JG_JOINT_PARA *FindJgJointPara(double main_w,double main_t);
extern BOOL FindInnerAngleGuiGe(double main_w,double main_t,double &inner_w,double &inner_t);
extern BOOL FindOuterAngleGuiGe(double main_w,double main_t,double &outer_w,double &outer_t);
extern BOOL FindOuterPlateGuiGe(double main_w,double main_t,double &plate_w,double &plate_t);
*/
//
extern double Q235A_STABLE_COEF[251];
extern double Q235B_STABLE_COEF[251];
extern double Q345A_STABLE_COEF[251];
extern double Q345B_STABLE_COEF[251];
extern double Q390A_STABLE_COEF[251];
extern double Q390B_STABLE_COEF[251];
extern double Q420A_STABLE_COEF[251];
extern double Q420B_STABLE_COEF[251];
extern double Q460A_STABLE_COEF[251];
extern double Q460B_STABLE_COEF[251];
BOOL JgZhunJuSerialize(CString sFileName, BOOL bWrite,BOOL bThrowError=TRUE);
BOOL JgGuiGeSerialize(CString sFileName, BOOL bWrite,BOOL bPrompt=TRUE);
BOOL TubeGuiGeSerialize(CString sFileName, BOOL bWrite,BOOL bPrompt=TRUE);
BOOL SlotGuiGeSerialize(CString sFileName, BOOL bWrite,BOOL bPrompt=TRUE);
BOOL FlatGuiGeSerialize(CString sFileName, BOOL bWrite,BOOL bPrompt=TRUE);
BOOL JgJointParaSerialize(CString sFileName, BOOL bWrite,BOOL bPrompt=TRUE);
BOOL LsGuiGeSerialize(CLsFamily* pLsFamily,CString sFileName, BOOL bWrite,BOOL bPrompt=TRUE);
BOOL BoltFamilySerialize(CLsFamily* pLsFamily,CString sFileName, BOOL bWrite,BOOL bPrompt=TRUE);
BOOL LsSubPartSerialize(CLsSubPartFamily* pFamily,CString sFileName, BOOL bWrite,BOOL bPrompt=TRUE);
BOOL LsSpaceSerialize(CString sFileName, BOOL bWrite,BOOL bPrompt=TRUE);
BOOL CInsertPlateSerialize(CString sFileName, BOOL bWrite,BOOL bPrompt=TRUE);
BOOL UInsertPlateSerialize(CString sFileName, BOOL bWrite,BOOL bPrompt=TRUE);
BOOL XInsertPlateSerialize(CString sFileName, BOOL bWrite,BOOL bPrompt=TRUE);
BOOL FlPSerialize(CString sFileName, BOOL bWrite,BOOL bPrompt=TRUE);
BOOL FlDSerialize(CString sFileName, BOOL bWrite,BOOL bPrompt=TRUE);
BOOL FlGSerialize(CString sFileName, BOOL bWrite,BOOL bPrompt=TRUE);
BOOL FlRSerialize(CString sFileName, BOOL bWrite,BOOL bPrompt=TRUE);
BOOL FlCoupleLevelSerialize(CString sFileName, BOOL bWrite,BOOL bPrompt=TRUE);
#ifdef __H_SHAPE_STEEL_
void HShapeGuiGeSerialize(CString sFileName,int iPartSubType, BOOL bWrite,BOOL bPrompt=TRUE);
#endif
BOOL AidedPartGuiGeSerialize(CString sFileName, BOOL bWrite,BOOL bPrompt=TRUE);
#endif
