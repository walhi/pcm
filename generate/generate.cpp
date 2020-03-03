#include "opencv2/opencv.hpp"
#include <iostream>
#include <cstdint>

using namespace std;
using namespace cv;

#define NEXT_FRAME_KEY 32
#define EXIT_KEY 27

#define PCM_PIXEL_0 30
#define PCM_PIXEL_1 127


void showHelp(void){
	std::cout << "Usage: -i input_file -o output_file [-s/b/f/16] " << endl;
	std::cout << "-i\tavi, mp4..." << endl;
	std::cout << "-o\t*.wav or - for use pipe" << endl;
	std::cout << "-s\tfor show input video" << endl;
	std::cout << "-b\tfor show video after binarization" << endl;
	std::cout << "-16\tuse 16 bit pcm. Default - 14 bit" << endl;
	std::cout << "-f\tfor use Full PCM Frame" << endl;
}

int main(int argc, char *argv[]){
	char *inFileName = NULL;
	bool b16 = false;
	bool fullPCM = false;

	if (argc == 1){
		showHelp();
		return 1;
	}

	for (int i=0; i < argc; i++){
		if (strcmp(argv[i], "-i") == 0)  inFileName = argv[i + 1];
		//if (strcmp(argv[i], "-o") == 0) outFileName = argv[i + 1];
		//if (strcmp(argv[i], "-d") == 0) {
		//	useDevice = true;
		//	deviceID = std::stoi(argv[i + 1]);
		//}
		if (strcmp(argv[i], "-16") == 0) b16 = true;
		//if (strcmp(argv[i], "-s") == 0) show = true;
		if (strcmp(argv[i], "-f") == 0) fullPCM= true;
		//if (strcmp(argv[i], "-b") == 0) {
		//	show = true;
		//	showBin = true;
		//}
		if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0)) {
			showHelp();
			return 0;
		}
		//std::cout << count << " " << argv[i] << '\n';
	}

	// ToDo: line in!
	if (inFileName == NULL/* && !useDevice*/){
		showHelp();
		return 1;
	}

	if (inFileName != NULL){
		if ((outfile = sf_open (outFileName, SFM_READ, &sfinfo)) == NULL){
			exit (1) ;
		}
	}

	uint16_t line = 0;
	while(1){

		Mat frame(482, 700, CV_8UC1, Scalar(PCM_PIXEL_0));

		// If the frame is empty, break immediately
		if (frame.empty())
			break;

		//memset()
		for (uint16_t i = 0; i < 700; i++){
			frame.at<uchar>(Point(i, line)) = PCM_PIXEL_1;
		}


		imshow("Frame", frame);

		char c;
		while(1){
			// Press  ESC on keyboard to exit
			c = (char)waitKey(5);
			if(c == NEXT_FRAME_KEY || c == EXIT_KEY)
				break;
		}
		line++;
		if(c == EXIT_KEY)
			break;
	}

	// Closes all the frames
	destroyAllWindows();

	return 0;
}
