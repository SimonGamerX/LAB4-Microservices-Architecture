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

// Hardware pin definitions matching Lab 4 guide and metronome circuit
constexpr int PIN_RED_LED   = 16;  // Red LED (Alarm)
constexpr int PIN_GREEN_LED = 13;  // Green LED (Cooling / Fan)
constexpr int PIN_BUZZER    = 21;  // Optional buzzer pin

// MCP3008 Software SPI Pin configuration (Page 8-9 in PDF guide)
constexpr int PIN_SPI_CLK  = 18;  // Clock (RPi Pin 12)
constexpr int PIN_SPI_DOUT = 23;  // Data Out / MISO (RPi Pin 16)
constexpr int PIN_SPI_DIN  = 24;  // Data In / MOSI (RPi Pin 18)
constexpr int PIN_SPI_CS   = 25;  // Chip Select (RPi Pin 22)

// ADC Channels
constexpr int ADC_CH_LIGHT = 0;   // Photoresistor (LDR) on Channel 0
constexpr int ADC_CH_TEMP  = 1;   // Potentiometer / Thermistor on Channel 1

// Service functions (C/C++ implementations)
void init_hardware();
int Calibrate_Sensors();
int Read_Light_Level();
int Read_Temperature(char scale);
void Set_Alarm_LED(int state);
void Set_Cooling_Indicator(int state);
void Trigger_Buzzer(int duration_ms);

// Low-level MCP3008 reader
int read_mcp3008_channel(int channel);
