cmake_minimum_required(VERSION 3.10)
project(RPiMicroserviceServer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_library(WIRINGPI_LIB wiringPi)

add_executable(rpi_service_server
    main.cpp
    services.cpp
    services.hpp
)

target_link_libraries(rpi_service_server PRIVATE Threads::Threads)

if(WIRINGPI_LIB)
    message(STATUS "Found wiringPi: ${WIRINGPI_LIB}")
    target_link_libraries(rpi_service_server PRIVATE ${WIRINGPI_LIB})
    target_compile_definitions(rpi_service_server PRIVATE REAL_WIRINGPI=1)
else()
    message(STATUS "wiringPi not found. Compiling in mock/simulation mode.")
endif()

if(WIN32)
    target_link_libraries(rpi_service_server PRIVATE ws2_32)
endif()
