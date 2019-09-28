// LicenseServerActivatePage.cpp : 实现文件
//

#include "stdafx.h"
#include "IBOM.h"
#include "afxdialogex.h"
#include "LicenseAuthorizeDlg.h"
#include "XhLicAgent.h"

// CLicenseServerActivatePage 对话框

IMPLEMENT_DYNAMIC(CLicenseServerActivatePage, CDialogEx)

CLicenseServerActivatePage::CLicenseServerActivatePage(CWnd* pParent /*=NULL*/)
	: CDialogEx(CLicenseServerActivatePage::IDD, pParent)
{
	m_iDogType=0;
	m_dwLicenseApplyTimeStamp=0;
}

CLicenseServerActivatePage::~CLicenseServerActivatePage()
{
}

void CLicenseServerActivatePage::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CLicenseServerActivatePage, CDialogEx)
	ON_BN_CLICKED(IDC_BN_ACTIVATE_END_LICENSE, OnBnActivateEndLicense)
	ON_BN_CLICKED(IDC_BN_ACTIVATE_SERVER_LICENSE, &CLicenseServerActivatePage::OnBnActivateServerLicense)
	ON_BN_CLICKED(IDC_BN_CREATE_IDENTITY_FILE, &CLicenseServerActivatePage::OnBnCreateIdentityFile)
	ON_BN_CLICKED(IDC_BN_IMPORT_AUTHORIZE_FILE, &CLicenseServerActivatePage::OnBnImportAuthorizeFile)
END_MESSAGE_MAP()


// CLicenseServerActivatePage 消息处理程序

#include "LicenseServerActivatePage.h"
void CLicenseServerActivatePage::OnBnActivateEndLicense()
{
	CLicenseAuthorizeDlg* m_pParentDialg=NULL;
	CWnd* pParent=GetParent();
	if(pParent)
		m_pParentDialg=(CLicenseAuthorizeDlg*)pParent->GetParent();
	if(m_pParentDialg)
	{
		ShowWindow(SW_HIDE);
		m_pParentDialg->ShowLanLicApplyPage(SW_SHOW);
	}
}

DWORD MakeTimeStamp(WORD year,WORD month,WORD day,WORD hour)
{
	DWORD stamp=year<<16;	//年份单独占2Bytes
	stamp+=month<<12;		//月份占13~16位
	stamp+=day<<6;			//日期占7~12位
	stamp+=hour&0x002F;		//小时占1~6位
	return stamp;
}
void CLicenseServerActivatePage::OnBnActivateServerLicense()
{
}

