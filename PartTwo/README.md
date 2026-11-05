# Lab 4 - Part Two: Atlas Thing Middleware & IoT-DDL

## Commands to Run

### 1. Install & Build Atlas Middleware (Raspberry Pi)
```bash
# Clone the Atlas middleware
git clone https://github.com/NickLiJunjie/AtlasThingMiddleware_RPI.git
cd AtlasThingMiddleware_RPI

# Copy our generated IoT-DDL configuration file
cp ../PartTwo/Atlas_IoTDDL.xml ConfigurationFiles/Atlas_IoTDDL.xml

# Compile
mkdir -p build && cd build
cmake ..
make

# Run the middleware (broadcasts tweets & listens on port 6668)
./Atlas
```

### 2. Run Demo IoT Application (Laptop)
```bash
cd PartTwo
python demo_app.py <RPi_IP_Address>
```
*(Or run `python demo_app.py <RPi_IP_Address> --auto` for non-interactive automated test).*

---

## Group Effort Distribution
- Member 1: IoT-DDL XML specification & ADC configuration (33.3%)
- Member 2: Atlas Thing Middleware setup & C++ service integration (33.3%)
- Member 3: Demo IoT Application (TCP JSON client on port 6668) (33.3%)
