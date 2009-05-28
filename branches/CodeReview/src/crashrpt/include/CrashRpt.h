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

// Client crash callback
typedef BOOL (CALLBACK *LPGETLOGFILE) (LPVOID lpvState);

typedef struct tagCRASHRPT_INFO
{
  WORD cb;              // Size of this structure in bytes
  PCTSTR pszAppName;    // Name of application
  PCTSTR pszAppVersion; // Application version
  PCTSTR pszEmailTo;    // E-mail address of crash reports recipient
  PCTSTR pszEmailSubject; // Subject of crash report e-mail 
  LPGETLOGFILE pfnCrashCallback;
}
CRASHRPT_INFO, *PCRASHRPT_INFO;

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
__declspec(deprecated("The Install() function is deprecated. Consider using crInstall() instead of it."))
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
__declspec(deprecated("The Uninstall() function is deprecated. Consider using crUninstall() instead of it."))
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
  PCRASHRPT_INFO pInfo,
  LPVOID* ppState
);

//-----------------------------------------------------------------------------
// crUninstall
//
//

CRASHRPTAPI 
int
crUninstall(LPVOID ppState);

//-----------------------------------------------------------------------------
// crInstallToCurrentThread
//   Installs C++ exception handlers for the current thread.
//
// Parameters
//    lpState     State information returned from Install()
//
// Remarks 
//    This call is needed when C++ exception mechanism is on (/EHsc compiler flag).
//    This function sets C++ exception handlers for the caller thread. If you have
//    several execution threads, you ought to call the function for each thread.

CRASHRPTAPI 
int 
crInstallToCurrentThread(
  IN LPVOID lpState);

//-----------------------------------------------------------------------------
// crUninstallToCurrentThread
//   Uninstalls C++ exception handlers from the current thread.
//
// Parameters
//    lpState     State information returned from Install()
//
// Remarks 
//    This call is needed when C++ exception mechanism is on (/EHsc compiler flag).
//    This function unsets C++ exception handlers for the caller thread. If you have
//    several execution threads, you ought to call the function for each thread.
//    After calling this functions the C++ exception handlers for current thread are
//    replaced with the handlers that were before call of crInstallToCurrentThread().

CRASHRPTAPI 
int 
crUninstallFromCurrentThread(
  IN LPVOID lpState);


#endif
