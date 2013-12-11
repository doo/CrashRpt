/************************************************************************************* 
This file is a part of CrashRpt library.
Copyright (c) 2003-2013 The CrashRpt project authors. All Rights Reserved.

Use of this source code is governed by a BSD-style license
that can be found in the License.txt file in the root of the source
tree. All contributing project authors may
be found in the Authors.txt file in the root of the source tree.
***************************************************************************************/

#include "stdafx.h"
#include "SharedMem.h"

CSharedMem::CSharedMem()
{
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  m_dwAllocGranularity = si.dwAllocationGranularity;
}

void CSharedMem::Destroy() {
  m_fileMapping.Unmap();
}

BOOL CSharedMem::Init(LPCTSTR szName, BOOL bOpenExisting, ULONG64 uSize)
{
  // If already initialised, do nothing
  if(m_fileMapping.GetHandle()!=NULL)
    return FALSE;

  // Either open existing file mapping or create new one
  if(!bOpenExisting)
  {
    /*ULARGE_INTEGER i;
    i.QuadPart = uSize;
    m_hFileMapping = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, i.HighPart, i.LowPart, szName);*/
    m_fileMapping.MapSharedMem(uSize, szName, NULL, NULL, PAGE_READWRITE, FILE_MAP_READ|FILE_MAP_WRITE);

  }
  else
  {
    //m_hFileMapping = OpenFileMapping(FILE_MAP_READ|FILE_MAP_WRITE, FALSE, szName);
    m_fileMapping.OpenMapping(szName, uSize, 0, FILE_MAP_READ|FILE_MAP_WRITE);
  }

  // Save name and size of the file mapping
  m_sName = szName;
  //m_uSize = uSize; 

  // Check file mapping is valid
  if(m_fileMapping.GetHandle() == NULL)
  {
    return FALSE;
  }

  return TRUE;
}

BOOL CSharedMem::IsInitialized()
{
  return m_fileMapping.GetHandle() != NULL;
}

WTL::CString CSharedMem::GetName()
{
  return m_sName;
}

ULONG64 CSharedMem::GetSize()
{
  return m_fileMapping.GetMappingSize();
  //return m_uSize;
}

LPBYTE CSharedMem::CreateView(DWORD dwOffset, DWORD /*dwLength*/)
{
  DWORD dwBaseOffs = dwOffset-dwOffset%m_dwAllocGranularity;
  DWORD dwDiff = dwOffset-dwBaseOffs;
  LPBYTE pPtr = ((LPBYTE)m_fileMapping.GetData()) + dwBaseOffs;

  return (pPtr+dwDiff);
}

void CSharedMem::DestroyView(LPBYTE /*pViewPtr*/)
{
}
