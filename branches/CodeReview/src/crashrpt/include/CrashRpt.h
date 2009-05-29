/*! \file   CrashRpt.h
 *  \brief  Defines the interface for the CrashRpt.DLL.
 *  \date   2003-2009
 *  \author Copyright (c) 2003 Michael Carruth
 *  \author zexspectrum_1980@mail.ru
 *  \todo
 */

#ifndef _CRASHRPT_H_
#define _CRASHRPT_H_

#include <windows.h>

// CrashRpt.h
#ifdef CRASHRPT_EXPORTS
 #define CRASHRPTAPI __declspec(dllexport) 
#else 
 #define CRASHRPTAPI __declspec(dllimport) 
#endif

//! Current CrashRpt version
#define CRASHRPT_VER 1100

//! Client crash callback function prototype
typedef BOOL (CALLBACK *LPGETLOGFILE) (LPVOID lpvState);

//! Crash reporting general info used by crInstall()
typedef struct tagCR_INSTALL_INFO
{
  WORD cb;                       //!< Size of this structure in bytes; must be initialized before using!
  PCTSTR pszAppName;             //!< Name of application.
  PCTSTR pszAppVersion;          //!< Application version.
  PCTSTR pszEmailTo;             //!< E-mail address of crash reports recipient.
  PCTSTR pszEmailSubject;        //!< Subject of crash report e-mail. 
  PCTSTR pszCrashSenderPath;     //!< Directory name where CrashSender.exe is located.
  LPGETLOGFILE pfnCrashCallback; //!< User crash callback.
}
CR_INSTALL_INFO, *PCR_INSTALL_INFO;


//! Additional exception info used by crGenerateCrashReport()
typedef struct tagCR_EXCEPTION_INFO
{
  WORD cb;                //!< Size of this structure in bytes; should be initialized before using
  unsigned int code;      //!< Exception code
  unsigned int subcode;   //!< Exception subcode
}
CR_EXCEPTION_INFO, *PCR_EXCEPTION_INFO;


// Exception types for crEmulateCrash
#define CR_WIN32_NULL_POINTER_EXCEPTION 1    //!< Null pointer WIN32 exception
#define CR_CPP_TERMINATE_CALL           2    //!< C++ terminate() call
#define CR_CPP_UNEXPECTED_CALL          3    //!< C++ unexpected() call
#define CR_CPP_PURE_CALL                4    //!< C++ pure virtual function call
#define CR_CPP_SECURITY_ERROR           5    //!< Buffer overrun error
#define CR_CPP_INVALID_PARAMETER        6    //!< Invalid parameter exception
#define CR_CPP_NEW_OPERATOR_ERROR       7    //!< C++ new operator fault
#define CR_CPP_SIGABRT                  8    //!< C++ SIGABRT signal (abort)
#define CR_CPP_SIGFPE                   9    //!< C++ SIGFPE signal (flotating point exception)
#define CR_CPP_SIGILL                   10   //!< C++ SIGILL signal (illegal instruction)
#define CR_CPP_SIGINT                   11   //!< C++ SIGINT signal (CTRL+C)
#define CR_CPP_SIGSEGV                  12   //!< C++ SIGSEGV signal ()
#define CR_CPP_SIGTERM                  13   //!< C++ SIGTERM signal (termination request)

/*! \defgroup CrashRptAPI CrashRpt API */
/*! \defgroup DeprecatedAPI Obsolete Functions */

/*! \ingroup DeprecatedAPI
 *  \brief Installs exception handlers for the current process.
 *
 *  \param[in] pfnCallback Client crash callback
 *  \param[in] pszEmailTo Email address to send crash report
 *  \param[in] pszEmailSubject Subject line to be used with email
 *
 *  \return Always returns NULL.
 *
 *  \deprecated
 *    This function is deprecated. It is still supported for compatiblity with
 *    older versions of CrashRpt, however consider using crInstall() function instead.
 *    This function is currently implemented as a wrapper for crInstall().    
 *
 *  \remarks
 *
 *    This function installs unhandled exception filter for all threads of calling process.
 *    It also installs various C++ exception/error handlers. For the list of handlers,
 *    please see crInstall().
 *
 *    On crash, the error report is sent by E-mail using address and subject passed to the
 *    function as lpTo and lpSubject parameters, respectively. When E-mail client is not available,
 *    user is offered to save the report to disk as ZIP archive. 
 *
 *    Passing NULL for lpTo will disable the E-mail feature and cause the crash 
 *    report to be saved to disk by default.
 */

