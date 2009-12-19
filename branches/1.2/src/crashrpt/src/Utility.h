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

   // Mirrors the content of a window.
   void SetLayoutRTL(HWND hWnd);

   // Formats the error message.
   CString FormatErrorMsg(DWORD dwErrorCode);

   CString GetBaseFileName(CString sFileName);

   CString GetFileExtension(CString sFileName);

   HWND FindAppWindow();
  
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
    if(lpsz==NULL)
      return NULL;

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
    if(lpsz==NULL)
      return NULL;

    int count = WideCharToMultiByte(CP_UTF8, 0, lpsz, -1, NULL, 0, NULL, NULL);
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

  LPCSTR a2utf8(LPCSTR lpsz)
  {
    if(lpsz==NULL)
      return NULL;

    // 1. Convert input ANSI string to widechar using 
    // MultiByteToWideChar(CP_ACP, ...) function (CP_ACP 
    // is current Windows system Ansi code page)
    
    // Calculate required buffer size
    int count = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpsz, -1, NULL, 0);
    if(count==0)
      return NULL;

    // Convert ANSI->UNICODE
    wchar_t* pBuffer = new wchar_t[count];
    int result = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpsz, -1, (LPWSTR)pBuffer, count);
    if(result==0)
    {
      delete [] pBuffer;
      return NULL;
    }  

    // 2. Convert output widechar string from previous call to 
    // UTF-8 using WideCharToMultiByte(CP_UTF8, ...)  function
     
    LPCSTR pszResult = (LPCSTR)w2utf8(pBuffer);
    delete [] pBuffer;
    return pszResult;
  }

  LPCSTR w2utf8(LPCWSTR lpsz)
  {
    if(lpsz==NULL)
      return NULL;
     
    // Calculate required buffer size
    int count = WideCharToMultiByte(CP_UTF8, 0, lpsz, -1, NULL, 0, NULL, NULL);
    if(count==0)
    {      
      return NULL;
    }

    // Convert UNICODE->UTF8
    LPSTR pBuffer = new char[count];
    int result = WideCharToMultiByte(CP_UTF8, 0, lpsz, -1, (LPSTR)pBuffer, count, NULL, NULL);    
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

LPCSTR t2utf8(LPCTSTR lpsz)
  {
#ifdef UNICODE    
    return w2utf8(lpsz);
#else
    return a2utf8(lpsz);
#endif
  }

private:
  std::vector<void*> m_ConvertedStrings;  
};

// wrapper for whatever critical section we have
class CCritSec 
{
  // make copy constructor and assignment operator inaccessible

  CCritSec(const CCritSec &refCritSec);
  CCritSec &operator=(const CCritSec &refCritSec);

  CRITICAL_SECTION m_CritSec;

public:

    CCritSec() 
    {
      InitializeCriticalSection(&m_CritSec);
    };

    ~CCritSec() 
    {
      DeleteCriticalSection(&m_CritSec);
    }

    void Lock() 
    {
      EnterCriticalSection(&m_CritSec);
    };

    void Unlock() 
    {
      LeaveCriticalSection(&m_CritSec);
    };
};

// locks a critical section, and unlocks it automatically
// when the lock goes out of scope
class CAutoLock 
{
  // make copy constructor and assignment operator inaccessible

  CAutoLock(const CAutoLock &refAutoLock);
  CAutoLock &operator=(const CAutoLock &refAutoLock);

protected:
  CCritSec * m_pLock;

public:
  CAutoLock(CCritSec * plock)
  {
    m_pLock = plock;
    m_pLock->Lock();
  };

  ~CAutoLock() 
  {
    m_pLock->Unlock();
  };
};


#endif	// #ifndef _UTILITY_H_
