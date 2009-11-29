#include "stdafx.h"
#include "ScreenCap.h"
#include <tchar.h>
#include "Utility.h"

// This function is used for monitor enumeration
BOOL CALLBACK EnumMonitorsProc(HMONITOR hMonitor, HDC /*hdcMonitor*/, LPRECT lprcMonitor, LPARAM dwData)
{	
  CScreenCapture* psc = (CScreenCapture*)dwData;

	MONITORINFOEX mi;
	HDC hDC = NULL;  
  HBITMAP hBitmap = NULL;
  BITMAPINFO bmi;    
  int nWidth = 0;
  int nHeight = 0;
  int nRowWidth = 0;
  LPBYTE pRowBits = NULL;
  CString sFileName;

  // Get monitor info
  mi.cbSize = sizeof(MONITORINFOEX);
	GetMonitorInfo(hMonitor, &mi);
  
	// Get the device context for this monitor
	hDC = CreateDC(_T("DISPLAY"), mi.szDevice, NULL, NULL); 
	if(hDC==NULL)
    goto cleanup;

  hBitmap = (HBITMAP)GetCurrentObject(hDC, OBJ_BITMAP);  
  if(hBitmap==NULL)
    goto cleanup;

  // Get monitor size
	nWidth = lprcMonitor->right - lprcMonitor->left;
	nHeight = lprcMonitor->bottom - lprcMonitor->top;
	
  // Init PNG writer
  sFileName = Utility::getTempFileName();
  BOOL bInit = psc->PngInit(nWidth, nHeight, sFileName);
  if(!bInit)
    goto cleanup;

  // We will get bitmap bits row by row
  nRowWidth = nWidth*3 + 10;
  pRowBits = new BYTE[nRowWidth];
  if(pRowBits==NULL)
    goto cleanup;
    
  memset(&bmi.bmiHeader, 0, sizeof(BITMAPINFOHEADER));
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); 
  bmi.bmiHeader.biWidth = nWidth;
  bmi.bmiHeader.biHeight = -nHeight;
  bmi.bmiHeader.biBitCount = 24;
  bmi.bmiHeader.biPlanes = 1;  

  int i;
  for(i=nHeight-1; i>=0; i--)
  {
    
    int nFetched = GetDIBits(hDC, hBitmap, i, 1, pRowBits, &bmi, DIB_RGB_COLORS);
    if(nFetched!=1)
      break;

    BOOL bWrite = psc->PngWriteRow(pRowBits);
    if(!bWrite)
      goto cleanup;   
  }
  
  psc->PngFinalize();
  
cleanup:

  // Clean up
  if(hDC)
    DeleteDC(hDC);

  if(pRowBits)
    delete [] pRowBits;

  // Next monitor
	return TRUE;
}


BOOL CScreenCapture::CaptureScreenRect(RECT rcCapture, std::vector<CString>& out_file_list)
{		
	EnumDisplayMonitors(NULL, &rcCapture, EnumMonitorsProc, (LPARAM)this);	
  out_file_list = m_out_file_list;
	return TRUE;
}

void CScreenCapture::GetScreenRect(LPRECT rcScreen)
{
	int nWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	int nHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	rcScreen->left = GetSystemMetrics(SM_XVIRTUALSCREEN);
	rcScreen->top = GetSystemMetrics(SM_YVIRTUALSCREEN);
	rcScreen->right = rcScreen->left + nWidth;
	rcScreen->bottom = rcScreen->top + nHeight;
}

BOOL CScreenCapture::PngInit(int nWidth, int nHeight, CString sFileName)
{  
  m_fp = NULL;
  m_png_ptr = NULL;
  m_info_ptr = NULL;

  m_out_file_list.push_back(sFileName);

#if _MSC_VER>=1400
  _tfopen_s(&m_fp, sFileName.GetBuffer(0), _T("wb"));
#else
  m_fp = _tfopen(sFileName.GetBuffer(0), _T("wb"));
#endif

  if (!m_fp)
  {    
    return FALSE;
  }

  m_png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 
    (png_voidp)NULL, NULL, NULL);
  if (!m_png_ptr)
    return FALSE;

  m_info_ptr = png_create_info_struct(m_png_ptr);
  if (!m_info_ptr)
  {
    png_destroy_write_struct(&m_png_ptr, (png_infopp)NULL);
    return FALSE;
  }

  /* Error handler*/
  if (setjmp(png_jmpbuf(m_png_ptr)))
  {
    png_destroy_write_struct(&m_png_ptr, &m_info_ptr);
    fclose(m_fp);
    return FALSE;
  }

  png_init_io(m_png_ptr, m_fp);

  /* set the zlib compression level */
  png_set_compression_level(m_png_ptr, Z_BEST_COMPRESSION);

  /* set other zlib parameters */
  png_set_compression_mem_level(m_png_ptr, 8);
  png_set_compression_strategy(m_png_ptr, Z_DEFAULT_STRATEGY);
  png_set_compression_window_bits(m_png_ptr, 15);
  png_set_compression_method(m_png_ptr, 8);
  png_set_compression_buffer_size(m_png_ptr, 8192);

  png_set_IHDR(
    m_png_ptr, 
    m_info_ptr, 
    nWidth, //width, 
    nHeight, //height,
    8, // bit_depth
    PNG_COLOR_TYPE_RGB, // color_type
    PNG_INTERLACE_NONE, // interlace_type
    PNG_COMPRESSION_TYPE_DEFAULT, 
    PNG_FILTER_TYPE_DEFAULT);

  png_set_bgr(m_png_ptr);

  /* write the file information */
  png_write_info(m_png_ptr, m_info_ptr);

  return TRUE;
}

BOOL CScreenCapture::PngWriteRow(LPBYTE pRow)
{
  png_bytep rows[1] = {pRow};
  png_write_rows(m_png_ptr, (png_bytepp)&rows, 1);
  return TRUE;
}

BOOL CScreenCapture::PngFinalize()
{
  /* end write */
  png_write_end(m_png_ptr, m_info_ptr);

  /* clean up */
  png_destroy_write_struct(&m_png_ptr, (png_infopp)&m_info_ptr);
  
  if(m_fp)
    fclose(m_fp);

  return TRUE;
}







