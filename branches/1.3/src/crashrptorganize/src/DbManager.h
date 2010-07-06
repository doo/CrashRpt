#pragma once
#include "stdafx.h"

class CDbManager
{
public:

  CDbManager();
  ~CDbManager();

  BOOL CreateDatabase(CString sFileName, BOOL bOpenExisting);
  void CloseDatabase();

private:


};


