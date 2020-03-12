#include "StdAfx.h"
#if defined(__TMA_FILE_)||defined(__LMA_FILE_)||defined(__TDA_FILE_)||defined(__LDS_FILE_)
#include "StdArx.h"
#include "TowerBom.h"
#include "RxTools.h"
#endif
#include "dbents.h"
#include "SortFunc.h"
#include "LicFuncDef.h"
#include "XhLicAgent.h"
#include "MenuFunc.h"
#include "RecogPartBom.h"
#include "XhCharString.h"
#if defined(__BOM_FILE_)
#include "direct.h"

static double TestDrawTextLength(const char* dimtext,double height,AcDbObjectId textStyleId)
{
	AcDbMText mtxt;
#ifdef _ARX_2007
	mtxt.setContents((ACHAR*)_bstr_t(dimtext));				//设置文字标注内容
#else
	mtxt.setContents(dimtext);				//设置文字标注内容
#endif
	mtxt.setWidth(strlen(dimtext)*height);					//每行文字的最大宽度
	mtxt.setTextHeight(height);
	mtxt.setTextStyle(textStyleId);		//文字插入点
	return mtxt.actualWidth();
}
static char QuerySteelBriefMatMark(const char* material_str)
{
	if(material_str==NULL)
		return 0;
	else if(strstr(material_str,"Q235")!=NULL)
		return 'S';
	else if(strstr(material_str,"Q345")!=NULL)
		return 'H';
	else if(strstr(material_str,"Q390")!=NULL)
		return 'G';
	else if(strstr(material_str,"Q420")!=NULL)
		return 'P';
	else if(strstr(material_str,"Q460")!=NULL)
		return 'T';
	else
		return 0;
}
static int GetParttype(char* spec)
{
	CString str(spec);
	int parttype=0;
	if(str.Find("Φ")>=0||str.Find("φ")>=0)	//不能使用spec[0]=='Φ'
	{
		if(strchr(spec,'/'))
			parttype = 3;	//钢板(法兰)
		else
			parttype = 4;	//钢管
	}
	else if(spec[0] == 'L')
		parttype = 1;	//角钢
	else if(spec[0] == '-')
		parttype = 3;	//钢板
	else if(spec[0]=='[')
		parttype = 6;	//槽钢[
	else
		parttype = 0;	//其余构件
	return parttype;
}
#endif

