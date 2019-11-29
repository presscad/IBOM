#include "stdafx.h"
#include "IBOM.h"
#include "Tool.h"
#include "FileTreeDlg.h"
#include "MainFrm.h"

#ifdef __TEST_
CLogFile logTest;
void CopyBomPartProp(IRecoginizer::BOMPART *pSrcPart,IRecoginizer::BOMPART *pDestPart)
{
	pDestPart->id=pSrcPart->id;
	pDestPart->bInput=pSrcPart->bInput;
	strcpy(pDestPart->sLabel,pSrcPart->sLabel);
	strcpy(pDestPart->sSizeStr,pSrcPart->sSizeStr);
	strcpy(pDestPart->materialStr,pSrcPart->materialStr);
	memcpy(pDestPart->arrItemWarningLevel,pSrcPart->arrItemWarningLevel, IRecoginizer::BOMPART::WARNING_ARR_LEN);
	pDestPart->rc=pSrcPart->rc;
	pDestPart->count=pSrcPart->count;
	pDestPart->length=pSrcPart->length;
	pDestPart->weight=pSrcPart->weight;
}

void LazyLoadImageRegion(CImageRegionHost *pRegion);
void TestSingleIBOMFile(const char* ibom_file,int sum_count,int cur_index)
{
	if(ibom_file==NULL)
		return;
	logTest.Log("===>%s",ibom_file);
	CIBOMDoc *pDoc=theApp.GetDocument();
	if(pDoc==NULL||ibom_file==NULL)
	{
		logTest.Log("�ļ���ʧ�ܣ�");
		return;
	}
	dataModel.Empty();
	//1.����IBOM�ļ�
	BOOL bRetCode=pDoc->OnOpenDocument(ibom_file);
	if(!bRetCode)
		return;
	CFileTreeDlg* pTreePage=((CMainFrame*)AfxGetMainWnd())->GetFileTreePage();
	if(pTreePage)
		pTreePage->RefreshTreeCtrl();
	int iRegion=0,nRegionCount=0;
	for(CImageFileHost *pImageFile=dataModel.EnumFirstImage();pImageFile;pImageFile=dataModel.EnumNextImage())
	{
		for(CImageRegionHost *pRegion=pImageFile->EnumFirstRegionHost();pRegion;pRegion=pImageFile->EnumNextRegionHost())
		{
			nRegionCount++;
		}
	}
	DisplayProcess(0);
	for(CImageFileHost *pImageFile=dataModel.EnumFirstImage();pImageFile;pImageFile=dataModel.EnumNextImage())
	{
		iRegion++;
		DisplayProcess((100*iRegion)/nRegionCount,CXhChar50("�ļ���%d/%d,��ǰ�ļ�����%d/%d",cur_index,sum_count,iRegion,nRegionCount));
		logTest.Log(pImageFile->szFileName);
		pImageFile->InitImageFile(pImageFile->szPathFileName,ibom_file);
		for(CImageRegionHost *pRegion=pImageFile->EnumFirstRegionHost();pRegion;pRegion=pImageFile->EnumNextRegionHost())
		{
			logTest.Log(pRegion->m_sName);
			CWaitCursor cursor;
			LazyLoadImageRegion(pRegion);
			ATOM_LIST<IRecoginizer::BOMPART> srcBomPartList;
			for(IRecoginizer::BOMPART *pBomPart=pRegion->EnumFirstPart();pBomPart;pBomPart=pRegion->EnumNextPart())
			{
				IRecoginizer::BOMPART *pNewPart=srcBomPartList.append();
				CopyBomPartProp(pBomPart,pNewPart);
			}
			pRegion->SummarizePartInfo();
			int i=0;
			for(IRecoginizer::BOMPART *pBomPart=pRegion->EnumFirstPart();pBomPart;pBomPart=pRegion->EnumNextPart())
			{				
				IRecoginizer::BOMPART *pSrcPart=srcBomPartList.GetByIndex(i);
				if( pSrcPart==NULL||
					stricmp(pSrcPart->sLabel,pBomPart->sLabel)!=0||
					stricmp(pSrcPart->sSizeStr,pBomPart->sSizeStr)!=0||
					pSrcPart->count!=pSrcPart->count||
					fabs(pSrcPart->length-pBomPart->length)>EPS2)
				{
					if(pSrcPart==NULL)
						logTest.Log(CXhChar50("δ�ҵ�%s��Ӧ�Ĺ���",(char*)pBomPart->sLabel));
					else
					{
						if( stricmp(pSrcPart->sLabel,pBomPart->sLabel)!=0||
							stricmp(pSrcPart->sSizeStr,pBomPart->sSizeStr)!=0||
							fabs(pSrcPart->length-pBomPart->length)>EPS2||
							pSrcPart->count!=pBomPart->count)
						{
							logTest.Log("%s-------------------------",pBomPart->sLabel);
							if(stricmp(pSrcPart->sLabel,pBomPart->sLabel)!=0)
								logTest.Log("%s		%s",pSrcPart->sLabel,pBomPart->sLabel);
							if(stricmp(pSrcPart->sSizeStr,pBomPart->sSizeStr)!=0)
								logTest.Log("%s		%s",pSrcPart->sSizeStr,pBomPart->sSizeStr);
							if(fabs(pSrcPart->length-pBomPart->length)>EPS2)
								logTest.Log("%.1f		%.1f",pSrcPart->length,pBomPart->length);
							if(pSrcPart->count!=pBomPart->count)
								logTest.Log("%d		%d",pSrcPart->count,pBomPart->count);
						}
						//logTest.Log("Org��%s	%s	%.1f	%d",(char*)pSrcPart->sLabel,(char*)pSrcPart->sSizeStr,pSrcPart->length,pSrcPart->count);
						//logTest.Log("New��%s	%s	%.1f	%d",(char*)pBomPart->sLabel,(char*)pBomPart->sSizeStr,pBomPart->length,pBomPart->count);
					}
				}
				i++;
			}
		}
	}
	DisplayProcess(100);
}

