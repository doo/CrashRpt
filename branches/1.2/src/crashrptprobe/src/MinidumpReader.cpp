#include "stdafx.h"
#include "MinidumpReader.h"
#include <assert.h>

CMiniDumpReader* g_pMiniDumpReader = NULL;

BOOL CALLBACK ReadProcessMemoryProc64(
  HANDLE hProcess,
  DWORD64 lpBaseAddress,
  PVOID lpBuffer,
  DWORD nSize,
  LPDWORD lpNumberOfBytesRead);

PVOID CALLBACK FunctionTableAccessProc64(
  HANDLE hProcess,
  DWORD64 AddrBase);

DWORD64 CALLBACK GetModuleBaseProc64(
  HANDLE hProcess,
  DWORD64 Address);

CMiniDumpReader::CMiniDumpReader()
{
  m_bLoaded = false;
  m_hFileMiniDump = INVALID_HANDLE_VALUE;
  m_hFileMapping = NULL;
  m_pMiniDumpStartPtr = NULL;
  
}

CMiniDumpReader::~CMiniDumpReader()
{
  Close();
}

int CMiniDumpReader::Open(CString sFileName)
{  
  if(m_bLoaded)
  {
    return 1;
  }

  m_hFileMiniDump = CreateFile(
    sFileName, 
    FILE_ALL_ACCESS, 
    0, 
    NULL, 
    OPEN_EXISTING, 
    NULL, 
    NULL);

  if(m_hFileMiniDump==INVALID_HANDLE_VALUE)
  {
    Close();
    return 1;
  }

  m_hFileMapping = CreateFileMapping(
    m_hFileMiniDump, 
    NULL, 
    PAGE_READONLY, 
    0, 
    0, 
    0);

  if(m_hFileMapping==NULL)
  {
    Close();
    return 2;
  }

  m_pMiniDumpStartPtr = MapViewOfFile(
    m_hFileMapping, 
    FILE_MAP_READ, 
    0, 
    0, 
    0);

  if(m_pMiniDumpStartPtr==NULL)
  {    
    Close();
    return 3;
  }
  
  m_DumpData.m_hProcess = (HANDLE)0x12345;
  
  BOOL bSymInit = SymInitialize(
    m_DumpData.m_hProcess, 
    NULL, 
    FALSE);

  if(!bSymInit)
  {
    m_DumpData.m_hProcess = NULL;
    Close();
    return 4;
  }
  
  ReadSysInfoStream();
  ReadExceptionStream();
  ReadModuleListStream();
  ReadThreadListStream();
  ReadMemoryListStream();
  StackWalk();
  
  m_bLoaded = true;
  return 0;
}

void CMiniDumpReader::Close()
{
  UnmapViewOfFile(m_pMiniDumpStartPtr);

  if(m_hFileMapping!=NULL)
  {
    CloseHandle(m_hFileMapping);
  }

  if(m_hFileMiniDump!=INVALID_HANDLE_VALUE)
  {
    CloseHandle(m_hFileMiniDump);
  }

  m_pMiniDumpStartPtr = NULL;

  if(m_DumpData.m_hProcess!=NULL)
  {
    SymCleanup(m_DumpData.m_hProcess);
  }
}

CString CMiniDumpReader::GetMinidumpString(LPVOID start_addr, RVA rva)
{
  MINIDUMP_STRING* pms = (MINIDUMP_STRING*)((LPBYTE)start_addr+rva);
  return CString(pms->Buffer, pms->Length);
}

int CMiniDumpReader::ReadSysInfoStream()
{
  LPVOID pStreamStart = NULL;
  ULONG uStreamSize = 0;
  MINIDUMP_DIRECTORY* pmd = NULL;
  BOOL bRead = FALSE;

  bRead = MiniDumpReadDumpStream(m_pMiniDumpStartPtr, SystemInfoStream, 
    &pmd, &pStreamStart, &uStreamSize);
  
  if(bRead)
  {
    MINIDUMP_SYSTEM_INFO* pSysInfo = (MINIDUMP_SYSTEM_INFO*)pStreamStart;
    
    m_DumpData.m_uProcessorArchitecture = pSysInfo->ProcessorArchitecture;
    m_DumpData.m_uchNumberOfProcessors = pSysInfo->NumberOfProcessors;
    m_DumpData.m_uchProductType = pSysInfo->ProductType;
    m_DumpData.m_ulVerMajor = pSysInfo->MajorVersion;
    m_DumpData.m_ulVerMinor = pSysInfo->MinorVersion;
    m_DumpData.m_ulVerBuild = pSysInfo->BuildNumber;
    m_DumpData.m_sCSDVer = GetMinidumpString(m_pMiniDumpStartPtr, pSysInfo->CSDVersionRva);

    // Clean up
    pStreamStart = NULL;
    uStreamSize = 0;    
    pmd = NULL;
  }
  else 
  {
    return 1;    
  }

  return 0;
}

