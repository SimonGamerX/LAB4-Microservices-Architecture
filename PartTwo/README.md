# Lab 4 - Part Two: Atlas Thing Middleware & IoT-DDL

## Commands to Run

### 1. Prerequisites: Install GCC 12, CppMicroServices & WiringPi (Raspberry Pi)
The Atlas middleware codebase is older and incompatible with GCC 14. GCC 12 is recommended:

```bash
# Install GCC 12 / G++ 12
sudo apt update
sudo apt install -y gcc-12 g++-12
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 100 --slave /usr/bin/g++ g++ /usr/bin/g++-12
sudo update-alternatives --set gcc /usr/bin/gcc-12
export CC=gcc-12
export CXX=g++-12

cd ~/LAB4-Microservices-Architecture/PartTwo/AtlasThingMiddleware_RPI/lib

# 1. Install CppMicroServices (Fixes "cppmicroservices/ServiceInterface.h: No such file or directory")
unzip -q CppMicroServices-development.zip 2>/dev/null || true
cd CppMicroServices-development
mkdir -p build && cd build
cmake -DCMAKE_C_COMPILER=gcc-12 -DCMAKE_CXX_COMPILER=g++-12 ..
make -j$(nproc)
sudo make install

# Ensure headers are in standard /usr/local/include/cppmicroservices
sudo cp -r /usr/local/include/cppmicroservices*/cppmicroservices /usr/local/include/ 2>/dev/null || true
sudo ldconfig

# 2. Install WiringPi (fixes multiple definition of comDat and missing Hardware line)
cd ~/LAB4-Microservices-Architecture/PartTwo/AtlasThingMiddleware_RPI/lib
unzip -q WiringPi-master.zip 2>/dev/null || true
cd WiringPi-master
find . -name "*.o" -delete
find . -name "*.so*" -delete
find . -name "drcNetCmd.h" -exec sed -i -E 's/\}[[:space:]]*comDat[[:space:]]*;/}; extern struct drcNetComStruct comDat;/' {} +
sed -i '1i struct drcNetComStruct comDat;' wiringPi/drcNet.c
find . -name "Makefile" -exec sed -i 's/CFLAGS =/CFLAGS = -fcommon /g' {} +

# Patch wiringPi.c to bypass missing "Hardware" line in modern kernels
python3 -c "
import re
path = 'wiringPi/wiringPi.c'
with open(path, 'r') as f:
    text = f.read()
text = re.sub(r'(int\s+piGpioLayout\s*\([^\)]*\)\s*\{)', r'\1\n  return 2;', text, count=1)
text = re.sub(r'(int\s+piBoardRev\s*\([^\)]*\)\s*\{)', r'\1\n  return 2;', text, count=1)
with open(path, 'w') as f:
    f.write(text)
print('wiringPi.c patched!')
"
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
