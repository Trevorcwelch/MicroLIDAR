//// Camera_10_fps.cpp : Defines the entry point for the console application.

#include "stdafx.h"
#include <Windows.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "InitializeCameraFunctions.h"

#include "lucamapi.h"
#include "lucamerr.h"
#include "lucamsci.h"


using namespace std;


// Forward declarations of functions in this file
bool TakeAPicture(const HANDLE hCamera, unsigned char * pRawBuffer);
void PrintError(char const * const pErrMsg);
void PrintLastCameraError(const HANDLE hCamera, char const * const pMessage);
bool initialize_camera(void* hCamera, LUCAM_SNAPSHOT&);
bool enable_snapshot_mode(void* handle, LUCAM_SNAPSHOT&);
bool disable_snapshot_mode(void* handle);
unsigned char* frame_memory_allocation(void* hCamera, const int imageWidth, const int imageHeight, const int pixelFormat);
bool get_image_format(void* hCamera, LUCAM_IMAGE_FORMAT& imf);
bool save_image(void* hCamera, const LUCAM_IMAGE_FORMAT& imf, unsigned char* imageBuffer, const char* pFileName);
bool process_raw_frame(void* hCamera, unsigned char* pRawBuffer, unsigned char* pRgbBuffer, LUCAM_IMAGE_FORMAT& imf, LUCAM_CONVERSION& params);

int main(int argc, char* argv[])
{
	const HANDLE hCamera = LucamCameraOpen(1);
	if (NULL == hCamera) {
		printf("ERROR: Unable to open a camera.\n");
		return EXIT_FAILURE;
	}

	bool success = true;
	int pictureNum = 0;
	const char * fileName = "54321A.bmp";

	// Color conversion parameters.
	LUCAM_CONVERSION conversionParams;
	conversionParams.CorrectionMatrix = LUCAM_CM_FLUORESCENT;
	conversionParams.DemosaicMethod = LUCAM_DM_HIGHER_QUALITY;


	// Empty Image format
	LUCAM_IMAGE_FORMAT imf;
	memset(&imf, 0, sizeof(imf));
	imf.Size = sizeof(imf);

	LUCAM_SNAPSHOT snapSettings;
	initialize_camera(hCamera, snapSettings);
	// Change any snapSettings here (exposure, gain)


	// Enabling fast frame is expensive, so if you can enable and do all the require capture then disable more efficient.
	if (!enable_snapshot_mode(hCamera, snapSettings)) return false;

	// Memory allocation here do not need to repeate this all the time.
	unsigned char* pRawBuffer = frame_memory_allocation(hCamera, snapSettings.format.width / snapSettings.format.subSampleX, snapSettings.format.height / snapSettings.format.subSampleY, snapSettings.format.pixelFormat);
	if (!pRawBuffer) return false;

	unsigned char* pRgbBuffer = frame_memory_allocation(hCamera, snapSettings.format.width / snapSettings.format.subSampleX, snapSettings.format.height / snapSettings.format.subSampleY, LUCAM_PF_24);

	if (!pRgbBuffer)
	{
		delete pRawBuffer;
		return false;
	}

	while (pictureNum++ <5) {
		success &= TakeAPicture(hCamera, pRawBuffer);
		if (success)
			success &= get_image_format(hCamera, imf);
		if (success)
			success &= process_raw_frame(hCamera, pRawBuffer, pRgbBuffer, imf, conversionParams);
		if (success)
			success &= save_image(hCamera, imf, pRgbBuffer, fileName++);
	}
	disable_snapshot_mode(hCamera);
	LucamCameraClose(hCamera);
	delete pRawBuffer;
	delete pRgbBuffer;
	return (success) ? EXIT_SUCCESS : EXIT_FAILURE;
}

static void
PrintError(char const * const pErrMsg)
{
	printf("Error: %s\n", pErrMsg);
}


static void
PrintLastCameraError(const HANDLE hCamera, char const * const pMessage)
{
	const ULONG error = LucamGetLastErrorForCamera(hCamera);
	char buf[1024];
	sprintf_s(buf, "%s (Error code %u)\n", pMessage, (unsigned int)error);
	PrintError(&buf[0]);
}

static ULONG
GetPixelSize(const int pixelFormat)
{
	switch (pixelFormat)
	{
	case LUCAM_PF_8:	return  8 / 8;
	case LUCAM_PF_16:	return 16 / 8;
	case LUCAM_PF_24:	return 24 / 8;
	case LUCAM_PF_32:	return 32 / 8;
	case LUCAM_PF_48:	return 48 / 8;
	}
	return 0;
}

static bool
TakeAPicture(const HANDLE hCamera, unsigned char* pRawBuffer)
{
	if (!LucamTakeFastFrame(hCamera, pRawBuffer)) {
		PrintLastCameraError(hCamera, "Unable to capture the image.");
		return false;
	}
	return true;
}


