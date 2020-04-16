#include "pcm.h"


#define PIXEL(frame, x, y) ((*(frame.ptr(x, y)))?1:0)

#define L0_POS 0
#define R0_POS 16
#define L1_POS 32
#define R1_POS 48
#define L2_POS 64
#define R2_POS 80
#define P_POS  96
#define Q_POS  112
#define S_POS  112

#define L0 buf[0]
#define R0 buf[1]
#define L1 buf[2]
#define R1 buf[3]
#define L2 buf[4]
#define R2 buf[5]

static uint32_t stairsCount = 0;
static uint32_t crcAllErrorCount = 0;
static uint32_t parityErrorCount = 0;

#define PCM_FRAME_WIDTH (PCM_WIDTH_BYTES + 1)
#define PCM_FRAME_HEIGHT (PCM_NTSC_HEIGHT + PCM_STAIRS)
uint8_t PCMFrame[PCM_FRAME_HEIGHT][PCM_FRAME_WIDTH] = {0};

void memsetBuffer(uint8_t value){
  memset(PCMFrame, value, (PCM_FRAME_WIDTH * PCM_FRAME_HEIGHT));
}

uint16_t buf[6];
uint16_t P;
uint16_t Q;

uint8_t header[16] = {0b11001100, 0, 0, 0, 0, 0, 0b00001100};

void readBlock(uint16_t j, bool type){
	L0 = (( PCMFrame[j + L0_POS][0] << 6)          | (PCMFrame[j + L0_POS][1] >> 2)) << 2;
	R0 = (((PCMFrame[j + R0_POS][1] & 0x03) << 12) | (PCMFrame[j + R0_POS][2] << 4) | ((PCMFrame[j + R0_POS][3] >> 4) & 0x0f)) << 2;
	L1 = (((PCMFrame[j + L1_POS][3] & 0x0f) << 10) | (PCMFrame[j + L1_POS][4] << 2) | ((PCMFrame[j + L1_POS][5] >> 6) & 0x03)) << 2;
	R1 = (((PCMFrame[j + R1_POS][5] & 0x3f) << 8)  | (PCMFrame[j + R1_POS][6]     )) << 2;
	L2 = (( PCMFrame[j + L2_POS][7] << 6)          | (PCMFrame[j + L2_POS][8] >> 2)) << 2;
	R2 = (((PCMFrame[j + R2_POS][8] & 0x03) << 12) | (PCMFrame[j + R2_POS][9] << 4) | ((PCMFrame[j + R2_POS][10] >> 4) & 0x0f)) << 2;
	P  = (((PCMFrame[j + P_POS][10] & 0x0f) << 10) | (PCMFrame[j + P_POS][11] << 2) | ((PCMFrame[j + P_POS][12] >> 6) & 0x03)) << 2;
	Q  = (((PCMFrame[j + Q_POS][12] & 0x3f) << 8) | (PCMFrame[j + Q_POS][13])) << 2;

	// 16 бит PCM
	if (type){
		L0 |= (PCMFrame[j + L0_POS][12] >> 4) & 0x03;
		R0 |= (PCMFrame[j + R0_POS][12] >> 2) & 0x03;
		L1 |= (PCMFrame[j + L1_POS][12] >> 0) & 0x03;
		R1 |= (PCMFrame[j + R1_POS][13] >> 6) & 0x03;
		L2 |= (PCMFrame[j + L2_POS][13] >> 4) & 0x03;
		R2 |= (PCMFrame[j + R2_POS][13] >> 2) & 0x03;
		P  |= (PCMFrame[j +  P_POS][13] >> 0) & 0x03;
	}
}

