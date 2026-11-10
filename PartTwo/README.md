# Lab 4 - Part Two: Atlas Thing Middleware & IoT-DDL

## Commands to Run

### 1. Install & Build Atlas Middleware (Raspberry Pi)
```bash
# 1. Clone the Atlas middleware
cd ~/LAB4-Microservices-Architecture/PartTwo
git clone https://github.com/NickLiJunjie/AtlasThingMiddleware_RPI.git

# 2. Enter the inner project directory (Note: Atlas project files are inside Atlas-IoT_thing or Atlas-IoT_Thing)
cd AtlasThingMiddleware_RPI/Atlas-IoT*

# 3. Copy our generated IoT-DDL configuration file into ConfigurationFiles/
cp ../../Atlas_IoTDDL.xml ConfigurationFiles/Atlas_IoTDDL.xml

# 4. Compile directly using cmake pointing to Main/
cmake Main/
make

# 5. Run the Atlas middleware (listens on port 6668)
./Atlas
```
*(If you were in an accidental `build/` directory, simply remove it with `rm -rf build` and `cd` into the inner `Atlas-IoT*` directory as shown above).*

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
