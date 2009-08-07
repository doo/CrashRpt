#pragma once

#include "stdafx.h"
#include "dbghelp.h"
#include <map>

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

  static void Close();

  static MdmpData m_DumpData; // Internally used data

private:
  
  static HANDLE m_hFileMiniDump; // Handle to open .DMP file
  static HANDLE m_hFileMapping;  // Handle to memory mapping
  static LPVOID m_pMiniDumpStartPtr; // Pointer to the biginning of memory-mapped minidump
  
  static CString GetMinidumpString(LPVOID start_addr, RVA rva);

  static int ReadSysInfoStream();
  static int ReadExceptionStream();
  static int ReadModuleListStream();
  static int ReadMemoryListStream();
  static int ReadThreadListStream();
  static int StackWalk();

  static BOOL CALLBACK ReadProcessMemoryProc64(
    HANDLE hProcess,
    DWORD64 lpBaseAddress,
    PVOID lpBuffer,
    DWORD nSize,
    LPDWORD lpNumberOfBytesRead);

  static PVOID CALLBACK FunctionTableAccessProc64(
    HANDLE hProcess,
    DWORD64 AddrBase);

  static DWORD64 CALLBACK GetModuleBaseProc64(
    HANDLE hProcess,
    DWORD64 Address);
};