#define      LOCAL_EPS             0.1
//////////////////////////////////////////////////////////////////////////
//静态函数实现
static int _LocalCompareVertLines(const f3dLine& line1,const f3dLine& line2)
{
	return ftoi(line1.startPt.x-line2.startPt.x);
}
static int _LocalCompareHoriLines(const f3dLine& line1,const f3dLine& line2)
{
	return ftoi(line2.startPt.y-line1.startPt.y);
}
static bool _LocalIsInteralLineInRect(GEPOINT lineStart,GEPOINT lineEnd,
	GEPOINT rgnTopLeft,GEPOINT rgnBtmRight,double minInnerLength=2)
{
	bool horiLine=false;
	if(fabs(lineStart.x-lineEnd.x)<LOCAL_EPS)
		horiLine=false;
	else if(fabs(lineStart.y-lineEnd.y)<LOCAL_EPS)
		horiLine=true;
	else	//斜线
		return false;
	if(horiLine)
	{
		double leftX=min(lineStart.x,lineEnd.x),rightX=max(lineStart.x,lineEnd.x);
		if(rightX<rgnTopLeft.x-minInnerLength||leftX>rgnBtmRight.x+minInnerLength)
			return false;
		rightX=min(rightX,rgnBtmRight.x);
		leftX=max(leftX,rgnTopLeft.x);
		return rightX-leftX>minInnerLength;
	}
	else
	{
		double topY=max(lineStart.y,lineEnd.y),btmY=min(lineStart.y,lineEnd.y);
		if(btmY>rgnTopLeft.y+minInnerLength||topY<rgnBtmRight.y-minInnerLength)
			return false;
		topY=min(topY,rgnTopLeft.y);
		btmY=max(btmY,rgnBtmRight.y);
		return topY-btmY>minInnerLength;
	}
}
//////////////////////////////////////////////////////////////////////////
//CPartBomTable
CPartBomTable::CPartBomTable(GEPOINT* vertArr)
{
	if(vertArr)
	{
		for(int i=0;i<4;i++)
			rgn_vert[i]=vertArr[i];
	}
}
CPartBomTable::~CPartBomTable()
{

}
void CPartBomTable::ExportBomToTxt()
{
	//CFileDialog savedlg(FALSE,"txt","mxb");
	CFileDialog savedlg(TRUE,"txt","Tower",
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		"统材信息(*.txt)|*.txt|所有文件(*.*)|*.*||");
	if(savedlg.DoModal()!=IDOK)
		return;
	FILE* fp=fopen(savedlg.GetPathName(),"wt");
	for(int row=0;row<m_xRowlines.GetSize()-1;row++)
	{
		for(int col=0;col<m_xCollines.GetSize()-1;col++)
		{
			CXhChar100 celltext;
			for(BOM_CELL* pCell=listCells.EnumObjectFirst();pCell;pCell=listCells.EnumObjectNext())
			{
				if(pCell->contents.GetLength()==0||pCell->iRow!=row||pCell->iCol!=col)
					continue;
				celltext.Append(pCell->contents);
			}
			celltext.ResizeLength(16,' ',true);
			celltext.Replace("%%c","φ");
			celltext.Replace("%%C","Φ");
			fprintf(fp,"%s",(char*)celltext);
		}
		fprintf(fp,"\n");
	}
	fclose(fp);
	WinExec(CXhChar200("notepad.exe %s",savedlg.GetPathName()),SW_SHOW);
}
//生成GUID字符串
static CXhChar500 CreateGuidString()
{
	GUID guid; 
	CXhChar500 szGUID; 
	if(S_OK==::CoCreateGuid(&guid)) 
	{
		szGUID.Printf("{%08X-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X} " 
			,guid.Data1,guid.Data2,guid.Data3,guid.Data4[0],guid.Data4[1] ,guid.Data4[2],
			guid.Data4[3],guid.Data4[4],guid.Data4[5],guid.Data4[6],guid.Data4[7]); 
	}
	return szGUID;
}
static BOOL GetCurFileName(CXhChar500 &sFilePath,CXhChar500 &sName)
{	//此处不定义，仅声明在VC7下编译时会产生编译错误　wjh-2019.1.7
	AcApDocument* pDoc = acDocManager->curDocument();
	if(pDoc==NULL)
		return FALSE;
	CString file_path;
	file_path=pDoc->fileName();
	sFilePath.Copy(file_path);
	if(file_path.CompareNoCase("Drawing1.dwg")==0)	//默认的DWG文件
		return FALSE;
	_splitpath(file_path,NULL,NULL,sName,NULL);
	return TRUE;
}
void CPartBomTable::ExportBomToBomdFile(const char* work_path,bool bPrompt)
{
	if(work_path==NULL)
		return;
	ARRAY_LIST<CXhChar200> partList;
	SEGI segI;
	int iFirstLabelCol=0;
	CXhChar16 sFirstLabel,sTailLabel;
	for(int row=0;row<m_xRowlines.GetSize()-1;row++)
	{
		CXhChar200 sPartText;
		for(int col=0;col<m_xCollines.GetSize()-1;col++)
		{
			CXhChar100 sCellText;
			for(BOM_CELL *pCell=listCells.EnumObjectFirst();pCell;pCell=listCells.EnumObjectNext())
			{
				if(pCell->contents.GetLength()==0||pCell->iRow!=row||pCell->iCol!=col)
					continue;
				sCellText.Append(pCell->contents);
				break;
			}
 			sCellText.Replace("%%c","φ");
			sCellText.Replace("%%C","Φ");
			if(col==0)
			{
				if(row==0)
					sFirstLabel.Copy(sCellText);
				else if(row==1&&sFirstLabel.GetLength()<=0)
				{
					sFirstLabel.Copy(sCellText);
					iFirstLabelCol=row;
				}
				else if(row==m_xRowlines.GetSize()-2)
					sTailLabel.Copy(sCellText);
			}
			if(col>0&&row==m_xRowlines.GetSize()-2&&sTailLabel.GetLength()<=0)
			{	//件号为空，取规格列
				if(sCellText.GetLength()>0)
					sTailLabel.Copy(sCellText);
			}
			//
			//sCellText.ResizeLength(16,' ',true);
			if(sCellText.GetLength()==0)
				sCellText.Copy("NULL");	//NULL代表字段为空，不添加占位符无法正确划分字段 wht 18-12-20
			sPartText.Append(sCellText);
			sPartText.Append("\t");	//使用空格分隔内容时，无法处理空列问题，改用\t分割内容 wht 18-12-20
		}
		partList.append(sPartText);
	}
	CXhChar500 sFileName,sFilePath;
	GetCurFileName(sFilePath,sFileName);
	CXhChar500 *psSeg=CPartBomTable::m_hashSegStrByFilePath.GetValue(sFilePath);
	if(psSeg==NULL)
	{
		SEGI segI;
		if(iFirstLabelCol==0&&sFirstLabel.GetLength()>0&&ParsePartNo(sFirstLabel,&segI,NULL))
			psSeg=CPartBomTable::m_hashSegStrByFilePath.SetValue(sFilePath,CXhChar500(segI.ToString()));
		if(psSeg==NULL)
		{	//以文件名为段号
			if(sFileName.GetLength()>0)
				psSeg=CPartBomTable::m_hashSegStrByFilePath.SetValue(sFilePath,sFileName);
			else
			{	//生成GUID作为唯一标识
				sFileName=CreateGuidString();
				psSeg=CPartBomTable::m_hashSegStrByFilePath.SetValue(sFilePath,sFileName);
			}
		}
	}
	CXhChar500 sDwgFilePath("%s/%s~%s.bomd",work_path,(char*)sFirstLabel,(char*)sTailLabel);
	if(psSeg!=NULL)
		sDwgFilePath.Printf("%s/%s#%s~%s.bomd",work_path,(char*)*psSeg,(char*)sFirstLabel,(char*)sTailLabel);
	FILE* fp=fopen(sDwgFilePath,"wt");
	for(CXhChar200 *pLineText=partList.GetFirst();pLineText;pLineText=partList.GetNext())
	{
		fprintf(fp,"%s",(char*)*pLineText);
		fprintf(fp,"\n");
	}
	fclose(fp);
	if(bPrompt)
		MessageBox(NULL,CXhChar500("成功提出%s~%s共%d个构件！",(char*)sFirstLabel,(char*)sTailLabel,partList.GetSize()),"提示",MB_OK);
}
extern int GetParttype(char* spec);
CHashStrList<CXhChar500> CPartBomTable::m_hashSegStrByFilePath;
void CPartBomTable::RetrieveParts()
{
	for(int row=0;row<m_xRowlines.GetSize()-1;row++)
	{
		CManuPart* pManuPart=m_xBomPartList.append();
		for(int col=0;col<m_xCollines.GetSize()-1;col++)
		{
			BOM_CELL* pCell=NULL;
			for(pCell=listCells.EnumObjectFirst();pCell;pCell=listCells.EnumObjectNext())
			{
				if(pCell->contents.GetLength()>0&&pCell->iRow==row&&pCell->iCol==col)
					break;
			}
			if(pCell==NULL)
				continue;
			if(col==0)	//件号
			{
				strcpy(pManuPart->sPartNo,pCell->contents);
				ParsePartNo(pManuPart->sPartNo,&pManuPart->nSeg,NULL);
			}
			if(col==1)	//规格
			{
				CXhChar100 sText(pCell->contents),ss;
				sText.Replace("%%c","φ");
				sText.Replace("%%C","Φ");
				if(strstr(sText,"-"))
				{
					sText.Replace("-"," ");
					ss.Copy(sText);
					if(strstr(sText,"Q"))
						sscanf(sText,"%s%s",pManuPart->sMaterial,(char*)ss);
					else
						strcpy(pManuPart->sMaterial,"Q235");
					ss.Replace("X"," ");
					int thick=0,width=0;
					sscanf(ss,"%d%d",&thick,&width);
					sprintf(pManuPart->sSpec,"-%d",thick);
				}
				else if(strstr(sText,"L"))
				{
					if(strstr(sText,"Q"))
					{
						sText.Replace("L"," ");
						sscanf(sText,"%s%s",pManuPart->sMaterial,(char*)ss);
						sprintf(pManuPart->sSpec,"L%s",(char*)ss);
					}
					else
					{
						strcpy(pManuPart->sSpec,sText);
						strcpy(pManuPart->sMaterial,"Q235");
					}
				}
				else if(strstr(sText,"Φ"))
				{
					if(strstr(sText,"Q"))
					{
						sText.Replace("Φ"," ");
						sscanf(sText,"%s%s",pManuPart->sMaterial,(char*)ss);
						sprintf(pManuPart->sSpec,"L%s",(char*)ss);
					}
					else
					{
						strcpy(pManuPart->sSpec,sText);
						strcpy(pManuPart->sMaterial,"Q235");
					}
				}
				pManuPart->cMaterial=QuerySteelBriefMatMark(pManuPart->sMaterial);
				pManuPart->part_type=GetParttype(pManuPart->sSpec);
			}
			if(col==2)	//长度
				pManuPart->length=atof(pCell->contents);
			if(col==3)	//数量
				pManuPart->nPartNum=atoi(pCell->contents);
			if(col==4)	//单重
				pManuPart->singWeight=atof(pCell->contents);
			if(col==5)	//总重
				pManuPart->sumWeight=atof(pCell->contents);
		}
	}
	//删除无意义的BOM
	for(CManuPart* pManuPart=m_xBomPartList.GetFirst();pManuPart;pManuPart=m_xBomPartList.GetNext())
	{
		if(strlen(pManuPart->sPartNo)<=0)
			m_xBomPartList.DeleteCursor();
	}
	m_xBomPartList.Clean();
}
void CPartBomTable::RetrieveBolts()
{
	/*CManuPart *pFirstPart=m_xBomPartList.GetFirst();
	SEGI segI=pFirstPart?pFirstPart->nSeg:SEGI((long)0);
	for(int row=0;row<m_xRowlines.GetSize()-1;row++)
	{
		CXhChar16 grade;
		CStatBolt* pStatBolt=m_xBomBoltList.append();
		pStatBolt->nSeg=segI;
		for(int col=0;col<m_xCollines.GetSize()-1;col++)
		{
			BOM_CELL* pCell=NULL;
			for(pCell=listCells.EnumObjectFirst();pCell;pCell=listCells.EnumObjectNext())
			{
				if(pCell->contents.GetLength()>0&&pCell->iRow==row&&pCell->iCol==col)
					break;
			}
			if(pCell==NULL)
				continue;
			if(col==0)	//名称
			{
				pStatBolt->type=0;
				if(strstr(pCell->contents,"脚")!=NULL||strstr(pCell->contents,"钉")!=NULL)
					pStatBolt->type=1;
				else if(strstr(pCell->contents,"垫")!=NULL||strstr(pCell->contents,"圈")!=NULL)
					pStatBolt->type=2;
			}
			if(col==1)	//级别
				grade.Copy(pCell->contents);
			if(col==2)	//规格
				strcpy(pStatBolt->sSpec,pCell->contents);
			//if(col==3)//图符
			if(col==4)	//数量
				pStatBolt->nCount=atoi(pCell->contents);
			if(col==5)	//重量
				pStatBolt->weight=atof(pCell->contents);
			if(col==6)	//备注
			{
				strcpy(pStatBolt->sNote,pCell->contents);
				if(strstr(pStatBolt->sNote,"双帽")!=NULL)
					pStatBolt->type=3;	//双帽螺栓
			}
		}
		if(pStatBolt->type==2)
			grade.Copy("Q235");
		else 
			grade.Copy("6.8");
		if(strlen(pStatBolt->sSpec)>0&&grade.GetLength()>0&&pStatBolt->type!=2)
		{
			grade.Replace("级","");
			CXhChar100 sSpec(grade);
			sSpec.Append(pStatBolt->sSpec);
			strcpy(pStatBolt->sSpec,sSpec);
		}
	}
	//删除无意义的BOM
	for(CStatBolt* pStatBolt=m_xBomBoltList.GetFirst();pStatBolt;pStatBolt=m_xBomBoltList.GetNext())
	{
		if(strlen(pStatBolt->sSpec)<=0)
			m_xBomBoltList.DeleteCursor();
	}
	m_xBomBoltList.Clean();*/
}
class CLocalAcDbObjLife
{
	AcDbObject *pCadDbObj;
public:
	CLocalAcDbObjLife(AcDbObject* pDbObj){pCadDbObj=pDbObj;}
	~CLocalAcDbObjLife(){
		if(pCadDbObj)
			pCadDbObj->close();
	}
};
BOOL CPartBomTable::RecogizeBomTable()
{
	BOM_CELL* pCell;
	f3dLine line;
	AcDbEntity *pEnt=NULL;
	ARRAY_LIST<f3dLine> arrHoriLines(0,50),arrVertLines(0,8);
	int i=0;
	while(i<2)
	{
		arrHoriLines.Empty();
		arrVertLines.Empty();
		i++;
		if(i==2)	//打碎块之后重新从界面选择关联实体 wht 18-12-24
			InternalSelectEntFromUI();
		AcDbBlockReference *pLineBlockRef=NULL;
		for(AcDbObjectId objId=m_xSelEntIdSet.GetFirst();objId;objId=m_xSelEntIdSet.GetNext())
		{
			acdbOpenAcDbEntity(pEnt,objId,AcDb::kForRead);
			if(pEnt)
				pEnt->close();
			else //if(pEnt==null)
				continue;
			AcDbLine*  pLine=NULL;
			AcDbText*  pText=NULL;
			AcDbMText* pMText=NULL;
			AcDbPolyline *pPolyline=NULL;
			AcGePoint3d acad_start,acad_end;
			if(pEnt->isKindOf(AcDbLine::desc())||pEnt->isKindOf(AcDbPolyline::desc()))
			{
				if(pEnt->isKindOf(AcDbLine::desc()))
				{
					pLine=(AcDbLine*)pEnt;
					acad_start=pLine->startPoint();
					acad_end=pLine->endPoint();
				}
				else if(pEnt->isKindOf(AcDbPolyline::desc()))
				{
					pPolyline=(AcDbPolyline*)pEnt;
					int nNumVertexs=pPolyline->numVerts();
					if(nNumVertexs==2)
					{
						pPolyline->getPointAt(0,acad_start);
						pPolyline->getPointAt(1,acad_end);
					}
					else
					{
						acad_start.set(0,0,0);
						acad_end.set(0,0,0);
					}
				}
				Cpy_Pnt(line.startPt,acad_start);
				Cpy_Pnt(line.endPt,acad_end);
				if(DISTANCE(line.startPt,line.endPt)>0)
				{
					if( fabs(line.startPt.x-line.endPt.x)<LOCAL_EPS &&
						_LocalIsInteralLineInRect(line.startPt,line.endPt,rgn_vert[0],rgn_vert[2]))
						arrVertLines.append(line);	//竖线
					else if(fabs(line.startPt.y-line.endPt.y)<LOCAL_EPS &&
						_LocalIsInteralLineInRect(line.startPt,line.endPt,rgn_vert[0],rgn_vert[2]))
						arrHoriLines.append(line);	//横线
				}
			}
			else if(pEnt->isKindOf(AcDbText::desc()))
			{
				AcDbText* pText=(AcDbText*)pEnt;
				pCell=listCells.AttachObject();
		#ifdef _ARX_2007
				pCell->contents.Copy(_bstr_t(pText->textString()));
		#else
				pCell->contents.Copy(pText->textString());
		#endif
				AcGePoint3d dimpos=pText->position();//alignmentPoint();
				double fTestH=pText->height();
				double fWidth=TestDrawTextLength(pCell->contents,fTestH,pText->textStyle());
				AcDb::TextHorzMode  horzMode=pText->horizontalMode();
				AcDb::TextVertMode  vertMode=pText->verticalMode();
				//获取AcDbText插入点 wht 18-12-20
				//If vertical mode is AcDb::kTextBase and horizontal mode is either AcDb::kTextLeft, AcDb::kTextAlign, or AcDb::kTextFit,
				//then the position point is the insertion point for the text object and, for AcDb::kTextLeft, 
				//the alignment point (DXF group code 11) is automatically calculated based on the other parameters in the text object.
				if(vertMode==AcDb::kTextBase)
				{
					if (horzMode==AcDb::kTextLeft||horzMode==AcDb::kTextAlign||horzMode==AcDb::kTextFit)
						dimpos = pText->position();
					else
						dimpos = pText->alignmentPoint();
				}
				else
					dimpos = pText->alignmentPoint();
				if(horzMode==AcDb::kTextLeft)
					dimpos.x+=fWidth*0.5;
				else if(horzMode==AcDb::kTextRight)
					dimpos.x-=fWidth*0.5;
				//
				if(vertMode==AcDb::kTextTop)
					dimpos.y-=fTestH*0.5;
				else if(vertMode==AcDb::kTextBottom)
					dimpos.y+=fTestH*0.5;
				Cpy_Pnt(pCell->dimpos,dimpos);
			}
			else if(pEnt->isKindOf(AcDbMText::desc()))
			{
				AcDbMText* pText=(AcDbMText*)pEnt;
				pCell=listCells.AttachObject();
		#ifdef _ARX_2007
				pCell->contents.Copy(_bstr_t(pText->contents()));
		#else
				pCell->contents.Copy(pText->contents());
		#endif
				AcGePoint3d dimpos=pText->location();
				double fTestH=pText->actualHeight();
				double fWidth=pText->actualWidth();
				AcDbMText::AttachmentPoint align_type=pText->attachment();
				if(align_type==AcDbMText::kTopLeft||align_type==AcDbMText::kMiddleLeft||align_type==AcDbMText::kBottomLeft)
					dimpos.x+=fWidth*0.5;
				else if(align_type==AcDbMText::kTopRight||align_type==AcDbMText::kMiddleRight||align_type==AcDbMText::kBottomRight)
					dimpos.x-=fWidth*0.5;
				if(align_type==AcDbMText::kTopLeft||align_type==AcDbMText::kTopCenter||align_type==AcDbMText::kTopRight)
					dimpos.y-=fTestH*0.5;
				else if(align_type==AcDbMText::kBottomLeft||align_type==AcDbMText::kBottomCenter||align_type==AcDbMText::kBottomRight)
					dimpos.y+=fTestH*0.5;
				Cpy_Pnt(pCell->dimpos,dimpos);
			}
			else if(pEnt->isKindOf(AcDbBlockReference::desc()))
			{
				AcDbBlockReference *pBlockRef=(AcDbBlockReference*)pEnt;
				AcDbBlockTableRecord *pTempBlockTableRecord;//定义块表记录指针
				acdbOpenObject(pTempBlockTableRecord,pBlockRef->blockTableRecord(),AcDb::kForRead);
				pTempBlockTableRecord->close();//关闭块表
				AcDbBlockTableRecordIterator *pIterator=NULL;
				pTempBlockTableRecord->newIterator( pIterator);
				if(pIterator)
				{
					bool bAllEntIsLine=true;
					for(;!pIterator->done();pIterator->step())
					{
						pIterator->getEntity(pEnt,AcDb::kForRead);
						CLocalAcDbObjLife entObj(pEnt);
						if(!pEnt->isKindOf(AcDbLine::desc()))
						{
							bAllEntIsLine=false;
							break;
						}
					}
					if(bAllEntIsLine)
						pLineBlockRef=pBlockRef;
					else
						pLineBlockRef=NULL;
				}
				if(pLineBlockRef&&i==1)
					break;
			}
		}
		if(pLineBlockRef&&i==1)
		{	//选中块，提示用户是否打碎继续提取 wht 18-12-24
			pLineBlockRef->highlight();
			AcDbObjectId objId=pLineBlockRef->objectId();
			acdbOpenAcDbEntity(pEnt,objId,AcDb::kForWrite);
			CLocalAcDbObjLife entLife(pEnt);
			pLineBlockRef=(AcDbBlockReference*)pEnt;
			if(pLineBlockRef&&AfxMessageBox("表格线是块，是否打碎块继续提取(Y/N)?",MB_YESNO)==IDYES)
			{
				pLineBlockRef->explodeToOwnerSpace();
				if(pLineBlockRef)
					pLineBlockRef->erase();
				//刷新屏幕
				actrTransactionManager->flushGraphics();
				acedUpdateDisplay();
				continue;
			}
			else
				break;
		}
		else
			break;
	}
	CHeapSort<f3dLine>::HeapSort(arrHoriLines.m_pData,arrHoriLines.GetSize(),_LocalCompareHoriLines);
	CHeapSort<f3dLine>::HeapSort(arrVertLines.m_pData,arrVertLines.GetSize(),_LocalCompareVertLines);
	if(arrHoriLines.GetSize()>0&&arrVertLines.GetSize()>1)
	{
		f3dLine line1;
		double GAP=0.5;
		int iTail=arrVertLines.GetSize()-1;
		line1.startPt=(arrVertLines[0].startPt.y<arrVertLines[0].endPt.y)?arrVertLines[0].endPt:arrVertLines[0].startPt;
		line1.endPt=(arrVertLines[iTail].startPt.y<arrVertLines[iTail].endPt.y)?arrVertLines[iTail].endPt:arrVertLines[iTail].startPt;
		if(fabs(line1.startPt.y-line1.endPt.y)<GAP&&(line1.startPt.y-arrHoriLines[0].startPt.y)>1)
		{	//补齐第一行顶部水平线 wht 18-07-31
			arrHoriLines.insert(line1,0);
		}
		//排除小于行高的竖线[5中的[有的图纸会三条线绘制 wht 18-12-20
		if(arrHoriLines.GetSize()>1)
		{
			double row_height=0;
			for(int i=1;i<arrHoriLines.GetSize();i++)
			{
				row_height=fabs(arrHoriLines[i].startPt.y-arrHoriLines[i-1].startPt.y);
				if(row_height>0)
					break;
			}
			for(int i=0;i<arrVertLines.GetSize();i++)
			{
				double len=DISTANCE(arrVertLines[i].startPt,arrVertLines[i].endPt);
				if(len<row_height)
				{
					if(arrVertLines.RemoveAt(i)&&i>0)
						i--;
				}
			}
		}
	}
	m_xRowlines.SetSize(0,arrHoriLines.GetSize());
	m_xCollines.SetSize(0,arrVertLines.GetSize());

	RECT rc;
	for(int row=0;row<arrHoriLines.GetSize();row++)
	{
		rc.top=ftoi(arrHoriLines[row].startPt.y);
		m_xRowlines.append(arrHoriLines[row]);
		bool bInitBottom=false;
		while(row<arrHoriLines.GetSize()){
			if(row==arrHoriLines.GetSize()-1)
				break;
			rc.bottom=ftoi(arrHoriLines[row+1].startPt.y);
			bInitBottom=true;
			if(rc.top-rc.bottom<1)
				row++;
			else
				break;
		};
		if(!bInitBottom)
			continue;
		for(pCell=listCells.EnumObjectFirst();pCell;pCell=listCells.EnumObjectNext())
		{
			if(pCell->contents.GetLength()==0)
				continue;
			if(pCell->dimpos.y<=rc.top&&pCell->dimpos.y>=rc.bottom)
			{
				pCell->iRow=m_xRowlines.GetSize()-1;
				pCell->rc.top=rc.top;
				pCell->rc.bottom=rc.bottom;
			}
		}
	}
	for(int col=0;col<arrVertLines.GetSize();col++)
	{
		rc.left=ftoi(arrVertLines[col].startPt.x);
		m_xCollines.append(arrVertLines[col]);
		while(col<arrVertLines.GetSize()){
			if(col==arrVertLines.GetSize()-1)
				break;
			rc.right=ftoi(arrVertLines[col+1].startPt.x);
			if(rc.right-rc.left<1)
				col++;
			else
				break;
		};
		for(pCell=listCells.EnumObjectFirst();pCell;pCell=listCells.EnumObjectNext())
		{
			if(pCell->contents.GetLength()==0)
				continue;
			if(pCell->dimpos.x>=rc.left&&pCell->dimpos.x<=rc.right)
			{
				pCell->iCol=m_xCollines.GetSize()-1;
				pCell->rc.left=rc.left;
				pCell->rc.right=rc.right;
			}
		}
	}
	return TRUE;
}

