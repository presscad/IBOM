// LicenseAuthorizeDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "IBOM.h"
#include "LicenseAuthorizeDlg.h"
#include "XhLicAgent.h"
#include "XhLdsLm.h"
#include "LicFuncDef.h"


// CLicenseAuthorizeDlg 对话框

IMPLEMENT_DYNAMIC(CLicenseAuthorizeDlg, CDialogEx)

CLicenseAuthorizeDlg::CLicenseAuthorizeDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CLicenseAuthorizeDlg::IDD, pParent)
	, m_iDogType(0)
	, m_dwIpAddress(0)
	, m_sMasterLicenseHostName(_T(""))
	, m_sComputerName(_T(""))
	, m_sLicenseType(_T(""))
{
	m_cLicenseMode=LIC_MODE_CHOICE;
	m_bRunning=false;
	m_pErrLogFile=NULL;
}

CLicenseAuthorizeDlg::~CLicenseAuthorizeDlg()
{
}

void CLicenseAuthorizeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_RDO_DOG_TYPE, m_iDogType);
	DDX_IPAddress(pDX, IDC_IP_PRODUCT_SERVER_IP, m_dwIpAddress);
	DDX_Text(pDX, IDC_E_PRODUCT_SERVER_NAME, m_sMasterLicenseHostName);
	DDX_Text(pDX, IDC_E_CUSTOMER_NAME, m_sComputerName);
	DDV_MaxChars(pDX, m_sComputerName, 50);
	DDX_Text(pDX, IDC_E_AUTHORIZE_TYPE, m_sLicenseType);
	DDX_Control(pDX, IDC_S_ERROR_MSG, m_ctrlErrorMsg);
}


BEGIN_MESSAGE_MAP(CLicenseAuthorizeDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BN_RETURN_TO_SERVER_LICENSE, OnBnReturnToServerLicense)
	ON_BN_CLICKED(IDC_BN_DOG_TEST, &CLicenseAuthorizeDlg::OnBnDogTest)
	ON_BN_CLICKED(IDC_BN_APPLY_FOR_LAN_LICENSE, &CLicenseAuthorizeDlg::OnBnApplyForLanLicense)
	ON_BN_CLICKED(IDOK, &CLicenseAuthorizeDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_RDO_DOG_TYPE,  &CLicenseAuthorizeDlg::OnRdoDogType)
	ON_BN_CLICKED(IDC_RDO_DOG_TYPE2, &CLicenseAuthorizeDlg::OnRdoDogType)
	ON_BN_CLICKED(IDC_RDO_DOG_TYPE3, &CLicenseAuthorizeDlg::OnRdoDogType)
END_MESSAGE_MAP()


