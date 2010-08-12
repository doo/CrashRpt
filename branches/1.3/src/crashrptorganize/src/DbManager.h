#pragma once
#include "stdafx.h"
#include "sqlite3.h"

class CDbManager
{
public:

  CDbManager();
  ~CDbManager();

  BOOL CreateDatabase(CString sFileName, BOOL bOpenExisting);
  void CloseDatabase();

private:

  BOOL ExecDDLQuery(char* szQuery);
  BOOL ImportErrorReport(CString sFileName);

  sqlite3* m_pDb;
  CString m_sLastError;
  CString m_sSymSearchPath; 
};

extern CDbManager* g_pDbMgr;

