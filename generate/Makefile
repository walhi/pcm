CC=g++
#CC=x86_64-w64-mingw32-g++

CFLAGS=-c -Wall
LDFLAGS=  -lopencv_core
LDFLAGS+= -lopencv_imgproc
LDFLAGS+= -lopencv_highgui
LDFLAGS+= -lopencv_video
LDFLAGS+= -lopencv_videoio
LDFLAGS+= -lsndfile

SOURCES=generate.cpp
SOURCES+=../pcm.cpp
SOURCES+=../crc.cpp
SOURCES+=../q_correction.cpp
OBJECTS=$(SOURCES:.cpp=.o)

EXECUTABLE=encoder

all: $(SOURCES) $(EXECUTABLE)


$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm *.o
