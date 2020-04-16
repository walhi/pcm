#ifndef PCM_H
#define PCM_H

#include "opencv2/opencv.hpp"
#include <cstdint>
#include <sndfile.h>
#include "crc.h"
#include "q_correction.h"
#include <bitset>

#define PCM_NTSC_HEIGHT 245
#define PCM_PAL_HEIGHT 294
#define PCM_WIDTH_BITS 128
#define PCM_WIDTH_BYTES 16
#define PCM_STAIRS 111 // 7*16

void init();

void PCMFrame2wav(SNDFILE *outfile, bool type);

bool wav2PCMFrame(SNDFILE *infile, bool type);

void readPCMFrame(cv::Mat frame, uint8_t offset);

void writePCMFrame(cv::Mat frame, uint8_t offset, bool full);

uint32_t samplesCount(void);

void printFrame(void);

void showStatistics(void);

bool searchStart(cv::Mat frame, uint16_t line, uint16_t *startPtr, float *pixelSizePtr);

void copyOutBuffer(void);

void memsetBuffer(uint8_t value);
#endif
