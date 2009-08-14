#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include "CrashRptProbe.h"

int main(int argc, char** argv)
{
  CrpHandle handle = -1;
  int res = crpOpenCrashReport(_T("0c9a73e8-f080-4f04-99c8-c9e07d317df4.zip"), 
    _T("8f3303040ca1607ddb2660550bd81dd0"), &handle);



  return 0;
}