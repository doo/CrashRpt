/*! \file   CrashRptProbe.h
 *  \brief  Defines the interface for the CrashRptProbe.DLL.
 *  \date   2009
 *  \author zexspectrum@gmail.com
 */

#ifndef __CRASHRPT_PROBE_H__
#define __CRASHRPT_PROBE_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CRASHRPTPROBE_EXPORTS
#define CRASHRPTPROBE_API __declspec(dllexport) WINAPI
#else
#define CRASHRPTPROBE_API __declspec(dllimport) WINAPI
#endif

typedef int CrpHandle;

/*! \defgroup CrashRptProbeAPI CrashRptProbe Functions*/
/*! \defgroup CrashRptProbeEnums CrashRptProbe Enumerations*/

/*! \ingroup CrashRptProbeAPI
 *  \brief Opens a zipped crash report file.
 *
 *  \return This function returns zero on success. 
 *
 *  \param[in] pszFileName Zipped report file name.
 *  \param[in] pszMd5Hash String containing MD5 hash for the ZIP file data.
 *  \param[in] pszSymSearchPath Symbol files (PDB) search path.
 *  \param[in] dwFlags Flags, reserved for future use.
 *  \param[out] pHandle Handle to the opened crash report.
 *
 *  \remarks
 *
 *  Use this function to open a ZIP archive containing an error report. The error report typically contains
 *  several compressed files, such as XML crash descriptor, crash minidump file, and optionally 
 *  application-defined files.
 *
 *  \a pszFileName should be the name of the error report (ZIP file) to open. This parameter is required.
 *
 *  \a pszMd5Hash is a string containing the MD5 hash calculated for \a pszFileName. The MD5
 *  hash is used for integrity checks. If this parameter is NULL, integrity check is not performed.
 *
 *  If the error report is delivered by HTTP, the MD5 hash can be extracted by server-side script from the
 *  'md5' parameter. When the error report is delivered by email, the MD5 hash is attached to the mail message.
 *  The integrity check can be performed to ensure the error report was not corrupted during delivery.
 *  For more information, see \ref sending_error_report.
 *
 *  \a pszSymSearchPath parameter defines where to look for symbols files (PDB). You can specify the list of 
 *  semicolom-separated directories to search in. If this parameter is NULL, the default search path is used.
 *
 *  Symbol files are required for crash report processing. They contain various information used by the debugger.
 *
 *  \a dwFlags is currently not used, should be zero.
 *
 *  \a pHandle parameter receives the handle to the opened crash report. If the function fails,
 *  this parameter becomes zero. 
 *
 *  On failure, use crpGetLastErrorMsg() function to get the last error message.
 *
 *  Use the crpCloseErrorReport() function to close the opened error report.
 *
 *  \note
 *
 *  The crpOpenErrorReportW() and crpOpenErrorReportA() are wide character and multibyte 
 *  character versions of crpOpenErrorReport(). 
 *
 *  The following example shows how to open an error report file:
 *
 * \code
 *   #include <CrashRptProbe.h>
 *   #include <stdio.h>
 *   ...
 *   
 *   CrpHandle hReport = 0;
 *   int result = crpOpenErrorReport(
 *                 _T("0b3b0c1b-3450-4c39-9459-42221ae66460.zip"), // Zip archive name
 *                 _T("2e4345603454a345064371ab34195316"), // MD5 hash for the file
 *                 _T("D:\\MyApp\\1.3.4\\Sym; D:\\MyApp\\1.3.5\\Sym"), // Where to look for symbols
 *                 0, // Reserved
 *                 &hReport
 *                 );
 *
 *  if(result!=0)
 *  {
 *    TCHAR szErrorMsg[256];
 *    crpGetLastErrorMsg(szErrorMsg, 256);
 *    return;
 *  }
 *
 *  // Do something with it...
 *
 *  // Finally, close the report
 *  crpCloseErrorReport(hReport);
 * \endcode
 *
 *  \sa 
 *    crpCloseErrorReport()
 */

int
CRASHRPTPROBE_API
crpOpenErrorReportW(
  __in LPCWSTR pszFileName,
  __in_opt LPCWSTR pszMd5Hash,
  __in_opt LPCWSTR pszSymSearchPath,
  __reserved DWORD dwFlags,
  __out CrpHandle* phReport
);

