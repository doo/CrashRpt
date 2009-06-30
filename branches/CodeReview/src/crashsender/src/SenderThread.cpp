#include "stdafx.h"
#include "SenderThread.h"
#include "MailMsg.h"
#include "smtpclient.h"
#include "httpsend.h"
#include "CrashRpt.h"

int attempt = 0;
AssyncNotification an;
CEmailMessage msg;
CSmtpClient smtp;  
CHttpSender http;
CMailMsg mailmsg;

void GetSenderThreadStatus(int& nProgressPct, std::vector<CString>& msg_log)
{
  msg_log.clear();
  an.m_cs.Lock();
  nProgressPct = an.m_nPercentCompleted;
  msg_log = an.m_statusLog;
  an.m_statusLog.clear();
  an.m_cs.Unlock();  
}

void CancelSenderThread()
{
  an.Cancel();
}

void FeedbackReady(int code)
{
  an.FeedbackReady(code);
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

BOOL SendOverSMTP(SenderThreadContext* pc)
{  
  if(pc->m_sEmailTo.IsEmpty())
  {
    an.SetProgress(_T("No E-mail address is specified for sending error report over SMTP; skipping."), 0);
    return FALSE;
  }
  msg.m_sFrom = (!pc->m_sEmailFrom.IsEmpty())?pc->m_sEmailFrom:pc->m_sEmailTo;
  msg.m_sTo = pc->m_sEmailTo;
  msg.m_sSubject = pc->m_sEmailSubject;
  msg.m_sText = pc->m_sEmailText;
  msg.m_aAttachments.insert(pc->m_sZipName);  
  int res = smtp.SendEmailAssync(&msg, &an); 
  return (res==0);
}

BOOL SendOverSMAPI(SenderThreadContext* pc)
{  
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

  an.SetProgress(_T("Launching the default email client"), 10);
  
  mailmsg.SetFrom(pc->m_sEmailFrom);
  mailmsg.SetTo(pc->m_sEmailTo);
  mailmsg.SetSubject(pc->m_sEmailSubject);
  CString sFileTitle = pc->m_sZipName;
  sFileTitle.Replace('/', '\\');
  int pos = sFileTitle.ReverseFind('\\');
  if(pos>=0)
    sFileTitle = sFileTitle.Mid(pos+1);
  mailmsg.SetMessage(pc->m_sEmailText);
  mailmsg.AddAttachment(pc->m_sZipName, sFileTitle);
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

  an.m_hCompletionEvent = CreateEvent(0, FALSE, FALSE, 0);
  an.m_hCancelEvent = CreateEvent(0, FALSE, FALSE, 0);
  an.m_hFeedbackEvent = CreateEvent(0, FALSE, FALSE, 0);

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

    WaitForSingleObject(an.m_hCompletionEvent, INFINITE);
    if(an.m_nCompletionStatus==0)
    {
      status = 0;
      break;
    }
  }

  if(status==0)
  {
    an.SetProgress(_T("[status_success]"), 0);      
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


