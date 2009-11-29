#ifndef __SCREENCAP_H__
#define __SCREENCAP_H__

extern "C" {
#include "png.h"
}

#include <vector>

class CScreenCapture
{
public:

  void GetScreenRect(LPRECT rcScreen);
  BOOL CaptureScreenRect(RECT rcCapture, std::vector<CString>& out_file_list);

  BOOL PngInit(int nWidth, int nHeight, CString sFileName);
  BOOL PngWriteRow(LPBYTE pRow);
  BOOL PngFinalize();
  
private:

  FILE* m_fp;
  png_structp m_png_ptr;
  png_infop m_info_ptr;
  std::vector<CString> m_out_file_list;
};

#endif //__SCREENCAP_H__