/*! \ingroup CrashRptProbeAPI
 *  \copydoc crpOpenErrorReportW()
 *
 */

int
CRASHRPTPROBE_API
crpOpenErrorReportA(
  __in LPCSTR pszFileName,
  __in_opt LPCSTR pszMd5Hash,
  __in_opt LPCSTR pszSymSearchPath,  
  __reserved DWORD dwFlags,
  __out CrpHandle* phReport
);

/*! \brief Character set-independent mapping of crpOpenErrorReportW() and crpOpenErrorReportA() functions. 
 *  \ingroup CrashRptProbeAPI
 */

#ifdef UNICODE
#define crpOpenErrorReport crpOpenErrorReportW
#else
#define crpOpenErrorReport crpOpenErrorReportA
#endif //UNICODE

/*! 
 *  \brief Closes the crash report.
 *  \return This function returns zero if successful, else non-zero.
 *  \param[in] hReport Handle to the opened error report.
 *
 *  \remarks
 *
 *  Use this function to close the error report previously opened with crpOpenErrorReport()
 *  function.
 *
 *  If this function fails, use crpGetLastErrorMsg() function to get the error message.
 *
 *  See crpOpenErrorReportW() for code example. 
 *
 *  \sa
 *    crpOpenErrorReport(), crpOpenErrorReportW(), crpOpenErrorReportA(), crpGetLastErrorMsg()
 */

int
CRASHRPTPROBE_API 
crpCloseErrorReport(
  CrpHandle hReport  
);

/*! \ingroup CrashRptProbeEnums
 *  \brief Property names passed to crpGetProperty() function. 
 *
 *  \remarks
 *
 *  An error report can be presented as a set of string properties. 
 *
 *  Some properties are simple (scalar values), while others are complex (vector values).
 *  Vector properties can be represented as sequences of simple properties.
 *  
 *  For example, the \c CRP_PROP_CRASHRPT_VERSION property is simple, because its value
 *  looks like "1103" (a number). The \c CRP_PROP_CRASH_GUID is also simple. Its value
 *  looks like "0b3b0c1b-3450-4c39-9459-42221ae66460" (a string).
 *
 *  You can think of vector properties as of tables containing rows of simple properties. 
 *  For example, \c CRP_PROP_STACK_MODULE_NAME represent the module name loaded by your application. 
 *  Since your application has many modules, the names of modules can be stored in a table. Each cell
 *  of the table can be accessed by its zero-based index.
 *
 *  Properties retrieveing the list of files contained in error report, the stack trace and the 
 *  list of modules are vectored, others are simple.
 *
 *  For code example, see crpGetPropertyW() function.
 */

enum CRP_ErrorReportProperty
{
  CRP_PROP_CRASHRPT_VERSION        = 1,  //!< Version of CrashRpt library that generated the report (e.g., "1103").
  CRP_PROP_CRASH_GUID              = 2,  //!< Globally unique identifier (GUID) of the error report (e.g., "0b3b0c1b-3450-4c39-9459-42221ae66460").
  CRP_PROP_APP_NAME                = 3,  //!< Application name (e.g., "MyApp").
  CRP_PROP_APP_VERSION             = 4,  //!< Application version (e.g., "1.3.5").
  CRP_PROP_IMAGE_NAME              = 5,  //!< Path to the executable file (e.g., "C:\\Program Files\\MyApp\\myapp.exe").
  CRP_PROP_OPERATING_SYSTEM        = 6,  //!< Opration system name, including build number and service pack (e.g., "Windows XP Build 2600 Service Pack 3").
  CRP_PROP_SYSTEM_TIME_UTC         = 7,  //!< Time (UTC) when the crash had occured (e.g., "").
  CRP_PROP_EXCEPTION_TYPE          = 8,  //!< Code of exception handler that cought the exception (e.g., "0").
  CRP_PROP_EXCEPTION_CODE          = 9,  //!< Exception code; for the structured exceptions only, hexadecimal (e.g., "0xC000005").
  CRP_PROP_INVPARAM_FUNCTION       = 10, //!< Function name; for invalid parameter errors only (e.g., "strcpy_s").
  CRP_PROP_INVPARAM_EXPRESSION     = 11, //!< Expression; for invalid parameter errors only (e.g., "size!=0").
  CRP_PROP_INVPARAM_FILE           = 12, //!< Source file name; for invalid parameter errors only (e.g., "memory.c").
  CRP_PROP_INVPARAM_LINE           = 13, //!< Source line; for invalid parameter errors only (e.g., "320").
  CRP_PROP_FPE_SUBCODE             = 14, //!< Subcode of floating point exception; for FPE exceptions only (e.g., "237").
  CRP_PROP_USER_EMAIL              = 15, //!< Email of the user who sent this report (e.g., "example@hotmail.com").
  CRP_PROP_PROBLEM_DESCRIPTION     = 16, //!< User-provided problem description (e.g., "I did something").
  
