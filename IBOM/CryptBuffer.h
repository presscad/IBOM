#ifndef __CRYPT_BUFFER_HEADER_FILE_
#define __CRYPT_BUFFER_HEADER_FILE_

#include "Buffer.h"

BOOL EncryptBuffer(CBuffer &buffer,char ciEncryptMode=false);
BOOL DecryptBuffer(CBuffer &buffer,char ciEncryptMode=false,DWORD exter_keyid=0);
#endif