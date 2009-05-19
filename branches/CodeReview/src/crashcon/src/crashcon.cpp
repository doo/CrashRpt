// crashcon.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <process.h>
#include "CrashRpt.h"

LPVOID lpvState = NULL;

int filter(unsigned int code, struct _EXCEPTION_POINTERS* ep)
{
  code; // this is to avoid  C4100 unreferenced formal parameter warning
  GenerateErrorReport(lpvState, ep);
  return EXCEPTION_EXECUTE_HANDLER;
}

int main(int argc, char* argv[])
{
  argc; // this is to avoid C4100 unreferenced formal parameter warning
  argv; // this is to avoid C4100 unreferenced formal parameter warning
  
  // Install crash reporting
  lpvState = Install(NULL, NULL, NULL);

#ifdef _DEBUG
   printf("Press a ENTER to simulate a null pointer exception...\n");
   getchar();
   __try
   {
      RaiseException(EXCEPTION_BREAKPOINT, 0, 0, NULL);
   } 
   __except(filter(GetExceptionCode(), GetExceptionInformation()))
   {
   }
#else
   printf("Press a ENTER to generate a null pointer exception...\n");
   getchar();
   int *p = 0;
   *p = 0;
#endif // _DEBUG
	return 0;
}