CRASHRPTAPI 
LPVOID 
Install(
   LPGETLOGFILE pfnCallback,
   LPCTSTR pszEmailTo,    
   LPCTSTR pszEmailSubject
   );

/*! \ingroup DeprecatedAPI
 *  \brief Uninstalls the exception filters set up by Install().
 *
 *  \param[in] lpState State information returned from Install(), can be NULL.
 *
 *  \reprecated
 *    This function is deprecated. It is still supported for compatiblity with
 *    older versions of CrashRpt, however consider using crInstall() function instead.
 *    This function is implemented as a wrapper for crUninstall().
  *
 *  \remarks
 *
 *    Call this function on application exit to uninstall all previously installed exception
 *    handlers.
 *
 *    The lpState parameter is unused and can be NULL.
 */

CRASHRPTAPI 
void 
Uninstall(
   IN LPVOID lpState                            // State from Install()
   );

/*! \ingroup DeprecatedAPI 
 *  \brief Adds a file to the crash report.
 *  
 *  \param[in] lpState State information returned from Install(), can be NULL.
 *  \param[in] lpFile  Fully qualified file name.
 *  \param[in] lpDesc  Description of file, used by Error Report Details dialog.
 *
 *  \deprecated
 *    This function is deprecated. It is still supported for compatiblity with
 *    older versions of CrashRpt, however consider using crAddFile() function instead.
 *    This function is implemented as a wrapper for crAddFile().
 *
 *  \remarks
 *
 *    This function can be called anytime after Install() to add one or more
 *    files to the generated crash report. However, the recommended way is to 
 *    call this function in crash callback.
 *  
 *    Function fails if lpFile doesn't exist at the moment of function call.
 *
 */

CRASHRPTAPI 
void 
AddFile(
   IN LPVOID lpState,                         
   IN LPCTSTR lpFile,                         
   IN LPCTSTR lpDesc                          
   );

/*! \ingroup DeprecatedAPI 
 *  \brief Generates the crash report.
 *  
 *  \param[in] lpState     State information returned from Install(), can be NULL.
 *  \param[im] pExInfo     Pointer to an EXCEPTION_POINTERS structure, can be NULL.
 * 
 *  \deprecated
 *    This function is deprecated. It is still supported for compatiblity with
 *    older versions of CrashRpt, however consider using crGenerateErrorReport() function instead.
 *    This function is implemented as a wrapper for crGenerateErrorReport().
 *
 *  \remarks
 *
 *    Call this function to manually generate a crash report.
 *
 *    The crash report contains the crash minidump, crash log in XML format and
 *    additional optional files added with AddFile().
 *
 *    If pExInfo is NULL, crash minidump might contain unusable information.
 *
 */

CRASHRPTAPI 
void 
//__declspec(deprecated("The GenerateErrorReport() function is deprecated. Consider using crGenerateErrorReport() instead."))
GenerateErrorReport(
   IN LPVOID lpState,
   IN PEXCEPTION_POINTERS pExInfo OPTIONAL
   );

/*! \ingroup CrashRptAPI 
 *  \brief  Installs exception handlers for current process and C++ exception handlers that
 *          function on per-process basis.
 *  \param
 *
 *    This function installs unhandled exception filter for all threads of calling process.
 *    It also installs various C++ exception/error handlers that function for all threads.
 *
 *    Below is the list of installed handlers:
 *     - WIN32 unhandled exception filter [ SetUnhandledExceptionFilter() ]
 *     - C++ pure virtual call handler [ _set_purecall_handler() ]
 *     - C++ invalid parameter handler [ _set_invalid_parameter_handler() ]
 *     - C++ new operator error handler [ _set_new_handler() ]
 *     - C++ buffer overrun handler (for old versions of CRT) [ _set_security_error_handler() ]
 *     - C++ abort handler [ signal(SIGABRT) ]
 *     - C++ floating point error handler [ signal(SIGFPE ]
 *     - C++ illegal instruction handler [ signal(SIGINT) ]
 *     - C++ termination request [ signal(SIGTERM) ]
 *
 *    On crash, the crash minidump file is created, which contains the CPU and 
 *    stack state information. Also XML file is created that contains additional 
 *    information that may be helpful for crash analysis.
 *
 *    When crash information is collected, another process CrashSender is launched 
 *    and the process where crash occur is terminated. The CrashSender process is 
 *    responsible for letting the user know about the crash and send the error report.
 * 
 *    The error report is sent by E-mail using address and subject passed to the
 *    function as CR_INSTALL_INFO structure members. When E-mail client is not available, 
 *    user is offered to save the report to disk as ZIP archive. 
 *
 */

