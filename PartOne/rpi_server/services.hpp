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

// Global list of exposed services
std::vector<ServiceDescriptor> get_available_services(const std::string& rpi_id);

// Actuator Pin Definitions (Lab 4)
constexpr int PIN_RED_LED      = 16;  // Red LED (Alarm Actuator, RPi Pin 36)
constexpr int PIN_GREEN_LED    = 13;  // Green LED (Cooling / Fan Actuator, RPi Pin 33)
constexpr int PIN_BUZZER       = 21;  // Optional buzzer pin (RPi Pin 40)

// ADS1115 16-bit I2C ADC Configuration (I2C: SDA=GPIO2, SCL=GPIO3)
constexpr int ADS1115_I2C_ADDR = 0x48; // Default I2C address with ADDR tied to GND
constexpr int ADC_CH_LIGHT     = 0;    // Photoresistor (LDR) on Channel A0

// KY-040 Rotary Encoder Pin Definitions (Simulated Thermostat Temperature Dial)
constexpr int PIN_ENCODER_CLK  = 17;   // Rotary Encoder CLK (RPi Pin 11)
constexpr int PIN_ENCODER_DT   = 27;   // Rotary Encoder DT  (RPi Pin 13)
constexpr int PIN_ENCODER_SW   = 22;   // Rotary Encoder Switch (Optional, RPi Pin 15)

// Service functions (C/C++ implementations)
void init_hardware();
int Calibrate_Sensors();
int Read_Light_Level();
int Read_Temperature(char scale);
void Set_Alarm_LED(int state);
void Set_Cooling_Indicator(int state);
void Trigger_Buzzer(int duration_ms);

// Low-level ADS1115 reader (16-bit ADC)
int read_ads1115_channel(int channel);
// Backward-compatible alias
inline int read_mcp3008_channel(int channel) { return read_ads1115_channel(channel); }