void writeBlock(uint16_t j, bool type){
	// Не пытайтесь это читать... Сломаетесь...
	PCMFrame[j + L0_POS][ 0]  = L0 >> 8;           // 8 бит
	PCMFrame[j + L0_POS][ 1] &= ~0xfc;             // Очистить 6 бит
	PCMFrame[j + L0_POS][ 1] |= L0 & 0xfc;         // Записать 6 бит

	PCMFrame[j + R0_POS][ 1] &= ~0x3;              // Очистить 2 бита
	PCMFrame[j + R0_POS][ 1] |= (R0 >> 14) & 0x3;  // Записать 2 бита
	PCMFrame[j + R0_POS][ 2]  = (R0 >> 6)  & 0xff; // 8 бит
	PCMFrame[j + R0_POS][ 3] &= ~0xf0;             // Очистить 4 бита
	PCMFrame[j + R0_POS][ 3] |= (R0 << 2)  & 0xf0; // Записать 4 бита

	PCMFrame[j + L1_POS][ 3] &= ~0x0f;             // Очистить 4 бита
	PCMFrame[j + L1_POS][ 3] |= (L1 >> 12) & 0x0f; // Записать 4 бита
	PCMFrame[j + L1_POS][ 4]  = (L1 >> 4)  & 0xff; // 8 бит
	PCMFrame[j + L1_POS][ 5] &= ~0xc0;             // Очистить 2 бита
	PCMFrame[j + L1_POS][ 5] |= (L1 << 4)  & 0xc0; // Записать 2 бита

	PCMFrame[j + R1_POS][ 5] &= ~0x3f; // 6 бит
	PCMFrame[j + R1_POS][ 5] |= (R1 >> 10) & 0x3f; // 6 бит
	PCMFrame[j + R1_POS][ 6]  = (R1 >> 2)  & 0xff; // 8 бит


	PCMFrame[j + L2_POS][ 7]  = L2 >> 8;           // 8 бит
	PCMFrame[j + L2_POS][ 8] &= ~0xfc;             // Очистить 6 бит
	PCMFrame[j + L2_POS][ 8] |= L2 & 0xfc;         // Записать 6 бит

	PCMFrame[j + R2_POS][ 8] &= ~0x3;              // Очистить 2 бита
	PCMFrame[j + R2_POS][ 8] |= (R2 >> 14) & 0x3;  // Записать 2 бита
	PCMFrame[j + R2_POS][ 9]  = (R2 >> 6)  & 0xff; // 8 бит
	PCMFrame[j + R2_POS][10] &= ~0xf0;             // Очистить 4 бита
	PCMFrame[j + R2_POS][10] |= (R2 << 2)  & 0xf0; // Записать 4 бита

	PCMFrame[j +  P_POS][10] &= ~0x0f;             // Очистить 4 бита
	PCMFrame[j +  P_POS][10] |= (P  >> 12) & 0x0f; // Записать 4 бита
	PCMFrame[j +  P_POS][11]  = (P  >> 4)  & 0xff; // 8 бит
	PCMFrame[j +  P_POS][12] &= ~0xc0;             // Очистить 2 бита
	PCMFrame[j +  P_POS][12] |= (P  << 4)  & 0xc0; // Записать 2 бита

	// 16 бит PCM
	if (type){
		PCMFrame[j + L0_POS][12] &= ~(0x03 << 4);
		PCMFrame[j + L0_POS][12] |= (L0 & 0x03) << 4;
		PCMFrame[j + R0_POS][12] &= ~(0x03 << 2);
		PCMFrame[j + R0_POS][12] |= (R0 & 0x03) << 2;
		PCMFrame[j + L1_POS][12] &= ~(0x03);
		PCMFrame[j + L1_POS][12] |= (L1 & 0x03);
		PCMFrame[j + R1_POS][13] &= ~(0x03 << 6);
		PCMFrame[j + R1_POS][13] |= (R1 & 0x03) << 6;
		PCMFrame[j + L2_POS][13] &= ~(0x03 << 4);
		PCMFrame[j + L2_POS][13] |= (L2 & 0x03) << 4;
		PCMFrame[j + R2_POS][13] &= ~(0x03 << 2);
		PCMFrame[j + R2_POS][13] |= (R2 & 0x03) << 2;
		PCMFrame[j +  P_POS][13] &= ~(0x03);
		PCMFrame[j +  P_POS][13] |= (P  & 0x03);
	} else {
		PCMFrame[j +  Q_POS][12] &= ~0x3f;
		PCMFrame[j +  Q_POS][12] |= (Q  >> 10) & 0x3f;
		PCMFrame[j +  Q_POS][13]  = (Q  >> 2)  & 0xff;
	}
}

