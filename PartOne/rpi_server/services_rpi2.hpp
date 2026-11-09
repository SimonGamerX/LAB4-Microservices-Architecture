#pragma once

#include <string>
#include <vector>

// Structure representing a service vector as specified in Lab 4 guide:
// (RPi identifier, service name, number of required input parameters, input parameter names)
struct ServiceDescriptor {
    std::string rpi_id;
    std::string service_name;
    int input_count;
    std::vector<std::string> param_names;
    std::string return_type; // "int" or "void"
    std::string description;

    std::string to_vector_string() const;
    std::string to_json() const;
};

// Global list of exposed services for RPi-2
std::vector<ServiceDescriptor> get_available_services(const std::string& rpi_id);

// RPi-2 Hardware Pin Definitions (Photoresistor & Alarm)
constexpr int PIN_RED_LED      = 16;   // Red LED (Alarm Actuator, RPi Pin 36)
constexpr int ADS1115_I2C_ADDR = 0x48; // ADS1115 16-bit I2C ADC (SDA=GPIO2, SCL=GPIO3)
constexpr int ADC_CH_LIGHT     = 0;    // Photoresistor (LDR) on Channel A0

// Service functions (C/C++ implementations on RPi-2)
void init_hardware();
int Calibrate_Sensors();
int Read_Light_Level();
void Set_Alarm_LED(int state);

// Low-level ADS1115 reader (16-bit ADC)
int read_ads1115_channel(int channel);