  CRP_PROP_FILE_COUNT              = 17, //!< Number of files contained in th error report (e.g., "2").
  CRP_PROP_FILE_ITEM_NAME          = 18, //!< File list: Name of the file contained in the report, vectored (e.g., "crasdhump.dmp").
  CRP_PROP_FILE_ITEM_DESCRIPTION   = 19, //!< File list: Description of the file contained in the report (e.g., "Crash Dump").

  CRP_PROP_STACK_FRAME_COUNT       = 100, //!< Count of frames in the stack trace (e.g., "10").
  CRP_PROP_STACK_MODULE_NAME       = 101, //!< Stack trace: module name (e.g., "ntdll.dll").
  CRP_PROP_STACK_SYMBOL_NAME       = 102, //!< Stack trace: symbol name (e.g., "RtlCopyMemory").
  CRP_PROP_STACK_OFFSET_IN_SYMBOL  = 103, //!< Stack trace: offset in symbol, hexadecimal (e.g., "0x23e").
  CRP_PROP_STACK_SOURCE_FILE       = 104, //!< Stack trace: source file name (e.g., "CrashRpt.cpp").
  CRP_PROP_STACK_SOURCE_LINE       = 105, //!< Stack trace: source file line number (e.g., "509").

  CRP_PROP_CPU_ARCHITECTURE        = 201, //!< Processor architecture.
  CRP_PROP_CPU_COUNT               = 202, //!< Number of processors.
  CRP_PROP_SYSTEM_TYPE             = 203, //!< Type of system (server or workstation).
  CRP_PROP_OS_VER_MAJOR            = 204, //!< OS major version.
  CRP_PROP_OS_VER_MINOR            = 205, //!< OS minor version.
  CRP_PROP_OS_VER_BUILD            = 206, //!< OS build number.
  CRP_PROP_OS_VER_CSD              = 207, //!< The latest service pack installed.

  CRP_PROP_EXCPTRS_EXCEPTION_CODE    = 300, //!< Code of the structured exception.
  CRP_PROP_EXCPTRS_EXCEPTION_ADDRESS = 301, //!< Exception address.

  CRP_PROP_MODULE_COUNT            = 401, //!< Count of modules.
  CRP_PROP_MODULE_NAME             = 402, //!< Module name.
  CRP_PROP_MODULE_BASE_ADDRESS     = 403, //!< Module base load address.
  CRP_PROP_MODULE_SIZE             = 404, //!< Module size.
  CRP_PROP_MODULE_SYMBOLS_LOADED   = 405  //!< Were symbols loaded for the module or not?
};


