#ifndef InitalizeCameraFunctions_H
#include "stdafx.h"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include <Windows.h>
#include "lucamapi.h"
#include "lucamerr.h"

bool is16Bit(const uint32_t pixelFormat);
bool isBinningSupported(void* camera);
std::vector<uint32_t> getBinningFactor(void* camera);
bool isSubSamplinggSupported(void* camera);
std::vector<uint32_t> getSubSamplingFactor(void* camera);
float getValue(void* camera, const uint32_t prop);
uint32_t getMaxWidth(void* camera);
uint32_t getMaxHeight(void* camera);
uint32_t getMinWidth(void* camera, const int pixelFormat, const int samplingFactor);
uint32_t getMinHeight(void* camera, const int pixelFormat, const int samplingFactor);
uint32_t toSmallestMultiple(const uint32_t& desiredValue, const uint32_t& multiple);
uint32_t toBiggestMultiple(const uint32_t& desiredValue, const uint32_t& multiple);
uint32_t getGoodWidth(void* camera, const int pixelFormat, const int samplingFactor, const uint32_t& desiredWidth);
uint32_t getGoodHeight(void* camera, const int pixelFormat, const int samplingFactor, const uint32_t& desiredHeight);

#endif // !InitalizeCameraFunctions_H

#pragma once