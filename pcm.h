#ifndef PCM_H
#define PCM_H

#include "opencv2/opencv.hpp"
#include <cstdint>
#include <sndfile.h>
#include "crc.h"

//#define FULL_PCM

//#ifdef FULL_PCM
//#define FRAME_HEIGHT 525
//#else
//#define FRAME_HEIGHT 480
//#endif

#define PCM_HEIGHT 245
#define PCM_WIDTH_BITS 128
#define PCM_WIDTH_BYTES PCM_WIDTH_BITS/8
#define PCM_STAIRS 112 // 7*16

void init();

void decodePCMFrame(SNDFILE *outfile, bool type);

void preparePCMFrame(cv::Mat frame, uint8_t offset);

uint32_t samplesCount(void);

void printFrame(void);

void showStatistics(void);

#endif
