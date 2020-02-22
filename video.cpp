#include "opencv2/opencv.hpp"
#include <iostream>
#include <cstdint>
#include <sndfile.h>

#include "pcm.h"


using namespace std;
using namespace cv;

SNDFILE *outfile;
SF_INFO sfinfo;

void showHelp(void){
	std::cout << "Usage: -i input_file -o output_file [-s/16] " << endl;
	std::cout << "-i\tavi, mp4..." << endl;
	std::cout << "-o\t*.wav or - for use pipe" << endl;
	std::cout << "-s\tfor show input video" << endl;
	std::cout << "-16\tuse 16 bit pcm. Default - 14 bit" << endl;
}

int main(int argc, char *argv[]){

	char  *inFileName = NULL;
	char *outFileName = NULL;
	bool show = false;
	bool b16 = false;

	if (argc == 1){
		showHelp();
		return 1;
	}

	// Перебираем каждый аргумент и выводим его порядковый номер и значение
	for (int i=0; i < argc; i++){
		if (strcmp(argv[i], "-i") == 0)  inFileName = argv[i + 1];
		if (strcmp(argv[i], "-o") == 0) outFileName = argv[i + 1];
		if (strcmp(argv[i], "-16") == 0) b16 = true;
		if (strcmp(argv[i], "-s") == 0) show = true;
		if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0)) {
			showHelp();
			return 0;
		}
		//std::cout << count << " " << argv[i] << '\n';
	}

	if (inFileName == NULL){
		showHelp();
		return 1;
	}

	if (outFileName == NULL){
		showHelp();
		return 1;
	}

  // Create a VideoCapture object and open the input file
  // If the input is the web camera, pass 0 instead of the video file name
  VideoCapture cap(inFileName);

  // Check if camera opened successfully
  if(!cap.isOpened()){
    cerr << "Error opening video stream or file" << endl;
    return -1;
  }

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
		decodePCMFrame(outfile, b16);

		preparePCMFrame(dst, 1);
		decodePCMFrame(outfile, b16);

    //imshow("Frame", dst);
		if (show)
			imshow("Frame", frame);

    // Press  ESC on keyboard to exit
    char c=(char)waitKey(25);
    if(c==27)
      break;
  }

  // When everything done, release the video capture object
  cap.release();

  // Closes all the frames
  destroyAllWindows();

	sfinfo.frames = samplesCount();
  sf_close(outfile);

	showStatistics();

	return 0;
}
