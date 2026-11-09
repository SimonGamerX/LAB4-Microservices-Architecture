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

// Global list of exposed services for RPi-1
std::vector<ServiceDescriptor> get_available_services(const std::string& rpi_id);

// RPi-1 Hardware Pin Definitions (Thermostat & Cooling Fan)
constexpr int PIN_GREEN_LED   = 12;  // Green LED (Cooling / Fan Actuator, RPi Pin 32)
constexpr int PIN_ENCODER_CLK = 20;  // Rotary Encoder CLK (RPi Pin 38)
constexpr int PIN_ENCODER_DT  = 21;  // Rotary Encoder DT  (RPi Pin 40)

// Service functions (C/C++ implementations on RPi-1)
void init_hardware();
int Calibrate_Sensors();
int Read_Temperature(char scale);
void Set_Cooling_Indicator(int state);

