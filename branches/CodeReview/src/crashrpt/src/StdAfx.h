// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__465AD6C5_1ACE_47ED_AD54_7ED140DFF7CC__INCLUDED_)
#define AFX_STDAFX_H__465AD6C5_1ACE_47ED_AD54_7ED140DFF7CC__INCLUDED_

// Change these values to use different versions
#define WINVER		0x0500
#define _WIN32_WINNT	0x0500
#define _WIN32_IE	0x0500
#define _RICHEDIT_VER	0x0100

#include <atlstr.h>
#include <atlbase.h>
#include <atlapp.h>

extern CAppModule _Module;

#include <atlwin.h>

#if _MSC_VER<1400
#define _TCSNCPY_S(strDest, sizeInBytes, strSource, count) _tcsncpy(strDest, strSource, count)
#define STRCPY_S(strDestination, numberOfElements, strSource) strcpy(strDestination, strSource)
#else
#define _TCSNCPY_S(strDest, sizeInBytes, strSource, count) _tcsncpy_s(strDest, sizeInBytes, strSource, count)
#define STRCPY_S(strDestination, numberOfElements, strSource) strcpy_s(strDestination, numberOfElements, strSource)
#endif


//#define CRASHRPTAPI extern "C" __declspec(dllexport)
#pragma warning(disable: 4100)
#define chSTR2(x) #x
#define chSTR(x) chSTR2(x)
#define chMSG(desc) message(__FILE__ "(" chSTR(__LINE__) "):" #desc)
#define todo(desc) message(__FILE__ "(" chSTR(__LINE__) "): TODO: " #desc)
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__465AD6C5_1ACE_47ED_AD54_7ED140DFF7CC__INCLUDED_)
