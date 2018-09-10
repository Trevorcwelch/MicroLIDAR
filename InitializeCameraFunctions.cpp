#include "stdafx.h"
#include "InitializeCameraFunctions.h"


bool is16Bit(const uint32_t pixelFormat)
{
	if ((pixelFormat == LUCAM_PF_16) || (pixelFormat == LUCAM_PF_48))
		return true;
	return false;
}

bool isBinningSupported(void* camera)
{
	if (camera)
	{

		LUCAM_SS_BIN_DESC sampling;
		if (LucamGetSubsampleBinDescription(camera, &sampling))
		{
			return (sampling.binNot1Count > 0);
		}
	}
	return false;
}

std::vector<uint32_t> getBinningFactor(void* camera)
{
	std::vector<uint32_t> list;
	if (camera)
	{
		list.push_back(1);
		LUCAM_SS_BIN_DESC sampling;
		if (LucamGetSubsampleBinDescription(camera, &sampling))
		{
			if (sampling.binNot1Count > 0)
			{
				for (int i = 0; i < sampling.binNot1Count; i++)
				{
					list.push_back(sampling.binFormatsNot1[i]);
				}
			}
		}
	}
	return list;
}

bool isSubSamplinggSupported(void* camera)
{
	if (camera)
	{

		LUCAM_SS_BIN_DESC sampling;
		if (LucamGetSubsampleBinDescription(camera, &sampling))
		{
			return (sampling.ssFormatsNot1 > 0);
		}
	}
	return false;
}
std::vector<uint32_t> getSubSamplingFactor(void* camera)
{
	std::vector<uint32_t> list;
	if (camera)
	{
		list.push_back(1);
		LUCAM_SS_BIN_DESC sampling;
		if (LucamGetSubsampleBinDescription(camera, &sampling))
		{
			if (sampling.ssNot1Count > 0)
			{
				for (int i = 0; i < sampling.ssNot1Count; i++)
				{
					list.push_back(sampling.ssFormatsNot1[i]);
				}
			}
		}
	}
	return list;
}

float getValue(void* camera, const uint32_t prop)
{
	float value = 0.0f;
	long flags = 0;
	if (camera)
	{
		if (!LucamGetProperty(camera, prop, &value, &flags))
			value = 0.0f;
	}
	return value;
}

uint32_t getMaxWidth(void* camera)
{
	return static_cast<uint32_t>(getValue(camera, LUCAM_PROP_MAX_WIDTH));
}

uint32_t getMaxHeight(void* camera)
{
	return static_cast<uint32_t>(getValue(camera, LUCAM_PROP_MAX_HEIGHT));
}

uint32_t getMinWidth(void* camera, const int pixelFormat, const int samplingFactor)
{
	uint32_t min = 0;
	unsigned long id;
	try
	{
		float value;
		long flags;
		if (LucamGetProperty(camera, LUCAM_PROP_UNIT_WIDTH, &value, &flags))
		{
			min = static_cast<uint32_t>(value);
			if (!LucamGetCameraId(camera, &id))
			{
				throw std::exception("Cannot read camera id.");
			}
			if (((id & 0x640) == 640) || ((id & 0x740) == 740))
			{
				min = 256;
				if (is16Bit(pixelFormat))
					min = 128;
			}
			min *= samplingFactor;
		}
	}
	catch (...)
	{
		min = 0;
	}
	return min;
}

uint32_t getMinHeight(void* camera, const int pixelFormat, const int samplingFactor)
{
	uint32_t min = 0;
	try
	{
		float value;
		long flags;
		if (LucamGetProperty(camera, LUCAM_PROP_UNIT_HEIGHT, &value, &flags))
		{
			min = static_cast<uint32_t>(value);
			min *= samplingFactor;
		}
	}
	catch (...)
	{
		min = 0;
	}
	return min;
}

uint32_t toSmallestMultiple(const uint32_t& desiredValue, const uint32_t& multiple)
{
	double value = desiredValue / (static_cast<double>(multiple));
	value = floor(value);
	value *= multiple;
	return static_cast<uint32_t>(value);
}

uint32_t toBiggestMultiple(const uint32_t& desiredValue, const uint32_t& multiple)
{
	double value = desiredValue / (static_cast<double>(multiple));
	value = ceil(value);
	value *= multiple;
	return static_cast<uint32_t>(value);
}

uint32_t getGoodWidth(void* camera, const int pixelFormat, const int samplingFactor, const uint32_t& desiredWidth)
{
	uint32_t good = 0;
	if (camera)
	{
		float value;
		long flags;
		if (LucamGetProperty(camera, LUCAM_PROP_UNIT_WIDTH, &value, &flags))
		{
			value *= samplingFactor;

			// NOTE: in some cases might want to use toBiggestMultipled() instead.
			good = toSmallestMultiple(desiredWidth, static_cast<uint32_t>(value));
			uint32_t minWidth = getMinWidth(camera, pixelFormat, samplingFactor);
			if (good < minWidth) good = minWidth;
			if (good > getMaxWidth(camera)) good = getMaxWidth(camera);
		}
	}
	return good;
}

uint32_t getGoodHeight(void* camera, const int pixelFormat, const int samplingFactor, const uint32_t& desiredHeight)
{
	uint32_t good = 0;
	if (camera)
	{
		float value;
		long flags;
		if (LucamGetProperty(camera, LUCAM_PROP_UNIT_HEIGHT, &value, &flags))
		{
			value *= samplingFactor;

			// NOTE: in some cases might want to use toBiggestMultiple() instead.
			good = toSmallestMultiple(desiredHeight, static_cast<uint32_t>(value));
			uint32_t min = getMinHeight(camera, pixelFormat, samplingFactor);
			if (good < min) good = min;
			if (good > getMaxHeight(camera)) good = getMaxHeight(camera);
		}
	}
	return good;
}


//int main(int argc, char* argv[])
//{
//	try
//	{
//		int camIndex = 1;
//		if (LucamNumCameras() < camIndex)
//			throwError(LucamCameraNotFound, "there is no camera connected.");
//		void* camera = LucamCameraOpen(camIndex);
//		if (!camera)
//			throwError(camera, "not able to connect to camera with index 1.");
//
//		LUCAM_FRAME_FORMAT ff;
//		float fr;
//		if (!LucamGetFormat(camera, &ff, &fr))
//			throwError(camera, "could not read actual frame format on camera.");
//		ff.width = getGoodWidth(camera, ff.pixelFormat, 1, 4240);
//		ff.height = getGoodHeight(camera, ff.pixelFormat, ff.subSampleY, 756);
//
//		if (!LucamSetFormat(camera, &ff, fr))
//			throwError(camera, "could not set camera format to updated frame format.");
//
//		std::cout << "Frame format as been written to camera successfully." << std::endl;
//	}
//	catch (std::exception& e)
//	{
//		std::cout << e.what() << std::endl;
//	}
//	return 0;
//}


