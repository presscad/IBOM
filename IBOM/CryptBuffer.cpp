#include "stdafx.h"
#include "Buffer.h"
#ifndef _LEGACY_LICENSE
#include "XhLicAgent.h"
#else
#include "Lic.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOL EncryptBuffer(CBuffer &buffer,char ciEncryptMode/*=false*/)
{
	DWORD dwOutLen=EncryptFileEx(buffer.GetBufferPtr(),buffer.GetLength(),NULL,ciEncryptMode);
	char *buffer_out=new char[dwOutLen];
	EncryptFileEx(buffer.GetBufferPtr(),buffer.GetLength(),buffer_out,ciEncryptMode);
	buffer.SeekToBegin();
	buffer.Write(buffer_out,dwOutLen);
	delete []buffer_out;
	return TRUE;
}
BOOL DecryptBuffer(CBuffer &buffer,char ciEncryptMode/*=false*/,DWORD exter_keyid)
{
	DWORD dwOutLen=DecryptFileEx(buffer.GetBufferPtr(),buffer.GetLength(),NULL,ciEncryptMode,exter_keyid);
	char *buffer_out=new char[dwOutLen];
	DecryptFileEx(buffer.GetBufferPtr(),buffer.GetLength(),buffer_out,ciEncryptMode,exter_keyid);
	buffer.SeekToBegin();
	buffer.Write(buffer_out,dwOutLen);
	delete []buffer_out;
	return TRUE;
}
