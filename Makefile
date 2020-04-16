CC=g++

CFLAGS=-c -Wall
LDFLAGS=  -lopencv_core
LDFLAGS+= -lopencv_imgproc
LDFLAGS+= -lopencv_highgui
LDFLAGS+= -lopencv_video
#LDFLAGS+= -lopencv_videoio
LDFLAGS+= -lsndfile

SOURCES=video.cpp pcm.cpp crc.cpp q_correction.cpp
OBJECTS=$(SOURCES:.cpp=.o)

EXECUTABLE=ggg

all: $(SOURCES) $(EXECUTABLE)


$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm *.o
