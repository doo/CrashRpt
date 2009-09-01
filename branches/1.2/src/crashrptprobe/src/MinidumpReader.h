#pragma once

#include "stdafx.h"
#include "dbghelp.h"
#include <map>
#include <vector>

struct MdmpModule
{
  ULONG64 m_uBaseAddr;
  ULONG64 m_uImageSize;
  CString m_sModuleName;
  BOOL m_bSymbolsLoaded;
};

struct MdmpMemRange
{
  ULONG64 m_u64StartOfMemoryRange;
  ULONG32 m_uDataSize;
  LPVOID m_pStartPtr;
};

struct MdmpStackFrame
{
  MdmpStackFrame()
  {
    m_dw64OffsInSymbol = 0;
    m_nSrcLineNumber = -1;
  }
  
  CString m_sModuleName;
  CString m_sSymbolName;
  DWORD64 m_dw64OffsInSymbol;
  CString m_sSrcFileName;
  int m_nSrcLineNumber;
};

struct MdmpData
{   
  HANDLE m_hProcess;
    
  USHORT m_uProcessorArchitecture;
  UCHAR  m_uchNumberOfProcessors;
  UCHAR  m_uchProductType;
  ULONG  m_ulVerMajor;
  ULONG  m_ulVerMinor;
  ULONG  m_ulVerBuild;
  CString m_sCSDVer;
    
  ULONG32 m_uExceptionCode;
  ULONG64 m_uExceptionAddress;  

  ULONG32  m_uExceptionThreadId;
  CONTEXT* m_pExceptionContext;
  CONTEXT* m_pThreadContext;
  ULONG64  m_uExceptionThreadStackAddress;
  
  std::vector<MdmpModule> m_Modules;
  std::vector<MdmpMemRange> m_MemRanges;  
  std::vector<MdmpStackFrame> m_StackTrace;  
};


class CMiniDumpReader
{
public:
  
  CMiniDumpReader();
  ~CMiniDumpReader();

  int Open(CString sFileName);
  void Close();

  MdmpData m_DumpData; // Internally used data

private:
  
  bool m_bLoaded;  

  HANDLE m_hFileMiniDump; // Handle to open .DMP file
  HANDLE m_hFileMapping;  // Handle to memory mapping
  LPVOID m_pMiniDumpStartPtr; // Pointer to the biginning of memory-mapped minidump
  
  CString GetMinidumpString(LPVOID pStartAddr, RVA rva);

  int ReadSysInfoStream();
  int ReadExceptionStream();
  int ReadModuleListStream();
  int ReadMemoryListStream();
  int ReadThreadListStream();
  int StackWalk();  
};

