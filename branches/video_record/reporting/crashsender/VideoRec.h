#pragma once
#include "stdafx.h"
#include "ScreenCap.h"
#include "vpx/vpx_encoder.h"

class CVideoRecorder
{
public:

	CVideoRecorder();
	~CVideoRecorder();

	BOOL RecordVideo(
			LPCTSTR szSaveToDir, 
			SCREENSHOT_TYPE type,
			DWORD dwProcessId,
			int nVideoDuration,
			int nVideoFrameInterval);

	BOOL EncodeVideo();

private:

	// Sets video frame parameters.
	void SetVideoFrameInfo(int nFrameId, ScreenshotInfo& ssi);

	// Loads a BMP file and returns its data.
	BOOL LoadImage(LPCTSTR szFileName, vpx_image_t *pImage);

	/* Internal variables */
	CString m_sSaveToDir; // Directory where to save recorded video frames.
	SCREENSHOT_TYPE m_ScreenshotType; // What part of desktop is captured.
	int m_nVideoDuration;
	int m_nVideoFrameInterval;
	std::vector<ScreenshotInfo> m_aVideoFrames; // Array of recorded video frames.
};