void CLicenseServerActivatePage::OnBnCreateIdentityFile()
{
	int c2v_len=GetC2VInfo((BYTE)m_iDogType,NULL);
	if(c2v_len==-1)
		AfxMessageBox("未找到加密锁, 请核实是否开启了防火墙或与插狗计算机存在通信故障或局域网内安装了多个网络狗服务!如确认防火墙未开,且局域网连接正常可以通过设定gsnetdog.ini(指定插有加密锁计算机的ip地址)直接访问!");
	else if(c2v_len==-2)
		AfxMessageBox("加密锁连接失败!");
	if(c2v_len<=0)
		return;	//获取失败
	CBuffer buffer;
	CHAR_ARRAY c2v(c2v_len);
	c2v_len=GetC2VInfo((BYTE)m_iDogType,c2v);
	COMPUTER computer;
	BYTE key[32]={0},details[192]={0};
	COMPUTER::Generate128Identity(key,&m_dwLicenseApplyTimeStamp,details,192);
	DWORD dwDetailBytes=0;
	for(dwDetailBytes=0;dwDetailBytes<192;dwDetailBytes+=16)
	{
		INT64PTR i64(details+dwDetailBytes);
		if(i64[0]==0&&i64[1]==0)
			break;
	}
	UINT uDogSerial=DogSerialNo();	//获取加密锁序列号
	buffer.WriteDword(m_dwLicenseApplyTimeStamp);		//写入指纹生成时间戳
	buffer.Write(key,32);				//写入指纹标识信息
	buffer.WriteDword(dwDetailBytes);	//写入指纹详细信息内存块长度
	buffer.Write(details,dwDetailBytes);//写入指纹详细信息
	buffer.WriteString(computer.sComputerName,51);	//写入主机名称
	BYTE ipv4[4];
	BYTE ciCount=(BYTE)COMPUTER::GetIP(NULL,computer.sComputerName,0);
	buffer.WriteByte(ciCount);	//写入IP数量
	for(BYTE i=0;i<ciCount;i++)
	{
		ipv4[0]=ipv4[1]=ipv4[2]=ipv4[3]=0;
		COMPUTER::GetIP(ipv4,computer.sComputerName,i);
		buffer.Write(ipv4,4);	//写入每个IP的地址
	}
	buffer.Write(c2v,c2v_len);
	//写入文件
	char fn[MAX_PATH];
	sprintf(fn,"IBOM%d.apl",uDogSerial);

	CFileDialog dlg(FALSE,"apl",fn,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,"用户的授权申请文件(*.apl)|*.apl|所有文件(*.*)|*.*||");
	if(dlg.DoModal()==IDOK)
	{
		//v2cfn=dlg.GetPathName().GetString();
		FILE* fp=fopen(dlg.GetPathName(),"wb");
		if(fp==NULL)
		{
			AfxMessageBox("APL文件打开失败!");
			return;
		}
		XHVERSION licapplyfile_version("0.0.0.1");
		//1.存储apl文件头信息
		UINT uDocType=2834535456;	//2834535456为一随机数表示信狐公司 授权申请文件
		fwrite(&uDocType,4,1,fp);	//doc_type, 
		fwrite(&licapplyfile_version,4,1,fp);// 
		//2.存储实质内容信息
		DWORD dwContentsLength=buffer.GetLength();
		fwrite(&dwContentsLength,4,1,fp);
		fwrite(buffer.GetBufferPtr(),buffer.GetLength(),1,fp);
		fclose(fp);
	}
}

//重启XhLicServ
BOOL RestartXhLicServ()
{
	TCHAR m_szServiceName[256]="XhLicServ";
	SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hSCM == NULL)
		return FALSE;
	SC_HANDLE hService = ::OpenService(hSCM, m_szServiceName, SERVICE_START | SERVICE_STOP);
	if (hService == NULL)
	{
		::CloseServiceHandle(hSCM);
		return FALSE;
	}
	SERVICE_STATUS status;
	BOOL stopped=FALSE,started=FALSE;
	stopped=::ControlService(hService, SERVICE_CONTROL_STOP, &status);
	started=::StartServiceA(hService,0,NULL);
	::CloseServiceHandle(hService);
	::CloseServiceHandle(hSCM);
	if (started)
		return TRUE;
	else
		return FALSE;
}

