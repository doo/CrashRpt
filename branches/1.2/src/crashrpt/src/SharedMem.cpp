#include "stdafx.h"
#include <assert.h>
#include "SharedMem.h"


CInterProcessCommunicator::CInterProcessCommunicator()
{
  m_hFileMapping = NULL;
  m_pViewStartPtr = NULL;
}

CInterProcessCommunicator::~CInterProcessCommunicator()
{
  Destroy();
}

BOOL CInterProcessCommunicator::Create(CString sCrashGUID)
{
  if(m_hFileMapping!=NULL)
  {
    assert(m_hFileMapping==NULL);
    return TRUE; // Already created
  }

  // Create file mapping
  CString sFileMappingName;
  sFileMappingName.Format(_T("CrashRptFileMapping_%s"), sCrashGUID);

  m_hFileMapping = ::CreateFileMapping(
    INVALID_HANDLE_VALUE, // Use system paging file
    NULL,                 // Use default security attributes
    PAGE_READWRITE,       // Allow reading and writing access
    0,
    sizeof(SharedMemory),   // Size of the file mapping
    sFileMappingName);  // Name of the file mapping

  if(m_hFileMapping==NULL)
  {
    assert(m_hFileMapping!=NULL);
    return FALSE; 
  }
  
  CString sEventName;
  sEventName.Format(_T("CrashRptEvent_%s"), sCrashGUID);

  m_hEvent = CreateEvent(
    NULL,      // Default security attrs
    TRUE,      // Manual reset event
    FALSE,     // Initial state
    sEventName // Event name
    );

  if(m_hEvent==NULL)
  {
    assert(m_hEvent!=NULL);
    return FALSE;
  }

  return TRUE; 
}

void CInterProcessCommunicator::Destroy()
{
  if(m_pViewStartPtr!=NULL)
    UnmapViewOfFile();

  if(m_hFileMapping!=NULL)
  {
    CloseHandle(m_hFileMapping);
    m_hFileMapping = NULL;
  }

  if(m_hEvent!=NULL)
  {
    CloseHandle(m_hEvent);
    m_hEvent = NULL;
  }
}

BOOL CInterProcessCommunicator::MapViewOfFile(DWORD dwFileOffs, SIZE_T dwNumberOfBytes)
{  
  if(m_pViewStartPtr!=NULL || m_hFileMapping==NULL)
  {
    assert(m_pViewStartPtr==NULL &&  m_hFileMapping!=NULL);
    return FALSE;
  }

  m_pViewStartPtr = ::MapViewOfFile(
    m_hFileMapping,
    FILE_MAP_ALL_ACCESS,
    0,
    dwFileOffs, // File offset
    dwNumberOfBytes);

  if(m_pViewStartPtr==NULL)
  {
    assert(m_pViewStartPtr!=NULL);
    return 1;
  }
    
  //Ok
  return 0;
}

BOOL CInterProcessCommunicator::UnmapViewOfFile()
{
  if(m_pViewStartPtr==NULL)
    return FALSE;
  
  BOOL bUnmap = ::UnmapViewOfFile(m_pViewStartPtr);
  m_pViewStartPtr = NULL;
  
  return bUnmap;
}

SMHeader* CInterProcessCommunicator::GetSharedMem()
{
  return (SMHeader*)m_pViewStartPtr;
}
