#include "stdafx.h"
#include "DbManager.h"
#include "strconv.h"
#include "CrashRptProbe.h"

// Global pointer to DbMgr
CDbManager* g_pDbMgr = NULL;

CDbManager::CDbManager()
{
  m_pDb = NULL;
}

CDbManager::~CDbManager()
{
  CloseDatabase();
}

BOOL CDbManager::CreateDatabase(CString sFileName, BOOL bOpenExisting)
{
  BOOL bStatus = FALSE;
  int result = -1;
  strconv_t strconv;
  sqlite3_stmt* pStmt = NULL;
  
  CloseDatabase();

  LPCSTR szFileName = strconv.t2utf8(sFileName);
  result = sqlite3_open(szFileName, &m_pDb);
  if(result!=SQLITE_OK)
    goto cleanup;
    
  if(!ExecDDLQuery("BEGIN TRANSACTION;"))
    goto cleanup;
 
  char* szStmtGeneralInfo = 
    "CREATE TABLE GeneralInfo ( \
        CreatorVersion TEXT \
        );";

  if(!ExecDDLQuery(szStmtGeneralInfo))
    goto cleanup;

  char* szInsert1 = "INSERT INTO TABLE GeneralInfo VALUES ( %Q );";

  char* szStmtErrorReports = 
    "CREATE TABLE ErrorReports ( \
        CrashGUID TEXT PRIMARY KEY, \
        FileName TEXT  \
       );";

  if(!ExecDDLQuery(szStmtErrorReports))
    goto cleanup;

  if(!ExecDDLQuery("COMMIT TRANSACTION;"))
    goto cleanup;

  bStatus = TRUE;

cleanup:

  if(!bStatus)
  {
    CloseDatabase();
  }

  return bStatus;
}

void CDbManager::CloseDatabase()
{
  if(m_pDb!=NULL)
  {
    int result = sqlite3_close(m_pDb);
    assert(result==SQLITE_OK);
    m_pDb = NULL;
  }
}

BOOL CDbManager::ExecDDLQuery(char* szQuery)
{
  int result = -1;
  
  sqlite3_stmt* pStmt = NULL;

  result = sqlite3_prepare_v2(m_pDb, szQuery, -1, &pStmt, NULL); 
  if(result!=SQLITE_OK)
  {
    sqlite3_finalize(pStmt);
    return FALSE;
  }

  result = sqlite3_step(pStmt);  
  sqlite3_finalize(pStmt);
  if(result!=SQLITE_DONE)
    return FALSE;

  return TRUE;
}

BOOL CDbManager::ImportErrorReport(CString sFileName)
{
  

  return TRUE;
}
