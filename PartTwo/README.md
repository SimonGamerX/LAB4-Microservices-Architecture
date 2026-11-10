# Lab 4 - Part Two: Atlas Thing Middleware & IoT-DDL

## Commands to Run

### 1. Prerequisites: Install CppMicroServices & WiringPi (Raspberry Pi)
The Atlas middleware requires `CppMicroServices` and `WiringPi` located in the `lib/` folder:

```bash
cd ~/LAB4-Microservices-Architecture/PartTwo/AtlasThingMiddleware_RPI/lib

# 1. Install CppMicroServices (Fixes "cppmicroservices/ServiceInterface.h: No such file or directory")
unzip -q CppMicroServices-development.zip 2>/dev/null || true
cd CppMicroServices-development
mkdir -p build && cd build
cmake ..
make -j$(nproc)
sudo make install

# Ensure headers are in standard /usr/local/include/cppmicroservices
sudo cp -r /usr/local/include/cppmicroservices*/cppmicroservices /usr/local/include/ 2>/dev/null || true
sudo ldconfig

# 2. Install WiringPi (if not already installed)
cd ~/LAB4-Microservices-Architecture/PartTwo/AtlasThingMiddleware_RPI/lib
unzip -q WiringPi-master.zip 2>/dev/null || true
cd WiringPi-master
./build
```

### 2. Build Atlas Middleware (Raspberry Pi)
```bash
cd ~/LAB4-Microservices-Architecture/PartTwo/AtlasThingMiddleware_RPI

# Copy our generated IoT-DDL configuration file
cp ../Atlas_IoTDDL.xml ConfigurationFiles/Atlas_IoTDDL.xml

# Compile (or resume compilation)
make

# Run the Atlas middleware (listens on port 6668)
./Atlas
```

### 3. Run Demo IoT Application (Laptop)
```bash
cd PartTwo
python demo_app.py <RPi_IP_Address>
```
*(Or run `python demo_app.py <RPi_IP_Address> --auto` for non-interactive automated test).*

---

## Group Effort Distribution
- Simone Pipola: IoT-DDL XML specification & ADC configuration (33.3%)
- Riccardo Carini: Atlas Thing Middleware setup & C++ service integration (33.3%)
- Nicola Ceccarini: Demo IoT Application (TCP JSON client on port 6668) (33.4%)
