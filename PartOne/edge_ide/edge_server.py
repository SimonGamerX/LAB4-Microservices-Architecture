#!/usr/bin/env python3
"""
Edge Layer Orchestrator & Mini-IDE Server
Course: B8019 - Programming the Internet of Things (Fall 2026)
Lab 4: Building Microservices for your Smart Space - Part One

Functions:
- Discovers and collects service vectors from RPi nodes: (RPi_ID, Service_Name, Num_Inputs, Param_Names)
- Provides Web-based Dashboard and Mini-IDE on http://localhost:5000
- Orchestrates composed IoT applications using:
    1) Single service execution (Direct invocation of a single service without conditions)
    2) Order-based relationships (Service B runs strictly after Service A succeeds)
    3) Condition-based relationships (Service B runs conditionally based on Service A's return value)
- Translates workflows into RPi service API calls, evaluates conditions, and collects results.
"""

import sys
import os
import json
import socket
import urllib.request
import urllib.parse
import urllib.error
from http.server import HTTPServer, SimpleHTTPRequestHandler
import threading
import time

PORT = 5000
STATIC_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "static")

# Known RPi endpoints (default to localhost:8080 for simulation; editable from UI)
rpi_endpoints = [
    {"rpi_id": "RPi-1", "host": "127.0.0.1", "port": 8080, "status": "UNKNOWN"}
]

# Cache of discovered service vectors
discovered_services = []

def query_rpi_services(host, port):
    """Queries an RPi node for its advertised service vectors."""
    url = f"http://{host}:{port}/services"
    req = urllib.request.Request(url, method="GET")
    try:
        with urllib.request.urlopen(req, timeout=3.0) as resp:
            if resp.status == 200:
                data = json.loads(resp.read().decode("utf-8"))
                return data
    except Exception as e:
        pass
    return None

def invoke_rpi_service(host, port, service_name, param_val=""):
    """
    Sends a service API call to the RPi C/C++ backend.
    Sends headers + JSON body together in one single atomic TCP segment to ensure
    the RPi server's single-buffer socket reader parses service_name and params correctly.
    """
    payload_str = json.dumps({"service_name": service_name, "param": str(param_val)})
    body_bytes = payload_str.encode("utf-8")

    # Clean exact path /call without query params so it matches path == "/call" on all RPi builds
    http_packet = (
        f"POST /call HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        f"Content-Type: application/json\r\n"
        f"Content-Length: {len(body_bytes)}\r\n"
        f"Connection: close\r\n\r\n"
    ).encode("utf-8") + body_bytes

    try:
        t0 = time.time()
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(4.0)
        sock.connect((host, int(port)))
        sock.sendall(http_packet)

        response_bytes = b""
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            response_bytes += chunk
        sock.close()

        latency_ms = int((time.time() - t0) * 1000)
        resp_str = response_bytes.decode("utf-8", errors="replace")

        # Split HTTP headers and body
        if "\r\n\r\n" in resp_str:
            header_part, body_part = resp_str.split("\r\n\r\n", 1)
        else:
            header_part = ""
            body_part = resp_str

        # Check for 404 or other HTTP error codes
        first_line = header_part.split("\r\n")[0] if header_part else ""
        if first_line and (" 200" not in first_line):
            return {
                "status": "ERROR",
                "message": f"{first_line}: {body_part.strip()}",
                "latency_ms": latency_ms
            }

        res_data = json.loads(body_part)
        res_data["latency_ms"] = latency_ms
        return res_data
    except Exception as e:
        return {"status": "ERROR", "message": str(e), "latency_ms": 0}

def refresh_discovery():
    """Polls all configured RPis and updates the catalog of service vectors."""
    global discovered_services
    catalog = []
    for node in rpi_endpoints:
        res = query_rpi_services(node["host"], node["port"])
        if res:
            node["status"] = "ONLINE"
            node["rpi_id"] = res.get("rpi_id", node["rpi_id"])
            for s in res.get("services", []):
                s["host"] = node["host"]
                s["port"] = node["port"]
                catalog.append(s)
        else:
            node["status"] = "OFFLINE"
    discovered_services = catalog
    return discovered_services

