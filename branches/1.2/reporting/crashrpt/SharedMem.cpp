/************************************************************************************* 
  This file is a part of CrashRpt library.

  Copyright (c) 2003, Michael Carruth
  All rights reserved.
 
  Redistribution and use in source and binary forms, with or without modification, 
  are permitted provided that the following conditions are met:
 
   * Redistributions of source code must retain the above copyright notice, this 
     list of conditions and the following disclaimer.
 
   * Redistributions in binary form must reproduce the above copyright notice, 
     this list of conditions and the following disclaimer in the documentation 
     and/or other materials provided with the distribution.
 
   * Neither the name of the author nor the names of its contributors 
     may be used to endorse or promote products derived from this software without 
     specific prior written permission.
 

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY 
  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
  SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR 
  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************************/

#include "stdafx.h"

CSharedMem::CSharedMem()
{
  m_dwAllocGranularity = 0;
  m_hFileMapping = NULL;
  m_pViewStartPtr = NULL;
  m_hAccessMutex = NULL;  
}

CSharedMem::~CSharedMem()
{
}

HANDLE __OpenFileMapping()
{
  __try{ // Use SEH handler to catch possible access violations
  
    return OpenFileMapping(
      FILE_MAP_READ|FILE_MAP_WRITE, // Read/write access
      FALSE,               // Do not inherit this handle
      FILE_MAPPING_NAME    // File mapping name
      );
  }
  __except(EXCEPTION_EXECUTE_HANDLER)
  {
    // Access violation - maybe mem page fault.    
    return NULL;    
  }
}


// Creates new file mapping or opens an existing one. 
BOOL CSharedMem::Init()
{ 
  BOOL bStatus = FALSE;
  CAutoLock lock(&m_csLock);

  // Get memory allocation granularity
  SYSTEM_INFO si;  
  GetSystemInfo(&si);
  m_dwAllocGranularity = si.dwAllocationGranularity;  
  if(m_dwAllocGranularity==0)
    goto cleanup;
    
  // Create access mutex
  // Try to open existing one
  m_hAccessMutex = OpenMutex(
    SYNCHRONIZE|MUTEX_MODIFY_STATE,
    FALSE,
    MUTEX_NAME // Mutex name
   );    
    
  if(m_hAccessMutex==NULL) // If doesn't exist, create new one
  {    
    m_hAccessMutex = CreateMutex(
      NULL, // security attrs
      FALSE, // Initial owner
      MUTEX_NAME // Mutex name
      );      
  }

  if(m_hAccessMutex==NULL)
  {
    goto cleanup;
  }
    
  // Try to open existing file mapping

  m_hFileMapping = __OpenFileMapping();
  if(m_hFileMapping==NULL)
  {
    // Create file mapping
    m_hFileMapping = ::CreateFileMapping(
      INVALID_HANDLE_VALUE, // Use system paging file
      NULL,                  // Use default security attributes
      PAGE_READWRITE,       // Allow reading and writing access
      0,
      sizeof(FileMappingDescriptor),   // Size of the file mapping
      FILE_MAPPING_NAME);  // Name of the file mapping

    SetLowIntegrityByName(FILE_MAPPING_NAME, SE_KERNEL_OBJECT);
  }

  if(m_hFileMapping==NULL)
  {
    assert(m_hFileMapping!=NULL);
    return 1; // 
  }


  bStatus = TRUE;

cleanup:

  if(!bStatus)
  {
    Destroy();
  }

  return bStatus;
}

// Frees all used resources
void CSharedMem::Destroy()
{  
  CAutoLock lock(&m_csLock);

  if(m_pViewStartPtr!=NULL)
    UnmapViewOfFile();

  if(m_hFileMapping!=NULL)
  {
    CloseHandle(m_hFileMapping);
    m_hFileMapping = NULL;
  }

  if(m_hAccessMutex!=NULL)
  {
    CloseHandle(m_hAccessMutex);
    m_hAccessMutex = NULL;
  }

}

BOOL CSharedMem::AcquireAccessMutex(DWORD dwWaitTime)
{
  if(m_hAccessMutex==NULL)
    return FALSE;

  DWORD dwWaitResult = WaitForSingleObject(m_hAccessMutex, dwWaitTime);
  if(dwWaitResult==WAIT_OBJECT_0)
    return TRUE;

  ATLASSERT(dwWaitResult==WAIT_OBJECT_0);
  return FALSE;
}

void CSharedMem::ReleaseAccessMutex()
{
  ATLASSERT(m_hAccessMutex!=NULL);
  ReleaseMutex(m_hAccessMutex);
}

// Maps view of shared memory 
BOOL CSharedMem::MapViewOfFile(DWORD dwFileOffs, SIZE_T dwNumberOfBytes)
{ 
  CAutoLock lock(&m_csLock);

  // Validate internal state
  if(m_pViewStartPtr!=NULL ||
     m_hFileMapping==NULL)
  {
    ATLASSERT(m_pViewStartPtr==NULL);
    ATLASSERT(m_hFileMapping!=NULL);
    return FALSE;
  }

  m_pViewStartPtr = ::MapViewOfFile(
    m_hFileMapping,
    FILE_MAP_READ|FILE_MAP_WRITE,
    0,
    dwFileOffs, // File offset
    dwNumberOfBytes);

  if(m_pViewStartPtr==NULL)
  {
    ATLASSERT(m_pViewStartPtr!=NULL);
    return FALSE;
  }
    
  // Done
  return TRUE;
}

// Unmaps view of shared memory
TRUE CSharedMem::UnmapViewOfFile()
{
  CAutoLock lock(&m_csLock);

  if(m_pViewStartPtr==NULL)
  {    
    // Invalid mapping ptr
    return FALSE;
  }

  BOOL bUnmap = ::UnmapViewOfFile(m_pViewStartPtr);

  m_pViewStartPtr = NULL;
  
  return bUnmap;
}

void CSharedMem::GetViewBase(DWORD dwAbsOffs, DWORD& dwGranOffs, DWORD& dwOffsInGran)
{
  dwOffsInGran = dwAbsOffs%m_dwAllocGranularity;
  dwGranOffs = dwAbsOffs - dwOffsInGran;  
}

