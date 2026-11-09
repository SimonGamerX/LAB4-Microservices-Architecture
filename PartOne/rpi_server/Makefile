CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2 -pthread

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    LIBS = -lwiringPi
else
    LIBS = -lws2_32
endif

TARGETS = rpi1_service_server rpi2_service_server rpi_service_server

all: $(TARGETS)

rpi1_service_server: main_rpi1.cpp services_rpi1.cpp services_rpi1.hpp
	$(CXX) $(CXXFLAGS) main_rpi1.cpp services_rpi1.cpp $(LIBS) -o rpi1_service_server

rpi2_service_server: main_rpi2.cpp services_rpi2.cpp services_rpi2.hpp
	$(CXX) $(CXXFLAGS) main_rpi2.cpp services_rpi2.cpp $(LIBS) -o rpi2_service_server

rpi_service_server: main.cpp services.cpp services.hpp
	$(CXX) $(CXXFLAGS) main.cpp services.cpp $(LIBS) -o rpi_service_server

clean:
	rm -f $(TARGETS) *.exe *.o
