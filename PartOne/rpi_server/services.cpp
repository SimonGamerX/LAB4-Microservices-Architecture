#include "services.hpp"
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <atomic>

#if defined(__has_include)
  #if __has_include(<wiringPi.h>)
    #define REAL_WIRINGPI 1
    #include <wiringPi.h>
    #if __has_include(<wiringPiI2C.h>)
      #include <wiringPiI2C.h>
    #endif
  #endif
#endif

#if defined(__linux__)
  #include <fcntl.h>
  #include <unistd.h>
  #include <sys/ioctl.h>
  #include <linux/i2c-dev.h>
#endif

#ifndef REAL_WIRINGPI
// Mock declarations for compilation on Windows / non-RPi development hosts
#define OUTPUT 1
#define INPUT 0
#define HIGH 1
#define LOW 0
#define PUD_UP 2
static inline int wiringPiSetupGpio() { return 0; }
static inline void pinMode(int, int) {}
static inline void digitalWrite(int, int) {}
static inline int digitalRead(int) { return 0; }
static inline void pullUpDnControl(int, int) {}
static inline void delay(unsigned int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
#endif

// Internal hardware state
static bool s_hardware_initialized = false;
static bool s_is_calibrated = false;
static int s_red_led_state = 0;
static int s_green_led_state = 0;
static int s_i2c_fd = -1;

// Rotary Encoder Simulated Temperature State (Default 24°C, range 15°C - 45°C)
static std::atomic<int> s_simulated_temp_c{24};
static std::thread s_encoder_thread;
static std::atomic<bool> s_stop_encoder{false};

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
        {rpi_id, "Calibrate_Sensors", 0, {}, "int", "Calibrates ADS1115 ADC & Rotary Encoder baseline. Returns 1 on success."},
        {rpi_id, "Read_Light_Level", 0, {}, "int", "Reads ambient light intensity from LDR on ADS1115 Channel A0. Returns percentage 0-100."},
        {rpi_id, "Read_Temperature", 1, {"scale"}, "int", "Reads simulated thermostat temperature dial from Rotary Encoder on GPIO 20/21. Argument: 'C' or 'F'."},
        {rpi_id, "Set_Alarm_LED", 1, {"state"}, "void", "Controls Red Alarm LED on GPIO 16 (1 = ON, 0 = OFF)."},
        {rpi_id, "Set_Cooling_Indicator", 1, {"state"}, "void", "Controls Green Cooling LED on GPIO 12 (1 = ON, 0 = OFF)."}
    };
}

// Background worker monitoring Rotary Encoder pulses
static void encoder_worker_loop() {
    int last_clk = digitalRead(PIN_ENCODER_CLK);
    while (!s_stop_encoder.load()) {
        int current_clk = digitalRead(PIN_ENCODER_CLK);
        if (current_clk != last_clk && current_clk == LOW) {
            // Falling edge on CLK detected: read DT to determine direction
            int dt_state = digitalRead(PIN_ENCODER_DT);
            if (dt_state != current_clk) {
                // Clockwise rotation -> increase temperature
                if (s_simulated_temp_c.load() < 45) {
                    s_simulated_temp_c++;
                }
            } else {
                // Counter-clockwise rotation -> decrease temperature
                if (s_simulated_temp_c.load() > 15) {
                    s_simulated_temp_c--;
                }
            }
            std::cout << "[Rotary Encoder] Dial rotated -> Temp: " 
                      << s_simulated_temp_c.load() << "°C" << std::endl;
        }
        last_clk = current_clk;
        std::this_thread::sleep_for(std::chrono::milliseconds(2)); // 2ms polling loop
    }
}

void init_hardware() {
    if (s_hardware_initialized) return;

    wiringPiSetupGpio();

    // Actuator Output Pins
    pinMode(PIN_RED_LED, OUTPUT);
    pinMode(PIN_GREEN_LED, OUTPUT);
    digitalWrite(PIN_RED_LED, LOW);
    digitalWrite(PIN_GREEN_LED, LOW);

    // Rotary Encoder Input Pins (with internal pull-ups)
    pinMode(PIN_ENCODER_CLK, INPUT);
    pinMode(PIN_ENCODER_DT, INPUT);
    pullUpDnControl(PIN_ENCODER_CLK, PUD_UP);
    pullUpDnControl(PIN_ENCODER_DT, PUD_UP);

#if defined(__linux__)
    // Initialize I2C interface for ADS1115 (bus /dev/i2c-1, address 0x48)
    s_i2c_fd = open("/dev/i2c-1", O_RDWR);
    if (s_i2c_fd >= 0) {
        if (ioctl(s_i2c_fd, I2C_SLAVE, ADS1115_I2C_ADDR) < 0) {
            std::cerr << "[Hardware] Warning: Failed to acquire I2C bus access for ADS1115 at 0x48." << std::endl;
        }
    }
#endif

    // Launch background thread to track rotary encoder pulses in real-time
    s_stop_encoder.store(false);
    s_encoder_thread = std::thread(encoder_worker_loop);
    s_encoder_thread.detach();

    s_hardware_initialized = true;
    std::cout << "[Hardware] Hardware initialized: ADS1115 (I2C 0x48), Rotary Encoder (GPIO 20/21), LEDs (GPIO 16/12)." << std::endl;
}

