#include "stdafx.h"
#include "SenderThread.h"
#include "MailMsg.h"
#include "smtpclient.h"
#include "httpsend.h"
#include "CrashRpt.h"
#include "md5.h"
#include "Utility.h"
#include "zip.h"
#include "CrashSender.h"

int attempt = 0;
AssyncNotification an;
CEmailMessage msg;
CSmtpClient smtp;  
CHttpSender http;
CMailMsg mailmsg;

int CalcFileMD5Hash(CString sFileName, CString& sMD5Hash)
{
  FILE* f = NULL;
  BYTE buff[512];
  MD5 md5;
  MD5_CTX md5_ctx;
  unsigned char md5_hash[16];
  int i;

  sMD5Hash.Empty();

#if _MSC_VER<1400
  f = _tfopen(sFileName.GetBuffer(0), _T("rb"));
#else
  _tfopen_s(&f, sFileName.GetBuffer(0), _T("rb"));
#endif

  if(f==NULL) 
    return -1;

  md5.MD5Init(&md5_ctx);
  
  while(!feof(f))
  {
    size_t count = fread(buff, 1, 512, f);
    if(count>0)
    {
      md5.MD5Update(&md5_ctx, buff, (unsigned int)count);
    }
  }

  fclose(f);
  md5.MD5Final(md5_hash, &md5_ctx);

  for(i=0; i<16; i++)
  {
    CString number;
    number.Format(_T("%02x"), md5_hash[i]);
    sMD5Hash += number;
  }

  return 0;
}

void GetSenderThreadStatus(int& nProgressPct, std::vector<CString>& msg_log)
{
  an.GetProgress(nProgressPct, msg_log); 
}

void CancelSenderThread()
{
  an.Cancel();
}

void FeedbackReady(int code)
{
  an.FeedbackReady(code);
}

BOOL CALLBACK MiniDumpCallback(
  __in     PVOID CallbackParam,
  __in     const PMINIDUMP_CALLBACK_INPUT CallbackInput,
  __inout  PMINIDUMP_CALLBACK_OUTPUT CallbackOutput )
{
  /*switch(CallbackInput.CallbackType)
  {    
  };*/
  return TRUE;
}


