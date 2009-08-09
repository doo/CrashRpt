#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include "CrashRptProbe.h"

int main(int argc, char** argv)
{
  CrpHandle handle = -1;
  int res = crpOpenCrashReport(_T("abc.zip"), &handle);



  return 0;
}