INT_PTR CLicenseAuthorizeDlg::DoModal(bool bAutoRenew/*=false*/)
{
	if(bAutoRenew&&RenewLanLicense()==0)
	{
		m_bLicfileImported=true;
		return IDOK;
	}
	return CDialogEx::DoModal();
}
// CLicenseAuthorizeDlg 消息处理程序
void CLicenseAuthorizeDlg::ShowLanLicApplyPage(int nCmdShow)
{
	CWnd* pWnd=GetDlgItem(IDC_E_PRODUCT_SERVER_NAME);
	if(pWnd)
		pWnd->ShowWindow(nCmdShow);
	pWnd=GetDlgItem(IDC_IP_PRODUCT_SERVER_IP);
	if(pWnd)
		pWnd->ShowWindow(nCmdShow);
	pWnd=GetDlgItem(IDC_S_PRODUCT_SERVER_IP);
	if(pWnd)
		pWnd->ShowWindow(nCmdShow);
	pWnd=GetDlgItem(IDC_S_PRODUCT_SERVER_NAME);
	if(pWnd)
		pWnd->ShowWindow(nCmdShow);
	pWnd=GetDlgItem(IDC_BN_APPLY_FOR_LAN_LICENSE);
	if(pWnd)
		pWnd->ShowWindow(nCmdShow);
	pWnd=GetDlgItem(IDC_BN_RETURN_TO_SERVER_LICENSE);
	if(pWnd)
		pWnd->ShowWindow(nCmdShow);
	InvalidateRect(&workpanelRc);
}
static DWORD MakeIpAddr(BYTE b1,BYTE b2,BYTE b3,BYTE b4)
{
	return (DWORD)(((DWORD)(b1)<<24)+((DWORD)(b2)<<16)+((DWORD)(b3)<<8)+((DWORD)(b4)));
}
static DWORD MakeIpAddr(BYTE* bytes)
{
	return MakeIpAddr(bytes[0],bytes[1],bytes[2],bytes[3]);
}
BOOL CLicenseAuthorizeDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	licServerPage.Create(IDD_LICENSE_SERVER_ACTIVATE_DLG,GetDlgItem(IDC_S_APPLY_LICENSE_PANEL));

	GetDlgItem(IDC_S_APPLY_LICENSE_PANEL)->GetWindowRect(&workpanelRc);//获取picture  control控件的区域
	ScreenToClient(workpanelRc);


	m_dwIpAddress=theApp.GetProfileInt("Settings","master_host_ip",0);
	m_sMasterLicenseHostName=theApp.GetProfileString("Settings","master_host_name","");

	if(m_cLicenseMode==LIC_MODE_CLIENT)
	{
		ShowLanLicApplyPage(SW_SHOW);
		licServerPage.ShowWindow(false);
		GetDlgItem(IDC_BN_RETURN_TO_SERVER_LICENSE)->ShowWindow(SW_HIDE);
	}
	else
	{
		licServerPage.ShowWindow(true);
		ShowLanLicApplyPage(SW_HIDE);
		if(LIC_MODE_SERVER==m_cLicenseMode)
			licServerPage.GetDlgItem(IDC_BN_ACTIVATE_END_LICENSE)->ShowWindow(SW_HIDE);
		else
			licServerPage.GetDlgItem(IDC_BN_ACTIVATE_END_LICENSE)->ShowWindow(SW_SHOW);
	}
	RECT rc;//=workpanelRc;
	GetDlgItem(IDC_S_APPLY_LICENSE_PANEL)->GetClientRect(&rc);
	rc.top+=2;
	rc.left+=1;
	rc.right-=1;
	rc.bottom-=1;
	licServerPage.MoveWindow(&rc);

	COMPUTER computer;
	m_sComputerName=computer.sComputerName;
	/*BYTE ipbytes[4];
	computer.GetIP(ipbytes,m_sComputerName);
	m_dwIpAddress=MakeIpAddr(ipbytes)*/
	m_iDogType=AgentDogType();
	BYTE authinfo[8];
	GetProductAuthorizeInfo(authinfo);
	if(authinfo[7]&0x01)
		m_sLicenseType+="永久授权";
	else if(authinfo[7]&0x02)
		m_sLicenseType+="期间授权";
	else
		m_sLicenseType="";
	UINT dwDogSerial=DogSerialNo();
	if(m_bRunning)
	{
		m_iDogType=AgentDogType();
		GetDlgItem(IDC_RDO_DOG_TYPE)->EnableWindow(FALSE);
		GetDlgItem(IDC_RDO_DOG_TYPE2)->EnableWindow(FALSE);
		GetDlgItem(IDC_RDO_DOG_TYPE3)->EnableWindow(FALSE);
	}
	licServerPage.m_iDogType=m_iDogType;
	GetDlgItem(IDC_S_HARDLOCK_SERIAL)->SetWindowText(CXhChar16("%d",dwDogSerial));
	UpdateData(FALSE);
	m_ctrlErrorMsg.EnableURL(FALSE);
	m_ctrlErrorMsg.EnableHover(FALSE);
	m_ctrlErrorMsg.SetColours(RGB(255,0,0),RGB(255,0,0),RGB(255,0,0));
	m_ctrlErrorMsg.SetWindowText(m_sErrorMsg);
	m_ctrlErrorMsg.SetURL(m_sErrorMsg);	//提示信息
	m_bLicfileImported=false;
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CLicenseAuthorizeDlg::OnBnDogTest()
{
	if(!FindDog((BYTE)m_iDogType))
		AfxMessageBox("未找到加密锁, 请核实是否开启了防火墙或与插狗计算机存在通信故障或局域网内安装了多个网络狗服务!如确认防火墙未开,且局域网连接正常可以通过设定gsnetdog.ini(指定插有加密锁计算机的ip地址)直接访问!");
	else if(m_iDogType==0&&!ConnectDog((BYTE)m_iDogType))
		AfxMessageBox("加密锁连接失败!");
	else
	{
		UINT dwDogSerial=DogSerialNo();
		GetDlgItem(IDC_S_HARDLOCK_SERIAL)->SetWindowText(CXhChar16("%d",dwDogSerial));
		MessageBox(CXhChar50("找到加密锁%d, 并成功进行了连接测试!\n",dwDogSerial));
	}
}

