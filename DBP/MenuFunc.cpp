#include "StdAfx.h"
#include "dbents.h"
#include "f_ent.h"
#include "f_alg_fun.h"
#include "ArrayList.h"
#include "SortFunc.h"
#include "LicFuncDef.h"
#include "XhLicAgent.h"
#include "MenuFunc.h"
#include "XhCharString.h"
#include "list.h"
#include "SegI.h"
#include "direct.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
//提取构件明细表
struct BOM_CELL{
	int iRow,iCol;	//0为基数
	RECT rc;
	GEPOINT dimpos;	//文本标注位置
	CXhChar50 contents;
};
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
	if(fabs(lineStart.x-lineEnd.x)<EPS)
		horiLine=false;
	else if(fabs(lineStart.y-lineEnd.y)<EPS)
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
void RecogizePartBom()
{
	if(!VerifyValidFunction(LICFUNC::FUNC_IDENTITY_READ_DWG))
	{
		AfxMessageBox("软件缺少合法使用授权!");
		return;
	}
	f3dPoint origin;
	ads_point org_L_T,org_R_B;
#ifdef _ARX_2007
	if (ads_getpoint(NULL,L"\n请选择构件明细表的左上角,<Enter退出>: ",org_L_T)!=RTNORM)
		return;
	if (ads_getpoint(NULL,L"\n请选择构件明细表的右下角,<Enter退出>: ",org_R_B)!=RTNORM)
		return;
	ads_command(RTSTR,L"ZOOM",RTSTR,L"e",RTNONE);	//不全显示可能导致未显示在屏幕中的区域提取失败 wjh-2016.12.16
#else
	if (ads_getpoint(NULL,"\n请选择构件明细表的左上角,<Enter退出>: ",org_L_T)!=RTNORM)
		return;
	if (ads_getpoint(NULL,"\n请选择构件明细表的右下角,<Enter退出>: ",org_R_B)!=RTNORM)
		return;
	ads_command(RTSTR,"ZOOM",RTSTR,"e",RTNONE);
#endif
	ads_name ss_name;
	resbuf verts4[4];
	GEPOINT rgn_vert[4];
	double gap=0.5;
	rgn_vert[0].Set(org_L_T[0]-gap,org_L_T[1]+gap);
	rgn_vert[1].Set(org_L_T[0]-gap,org_R_B[1]-gap);
	rgn_vert[2].Set(org_R_B[0]+gap,org_R_B[1]-gap);
	rgn_vert[3].Set(org_R_B[0]+gap,org_L_T[1]+gap);
	long i,ll;
	for(i=0;i<4;i++)
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
#if defined(_ARX_2007)&&!defined(_ZRX_2012)
	if (acedSSGet(L"cp",&verts4[0],NULL,NULL,ss_name)!=RTNORM)
#else
	if (acedSSGet("cp",&verts4[0],NULL,NULL,ss_name)!=RTNORM)
#endif
	{
		acedSSFree(ss_name);
		AfxMessageBox("没有识别出构件明细表");
	}
#ifdef _ARX_2007
	ads_command(RTSTR,L"ZOOM",RTSTR,"P",RTNONE);	//提取完明细表后，要恢复原来的缩放显示状态 wjh-2016.12.16
#else
	ads_command(RTSTR,"ZOOM",RTSTR,"P",RTNONE);
#endif
	BOM_CELL* pCell;
	ads_name entname;
	acedSSLength(ss_name,&ll);
	f3dLine line;
	CXhSimpleList<BOM_CELL> listCells;
	ARRAY_LIST<f3dLine> arrHoriLines(0,50),arrVertLines(0,8);
	for(i=0;i<ll;i++)
	{
		AcDbObjectId entId;
		AcDbEntity *pEnt;
		AcDbLine*  pLine=NULL;
		AcDbText*  pText=NULL;
		AcDbMText* pMText=NULL;
		AcDbPolyline *pPolyline=NULL;
		acedSSName(ss_name,i,entname);
		acdbGetObjectId(entId,entname);
		acdbOpenObject(pEnt,entId,AcDb::kForRead);
		AcGePoint3d acad_start,acad_end;
		if(pEnt->isKindOf(AcDbLine::desc()))
		{
			pLine=(AcDbLine*)pEnt;
			acad_start=pLine->startPoint();
			acad_end=pLine->endPoint();
			Cpy_Pnt(line.startPt,acad_start);
			Cpy_Pnt(line.endPt,acad_end);
			if( fabs(line.startPt.x-line.endPt.x)<EPS &&
				_LocalIsInteralLineInRect(line.startPt,line.endPt,rgn_vert[0],rgn_vert[2]))
				arrVertLines.append(line);	//竖线
			else if(fabs(line.startPt.y-line.endPt.y)<EPS &&
				_LocalIsInteralLineInRect(line.startPt,line.endPt,rgn_vert[0],rgn_vert[2]))
				arrHoriLines.append(line);	//横线
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
			double fLen=TestDrawTextLength(pCell->contents,pText->height(),pText->textStyle());
			AcGePoint3d dimpos=pText->position();
			dimpos.x+=fLen*0.5;
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
			dimpos.x+=pText->actualWidth()*0.5;	//取中点为基准位置
			Cpy_Pnt(pCell->dimpos,dimpos);
		}
		pEnt->close();
	}
	acedSSFree(ss_name);
	CHeapSort<f3dLine>::HeapSort(arrHoriLines.m_pData,arrHoriLines.GetSize(),_LocalCompareHoriLines);
	CHeapSort<f3dLine>::HeapSort(arrVertLines.m_pData,arrVertLines.GetSize(),_LocalCompareVertLines);
	int row,col;
	ARRAY_LIST<f3dLine>rowlines(0,arrHoriLines.GetSize()),collines(0,arrVertLines.GetSize());
	for(row=0;row<arrHoriLines.GetSize();row++)
	{
		RECT rc;
		rc.top=ftoi(arrHoriLines[row].startPt.y);
		rowlines.append(arrHoriLines[row]);
		while(row<arrHoriLines.GetSize()){
			if(row==arrHoriLines.GetSize()-1)
				break;
			rc.bottom=ftoi(arrHoriLines[row+1].startPt.y);
			if(rc.top-rc.bottom<1)
				row++;
			else
				break;
		};
		for(pCell=listCells.EnumObjectFirst();pCell;pCell=listCells.EnumObjectNext())
		{
			if(pCell->contents.Length==0)
				continue;
			if(pCell->dimpos.y<=rc.top&&pCell->dimpos.y>=rc.bottom)
			{
				pCell->iRow=rowlines.GetSize()-1;
				pCell->rc.top=rc.top;
				pCell->rc.bottom=rc.bottom;
			}
		}
	}
	for(col=0;col<arrVertLines.GetSize();col++)
	{
		RECT rc;
		rc.left=ftoi(arrVertLines[col].startPt.x);
		collines.append(arrVertLines[col]);
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
			if(pCell->contents.Length==0)
				continue;
			if(pCell->dimpos.x>=rc.left&&pCell->dimpos.x<=rc.right)
			{
				pCell->iCol=collines.GetSize()-1;
				pCell->rc.left=rc.left;
				pCell->rc.right=rc.right;
			}
		}
	}
	ARRAY_LIST<CXhChar200> partList;
	SEGI segI;
	CXhChar16 sFirstLabel,sTailLabel;
	for(row=0;row<rowlines.GetSize()-1;row++)
	{
		CXhChar200 sPartText;
		for(col=0;col<collines.GetSize()-1;col++)
		{
			CXhChar100 sCellText;
			for(pCell=listCells.EnumObjectFirst();pCell;pCell=listCells.EnumObjectNext())
			{
				if(pCell->contents.Length==0||pCell->iRow!=row||pCell->iCol!=col)
					continue;
				sCellText.Append(pCell->contents);
			}
			if(col==0)
			{
				if(row==0)
					sFirstLabel.Copy(sCellText);
				else if(row==rowlines.GetSize()-2)
					sTailLabel.Copy(sCellText);
			}
			//
			sCellText.ResizeLength(16,' ',true);
			sPartText.Append(sCellText);
		}
		partList.append(sPartText);
	}
	CString sWorkPath;
	GetCurWorkPath(sWorkPath,"BOM",TRUE);
	CXhChar500 sDwgFilePath("%s/%s~%s.bomd",sWorkPath,(char*)sFirstLabel,(char*)sTailLabel);
	FILE* fp=fopen(sDwgFilePath,"wt");
	for(CXhChar200 *pLineText=partList.GetFirst();pLineText;pLineText=partList.GetNext())
	{
		fprintf(fp,"%s",(char*)*pLineText);
		fprintf(fp,"\n");
	}
	fclose(fp);
	MessageBox(NULL,CXhChar500("成功提出%s~%s共%d个构件！",(char*)sFirstLabel,(char*)sTailLabel,partList.GetSize()),"提示",MB_OK);
	//重复执行命令
	RecogizePartBom();
	//WinExec(CXhChar200("notepad.exe %s",(char*)sDwgFilePath),SW_SHOW);
	/*CFileDialog savedlg(FALSE,"txt","mxb");
	if(savedlg.DoModal()!=IDOK)
		return;
	FILE* fp=fopen(savedlg.GetPathName(),"wt");
	for(row=0;row<rowlines.GetSize()-1;row++)
	{
		for(col=0;col<collines.GetSize()-1;col++)
		{
			CXhChar100 celltext;
			for(pCell=listCells.EnumObjectFirst();pCell;pCell=listCells.EnumObjectNext())
			{
				if(pCell->contents.Length==0||pCell->iRow!=row||pCell->iCol!=col)
					continue;
				celltext.Append(pCell->contents);
			}
			celltext.ResizeLength(16,' ',true);
			fprintf(fp,"%s",(char*)celltext);
		}
		fprintf(fp,"\n");
	}
	fclose(fp);
	WinExec(CXhChar200("notepad.exe %s",savedlg.GetPathName()),SW_SHOW);*/
}