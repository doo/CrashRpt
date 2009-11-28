#ifndef __SCREENCAP_H__
#define __SCREENCAP_H__

extern "C" {
#include "png.h"
}

class CScreenCapture
{
public:

  void GetScreenRect(LPRECT rcScreen);
  int CaptureScreenRect(RECT rcCapture);

  BOOL PngInit(int nWidth, int nHeight, CString sFileName);
  BOOL PngWriteRow(LPBYTE pRow);
  BOOL PngFinalize();

private:

  FILE* m_fp;
  png_structp m_png_ptr;
  png_infop m_info_ptr;

};

#endif //__SCREENCAP_H__


