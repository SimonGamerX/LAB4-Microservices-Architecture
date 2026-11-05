#include "services.hpp"
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <cmath>
#include <cstdlib>

#if defined(__has_include)
  #if __has_include(<wiringPi.h>)
    #define REAL_WIRINGPI 1
    #include <wiringPi.h>
  #endif
#endif

#ifndef REAL_WIRINGPI
// Mock declarations for compilation on Windows / non-RPi development hosts
#define OUTPUT 1
#define INPUT 0
#define HIGH 1
#define LOW 0
static inline int wiringPiSetupGpio() { return 0; }
static inline void pinMode(int, int) {}
static inline void digitalWrite(int, int) {}
static inline int digitalRead(int) { return 0; }
static inline void delay(unsigned int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
#endif

// Internal hardware state
static bool s_hardware_initialized = false;
static bool s_is_calibrated = false;
static int s_red_led_state = 0;
static int s_green_led_state = 0;

// Format as exact service vector: (RPi identifier, service name, number of inputs, input parameter names)
std::string ServiceDescriptor::to_vector_string() const {
    std::ostringstream ss;
    ss << "(" << rpi_id << ", " << service_name << ", " << input_count;
    if (input_count > 0) {
        ss << ", ";
        for (size_t i = 0; i < param_names.size(); ++i) {
            ss << param_names[i];
            if (i + 1 < param_names.size()) ss << "/";
        }
    } else {
        ss << ", None";
    }
    ss << ")";
    return ss.str();
}

std::string ServiceDescriptor::to_json() const {
    std::ostringstream ss;
    ss << "{\"rpi_id\":\"" << rpi_id << "\","
       << "\"service_name\":\"" << service_name << "\","
       << "\"input_count\":" << input_count << ","
       << "\"param_names\":[";
    for (size_t i = 0; i < param_names.size(); ++i) {
        ss << "\"" << param_names[i] << "\"";
        if (i + 1 < param_names.size()) ss << ",";
    }
    ss << "],\"return_type\":\"" << return_type << "\","
       << "\"description\":\"" << description << "\"}";
    return ss.str();
}

std::vector<ServiceDescriptor> get_available_services(const std::string& rpi_id) {
    return {
        {rpi_id, "Calibrate_Sensors", 0, {}, "int", "Calibrates ADC channels and validates sensor baselines. Returns 1 on success."},
        {rpi_id, "Read_Light_Level", 0, {}, "int", "Reads ambient light intensity from LDR on ADC CH0. Returns percentage 0-100."},
        {rpi_id, "Read_Temperature", 1, {"scale"}, "int", "Reads analog thermal/potentiometer level from ADC CH1. Argument: 'C' or 'F'."},
        {rpi_id, "Set_Alarm_LED", 1, {"state"}, "void", "Controls Red Alarm LED on GPIO 16 (1 = ON, 0 = OFF)."},
        {rpi_id, "Set_Cooling_Indicator", 1, {"state"}, "void", "Controls Green Cooling LED on GPIO 13 (1 = ON, 0 = OFF)."},
        {rpi_id, "Trigger_Buzzer", 1, {"duration_ms"}, "void", "Emits an audible buzzer alert for the specified duration in ms."}
    };
}

void init_hardware() {
    if (s_hardware_initialized) return;

    wiringPiSetupGpio();

    // Actuator Output Pins
    pinMode(PIN_RED_LED, OUTPUT);
    pinMode(PIN_GREEN_LED, OUTPUT);
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_RED_LED, LOW);
    digitalWrite(PIN_GREEN_LED, LOW);
    digitalWrite(PIN_BUZZER, LOW);

    // MCP3008 Software SPI Pins (Page 8-9 in PDF)
    pinMode(PIN_SPI_CLK, OUTPUT);
    pinMode(PIN_SPI_DIN, OUTPUT);
    pinMode(PIN_SPI_CS, OUTPUT);
    pinMode(PIN_SPI_DOUT, INPUT);

    digitalWrite(PIN_SPI_CS, HIGH);
    digitalWrite(PIN_SPI_CLK, LOW);

    s_hardware_initialized = true;
    std::cout << "[Hardware] GPIO and Software SPI pins configured successfully." << std::endl;
}

