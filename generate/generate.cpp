#include "opencv2/opencv.hpp"
#include <iostream>
#include <cstdint>
#include <sndfile.h>
#include "../pcm.h"

#include <chrono>
#include <ctime>

using namespace std;
using namespace cv;

#define NEXT_FRAME_KEY 32
#define EXIT_KEY 27

#define PCM_PIXEL_0 30
#define PCM_PIXEL_1 127

SNDFILE *infile;
SF_INFO sfinfo;

void showHelp(void){
	std::cout << "Usage: -i input_file -o output_file [-s/b/f/16] " << endl;
	std::cout << "-i\t*.wav" << endl;
	std::cout << "-o\t*.avi" << endl;
	std::cout << "-v\tfor show output video" << endl;
	std::cout << "-16\tuse 16 bit pcm. Default - 14 bit" << endl;
	std::cout << "-f\tfor use Full PCM Frame" << endl;
	std::cout << "-s\tfor use step-by-step generation" << endl;
}

int main(int argc, char *argv[]){
	char  *inFileName = NULL;
	char *outFileName = NULL;
	bool b16 = false;
	bool show = false;
	bool step = false;
	bool fullPCM = false;

	if (argc == 1){
		showHelp();
		return 1;
	}

	for (int i=0; i < argc; i++){
		if (strcmp(argv[i], "-i") == 0)  inFileName = argv[i + 1];
		if (strcmp(argv[i], "-o") == 0) outFileName = argv[i + 1];
		if (strcmp(argv[i], "-16") == 0) b16 = true;
		if (strcmp(argv[i], "-v") == 0) show = true;
		if (strcmp(argv[i], "-s") == 0){
			show = true;
			step = true;
		}
		if (strcmp(argv[i], "-f") == 0) fullPCM= true;
		if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0)) {
			showHelp();
			return 0;
		}
	}

	// ToDo: line in!
	if (inFileName == NULL/* && !useDevice*/){
		showHelp();
		return 1;
	}

	if (outFileName == NULL && !show){
		showHelp();
		return 1;
	}

	if (inFileName != NULL){
		if ((infile = sf_open (inFileName, SFM_READ, &sfinfo)) == NULL){
			exit (1) ;
		}
	}

#define FRAME_HEIGHT 492
	//#define FRAME_HEIGHT 576

	VideoWriter video;
	if (outFileName != NULL){
		//int fourcc = CV_FOURCC('D', 'V', '2', '5');
		//video.open(outFileName, fourcc, 29.97, Size(720, FRAME_HEIGHT), false);
		// PNG костыль
		video.open("img_%04d.png", 0, 0, Size(720, FRAME_HEIGHT), false);
		// for windows
		//video.open(outFileName, 0, 29.97, Size(720, FRAME_HEIGHT), false);
		if (!video.isOpened() )
			throw(std::string("Could not open video file for write"));
	}

	uint16_t count = 0;

	auto start = chrono::steady_clock::now();
	while(1){

		Mat frame(FRAME_HEIGHT, 144, CV_8UC1, Scalar(PCM_PIXEL_0)); // 720 / 5 = 144
		Mat dst(FRAME_HEIGHT, 720, CV_8UC1, Scalar(PCM_PIXEL_0));
		Mat dst2;

		// If the frame is empty, break immediately
		if (frame.empty())
			break;

		if (!wav2PCMFrame(infile, b16)) break;
		writePCMFrame(frame, 0, fullPCM);
		if (!wav2PCMFrame(infile, b16)) break;
		writePCMFrame(frame, 1, fullPCM);

		count++;

		resize(frame, dst, dst.size(), 5, 0, INTER_NEAREST);

		if (video.isOpened()){
			video.write(dst);
		}

		if (show){
			imshow("Frame", dst);
      char c;
			if (step){
				while(1){
					// Press  ESC on keyboard to exit
					c = (char)waitKey(5);
					if(c == NEXT_FRAME_KEY || c == EXIT_KEY)
						break;
				}
				if(c == EXIT_KEY)
					break;
			} else {
				c = (char)waitKey(25);
				if(c == EXIT_KEY)
					break;
			}
		}
	}

	if (video.isOpened()){
		video.release();
	}

	auto end = chrono::steady_clock::now();
	float sec = chrono::duration_cast<chrono::milliseconds>(end - start).count() / 1000.0;
	std::cout << "Count frames: " << (uint32_t)count << std::endl;
	std::cout << "Time: " << sec << std::endl;
	std::cout << "FPS: " << (count / sec) << std::endl;

	// Closes all the frames
	destroyAllWindows();

	return 0;
}
