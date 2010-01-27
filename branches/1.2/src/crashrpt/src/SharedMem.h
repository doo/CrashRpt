#pragma once
#include <atlmisc.h>

struct StringLocator
{
  StringLocator()
  {
    m_lOffset = -1;
    m_cchLength = 0;
  }

  LONG m_lOffset;
  WORD m_cchLength;
};

struct SMFileItem
{
  StringLocator m_DestFile;
  StringLocator m_SrcFile;
  StringLocator m_Description;
  BOOL m_bMakeCopy;
};

struct SMHeader
{
  WCHAR m_szSignature[8];         // This should always be L"CrashRpt".
  DWORD m_dwDataSize;             // Size of the whole data, including size of this header.
  StringLocator m_EmailSubject;   
  StringLocator m_EmailTo;        
  StringLocator m_Url;
  StringLocator m_AppName;
  StringLocator m_AppVersion;
  StringLocator m_ImageName[_MAX_PATH];
  StringLocator m_ErrorReportDirName[_MAX_PATH];
  UINT uPriorities[3];
  StringLocator szPrivacyPolicyUrl[1024];  
};

class CInterProcessCommunicator
{
public:
  
  CInterProcessCommunicator();
  ~CInterProcessCommunicator();

  BOOL Create(CString sCrashGUID);
  void Destroy();
  
  SMHeader* GetSharedMem();
  void SetEvent();
  void WaitForEvent();

private:

  BOOL MapViewOfFile(DWORD dwFileOffs, SIZE_T dwNumberOfBytes);
  BOOL UnmapViewOfFile();
    
  HANDLE m_hFileMapping;   
  LPVOID m_pViewStartPtr;   
  HANDLE m_hEvent;     
};