bool searchStart(cv::Mat frame, uint16_t line, uint16_t *startPtr, float *pixelSizePtr){
	uint16_t black = 0;
	uint16_t start = 0;
	float pixelSize = 1;
	bool startSync = false;
	// Поиск начала данных в строке
	for(int i = 0; i < frame.cols; i++){
		if (PIXEL(frame, line, i)){
			start = i;
			if (black)
				startSync = true;
		} else {
			if (startSync){
				start += black + 1;
				break;
			}
			if (start){
				black++;
      }
		}
	}

	if (!start){ // Пустая строка
		return false;
	}

	// Поиск конца данных в строке
	uint16_t end = 0;
	for(int i = frame.cols - 1; i >= 0; i--){
		if (PIXEL(frame, line, i)){
			end = i;
		} else {
			if (end){
				end -= black;
				break;
			}
		}
	}
	pixelSize = (float)(end - start) / PCM_WIDTH_BITS;
	//fprintf(stderr, "%d\t%d\t%d\t%.2f\n", black, start, end, pixelSize);

	if (pixelSize < 0/* || pixelSize > */){ // Пустая строка
		return false;
	}

	if (pixelSizePtr != NULL)
		*pixelSizePtr = pixelSize;

	if (startPtr != NULL)
		*startPtr = start;

	return true;
}

bool readLine(cv::Mat frame, uint16_t line, uint8_t *output, uint16_t startPosition, float pixelSize){
	memset(output, 0, PCM_FRAME_WIDTH);
	for(int i = 0; i < PCM_WIDTH_BITS; i++){
    if (PIXEL(frame, line, (int)(i * pixelSize + startPosition + pixelSize / 2)))
      output[i >> 3] |= 1 << (7 - (i & 0b111));
	}
	uint16_t crcCheck = Calculate_CRC_CCITT(output, 14);
	uint16_t crcOriginal = output[14] << 8 | output[15];
  output[16] = (crcCheck != crcOriginal)?1:0;
	return !output[16];
}

void readPCMFrame(cv::Mat frame, uint8_t offset){
	static uint32_t frameCount = 0;
	uint16_t crcErrorCount = 0;
	uint16_t clearLinesCount = 0;
  int PCMLineCounter = 0;

	// перенос
	memcpy(&(PCMFrame[0]), &(PCMFrame[PCM_FRAME_HEIGHT - PCM_STAIRS]), ((PCM_STAIRS) * PCM_FRAME_WIDTH));

  //fprintf(stderr, "frame height %d\n", frame.rows);
	for(int i = 2 + offset; i <= frame.rows; i += 2){
		if (PCMLineCounter >= PCM_NTSC_HEIGHT){
			break;
		}
		//fprintf(stderr, "decode %d line (%d, %d)\n", i, PCMLineCounter + 1, PCMLineCounter + PCM_STAIRS);

		uint8_t *PCMLinePtr = PCMFrame[PCMLineCounter + PCM_STAIRS];

		uint16_t start = 0;
		float pixelSize = 1;

		if (!searchStart(frame, i, &start, &pixelSize)){
			memset(PCMLinePtr, 0, PCM_FRAME_WIDTH);
      PCMLinePtr[16] = 0xf0;
			clearLinesCount++;
		}

		// Чтение данных
		if (!readLine(frame, i, PCMLinePtr, start, pixelSize)){
			if (!readLine(frame, i, PCMLinePtr, start - 1, pixelSize)){
				if (!readLine(frame, i, PCMLinePtr, start + 1, pixelSize)){
					PCMLinePtr[16] = 0xf1;
					crcErrorCount++;
				}
			}
		}

    //for(int j = frame.cols/4; j < frame.cols/2; j++){
    //  frame.at<uchar>(cv::Point(j, i)) = 50;
    //}

    PCMLineCounter++;
	}

	frameCount++;

  /*
  if (clearLinesCount){
		fprintf(stderr, "Field %6d. clear lines: %d\n", frameCount, clearLinesCount);
  }
  */

  if (crcErrorCount){
		fprintf(stderr, "Field %6d. CRC errors: %d(%d) - %.2f%%(%.2f%%)\n", frameCount, crcErrorCount, crcErrorCount - clearLinesCount, crcErrorCount / 2.45, (crcErrorCount - clearLinesCount) / 2.45);
  }

  crcAllErrorCount += crcErrorCount;
}

