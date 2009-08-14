#include "stdafx.h"
#include "CrashDescReader.h"
#include "tinyxml.h"

CCrashDescReader::CCrashDescReader()
{
  m_bLoaded = false;
}

CCrashDescReader::~CCrashDescReader()
{
}

int CCrashDescReader::Load(CString sFileName)
{
  USES_CONVERSION;
  TiXmlDocument doc;

  LPCSTR szFileName = T2A(sFileName.GetBuffer(0));
  bool bLoaded = doc.LoadFile(szFileName);
  if(!bLoaded)
    return 1;

  

  m_bLoaded = true;
  return 0;
}

