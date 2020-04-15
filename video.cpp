#include "opencv2/opencv.hpp"
#include <iostream>
#include <cstdint>
#include <sndfile.h>

#include "pcm.h"

#include <chrono>
#include <ctime>

using namespace std;
using namespace cv;

SNDFILE *outfile;
SF_INFO sfinfo;

void showHelp(void){
	std::cout << "Usage: -i input_file -o output_file [-s/b/f/16] " << endl;
	std::cout << "-i\tavi, mp4..." << endl;
	std::cout << "-o\t*.wav or - for use pipe" << endl;
	std::cout << "-v\tfor show input video" << endl;
	std::cout << "-c\tfor show \"Copy out\" video" << endl;
	std::cout << "-s\tfor show input video step-by-step" << endl;
	std::cout << "-b\tfor show video after binarization" << endl;
	std::cout << "-16\tuse 16 bit pcm. Default - 14 bit" << endl;
}

#define NEXT_FRAME_KEY 32
#define EXIT_KEY 27
#define SHOW_FRAME_KEY 0x73

#define PCM_PIXEL_0 30
#define PCM_PIXEL_1 127

#define FRAME_HEIGHT 492

int main(int argc, char *argv[]){

	char  *inFileName = NULL;
	char *outFileName = NULL;
	bool show = false;
	bool showStep = false;
	bool showBin = false;
	bool copyOut = false;
	bool b16 = false;

	bool useDevice = false;

	int deviceID;

	if (argc == 1){
		showHelp();
		return 1;
	}

	// Перебираем каждый аргумент и выводим его порядковый номер и значение
	for (int i=0; i < argc; i++){
		if (strcmp(argv[i], "-i") == 0)  inFileName = argv[i + 1];
		if (strcmp(argv[i], "-o") == 0) outFileName = argv[i + 1];
		if (strcmp(argv[i], "-d") == 0) {
			useDevice = true;
			deviceID = std::stoi(argv[i + 1]);
		}
		if (strcmp(argv[i], "-16") == 0) b16 = true;
		if (strcmp(argv[i], "-v") == 0) show = true;
		if (strcmp(argv[i], "-b") == 0) {
			show = true;
			showBin = true;
		}
		if (strcmp(argv[i], "-s") == 0) {
			show = true;
			showStep = true;
		}
		if (strcmp(argv[i], "-c") == 0) {
			show = true;
			showBin = true;
			copyOut = true;
		}
		if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0)) {
			showHelp();
			return 0;
		}
		//std::cout << count << " " << argv[i] << '\n';
	}

	if (inFileName == NULL && !useDevice){
		showHelp();
		return 1;
	}

	if (outFileName == NULL && !show){
		showHelp();
		return 1;
	}

  // Create a VideoCapture object and open the input file
  // If the input is the web camera, pass 0 instead of the video file name
	VideoCapture cap;
	if (useDevice) {
		cap.open(deviceID);
	} else {
		cap.open(inFileName);
	}

  // Check if camera opened successfully
  if(!cap.isOpened()){
    cerr << "Error opening video stream or file" << endl;
    return -1;
  }

	if (outFileName != NULL){
		memset (&sfinfo, 0, sizeof (sfinfo)) ;
		sfinfo.samplerate	= 44100;
		//sfinfo.frames   = 100;
		sfinfo.channels = 2 ;
		if (strcmp(outFileName, "-") == 0){
			sfinfo.format   = (SF_FORMAT_AU | SF_FORMAT_PCM_16);
		} else {
			sfinfo.format   = (SF_FORMAT_WAV | SF_FORMAT_PCM_16);
		}


		if ((outfile = sf_open (outFileName, SFM_WRITE, &sfinfo)) == NULL){
			exit (1) ;
		}
	}

	auto start = chrono::steady_clock::now();
  uint32_t countFrames = 0;
  while(1){

    Mat srcFrame;
		Mat greyFrame;
		Mat binFrame;
		Mat fullFrame(FRAME_HEIGHT, 720, CV_8UC1, Scalar(PCM_PIXEL_0));

    // Capture frame-by-frame
    cap >> srcFrame;
		//bool bSuccess = cap.read(frame);

		// If the frame is empty, break immediately
		//if (!bSuccess)
		if (srcFrame.empty())
      break;

    countFrames++;

		if (srcFrame.size().width != 720)
			resize(srcFrame, srcFrame, Size(720, srcFrame.size().height), 0, 0, INTER_NEAREST);


		cvtColor(srcFrame, greyFrame, CV_BGR2GRAY);
    if (greyFrame.size().height == fullFrame.size().height){
      // послали итак целый PCM кадр
      threshold(greyFrame, fullFrame, 0, 255, THRESH_BINARY + THRESH_OTSU);
    } else {
      // Фрейм не полный.
      fprintf(stderr, "Frame not full. ");
      threshold(greyFrame, binFrame, 0, 255, THRESH_BINARY + THRESH_OTSU);
      // Перенос данных в конец полного фрейма. Подсчет количества пустых строк внизу
      for (int i = binFrame.size().height - 1; i >= 0; i--){
        if (searchStart(binFrame, i, NULL, NULL)){
          // PCM данные найдены. Перенесем их.
          fprintf(stderr, "Lines count: %d\n", i + 1);
          memcpy(fullFrame.data + fullFrame.elemSize() * fullFrame.size().width * (FRAME_HEIGHT - i - 1), binFrame.data, binFrame.elemSize() * binFrame.size().width * (i + 1));
          break;
        }
      }
    }

		if (outFileName != NULL){
			if (copyOut){
				Mat pcmFrame(FRAME_HEIGHT, 144, CV_8UC1, Scalar(PCM_PIXEL_0)); // 720 / 5 = 144
				readPCMFrame(fullFrame, 0);
				copyOutBuffer();
				writePCMFrame(pcmFrame, 0, true);
				PCMFrame2wav(outfile, b16);

				readPCMFrame(fullFrame, 1);
				copyOutBuffer();
				writePCMFrame(pcmFrame, 1, true);
				PCMFrame2wav(outfile, b16);
				resize(pcmFrame, fullFrame, fullFrame.size(), 5, 0, INTER_NEAREST);
			} else {
				readPCMFrame(fullFrame, 0);
				PCMFrame2wav(outfile, b16);
        if (showStep)
          printFrame();

				readPCMFrame(fullFrame, 1);
				PCMFrame2wav(outfile, b16);
        if (showStep)
          printFrame();
			}
		}


    for(int j = fullFrame.cols/4; j < fullFrame.cols/2; j++){
      fullFrame.at<uchar>(cv::Point(j, 0)) = 255;
    }

		if (show){
			char c;
			if (showBin){
				imshow("Frame", fullFrame);
			} else {
				imshow("Frame", srcFrame);
			}
			if (showStep){
				while(1){
					// Press  ESC on keyboard to exit
					c = (char)waitKey(5);
          if(c == SHOW_FRAME_KEY)
            printFrame();
					if(c == NEXT_FRAME_KEY || c == EXIT_KEY)
						break;
				}
				if(c == EXIT_KEY)
					break;
			} else {
				c = (char)waitKey(5);
				if(c == EXIT_KEY)
					break;
			}
		}
    //memsetBuffer(0xfa);

  }

  // When everything done, release the video capture object
  cap.release();

  // Closes all the frames
  destroyAllWindows();

	if (outFileName != NULL){
		sfinfo.frames = samplesCount();
		sf_close(outfile);

		showStatistics();
	}
  auto end = chrono::steady_clock::now();
	float sec = chrono::duration_cast<chrono::milliseconds>(end - start).count() / 1000.0;
	std::cout << "Count frames: " << (uint32_t)countFrames << std::endl;
	std::cout << "Time: " << sec << std::endl;
	std::cout << "FPS: " << (countFrames / sec) << std::endl;

	return 0;
}
