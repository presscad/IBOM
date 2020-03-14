#ifndef RECOG_PART_BOM_H
#define RECOG_PART_BOM_H

#include "f_ent.h"
#include "f_alg_fun.h"
#include "f_ent_list.h"
#include "ArrayList.h"
#include "XhCharString.h"
#include "list.h"
#include "HashTable.h"
#include "material.h"
//��ȡ������ϸ��
struct BOM_CELL{
	int iRow,iCol;	//0Ϊ����
	RECT rc;
	GEPOINT dimpos;	//�ı���עλ��
	CXhChar50 contents;
	BOM_CELL(){iRow=iCol=-1;}
	BOM_CELL(const BOM_CELL& srcCell);
	BOM_CELL& operator=(const BOM_CELL& srcCell);
	void Clone(const BOM_CELL& srcCell);
};

struct BLOCK_ROW {
	int iRow;
	GEPOINT blockPos;
	CXhSimpleList<BOM_CELL> listCells;
	BLOCK_ROW() { iRow = -1; }
	BLOCK_ROW(const BLOCK_ROW& srcRow);
	BLOCK_ROW& operator=(const BLOCK_ROW& srcRow);
	void Clone(const BLOCK_ROW& srcRow);
};

class CPartBomTable{
private:
	GEPOINT leftTop,rightBottom;
	GEPOINT rgn_vert[4];
	CXhSimpleList<BOM_CELL> listCells;
	ARRAY_LIST<f3dLine> m_xRowlines;
	ARRAY_LIST<f3dLine> m_xCollines;
	ATOM_LIST<CManuPart> m_xBomPartList;
	ATOM_LIST<CStatBolt> m_xBomBoltList;
	ATOM_LIST<BLOCK_ROW> m_xBlockRowList;
	BOOL InternalSelectEntFromUI();
	int GetAllRow(ATOM_LIST<BLOCK_ROW>& rowList);
public:
	CHashSet<AcDbObjectId> m_xSelEntIdSet;
public:
	CPartBomTable(GEPOINT* vertArr=NULL);
	~CPartBomTable();
	//
	const static BYTE RECOG_PART = 0;
	const static BYTE RECOG_BOLT = 1;
	BOOL SelectEntFromUI(BYTE ciPartType);
	BOOL RecogizeBomTable();
	void RetrieveParts();
	void RetrieveBolts();
	void ExportBomToTxt();
	void ExportBomToBomdFile(const char* work_path,bool bPrompt);
	BOM_CELL* EnumFirstBom(){return listCells.EnumObjectFirst();}
	BOM_CELL* EnumNextBom(){return listCells.EnumObjectNext();}
	CManuPart* EnumFirstPart(){return m_xBomPartList.GetFirst();}
	CManuPart* EnumNextPart(){return m_xBomPartList.GetNext();}
	CStatBolt* EnumFirstBolt(){return m_xBomBoltList.GetFirst();}
	CStatBolt* EnumNextBolt(){return m_xBomBoltList.GetNext();}

public:
	static CHashStrList<CXhChar500> m_hashSegStrByFilePath;	//ͬһ��ͼֽʹ��һ���κ�
};
#endif