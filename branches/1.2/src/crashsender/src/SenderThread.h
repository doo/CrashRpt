#pragma once
#include <vector>
#include <dbghelp.h>
#include "AssyncNotification.h"
#include "MailMsg.h"
#include "smtpclient.h"
#include "httpsend.h"

class CErrorReportSender
{
public:

  CErrorReportSender();
  ~CErrorReportSender();

  BOOL DoWork();

  void WaitForCompletion();

  void GetStatus(int& nProgressPct, std::vector<CString>& msg_log);
  void Cancel();
  void FeedbackReady(int code);

private:

  void DoWorkAssync();
  static DWORD WINAPI WorkerThread(LPVOID lpParam);  
  
  BOOL CollectCrashFiles();
  int CalcFileMD5Hash(CString sFileName, CString& sMD5Hash);
  BOOL TakeDesktopScreenshot();
  BOOL CreateMiniDump();
  static BOOL CALLBACK MiniDumpCallback(PVOID CallbackParam, const PMINIDUMP_CALLBACK_INPUT CallbackInput,
                PMINIDUMP_CALLBACK_OUTPUT CallbackOutput); 
  BOOL CompressReportFiles();

  BOOL SendReport();
  BOOL SendOverHTTP();
  CString FormatEmailText();
  BOOL SendOverSMTP();
  BOOL SendOverSMAPI();

  HANDLE m_hThread;  // Handle to the worker thread
  int m_SendAttempt; // Number of current sending attempt
  AssyncNotification m_Assync; // Used for communication with the main thread
  CEmailMessage m_EmailMsg;    // Email message to send
  CSmtpClient m_SmtpClient;    // Used to send report over SMTP 
  CHttpRequestSender m_HttpSender;    // Used to send report over HTTP
  CMailMsg m_MapiSender;       // Used to send report over SMAPI
  CString m_sZipName;          // Name of the ZIP archive to send
};

extern CErrorReportSender g_ErrorReportSender;