#ifndef __SCREENCAP_H__
#define __SCREENCAP_H__

extern "C" {
#include "png.h"
}

#include <vector>

class CScreenCapture
{
public:

  CScreenCapture();

  void GetScreenRect(LPRECT rcScreen);
  BOOL CaptureScreenRect(RECT rcCapture, CString sSaveDirName, int nIdStartFrom, std::vector<CString>& out_file_list);

  BOOL PngInit(int nWidth, int nHeight, CString sFileName);
  BOOL PngWriteRow(LPBYTE pRow);
  BOOL PngFinalize();

  /* Member variables. */

  int m_nIdStartFrom;
  CString m_sSaveDirName;
  FILE* m_fp;
  png_structp m_png_ptr;
  png_infop m_info_ptr;  
  std::vector<CString> m_out_file_list;
};

#endif //__SCREENCAP_H__


