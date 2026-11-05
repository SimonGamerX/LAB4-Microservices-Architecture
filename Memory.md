# Lab 4 Project Memory & Context

**Course**: B8019 - Programming the Internet of Things (Fall 2026)  
**Instructor**: Prof. Sumi Helal  
**Teaching Assistant**: Junjie Li (`lijunjie@ufl.edu`)  
**Group Identifier**: `iotpj`  
**Workspace**: `c:\Users\Withe\OneDrive\Desktop\LAB4`  

---

## 1. Project Objectives
Transition the Virtual Smart Space (VSS) from monolithic embedded code (Labs 1–3 Metronome) to a **Microservices Architecture**:
- **Part One (Custom SOA)**: Expose C/C++ services on the RPi as 4-tuple vectors `(RPi, Service, Inputs, Params)` and orchestrate them from a laptop Edge Mini-IDE using **Order-based** (`RUN A THEN B`) and **Condition-based** (`IF A [op] [val] DO B`) relationships.
- **Part Two (Atlas Middleware)**: Describe the Thing in **IoT-DDL XML**, deploy the official **Atlas Thing Middleware**, and invoke dynamically generated microservices over **TCP port 6668** using a Demo IoT Application.

---

## 2. Hardware Architecture & Wiring

### Devices Needed (Mandatory Checklist)
1. **Raspberry Pi 4B** (+ MicroSD card + USB-C power supply).
2. **Red GPIO Extension Board (T-Cobbler)** + Ribbon Cable (already in breadboard).
3. **MCP3008 ADC** (16-pin DIP chip - required because RPi has no analog GPIOs).
4. **Photoresistor (LDR)** + 10kΩ resistor (Ambient light sensing on ADC CH0).
5. **10kΩ Potentiometer** (Simulated temperature dial on ADC CH1; knob enables instant condition demo).
6. **Red LED** (Alarm indicator on GPIO 16 with 220Ω resistor).
7. **Green LED** (Cooling/Fan indicator on GPIO 13 with 220Ω resistor).
8. **Jumper Wires** (M-F and M-M).
9. **Laptop** (Edge layer, web dashboard).

### Pin Mapping Table
| Device / Function | Breadboard Connection | Raspberry Pi / T-Cobbler Pin |
| :--- | :--- | :--- |
| **MCP3008 VDD & VREF** | 3.3V Power Rail | `3V3` (Pin 1) |
| **MCP3008 AGND & DGND** | Ground Rail | `GND` (Pin 6/9) |
| **MCP3008 CLK** | Pin 13 | `GPIO18` (Pin 12) |
| **MCP3008 DOUT (MISO)**| Pin 12 | `GPIO23` (Pin 16) |
| **MCP3008 DIN (MOSI)** | Pin 11 | `GPIO24` (Pin 18) |
| **MCP3008 CS / SHDN** | Pin 10 | `GPIO25` (Pin 22) |
| **LDR (Light)** | Pin 1 (CH0) | Voltage divider with 10kΩ pull-down to GND |
| **Potentiometer (Temp)**| Pin 2 (CH1) | Center wiper pin (outer pins to 3.3V and GND) |
| **Red LED (Alarm)** | Breadboard row | `GPIO16` (Pin 36) through 220Ω resistor to GND |
| **Green LED (Cooling)**| Breadboard row | `GPIO13` (Pin 33) through 220Ω resistor to GND |

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
│   │   ├── services.cpp        # C++ hardware functions & Software SPI bit-banging
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
│   ├── Atlas_IoTDDL.xml        # Valid IoT-DDL XML describing 4 bit-thing entities
│   ├── demo_app.py             # TCP Port 6668 JSON client (interactive + automated)
│   └── README.md               # Commands-only execution reference
└── Memory.md                   # This project memory and context document
```

---

## 4. Execution Commands Reference

### Part One (Physical & Edge Layers)
- **On Raspberry Pi**:
  ```bash
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

## 5. Next Steps (Pending Physical Hardware)
1. Procure the MCP3008 ADC, LDR, potentiometer, and 2 LEDs.
2. Wire the breadboard using the pin table above.
3. Test Part One using the commands in `PartOne/README.md`.
4. Deploy Atlas Middleware and test Part Two using `PartTwo/README.md`.
5. Prepare submission `.zip` containing `PartOne/` and `PartTwo/`.