void CLicenseAuthorizeDlg::OnBnReturnToServerLicense()
{
	ShowLanLicApplyPage(SW_HIDE);
	licServerPage.ShowWindow(true);
	InvalidateRect(&workpanelRc);
}

class NAMEDPIPE_LIFE{
	BYTE ciOpenMode,ciErrorStatus;
	DWORD dwErrorCode;
	HANDLE hPipe;
	SECURITY_ATTRIBUTES sa;
	void InitSecurityAttributes()
	{
		PSECURITY_DESCRIPTOR    pSD;
		// create a security NULL security
		// descriptor, one that allows anyone
		// to write to the pipe... WARNING
		// entering NULL as the last attribute
		// of the CreateNamedPipe() will
		// indicate that you wish all
		// clients connecting to it to have
		// all of the same security attributes
		// as the user that started the
		// pipe server.

		pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR,
			SECURITY_DESCRIPTOR_MIN_LENGTH);
		if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
		{
			LocalFree((HLOCAL)pSD);
			return;
		}
		// add a NULL disc. ACL to the
		// security descriptor.
		if (!SetSecurityDescriptorDacl(pSD, TRUE, (PACL) NULL, FALSE))
		{
			LocalFree((HLOCAL)pSD);
			return;
		}

		sa.nLength = sizeof(sa);
		sa.lpSecurityDescriptor = pSD;
		sa.bInheritHandle = TRUE;
	}
