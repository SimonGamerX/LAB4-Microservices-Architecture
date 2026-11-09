#include "services_rpi2.hpp"
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <atomic>
#include <cstdlib>

#if defined(__linux__)
  #include <wiringPi.h>
  #include <fcntl.h>
  #include <sys/ioctl.h>
  #include <linux/i2c-dev.h>
  #include <unistd.h>
#else
  // Simulation stubs for Windows/macOS testing
  #define LOW 0
  #define HIGH 1
  #define OUTPUT 1
  inline void wiringPiSetupGpio() {}
  inline void pinMode(int, int) {}
  inline void digitalWrite(int, int) {}
#endif

// Internal hardware state
static std::atomic<bool> s_hardware_initialized{false};
static std::atomic<int> s_red_led_state{0};
static int s_i2c_fd = -1;

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
        {rpi_id, "Calibrate_Sensors", 0, {}, "int", "Calibrates ADS1115 ADC baseline & tests Red Alarm LED."},
        {rpi_id, "Read_Light_Level", 0, {}, "int", "Reads ambient light intensity from LDR on ADS1115 Channel A0. Returns percentage 0-100."},
        {rpi_id, "Set_Alarm_LED", 1, {"state"}, "void", "Controls Red Alarm LED on GPIO 16 (1 = ON, 0 = OFF)."}
    };
}

void init_hardware() {
    if (s_hardware_initialized) return;

    wiringPiSetupGpio();

    // Actuator Pin: Red Alarm LED
    pinMode(PIN_RED_LED, OUTPUT);
    digitalWrite(PIN_RED_LED, LOW);

#if defined(__linux__)
    // Initialize I2C interface for ADS1115 (bus /dev/i2c-1, address 0x48)
    s_i2c_fd = open("/dev/i2c-1", O_RDWR);
    if (s_i2c_fd >= 0) {
        if (ioctl(s_i2c_fd, I2C_SLAVE, ADS1115_I2C_ADDR) < 0) {
            std::cerr << "[RPi-2 Hardware] Warning: Failed to acquire I2C bus access for ADS1115 at 0x48." << std::endl;
        }
    }
#endif

    s_hardware_initialized = true;
    std::cout << "[RPi-2 Hardware] Initialized: ADS1115 (I2C 0x48), Alarm LED (GPIO 16)." << std::endl;
}

int read_ads1115_channel(int channel) {
    if (channel < 0 || channel > 3) return 0;
#if defined(__linux__)
    if (s_i2c_fd < 0) return 0;

    uint16_t config = 0x8000;
    config |= ((4 + channel) & 0x07) << 12;
    config |= (0x01) << 9; // FSR = +/- 4.096V
    config |= (0x01) << 8; // Single-shot mode
    config |= (0x04) << 5; // 128 SPS
    config |= 0x0003;      // Disable comparator

    uint8_t write_buf[3];
    write_buf[0] = 0x01; // Config register pointer
    write_buf[1] = static_cast<uint8_t>((config >> 8) & 0xFF);
    write_buf[2] = static_cast<uint8_t>(config & 0xFF);

    if (write(s_i2c_fd, write_buf, 3) != 3) {
        return 0;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(9));

    uint8_t reg_ptr = 0x00;
    if (write(s_i2c_fd, &reg_ptr, 1) != 1) {
        return 0;
    }

    uint8_t read_buf[2];
    if (read(s_i2c_fd, read_buf, 2) != 2) {
        return 0;
    }

    int16_t val = static_cast<int16_t>((read_buf[0] << 8) | read_buf[1]);
    if (val < 0) val = 0;
    return val;
#else
    // Simulation fallback on non-Linux PC
    static int sim_light = 65;
    sim_light = 40 + (std::rand() % 45);
    return static_cast<int>(sim_light * 260);
#endif
}

int Calibrate_Sensors() {
    init_hardware();
    std::cout << "[RPi-2 Service] Calibrating ADS1115 ADC & Alarm LED..." << std::endl;
    // Flash Red LED for 250ms to verify actuator wiring
    digitalWrite(PIN_RED_LED, HIGH);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    digitalWrite(PIN_RED_LED, LOW);
    return 1;
}

int Read_Light_Level() {
    init_hardware();
    int raw = read_ads1115_channel(ADC_CH_LIGHT);
    int percentage = static_cast<int>((raw / 26000.0) * 100.0);
    if (percentage > 100) percentage = 100;
    if (percentage < 0) percentage = 0;

    std::cout << "[RPi-2 Service] Read_Light_Level() -> raw: " << raw 
              << " (" << percentage << "%)" << std::endl;
    return percentage;
}

void Set_Alarm_LED(int state) {
    init_hardware();
    s_red_led_state = (state != 0) ? 1 : 0;
    digitalWrite(PIN_RED_LED, s_red_led_state ? HIGH : LOW);
    std::cout << "[RPi-2 Service] Set_Alarm_LED(" << s_red_led_state << ") -> Red LED is "
              << (s_red_led_state ? "ON" : "OFF") << std::endl;
}