bool initialize_camera(void* hCamera, LUCAM_SNAPSHOT& cam)
{
	// Get Camera current frame format.
	LUCAM_FRAME_FORMAT frameFormat;

	float frameRate;
	if (!LucamGetFormat(hCamera, &frameFormat, &frameRate)) PrintLastCameraError(hCamera, "could not read actual frame format on camera.");

	//LUCAM_PROP_UNIT_HEIGHT 
	frameFormat.height = 756;
	//frameFormat.yOffset = 1037;


	frameFormat.width = getGoodWidth(hCamera, frameFormat.pixelFormat, 1, frameFormat.width);
	frameFormat.height = getGoodHeight(hCamera, frameFormat.pixelFormat, frameFormat.subSampleY, frameFormat.height);

	if (!LucamSetFormat(hCamera, &frameFormat, frameRate)) PrintLastCameraError(hCamera, "could not set camera format to updated frame format.");


	//exposure and reserved properies (reserved always zero)
	cam.bufferlastframe = FALSE;
	cam.exposure = 2;
	cam.exposureDelay = 0;
	cam.flReserved1 = 0;
	cam.flReserved2 = 0;
	cam.ulReserved1 = 0;
	cam.ulReserved2 = 0;

	//Initializing of the LUCAM_FRAME_FORMAT struct
	cam.format = frameFormat;

	//Gain, shuttertype, and strobe properties
	cam.gain = 10.0;
	LONG flags;
	if (!LucamGetProperty(hCamera, LUCAM_PROP_GAIN_BLUE, &cam.gainBlue, &flags)) cam.gainBlue = 1.0;
	if (!LucamGetProperty(hCamera, LUCAM_PROP_GAIN_RED, &cam.gainRed, &flags)) cam.gainRed = 1.0;
	if (!LucamGetProperty(hCamera, LUCAM_PROP_GAIN_GREEN1, &cam.gainGrn1, &flags)) cam.gainGrn1 = 1.0;
	if (!LucamGetProperty(hCamera, LUCAM_PROP_GAIN_GREEN2, &cam.gainGrn2, &flags)) cam.gainGrn2 = 1.0;
	cam.shutterType = LUCAM_SHUTTER_TYPE_GLOBAL;
	cam.strobeDelay = 1.0;
	cam.useStrobe = FALSE;
	cam.useHwTrigger = FALSE;
	cam.timeout = 500;

	return true;
}

bool enable_snapshot_mode(void* hCamera, LUCAM_SNAPSHOT& snapSettings)
{
	if (!LucamEnableFastFrames(hCamera, &snapSettings)) {
		PrintLastCameraError(hCamera, "Unable to enable fast frame.");
		return false;
	}
	return true;
}

bool disable_snapshot_mode(void* hCamera)
{
	if (!LucamDisableFastFrames(hCamera)) {
		PrintLastCameraError(hCamera, "Unable to disable fast frame.");
		return false;
	}
	return true;

}

// it is responsibility of user to free allocated memory by this function.
unsigned char* frame_memory_allocation(void* hCamera, const int imageWidth, const int imageHeight, const int pixelFormat)
{
	const ULONG numPixels = imageWidth * imageHeight;
	const ULONG pixelSize = GetPixelSize(pixelFormat);
	if (0 == pixelSize) {
		PrintError("The camera is using a pixel format unsupported by this program.");
		return nullptr;
	}
	const ULONG imageSize = numPixels * pixelSize;

	BYTE * pRawBuffer = new BYTE[imageSize * pixelSize];
	if (!pRawBuffer) {
		PrintError("Unable to allocate buffer for image\n");
		return nullptr;
	}
	return pRawBuffer;
}

bool process_raw_frame(void* hCamera, unsigned char* pRawBuffer, unsigned char* pRgbBuffer, LUCAM_IMAGE_FORMAT& imf, LUCAM_CONVERSION& params)
{
	if (!LucamConvertFrameToRgb24(hCamera, pRgbBuffer, pRawBuffer, imf.Width, imf.Height, imf.PixelFormat, &params)) {
		PrintLastCameraError(hCamera, "Unable to format the raw image");
		return false;
	}
	imf.PixelFormat = LUCAM_PF_24;
	return true;
}

bool get_image_format(void* hCamera, LUCAM_IMAGE_FORMAT& imf)
{
	if (!LucamGetStillImageFormat(hCamera, &imf)) {
		PrintLastCameraError(hCamera, "Unable to read image format from camera.");
		return false;
	}
	return true;
}

bool save_image(void* hCamera, const LUCAM_IMAGE_FORMAT& imf, unsigned char* imageBuffer, const char* pFileName)
{
	if (!LucamSaveImageEx(hCamera, imf.Width, imf.Height, imf.PixelFormat, imageBuffer, pFileName)) {
		std::stringstream ss;
		ss << "Unable to save the image to file {" << pFileName << "}";
		PrintLastCameraError(hCamera, ss.str().c_str());
		return false;
	}

	printf("Image successfully saved to '%s'\n", pFileName);
	return true;
}

bool is_camera_color(void* hCamera)
{
	float tmp;
	long flag;

	if (!LucamGetProperty(hCamera, LUCAM_PROP_COLOR_FORMAT, &tmp, &flag)) return false;
	return (static_cast<ULONG>(tmp) != LUCAM_CF_MONO);
}