CC=g++
CFLAGS=-c -Wall
LDFLAGS=-lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_ml -lopencv_video -lopencv_features2d -lopencv_calib3d -lopencv_objdetect -lopencv_contrib -lopencv_legacy -lopencv_stitching -lsndfile -lasound
SOURCES=video.cpp pcm.cpp crc.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=ggg

all: $(SOURCES) $(EXECUTABLE)


$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@
