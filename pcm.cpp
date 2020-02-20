#include "pcm.h"


#define PIXEL(frame, x, y) ((*(frame.ptr(x, y)))?1:0)


#define SWAP_PCM_FRAME() memcpy(&(PCMFrame[0]), &(PCMFrame[PCM_HEIGHT]), ((PCM_STAIRS) * PCM_WIDTH_BYTES))

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
static uint32_t crcErrorCount = 0;
static uint32_t parityErrorCount = 0;


static uint8_t PCMFrame[PCM_HEIGHT + PCM_STAIRS][PCM_WIDTH_BYTES];

void preparePCMFrame(cv::Mat frame, uint8_t offset){
	// перенос
	SWAP_PCM_FRAME();

	uint8_t *line;
	for(int j = 0; j < PCM_HEIGHT; j++){
		uint16_t black = 0;
		uint16_t start = 0;
		uint16_t end = 0;
		float pixelSize = 1;
		bool startSync = false;
		// Поиск начала данных в строке
		for(int i = 0; i < frame.cols; i++){
			if (PIXEL(frame, j * 2 + offset, i)){
        start = i;
				if (black)
					startSync = true;
			} else {
				if (startSync){
					start += black + 1;
					break;
				}
				if (start)
					black++;
			}
		}
		if (!start){ // Пустая строка
			continue;
		}
		// Поиск конца данных в строке
		for(int i = frame.cols - 1; i >= 0; i--){
			if (PIXEL(frame, j * 2 + offset, i)){
        end = i;
			} else {
				if (end){
					end -= black;
					break;
				}
			}
		}
		pixelSize = (float)(end - start) / PCM_WIDTH_BITS;
		//printf("%d\t%d\t%.2f\n", start, end, pixelSize);

		// Чтение данных
		line = PCMFrame[j + PCM_STAIRS];
		memset(line, 0, PCM_WIDTH_BYTES);
		for(int i = 0; i < PCM_WIDTH_BITS; i ++){
			line[i >> 3] |= PIXEL(frame, j * 2 + offset + 20, (int)(i * pixelSize + start + 2)) << (7 - i & 0b111); // /8 and %8
		}
		uint16_t crcCheck = Calculate_CRC_CCITT(line, 14);
		uint16_t crcOriginal = line[14] << 8 | line[15];
		if (crcCheck != crcOriginal){
			line[14] = 0;
			line[15] = 0;
			//printf("CRC ERROR 0x%04x 0x%04x\n", crcOriginal, crcCheck);
			crcErrorCount++;
		}
	}
}


void decodePCMFrame(SNDFILE *outfile){
	uint16_t buf[6];
	for(int j = 0; j < PCM_HEIGHT; j++){
		stairsCount++;

		L0 = (( PCMFrame[j + L0_POS][0] << 6)          | (PCMFrame[j + L0_POS][1] >> 2)) << 2;
		R0 = (((PCMFrame[j + R0_POS][1] & 0x03) << 12) | (PCMFrame[j + R0_POS][2] << 4) | (PCMFrame[j + R0_POS][3] >> 4)) << 2;
		L1 = (((PCMFrame[j + L1_POS][3] & 0x0f) << 10) | (PCMFrame[j + L1_POS][4] << 2) | (PCMFrame[j + L1_POS][5] >> 6)) << 2;
		R1 = (((PCMFrame[j + R1_POS][5] & 0x3f) << 8)  | (PCMFrame[j + R1_POS][6]     )) << 2;
		L2 = (( PCMFrame[j + L2_POS][7] << 6)          | (PCMFrame[j + L2_POS][8] >> 2)) << 2;
		R2 = (((PCMFrame[j + R2_POS][8] & 0x03) << 12) | (PCMFrame[j + R2_POS][9] << 4) | (PCMFrame[j + R2_POS][10] >> 4)) << 2;
		uint16_t P  = (((PCMFrame[j + P_POS][10] & 0x0f) << 10) | (PCMFrame[j + P_POS][11] << 2) | ((PCMFrame[j + P_POS][12] >> 6) & 0x03)) << 2;
		uint16_t Q  = (((PCMFrame[j + Q_POS][12] & 0x3f) << 8) | (PCMFrame[j + Q_POS][13])) << 2;

		// 16 бит PCM
		if (1){
			L0 |= (PCMFrame[j + L0_POS][12] >> 4) & 0x03;
			R0 |= (PCMFrame[j + R0_POS][12] >> 2) & 0x03;
			L1 |= (PCMFrame[j + L1_POS][12] >> 0) & 0x03;
			R1 |= (PCMFrame[j + R1_POS][13] >> 6) & 0x03;
			L2 |= (PCMFrame[j + L2_POS][13] >> 4) & 0x03;
			R2 |= (PCMFrame[j + R2_POS][13] >> 2) & 0x03;
			P  |= (PCMFrame[j +  P_POS][13] >> 0) & 0x03;
		}

		uint8_t crcErrors = PCMFrame[j + L0_POS][15]?0:1 + PCMFrame[j + R0_POS][15]?0:1 + PCMFrame[j + L1_POS][15]?0:1 + PCMFrame[j + R1_POS][15]?0:1 + PCMFrame[j + L2_POS][15]?0:1 + PCMFrame[j + R2_POS][15]?0:1 + PCMFrame[j + P_POS][15]?0:1;
		//printf("%04x\n", buf[i]);;

		uint16_t PC = L0 ^ R0 ^ L1 ^ R1 ^ L2 ^ R2;
		if (P != PC){
			parityErrorCount++;
			if (crcErrors)
				memset(buf, 0, sizeof(buf));
			//printf("Parity error\n");
		}

		sf_write_short(outfile, (int16_t *)buf, 6);
	}
}


uint32_t samplesCount(void){
	return(stairsCount * 3);
}

void showStatistics(void){
	printf("Stairs count: %d\n", stairsCount);
	printf("CRC errors count: %d\n", crcErrorCount);
	printf("Parity errors count: %d\n", parityErrorCount);
}
