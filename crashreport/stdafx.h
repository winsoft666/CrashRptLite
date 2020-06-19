#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif
#include <SDKDDKVer.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
#define _AFX_ALL_WARNINGS
#include <afxwin.h>
#include <afxext.h>
#include <afxdisp.h>
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h> 
#endif               // _AFX_NO_AFXCMN_SUPPORT


#include <errno.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <iostream>
#include <fstream>

#include <dbghelp.h>
#include <mapi.h>          // MAPI function defs
#include <sys/stat.h>
#include <shellapi.h>
#include <time.h>
#include <Psapi.h>
#include <tlhelp32.h>

#if _MSC_VER<1400
#define _TCSCPY_S(strDestination, numberOfElements, strSource) _tcscpy(strDestination, strSource)
#define _TCSNCPY_S(strDest, sizeInBytes, strSource, count) _tcsncpy(strDest, strSource, count)
#define STRCPY_S(strDestination, numberOfElements, strSource) strcpy(strDestination, strSource)
#define _TFOPEN_S(_File, _Filename, _Mode) _File = _tfopen(_Filename, _Mode);
#else
#define _TCSCPY_S(strDestination, numberOfElements, strSource) _tcscpy_s(strDestination, numberOfElements, strSource)
#define _TCSNCPY_S(strDest, sizeInBytes, strSource, count) _tcsncpy_s(strDest, sizeInBytes, strSource, count)
#define STRCPY_S(strDestination, numberOfElements, strSource) strcpy_s(strDestination, numberOfElements, strSource)
#define _TFOPEN_S(_File, _Filename, _Mode) _tfopen_s(&(_File), _Filename, _Mode);
#endif
