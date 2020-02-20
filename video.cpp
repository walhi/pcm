#include "opencv2/opencv.hpp"
#include <iostream>
#include <cstdint>
#include <sndfile.h>

using namespace std;
using namespace cv;

#define FRAME_HEIGHT 480
#define PCM_HEIGHT 240
#define PCM_WIDTH_BITS 128
#define PCM_WIDTH_BYTES PCM_WIDTH_BITS/8
#define PCM_STAIRS 112 // 7*16

#define L0_POS 0
#define R0_POS 16
#define L1_POS 32
#define R1_POS 48
#define L2_POS 64
#define R2_POS 80
#define P_POS  96
#define Q_POS  112
#define S_POS  112


#define PIXEL(frame, x, y) ((*(frame.ptr(x, y)))?1:0)
#define SWAP_PCM_FRAME() memcpy(&(PCMFrame[0]), &(PCMFrame[PCM_HEIGHT]), (PCM_STAIRS * PCM_WIDTH_BYTES))

uint8_t PCMFrame[PCM_HEIGHT + PCM_STAIRS][PCM_WIDTH_BYTES];
uint32_t stairsCount = 0;
uint32_t crcErrorCount = 0;
uint32_t parityErrorCount = 0;

SNDFILE *outfile;

static const uint16_t CRC_CCITT_TABLE[256] =
{
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define for_endian(size) for (int i = 0; i < size; ++i)
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define for_endian(size) for (int i = size - 1; i >= 0; --i)
#else
#error "Endianness not detected"
#endif

#define printb(value)                                   \
({                                                      \
        typeof(value) _v = value;                       \
        __printb((typeof(_v) *) &_v, sizeof(_v));       \
})

void __printb(void *value, size_t size)
{
        uint8_t byte;
        size_t blen = sizeof(byte) * 8;
        uint8_t bits[blen + 1];

        bits[blen] = '\0';
        for_endian(size) {
                byte = ((uint8_t *) value)[i];
                memset(bits, '0', blen);
                for (int j = 0; byte && j < blen; ++j) {
                        if (byte & 0x80)
                                bits[j] = '1';
                        byte <<= 1;
                }
                printf("%s ", bits);
        }
        //printf("\n");
}



uint16_t Calculate_CRC_CCITT(const uint8_t* buffer, int size)
{
    uint16_t tmp;
    uint16_t crc = 0xffff;

    for (int i = 0; i < size; i++)
    {
        tmp = (crc >> 8) ^ buffer[i];
        crc = (crc << 8) ^ CRC_CCITT_TABLE[tmp];
    }

    return crc;
}

int start = 24;
int end = 677;
float pixelSize = 5.1015625;

void preparePCMFrame(Mat frame, uint8_t offset){
	uint8_t *line;
	for(int j = 0; j < PCM_HEIGHT; j++){
		line = PCMFrame[j + PCM_STAIRS];
		memset(line, 0, PCM_WIDTH_BYTES);
		for(int i = 0; i < PCM_WIDTH_BITS; i ++){
			line[i >> 3] |= PIXEL(frame, j * 2 + offset, (int)(i * pixelSize + start + 2)) << (7 - i & 0b111); // /8 and %8
		}
		uint16_t crcCheck = Calculate_CRC_CCITT(line, 14);
		uint16_t crcOriginal = line[14] << 8 | line[15];
		if (crcCheck != crcOriginal){
			line[14] = 0;
			line[15] = 0;
			//printf("CRC ERROR 0x%04x 0x%04x\n", crcOriginal, crcCheck);
			crcErrorCount++;
		}
		//for(int i = 0; i < PCM_WIDTH_BYTES; i++)
		//	printb(PCMFrame[j][i]);
		//printf("\n");
	}
}

#define L0 buf[0]
#define R0 buf[1]
#define L1 buf[2]
#define R1 buf[3]
#define L2 buf[4]
#define R2 buf[5]

void decodePCMFrame(){
	uint16_t buf[6];
	for(int j = 0; j < PCM_HEIGHT; j++){
		/*
		for(int i = 0; i < PCM_WIDTH_BYTES; i++)
			printb(PCMFrame[j + L0_POS][i]);
		printf("\n");
		for(int i = 0; i < PCM_WIDTH_BYTES; i++)
			printb(PCMFrame[j + R0_POS][i]);
		printf("\n");
		for(int i = 0; i < PCM_WIDTH_BYTES; i++)
			printb(PCMFrame[j + L1_POS][i]);
		printf("\n");
		for(int i = 0; i < PCM_WIDTH_BYTES; i++)
			printb(PCMFrame[j + R1_POS][i]);
		printf("\n");
		for(int i = 0; i < PCM_WIDTH_BYTES; i++)
			printb(PCMFrame[j + L2_POS][i]);
		printf("\n");
		for(int i = 0; i < PCM_WIDTH_BYTES; i++)
			printb(PCMFrame[j + R2_POS][i]);
		printf("\n");
		for(int i = 0; i < PCM_WIDTH_BYTES; i++)
			printb(PCMFrame[j + P_POS][i]);
		printf("\n");
		*/
		stairsCount++;

		//printf("%d\n", j);

		L0 = (( PCMFrame[j + L0_POS][0] << 6)          | (PCMFrame[j + L0_POS][1] >> 2)) << 2;
		R0 = (((PCMFrame[j + R0_POS][1] & 0x03) << 12) | (PCMFrame[j + R0_POS][2] << 4) | (PCMFrame[j + R0_POS][3] >> 4)) << 2;
		L1 = (((PCMFrame[j + L1_POS][3] & 0x0f) << 10) | (PCMFrame[j + L1_POS][4] << 2) | (PCMFrame[j + L1_POS][5] >> 6)) << 2;
		R1 = (((PCMFrame[j + R1_POS][5] & 0x3f) << 8)  | (PCMFrame[j + R1_POS][6]     )) << 2;
		L2 = (( PCMFrame[j + L2_POS][7] << 6)          | (PCMFrame[j + L2_POS][8] >> 2)) << 2;
		R2 = (((PCMFrame[j + R2_POS][8] & 0x03) << 12) | (PCMFrame[j + R2_POS][9] << 4) | (PCMFrame[j + R2_POS][10] >> 4)) << 2;
		uint16_t P  = (((PCMFrame[j + P_POS][10] & 0x0f) << 10) | (PCMFrame[j + P_POS][11] << 2) | ((PCMFrame[j + P_POS][12] >> 6) & 0x03)) << 2;
		uint16_t Q  = (((PCMFrame[j + Q_POS][12] & 0x3f) << 8) | (PCMFrame[j + Q_POS][13])) << 2;

		// 16 бит PCM
		if (0){
			L0 |= (PCMFrame[j + L0_POS][12] >> 4) & 0x03;
			R0 |= (PCMFrame[j + R0_POS][12] >> 2) & 0x03;
			L1 |= (PCMFrame[j + L1_POS][12] >> 0) & 0x03;
			R1 |= (PCMFrame[j + R1_POS][13] >> 6) & 0x03;
			L2 |= (PCMFrame[j + L2_POS][13] >> 4) & 0x03;
			R2 |= (PCMFrame[j + R2_POS][13] >> 2) & 0x03;
			P  |= (PCMFrame[j +  P_POS][13] >> 0) & 0x03;
		}

		//for(int i = 0; i < 6; i++){
		//	printb(buf[i]);
		//	//printf("%04x\n", buf[i]);
		//	printf("\n");
		//}

		uint8_t crcErrors = PCMFrame[j + L0_POS][15]?0:1 + PCMFrame[j + R0_POS][15]?0:1 + PCMFrame[j + L1_POS][15]?0:1 + PCMFrame[j + R1_POS][15]?0:1 + PCMFrame[j + L2_POS][15]?0:1 + PCMFrame[j + R2_POS][15]?0:1 + PCMFrame[j + P_POS][15]?0:1;
		//printf("%04x\n", buf[i]);;

		uint16_t PC = L0 ^ R0 ^ L1 ^ R1 ^ L2 ^ R2;
		if (P != PC){
			//printb(P);
			//printf("\n");
			//printb(PC);
			//printf("\n");
			parityErrorCount++;
			if (crcErrors)
				memset(buf, 0, sizeof(buf));
			//printf("Parity error\n");
		}

		//printf("\n" );
		sf_write_short(outfile, (int16_t *)buf, 6);
	}
}

SF_INFO sfinfo;

int main(){

  // Create a VideoCapture object and open the input file
  // If the input is the web camera, pass 0 instead of the video file name
  //VideoCapture cap("sine_1khz_16b_.avi");
  VideoCapture cap("sine_1kz_16b.avi");
  //VideoCapture cap("OVER9000!!!.avi");

  // Check if camera opened successfully
  if(!cap.isOpened()){
    cout << "Error opening video stream or file" << endl;
    return -1;
  }


	memset (&sfinfo, 0, sizeof (sfinfo)) ;
	sfinfo.samplerate	= 44100;
	//sfinfo.frames   = 100;
	sfinfo.channels = 2 ;
	sfinfo.format   = (SF_FORMAT_WAV | SF_FORMAT_PCM_16) ;

	if ((outfile = sf_open ("test.wav", SFM_WRITE, &sfinfo)) == NULL){
    exit (1) ;
  }

  while(1){

    Mat frame;
		Mat dst;
    // Capture frame-by-frame
    cap >> frame;

    // If the frame is empty, break immediately
    if (frame.empty())
      break;

    threshold(frame, dst, 79, 255, THRESH_BINARY);

		preparePCMFrame(dst, 0);
		decodePCMFrame();
		SWAP_PCM_FRAME();

		preparePCMFrame(dst, 1);
		decodePCMFrame();
		SWAP_PCM_FRAME();

		//break;

    // Display the resulting frame
    //imshow( "Frame", dst);

    // Press  ESC on keyboard to exit
    char c=(char)waitKey(25);
    if(c==27)
      break;
  }

  // When everything done, release the video capture object
  cap.release();

  // Closes all the frames
  destroyAllWindows();

	sfinfo.frames = stairsCount * 3;
  sf_close(outfile);


	printf("Stairs count: %d\n", stairsCount);
	printf("CRC errors count: %d\n", crcErrorCount);
	printf("Parity errors count: %d\n", parityErrorCount);

	return 0;
}
