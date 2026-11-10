# Lab 4 - Microservices Architecture for Smart Space

This project transitions the Virtual Smart Space (VSS) into a **Microservices-oriented Architecture (SOA)**, organized into two complementary implementations:
- **Part One (Custom SOA)**: Custom C++ REST service server on Raspberry Pi orchestrated by a web-based Edge Mini-IDE.
- **Part Two (Atlas Thing Middleware)**: Standardized Thing described via **IoT-DDL XML**, deployed on the official **Atlas Thing Middleware** and accessed over TCP port 6668.

---

## Part One: Custom SOA & Edge Mini-IDE

### 1. Raspberry Pi (Hardware / C++ Services)
Compile the standalone service server:
```bash
cd PartOne/rpi_server
make
```

Run the server (hosts all hardware services partitioned logically as RPi-1 and RPi-2):
```bash
./rpi_service_server RPi 8080
```

- **RPi-1 Services**: 
  - `Read_Temperature`: Rotary Encoder on GPIO 20/21 (CLK/DT).
  - `Set_Cooling_Indicator`: Green Cooling LED on GPIO 12.
  - `Calibrate_Sensors`: Calibrates Rotary Encoder baseline & tests Green Cooling LED.
- **RPi-2 Services**: 
  - `Read_Light_Level`: Ambient light intensity from TEMT6000 / LDR on ADS1115 ADC (I2C Channel A0).
  - `Set_Alarm_LED`: Red Alarm LED on GPIO 16.
  - `Calibrate_Sensors`: Calibrates ADS1115 ADC baseline & tests Red Alarm LED.

### 2. Laptop (Edge Dashboard & Mini-IDE)
Start the edge orchestrator server:
```bash
cd PartOne/edge_ide
python edge_server.py
```
Open in browser: `http://localhost:5000`

The Mini-IDE allows composing IoT applications using:
- **Single Service Commands**: Direct execution (`RUN <Service> [arg]`).
- **Order-Based Rules**: Sequential chaining (`RUN <Service_A> [argA] THEN <Service_B> [argB]`).
- **Condition-Based Rules**: Dynamic threshold logic (`IF <Service_A> [op] [val] DO <Service_B> [argB]`).

---

## Part Two: Atlas Thing Middleware & IoT-DDL

### 1. Prerequisites (Raspberry Pi)
Ensure `CppMicroServices` is installed on the system:
```bash
cd ~/LAB4-Microservices-Architecture/PartTwo/AtlasThingMiddleware_RPI/lib/CppMicroServices-development
mkdir -p build && cd build
cmake ..
make -j$(nproc)
sudo make install
sudo ldconfig
```

### 2. Build & Run Atlas Middleware (Raspberry Pi)
Copy the IoT-DDL specification and start the middleware:
```bash
cd ~/LAB4-Microservices-Architecture/PartTwo/AtlasThingMiddleware_RPI

# Copy the generated IoT-DDL configuration file
cp ../Atlas_IoTDDL.xml ConfigurationFiles/Atlas_IoTDDL.xml

# Compile Atlas middleware
make

# Run the Atlas middleware (listens on TCP port 6668)
./Atlas
```

### 3. Run Demo IoT Application (Laptop)
Execute the client application communicating with Atlas over TCP socket:
```bash
cd PartTwo
python demo_app.py <RPi_IP_Address>
```
*(Or run `python demo_app.py <RPi_IP_Address> --auto` for non-interactive automated test).*

---

## Group Effort Distribution

### Part One
- Nicola Ceccarini: Hardware setup & C++ services (33.4%)
- Riccardo Carini: C++ socket server & service vectors (33.3%)
- Simone Pipola: Edge Mini-IDE & orchestration (33.3%)

### Part Two
- Simone Pipola: IoT-DDL XML specification & ADC configuration (33.3%)
- Riccardo Carini: Atlas Thing Middleware setup & C++ service integration (33.3%)
- Nicola Ceccarini: Demo IoT Application (TCP JSON client on port 6668) (33.4%)
