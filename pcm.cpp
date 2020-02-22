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

uint16_t buf[6];
uint16_t P;
uint16_t Q;

void readBlock(uint16_t j){
	L0 = (( PCMFrame[j + L0_POS][0] << 6)          | (PCMFrame[j + L0_POS][1] >> 2)) << 2;
	R0 = (((PCMFrame[j + R0_POS][1] & 0x03) << 12) | (PCMFrame[j + R0_POS][2] << 4) | (PCMFrame[j + R0_POS][3] >> 4)) << 2;
	L1 = (((PCMFrame[j + L1_POS][3] & 0x0f) << 10) | (PCMFrame[j + L1_POS][4] << 2) | (PCMFrame[j + L1_POS][5] >> 6)) << 2;
	R1 = (((PCMFrame[j + R1_POS][5] & 0x3f) << 8)  | (PCMFrame[j + R1_POS][6]     )) << 2;
	L2 = (( PCMFrame[j + L2_POS][7] << 6)          | (PCMFrame[j + L2_POS][8] >> 2)) << 2;
	R2 = (((PCMFrame[j + R2_POS][8] & 0x03) << 12) | (PCMFrame[j + R2_POS][9] << 4) | (PCMFrame[j + R2_POS][10] >> 4)) << 2;
	P  = (((PCMFrame[j + P_POS][10] & 0x0f) << 10) | (PCMFrame[j + P_POS][11] << 2) | ((PCMFrame[j + P_POS][12] >> 6) & 0x03)) << 2;
	Q  = (((PCMFrame[j + Q_POS][12] & 0x3f) << 8) | (PCMFrame[j + Q_POS][13])) << 2;

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
}



void preparePCMFrame(cv::Mat frame, uint8_t offset){
	// перенос
	memcpy(&(PCMFrame[0]), &(PCMFrame[PCM_HEIGHT]), ((PCM_STAIRS) * PCM_WIDTH_BYTES));
#ifndef FULL_PCM
	memset(PCMFrame[PCM_STAIRS], 0, 11 * PCM_WIDTH_BYTES);
#endif

	int16_t header = 0;

	uint8_t *line;
	int pcmLine = 0;
	for(int j = 0; j < frame.rows; j+=2){
		if (pcmLine >= PCM_HEIGHT){
			break;
		}

		uint16_t black = 0;
		uint16_t start = 0;
		uint16_t end = 0;
		float pixelSize = 1;
		bool startSync = false;
		// Поиск начала данных в строке
		for(int i = 0; i < frame.cols; i++){
			if (PIXEL(frame, j + offset, i)){
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

		if (header > 0){
			header--;
			continue;
		}

		// Поиск конца данных в строке
		for(int i = frame.cols - 1; i >= 0; i--){
			if (PIXEL(frame, j + offset, i)){
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
#ifdef FULL_PCM
		line = PCMFrame[pcmLine + PCM_STAIRS];
#else
		line = PCMFrame[pcmLine + PCM_STAIRS + 11];
#endif
		memset(line, 0, PCM_WIDTH_BYTES);
		for(int i = 0; i < PCM_WIDTH_BITS; i ++){
			line[i >> 3] |= PIXEL(frame, j + offset, (int)(i * pixelSize + start + 2)) << (7 - (i & 0b111)); // /8 and %8
		}
		uint16_t crcCheck = Calculate_CRC_CCITT(line, 14);
		uint16_t crcOriginal = line[14] << 8 | line[15];
		if (crcCheck != crcOriginal){
			line[14] = 0;
			line[15] = 0;
			//printf("CRC ERROR 0x%04x 0x%04x\n", crcOriginal, crcCheck);
			crcErrorCount++;
		}
		pcmLine++;
	}

#ifndef FULL_PCM
	// Восстановление потраченных строк


#endif
}


void decodePCMFrame(SNDFILE *outfile){
	//printf("\n");
	//printFrame();
	for(int j = 0; j < PCM_HEIGHT; j++){
		stairsCount++;
		readBlock(j);

		uint8_t crcErrors = 0;
		if (!PCMFrame[j + L0_POS][15]){crcErrors++; /*printf("L0\n");*/}
		if (!PCMFrame[j + R0_POS][15]){crcErrors++; /*printf("R0\n");*/}
		if (!PCMFrame[j + L1_POS][15]){crcErrors++; /*printf("L1\n");*/}
		if (!PCMFrame[j + R1_POS][15]){crcErrors++; /*printf("R1\n");*/}
		if (!PCMFrame[j + L2_POS][15]){crcErrors++; /*printf("L2\n");*/}
		if (!PCMFrame[j + R2_POS][15]){crcErrors++; /*printf("R2\n");*/}
		if (!PCMFrame[j +  P_POS][15]){crcErrors++; /*printf(" P\n");*/}
		//if (!PCMFrame[j +  Q_POS][15]) crcErrors++;
		//printf("%d\n", crcErrors);

		uint16_t PC = L0 ^ R0 ^ L1 ^ R1 ^ L2 ^ R2;
		if (P != PC && PCMFrame[j + P_POS][15]/* && crcErrors == 1*/){
			parityErrorCount++;
			if (!PCMFrame[j + L0_POS][15]){L0 = P  ^ R0 ^ L1 ^ R1 ^ L2 ^ R2;}
			if (!PCMFrame[j + R0_POS][15]){R0 = L0 ^ P  ^ L1 ^ R1 ^ L2 ^ R2;}
			if (!PCMFrame[j + L1_POS][15]){L1 = L0 ^ R0 ^ P  ^ R1 ^ L2 ^ R2;}
			if (!PCMFrame[j + R1_POS][15]){R1 = L0 ^ R0 ^ L1 ^ P  ^ L2 ^ R2;}
			if (!PCMFrame[j + L2_POS][15]){L2 = L0 ^ R0 ^ L1 ^ R1 ^ P  ^ R2;}
			if (!PCMFrame[j + R2_POS][15]){R2 = L0 ^ R0 ^ L1 ^ R1 ^ L2 ^ P ;}

		}/* else if (crcErrors > 1){
			memset(buf, 0, sizeof(uint16_t) * 6);
			//printf("%d\n", crcErrors);

			}*/

		sf_write_short(outfile, (int16_t *)buf, 6);
	}
}

uint32_t samplesCount(void){
	return(stairsCount * 3);
}

void printFrame(void){
	for(int j = 0; j < PCM_HEIGHT + PCM_STAIRS; j++){
		for(int i = 0; i < PCM_WIDTH_BYTES; i++){
			printf("0x%02x ", PCMFrame[j][i]);
		}
		printf("\n");
	}
	printf("\n");
}

void showStatistics(void){
	printf("Stairs count: %d\n", stairsCount);
	printf("CRC errors count: %d\n", crcErrorCount);
	printf("Parity errors count: %d\n", parityErrorCount);
}
