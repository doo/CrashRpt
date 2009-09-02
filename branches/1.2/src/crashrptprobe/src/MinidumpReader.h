#pragma once

#include "stdafx.h"
#include "dbghelp.h"
#include <map>
#include <vector>

// Describes a loaded module
struct MdmpModule
{
  ULONG64 m_uBaseAddr;   // Base address
  ULONG64 m_uImageSize;  // Size of module
  CString m_sModuleName; // Module name
  BOOL m_bSymbolsLoaded; // Were symbols loaded for this module?
};

// Describes a memory range
struct MdmpMemRange
{
  ULONG64 m_u64StartOfMemoryRange; // Starting address
  ULONG32 m_uDataSize;             // Size of data
  LPVOID m_pStartPtr;              // Pointer to the memrange data stored in minidump
};

// Describes a stack frame
struct MdmpStackFrame
{
  MdmpStackFrame()
  {
    m_dw64OffsInSymbol = 0;
    m_nSrcLineNumber = -1;
  }
  
  CString m_sModuleName;      // Name of module
  CString m_sSymbolName;      // Name of symbol
  DWORD64 m_dw64OffsInSymbol; // Offset in symbol
  CString m_sSrcFileName;     // Name of source file
  int m_nSrcLineNumber;       // Line number in the source file
};

// Minidump data
struct MdmpData
{   
  MdmpData()
  {
    m_hProcess = INVALID_HANDLE_VALUE;
    m_pExceptionContext = NULL;
    m_pThreadContext = NULL;
  }

  HANDLE m_hProcess; // Process ID
    
  USHORT m_uProcessorArchitecture; // CPU architecture
  UCHAR  m_uchNumberOfProcessors;  // Number of processors
  UCHAR  m_uchProductType;         // Type of machine (workstation, server, ...)
  ULONG  m_ulVerMajor;             // OS major version number
  ULONG  m_ulVerMinor;             // OS minor version number
  ULONG  m_ulVerBuild;             // OS build number
  CString m_sCSDVer;               // The latest service pack installed
    
  ULONG32 m_uExceptionCode;        // Structured exception's code
  ULONG64 m_uExceptionAddress;     // Exception's address

  ULONG32  m_uExceptionThreadId;   // Exceptions's thread ID
  CONTEXT* m_pExceptionContext;    // 
  CONTEXT* m_pThreadContext;       //
  ULONG64  m_uExceptionThreadStackAddress; // Stack starting address
  
  std::vector<MdmpModule> m_Modules; // The list of loaded modules
  std::vector<MdmpMemRange> m_MemRanges; // The list of memory ranges
  std::vector<MdmpStackFrame> m_StackTrace; // Exception stack trace
};

// Class for opening minidumps
class CMiniDumpReader
{
public:
  
  /* Construction/destruction */

  CMiniDumpReader();
  ~CMiniDumpReader();

  /* Operations */

  // Opens a minidump (DMP) file
  int Open(CString sFileName, CString sSymSearchPath);

  // Closes the opened minidump file
  void Close();

  MdmpData m_DumpData; // Minidump data

private:

  /* Internally used member functions */
  
  // Helper function which extracts a UNICODE string from the minidump
  CString GetMinidumpString(LPVOID pStartAddr, RVA rva);

  // Reads MINIDUMP_SYSTEM_INFO stream
  int ReadSysInfoStream();

  // Reads MINIDUMP_EXCEPTION_STREAM stream
  int ReadExceptionStream();

  // Reads MINIDUMP_MODULE_LIST stream
  int ReadModuleListStream();

  // Reads MINIDUMP_MEMORY_LIST stream
  int ReadMemoryListStream();

  // Reads MINIDUMP_THREAD_LIST stream
  int ReadThreadListStream();

  // Retreives stack trace
  int StackWalk();  

  /* Member variables */
  
  HANDLE m_hFileMiniDump; // Handle to opened .DMP file
  HANDLE m_hFileMapping;  // Handle to memory mapping object
  LPVOID m_pMiniDumpStartPtr; // Pointer to the biginning of memory-mapped minidump  

  BOOL m_bLoaded;  // Is minidump loaded?
  BOOL m_bReadSysInfoStream; // Was system info stream read?
  BOOL m_bReadExceptionStream; // Was exception stream read?
  BOOL m_bReadModuleListStream; // Was module list stream read?
  BOOL m_bReadMemoryListStream; // Was memory list stream read?
  BOOL m_bReadThreadListStream; // Was thread list stream read?
  BOOL m_bStackWalk; // Was stack trace retrieved?
};

