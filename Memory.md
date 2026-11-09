# Lab 4 Project Memory & Context.

**Course**: B8019 - Programming the Internet of Things (Fall 2026)  
**Instructor**: Prof. Sumi Helal  
**Teaching Assistant**: Junjie Li (`lijunjie@ufl.edu`)  
**Group Identifier**: `iotpj`  
**Workspace**: `c:\Users\rosae\Desktop\LAB4-Microservices-Architecture`  

---

## 1. Project Objectives
Transition the Virtual Smart Space (VSS) from monolithic embedded code (Labs 1–3 Metronome) to a **Microservices Architecture**:
- **Part One (Custom SOA)**: Expose C/C++ services on the RPi as 4-tuple vectors `(RPi, Service, Inputs, Params)` and orchestrate them from a laptop Edge Mini-IDE using **Order-based** (`RUN A THEN B`) and **Condition-based** (`IF A [op] [val] DO B`) relationships.
- **Part Two (Atlas Middleware)**: Describe the Thing in **IoT-DDL XML**, deploy the official **Atlas Thing Middleware**, and invoke dynamically generated microservices over **TCP port 6668** using a Demo IoT Application.

---

## 2. Hardware Architecture & Minimal Wiring (ADS1115 + Rotary Encoder)

### Devices Checklist (Single 3.3V Rail Configuration)
1. **Raspberry Pi 4B** (+ MicroSD card + USB-C power supply).
2. **Red GPIO Extension Board (T-Cobbler)** + Ribbon Cable (in breadboard).
3. **ADS1115 ADC Breakout Board** (16-bit 4-channel I2C ADC module; address 0x48 on SDA=GPIO2, SCL=GPIO3).
4. **Photoresistor (LDR)** + 10kΩ resistor (Ambient light sensing on ADS1115 Channel A0 — 0 jumper wires).
5. **KY-040 5-Pin Rotary Encoder** (Simulated room thermostat dial: CLK on GPIO 20, DT on GPIO 21).
6. **Red LED** (Safety / Alarm indicator on GPIO 16 with 220Ω resistor).
7. **Green LED** (Cooling / Fan indicator on GPIO 12 with 220Ω resistor).
8. **Minimal Jumper Wires** (12 wires total across the entire breadboard).
9. **Laptop** (Edge layer, web dashboard).

### Pin Mapping Table
| Device / Function | Component Pin | Raspberry Pi / T-Cobbler Pin | Breadboard Connection |
| :--- | :--- | :--- | :--- |
| **Main 3.3V Power** | `3V3` | `3V3` (Pin 1) | Breadboard Red `(+)` Rail |
| **Main Ground** | `GND` | `GND` (Pin 6/9) | Breadboard Blue `(-)` Rail |
| **ADS1115 VDD** | `VDD` | — | Breadboard Red `(+)` Rail |
| **ADS1115 GND** | `GND` | — | Breadboard Blue `(-)` Rail |
| **ADS1115 SCL** | `SCL` | `SCL` / `GPIO3` (Pin 5) | Breadboard Row 22 |
| **ADS1115 SDA** | `SDA` | `SDA` / `GPIO2` (Pin 3) | Breadboard Row 23 |
| **ADS1115 ADDR** | `ADDR` | — | *Unconnected* (onboard pull-down sets 0x48) |
| **ADS1115 ALRT** | `ALRT` | — | *Unconnected* |
| **LDR (Light)** | Leg 1 to 2 | Channel A0 | Leg 1 in `(+)` Rail, Leg 2 in Row 26 (A0) |
| **10kΩ Pull-down** | Lead 1 to 2 | Channel A0 | Lead 1 in Row 26 (A0), Lead 2 in `(-)` Rail |
| **Rotary Encoder `+`**| `+` | — | Breadboard Red `(+)` Rail |
| **Rotary Encoder `GND`**| `GND` | — | Breadboard Blue `(-)` Rail |
| **Rotary Encoder `CLK`**| `CLK` | `GPIO20` (Pin 38) | Breadboard / direct wire |
| **Rotary Encoder `DT`** | `DT` | `GPIO21` (Pin 40) | Breadboard / direct wire |
| **Rotary Encoder `SW`** | `SW` | — | *Unconnected* |
| **Red LED (Alarm)** | Anode (+) | `GPIO16` (Pin 36) | Long leg in Row 11; short leg via 220Ω to `(-)` Rail |
| **Green LED (Cooling)**| Anode (+) | `GPIO12` (Pin 32) | Long leg in Row 14; short leg via 220Ω to `(-)` Rail |