public:
	NAMEDPIPE_LIFE(HANDLE hNamedPipe,BYTE SERVER0_CLIENT1=0){
		dwErrorCode=0;
		ciErrorStatus=0;
		sa.lpSecurityDescriptor=NULL;
		ciOpenMode=SERVER0_CLIENT1;
		if(ciOpenMode==0&&ConnectNamedPipe(hNamedPipe,NULL))
			hPipe=hNamedPipe;
		else
		{
			hPipe=NULL;
			ciErrorStatus=4;
			dwErrorCode=GetLastError();
		}
	}
	NAMEDPIPE_LIFE(const char* pipeName,BYTE SERVER0_CLIENT1=0){
		ciErrorStatus=0;
		dwErrorCode=0;
		sa.lpSecurityDescriptor=NULL;
		if((ciOpenMode=SERVER0_CLIENT1)==0)
			InitSecurityAttributes();
		hPipe=NULL;
		if(ciOpenMode==0)
			hPipe=CreateNamedPipe(pipeName,PIPE_ACCESS_DUPLEX,PIPE_TYPE_BYTE|PIPE_READMODE_BYTE,32,0,0,2000,&sa);
		else if(ciOpenMode==1)
		{
			if(!WaitNamedPipe(pipeName,NMPWAIT_USE_DEFAULT_WAIT))
			{
				int iError=GetLastError();
				if(iError>=1326||iError<=1331)
					ciErrorStatus=1;	//登录主机失败
				else //if(iError==53)	
					ciErrorStatus=2;	//主机无响应
			}
			else if((hPipe=CreateFile(pipeName,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,(HANDLE)NULL))==INVALID_HANDLE_VALUE)
				ciErrorStatus=3;		//管道创建失败
			if(ciErrorStatus>0)
				dwErrorCode=GetLastError();
		}
	}
	HANDLE PipeHandle(){return hPipe;}
	~NAMEDPIPE_LIFE(){
		if(ciOpenMode==0&&sa.lpSecurityDescriptor)
			LocalFree((HLOCAL)sa.lpSecurityDescriptor);
		if(hPipe)
		{
			if(ciOpenMode==0)
				DisconnectNamedPipe(hPipe);
			CloseHandle(hPipe);
		}
	}
	CXhChar200 GetErrorStr()
	{
		CXhChar200 sError;
		if(dwErrorCode==2)
		{
			sError.Copy("系统找不到指定的文件");
			if(ciOpenMode==1)
				sError.Append(". 请核实主授权服务是否已启动，错误代码#2!");
		}
		else if(dwErrorCode==53)
		{
			sError.Copy("找不到网络路径");
			if(ciOpenMode==1)
				sError.Append(". 请核实主授权服务器名称及IP是否正确，错误代码#53!");
		}
		else if(dwErrorCode==71)
			sError.Copy("已达到XP计算机的连接数最大值，无法再同此远程计算机连接。 ");
		else if(dwErrorCode==1326)
			sError.Copy("登录失败: 未知的用户名或错误密码。确认是否可以正常访问授权服务器。");
		else if(ciErrorStatus==1)
			sError.Printf("登录服务器失败，错误代码#%d！",dwErrorCode);
		else if(ciErrorStatus==2)
			sError.Printf("服务器无响应，错误代码#%d！",dwErrorCode);
		else if(ciErrorStatus==3)
			sError.Printf("管道创建失败，错误代码#%d！",dwErrorCode);
		else if(ciErrorStatus==4)
			sError.Printf("连接管道失败，错误代码#%d！",dwErrorCode);
		return sError;
	}
};
bool CLicenseAuthorizeDlg::_ImportLicFile(char* licfile)
{
	DWORD version[2];
	BYTE* byteVer=(BYTE*)version;
	WORD wModule=0;
	version[1]=20161108;	//版本发布日期
	BYTE cProductType=PRODUCT_IBOM;
#ifdef __TMA_
#ifdef _LOCAL_ENCRYPT
	cProductType=PRODUCT_CTMA;
#else
	cProductType=PRODUCT_TMA;
#endif
	byteVer[0]=5;
	byteVer[1]=1;
	byteVer[2]=4;
	byteVer[3]=0;
#elif defined(__LMA_)
	//sprintf(lic_file,"%sLMA.lic",APP_PATH);
	wModule=1;
	byteVer[0]=2;
	byteVer[1]=0;
	byteVer[2]=6;
	byteVer[3]=0;
	cProductType=PRODUCT_LMA;
#elif defined(__TSA_)
	//sprintf(lic_file,"%sTSA.lic",APP_PATH);
	wModule=2;
	byteVer[0]=1;
	byteVer[1]=8;
	byteVer[2]=1;
	byteVer[3]=2;
	cProductType=PRODUCT_TSA;
#elif defined(__LDS_)
	//sprintf(lic_file,"%sLDS.lic",APP_PATH);
	wModule=3;
	byteVer[0]=1;
	byteVer[1]=3;
	byteVer[2]=6;
	byteVer[3]=0;
	cProductType=PRODUCT_LDS;
#elif defined(__TDA_)
	//sprintf(lic_file,"%sTDA.lic",APP_PATH);
	wModule=3;
	byteVer[0]=1;
	byteVer[1]=3;
	byteVer[2]=6;
	byteVer[3]=0;
	cProductType=PRODUCT_TDA;
#elif defined(__IBOM_)
	//sprintf(lic_file,"%IBOM.lic",APP_PATH);
	wModule=3;
	byteVer[0]=1;
	byteVer[1]=0;
	byteVer[2]=0;
	byteVer[3]=1;
	cProductType=PRODUCT_IBOM;
#endif
	DWORD retCode=ImportLicFile(licfile,PRODUCT_IBOM,version);	
	CXhChar50 errormsgstr;
	m_bLicfileImported=false;
	if(retCode==0)
	{
		if(GetLicVersion()<1000004)
#ifdef AFX_TARG_ENU_ENGLISH
			errormsgstr.Copy("The certificate file has been out of date, the current software's version must use the new certificate file！");
#else 
			errormsgstr.Copy("该证书文件已过时，当前软件版本必须使用新证书！");
#endif
		else if(!VerifyValidFunction(LICFUNC::FUNC_IDENTITY_BASIC))
#ifdef AFX_TARG_ENU_ENGLISH
			errormsgstr.Copy("Software Lacks of legitimate use authorized!");
#else 
			errormsgstr.Copy("软件缺少合法使用授权!");
#endif
		else
		{
			m_bLicfileImported=true;
			WriteProfileString("Settings","lic_file",licfile);
		}
		if(!m_bLicfileImported)
			ExitCurrentDogSession();
	}
	else
	{
#ifdef AFX_TARG_ENU_ENGLISH
		if(retCode==-1)
			errormsgstr.Copy("0# Hard dog failed to initialize!");
		else if(retCode==1)
			errormsgstr.Copy("1# Unable to open the license file!");
		else if(retCode==2)
			errormsgstr.Copy("2# License file was damaged!");
		else if(retCode==3||retCode==4)
			errormsgstr.Copy("3# License file not found or does'nt match the hard lock!");
		else if(retCode==5)
			errormsgstr.Copy("5# License file doesn't match the authorized products in hard lock!");
		else if(retCode==6)
			errormsgstr.Copy("6# Beyond the scope of authorized version !");
		else if(retCode==7)
			errormsgstr.Copy("7# Beyond the scope of free authorize version !");
		else if(retCode==8)
			errormsgstr.Copy("8# The serial number of license file does'nt match the hard lock!");
#else 
		if(retCode==-1)
			errormsgstr.Copy("0#加密狗初始化失败!");
		else if(retCode==1)
			errormsgstr.Copy("1#无法打开证书文件");
		else if(retCode==2)
			errormsgstr.Copy("2#证书文件遭到破坏");
		else if(retCode==3||retCode==4)
			errormsgstr.Copy("3#执行目录下不存在证书文件或证书文件与加密狗不匹配");
		else if(retCode==5)
			errormsgstr.Copy("5#证书与加密狗产品授权版本不匹配");
		else if(retCode==6)
			errormsgstr.Copy("6#超出版本使用授权范围");
		else if(retCode==7)
			errormsgstr.Copy("7#超出免费版本升级授权范围");
		else if(retCode==8)
			errormsgstr.Copy("8#证书序号与当前加密狗不一致");
#endif
		ExitCurrentDogSession();
		m_bLicfileImported=false;
	}
	if(!m_bLicfileImported)
	{
		m_ctrlErrorMsg.SetWindowText(errormsgstr);
		m_ctrlErrorMsg.SetURL((char*)errormsgstr);	//提示信息
	}
	return m_bLicfileImported;
}
void CLicenseAuthorizeDlg::ImportServerPrimaryLicFile(char* licfile)
{
	if(_ImportLicFile(licfile))
		OnBnClickedOk();
}
void CLicenseAuthorizeDlg::OnBnApplyForLanLicense()
{
	CWaitCursor waitCursor;
	UpdateData();
	theApp.WriteProfileInt("Settings","master_host_ip",m_dwIpAddress);
	theApp.WriteProfileString("Settings","master_host_name",m_sMasterLicenseHostName);
	CXhChar200 srvname("%s",m_sMasterLicenseHostName);
	BYTE* ipbytes=(BYTE*)&m_dwIpAddress;
	if(srvname.Length==0&&m_dwIpAddress!=0)
		srvname.Printf("%d.%d.%d.%d",ipbytes[3],ipbytes[2],ipbytes[1],ipbytes[0]);
	CXhChar200 corePipeName("\\\\%s\\Pipe\\XhLicServ\\CorePipe",(char*)srvname);
	CXhChar200 readPipeName("\\\\%s\\Pipe\\XhLicServ\\%s_ListenPipe",(char*)srvname,m_sComputerName);
	NAMEDPIPE_LIFE pipeWrite(corePipeName,1);
	CXhChar200 sError=pipeWrite.GetErrorStr();
	if(sError.Length>0)
	{
		sError.Append(CXhChar100(". 核心通信管道{%s}连接失败!",(char*)corePipeName));
		AfxMessageBox(sError);
		return;
	}
	CXhChar16 szSubKey="IBOM";
	CXhChar200 errfile("%s%s.err",theApp.execute_path,(char*)szSubKey);
	CTempFileBuffer error(errfile);
	error.EnableAutoDelete(false);
	m_pErrLogFile=&error;
	LICSERV_MSGBUF cmdmsg;
	cmdmsg.command_id=LICSERV_MSGBUF::APPLY_FOR_LICENSE;
	cmdmsg.src_code=0;
	CBuffer buffer;
	COMPUTER computer;
	BYTE identity[32],details[512]={0};
	computer.Generate128Identity(identity,NULL,details,512);
	//COMPUTER::Verify128Identity(identity);
	WORD wDetailsLen=0;
	for(WORD i=0;i<512;i+=16)
	{
		if(((__int64*)(details+i))[0]==0&&((__int64*)(details+i))[1]==0)
			break;
	}
	wDetailsLen=i;
	CXhChar16 ipstr("%d.%d.%d.%d",computer.computer_ip[0],computer.computer_ip[1],computer.computer_ip[2],computer.computer_ip[3]);
	//ReadFile(hCorePipe,msg_contents,msg.msg_length-32,&nBytesRead,NULL);
	//char status,cApplyProductType=0;
	DWORD nBytesRead,nBytesWritten;
	CXhChar50 hostname,hostipstr;
	buffer.WriteString(ipstr,17);
	buffer.WriteString(m_sComputerName,51);
	buffer.WriteWord(wDetailsLen);
	buffer.Write(details,wDetailsLen);
	buffer.WriteByte(PRODUCT_IBOM);
	buffer.WriteDword((DWORD)time(NULL));
	buffer.Write(identity,32);
	cmdmsg.msg_length=buffer.GetLength();
	error.WriteInteger(buffer.GetLength());
	error.Write(buffer.GetBufferPtr(),buffer.GetLength());
	if(!WriteFile(pipeWrite.PipeHandle(),&cmdmsg,9,&nBytesWritten,NULL))
	{
		error.WriteBooleanThin(false);
		return;
	}
	else
		error.WriteBooleanThin(true);
	if(!WriteFile(pipeWrite.PipeHandle(),buffer.GetBufferPtr(),buffer.GetLength(),&nBytesWritten,NULL))
	{
		error.WriteBooleanThin(false);
		return;
	}
	else
		error.WriteBooleanThin(true);
	char state,status=-100;
	DWORD dwRecvBytesLen=0;
	NAMEDPIPE_LIFE pipeRead(readPipeName,1);
	if(!(state=ReadFile(pipeRead.PipeHandle(),&status,1,&nBytesRead,NULL))||status<=0)
	{

		if(pipeRead.PipeHandle()==NULL)
		{
			sError=pipeRead.GetErrorStr();
			sError.Append(". 实例通信管道连接失败!");
			AfxMessageBox(sError);
		}
		error.WriteByte(status);
		if(status==0)
			AfxMessageBox("子证书生成失败，请查看授权主服务器下c:\\service.log日志以排除故障");
		else if(status==-1)
			AfxMessageBox("当前主机与授权主服务器时钟存在过大偏差，请校准时钟后再申请授权");
		else if(status==-2)
			AfxMessageBox("主服务器缺少对相应产品的主授权服务文件");
		else if(status==-3)
			AfxMessageBox("主服务器对相应产品的主授权服务子授权数量已满");
		return;
	}
	else
		error.WriteByte(status);
	if(!ReadFile(pipeRead.PipeHandle(),&dwRecvBytesLen,4,&nBytesRead,NULL))
	{
		error.WriteInteger(0);
		return;
	}
	else
		error.WriteInteger(nBytesRead);
	BYTE_ARRAY licbuf_bytes(dwRecvBytesLen);
	if(!ReadFile(pipeRead.PipeHandle(),licbuf_bytes,dwRecvBytesLen,&nBytesRead,NULL))
	{
		error.WriteBooleanThin(false);
		return;
	}
	else
		error.WriteBooleanThin(true);
	//根据解析数据生成本地证书的注册表部分及子证书文件部分
	//1.写入724个注册表初始播放的种子
	if(!theApp.WriteProfileBinary("",_T("Obfuscator"),(BYTE*)licbuf_bytes,724))
	{
		error.WriteBooleanThin(false);
		error.Write(licbuf_bytes,724);
		return;
	}
	else
		error.WriteBooleanThin(true);
	CXhChar200 licfile("%s%s.lic",theApp.execute_path,(char*)szSubKey);
	error.WriteString(licfile);
	FILE* fp=fopen(licfile,"wb");
	if(fp==NULL)
		error.WriteBooleanThin(false);
	else
	{
		error.WriteBooleanThin(true);
		fwrite(licbuf_bytes+724,dwRecvBytesLen-724,1,fp);
		fclose(fp);
	}
	WIN32_FIND_DATA FindFileData;
	if(FindFirstFileA(licfile,&FindFileData)==NULL)
		AfxMessageBox(CXhChar200("'%s'证书文件未找到",(char*)licfile));
	else
		MessageBox(CXhChar200("'%s'证书文件已生成",(char*)licfile));
	if(_ImportLicFile(licfile))
	{
		error.EnableAutoDelete(true);
		OnBnClickedOk();
	}
}