int CMiniDumpReader::ReadExceptionStream()
{
  LPVOID pStreamStart = NULL;
  ULONG uStreamSize = 0;
  MINIDUMP_DIRECTORY* pmd = NULL;
  BOOL bRead = FALSE;

  bRead = MiniDumpReadDumpStream(
    m_pMiniDumpStartPtr, 
    ExceptionStream, 
    &pmd, 
    &pStreamStart, 
    &uStreamSize);

  if(bRead)
  {
    MINIDUMP_EXCEPTION_STREAM* pExceptionStream = (MINIDUMP_EXCEPTION_STREAM*)pStreamStart;
    if(pExceptionStream!=NULL && 
      uStreamSize>=sizeof(MINIDUMP_EXCEPTION_STREAM))
    {
      m_DumpData.m_uExceptionThreadId = pExceptionStream->ThreadId;
      m_DumpData.m_uExceptionCode = pExceptionStream->ExceptionRecord.ExceptionCode;
      m_DumpData.m_uExceptionAddress = pExceptionStream->ExceptionRecord.ExceptionAddress;    
      m_DumpData.m_pExceptionContext = (CONTEXT*)((LPBYTE)m_pMiniDumpStartPtr+pExceptionStream->ThreadContext.Rva);
    }    
  }
  else
  {
    return 1;
  }

  return 0;
}

int CMiniDumpReader::ReadModuleListStream()
{
  USES_CONVERSION; 

  LPVOID pStreamStart = NULL;
  ULONG uStreamSize = 0;
  MINIDUMP_DIRECTORY* pmd = NULL;
  BOOL bRead = FALSE;

  bRead = MiniDumpReadDumpStream(
    m_pMiniDumpStartPtr, 
    ModuleListStream, 
    &pmd, 
    &pStreamStart, 
    &uStreamSize);

  if(bRead)
  {
    MINIDUMP_MODULE_LIST* pModuleStream = (MINIDUMP_MODULE_LIST*)pStreamStart;
    if(pModuleStream!=NULL)
    {
      ULONG32 uNumberOfModules = pModuleStream->NumberOfModules;
      ULONG32 i;
      for(i=0; i<uNumberOfModules; i++)
      {
        MINIDUMP_MODULE* pModule = 
          (MINIDUMP_MODULE*)((LPBYTE)pModuleStream->Modules+i*sizeof(MINIDUMP_MODULE));

        MdmpModule m;
        m.m_uBaseAddr = pModule->BaseOfImage;
        m.m_uImageSize = pModule->SizeOfImage;
        //m.m_sModuleName = GetMinidumpString(m_pMiniDumpStartPtr, pModule->ModuleNameRva);
               
        LPSTR szModuleName = T2A(CString(m.m_sModuleName).GetBuffer(0));
        SymLoadModuleEx(
          m_DumpData.m_hProcess,
          NULL,
          szModuleName,
          NULL,
          m.m_uBaseAddr,
          (DWORD)m.m_uImageSize,
          NULL,
          0);
        DWORD dwLastError = GetLastError();
        m.m_bSymbolsLoaded = (dwLastError==0)?TRUE:FALSE;

        m_DumpData.m_Modules.push_back(m);
      }
    }
  }
  else
  {
    return 1;
  }

  return 0;
}

int CMiniDumpReader::ReadMemoryListStream()
{
  LPVOID pStreamStart = NULL;
  ULONG uStreamSize = 0;
  MINIDUMP_DIRECTORY* pmd = NULL;
  BOOL bRead = FALSE;

  bRead = MiniDumpReadDumpStream(
    m_pMiniDumpStartPtr, 
    MemoryListStream, 
    &pmd, 
    &pStreamStart, 
    &uStreamSize);

  if(bRead)
  {
    MINIDUMP_MEMORY_LIST* pMemStream = (MINIDUMP_MEMORY_LIST*)pStreamStart;
    if(pMemStream!=NULL)
    {
      ULONG32 uNumberOfMemRanges = pMemStream->NumberOfMemoryRanges;
      ULONG i;
      for(i=0; i<uNumberOfMemRanges; i++)
      {
        MINIDUMP_MEMORY_DESCRIPTOR* pMemDesc = (MINIDUMP_MEMORY_DESCRIPTOR*)(&pMemStream->MemoryRanges[i]);
        MdmpMemRange mr;
        mr.m_u64StartOfMemoryRange = pMemDesc->StartOfMemoryRange;
        mr.m_uDataSize = pMemDesc->Memory.DataSize;
        mr.m_pStartPtr = (LPBYTE)m_pMiniDumpStartPtr+pMemDesc->Memory.Rva;

        m_DumpData.m_MemRanges.push_back(mr);
      }
    }
  }
  else    
  {
    return 1;
  }

  return 0;
}
  
