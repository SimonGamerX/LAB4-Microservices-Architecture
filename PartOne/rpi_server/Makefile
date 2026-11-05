CXX ?= g++
CXXFLAGS ?= -O2 -Wall -std=c++17 -pthread
TARGET = rpi_service_server

SRCS = main.cpp services.cpp
OBJS = $(SRCS:.cpp=.o)

# Check if wiringPi library is available
WIRINGPI_EXISTS := $(shell ldconfig -p | grep libwiringPi || true)

ifneq ($(strip $(WIRINGPI_EXISTS)),)
    LDFLAGS += -lwiringPi
    CXXFLAGS += -DREAL_WIRINGPI=1
    $(info [+] Found wiringPi on system. Linking against -lwiringPi.)
else
    $(info [-] wiringPi not found on build host. Building in mock/simulation mode.)
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
