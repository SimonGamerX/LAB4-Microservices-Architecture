cmake_minimum_required(VERSION 3.10)
project(rpi_microservices)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WIN32)
    set(EXTRA_LIBS ws2_32)
else()
    set(EXTRA_LIBS wiringPi pthread)
endif()

# Standalone RPi Service Server (hosting logical RPi-1 and RPi-2 services)
add_executable(rpi_service_server
    main.cpp
    services.cpp
)
target_link_libraries(rpi_service_server PRIVATE ${EXTRA_LIBS})
