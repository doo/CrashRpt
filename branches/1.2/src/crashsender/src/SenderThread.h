#pragma once
#include <vector>

class CErrorReportSender
{
public:

  CErrorReportSender();
  ~CErrorReportSender();

private:
  
  void GetSenderThreadStatus(int& nProgressPct, std::vector<CString>& msg_log);
  void CancelSenderThread();
  void FeedbackReady(int code);

  static DWORD WINAPI CollectorThread(LPVOID lpParam);
  static DWORD WINAPI SenderThread(LPVOID lpParam);
};
