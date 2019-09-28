#pragma once

// CLicenseServerActivatePage �Ի���
class CLicenseServerActivatePage : public CDialogEx
{
	DECLARE_DYNAMIC(CLicenseServerActivatePage)
public:
	CLicenseServerActivatePage(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CLicenseServerActivatePage();

// �Ի�������
	enum { IDD = IDD_LICENSE_SERVER_ACTIVATE_DLG };
	int m_iDogType;
	DWORD m_dwLicenseApplyTimeStamp;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnActivateEndLicense();
	afx_msg void OnBnActivateServerLicense();
	afx_msg void OnBnCreateIdentityFile();
	afx_msg void OnBnImportAuthorizeFile();
};
template<class TYPE> class DATA_PTR{
	int  m_iCurrElem;
	TYPE* m_data;
public:
	DATA_PTR(BYTE* data,int iElem=0)
	{
		m_iCurrElem=iElem;
		m_data=(TYPE*)data;
	}
	DATA_PTR(char* data,int iElem=0)
	{
		m_iCurrElem=iElem;
		m_data=(TYPE*)data;
	}
	operator TYPE&(){return m_data[m_iCurrElem];}
	TYPE& operator =(const TYPE& v){return m_data[m_iCurrElem]=v;}
    TYPE& operator[](int i)//���������Ż�ȡ�߶�ʵ��
	{
		return m_data[i];
	}
};

typedef DATA_PTR< DWORD > DWORDPTR;
typedef DATA_PTR<__int64> INT64PTR;