BOOL CreateMinidump()
{ 
  BOOL bStatus = FALSE;
  HMODULE hDbgHelp = NULL;
  HANDLE hFile = NULL;
  MINIDUMP_EXCEPTION_INFORMATION mei;
  MINIDUMP_CALLBACK_INFORMATION mci;
  CString sMinidumpFile = g_CrashInfo.m_sErrorReportDirName + _T("\\crashdump.dmp");

  an.SetProgress(_T("Creating crash dump file..."), 0, false);
  an.SetProgress(_T("[crating_dump]"), 0, false);

  // Load dbghelp.dll
  hDbgHelp = LoadLibrary(g_CrashInfo.m_sDbgHelpPath);
  if(hDbgHelp==NULL)
  {
    an.SetProgress(_T("dbghelp.dll couldn't be loaded."), 0, false);
    goto cleanup;
  }

  // Create the file
  hFile = CreateFile(
    sMinidumpFile,
    GENERIC_WRITE,
    0,
    NULL,
    CREATE_ALWAYS,
    FILE_ATTRIBUTE_NORMAL,
    NULL);

  if(hFile==INVALID_HANDLE_VALUE)
  {
    ATLASSERT(hFile!=INVALID_HANDLE_VALUE);
    an.SetProgress(_T("Couldn't create dump file."), 0, false);
    return FALSE;
  }

  // Write minidump to the file
  mei.ThreadId = g_CrashInfo.m_dwThreadId;
  mei.ExceptionPointers = &g_CrashInfo.m_ExInfo;
  mei.ClientPointers = TRUE;
  
  mci.CallbackRoutine = MiniDumpCallback;
  mci.CallbackParam = 0;

  typedef BOOL (WINAPI *LPMINIDUMPWRITEDUMP)(
    HANDLE hProcess, 
    DWORD ProcessId, 
    HANDLE hFile, 
    MINIDUMP_TYPE DumpType, 
    CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, 
    CONST PMINIDUMP_USER_STREAM_INFORMATION UserEncoderParam, 
    CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

  LPMINIDUMPWRITEDUMP pfnMiniDumpWriteDump = 
    (LPMINIDUMPWRITEDUMP)GetProcAddress(hDbgHelp, "MiniDumpWriteDump");
  if(!pfnMiniDumpWriteDump)
  {    
    an.SetProgress(_T("Bad MiniDumpWriteDump function."), 0, false);
    return FALSE;
  }

  HANDLE hProcess = OpenProcess(
    PROCESS_ALL_ACCESS, 
    FALSE, 
    g_CrashInfo.m_dwProcessId);

  BOOL bWriteDump = pfnMiniDumpWriteDump(
    hProcess,
    g_CrashInfo.m_dwProcessId,
    hFile,
    g_CrashInfo.m_MinidumpType,
    &mei,
    NULL,
    &mci);
 
  if(!bWriteDump)
  {    
    an.SetProgress(_T("Error writing dump."), 0, false);
    an.SetProgress(Utility::FormatErrorMsg(GetLastError()), 0, false);
    goto cleanup;
  }

  bStatus = TRUE;
  an.SetProgress(_T("Finished creating dump."), 100, false);

cleanup:

  // Close file
  if(hFile)
    CloseHandle(hFile);

  if(hDbgHelp)
    FreeLibrary(hDbgHelp);

  return bStatus;
}

BOOL CompressReportFiles(SenderThreadContext* pc)
{ 
  BOOL bStatus = FALSE;
  strconv_t strconv;
  zipFile hZip = NULL;
  CString sMsg;
  LONG64 lTotalSize = 0;
  LONG64 lTotalCompressed = 0;
  BYTE buff[1024];
  DWORD dwBytesRead=0;
  CString sZipName;

  an.SetProgress(_T("Start compressing files..."), 0, false);
  an.SetProgress(_T("[compressing_files]"), 0, false);
  an.SetProgress(_T("Calculating total size of files to compress..."), 0, false);

  std::map<CString, FileItem>::iterator it;
  for(it=g_CrashInfo.m_FileItems.begin(); it!=g_CrashInfo.m_FileItems.end(); it++)
  {    
    if(an.IsCancelled())    
      goto cleanup;

    CString sFileName = it->second.m_sSrcFile.GetBuffer(0);
    HANDLE hFile = CreateFile(sFileName, 
      GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL); 
    if(hFile==INVALID_HANDLE_VALUE)
    {
      sMsg.Format(_T("Couldn't open file %s"), sFileName);
      an.SetProgress(sMsg, 0, false);
      continue;
    }

    LARGE_INTEGER lFileSize;
    BOOL bGetSize = GetFileSizeEx(hFile, &lFileSize);
    if(!bGetSize)
    {
      sMsg.Format(_T("Couldn't get file size of %s"), sFileName);
      an.SetProgress(sMsg, 0, false);
      CloseHandle(hFile);
      continue;
    }

    lTotalSize += lFileSize.QuadPart;
    CloseHandle(hFile);
  }

  sMsg.Format(_T("Total file size for compression is %I64d"), lTotalSize);
  an.SetProgress(sMsg, 0, false);

  sZipName = g_CrashInfo.m_sErrorReportDirName + _T(".zip");  
  pc->m_sZipName = sZipName;
  
  sMsg.Format(_T("Creating ZIP archive file %s"), sZipName);
  an.SetProgress(sMsg, 1, false);

  hZip = zipOpen(strconv.t2a(sZipName.GetBuffer(0)), APPEND_STATUS_CREATE);
  if(hZip==NULL)
  {
    an.SetProgress(_T("Failed to create ZIP file."), 100, true);
    goto cleanup;
  }

  for(it=g_CrashInfo.m_FileItems.begin(); it!=g_CrashInfo.m_FileItems.end(); it++)
  { 
    if(an.IsCancelled())    
      return FALSE;
    
    CString sDstFileName = it->second.m_sDestFile.GetBuffer(0);
    CString sFileName = it->second.m_sSrcFile.GetBuffer(0);
    CString sDesc = it->second.m_sDesc;

    sMsg.Format(_T("Compressing %s"), sDstFileName);
    an.SetProgress(sMsg, 0, false);
        
    HANDLE hFile = CreateFile(sFileName, 
      GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL); 
    if(hFile==INVALID_HANDLE_VALUE)
    {
      sMsg.Format(_T("Couldn't open file %s"), sFileName);
      an.SetProgress(sMsg, 0, false);
      continue;
    }

    BY_HANDLE_FILE_INFORMATION fi;
    GetFileInformationByHandle(hFile, &fi);

    SYSTEMTIME st;
    FileTimeToSystemTime(&fi.ftCreationTime, &st);

    zip_fileinfo info;
    info.dosDate = 0;
    info.tmz_date.tm_year = st.wYear;
    info.tmz_date.tm_mon = st.wMonth;
    info.tmz_date.tm_mday = st.wDay;
    info.tmz_date.tm_hour = st.wHour;
    info.tmz_date.tm_min = st.wMinute;
    info.tmz_date.tm_sec = st.wSecond;
    info.external_fa = FILE_ATTRIBUTE_NORMAL;
    info.internal_fa = FILE_ATTRIBUTE_NORMAL;

    int n = zipOpenNewFileInZip( hZip, strconv.t2a(sDstFileName), &info,
              NULL, 0, NULL, 0, strconv.t2a(sDesc), Z_DEFLATED, Z_DEFAULT_COMPRESSION);
    if(n!=0)
    {
      sMsg.Format(_T("Couldn't compress file %s"), sDstFileName);
      an.SetProgress(sMsg, 0, false);
      continue;
    }

    for(;;)
    {
      if(an.IsCancelled())    
        goto cleanup;

      BOOL bRead = ReadFile(hFile, buff, 1024, &dwBytesRead, NULL);
      if(!bRead || dwBytesRead==0)
        break;

      int res = zipWriteInFileInZip(hZip, buff, dwBytesRead);
      if(res!=0)
      {
        zipCloseFileInZip(hZip);
        sMsg.Format(_T("Couldn't write to compressed file %s"), sDstFileName);
        an.SetProgress(sMsg, 0, false);        
        break;
      }

      lTotalCompressed += dwBytesRead;

      float fProgress = 100.0f*lTotalCompressed/lTotalSize;
      an.SetProgress((int)fProgress, false);
    }

    zipCloseFileInZip(hZip);
    CloseHandle(hFile);
  }

  if(lTotalSize==lTotalCompressed)
    bStatus = TRUE;

cleanup:

  if(hZip!=NULL)
    zipClose(hZip, NULL);

  if(bStatus)
    an.SetProgress(_T("Finished compressing files...OK"), 100, true);
  else
    an.SetProgress(_T("File compression failed."), 100, true);

  return bStatus;
}

BOOL SendOverHTTP(SenderThreadContext* pc)
{  
  if(pc->m_sUrl.IsEmpty())
  {
    an.SetProgress(_T("No URL specified for sending error report over HTTP; skipping."), 0);
    return FALSE;
  }
  BOOL bSend = http.SendAssync(pc->m_sUrl, pc->m_sZipName, &an);  
  return bSend;
}

CString FormatEmailText(SenderThreadContext* pc)
{
  CString sFileTitle = pc->m_sZipName;
  sFileTitle.Replace('/', '\\');
  int pos = sFileTitle.ReverseFind('\\');
  if(pos>=0)
    sFileTitle = sFileTitle.Mid(pos+1);

  CString sText;

  sText += _T("This is an error report from ") + pc->m_sAppName + _T(" ") + pc->m_sAppVersion+_T(".\n\n");
 
  if(!pc->m_sEmailFrom.IsEmpty())
  {
    sText += _T("This error report was sent by ") + pc->m_sEmailFrom + _T(".\n");
    sText += _T("If you need additional info about the problem, you may want to contact this user again.\n\n");
  }     

  if(!pc->m_sEmailFrom.IsEmpty())
  {
    sText += _T("The user has provided the following problem description:\n<<< ") + pc->m_sEmailText + _T(" >>>\n\n");    
  }

  sText += _T("You may find detailed information about the error in files attached to this message:\n\n");
  sText += sFileTitle + _T(" is a ZIP archive which contains crash descriptor XML (crashrpt.xml), crash minidump (crashdump.dmp) ");
  sText += _T("and possibly other files that your application added to the crash report.\n\n");

  sText += sFileTitle + _T(".md5 file contains MD5 hash for the ZIP archive. You might want to use this file to check integrity of the error report.\n\n");
  
  sText += _T("For additional information about using error reports, see FAQ http://code.google.com/p/crashrpt/wiki/FAQ#Using_Error_Reports\n");

  return sText;
}

BOOL SendOverSMTP(SenderThreadContext* pc)
{  
  strconv_t strconv;

  if(pc->m_sEmailTo.IsEmpty())
  {
    an.SetProgress(_T("No E-mail address is specified for sending error report over SMTP; skipping."), 0);
    return FALSE;
  }
  msg.m_sFrom = (!pc->m_sEmailFrom.IsEmpty())?pc->m_sEmailFrom:pc->m_sEmailTo;
  msg.m_sTo = pc->m_sEmailTo;
  msg.m_sSubject = pc->m_sEmailSubject;
  msg.m_sText = FormatEmailText(pc);
  
  msg.m_aAttachments.insert(pc->m_sZipName);  

  // Create and attach MD5 hash file
  CString sErrorRptHash;
  CalcFileMD5Hash(pc->m_sZipName, sErrorRptHash);
  CString sFileTitle = pc->m_sZipName;
  sFileTitle.Replace('/', '\\');
  int pos = sFileTitle.ReverseFind('\\');
  if(pos>=0)
    sFileTitle = sFileTitle.Mid(pos+1);
  sFileTitle += _T(".md5");
  CString sTempDir;
  Utility::getTempDirectory(sTempDir);
  CString sTmpFileName = sTempDir +_T("\\")+ sFileTitle;
  FILE* f = NULL;
  _TFOPEN_S(f, sTmpFileName, _T("wt"));
  if(f!=NULL)
  {   
    LPCSTR szErrorRptHash = strconv.t2a(sErrorRptHash.GetBuffer(0));
    fwrite(szErrorRptHash, strlen(szErrorRptHash), 1, f);
    fclose(f);
    msg.m_aAttachments.insert(sTmpFileName);  
  }

  int res = smtp.SendEmailAssync(&msg, &an); 
  return (res==0);
}

BOOL SendOverSMAPI(SenderThreadContext* pc)
{  
  strconv_t strconv;

  if(pc->m_sEmailTo.IsEmpty())
  {
    an.SetProgress(_T("No E-mail address is specified for sending error report over Simple MAPI; skipping."), 0);
    return FALSE;
  }

  an.SetProgress(_T("Sending error report using Simple MAPI"), 0, false);
  an.SetProgress(_T("Initializing MAPI"), 1);

  BOOL bMAPIInit = mailmsg.MAPIInitialize();
  if(!bMAPIInit)
  {
    an.SetProgress(mailmsg.GetLastErrorMsg(), 100, false);
    return FALSE;
  }
  
  if(attempt!=0)
  {
    an.SetProgress(_T("[confirm_launch_email_client]"), 0);
    int confirm = 1;
    an.WaitForFeedback(confirm);
    if(confirm!=0)
    {
      an.SetProgress(_T("Cancelled by user"), 100, false);
      return FALSE;
    }
  }

  CString msg;
  CString sMailClientName;
  mailmsg.DetectMailClient(sMailClientName);
  
  msg.Format(_T("Launching the default email client (%s)"), sMailClientName);
  an.SetProgress(msg, 10);

  mailmsg.SetFrom(pc->m_sEmailFrom);
  mailmsg.SetTo(pc->m_sEmailTo);
  mailmsg.SetSubject(pc->m_sEmailSubject);
  CString sFileTitle = pc->m_sZipName;
  sFileTitle.Replace('/', '\\');
  int pos = sFileTitle.ReverseFind('\\');
  if(pos>=0)
    sFileTitle = sFileTitle.Mid(pos+1);
  mailmsg.SetMessage(FormatEmailText(pc));
  mailmsg.AddAttachment(pc->m_sZipName, sFileTitle);

  // Create and attach MD5 hash file
  CString sErrorRptHash;
  CalcFileMD5Hash(pc->m_sZipName, sErrorRptHash);
  sFileTitle += _T(".md5");
  CString sTempDir;
  Utility::getTempDirectory(sTempDir);
  CString sTmpFileName = sTempDir +_T("\\")+ sFileTitle;
  FILE* f = NULL;
  _TFOPEN_S(f, sTmpFileName, _T("wt"));
  if(f!=NULL)
  { 
    LPCSTR szErrorRptHash = strconv.t2a(sErrorRptHash.GetBuffer(0));
    fwrite(szErrorRptHash, strlen(szErrorRptHash), 1, f);
    fclose(f);
    mailmsg.AddAttachment(sTmpFileName, sFileTitle);  
  }

  BOOL bSend = mailmsg.Send();
  if(!bSend)
    an.SetProgress(mailmsg.GetLastErrorMsg(), 100, false);
  else
    an.SetProgress(_T("Sent OK"), 100, false);
  
  return bSend;
}

DWORD WINAPI SenderThread(LPVOID lpParam)
{
  SenderThreadContext* pc = (SenderThreadContext*)lpParam;

  int status = 1;

  BOOL bCompress = CompressReportFiles(pc);
  if(bCompress)
  {
    std::multimap<int, int> order;

    std::pair<int, int> pair3(pc->m_uPriorities[CR_SMAPI], CR_SMAPI);
    order.insert(pair3);

    std::pair<int, int> pair2(pc->m_uPriorities[CR_SMTP], CR_SMTP);
    order.insert(pair2);

    std::pair<int, int> pair1(pc->m_uPriorities[CR_HTTP], CR_HTTP);
    order.insert(pair1);

    std::multimap<int, int>::reverse_iterator rit;
    
    for(rit=order.rbegin(); rit!=order.rend(); rit++)
    {
      an.SetProgress(_T("[sending_attempt]"), 0);
      attempt++;    

      if(an.IsCancelled()){ break; }

      int id = rit->second;

      BOOL bResult = FALSE;

      if(id==CR_HTTP)
        bResult = SendOverHTTP(pc);    
      else if(id==CR_SMTP)
        bResult = SendOverSMTP(pc);  
      else if(id==CR_SMAPI)
        bResult = SendOverSMAPI(pc);

      if(bResult==FALSE)
        continue;

      if(id==CR_SMAPI && bResult==TRUE)
      {
        status = 0;
        break;
      }

      if(0==an.WaitForCompletion())
      {
        status = 0;
        break;
      }
    }
  }

  if(status==0)
  {
    an.SetProgress(_T("[status_success]"), 0);
    // Move the ZIP to Recycle Bin
    Utility::RecycleFile(pc->m_sZipName, false);
    Utility::RecycleFile(pc->m_sErrorReportDirName, false);
  }
  else
  {
    CString str;
    str.Format(_T("The error report is saved to '%s'"), pc->m_sZipName);
    an.SetProgress(str, 0);    
    an.SetProgress(_T("[status_failed]"), 0);    
  }

  an.SetCompleted(status);
  
  return 0;
}




DWORD WINAPI CollectorThread(LPVOID /*lpParam*/)
{ 
  BOOL bStatus = FALSE;
  CString str;
  CString sErrorReportDir = g_CrashInfo.m_sErrorReportDirName;
  CString sSrcFile;
  CString sDestFile;
  HANDLE hSrcFile = INVALID_HANDLE_VALUE;
  HANDLE hDestFile = INVALID_HANDLE_VALUE;
  LARGE_INTEGER lFileSize;
  BOOL bGetSize = FALSE;
  LPBYTE buffer[4096];
  LARGE_INTEGER lTotalWritten;
  DWORD dwBytesRead=0;
  DWORD dwBytesWritten=0;
  BOOL bRead = FALSE;
  BOOL bWrite = FALSE;

  BOOL bCreateMinidump = CreateMinidump();
  
  // Copy application-defined files that should be copied on crash
  an.SetProgress(_T("Start collecting information about the crash..."), 0, false);
  an.SetProgress(_T("[collecting_crash_info]"), 0, false);
  
  std::map<CString, FileItem>::iterator it;
  for(it=g_CrashInfo.m_FileItems.begin(); it!=g_CrashInfo.m_FileItems.end(); it++)
  {
    if(an.IsCancelled())
      goto cleanup;

    if(it->second.m_bMakeCopy)
    {
      str.Format(_T("Copying file %s."), it->second.m_sSrcFile);
      an.SetProgress(str, 0, false);
      
      hSrcFile = CreateFile(it->second.m_sSrcFile, GENERIC_READ, 
        FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
      if(hSrcFile==INVALID_HANDLE_VALUE)
      {
        str.Format(_T("Error opening file %s."), it->second.m_sSrcFile);
        an.SetProgress(str, 0, false);
      }
      
      bGetSize = GetFileSizeEx(hSrcFile, &lFileSize);
      if(!bGetSize)
      {
        str.Format(_T("Couldn't get file size of %s"), it->second.m_sSrcFile);
        an.SetProgress(str, 0, false);
        CloseHandle(hSrcFile);
        hSrcFile = INVALID_HANDLE_VALUE;
        continue;
      }

      sDestFile = sErrorReportDir + _T("\\") + it->second.m_sDestFile;
      
      hDestFile = CreateFile(sDestFile, GENERIC_WRITE, 
        FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
      if(hDestFile==INVALID_HANDLE_VALUE)
      {
        str.Format(_T("Error creating file %s."), sDestFile);
        an.SetProgress(str, 0, false);
        CloseHandle(hSrcFile);
        hSrcFile = INVALID_HANDLE_VALUE;
        continue;
      }

      lTotalWritten.QuadPart = 0;

      for(;;)
      {        
        if(an.IsCancelled())
          goto cleanup;
    
        bRead = ReadFile(hSrcFile, buffer, 4096, &dwBytesRead, NULL);
        if(!bRead || dwBytesRead==0)
          break;

        bWrite = WriteFile(hDestFile, buffer, dwBytesRead, &dwBytesWritten, NULL);
        if(!bWrite || dwBytesRead!=dwBytesWritten)
          break;

        lTotalWritten.QuadPart += dwBytesWritten;

        int nProgress = (int)(100.0f*lTotalWritten.QuadPart/lFileSize.QuadPart);

        an.SetProgress(nProgress, false);
      }

      if(lTotalWritten.QuadPart!=lFileSize.QuadPart)
        goto cleanup; // Error copying file

      CloseHandle(hSrcFile);
      hSrcFile = INVALID_HANDLE_VALUE;
      CloseHandle(hDestFile);
      hDestFile = INVALID_HANDLE_VALUE;
    }
  }

  // Success
  bStatus = TRUE;

cleanup:
  
  if(hSrcFile!=INVALID_HANDLE_VALUE)
    CloseHandle(hSrcFile);

  if(hDestFile!=INVALID_HANDLE_VALUE)
    CloseHandle(hDestFile);

  if(bStatus)
  {
    an.SetProgress(_T("Finished collecting information about the crash...OK"), 100, false);
    an.SetProgress(_T("[completed_collecting_crash_info]"), 0, true);
  }
  else
  {    
    an.SetProgress(_T("[status_exit_silently]"), 0, true);
  }

  return 0;
}