void BatchTestRecogBomPart()
{
	//1.��ȡIBOM�ļ��б�
	ATOM_LIST<CXhChar500> filePathList;
	DWORD nFileNumbers = 500;    //CFileDialog���ѡ���ļ�����
	CString filter="IBOM(*.ibom)|*.ibom|�����ļ�(*.*)|*.*||";
	CFileDialog fileDlg(TRUE,"ibom","IBOM.ibom",OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_ALLOWMULTISELECT,filter);
	fileDlg.m_ofn.lpstrTitle="ѡ������Ե�IBOM�ļ�";
	TCHAR *pBufOld = fileDlg.m_ofn.lpstrFile;   //����ɵ�ָ��
	DWORD dwMaxOld = fileDlg.m_ofn.nMaxFile;    //����ɵ�����
	fileDlg.m_ofn.lpstrFile = new TCHAR[ nFileNumbers * MAX_PATH];
	ZeroMemory(fileDlg.m_ofn.lpstrFile,sizeof(TCHAR) * nFileNumbers * MAX_PATH);
	fileDlg.m_ofn.nMaxFile = nFileNumbers * MAX_PATH;    //�������,��MSDN����ΪfileDlg.m_ofn.lpstrFileָ��Ļ��������ַ���
	if(fileDlg.DoModal()==IDOK)
	{	//0.�����ֶ���HTREEITEMӳ���ϵ
		POSITION pos=fileDlg.GetStartPosition();
		while(pos)
		{
			CString sPathName=fileDlg.GetNextPathName(pos);		//��ȡ�ļ��� 
			filePathList.append(CXhChar500(sPathName));
		}
	}
	delete [](fileDlg.m_ofn.lpstrFile);        //������ڴ�
	fileDlg.m_ofn.lpstrFile = pBufOld;        //��ԭ֮ǰ��ָ��,��Ϊû�������CFileDialogԴ�����,��������Ƿ�����,�����Ȼ�ԭ�ϰ�,
	fileDlg.m_ofn.nMaxFile = dwMaxOld;        //��ԭ֮ǰ�������
	//2.�����򿪲���
	int nSumCount=filePathList.GetNodeNum();
	for(int i=0;i<filePathList.GetNodeNum();i++)
	{
		CXhChar500 *pPath=filePathList.GetByIndex(i);
		if(pPath)
			TestSingleIBOMFile(*pPath,nSumCount,i+1);
		else
			TestSingleIBOMFile(NULL,nSumCount,i+1);
	}
}


void CIBOMView::OnTest()
{
	logTest.InitLogFile("D:\\IBOM-Test.log",true);
	logTest.EnableTimestamp(false);
	BatchTestRecogBomPart();
	logTest.ShowToScreen();
}
#endif