void CLicenseServerActivatePage::OnBnImportAuthorizeFile()
{
	UINT uDogSerial=DogSerialNo();	//获取加密锁序列号
	//写入文件
	CXhChar16 productname="IBOM";
	char fn[MAX_PATH];
	strcpy(fn,productname);
	strcat(fn,CXhChar16("%d",uDogSerial));
	CFileDialog dlg(TRUE,"aut",fn,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,"用户的授权服务文件(*.aut)|*.aut|所有文件(*.*)|*.*||");
	if(dlg.DoModal()!=IDOK)
		return;
	FILE* fp=fopen(dlg.GetPathName(),"rb");
	if(fp==NULL)
	{
		AfxMessageBox("用户主服务器授权文件打开失败!");
		return;
	}
	XHVERSION authfile_version("0.0.0.1");
	//1.读取authentication文件头信息
	UINT uDocType=2834534455;	//2834534455为一随机数表示信狐公司 授权文件
	fread(&uDocType,4,1,fp);	//doc_type, 
	if(uDocType!=2834534455)
	{
		AfxMessageBox("用户主服务器授权文件类型验证失败!");
		return;
	}
	fread(&authfile_version,4,1,fp);// 
	//2.读取实质内容信息
	DWORD dwContentsLength=0,dwParam=0;
	fread(&dwContentsLength,4,1,fp);
	char header_bytes[75]={0};
	CHAR_ARRAY contents(dwContentsLength);
	CBuffer buffer(contents,dwContentsLength);
	fread(contents,dwContentsLength,1,fp);
	fclose(fp);
	BYTE identity[32]={0};
	buffer.Read(identity,32);
	if(!COMPUTER::Verify128Identity(identity))
	{
		AfxMessageBox("当前导入授权文件与当前用户主机指纹信息验证失败!");
		return;
	}
	// 1)提取并加载V2C更新文件
	WORD v2cbuf_len=0,wContentsLen=0;
	buffer.ReadWord(&v2cbuf_len);
	if(v2cbuf_len>0)
	{
		CHAR_ARRAY v2cbytes(v2cbuf_len);
		buffer.Read(v2cbytes,11);
		XHVERSION v2c_version("0.0.0.1");
		memcpy(&v2c_version,v2cbytes,4);
		memcpy(&uDogSerial,v2cbytes+4,4);
		m_iDogType=*(v2cbytes+8);
		memcpy(&v2cbuf_len,v2cbytes+9,2);
		buffer.Read(v2cbytes+11,v2cbuf_len);
		int status=UpdateByV2CInfoData(v2cbytes,11+v2cbuf_len);
		if(status!=1&&status!=-6)
		{	//status==-6表示本次授权不更新加密锁（因当前V2C升级序号与加密狗不匹配）
			AfxMessageBox("加密锁授权信息加载失败!");
			return;
		}
	}
	// 2)提取、生成并注册主授权服务文件
	buffer.ReadWord(&wContentsLen);
	if(wContentsLen>0)
	{
		CXhChar200 svcfile("%s%s.svc",theApp.execute_path,(char*)productname);
		if((fp=fopen(svcfile,"wb"))!=NULL)
		{
			fwrite(buffer.GetCursorBuffer(),wContentsLen,1,fp);
			buffer.Offset(wContentsLen);
			fclose(fp);
			HKEY hKey=NULL,hSoftwareKey=NULL;
			RegOpenKeyEx(HKEY_LOCAL_MACHINE,"Software\\Xerofox\\XhLicServ\\SrvLicenses",0,KEY_WRITE,&hKey);
			if(hKey==NULL)
			{
				DWORD dw;
				RegOpenKeyEx(HKEY_LOCAL_MACHINE,"Software",0,KEY_WRITE,&hSoftwareKey);
				RegCreateKeyEx(hSoftwareKey, "Xerofox\\XhLicServ\\SrvLicenses", 0, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_READ, NULL, &hKey, &dw);
			}
			if(hKey==NULL||RegSetValueEx(hKey,productname,NULL,REG_SZ,(BYTE*)(char*)svcfile,svcfile.Length)!= ERROR_SUCCESS)
				AfxMessageBox("主授权服务文件注册失败");//FALSE;
			if(hKey)
				RegCloseKey(hKey);
			//重启信狐授权服务
			RestartXhLicServ();
		}
		else
			AfxMessageBox(CXhChar50("主授权服务文件{%s}保存失败",(char*)svcfile));
	}
	// 3)提取并生成主证书文件
	buffer.ReadWord(&wContentsLen);
	CXhChar200 licfile("%s%s.lic",theApp.execute_path,(char*)productname);
	if((fp=fopen(licfile,"wb"))!=NULL)
	{
		fwrite(buffer.GetCursorBuffer(),wContentsLen,1,fp);
		fclose(fp);
		CWnd* pParent=GetParent();
		CLicenseAuthorizeDlg* m_pParentDialg=pParent!=NULL?(CLicenseAuthorizeDlg*)pParent->GetParent():NULL;
		if(m_pParentDialg&&!m_pParentDialg->m_bRunning)
			m_pParentDialg->ImportServerPrimaryLicFile(licfile);
	}
	else
		AfxMessageBox(CXhChar50("主证书文件{%s}保存失败",(char*)licfile));
}