/*! \ingroup CrashRptProbeAPI
 *  \brief Retrieves a string property from crash report.
 *  \return This function returns zero on success.
 *
 *  \param[in]  hReport Handle to the previously opened crash report.
 *  \param[in]  nPropId Property ID.
 *  \param[in]  nIndex Index of the property in the table.
 *  \param[out] lpszBuffer Output buffer.
 *  \param[in]  cchBuffSize Size of output buffer.
 *  \param[out] pcchCount Count of characters written to the buffer.
 *
 *  \remarks
 *
 *  Use this function to retrieve data from the crash report previously opened with the
 *  crpOpenErrorReport() function.
 *
 *  \a hReport should be the handle to the opened error report.
 *
 *  \a nPropId represents the ID of the property to retrieve. For the list of available 
 *  properties, see CRP_ErrorReportProperty() enumeration.
 *
 *  \a nIndex defines the zero-based index of the table row to retrieve (for vectored properties).
 *  
 *  \a lpszBuffer defines the buffer where retrieved property value will be placed. If this parameter
 *  is NULL, it is ignored and \pcchCount is set with the required size of the buffer (count of bytes).
 *
 *  \a cchBuffSize defines the buffer size. To calculate required buffer size, set \a lpszBuffer with NULL, 
 *  the function will set \pcchCount with the number of bytes required.
 *
 *  \a pcchCount is set with the actual count of bytes copied to the \a lpszBuffer. If this parameter is NULL,
 *  it is ignored.
 *
 *  If this function fails, use crpGetLastErrorMsg() function to get the error message.
 *
 *  The following example shows how to retrieve a simple property:
 *
 *  \code
 *  const int BUFF_SIZE = 1024;
 *  TCHAR szBuffer[BUFF_SIZE];
 *
 *  // Get the CrashRpt version that generated the error report
 *  int result = crpGetProperty(CRP_CRASHRPT_VERSION, 0, szBuffer, BUFF_SIZE, NULL);
 *  _tprintf(_T("CrashRpt ver: %s\n"), szBuffer);
 *  
 *  // Get application name
 *  result = crpGetProperty(CRP_APP_NAME, 0, szBuffer, BUFF_SIZE, NULL);
 *  _tprintf(_T("Application name: %s\n"), szBuffer);
 *
 *  \endcode
 *
 *  The following example shows how to retrieve the stack trace from the error report:
 *
 *  \code
 *  CrpHandle hReport; // Handle to the opened error report (should be opened previously).
 *
 *  // Retrieve the stack trace. This will help to see where exception occurred.
 *  const int BUFF_SIZE = 1024;
 *  TCHAR szBuffer[BUFF_SIZE];
 *  
 *  // Get count of stack frames
 *  int result = crpGetProperty(hReport, CRP_PROP_STACK_FRAME_COUNT, 0, szBuffer, BUFF_SIZE, NULL);
 *  _tprintf(_T("Count of frames in the error report: %s\n"), szBuffer);
 *  
 *  // Cast to long
 *  long nStackFramesCount = atol(szBuffer);
 *
 *  int i;
 *  for(i=0; i<nStackFrameCount; i++)
 *  {
 *    // Get module name
 *    int result = crpGetProperty(hReport, CRP_APP_NAME, 0, szBuffer, BUFF_SIZE, NULL);
 *    _tprintf(_T("%d. %s!\n"), i+1, szBuffer);
 *
 *    // Get symbol name
 *    int result = crpGetProperty(hReport, CRP_PROP_STACK_MODULE_NAME, 0, szBuffer, BUFF_SIZE, NULL);
 *    _tprintf(_T("%s+"), szBuffer);
 *
 *    // Get offset in symbol
 *    int result = crpGetProperty(hReport, CRP_PROP_STACK_OFFSET_IN_SYMBOL, 0, szBuffer, BUFF_SIZE, NULL);
 *    _tprintf(_T("%s\n"), szBuffer);
 *  }
 *  \endcode
 *
 *  Example output:
 *  \code
 *  1. ntdll.dll!RtlCopyMemory+0x42
 *  2. CrashRptTest.exe!EmulateCrash+0xa4
 *  3. ....
 *  \endcode
 *
 *  \note
 *  The crpGetPropertyW() and crpGetPropertyA() are wide character and multibyte 
 *  character versions of crpGetProperty(). 
 *
 *  \sa
 *    crpGetPropertyW(), crpGetPropertyA(), crpOpenErrorReport(), crpGetLastErrorMsg()
 */ 

int
CRASHRPTPROBE_API 
crpGetPropertyW(
  CrpHandle hReport,
  CRP_ErrorReportProperty nPropId,
  INT nIndex,
  __out_ecount_z(pcchBuffSize) LPWSTR lpszBuffer,
  ULONG cchBuffSize,
  __out PULONG pcchCount
);

/*! \ingroup CrashRptProbeAPI
 *  \copydoc crpGetPropertyW()
 *
 */

int
CRASHRPTPROBE_API 
crpGetPropertyA(
  CrpHandle hReport,
  CRP_ErrorReportProperty nPropId,
  INT nIndex,
  __out_ecount_z(pcchBuffSize) LPSTR lpszBuffer,
  ULONG pcchBuffSize,
  __out PULONG pcchCount
);