BOOL CPartBomTable::SelectEntFromUI(BYTE ciPartType)
{
	AcDbDatabase *pCurDataBase=acdbHostApplicationServices()->workingDatabase();
	if(pCurDataBase&&!pCurDataBase->ucsorg().isEqualTo(AcGePoint3d(0,0,0)))
	{	//确定当前坐标系为世界坐标系，否则可能导致无法正确提取 wht 18-08-08
		AcGeMatrix3d mat;
		mat.setCoordSystem(AcGePoint3d(0,0,0),AcGeVector3d(1,0,0),AcGeVector3d(0,1,0),AcGeVector3d(0,0,1));
		acedSetCurrentUCS(mat);	
	}
	f3dPoint origin;
	ads_point org_L_T,org_R_B;
	CXhChar100 sLeftTop("\n请选择构件明细表的左上角,<Enter退出>:");
	CXhChar100 sRightBottom("\n请选择构件明细表的右下角,<Enter退出>:");
	if(ciPartType==RECOG_BOLT)
	{
		sLeftTop.Copy("\n请选择螺栓明细表的左上角,<Enter退出>:");
		sRightBottom.Copy("\n请选择螺栓明细表的右下角,<Enter退出>:");
	}
#ifdef _ARX_2007
	if(ads_getpoint(NULL,_bstr_t(sLeftTop),org_L_T)!=RTNORM)
		return FALSE;
	if(ads_getpoint(NULL,_bstr_t(sRightBottom),org_R_B)!=RTNORM)
		return FALSE;
	ads_command(RTSTR,L"ZOOM",RTSTR,L"e",RTNONE);	//不全显示可能导致未显示在屏幕中的区域提取失败 wjh-2016.12.16
#else
	if(ads_getpoint(NULL,sLeftTop,org_L_T)!=RTNORM)
		return FALSE;
	if(ads_getpoint(NULL,sRightBottom,org_R_B)!=RTNORM)
		return FALSE;
	ads_command(RTSTR,"ZOOM",RTSTR,"e",RTNONE);
#endif
	double gap=0.5;
	leftTop.Set(org_L_T[0],org_L_T[1],0);
	rightBottom.Set(org_R_B[0],org_R_B[1],0);
	//GEPOINT rgn_vert[4];
	rgn_vert[0].Set(org_L_T[0]-gap,org_L_T[1]+gap);
	rgn_vert[1].Set(org_L_T[0]-gap,org_R_B[1]-gap);
	rgn_vert[2].Set(org_R_B[0]+gap,org_R_B[1]-gap);
	rgn_vert[3].Set(org_R_B[0]+gap,org_L_T[1]+gap);
	return InternalSelectEntFromUI();
}

