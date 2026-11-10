CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2 -pthread

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    LIBS = -lwiringPi -lcrypt -lrt -lm
else
    LIBS = -lws2_32
endif

TARGET = rpi_service_server

all: $(TARGET)

$(TARGET): main.cpp services.cpp services.hpp
	$(CXX) $(CXXFLAGS) main.cpp services.cpp $(LIBS) -o $(TARGET)

clean:
	rm -f $(TARGET) *.exe *.o
