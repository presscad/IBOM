#pragma once
#include "PartLib.h"
#include "HashTable.h"
#include "Recog.h"

typedef struct tagAIDED_PART
{
	char   guige[16];
	double weight;
	short siUnitType;
}AIDED_PART;

extern WORD aided_part_N;
extern AIDED_PART aided_part_table[];	//¸¨Öú¹¹¼þ
extern double CalPartWeight(IRecoginizer::BOMPART *pPart);
extern void CalAndSyncTheoryWeightToWeight(IRecoginizer::BOMPART *pPart,BOOL bForceCalWeight);

class CPartLibrary
{
private:
	static CHashStrList<JG_PARA_TYPE*> m_hashJgParaPtrBySpec;
	static CHashStrList<TUBE_PARA_TYPE*> m_hashTubeParaPtrBySpec;
	static void InitJgParaTbl();
	static void InitTubeParaTbl();
public:
	static JG_PARA_TYPE *FindJgParaByUnitWeight(double unit_weight);
	static TUBE_PARA_TYPE *FindTubeParaByUnitWeight(double unit_weight);
	static JG_PARA_TYPE *FindJgParaBySpec(const char* spec);
	static TUBE_PARA_TYPE *FindTubeParaBySpec(const char* spec);
	static AIDED_PART* FindAidedPart(const char* spec);
	static void LoadPartLibrary(const char* app_path);
};