#define PCM_PIXEL_0 30
#define PCM_PIXEL_1 127
#define HEADER_COPY 3        // 0 - disabled
#define HEADER_P_COR 2       // 0 - enabled
#define HEADER_Q_COR 1       // 0 - enabled
#define HEADER_PREEMPHASIS 0 // 0 - enabled

inline void writeLine(cv::Mat frame, uint16_t offset, uint8_t *data)
{
  // sync
  frame.at<uchar>(cv::Point(1, offset)) = PCM_PIXEL_1;
  frame.at<uchar>(cv::Point(3, offset)) = PCM_PIXEL_1;

  // write pixels
  for(int pcmPixel = 0; pcmPixel < PCM_WIDTH_BITS; pcmPixel++){
    if (data[pcmPixel >> 3] & (1 << (7 - (pcmPixel & 0x07)))){
      frame.at<uchar>(cv::Point(5 + pcmPixel, offset)) = PCM_PIXEL_1;
    }
  }

  // white lvl
  for(int x = 5 + PCM_WIDTH_BITS + 1; x < frame.cols; x++){
    frame.at<uchar>(cv::Point(x, offset)) = 240;
  }
}

void writePCMFrame(cv::Mat frame, uint8_t offset, bool full){
	int16_t headerLines = 2;
  header[13] = (1 << HEADER_COPY) | (1 << HEADER_P_COR) | (1 << HEADER_Q_COR) | (1 << HEADER_PREEMPHASIS);
  uint16_t crc = Calculate_CRC_CCITT(header, 14);
  header[14] = crc >> 8;
	header[15] = crc & 0xff;
  writeLine(frame, 0, header);
  writeLine(frame, 1, header);

  for(int pcmLine = 0; pcmLine < PCM_NTSC_HEIGHT; pcmLine++){
		uint8_t* pcmLinePtr = PCMFrame[pcmLine];
		// calc CRC
		crc = Calculate_CRC_CCITT(pcmLinePtr, 14);
		pcmLinePtr[14] = crc >> 8;
		pcmLinePtr[15] = crc & 0xff;

		writeLine(frame, ((pcmLine * 2) + offset + headerLines), pcmLinePtr);
	}
}

