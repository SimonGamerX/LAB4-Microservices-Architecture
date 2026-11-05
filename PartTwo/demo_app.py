#!/usr/bin/env python3
"""
Demo IoT Application for Lab 4 - Part Two
Course: B8019 - Programming the Internet of Things
Target: Atlas Thing Middleware (Unicast TCP JSON on Port 6668)

Format of Service Call Tweet:
{
  "Tweet Type": "Service call",
  "Thing ID": "MySmartThing01",
  "Space ID": "MySmartSpace",
  "Service Name": "<Service_Name>",
  "Service Inputs": "(<args>)"
}
"""

import socket
import json
import sys
import time

DEFAULT_PORT = 6668
THING_ID = "MySmartThing01"
SPACE_ID = "MySmartSpace"

def send_service_call(rpi_ip, service_name, inputs="()", port=DEFAULT_PORT):
    """
    Sends a unicast JSON service call to the Atlas Thing Middleware over TCP port 6668.
    """
    payload = {
        "Tweet Type": "Service call",
        "Thing ID": THING_ID,
        "Space ID": SPACE_ID,
        "Service Name": service_name,
        "Service Inputs": str(inputs)
    }

    message = json.dumps(payload)
    print(f"\n[>] Sending Service Call to {rpi_ip}:{port} ...")
    print(f"    Payload: {message}")

    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5.0)
        sock.connect((rpi_ip, port))
        sock.sendall(message.encode("utf-8"))

        raw_response = sock.recv(4096).decode("utf-8")
        sock.close()

        print(f"[<] Response Received:")
        print(f"    {raw_response}")

        try:
            return json.loads(raw_response)
        except json.JSONDecodeError:
            return {"raw": raw_response}

    except Exception as e:
        print(f"[!] Connection failed: {e}")
        return None

def run_automated_demo(rpi_ip):
    """
    Executes a complete demonstration scenario calling both sensor services
    and conditional actuator services.
    """
    print("\n" + "=" * 55)
    print("  RUNNING AUTOMATED DEMO OF ATLAS MICROSERVICES")
    print("=" * 55)

    # 1. Read Light Level from ADC CH0
    print("\n--- 1. Invoking Read_Light_Level() ---")
    res_light = send_service_call(rpi_ip, "Read_Light_Level", "()")
    time.sleep(1)

    # 2. Read Temperature from ADC CH1
    print("\n--- 2. Invoking Read_Temperature() ---")
    res_temp = send_service_call(rpi_ip, "Read_Temperature", "()")
    time.sleep(1)

    # 3. Test Alarm Actuator (Red LED on GPIO 16)
    print("\n--- 3. Invoking Set_Alarm_LED(1) [Turn ON] ---")
    send_service_call(rpi_ip, "Set_Alarm_LED", "(1)")
    time.sleep(2)

    print("\n--- 4. Invoking Set_Alarm_LED(0) [Turn OFF] ---")
    send_service_call(rpi_ip, "Set_Alarm_LED", "(0)")
    time.sleep(1)

    # 4. Test Cooling Actuator (Green LED on GPIO 13)
    print("\n--- 5. Invoking Set_Cooling_Indicator(1) [Turn ON] ---")
    send_service_call(rpi_ip, "Set_Cooling_Indicator", "(1)")
    time.sleep(2)

    print("\n--- 6. Invoking Set_Cooling_Indicator(0) [Turn OFF] ---")
    send_service_call(rpi_ip, "Set_Cooling_Indicator", "(0)")

    print("\n[+] Automated demo sequence finished.\n")

def interactive_menu(rpi_ip):
    while True:
        print("\n" + "-" * 40)
        print("  Atlas Microservice Invoker")
        print(f"  Target RPi: {rpi_ip}:{DEFAULT_PORT}")
        print("-" * 40)
        print("1. Call Read_Light_Level()")
        print("2. Call Read_Temperature()")
        print("3. Call Set_Alarm_LED(1) [Red ON]")
        print("4. Call Set_Alarm_LED(0) [Red OFF]")
        print("5. Call Set_Cooling_Indicator(1) [Green ON]")
        print("6. Call Set_Cooling_Indicator(0) [Green OFF]")
        print("7. Run Full Automated Demo")
        print("0. Exit")
        
        choice = input("\nEnter choice (0-7): ").strip()
        if choice == "1":
            send_service_call(rpi_ip, "Read_Light_Level", "()")
        elif choice == "2":
            send_service_call(rpi_ip, "Read_Temperature", "()")
        elif choice == "3":
            send_service_call(rpi_ip, "Set_Alarm_LED", "(1)")
        elif choice == "4":
            send_service_call(rpi_ip, "Set_Alarm_LED", "(0)")
        elif choice == "5":
            send_service_call(rpi_ip, "Set_Cooling_Indicator", "(1)")
        elif choice == "6":
            send_service_call(rpi_ip, "Set_Cooling_Indicator", "(0)")
        elif choice == "7":
            run_automated_demo(rpi_ip)
        elif choice == "0":
            break
        else:
            print("Invalid choice.")

if __name__ == "__main__":
    target_ip = sys.argv[1] if len(sys.argv) > 1 else input("Enter Raspberry Pi IP address: ").strip()
    if not target_ip:
        target_ip = "127.0.0.1"

    if "--auto" in sys.argv:
        run_automated_demo(target_ip)
    else:
        interactive_menu(target_ip)