int CLicenseAuthorizeDlg::RenewLanLicense()	//续借局域网授权
{
	CWaitCursor waitCursor;
	DWORD dwIpAddress=theApp.GetProfileInt("Settings","master_host_ip",0);
	CString masterLicenseHostName=theApp.GetProfileString("Settings","master_host_name","");
	CXhChar200 srvname("%s",masterLicenseHostName);
	BYTE* ipbytes=(BYTE*)&dwIpAddress;
	if(srvname.Length==0&&dwIpAddress!=0)
		srvname.Printf("%d.%d.%d.%d",ipbytes[3],ipbytes[2],ipbytes[1],ipbytes[0]);
	COMPUTER computer;
	CXhChar200 corePipeName("\\\\%s\\Pipe\\XhLicServ\\CorePipe",(char*)srvname);
	CXhChar200 readPipeName("\\\\%s\\Pipe\\XhLicServ\\%s_ListenPipe",(char*)srvname,computer.sComputerName);
	NAMEDPIPE_LIFE pipeWrite(corePipeName,1);
	CXhChar200 sError=pipeWrite.GetErrorStr();
	if(sError.Length>0)
	{
		//sError.Append(CXhChar100(". 核心通信管道{%s}连接失败!",(char*)corePipeName));
		//AfxMessageBox(sError);
		return -2;
	}
	CXhChar16 szSubKey="product";
	DWORD version[2];
	BYTE* byteVer=(BYTE*)version;
	version[1]=20160808;	//版本发布日期
	BYTE cProductType=PRODUCT_TMA;
#ifdef __TMA_
#ifdef _LOCAL_ENCRYPT
	cProductType=PRODUCT_CTMA;
#else
	cProductType=PRODUCT_TMA;
#endif
	byteVer[0]=5;
	byteVer[1]=1;
	byteVer[2]=3;
	byteVer[3]=0;
	szSubKey.Copy("TMA");
#elif defined(__LMA_)
	//sprintf(lic_file,"%sLMA.lic",APP_PATH);
	byteVer[0]=2;
	byteVer[1]=0;
	byteVer[2]=6;
	byteVer[3]=0;
	cProductType=PRODUCT_LMA;
	szSubKey.Copy("LMA");
#elif defined(__TSA_)
	//sprintf(lic_file,"%sTSA.lic",APP_PATH);
	byteVer[0]=1;
	byteVer[1]=8;
	byteVer[2]=1;
	byteVer[3]=2;
	cProductType=PRODUCT_TSA;
	szSubKey.Copy("TSA");
#elif defined(__LDS_)
	//sprintf(lic_file,"%sLDS.lic",APP_PATH);
	byteVer[0]=1;
	byteVer[1]=3;
	byteVer[2]=6;
	byteVer[3]=0;
	cProductType=PRODUCT_LDS;
	szSubKey.Copy("LDS");
#elif defined(__TDA_)
	//sprintf(lic_file,"%sTDA.lic",APP_PATH);
	byteVer[0]=1;
	byteVer[1]=3;
	byteVer[2]=6;
	byteVer[3]=0;
	cProductType=PRODUCT_TDA;
	szSubKey.Copy("TDA");
#else
	AfxMessageBox(CXhChar50("目前暂不支持的产品授权#%d",PRODUCT_IBOM));
#endif
	CXhChar200 errfile("%s%s.err",theApp.execute_path,(char*)szSubKey);
	CTempFileBuffer error(errfile);
	error.EnableAutoDelete(false);
	//m_pErrLogFile=&error;
	LICSERV_MSGBUF cmdmsg;
	cmdmsg.command_id=LICSERV_MSGBUF::APPLY_FOR_LICENSE;
	cmdmsg.src_code=0;
	CBuffer buffer;
	BYTE identity[32],details[512]={0};
	computer.Generate128Identity(identity,NULL,details,512);
	//COMPUTER::Verify128Identity(identity);
	WORD wDetailsLen=0;
	for(WORD i=0;i<512;i+=16)
	{
		if(((__int64*)(details+i))[0]==0&&((__int64*)(details+i))[1]==0)
			break;
	}
	wDetailsLen=i;
	CXhChar16 ipstr("%d.%d.%d.%d",computer.computer_ip[0],computer.computer_ip[1],computer.computer_ip[2],computer.computer_ip[3]);
	//ReadFile(hCorePipe,msg_contents,msg.msg_length-32,&nBytesRead,NULL);
	//char status,cApplyProductType=0;
	DWORD nBytesRead,nBytesWritten;
	CXhChar50 hostname,hostipstr;
	buffer.WriteString(ipstr,17);
	buffer.WriteString(computer.sComputerName,51);
	buffer.WriteWord(wDetailsLen);
	buffer.Write(details,wDetailsLen);
	buffer.WriteByte(PRODUCT_IBOM);
	buffer.WriteDword((DWORD)time(NULL));
	buffer.Write(identity,32);
	cmdmsg.msg_length=buffer.GetLength();
	error.WriteInteger(buffer.GetLength());
	error.Write(buffer.GetBufferPtr(),buffer.GetLength());
	if(!WriteFile(pipeWrite.PipeHandle(),&cmdmsg,9,&nBytesWritten,NULL))
	{
		error.WriteBooleanThin(false);
		return -2;
	}
	else
		error.WriteBooleanThin(true);
	if(!WriteFile(pipeWrite.PipeHandle(),buffer.GetBufferPtr(),buffer.GetLength(),&nBytesWritten,NULL))
	{
		error.WriteBooleanThin(false);
		return -2;
	}
	else
		error.WriteBooleanThin(true);
	char state,status=-100;
	DWORD dwRecvBytesLen=0;
	NAMEDPIPE_LIFE pipeRead(readPipeName,1);
	if(!(state=ReadFile(pipeRead.PipeHandle(),&status,1,&nBytesRead,NULL))||status<=0)
	{

		if(pipeRead.PipeHandle()==NULL)
		{
			sError=pipeRead.GetErrorStr();
			sError.Append(". 实例通信管道连接失败!");
			AfxMessageBox(sError);
		}
		error.WriteByte(status);
		if(status==0)
			AfxMessageBox("子证书生成失败，请查看授权主服务器下c:\\service.log日志以排除故障");
		else if(status==-1)
			AfxMessageBox("当前主机与授权主服务器时钟存在过大偏差，请校准时钟后再申请授权");
		else if(status==-2)
			AfxMessageBox("主服务器缺少对相应产品的主授权服务文件");
		else if(status==-3)
			AfxMessageBox("主服务器对相应产品的主授权服务子授权数量已满");
		return -2;
	}
	else
		error.WriteByte(status);
	if(!ReadFile(pipeRead.PipeHandle(),&dwRecvBytesLen,4,&nBytesRead,NULL))
	{
		error.WriteInteger(0);
		return -2;
	}
	else
		error.WriteInteger(nBytesRead);
	BYTE_ARRAY licbuf_bytes(dwRecvBytesLen);
	if(!ReadFile(pipeRead.PipeHandle(),licbuf_bytes,dwRecvBytesLen,&nBytesRead,NULL))
	{
		error.WriteBooleanThin(false);
		return -2;
	}
	else
		error.WriteBooleanThin(true);
	//根据解析数据生成本地证书的注册表部分及子证书文件部分
	//1.写入724个注册表初始播放的种子
	if(!theApp.WriteProfileBinary("",_T("Obfuscator"),(BYTE*)licbuf_bytes,724))
	{
		error.WriteBooleanThin(false);
		error.Write(licbuf_bytes,724);
		return -2;
	}
	else
		error.WriteBooleanThin(true);
	CXhChar200 licfile("%s%s.lic",theApp.execute_path,(char*)szSubKey);
	error.WriteString(licfile);
	FILE* fp=fopen(licfile,"wb");
	if(fp==NULL)
		error.WriteBooleanThin(false);
	else
	{
		error.WriteBooleanThin(true);
		fwrite(licbuf_bytes+724,dwRecvBytesLen-724,1,fp);
		fclose(fp);
	}
	WIN32_FIND_DATA FindFileData;
	if(FindFirstFileA(licfile,&FindFileData)==NULL)
		AfxMessageBox(CXhChar200("'%s'证书文件未找到",(char*)licfile));
	int retCode=ImportLicFile(licfile,cProductType,version);
	if(retCode==0&&(GetLicVersion()<1000004||!VerifyValidFunction(LICFUNC::FUNC_IDENTITY_BASIC)))
		retCode=9;	//通过局域网续借授权失败
	if(retCode==0)
		error.EnableAutoDelete(true);
	else
		ExitCurrentDogSession();
	return retCode;
}

