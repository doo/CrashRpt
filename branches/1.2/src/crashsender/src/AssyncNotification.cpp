#include "stdafx.h"
#include "AssyncNotification.h"



AssyncNotification::AssyncNotification()
{
  m_nCompletionStatus = -1;    
  m_nPercentCompleted = 0;
  m_hCompletionEvent = CreateEvent(0, FALSE, FALSE, 0);
  m_hCancelEvent = CreateEvent(0, FALSE, FALSE, 0);
  m_hFeedbackEvent = CreateEvent(0, FALSE, FALSE, 0);
}

void AssyncNotification::SetProgress(CString sStatusMsg, int percentCompleted, bool bRelative)
{
  m_cs.Lock();

  m_statusLog.push_back(sStatusMsg);

  if(bRelative)
  {
    m_nPercentCompleted += percentCompleted;
    if(m_nPercentCompleted>100)
      m_nPercentCompleted = 100;      
  }
  else
    m_nPercentCompleted = percentCompleted;

  m_cs.Unlock();
}

void AssyncNotification::SetProgress(int percentCompleted, bool bRelative)
{
  m_cs.Lock();
  
  if(bRelative)
  {
    m_nPercentCompleted += percentCompleted;
    if(m_nPercentCompleted>100)
      m_nPercentCompleted = 100;      
  }
  else
    m_nPercentCompleted = percentCompleted;

  m_cs.Unlock();
}

void AssyncNotification::GetProgress(int& nProgressPct, std::vector<CString>& msg_log)
{
  msg_log.clear();
  
  m_cs.Lock();
  nProgressPct = m_nPercentCompleted;
  msg_log = m_statusLog;
  m_statusLog.clear();
  m_cs.Unlock();
}

void AssyncNotification::SetCompleted(int nCompletionStatus)
{
  m_cs.Lock();
  m_nCompletionStatus = nCompletionStatus;
  m_cs.Unlock();
  SetEvent(m_hCompletionEvent);
}

int AssyncNotification::WaitForCompletion()
{
  WaitForSingleObject(m_hCompletionEvent, INFINITE);
  
  int status = -1;
  m_cs.Lock();
  status = m_nCompletionStatus;
  m_cs.Unlock();

  return status;
}

void AssyncNotification::Cancel()
{
  SetProgress(_T("[cancelled_by_user]"), 0);
  SetEvent(m_hCancelEvent);
}

bool AssyncNotification::IsCancelled()
{
  DWORD dwWaitResult = WaitForSingleObject(m_hCancelEvent, 0);
  if(dwWaitResult==WAIT_OBJECT_0)
  {
    SetEvent(m_hCancelEvent);      
    return true;
  }
  
  return false;
}

void AssyncNotification::WaitForFeedback(int &code)
{
  ResetEvent(m_hFeedbackEvent);      
  WaitForSingleObject(m_hFeedbackEvent, INFINITE);
  m_cs.Lock();
  code = m_nCompletionStatus;
  m_cs.Unlock();
}

void AssyncNotification::FeedbackReady(int code)
{
  m_cs.Lock();
  m_nCompletionStatus = code;
  m_cs.Unlock();
  SetEvent(m_hFeedbackEvent);      
}
