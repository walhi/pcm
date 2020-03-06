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


static uint8_t PCMFrame[PCM_NTSC_HEIGHT + PCM_STAIRS][PCM_WIDTH_BYTES] = {0};

uint16_t buf[6];
uint16_t P;
uint16_t Q;

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

void readPCMFrame(cv::Mat frame, uint8_t offset, bool full){
	int16_t header = 1;
	// перенос
	memcpy(&(PCMFrame[0]), &(PCMFrame[PCM_NTSC_HEIGHT]), ((PCM_STAIRS) * PCM_WIDTH_BYTES));

	if (!full) {
		memset(PCMFrame[PCM_STAIRS], 0, 11 * PCM_WIDTH_BYTES);
		header = 0;
	}

	uint8_t *line;
	int pcmLine = 0;
	for(int j = 0; j < frame.rows; j+=2){
		if (pcmLine >= PCM_NTSC_HEIGHT){
			break;
		}

		uint16_t black = 0;
		uint16_t start = 0;
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


		// Поиск конца данных в строке
		uint16_t end = 0;
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

		if (pixelSize < 0/* || pixelSize > */){ // Пустая строка
			continue;
		}


		if (header > 0){
			header--;
			continue;
		}

		// Чтение данных
		if(full)
			line = PCMFrame[pcmLine + PCM_STAIRS];
		else
			line = PCMFrame[pcmLine + PCM_STAIRS + 11];

		memset(line, 0, PCM_WIDTH_BYTES);
		for(int i = 0; i < PCM_WIDTH_BITS; i ++){
			line[i >> 3] |= PIXEL(frame, j + offset, (int)(i * pixelSize + start + pixelSize / 2)) << (7 - (i & 0b111)); // /8 and %8
		}
		uint16_t crcCheck = Calculate_CRC_CCITT(line, 14);
		uint16_t crcOriginal = line[14] << 8 | line[15];
		if (crcCheck != crcOriginal){
			line[14] = 0;
			line[15] = 0;
			printf("CRC ERROR %d\t%d\t%d\t%.2f\t0x%04x\t0x%04x\n", pcmLine, start, end, pixelSize, crcOriginal, crcCheck);
			crcErrorCount++;
		}
		pcmLine++;
	}

	if (!full) {
		// Восстановление потраченных строк
	}
}

#define PCM_PIXEL_0 30
#define PCM_PIXEL_1 127

void writePCMFrame(cv::Mat frame, uint8_t offset, bool full){
	int16_t header = 2;

	for(int pcmLine = 0; pcmLine < PCM_NTSC_HEIGHT; pcmLine++){
		uint8_t* pcmLinePtr = PCMFrame[pcmLine];
		// calc CRC
		uint16_t crc = Calculate_CRC_CCITT(pcmLinePtr, 14);
		pcmLinePtr[14] = crc >> 8;
		pcmLinePtr[15] = crc & 0xff;


		uint16_t imageLine = (pcmLine * 2) + offset + header;

		// sync
		//frame.at<uchar>(cv::Point(0, imageLine)) = PCM_PIXEL_0;
		frame.at<uchar>(cv::Point(1, imageLine)) = PCM_PIXEL_1;
		//frame.at<uchar>(cv::Point(2, imageLine)) = PCM_PIXEL_0;
		frame.at<uchar>(cv::Point(3, imageLine)) = PCM_PIXEL_1;
		//frame.at<uchar>(cv::Point(4, imageLine)) = PCM_PIXEL_0;

		// write pixels
		for(int pcmPixel = 0; pcmPixel < PCM_WIDTH_BITS; pcmPixel++){
			if (pcmLinePtr[pcmPixel >> 3] & (1 << (7 - (pcmPixel & 0x07)))){
				frame.at<uchar>(cv::Point(5 + pcmPixel, imageLine)) = PCM_PIXEL_1;
			}
		}
		/*
		for(int ggg = 0; ggg < PCM_WIDTH_BYTES; ggg++){
			uint8_t hhh = PCMFrame[pcmLine][ggg];
			if (hhh & (1 << 0)) frame.at<uchar>(cv::Point(5 + ggg * 8 + 0, imageLine)) = PCM_PIXEL_1;
			if (hhh & (1 << 1)) frame.at<uchar>(cv::Point(5 + ggg * 8 + 1, imageLine)) = PCM_PIXEL_1;
			if (hhh & (1 << 2)) frame.at<uchar>(cv::Point(5 + ggg * 8 + 2, imageLine)) = PCM_PIXEL_1;
			if (hhh & (1 << 3)) frame.at<uchar>(cv::Point(5 + ggg * 8 + 3, imageLine)) = PCM_PIXEL_1;
			if (hhh & (1 << 4)) frame.at<uchar>(cv::Point(5 + ggg * 8 + 4, imageLine)) = PCM_PIXEL_1;
			if (hhh & (1 << 5)) frame.at<uchar>(cv::Point(5 + ggg * 8 + 5, imageLine)) = PCM_PIXEL_1;
			if (hhh & (1 << 6)) frame.at<uchar>(cv::Point(5 + ggg * 8 + 6, imageLine)) = PCM_PIXEL_1;
			if (hhh & (1 << 7)) frame.at<uchar>(cv::Point(5 + ggg * 8 + 7, imageLine)) = PCM_PIXEL_1;
		}
		*/

		// white lvl
		//frame.at<uchar>(cv::Point(5 + PCM_WIDTH_BITS, imageLine)) = PCM_PIXEL_0;
		for(int x = 5 + PCM_WIDTH_BITS + 1; x < frame.cols; x++){
			frame.at<uchar>(cv::Point(x, imageLine)) = 240;
		}

	}
}

