cmake_minimum_required(VERSION 3.10)
project(rpi_microservices)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WIN32)
    set(EXTRA_LIBS ws2_32)
else()
    set(EXTRA_LIBS wiringPi pthread)
endif()

# RPi-1 Target: Thermostat Dial & Cooling Fan LED
add_executable(rpi1_service_server
    main_rpi1.cpp
    services_rpi1.cpp
)
target_link_libraries(rpi1_service_server PRIVATE ${EXTRA_LIBS})

# RPi-2 Target: Light Sensor & Alarm LED
add_executable(rpi2_service_server
    main_rpi2.cpp
    services_rpi2.cpp
)
target_link_libraries(rpi2_service_server PRIVATE ${EXTRA_LIBS})

# Unified Server Target (Single process alternative)
add_executable(rpi_service_server
    main.cpp
    services.cpp
)
target_link_libraries(rpi_service_server PRIVATE ${EXTRA_LIBS})
