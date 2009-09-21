///////////////////////////////////////////////////////////////////////////////
//
//  Module: Utility.h
//
//    Desc: Misc static helper methods
//
// Copyright (c) 2003 Michael Carruth
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _UTILITY_H_
#define _UTILITY_H_

#pragma once

#include <shlobj.h>


////////////////////////////// Class Definitions /////////////////////////////

// ===========================================================================
// Utility
// 
// See the module comment at top of file.
//
namespace Utility  
{
   CString getAppName();   

   CString getTempFileName();

   int getTempDirectory(CString& strTemp);

   // Returns path to directory where EXE or DLL module is located.
   CString GetModulePath(HMODULE hModule);

   // Generates unique identifier (GUID)
   int GenerateGUID(CString& sGUID);  

   // Returns current system time as string (uses UTC time format).
   int GetSystemTimeUTC(CString& sTime);  

   // Returns friendly name of operating system (name, version, service pack)
   int GetOSFriendlyName(CString& sOSName);  

   // Returns path to a special folder (for example %LOCAL_APP_DATA%)
   int GetSpecialFolder(int csidl, CString& sFolderPath);

   // Replaces restricted characters in file name
   CString ReplaceInvalidCharsInFileName(CString sFileName);

   // Moves a file to the Recycle Bin or removes the file permanently
   int RecycleFile(CString sFilePath, bool bPermanentDelete);

   CString GetINIString(LPCTSTR pszFileName, LPCTSTR pszSection, LPCTSTR pszName);
   CString GetINIString(LPCTSTR pszSection, LPCTSTR pszName);

   void SetLayoutRTL(HWND hWnd);
};

#include <vector>
class strconv_t
{
public:
  strconv_t(){}
  ~strconv_t()
  {
    unsigned i;
    for(i=0; i<m_ConvertedStrings.size(); i++)
    {
      delete [] m_ConvertedStrings[i];
    }
  }

  LPCWSTR a2w(LPCSTR lpsz)
  {
    int count = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpsz, -1, NULL, 0);
    if(count==0)
      return NULL;

    void* pBuffer = (void*) new wchar_t[count];
    int result = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpsz, -1, (LPWSTR)pBuffer, count);
    if(result==0)
    {
      delete [] pBuffer;
      return NULL;
    }    

    m_ConvertedStrings.push_back(pBuffer);
    return (LPCWSTR)pBuffer;
  }

  LPCSTR w2a(LPCWSTR lpsz)
  {    
    int count = WideCharToMultiByte(CP_ACP, 0, lpsz, -1, NULL, 0, NULL, NULL);
    if(count==0)
      return NULL;

    void* pBuffer = (void*) new char[count];
    int result = WideCharToMultiByte(CP_ACP, 0, lpsz, -1, (LPSTR)pBuffer, count, NULL, NULL);
    if(result==0)
    {
      delete [] pBuffer;
      return NULL;
    }    

    m_ConvertedStrings.push_back(pBuffer);
    return (LPCSTR)pBuffer;
  }

  LPCSTR t2a(LPCTSTR lpsz)
  {
#ifdef UNICODE    
    return w2a(lpsz);
#else
    return lpsz;
#endif
  }

LPCWSTR t2w(LPCTSTR lpsz)
  {
#ifdef UNICODE    
    return lpsz;
#else
    return a2w(lpsz);
#endif
  }

  LPCTSTR a2t(LPCSTR lpsz)
  {
#ifdef UNICODE    
    return a2w(lpsz);
#else
    return lpsz;
#endif
  }

LPCTSTR w2t(LPCWSTR lpsz)
  {
#ifdef UNICODE    
    return lpsz;
#else
    return w2a(lpsz);
#endif
  }

private:
  std::vector<void*> m_ConvertedStrings;  
};



#endif	// #ifndef _UTILITY_H_