void PCMFrame2wav(SNDFILE *outfile, bool type){
	//printf("\n");
	//printFrame();
	for(int j = 0; j < PCM_NTSC_HEIGHT; j++){
		stairsCount++;
		readBlock(j, type);

		uint8_t crcErrors = 0;
		if (PCMFrame[j + L0_POS][15] == 0){crcErrors++; /*printf("L0\n");*/}
		if (PCMFrame[j + R0_POS][15] == 0){crcErrors++; /*printf("R0\n");*/}
		if (PCMFrame[j + L1_POS][15] == 0){crcErrors++; /*printf("L1\n");*/}
		if (PCMFrame[j + R1_POS][15] == 0){crcErrors++; /*printf("R1\n");*/}
		if (PCMFrame[j + L2_POS][15] == 0){crcErrors++; /*printf("L2\n");*/}
		if (PCMFrame[j + R2_POS][15] == 0){crcErrors++; /*printf("R2\n");*/}
		if (PCMFrame[j +  P_POS][15] == 0){crcErrors++; /*printf(" P\n");*/}
		//if (!PCMFrame[j +  Q_POS][15]) crcErrors++;

		fprintf(stderr, "%d ", crcErrors);
		if (crcErrors == 1){
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
		}

		sf_write_short(outfile, (int16_t *)buf, 6);
	}
}

bool wav2PCMFrame(SNDFILE *infile, bool type){
	// Перенос конца прошлого блока в начало текущего
	memcpy(&(PCMFrame[0]), &(PCMFrame[PCM_NTSC_HEIGHT]), ((PCM_STAIRS) * PCM_WIDTH_BYTES));

	// Заполнение
	for(int j = 0; j < PCM_NTSC_HEIGHT; j++){
		sf_count_t count = sf_read_short(infile, (int16_t *)buf, 6);
		if (count != 6){
			return(false);
		}
		P = L0 ^ R0 ^ L1 ^ R1 ^ L2 ^ R2;
		writeBlock(j, type);
		//printf("%04x %04x %04x %04x %04x %04x %04x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], P);
		//readBlock(j, type);
		//printf("%04x %04x %04x %04x %04x %04x %04x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], P);
		//printf("\n");
		//if (j > 100) exit(0);
	}
	return true;
}

uint32_t samplesCount(void){
	return(stairsCount * 3);
}

void printFrame(void){
	for(int j = 0; j < PCM_NTSC_HEIGHT + PCM_STAIRS; j++){
		for(int i = 0; i < PCM_WIDTH_BYTES; i++){
			fprintf(stderr, "0x%02x ", PCMFrame[j][i]);
		}
		fprintf(stderr, "\n");
	}
	fprintf(stderr, "\n");
}

void showStatistics(void){
	fprintf(stderr, "Stairs count: %d\n", stairsCount);
	fprintf(stderr, "CRC errors count: %d\n", crcErrorCount);
	fprintf(stderr, "Parity errors count: %d\n", parityErrorCount);
}
