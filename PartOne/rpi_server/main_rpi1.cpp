#include "services_rpi1.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstring>
#include <thread>
#include <mutex>
#include <algorithm>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef int socklen_t;
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #define closesocket close
  #define SOCKET int
  #define INVALID_SOCKET -1
  #define SOCKET_ERROR -1
#endif

static const int DEFAULT_PORT = 8080;
static std::string g_rpi_id = "RPi-1";

std::string extract_json_field(const std::string& json, const std::string& field) {
    std::string needle = "\"" + field + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos);
    if (pos == std::string::npos) return "";
    pos++;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\"')) pos++;
    size_t end = pos;
    while (end < json.size() && json[end] != '\"' && json[end] != ',' && json[end] != '}' && json[end] != '\r' && json[end] != '\n') end++;
    return json.substr(pos, end - pos);
}

std::string execute_service_call(const std::string& service_name, const std::string& arg_str) {
    std::ostringstream response;

    if (service_name == "Calibrate_Sensors") {
        int res = Calibrate_Sensors();
        response << "{\"status\":\"SUCCESS\",\"service\":\"" << service_name 
                 << "\",\"rpi_id\":\"" << g_rpi_id << "\",\"return_value\":" << res << "}";
    }
    else if (service_name == "Read_Temperature") {
        char scale = 'C';
        if (!arg_str.empty() && (arg_str[0] == 'F' || arg_str[0] == 'f')) {
            scale = 'F';
        }
        int res = Read_Temperature(scale);
        response << "{\"status\":\"SUCCESS\",\"service\":\"" << service_name 
                 << "\",\"rpi_id\":\"" << g_rpi_id
                 << "\",\"scale\":\"" << scale << "\",\"return_value\":" << res << "}";
    }
    else if (service_name == "Set_Cooling_Indicator") {
        int state = 0;
        if (!arg_str.empty()) {
            state = std::atoi(arg_str.c_str());
        }
        Set_Cooling_Indicator(state);
        response << "{\"status\":\"SUCCESS\",\"service\":\"" << service_name 
                 << "\",\"rpi_id\":\"" << g_rpi_id
                 << "\",\"state\":" << state << ",\"return_value\":\"void\"}";
    }
    else {
        response << "{\"status\":\"ERROR\",\"message\":\"Unknown service: " << service_name << " on " << g_rpi_id << "\"}";
    }

    return response.str();
}

void handle_client(SOCKET client_socket) {
    std::string request;
    char buffer[4096];
    int bytes_received = 0;

    while (true) {
        bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) break;
        buffer[bytes_received] = '\0';
        request.append(buffer, bytes_received);

        size_t header_end = request.find("\r\n\r\n");
        if (header_end != std::string::npos) {
            size_t cl_pos = request.find("Content-Length:");
            if (cl_pos == std::string::npos) cl_pos = request.find("content-length:");
            if (cl_pos != std::string::npos) {
                size_t val_start = cl_pos + 15;
                while (val_start < request.size() && request[val_start] == ' ') val_start++;
                size_t val_end = request.find("\r\n", val_start);
                int content_length = std::atoi(request.substr(val_start, val_end - val_start).c_str());
                size_t body_len = request.size() - (header_end + 4);
                if (static_cast<int>(body_len) >= content_length) break;
            } else {
                break;
            }
        }
    }

    std::string method, path;
    std::istringstream req_stream(request);
    req_stream >> method >> path;

    std::string response_body;
    std::string http_status = "200 OK";

    if (method == "GET" && path == "/services") {
        auto services = get_available_services(g_rpi_id);
        std::ostringstream json_out;
        json_out << "{\"rpi_id\":\"" << g_rpi_id << "\",\"services\":[";
        for (size_t i = 0; i < services.size(); ++i) {
            json_out << services[i].to_json();
            if (i + 1 < services.size()) json_out << ",";
        }
        json_out << "]}";
        response_body = json_out.str();
    }
    else if (method == "POST" && path == "/call") {
        size_t body_pos = request.find("\r\n\r\n");
        std::string body = (body_pos != std::string::npos) ? request.substr(body_pos + 4) : "";
        std::string service_name = extract_json_field(body, "service_name");
        std::string param = extract_json_field(body, "param");

        if (service_name.empty()) {
            http_status = "400 Bad Request";
            response_body = "{\"status\":\"ERROR\",\"message\":\"Missing service_name\"}";
        } else {
            response_body = execute_service_call(service_name, param);
        }
    }
    else {
        http_status = "404 Not Found";
        response_body = "{\"error\":\"Path not found\"}";
    }

    std::ostringstream http_response;
    http_response << "HTTP/1.1 " << http_status << "\r\n"
                  << "Content-Type: application/json\r\n"
                  << "Content-Length: " << response_body.size() << "\r\n"
                  << "Access-Control-Allow-Origin: *\r\n"
                  << "Connection: close\r\n\r\n"
                  << response_body;

    std::string full_resp = http_response.str();
    send(client_socket, full_resp.c_str(), static_cast<int>(full_resp.size()), 0);
    closesocket(client_socket);
}

int main(int argc, char* argv[]) {
    int port = DEFAULT_PORT;
    if (argc > 1) g_rpi_id = argv[1];
    if (argc > 2) port = std::atoi(argv[2]);

    init_hardware();

#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "[RPi-1] Failed to create socket." << std::endl;
        return 1;
    }

    int opt = 1;
#ifdef _WIN32
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "[RPi-1] Bind failed on port " << port << std::endl;
        closesocket(server_socket);
        return 1;
    }

    if (listen(server_socket, 10) == SOCKET_ERROR) {
        std::cerr << "[RPi-1] Listen failed." << std::endl;
        closesocket(server_socket);
        return 1;
    }

    std::cout << "==================================================" << std::endl;
    std::cout << "  RPi-1 Service Server (Thermostat & Cooling Fan)" << std::endl;
    std::cout << "  Node Identifier: " << g_rpi_id << std::endl;
    std::cout << "  Listening on:    http://0.0.0.0:" << port << std::endl;
    std::cout << "  Services:        Read_Temperature, Set_Cooling_Indicator" << std::endl;
    std::cout << "==================================================" << std::endl;

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        SOCKET client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket != INVALID_SOCKET) {
            std::thread(handle_client, client_socket).detach();
        }
    }

    closesocket(server_socket);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}

