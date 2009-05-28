///////////////////////////////////////////////////////////////////////////////
//
//  Module: CrashRpt.h
//
//    Desc: Defines the interface for the CrashRpt.DLL.
//
// Copyright (c) 2003 Michael Carruth
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _CRASHRPT_H_
#define _CRASHRPT_H_

#include <windows.h>

// CrashRpt.h
#ifdef CRASHRPT_EXPORTS
 #define CRASHRPTAPI __declspec(dllexport) 
#else 
 #define CRASHRPTAPI __declspec(dllimport) 
#endif

// Current CrashRpt version (1.1.0)
#define CRASHRPT_VER 1100

// Client crash callback function prototype
typedef BOOL (CALLBACK *LPGETLOGFILE) (LPVOID lpvState);

// Crash reporting general info
typedef struct tagCR_INSTALL_INFO
{
  WORD cb;                       // Size of this structure in bytes
  PCTSTR pszAppName;             // Name of application
  PCTSTR pszAppVersion;          // Application version
  PCTSTR pszEmailTo;             // E-mail address of crash reports recipient
  PCTSTR pszEmailSubject;        // Subject of crash report e-mail 
  PCTSTR pszCrashSenderPath;     // Directory name where CrashSender.exe is located
  LPGETLOGFILE pfnCrashCallback; // User crash callback
}
CR_INSTALL_INFO, *PCR_INSTALL_INFO;


// Additional exception info 
typedef struct tagCR_EXCEPTION_INFO
{
  WORD cb;                // Size of this structure in bytes; should be initialized before using
  unsigned int code;      // Exception code
  unsigned int subcode;   // Exception subcode
}
CR_EXCEPTION_INFO, *PCR_EXCEPTION_INFO;


// Exception types for crRaiseException
#define CR_WIN32_NULL_POINTER_EXCEPTION 1
#define CR_CPP_TERMINATE_CALL           2
#define CR_CPP_UNEXPECTED_CALL          3
#define CR_CPP_PURE_CALL                4
#define CR_CPP_SECURITY_ERROR           5
#define CR_CPP_INVALID_PARAMETER        6
#define CR_CPP_NEW_OPERATOR_ERROR       7
#define CR_CPP_SIGABRT                  8
#define CR_CPP_SIGFPE                   9
#define CR_CPP_SIGILL                   10
#define CR_CPP_SIGINT                   11
#define CR_CPP_SIGSEGV                  12
#define CR_CPP_SIGTERM                  13



//-----------------------------------------------------------------------------
// Install
//    Note: This function is deprecated. It is still supported for compatiblity with
//    older versions of CrashRpt, however consider using crInstall() function instead.
//
//    Initializes the library and optionally set the client crash callback and
//    sets up the email details.
//
// Parameters
//    pfn         Client crash callback
//    lpTo        Email address to send crash report
//    lpSubject   Subject line to be used with email
//
// Return Values
//    If the function succeeds, the return value is a pointer to the underlying
//    crash object created.  This state information is required as the first
//    parameter to all other crash report functions.
//
// Remarks
//    Passing NULL for lpTo will disable the email feature and cause the crash 
//    report to be saved to disk.
//
CRASHRPTAPI 
LPVOID 
__declspec(deprecated("The Install() function is deprecated. Consider using crInstall() instead."))
Install(
   IN LPGETLOGFILE pfn OPTIONAL,                // client crash callback
   IN LPCTSTR lpTo OPTIONAL,                    // Email:to
   IN LPCTSTR lpSubject OPTIONAL                // Email:subject
   );


//-----------------------------------------------------------------------------
// Uninstall
//    Uninstalls the unhandled exception filter set up in Install().
//
// Parameters
//    lpState     State information returned from Install()
//
// Return Values
//    void
//
// Remarks
//    This call is optional.  The crash report library will automatically 
//    deinitialize when the library is unloaded.  Call this function to
//    unhook the exception filter manually.
//
CRASHRPTAPI 
void 
__declspec(deprecated("The Uninstall() function is deprecated. Consider using crUninstall() instead."))
Uninstall(
   IN LPVOID lpState                            // State from Install()
   );

//-----------------------------------------------------------------------------
// AddFile
//    Adds a file to the crash report.
//
// Parameters
//    lpState     State information returned from Install()
//    lpFile      Fully qualified file name
//    lpDesc      Description of file, used by details dialog
//
// Return Values
//    void
//
// Remarks
//    This function can be called anytime after Install() to add one or more
//    files to the generated crash report.
//
CRASHRPTAPI 
void 
__declspec(deprecated("The AddFile() function is deprecated. Consider using crAddFile() instead."))
AddFile(
   IN LPVOID lpState,                           // State from Install()
   IN LPCTSTR lpFile,                           // File name
   IN LPCTSTR lpDesc                            // File desc
   );

//-----------------------------------------------------------------------------
// GenerateErrorReport
//    Generates the crash report.
//
// Parameters
//    lpState     State information returned from Install()
//    pExInfo     Pointer to an EXCEPTION_POINTERS structure
//
// Return Values
//    void
//
// Remarks
//    Call this function to manually generate a crash report.
//
CRASHRPTAPI 
void 
__declspec(deprecated("The GenerateErrorReport() function is deprecated. Consider using crGenerateErrorReport() instead."))
GenerateErrorReport(
   IN LPVOID lpState,
   IN PEXCEPTION_POINTERS pExInfo OPTIONAL
   );

//-----------------------------------------------------------------------------
// crInstall
//
//

CRASHRPTAPI 
int
crInstall(
  PCR_INSTALL_INFO pInfo
);

//-----------------------------------------------------------------------------
// crUninstall
//
//

CRASHRPTAPI 
int
crUninstall();

//-----------------------------------------------------------------------------
// crInstallToCurrentThread
//   Installs C++ exception handlers for the current thread.
//
//
// Remarks 
//    This call is needed when C++ exception mechanism is on (/EHsc compiler flag).
//    This function sets C++ exception handlers for the caller thread. If you have
//    several execution threads, you ought to call the function for each thread.

CRASHRPTAPI 
int 
crInstallToCurrentThread();

//-----------------------------------------------------------------------------
// crUninstallToCurrentThread
//   Uninstalls C++ exception handlers from the current thread.
//
//
// Remarks 
//    This call is needed when C++ exception mechanism is on (/EHsc compiler flag).
//    This function unsets C++ exception handlers for the caller thread. If you have
//    several execution threads, you ought to call the function for each thread.
//    After calling this functions the C++ exception handlers for current thread are
//    replaced with the handlers that were before call of crInstallToCurrentThread().

CRASHRPTAPI 
int 
crUninstallFromCurrentThread();

//-----------------------------------------------------------------------------
// crAddFile

CRASHRPTAPI 
int
crAddFile(
   PCTSTR pszFile,
   PCTSTR pszDesc 
   );

//-----------------------------------------------------------------------------
// crGenerateErrorReport

CRASHRPTAPI 
int 
crGenerateErrorReport(
   _EXCEPTION_POINTERS* pExInfo = NULL,
   CR_EXCEPTION_INFO* pAdditionalInfo = NULL
   );

//-----------------------------------------------------------------------------
// crExceptionFilter

CRASHRPTAPI
int 
crExceptionFilter(
  unsigned int code, 
  struct _EXCEPTION_POINTERS* ep);

//-----------------------------------------------------------------------------
// crRaiseException

CRASHRPTAPI
int
crEmulateCrash(
  unsigned ExceptionType);



#endif //_CRASHRPT_H_