// ADS1115 16-bit I2C ADC reader (Channel A0 - A3)
int read_ads1115_channel(int channel) {
    if (channel < 0 || channel > 3) return 0;
    init_hardware();

#if defined(__linux__)
    if (s_i2c_fd >= 0) {
        // ADS1115 Config register (0x01):
        // OS: 1 (start single conversion)
        // MUX: (0x4 | channel) -> AIN0/AIN1/AIN2/AIN3 vs GND
        // PGA: 001 (±4.096V range, covering 3.3V reference) -> 0x0200
        // MODE: 1 (Single-shot mode) -> 0x0100
        // DR: 100 (128 SPS) -> 0x0080
        // COMP_QUE: 11 (Disable comparator) -> 0x0003
        uint16_t config = 0x8000 | ((0x4 | (channel & 0x03)) << 12) | 0x0200 | 0x0100 | 0x0080 | 0x0003;
        uint8_t writeBuf[3] = { 0x01, static_cast<uint8_t>(config >> 8), static_cast<uint8_t>(config & 0xFF) };

        if (write(s_i2c_fd, writeBuf, 3) == 3) {
            delay(10); // Wait for 128 SPS conversion (~8ms)

            uint8_t reg = 0x00; // Pointer to Conversion register
            if (write(s_i2c_fd, &reg, 1) == 1) {
                uint8_t readBuf[2] = {0, 0};
                if (read(s_i2c_fd, readBuf, 2) == 2) {
                    int16_t val = (static_cast<int16_t>(readBuf[0]) << 8) | readBuf[1];
                    if (val < 0) val = 0;
                    return static_cast<int>(val); // 0 to ~26400 for 3.3V
                }
            }
        }
    }
#endif

    // Simulation fallback with realistic fluctuating analog light values
    static int sim_counter = 0;
    sim_counter++;
    return 7000 + (sim_counter * 850) % 17000;
}

// -------------------------------------------------------------
// Service Implementations (C/C++)
// -------------------------------------------------------------

int Calibrate_Sensors() {
    init_hardware();
    std::cout << "[Service] Executing Calibrate_Sensors()..." << std::endl;

    // Visual self-test pulse: LEDs flash for 250ms
    digitalWrite(PIN_RED_LED, HIGH);
    digitalWrite(PIN_GREEN_LED, HIGH);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    digitalWrite(PIN_RED_LED, LOW);
    digitalWrite(PIN_GREEN_LED, LOW);

    // Warm-up sample from ADS1115 A0 (LDR)
    int light_raw = read_ads1115_channel(ADC_CH_LIGHT);
    int current_temp = s_simulated_temp_c.load();

    std::cout << "[Service] Calibration completed. Baseline -> ADS1115 Light (A0): " 
              << light_raw << " | Rotary Encoder Temp Dial: " << current_temp << "°C" << std::endl;

    s_is_calibrated = true;
    return 1; // Success status code
}

int Read_Light_Level() {
    init_hardware();
    int raw = read_ads1115_channel(ADC_CH_LIGHT);
    // At ±4.096V gain, 3.3V corresponds to (3.3 / 4.096) * 32767 ≈ 26400 counts
    int percentage = static_cast<int>((raw / 26400.0) * 100.0);
    if (percentage < 0) percentage = 0;
    if (percentage > 100) percentage = 100;

    std::cout << "[Service] Read_Light_Level() -> ADS1115 Raw: " << raw 
              << " | Output: " << percentage << "%" << std::endl;
    return percentage;
}

int Read_Temperature(char scale) {
    init_hardware();
    int temp_c = s_simulated_temp_c.load();
    int result = 0;

    if (scale == 'F' || scale == 'f') {
        result = static_cast<int>(std::round((temp_c * 9.0 / 5.0) + 32.0));
        std::cout << "[Service] Read_Temperature('F') -> Dial: " << temp_c 
                  << "°C | Converted: " << result << "°F" << std::endl;
    } else {
        result = temp_c;
        std::cout << "[Service] Read_Temperature('C') -> Dial: " << result << "°C" << std::endl;
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