CRASHRPTAPI 
int
crInstall(
  PCR_INSTALL_INFO pInfo
);

/*! \ingroup CrashRptAPI 
 *  \brief Unsinstalls exception handlers previously installed with crInstall().
 *
 */

CRASHRPTAPI 
int
crUninstall();

/*! \ingroup CrashRptAPI  
 *  \brief Installs C++ exception/error handlers for the current thread.
 *
 *  \return This function returns zero if succeeded.
 *   
 *  \remarks
 *   This call is needed when C++ exception mechanism is on (/EHsc compiler flag).
 *   This function sets C++ exception handlers for the caller thread. If you have
 *   several execution threads, you ought to call the function for each thread,
 *   except the main thread.
 *  
 *   The list of C++ exception\error handlers installed with this function:
 *    - terminate handler [ set_terminate() ]
 *    - unexpected handler [ set_unexpected() ]
 *    - illegal instruction handler [ signal(SIGILL) ]
 *    - illegal storage access handler [ signal(SIGSEGV) ]    
 *
 *   The crInstall() function automatically installs C++ exception handlers for the
 *   main thread, so no need to call crInstallToCurrentThread() for the main thread.
 *
 *   This function fails if calling it twice for the same thread.
 * 
 *   Call crUninstallFromCurrentThread() to uninstall C++ exception handlers from
 *   current thread.
 */

CRASHRPTAPI 
int 
crInstallToCurrentThread();

/*! \ingroup CrashRptAPI  
 *  \brief Uninstalls C++ exception handlers from the current thread.
 *  \return This function returns zero if succeeded.
 *  
 *  \remarks
 *    This call is needed when C++ exception mechanism is on (/EHsc compiler flag).
 *    This function unsets C++ exception handlers for the caller thread. If you have
 *    several execution threads, you ought to call the function for each thread.
 *    After calling this function, the C++ exception handlers for current thread are
 *    replaced with the handlers that were before call of crInstallToCurrentThread().
 *
 *    This function fails if crInstallToCurrentThread() wasn't called for current thread.
 *
 *    No need to call this function for the main execution thread. The crUninstall()
 *    will automatically uninstall C++ exception handlers for the main thread.
 */

CRASHRPTAPI 
int 
crUninstallFromCurrentThread();



/*! \ingroup CrashRptAPI  
 *  \brief Adds a file to crash report.
 * 
 *  \return This function returns zero if succeeded.
 *
 *  \param[in] pszFile Absolute path to the file to add.
 *  \param[in] pszDesc File description (used in Error Report Details dialog).
 *
 *    This function can be called anytime after Install() to add one or more
 *    files to the generated crash report. However, the recommended way is to 
 *    call this function in crash callback.
 *  
 *    Function fails if pszFile doesn't exist at the moment of function call.
 *
 */

CRASHRPTAPI 
int
crAddFile(
   PCTSTR pszFile,
   PCTSTR pszDesc 
   );




/*! \ingroup CrashRptAPI  
 *  \brief Manually generates an errror report.
 *
 *  \return This function returns zero if succeeded.
 *  
 *  \param[in] pExInfo Exception pointers.
 *  \param[in] pAdditionalInfo Additional information.
 *
 *  \remarks
 *    
 *
 */

CRASHRPTAPI 
int 
crGenerateErrorReport(
   _EXCEPTION_POINTERS* pExInfo,
   CR_EXCEPTION_INFO* pAdditionalInfo
   );


/*! \ingroup CrashRptAPI 
 *  \brief Can be used as C++ structured exception filter.
 *
 *  \return This function doesn't return if succeded.
 *
 *  \param[in] code Exception code.
 *  \param[in] ep   Exception pointers.
 *
 *  \remarks
 *     This function should be called instead of C++ structured exception filter
 *     inside of __try __except(Expression) statement. The function generates a error report
 *     and terminates calling process.
 *
 *     The exception code is usually retrieved with GetExceptionCode() intrinstic function
 *     and the exception pointers are retrieved with GetExceptionInformation() intrinstic 
 *     function.
 *
 *     If an error occurs, this function returns EXCEPTION_CONTINUE_SEARCH.
 */

CRASHRPTAPI
int 
crExceptionFilter(
  unsigned int code, 
  struct _EXCEPTION_POINTERS* ep);

/*! \ingroup CrashRptAPI  
 *  \brief Emulates a predefined crash situation.
 *
 */

CRASHRPTAPI
int
crEmulateCrash(
  unsigned ExceptionType);



#endif //_CRASHRPT_H_