// Software SPI MCP3008 bit-banging per PDF Appendix
int read_mcp3008_channel(int channel) {
    if (channel < 0 || channel > 7) return 0;
    init_hardware();

#ifndef REAL_WIRINGPI
    // Simulation fallback with realistic fluctuating analog values
    static int sim_counter = 0;
    sim_counter++;
    if (channel == ADC_CH_LIGHT) {
        // Simulates light between 200 and 850 (10-bit count)
        return 300 + (sim_counter * 37) % 550;
    } else {
        // Simulates temperature reading around 24C - 31C (equiv ADC count ~ 450 - 620)
        return 480 + (sim_counter * 29) % 200;
    }
#else
    // 1. Bring CS low to initiate SPI transaction
    digitalWrite(PIN_SPI_CS, LOW);
    digitalWrite(PIN_SPI_CLK, LOW);

    // Command byte: Start bit (1), Single-ended mode (1), followed by 3-bit channel address
    int command = 0x18 | (channel & 0x07); // 0b00011xxx
    command <<= 3; // Shift to send 5 bits

    // Clock out the 5 command bits (MOSI / DIN)
    for (int i = 0; i < 5; ++i) {
        digitalWrite(PIN_SPI_DIN, (command & 0x80) ? HIGH : LOW);
        command <<= 1;
        digitalWrite(PIN_SPI_CLK, HIGH);
        digitalWrite(PIN_SPI_CLK, LOW);
    }

    // Read 1 null bit + 10 data bits from DOUT (MISO)
    int adc_out = 0;
    for (int i = 0; i < 12; ++i) {
        digitalWrite(PIN_SPI_CLK, HIGH);
        digitalWrite(PIN_SPI_CLK, LOW);
        adc_out <<= 1;
        if (digitalRead(PIN_SPI_DOUT) == HIGH) {
            adc_out |= 0x1;
        }
    }

    // Bring CS high to end transaction
    digitalWrite(PIN_SPI_CS, HIGH);

    return adc_out & 0x3FF; // Mask 10-bit integer (0 - 1023)
#endif
}

// -------------------------------------------------------------
// Service Implementations (C/C++)
// -------------------------------------------------------------

int Calibrate_Sensors() {
    init_hardware();
    std::cout << "[Service] Executing Calibrate_Sensors()..." << std::endl;

    // Pulse indicators briefly to show calibration sequence
    digitalWrite(PIN_RED_LED, HIGH);
    digitalWrite(PIN_GREEN_LED, HIGH);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    digitalWrite(PIN_RED_LED, LOW);
    digitalWrite(PIN_GREEN_LED, LOW);

    // Warm-up samples on both channels
    int light_raw = read_mcp3008_channel(ADC_CH_LIGHT);
    int temp_raw = read_mcp3008_channel(ADC_CH_TEMP);

    std::cout << "[Service] Calibration completed. Raw Baseline -> Light: " 
              << light_raw << ", Temp/Pot: " << temp_raw << std::endl;

    s_is_calibrated = true;
    return 1; // Success status code
}

int Read_Light_Level() {
    init_hardware();
    int raw = read_mcp3008_channel(ADC_CH_LIGHT); // 0 - 1023
    // Map raw ADC reading to 0% - 100%
    int percentage = static_cast<int>((raw / 1023.0) * 100.0);
    if (percentage < 0) percentage = 0;
    if (percentage > 100) percentage = 100;

    std::cout << "[Service] Read_Light_Level() -> Raw: " << raw 
              << " | Output: " << percentage << "%" << std::endl;
    return percentage;
}

int Read_Temperature(char scale) {
    init_hardware();
    int raw = read_mcp3008_channel(ADC_CH_TEMP); // 0 - 1023
    
    // Convert 10-bit ADC reading across 3.3V reference to Celsius (15C to 45C span)
    double voltage = (raw * 3.3) / 1023.0;
    // Linear mapping: 0V -> 15°C, 3.3V -> 45°C
    double temp_c = 15.0 + (voltage / 3.3) * 30.0;
    int result = 0;

    if (scale == 'F' || scale == 'f') {
        result = static_cast<int>(std::round((temp_c * 9.0 / 5.0) + 32.0));
        std::cout << "[Service] Read_Temperature('F') -> Raw: " << raw 
                  << " | " << result << "°F" << std::endl;
    } else {
        result = static_cast<int>(std::round(temp_c));
        std::cout << "[Service] Read_Temperature('C') -> Raw: " << raw 
                  << " | " << result << "°C" << std::endl;
    }
    return result;
}

void Set_Alarm_LED(int state) {
    init_hardware();
    s_red_led_state = (state != 0) ? 1 : 0;
    digitalWrite(PIN_RED_LED, s_red_led_state ? HIGH : LOW);
    std::cout << "[Service] Set_Alarm_LED(" << s_red_led_state << ") -> Red LED is "
              << (s_red_led_state ? "ON" : "OFF") << std::endl;
}

void Set_Cooling_Indicator(int state) {
    init_hardware();
    s_green_led_state = (state != 0) ? 1 : 0;
    digitalWrite(PIN_GREEN_LED, s_green_led_state ? HIGH : LOW);
    std::cout << "[Service] Set_Cooling_Indicator(" << s_green_led_state << ") -> Green LED is "
              << (s_green_led_state ? "ON" : "OFF") << std::endl;
}

void Trigger_Buzzer(int duration_ms) {
    init_hardware();
    if (duration_ms <= 0) duration_ms = 100;
    if (duration_ms > 2000) duration_ms = 2000; // safety clamp
    std::cout << "[Service] Trigger_Buzzer(" << duration_ms << " ms)" << std::endl;
    digitalWrite(PIN_BUZZER, HIGH);
    std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
    digitalWrite(PIN_BUZZER, LOW);
}