/*! \brief Character set-independent mapping of crpGetPropertyW() and crpGetPropertyA() functions. 
 *  \ingroup CrashRptProbeAPI
 */

#ifdef UNICODE
#define crpGetProperty crpGetPropertyW
#else
#define crpGetProperty crpGetPropertyA
#endif //UNICODE

/*! \ingroup CrashRptProbeAPI
 *  \brief Extracts a file from the opened error report.
 *  \return This function returns zero if succeeded.
 *
 *  \param[in] hReport Handle to the opened error report.
 *  \param[in] lpszFileName The name of the file to extract.
 *  \param[in] lpszFileSaveAs The resulting name of the extracted file.
 *
 *  \remarks
 *
 *  Use this function to extract a contained file from the error report (ZIP) file.
 *
 *  If this function fails, use crpGetLastErrorMsg() to retrieve the error message.
 *
 *  \note
 *    The crpExtractFileW() and crpExtractFileA() are wide character and multibyte 
 *    character versions of crpExtractFile(). 
 *
 *  \sa
 *    crpExtractFileA(), crpExtractFileW(), crpExtractFile()
 */

int
CRASHRPTPROBE_API 
crpExtractFileW(
  CrpHandle hReport,
  LPCWSTR lpszFileName,
  LPCWSTR lpszFileSaveAs
);

/*! \ingroup CrashRptProbeAPI
 *  \copydoc crpExtractFileW() 
 */

int
CRASHRPTPROBE_API 
crpExtractFileA(
  CrpHandle hReport,
  LPCSTR lpszFileName,
  LPCSTR lpszFileSaveAs
);

/*! \brief Character set-independent mapping of crpExtractFileW() and crpExtractFileA() functions. 
 *  \ingroup CrashRptProbeAPI
 */

#ifdef UNICODE
#define crpExtractFile crpExtractFileW
#else
#define crpExtractFile crpExtractFileA
#endif //UNICODE

/*! \ingroup CrashRptProbeAPI 
 *  \brief Gets the last CrashRptProbe error message.
 *
 *  \return This function returns length of error message in characters.
 *
 *  \param[out] pszBuffer Pointer to the buffer.
 *  \param[in]  uBuffSize Size of buffer in characters.
 *
 *  \remarks
 *
 *  This function gets the last CrashRptProbe error message. You can use this function
 *  to retrieve the text status of the last called CrashRptProbe function.
 *
 *  If buffer is too small for the error message, the message is truncated.
 *
 *  \note 
 *    crpGetLastErrorMsgW() and crpGetLastErrorMsgA() are wide-character and multi-byte character versions
 *    of crpGetLastErrorMsg(). The crGetLastErrorMsg() macro defines character set independent mapping.
 *
 *  The following example shows how to use crpGetLastErrorMsg() function.
 *
 *  \code
 *  
 *  // .. call some CrashRptProbe function
 *
 *  // Get the status message
 *  TCHAR szErrorMsg[256];
 *  crpGetLastErrorMsg(szErrorMsg, 256);
 *  \endcode
 *
 *  \sa crpGetLastErrorMsgA(), crpGetLastErrorMsgW(), crpGetLastErrorMsg()
 */

int
CRASHRPTPROBE_API
crpGetLastErrorMsgW(
  __out_ecount(cchBuffSize) LPTSTR pszBuffer, 
  __in UINT cchBuffSize);

/*! \ingroup CrashRptProbeAPI
 *  \copydoc crpGetLastErrorMsgW()
 *
 */

int
CRASHRPTPROBE_API
crpGetLastErrorMsgA(
  __out_ecount(cchBuffSize) LPSTR pszBuffer, 
  __in UINT cchBuffSize);

/*! \brief Defines character set-independent mapping for crpGetLastErrorMsgW() and crpGetLastErrorMsgA().
 *  \ingroup CrashRptProbeAPI
 */

#ifdef UNICODE
#define crpGetLastErrorMsg crpGetLastErrorMsgW
#else
#define crpGetLastErrorMsg crpGetLastErrorMsgA
#endif //UNICODE

#ifdef __cplusplus
}
#endif

#endif __CRASHRPT_PROBE_H__