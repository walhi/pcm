#include "opencv2/opencv.hpp"
#include <iostream>
#include <cstdint>
#include <sndfile.h>

#include "pcm.h"


using namespace std;
using namespace cv;

SNDFILE *outfile;
SF_INFO sfinfo;

int main(){

  // Create a VideoCapture object and open the input file
  // If the input is the web camera, pass 0 instead of the video file name
  //VideoCapture cap("sine_1khz_16b_.avi");
  VideoCapture cap("16b.avi");
	//VideoCapture cap("sine_1kz_16b.avi");
	//VideoCapture cap("sin_walhi_1khz.avi");
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
		decodePCMFrame(outfile);

		preparePCMFrame(dst, 1);
		decodePCMFrame(outfile);

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

	sfinfo.frames = samplesCount();
  sf_close(outfile);

	showStatistics();

	return 0;
}