def execute_composed_program(composition):
    """
    Translates and executes a composed IoT application on the Edge.
    Supported step relationships:
      - 'single': Direct execution of a single service without condition.
      - 'order': Run Service A, wait for success, then run Service B.
      - 'condition': Run Service A, test condition (>, <, ==, !=, >=, <=) on output, then conditionally run Service B.
    """
    trace = []
    overall_success = True

    for idx, rule in enumerate(composition.get("rules", [])):
        rule_type = rule.get("type", "order")
        rule_desc = rule.get("description", f"Rule #{idx+1}")

        step_log = {
            "step_index": idx + 1,
            "type": rule_type,
            "description": rule_desc,
            "actions": [],
            "status": "PENDING"
        }

        if rule_type in ("single", "single_service"):
            # Single service execution without condition
            srv = rule.get("service") or rule.get("service_a")
            if not srv:
                step_log["status"] = "ERROR"
                step_log["error"] = "No service specified for single service rule."
                trace.append(step_log)
                overall_success = False
                continue

            res = invoke_rpi_service(srv["host"], srv["port"], srv["name"], srv.get("param", ""))
            step_log["actions"].append({
                "service": srv["name"],
                "target": f"{srv['host']}:{srv['port']}",
                "input": srv.get("param", ""),
                "result": res
            })
            step_log["status"] = "SUCCESS" if res.get("status") == "SUCCESS" else "FAILED"
            if res.get("status") != "SUCCESS":
                step_log["error"] = res.get("message", "Service execution failed.")
                overall_success = False

        elif rule_type == "order":
            # Order-based relationship: Service A then Service B
            srv_a = rule.get("service_a") # {host, port, name, param}
            srv_b = rule.get("service_b")

            # Execute Service A
            res_a = invoke_rpi_service(srv_a["host"], srv_a["port"], srv_a["name"], srv_a.get("param", ""))
            step_log["actions"].append({
                "service": srv_a["name"],
                "target": f"{srv_a['host']}:{srv_a['port']}",
                "input": srv_a.get("param", ""),
                "result": res_a
            })

            if res_a.get("status") == "SUCCESS":
                # Service A succeeded -> Execute Service B
                time.sleep(0.1) # brief inter-call pacing
                res_b = invoke_rpi_service(srv_b["host"], srv_b["port"], srv_b["name"], srv_b.get("param", ""))
                step_log["actions"].append({
                    "service": srv_b["name"],
                    "target": f"{srv_b['host']}:{srv_b['port']}",
                    "input": srv_b.get("param", ""),
                    "result": res_b
                })
                step_log["status"] = "SUCCESS" if res_b.get("status") == "SUCCESS" else "FAILED"
                if res_b.get("status") != "SUCCESS":
                    step_log["error"] = res_b.get("message", "Service B failed.")
                    overall_success = False
            else:
                step_log["status"] = "FAILED_PREREQUISITE"
                step_log["error"] = res_a.get("message") or f"Service {srv_a['name']} failed; Service {srv_b['name']} aborted per order constraint."
                overall_success = False

        elif rule_type == "condition":
            # Condition-based relationship: If Service A result [operator] [threshold], run Service B
            srv_a = rule.get("service_a")
            srv_b = rule.get("service_b")
            operator = rule.get("operator", "==")
            threshold = float(rule.get("threshold", 0))

            res_a = invoke_rpi_service(srv_a["host"], srv_a["port"], srv_a["name"], srv_a.get("param", ""))
            step_log["actions"].append({
                "service": srv_a["name"],
                "target": f"{srv_a['host']}:{srv_a['port']}",
                "input": srv_a.get("param", ""),
                "result": res_a
            })

            if res_a.get("status") == "SUCCESS":
                ret_val = res_a.get("return_value")
                try:
                    num_val = float(ret_val)
                    condition_met = False
                    if operator == ">": condition_met = (num_val > threshold)
                    elif operator == "<": condition_met = (num_val < threshold)
                    elif operator == ">=": condition_met = (num_val >= threshold)
                    elif operator == "<=": condition_met = (num_val <= threshold)
                    elif operator == "==": condition_met = (num_val == threshold)
                    elif operator == "!=": condition_met = (num_val != threshold)

                    step_log["condition_eval"] = {
                        "value": num_val,
                        "operator": operator,
                        "threshold": threshold,
                        "met": condition_met
                    }

                    if condition_met:
                        time.sleep(0.1)
                        res_b = invoke_rpi_service(srv_b["host"], srv_b["port"], srv_b["name"], srv_b.get("param", ""))
                        step_log["actions"].append({
                            "service": srv_b["name"],
                            "target": f"{srv_b['host']}:{srv_b['port']}",
                            "input": srv_b.get("param", ""),
                            "result": res_b
                        })
                        step_log["status"] = "CONDITION_MET_AND_EXECUTED"
                        if res_b.get("status") != "SUCCESS":
                            step_log["error"] = res_b.get("message", "Service B failed.")
                    else:
                        step_log["status"] = "CONDITION_NOT_MET_SKIPPED"
                except (ValueError, TypeError) as ex:
                    step_log["status"] = "CONDITION_EVAL_ERROR"
                    step_log["error"] = f"Return value {ret_val} could not be compared with threshold."
            else:
                step_log["status"] = "FAILED_EVALUATION"
                step_log["error"] = res_a.get("message") or f"Evaluation of service {srv_a['name']} failed."
                overall_success = False

        trace.append(step_log)

    return {"overall_success": overall_success, "trace": trace}

