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


/*! \ingroup CrashRptProbeAPI
 *  \brief Opens a zipped crash report file.
 *
 *  \return This function returns zero on success. 
 *
 *  \param[in] pszFileName Zipped report file name.
 *  \param[in] pszMd5Hash String containing MD5 hash for the ZIP file.
 *  \param[in] dwFlags Flags, reserved for future use.
 *  \param[out] pHandle Handle to the opened crash report.
 *
 *  \remarks
 *
 *  \a pszFileName should be the name of an error report (ZIP file) to open. This parameter is required.
 *
 *  \a pszMd5Hash is a string containing the MD5 hash calculated for \a pszFileName. The MD5
 *  hash is used for integrity checks. If this parameter is NULL, integrity check is not performed.
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
 *  character versions of crpOpenCrashReport(). 
 *
 *  \sa 
 *    crpCloseErrorReport()
 */

int
CRASHRPTPROBE_API
crpOpenErrorReportW(
  LPCWSTR pszFileName,
  LPCWSTR pszMd5Hash,
  __reserved DWORD dwFlags,
  __out CrpHandle* phReport
);

/*! \ingroup CrashRptProbeAPI
 *  \copydoc crpOpenErrorReportW
 *
 */

int
CRASHRPTPROBE_API
crpOpenErrorReportA(
  LPCSTR pszFileName,
  LPCSTR pszMd5Hash,
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
 *  \param[in] hReport Handle to the crash report.
 *
 *  \remarks
 *
 */

int
CRASHRPTPROBE_API 
crpCloseErrorReport(
  CrpHandle hReport  
);

/* Property names passed to crpGetStrProperty() and crpGetIntProperty() */

//! Version of CrashRpt library that generated the report
#define CRP_PROP_CRASHRPT_VERSION    1 

#define CRP_PROP_CRASH_GUID          2 //!< Globally unique identifier of the error report (string)
#define CRP_PROP_APP_NAME            3 //!< Application name (string)
#define CRP_PROP_APP_VERSION         4 //!< Application version (string)
#define CRP_PROP_IMAGE_NAME          5 //!< Path to the executable file (string)
#define CRP_PROP_OPERATING_SYSTEM    6 //!< Opration system name, including build number and service pack (string)
#define CRP_PROP_SYSTEM_TIME_UTC     7 //!< Time (UTC) when the crash had occured (string)
#define CRP_PROP_EXCEPTION_TYPE      8 //!< Code of exception handler that cought the exception (long)
#define CRP_PROP_EXCEPTION_CODE      9 //!< Exception code; for the structured exceptions only (long)
#define CRP_PROP_INVPARAM_FUNCTION   10 //!< Function name; for invalid parameter errors only (string)
#define CRP_PROP_INVPARAM_EXPRESSION 11 //!< Expression; for invalid parameter errors only (string)
#define CRP_PROP_INVPARAM_FILE       12 //!< Source file name; for invalid parameter errors only (string)
#define CRP_PROP_INVPARAM_LINE       13 //!< Source line; for invalid parameter errors only (long)
#define CRP_PROP_FPE_SUBCODE         14 //!< Subcode of floating point exception; for FPE exceptions only (long)
#define CRP_PROP_USER_EMAIL          15 //!< Email of the user who sent this report (string)
#define CRP_PROP_PROBLEM_DESCRIPTION 16 //!< User-provided problem description (string)
#define CRP_PROP_FILE_COUNT          17 //!< Number of files contained in th error report (long)
#define CRP_PROP_FILE_ITEM_NAME      18 //!< Name of the file contained in the report (string)
#define CRP_PROP_FILE_ITEM_DESCRIPTION 19 //!< Description of the file contained in the report (string)

#define CRP_PROP_STACK_FRAME_COUNT      100 //!< Count of frames in stack trace (long)
#define CRP_PROP_STACK_MODULE_NAME      101 //!< Stack trace: module name (string)
#define CRP_PROP_STACK_SYMBOL_NAME      102 //!< Stack trace: symbol name (string)
#define CRP_PROP_STACK_OFFSET_IN_SYMBOL 103 //!< Stack trace: offset in symbol (long)
#define CRP_PROP_STACK_SOURCE_FILE      104 //!< Stack trace: source file name (string)
#define CRP_PROP_STACK_SOURCE_LINE      105 //!< Stack trace: source file line number (long)

#define CRP_PROP_CPU_ARCHITECTURE    106 //!<
#define CRP_PROP_CPU_NUMBER          107 //!<
#define CRP_PROP_SYSTEM_TYPE         108 //!<
#define CRP_PROP_OS_VER_MAJOR        109 //!<
#define CRP_PROP_OS_VER_MINOR        110 //!<
#define CRP_PROP_OS_VER_BUILD        111 //!<
#define CRP_PROP_OS_VER_CSD          112 //!< The latest service pack installed

#define CRP_PROP_EXCPTRS_EXCEPTION_CODE      113
#define CRP_PROP_EXCPTRS_EXCEPTION_ADDRESS   114

#define CRP_PROP_MODULE_NAME           115
#define CRP_PROP_MODULE_BASE_ADDRESS   116
#define CRP_PROP_MODULE_SIZE           117
#define CRP_PROP_MODULE_SYMBOLS_LOADED 118


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
 *  Use this function to retrieve data from the crash report previously opened the with crpOpenErrorReport() function.
 *
 *  CRP_PROP_CRASHRPT_VERSION
 *
 */ 

int
CRASHRPTPROBE_API 
crpGetPropertyW(
  CrpHandle hReport,
  INT nPropId,
  INT nIndex,
  __out_ecount_z(pcchBuffSize) LPWSTR lpszBuffer,
  ULONG cchBuffSize,
  __out PULONG pcchCount
);

/*! \ingroup CrashRptProbeAPI
 *  \copydoc crpGetPropertyW
 *
 */

int
CRASHRPTPROBE_API 
crpGetPropertyA(
  CrpHandle hReport,
  INT nPropId,
  INT nIndex,
  __out_ecount_z(pcchBuffSize) LPSTR lpszBuffer,
  ULONG pcchBuffSize,
  __out PULONG pcchCount
);

/*! \brief Character set-independent mapping of crpGetStrPropertyW() and crpGetStrPropertyA() functions. 
 *  \ingroup CrashRptProbeAPI
 */

#ifdef UNICODE
#define crpGetProperty crpGetPropertyW
#else
#define crpGetProperty crpGetPropertyA
#endif //UNICODE

/*! \ingroup CrashRptProbeAPI
 *  \brief
 *
 */

int
CRASHRPTPROBE_API 
crpExtractFileW(
  CrpHandle hReport,
  LPCWSTR lpszFileName,
  LPCWSTR lpszFileSaveAs
);

/*! \ingroup CrashRptProbeAPI
 *  \copydoc crpExtractFileW 
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


int
CRASHRPTPROBE_API
crpGetLastErrorMsgW(
  __out_ecount(cchBuffSize) LPTSTR pszBuffer, 
  __in UINT cchBuffSize);

int
CRASHRPTPROBE_API
crpGetLastErrorMsgA(
  __out_ecount(cchBuffSize) LPSTR pszBuffer, 
  __in UINT cchBuffSize);

#ifdef UNICODE
#define crpGetLastErrorMsg crpGetLastErrorMsgW
#else
#define crpGetLastErrorMsg crpGetLastErrorMsgA
#endif //UNICODE

#ifdef __cplusplus
}
#endif

#endif __CRASHRPT_PROBE_H__