---

## 3. Workspace Structure & Status

```
LAB4/
├── LAB4 Guides/
│   ├── Lab 4 – Build Microservices for your Smart Space-1.pdf
│   └── Lab4_Microservices_Smart_Space.pptx
├── PartOne/
│   ├── rpi_server/             # C/C++ services & REST server (Port 8080)
│   │   ├── main.cpp            # Server entry point, dispatching, socket listener
│   │   ├── services.cpp        # C++ hardware functions, ADS1115 I2C & Rotary Encoder thread
│   │   ├── services.hpp        # Service vector definitions & headers
│   │   ├── Makefile            # Standalone build for Raspberry Pi (make)
│   │   └── CMakeLists.txt      # CMake build configuration
│   ├── edge_ide/               # Minimalist Edge layer (Port 5000)
│   │   ├── edge_server.py      # Python orchestrator & service vector collector
│   │   └── static/             # Clean 3-section dashboard (Vectors, Composer, Output)
│   │       ├── index.html
│   │       ├── style.css
│   │       └── app.js
│   └── README.md               # Commands-only execution reference
├── PartTwo/
│   ├── Atlas_IoTDDL.xml        # Valid IoT-DDL XML describing 4 entities (ADS1115 & Rotary Encoder)
│   ├── demo_app.py             # TCP Port 6668 JSON client (interactive + automated)
│   └── README.md               # Commands-only execution reference
└── Memory.md                   # This project memory and context document
```

---

## 4. Execution Commands Reference

### Part One (Physical & Edge Layers)
- **On Raspberry Pi**:
  ```bash
  # Ensure I2C is enabled:
  sudo raspi-config
  # Interface Options -> I2C -> Enable -> Yes
  
  cd PartOne/rpi_server
  make
  ./rpi_service_server RPi-1 8080
  ```
- **On Laptop**:
  ```bash
  cd PartOne/edge_ide
  python edge_server.py
  # Open browser: http://localhost:5000
  ```

### Part Two (Atlas Middleware & Demo App)
- **On Raspberry Pi**:
  ```bash
  git clone https://github.com/NickLiJunjie/AtlasThingMiddleware_RPI.git
  cd AtlasThingMiddleware_RPI
  cp ../PartTwo/Atlas_IoTDDL.xml ConfigurationFiles/Atlas_IoTDDL.xml
  mkdir -p build && cd build
  cmake ..
  make
  ./Atlas
  ```
- **On Laptop**:
  ```bash
  cd PartTwo
  python demo_app.py <RPi_IP_Address>
  # For non-interactive auto test:
  python demo_app.py <RPi_IP_Address> --auto
  ```

---

## 5. Summary of Actuators & Condition Scenarios
- **Green LED (Cooling / Fan)**: Controlled via `Set_Cooling_Indicator(state)` on GPIO 12.
  *Demo rule*: `IF Read_Temperature > 28 DO Set_Cooling_Indicator 1` (Turning the rotary encoder past 28°C turns on the cooling indicator).
- **Red LED (Alarm / Danger)**: Controlled via `Set_Alarm_LED(state)` on GPIO 16.
  *Demo rule*: `IF Read_Temperature > 35 DO Set_Alarm_LED 1` (Overheat alarm) or `IF Read_Light_Level < 15 DO Set_Alarm_LED 1` (Intrusion / Darkness alarm).
- **Calibration**: `Calibrate_Sensors()` flashes both LEDs for 250ms to signal hardware operational status.