void CLicenseAuthorizeDlg::OnBnClickedOk()
{
	UpdateData();
	theApp.WriteProfileInt("Settings","master_host_ip",m_dwIpAddress);
	theApp.WriteProfileString("Settings","master_host_name",m_sMasterLicenseHostName);
	CDialogEx::OnOK();
}


void CLicenseAuthorizeDlg::OnRdoDogType()
{
	UpdateData();
	licServerPage.m_iDogType=m_iDogType;
}

void CLicenseAuthorizeDlg::InitLicenseModeByLicFile(const char* lic_file)
{
	m_cLicenseMode=LIC_MODE_CHOICE;
	CString sLicFile;
	if(lic_file==NULL)
		sLicFile=theApp.GetProfileString("Settings","lic_file");
	else
		sLicFile=lic_file;
	FILE* fp=fopen(sLicFile,"rb");
	if(fp==NULL)
		return;
	DWORD dwLicVersion=0;
	fread(&dwLicVersion,4,1,fp);
	if(dwLicVersion>=1000004)
	{
		fseek(fp,3,SEEK_CUR);	//1Byte cDogType+2Byte wModule
		BYTE ciLicenseType=0;
		fread(&ciLicenseType,1,1,fp);
		if(ciLicenseType==1)
			m_cLicenseMode=LIC_MODE_SERVER;
		else if(ciLicenseType==2)	//网络版动态子证书文件
			m_cLicenseMode=LIC_MODE_CLIENT;
	}
	fclose(fp);
}