int CMiniDumpReader::ReadThreadListStream()
{
  LPVOID pStreamStart = NULL;
  ULONG uStreamSize = 0;
  MINIDUMP_DIRECTORY* pmd = NULL;
  BOOL bRead = FALSE;

  bRead = MiniDumpReadDumpStream(
    m_pMiniDumpStartPtr, 
    ThreadListStream, 
    &pmd, 
    &pStreamStart, 
    &uStreamSize);

  if(bRead)
  {
    MINIDUMP_THREAD_LIST* pThreadList = (MINIDUMP_THREAD_LIST*)pStreamStart;
    if(pThreadList!=NULL && 
      uStreamSize>=sizeof(MINIDUMP_THREAD_LIST))
    {
      ULONG32 uThreadCount = pThreadList->NumberOfThreads;

      ULONG32 i;
      for(i=0; i<uThreadCount; i++)
      {
        MINIDUMP_THREAD* pThread = (MINIDUMP_THREAD*)(&pThreadList->Threads[i]);

        // Read thread info
        if(pThread->ThreadId==m_DumpData.m_uExceptionThreadId)
        {
          m_DumpData.m_uExceptionThreadStackAddress = 
            pThread->Stack.StartOfMemoryRange;
          
          m_DumpData.m_pThreadContext = 
            (CONTEXT*)(((LPBYTE)m_pMiniDumpStartPtr)+pThread->ThreadContext.Rva);
        }        
      }
    }  
  }
  else
  {
    return 1;
  }

  return 0;
}

int CMiniDumpReader::StackWalk()
{  
  if(m_DumpData.m_pExceptionContext==NULL)
    return 1;

  g_pMiniDumpReader = this;

  // Init stack frame with correct initial values
  // See this:
  // http://www.codeproject.com/KB/threads/StackWalker.aspx
  //
  // Given a current dbghelp, your code should:
  //  1. Always use StackWalk64
  //  2. Always set AddrPC to the current instruction pointer (Eip on x86, Rip on x64 and StIIP on IA64)
  //  3. Always set AddrStack to the current stack pointer (Esp on x86, Rsp on x64 and IntSp on IA64)
  //  4. Set AddrFrame to the current frame pointer when meaningful. On x86 this is Ebp, on x64 you 
  //     can use Rbp (but is not used by VC2005B2; instead it uses Rdi!) and on IA64 you can use RsBSP. 
  //     StackWalk64 will ignore the value when it isn't needed for unwinding.
  //  5. Set AddrBStore to RsBSP for IA64. 

  STACKFRAME64 sf;
  memset(&sf, 0, sizeof(STACKFRAME64));

  sf.AddrPC.Mode = AddrModeFlat;  
  sf.AddrFrame.Mode = AddrModeFlat;  
  sf.AddrStack.Mode = AddrModeFlat;  
  sf.AddrBStore.Mode = AddrModeFlat;  

  DWORD dwMachineType = 0;
  switch(m_DumpData.m_uProcessorArchitecture)
  {
  case PROCESSOR_ARCHITECTURE_INTEL: 
    dwMachineType = IMAGE_FILE_MACHINE_I386;
    sf.AddrPC.Offset = ((CONTEXT*)m_DumpData.m_pExceptionContext)->Eip;
    sf.AddrFrame.Offset = ((CONTEXT*)m_DumpData.m_pExceptionContext)->Ebp;
    sf.AddrStack.Offset = ((CONTEXT*)m_DumpData.m_pExceptionContext)->Esp;
    break;
  /*case PROCESSOR_ARCHITECTURE_IA64:
    dwMachineType = IMAGE_FILE_MACHINE_IA64;
    sf.AddrPC.Offset = ((CONTEXT_IA64*)m_DumpData.m_pExceptionContext)->Rip;
    sf.AddrFrame.Offset = ((CONTEXT_IA64*)m_DumpData.m_pExceptionContext)->Rsp;
    sf.AddrStack.Offset = ((CONTEXT_IA64*)m_DumpData.m_pExceptionContext)->Rsp;
    break;
  case PROCESSOR_ARCHITECTURE_AMD64:
    dwMachineType = IMAGE_FILE_MACHINE_AMD64;
    sf.AddrPC.Offset = ((CONTEXT_AMD64*)m_DumpData.m_pExceptionContext)->StIIP;
    sf.AddrFrame.Offset = ((CONTEXT_AMD64*)m_DumpData.m_pExceptionContext)->IntSp;
    sf.AddrStack.Offset = ((CONTEXT_AMD64*)m_DumpData.m_pExceptionContext)->IntSp;
    sf.AddrBStore.Offset = ((CONTEXT_AMD64*)m_DumpData.m_pExceptionContext)->RsBSP;
    break;*/
  default:
    {
      assert(0);
      return 1; // Unknown machine type
    }
  }

  for(;;)
  {    
    BOOL bWalk = StackWalk64(
      dwMachineType,               // machine type
      m_DumpData.m_hProcess,       // our process handle
      (HANDLE)m_DumpData.m_uExceptionThreadId, // this even can be NULL
      &sf,                         // stack frame
      dwMachineType==IMAGE_FILE_MACHINE_I386?NULL:m_DumpData.m_pExceptionContext, // used for non-I386 machines 
      ReadProcessMemoryProc64,     // our routine
      FunctionTableAccessProc64,   // our routine
      GetModuleBaseProc64,         // our routine
      NULL                         // safe to be NULL
      );

    if(!bWalk)
      break;      

    MdmpStackFrame stack_frame;
  
    // Get module info
    IMAGEHLP_MODULE64 mi;
    memset(&mi, 0, sizeof(IMAGEHLP_MODULE64));
    mi.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
    SymGetModuleInfo64(m_DumpData.m_hProcess, sf.AddrPC.Offset, &mi); 
    DWORD dwLastError = GetLastError();
    if(dwLastError==0)
    {
      stack_frame.m_sModuleName = mi.ModuleName;
    }

    // Get symbol info
    DWORD64 dwDisp64;
    BYTE buffer[4096];
    SYMBOL_INFO* sym_info = (SYMBOL_INFO*)buffer;
    sym_info->SizeOfStruct = sizeof(SYMBOL_INFO);
    sym_info->MaxNameLen = 4096-sizeof(SYMBOL_INFO)-1;
    BOOL bGetSym = SymFromAddr(
      m_DumpData.m_hProcess, 
      sf.AddrPC.Offset, 
      &dwDisp64, 
      sym_info);
    
    if(bGetSym)
    {
      stack_frame.m_sSymbolName = CString(sym_info->Name, sym_info->NameLen);
      stack_frame.m_dw64OffsInSymbol = dwDisp64;
    }

    // Get source filename and line
    DWORD dwDisplacement;
    IMAGEHLP_LINE64 line;
    BOOL bGetLine = SymGetLineFromAddr64(
      m_DumpData.m_hProcess, 
      sf.AddrPC.Offset,
      &dwDisplacement,
      &line);

    if(bGetLine)
    {
      stack_frame.m_sSrcFileName = line.FileName;
      stack_frame.m_nSrcLineNumber = line.LineNumber;
    }

    m_DumpData.m_StackTrace.push_back(stack_frame);
  }

  return 0;
}


