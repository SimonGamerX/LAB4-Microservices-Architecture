# Lab 4 - Part One

## Commands to Run

### 1. Raspberry Pi (Hardware / C++ Services)
Compile the standalone service server:
```bash
cd rpi_server
make
```

Run the server (hosts all hardware services partitioned logically as RPi-1 and RPi-2):
```bash
./rpi_service_server RPi 8080
```
- **RPi-1 Services**: `Read_Temperature` (Rotary Encoder on GPIO 20/21) & `Set_Cooling_Indicator` (Green LED on GPIO 12).
- **RPi-2 Services**: `Read_Light_Level` (ADS1115 ADC on I2C A0) & `Set_Alarm_LED` (Red LED on GPIO 16).

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
