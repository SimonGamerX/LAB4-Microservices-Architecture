#include "services_rpi1.hpp"
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <atomic>
#include <cstdlib>

#if defined(__linux__)
  #include <wiringPi.h>
#else
  // Simulation stubs for Windows/macOS testing
  #define LOW 0
  #define HIGH 1
  #define INPUT 0
  #define OUTPUT 1
  #define PUD_UP 2
  inline void wiringPiSetupGpio() {}
  inline void pinMode(int, int) {}
  inline void digitalWrite(int, int) {}
  inline int digitalRead(int) { return 0; }
  inline void pullUpDnControl(int, int) {}
#endif

// Internal hardware state
static std::atomic<bool> s_hardware_initialized{false};
static std::atomic<int> s_simulated_temp_c{24}; // Default room temperature
static std::atomic<int> s_green_led_state{0};
static std::thread s_encoder_thread;
static std::atomic<bool> s_stop_encoder{false};

std::string ServiceDescriptor::to_vector_string() const {
    std::ostringstream ss;
    ss << "(" << rpi_id << ", " << service_name << ", " << input_count;
    if (param_names.empty()) {
        ss << ", None)";
    } else {
        ss << ", ";
        for (size_t i = 0; i < param_names.size(); ++i) {
            ss << param_names[i] << (i + 1 < param_names.size() ? "/" : "");
        }
        ss << ")";
    }
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
        {rpi_id, "Calibrate_Sensors", 0, {}, "int", "Calibrates Rotary Encoder baseline & tests Green Cooling LED."},
        {rpi_id, "Read_Temperature", 1, {"scale"}, "int", "Reads simulated thermostat dial from Rotary Encoder on GPIO 20/21. Argument: 'C' or 'F'."},
        {rpi_id, "Set_Cooling_Indicator", 1, {"state"}, "void", "Controls Green Cooling LED on GPIO 12 (1 = ON, 0 = OFF)."}
    };
}

// Background worker monitoring Rotary Encoder pulses
static void encoder_worker_loop() {
    int last_clk = digitalRead(PIN_ENCODER_CLK);
    while (!s_stop_encoder.load()) {
        int current_clk = digitalRead(PIN_ENCODER_CLK);
        if (current_clk != last_clk && current_clk == LOW) {
            int dt_state = digitalRead(PIN_ENCODER_DT);
            if (dt_state != current_clk) {
                if (s_simulated_temp_c.load() < 45) {
                    s_simulated_temp_c++;
                }
            } else {
                if (s_simulated_temp_c.load() > 15) {
                    s_simulated_temp_c--;
                }
            }
            std::cout << "[RPi-1 Rotary Encoder] Dial rotated -> Temp: " 
                      << s_simulated_temp_c.load() << "°C" << std::endl;
        }
        last_clk = current_clk;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}

void init_hardware() {
    if (s_hardware_initialized) return;

    wiringPiSetupGpio();

    // Actuator Pin: Green Cooling LED
    pinMode(PIN_GREEN_LED, OUTPUT);
    digitalWrite(PIN_GREEN_LED, LOW);

    // Sensor Pins: Rotary Encoder Input Pins (with internal pull-ups)
    pinMode(PIN_ENCODER_CLK, INPUT);
    pinMode(PIN_ENCODER_DT, INPUT);
    pullUpDnControl(PIN_ENCODER_CLK, PUD_UP);
    pullUpDnControl(PIN_ENCODER_DT, PUD_UP);

    // Launch background thread to track rotary encoder pulses in real-time
    s_stop_encoder.store(false);
    s_encoder_thread = std::thread(encoder_worker_loop);
    s_encoder_thread.detach();

    s_hardware_initialized = true;
    std::cout << "[RPi-1 Hardware] Initialized: Rotary Encoder (GPIO 20/21), Cooling LED (GPIO 12)." << std::endl;
}

int Calibrate_Sensors() {
    init_hardware();
    std::cout << "[RPi-1 Service] Calibrating Thermostat & Cooling Indicator..." << std::endl;
    // Flash Green LED for 250ms to verify actuator wiring
    digitalWrite(PIN_GREEN_LED, HIGH);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    digitalWrite(PIN_GREEN_LED, LOW);
    return 1;
}

int Read_Temperature(char scale) {
    init_hardware();
    int temp_c = s_simulated_temp_c.load();

#if !defined(__linux__)
    // Simulation: add small jitter if on PC
    static int sim_counter = 0;
    sim_counter = (sim_counter + 1) % 5;
    if (sim_counter == 0) temp_c = 22 + (std::rand() % 9);
#endif

    if (scale == 'F' || scale == 'f') {
        int temp_f = static_cast<int>(temp_c * 9.0 / 5.0 + 32.0);
        std::cout << "[RPi-1 Service] Read_Temperature('F') -> " << temp_f << " °F" << std::endl;
        return temp_f;
    }

    std::cout << "[RPi-1 Service] Read_Temperature('C') -> " << temp_c << " °C" << std::endl;
    return temp_c;
}

void Set_Cooling_Indicator(int state) {
    init_hardware();
    s_green_led_state = (state != 0) ? 1 : 0;
    digitalWrite(PIN_GREEN_LED, s_green_led_state ? HIGH : LOW);
    std::cout << "[RPi-1 Service] Set_Cooling_Indicator(" << s_green_led_state << ") -> Green LED is "
              << (s_green_led_state ? "ON" : "OFF") << std::endl;
}