void PCMFrame2wav(SNDFILE *outfile, bool type){
  uint16_t parityError = 0;
  static uint16_t fieldCount = 0;
	//fprintf(stderr, "\n");
	//printFrame();
	for(int j = 0; j < PCM_NTSC_HEIGHT; j++){
		stairsCount++;
		readBlock(j, type);

		uint8_t crcErrors = 0;
		if (PCMFrame[j + L0_POS][16]){crcErrors++;}
		if (PCMFrame[j + R0_POS][16]){crcErrors++;}
		if (PCMFrame[j + L1_POS][16]){crcErrors++;}
    if (PCMFrame[j + R1_POS][16]){crcErrors++;}
    if (PCMFrame[j + L2_POS][16]){crcErrors++;}
    if (PCMFrame[j + R2_POS][16]){crcErrors++;}
    // if (PCMFrame[j +  P_POS][16]){crcErrors++; /*fprintf(stderr, "P\n");*/}
		// if (PCMFrame[j +  Q_POS][16]){crcErrors++; /*fprintf(stderr, "Q\n");*/}

		if (crcErrors){
			//uint16_t PC = L0 ^ R0 ^ L1 ^ R1 ^ L2 ^ R2;
			if (!PCMFrame[j + P_POS][16] && crcErrors == 1){
				parityError++;
				if (PCMFrame[j + L0_POS][16]){
          //fprintf(stderr, "L0 0x%04x -> ", L0);
          L0 = P  ^ R0 ^ L1 ^ R1 ^ L2 ^ R2;
          //fprintf(stderr, "0x%04x\n", L0);
        }
				if (PCMFrame[j + R0_POS][16]){
          //fprintf(stderr, "R0 0x%04x -> ", R0);
          R0 = L0 ^ P  ^ L1 ^ R1 ^ L2 ^ R2;
          //fprintf(stderr, "0x%04x\n", R0);
        }
				if (PCMFrame[j + L1_POS][16]){
          //fprintf(stderr, "L1 0x%04x -> ", L1);
          L1 = L0 ^ R0 ^ P  ^ R1 ^ L2 ^ R2;
          //fprintf(stderr, "0x%04x\n", L1);
        }
				if (PCMFrame[j + R1_POS][16]){
          //fprintf(stderr, "R1 0x%04x -> ", R1);
          R1 = L0 ^ R0 ^ L1 ^ P  ^ L2 ^ R2;
          //fprintf(stderr, "0x%04x\n", R1);
        }
				if (PCMFrame[j + L2_POS][16]){
          //fprintf(stderr, "L2 0x%04x -> ", L2);
          L2 = L0 ^ R0 ^ L1 ^ R1 ^ P  ^ R2;
          //fprintf(stderr, "0x%04x\n", L2);
        }
				if (PCMFrame[j + R2_POS][16]){
          //fprintf(stderr, "R2 0x%04x -> ", R2);
          R2 = L0 ^ R0 ^ L1 ^ R1 ^ L2 ^ P ;
          //fprintf(stderr, "0x%04x\n", R2);
        }
			} else {
        fprintf(stderr, "Field %6d, stairs %d. CRC errors: %d\n", fieldCount, j, crcErrors);
        fprintf(stderr, "Use Q correction\n");
        uint16_t Sp = L0 ^ R0 ^ L1 ^ R1 ^ L2 ^ R2 ^ P;
        uint16_t Sq = qCorTS(6, L0) ^ qCorTS(5, R0) ^ qCorTS(4, L1) ^ qCorTS(3, R1) ^ qCorTS(2, L2) ^ qCorTS(1, R2) ^ Q;

        if (PCMFrame[j + L0_POS][16] & PCMFrame[j + R0_POS][16]){ // L0 R0
          fprintf(stderr, "L0 R0\n");
          uint16_t E1 = qCorMul(T_I_m1, Sp ^ qCorMul(T_m5, Sq));
          uint16_t E2 = E1 ^ Sp;
          L0 ^= E1;
          R0 ^= E2;
        } else if (PCMFrame[j + R0_POS][16] & PCMFrame[j + L1_POS][16]){ // R0 L1
          fprintf(stderr, "R0 L1\n");
          uint16_t E1 = qCorMul(T_I_m1, Sp ^ qCorMul(T_m4, Sq));
          uint16_t E2 = E1 ^ Sp;
          R0 ^= E1;
          L1 ^= E2;
        } else if (PCMFrame[j + L1_POS][16] & PCMFrame[j + R1_POS][16]){ // L1 R1
          fprintf(stderr, "L1 R1\n");
          uint16_t E1 = qCorMul(T_I_m1, Sp ^ qCorMul(T_m3, Sq));
          uint16_t E2 = E1 ^ Sp;
          L1 ^= E1;
          R1 ^= E2;
        } else if (PCMFrame[j + R1_POS][16] & PCMFrame[j + L2_POS][16]){ // R1 L2
          fprintf(stderr, "R1 L2\n");
          uint16_t E1 = qCorMul(T_I_m1, Sp ^ qCorMul(T_m2, Sq));
          uint16_t E2 = E1 ^ Sp;
          R1 ^= E1;
          L2 ^= E2;
        } else if (PCMFrame[j + L2_POS][16] & PCMFrame[j + R2_POS][16]){ // L2 R2
          fprintf(stderr, "L2 R2\n");
          uint16_t E1 = qCorMul(T_I_m1, Sp ^ qCorMul(T_m1, Sq));
          uint16_t E2 = E1 ^ Sp;
          L2 ^= E1;
          R2 ^= E2;
        } else if (PCMFrame[j + R2_POS][16] & PCMFrame[j + P_POS][16]){ // R2 P
          fprintf(stderr, "R2 P\n");
          uint16_t E1 = qCorMul(T_m1, Sq);
          R2 ^= E1;
        } else if (PCMFrame[j + L0_POS][16] & PCMFrame[j + L1_POS][16]){ // L0 L1
          fprintf(stderr, "L0 L1\n");
          uint16_t E1 = qCorMul(T2_I_m1, Sp ^ qCorMul(T_m4, Sq));
          uint16_t E2 = E1 ^ Sp;
          L0 ^= E1;
          L1 ^= E2;
        } else if (PCMFrame[j + R0_POS][16] & PCMFrame[j + R1_POS][16]){ // R0 R1
          fprintf(stderr, "R1 R1\n");
          uint16_t E1 = qCorMul(T2_I_m1, Sp ^ qCorMul(T_m3, Sq));
          uint16_t E2 = E1 ^ Sp;
          R0 ^= E1;
          R1 ^= E2;
        } else if (PCMFrame[j + L1_POS][16] & PCMFrame[j + L2_POS][16]){ // L1 L2
          fprintf(stderr, "L1 L2\n");
          uint16_t E1 = qCorMul(T2_I_m1, Sp ^ qCorMul(T_m2, Sq));
          uint16_t E2 = E1 ^ Sp;
          L1 ^= E1;
          L2 ^= E2;
        } else if (PCMFrame[j + R1_POS][16] & PCMFrame[j + R2_POS][16]){ // R1 R2
          fprintf(stderr, "R1 R2\n");
          uint16_t E1 = qCorMul(T2_I_m1, Sp ^ qCorMul(T_m1, Sq));
          uint16_t E2 = E1 ^ Sp;
          R1 ^= E1;
          R2 ^= E2;
        } else if (PCMFrame[j + L2_POS][16] & PCMFrame[j + P_POS][16]){ // L2 P
          fprintf(stderr, "L2 P\n");
          uint16_t E1 = qCorMul(T_m2, Sq);
          L2 ^= E1;
        } else if (PCMFrame[j + L0_POS][16] & PCMFrame[j + R1_POS][16]){ // L0 R1
          fprintf(stderr, "L0 R1\n");
          uint16_t E1 = qCorMul(T3_I_m1, Sp ^ qCorMul(T_m3, Sq));
          uint16_t E2 = E1 ^ Sp;
          L0 ^= E1;
          R1 ^= E2;
        } else if (PCMFrame[j + R0_POS][16] & PCMFrame[j + L2_POS][16]){ // R0 L2
          fprintf(stderr, "R0 L2\n");
          uint16_t E1 = qCorMul(T3_I_m1, Sp ^ qCorMul(T_m2, Sq));
          uint16_t E2 = E1 ^ Sp;
          R0 ^= E1;
          L2 ^= E2;
        } else if (PCMFrame[j + L1_POS][16] & PCMFrame[j + R2_POS][16]){ // L1 R2
          fprintf(stderr, "L1 R2\n");
          uint16_t E1 = qCorMul(T3_I_m1, Sp ^ qCorMul(T_m1, Sq));
          uint16_t E2 = E1 ^ Sp;
          L1 ^= E1;
          R2 ^= E2;
        } else if (PCMFrame[j + R1_POS][16] & PCMFrame[j + P_POS][16]){ // R1 P
          fprintf(stderr, "R1 P\n");
          uint16_t E1 = qCorMul(T_m3, Sq);
          R1 ^= E1;
        } else if (PCMFrame[j + L0_POS][16] & PCMFrame[j + L2_POS][16]){ // L0 L2
          fprintf(stderr, "L0 L2\n");
          uint16_t E1 = qCorMul(T4_I_m1, Sp ^ qCorMul(T_m2, Sq));
          uint16_t E2 = E1 ^ Sp;
          L0 ^= E1;
          L2 ^= E2;
        } else if (PCMFrame[j + R0_POS][16] & PCMFrame[j + R2_POS][16]){ // R0 R2
          fprintf(stderr, "R0 R2\n");
          uint16_t E1 = qCorMul(T4_I_m1, Sp ^ qCorMul(T_m1, Sq));
          uint16_t E2 = E1 ^ Sp;
          R0 ^= E1;
          R2 ^= E2;
        } else if (PCMFrame[j + L1_POS][16] & PCMFrame[j + P_POS][16]){ // L1 P
          fprintf(stderr, "L1 P\n");
          uint16_t E1 = qCorMul(T_m4, Sq);
          L1 ^= E1;
        } else if (PCMFrame[j + L0_POS][16] & PCMFrame[j + R2_POS][16]){ // L0 R2
          fprintf(stderr, "L0 R2\n");
          uint16_t E1 = qCorMul(T5_I_m1, Sp ^ qCorMul(T_m1, Sq));
          uint16_t E2 = E1 ^ Sp;
          L0 ^= E1;
          R2 ^= E2;
        } else if (PCMFrame[j + R0_POS][16] & PCMFrame[j + P_POS][16]){ // R0 P
          fprintf(stderr, "R0 P\n");
          uint16_t E1 = qCorMul(T_m5, Sq);
          R0 ^= E1;
        } else if (PCMFrame[j + L0_POS][16] & PCMFrame[j + P_POS][16]){ // L0 P
          fprintf(stderr, "L0 P\n");
          uint16_t E1 = qCorMul(T_m6, Sq);
          L0 ^= E1;
        }
      }
		}

		sf_write_short(outfile, (int16_t *)buf, 6);
	}
  if (parityError){
    //printFrame();
    parityErrorCount += parityError;
    //fprintf(stderr, "Field %6d. Parity errors: %d\n", fieldCount, parityError);
  }
  fieldCount++;
}

