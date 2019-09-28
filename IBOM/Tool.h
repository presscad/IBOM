#pragma once
#ifndef __TOOL_H_
#define __TOOL_H_
#include "Recog.h"

extern char APP_PATH[MAX_PATH];
void GetAppPath(char* App_Path,const char *src_path=NULL,char *file_name=NULL,char *extension=NULL);
LONG SetRegKey(HKEY key,const char* subkey,const char* sEntry,const char* sValue);
LONG GetRegKey(HKEY key,const char* subkey,const char* sEntry,char* retdata);
BOOL ReadCadPath(char* cad_path);
BOOL ParseSpec(char* spec_str,double* thick,double* width,int* cls_id,int *sub_type=NULL,bool bPartLabelStr=false,char *real_spec=NULL,char* material=NULL);
BOOL ParseSpec(IRecoginizer::BOMPART *pBomPart);
void DisplayProcess(int percent,const char *sTitle=NULL);
#endif