BOOL CPartBomTable::InternalSelectEntFromUI()
{
	resbuf verts4[4];
	for(int i=0;i<4;i++)
	{
		verts4[i].restype=5002;
		verts4[i].resval.rpoint[X] = rgn_vert[i].x;
		verts4[i].resval.rpoint[Y] = rgn_vert[i].y;
		verts4[i].resval.rpoint[Z] = rgn_vert[i].z;
		if(i<3)
			verts4[i].rbnext=&verts4[i+1];
		else
			verts4[3].rbnext=NULL;
	}
	ads_name ss_name,entname;
#if defined(_ARX_2007)&&!defined(_ZRX_2012)
	if (acedSSGet(L"cp",&verts4[0],NULL,NULL,ss_name)!=RTNORM)
#else
	if (acedSSGet("cp",&verts4[0],NULL,NULL,ss_name)!=RTNORM)
#endif
	{
		acedSSFree(ss_name);
		return FALSE;
	}
	ads_point L_T,R_B;
	L_T[X]=rgn_vert[0].x;
	L_T[Y]=rgn_vert[0].y;
	L_T[Z]=0;
	R_B[X]=rgn_vert[2].x;
	R_B[Y]=rgn_vert[2].y;
	R_B[Z]=0;
#ifdef _ARX_2007
	ads_command(RTSTR,L"ZOOM",RTSTR,"W",RTPOINT,L_T,RTPOINT,R_B,RTNONE);
#else
	ads_command(RTSTR,"ZOOM",RTSTR,"W",RTPOINT,L_T,RTPOINT,R_B,RTNONE);
#endif
	long ll=0;
	acedSSLength(ss_name,&ll);
	m_xSelEntIdSet.Empty();
	for(int i=0;i<ll;i++)
	{
		AcDbObjectId entId;
		AcDbEntity *pEnt;
		acedSSName(ss_name,i,entname);
		acdbGetObjectId(entId,entname);
		acdbOpenObject(pEnt,entId,AcDb::kForRead);
		if(pEnt==NULL)
			continue;
		pEnt->close();
		m_xSelEntIdSet.SetValue(entId.handle(),entId);
	}
	acedSSFree(ss_name);
	return TRUE;
}
//////////////////////////////////////////////////////////////////////////
static bool IsHasBomRecogFunction()
{
#if defined(__LDS_FILE_)||defined(__TDA_FILE_)
	return (VerifyValidFunction(LICFUNC::FUNC_IDENTITY_INTERNAL_TEST)||VerifyValidFunction(LICFUNC::FUNC_IDENTITY_SMART_DRAWING))!=FALSE;
#elif defined(__LMA_FILE_)||defined(__TMA_FILE_)
	return VerifyValidFunction(LICFUNC::FUNC_IDENTITY_INTERNAL_TEST)!=FALSE;
#elif defined(__IBOM_)
 	return (VerifyValidFunction(LICFUNC::FUNC_IDENTITY_INTERNAL_TEST)||VerifyValidFunction(LICFUNC::FUNC_IDENTITY_READ_DWG))!=FALSE;
#else
	return VerifyValidFunction(LICFUNC::FUNC_IDENTITY_INTERNAL_TEST)!=FALSE;
#endif
}
#if defined(__LDS_FILE_)||defined(__TMA_FILE_)||defined(__LMA_FILE_)||defined(__TDA_FILE_)
void RecogizePartBomToTxt()
{
	if(!IsHasBomRecogFunction())
		return;
	CPartBomTable xPartBomTable;
	if(!xPartBomTable.SelectEntFromUI(CPartBomTable::RECOG_PART))
		return;
	if(xPartBomTable.RecogizeBomTable())
		xPartBomTable.ExportBomToTxt();
}
void RecogizePartBomToBom()
{
	if(!IsHasBomRecogFunction())
		return;
	CAcModuleResourceOverride resOverride;
	CTowerBOM xBomObj;
	//提取构件明细
	while(1)
	{
		CPartBomTable xPartBomTable;
		if(!xPartBomTable.SelectEntFromUI(CPartBomTable::RECOG_PART))
			break;
		xPartBomTable.RecogizeBomTable();
		xPartBomTable.RetrieveParts();
		for(CManuPart* pBomPart=xPartBomTable.EnumFirstPart();pBomPart;pBomPart=xPartBomTable.EnumNextPart())
			xBomObj.Parts.append(*pBomPart);
	}
	//提取螺栓明细
	while(1)
	{
		CPartBomTable xPartBomTable;
		if(!xPartBomTable.SelectEntFromUI(CPartBomTable::RECOG_BOLT))
			break;
		xPartBomTable.RecogizeBomTable();
		xPartBomTable.RetrieveBolts();
		for(CStatBolt* pBomBolt=xPartBomTable.EnumFirstBolt();pBomBolt;pBomBolt=xPartBomTable.EnumNextBolt())
			xBomObj.Bolts.append(*pBomBolt);
	}
	//导出到指定文件
	//CFileDialog savedlg(FALSE,"bom","mxb");
	CFileDialog savedlg(TRUE,"bom","Tower",
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		"统材信息(*.bom)|*.bom|所有文件(*.*)|*.*||");
	if(savedlg.DoModal()!=IDOK)
		return;
	xBomObj.WriteTextBOMFile(savedlg.GetPathName());
}
#elif defined(__BOM_FILE_)
static void MakeDirectory(char *path)
{
	char bak_path[MAX_PATH],drive[MAX_PATH];
	strcpy(bak_path,path);
	char *dir = strtok(bak_path,"/\\");
	if(strlen(dir)==2&&dir[1]==':')
	{
		strcpy(drive,dir);
		strcat(drive,"\\");
		_chdir(drive);
		dir = strtok(NULL,"/\\");
	}
	while(dir)
	{
		_mkdir(dir);
		_chdir(dir);
		dir = strtok(NULL,"/\\");
	}
}
static BOOL GetCurWorkPath(CString& file_path,const char* bomd_folder,BOOL bEscapeChar/*=TRUE*/)
{
	AcApDocument* pDoc = acDocManager->curDocument();
	if(pDoc==NULL)
		return FALSE;
	CXhChar500 sWorkDir,sName;
	file_path=pDoc->fileName();
	if(file_path.CompareNoCase("Drawing1.dwg")==0)	//默认的DWG文件
		return FALSE;
	_splitpath(file_path,NULL,NULL,sName,NULL);
	int index=file_path.ReverseFind('\\');	//反向查找'\\'
	file_path=file_path.Left(index);		//移除文件名
	//
	if(bomd_folder!=NULL)
		sWorkDir.Printf("%s\\%s",file_path,bomd_folder);
	else
		sWorkDir.Printf("%s\\%s",file_path,(char*)sName);
	MakeDirectory(sWorkDir);
	if(bEscapeChar)
		sWorkDir.Append("\\");
	file_path.Format("%s",(char*)sWorkDir);
	return TRUE;
}

void RecogizePartBomToBomdFile()
{
	if(!IsHasBomRecogFunction())
	{
		AfxMessageBox("软件缺少合法使用授权!");
		return;
	}
	CPartBomTable xPartBomTable;
	if(!xPartBomTable.SelectEntFromUI(CPartBomTable::RECOG_PART))
		return;
	if(xPartBomTable.RecogizeBomTable())
	{
		CString sWorkPath;
		GetCurWorkPath(sWorkPath,"BOM",TRUE);
		xPartBomTable.ExportBomToBomdFile(sWorkPath,true);
	}
	RecogizePartBomToBomdFile();
}
#endif