BOOL CALLBACK ReadProcessMemoryProc64(
  HANDLE hProcess,
  DWORD64 lpBaseAddress,
  PVOID lpBuffer,
  DWORD nSize,
  LPDWORD lpNumberOfBytesRead)
{
  *lpNumberOfBytesRead = 0;

  // Validate input parameters
  if(hProcess!=g_pMiniDumpReader->m_DumpData.m_hProcess ||
     lpBaseAddress==NULL ||
     lpBuffer==NULL ||
     nSize==0)
  {
    // Invalid parameter
    return FALSE;
  }

  ULONG i;
  for(i=0; i<g_pMiniDumpReader->m_DumpData.m_MemRanges.size(); i++)
  {
    MdmpMemRange& mr = g_pMiniDumpReader->m_DumpData.m_MemRanges[i];
    if(lpBaseAddress>=mr.m_u64StartOfMemoryRange &&
      lpBaseAddress<mr.m_u64StartOfMemoryRange+mr.m_uDataSize)
    {
      DWORD64 dwOffs = lpBaseAddress-mr.m_u64StartOfMemoryRange;
      
      DWORD dwBytesRead = 0;
      
      if(mr.m_uDataSize-dwOffs>nSize)
        dwBytesRead = nSize;
      else
        dwBytesRead = (DWORD)(mr.m_uDataSize-dwOffs);

      *lpNumberOfBytesRead = dwBytesRead;

      if(dwBytesRead>0)
      {
        assert(nSize>=dwBytesRead);
        memcpy(lpBuffer, (LPBYTE)mr.m_pStartPtr+dwOffs, dwBytesRead);
      }

      return TRUE;
    }
  }

  return FALSE;
}

PVOID CALLBACK FunctionTableAccessProc64(
  HANDLE hProcess,
  DWORD64 AddrBase)
{   
  return SymFunctionTableAccess64(hProcess, AddrBase);
}

DWORD64 CALLBACK GetModuleBaseProc64(
  HANDLE hProcess,
  DWORD64 Address)
{  
  return SymGetModuleBase64(hProcess, Address);
}