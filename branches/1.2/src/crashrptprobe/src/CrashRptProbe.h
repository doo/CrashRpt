#ifndef __CRASHRPT_PROBE_H__
#define __CRASHRPT_PROBE_H__


#ifdef CRASHRPTPROBE_EXPORTS
#define CRASHRPTPROBE_API __declspec(dllexport)
#else
#define CRASHRPTPROBE_API __declspec(dllimport)
#endif

/*! \defgroup CrashRptProbeAPI */

/*! \ingroup CrashRptProbeAPI
 *  \brief Opens a zipped crash report file.
 *
 *  \return This function returns zero on success, else non-zero.
 *
 *  \param[in] lpszFileName Zipped report file name.
 *  \param[out] lpuHandle Handle to the opened crash report.
 *
 *  \remarks
 */

CRASHRPTPROBE_API int
craOpenCrashReportA(
  LPCSTR lpszFileName,
  LPUINT lpuHandle
);

#endif __CRASHRPT_PROBE_H__