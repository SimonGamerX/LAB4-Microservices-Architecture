# Lab 4 - Part One

## Commands to Run

### 1. Raspberry Pi (Hardware / C++ Services)
Compile both RPi service servers:
```bash
cd rpi_server
make
```

Run both servers (in separate terminals or as background processes):
```bash
# Terminal 1 - RPi-1 (Thermostat Dial on GPIO 20/21 + Cooling Fan LED on GPIO 12):
./rpi1_service_server RPi-1 8080

# Terminal 2 - RPi-2 (ADS1115 Light Sensor on I2C A0 + Alarm LED on GPIO 16):
./rpi2_service_server RPi-2 8081
```
*(Alternative: you can also run the unified single-process server: `./rpi_service_server RPi-1 8080`)*

### 2. Laptop (Edge Dashboard & Mini-IDE)
```bash
cd edge_ide
python edge_server.py
```
Open in browser: `http://localhost:5000`

---

## Group Effort Distribution
- Nicola Ceccarini: Hardware setup & C++ services (33.4%)
- Riccardo Carini: C++ socket server & service vectors (33.3%)
- Simone Pipola: Edge Mini-IDE & orchestration (33.3%)