class EdgeRequestHandler(SimpleHTTPRequestHandler):
    """HTTP Request Handler for Edge Dashboard & Mini-IDE."""

    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=STATIC_DIR, **kwargs)

    def do_GET(self):
        if self.path == "/api/discovery":
            refresh_discovery()
            self.send_json_response(200, {
                "nodes": rpi_endpoints,
                "services": discovered_services
            })
        else:
            super().do_GET()

    def do_POST(self):
        content_len = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(content_len).decode("utf-8")

        if self.path == "/api/config_nodes":
            # Add or update RPi node IP/port
            try:
                data = json.loads(body)
                global rpi_endpoints
                rpi_endpoints = data.get("nodes", rpi_endpoints)
                refresh_discovery()
                self.send_json_response(200, {"status": "UPDATED", "nodes": rpi_endpoints})
            except Exception as e:
                self.send_json_response(400, {"status": "ERROR", "message": str(e)})

        elif self.path == "/api/execute":
            # Execute composed IoT workflow
            try:
                comp = json.loads(body)
                results = execute_composed_program(comp)
                self.send_json_response(200, results)
            except Exception as e:
                self.send_json_response(500, {"status": "ERROR", "message": str(e)})

        elif self.path == "/api/direct_call":
            # Direct manual call to single service for testing
            try:
                req = json.loads(body)
                res = invoke_rpi_service(req["host"], req["port"], req["service_name"], req.get("param", ""))
                self.send_json_response(200, res)
            except Exception as e:
                self.send_json_response(500, {"status": "ERROR", "message": str(e)})
        else:
            self.send_json_response(404, {"error": "Not Found"})

    def send_json_response(self, status_code, data):
        payload = json.dumps(data).encode("utf-8")
        self.send_response(status_code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Content-Length", str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)

def run_server():
    server_addr = ("", PORT)
    httpd = HTTPServer(server_addr, EdgeRequestHandler)
    print("=" * 60)
    print("  EDGE LAYER - Mini-IDE & Orchestrator Dashboard")
    print(f"  Open your browser at: http://localhost:{PORT}")
    print("=" * 60)
    refresh_discovery()
    httpd.serve_forever()

if __name__ == "__main__":
    run_server()