void copyOutBuffer(void){
	memcpy(&(PCMFrame[0]), &(PCMFrame[PCM_STAIRS]), (PCM_NTSC_HEIGHT * PCM_FRAME_WIDTH));
}

bool wav2PCMFrame(SNDFILE *infile, bool type){
	// Перенос конца прошлого блока в начало текущего
	memcpy(&(PCMFrame[0]), &(PCMFrame[PCM_NTSC_HEIGHT]), ((PCM_STAIRS) * PCM_FRAME_WIDTH));

	// Заполнение
	for(int j = 0; j < PCM_NTSC_HEIGHT; j++){
		sf_count_t count = sf_read_short(infile, (int16_t *)buf, 6);
		if (count != 6){
			return(false);
		}
		P = L0 ^ R0 ^ L1 ^ R1 ^ L2 ^ R2;
		writeBlock(j, type);
		//fprintf(stderr, "%04x %04x %04x %04x %04x %04x %04x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], P);
		//readBlock(j, type);
		//fprintf(stderr, "%04x %04x %04x %04x %04x %04x %04x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], P);
		//fprintf(stderr, "\n");
		//if (j > 100) exit(0);
	}
	return true;
}

uint32_t samplesCount(void){
	return(stairsCount * 3);
}

void printFrame(void){
  /*
	for(int j = 0; j < PCM_FRAME_HEIGHT; j++){
    fprintf(stderr, "line %03d ", j);
		for(int i = 0; i < PCM_FRAME_WIDTH; i++){
			fprintf(stderr, "%02x ", PCMFrame[j][i]);
		}
		fprintf(stderr, "\n");
	}
  */
	for(int j = 109; j < 113; j++){
    fprintf(stderr, "line %03d ", j);
		for(int i = 0; i < PCM_FRAME_WIDTH; i++){
			fprintf(stderr, "%02x ", PCMFrame[j][i]);
		}
		fprintf(stderr, "\n");
	}

	for(int j = PCM_FRAME_HEIGHT - 5; j < PCM_FRAME_HEIGHT; j++){
    fprintf(stderr, "line %03d ", j);
		for(int i = 0; i < PCM_FRAME_WIDTH; i++){
			fprintf(stderr, "%02x ", PCMFrame[j][i]);
		}
		fprintf(stderr, "\n");
	}

	fprintf(stderr, "\n");
}

void showStatistics(void){
	fprintf(stderr, "Stairs count: %d\n", stairsCount);
	fprintf(stderr, "CRC errors count: %d\n", crcAllErrorCount);
	fprintf(stderr, "Parity errors count: %d\n", parityErrorCount);
}
