#include "services.hpp"
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

// Helper function to extract a simple JSON string property
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

// Helper function to extract query parameter from URL query string
std::string extract_query_param(const std::string& query, const std::string& key) {
    std::string needle = key + "=";
    size_t pos = query.find(needle);
    if (pos == std::string::npos) return "";
    pos += needle.length();
    size_t end = query.find('&', pos);
    if (end == std::string::npos) end = query.length();
    return query.substr(pos, end - pos);
}

// Service dispatcher function
std::string execute_service_call(const std::string& service_name, const std::string& arg_str) {
    std::ostringstream response;
    std::string logical_rpi = "RPi-1";
    if (service_name == "Read_Light_Level" || service_name == "Set_Alarm_LED") {
        logical_rpi = "RPi-2";
    }

    if (service_name == "Calibrate_Sensors") {
        int res = Calibrate_Sensors();
        response << "{\"status\":\"SUCCESS\",\"service\":\"" << service_name 
                 << "\",\"rpi_id\":\"" << logical_rpi << "\",\"return_value\":" << res << "}";
    }
    else if (service_name == "Read_Light_Level") {
        int res = Read_Light_Level();
        response << "{\"status\":\"SUCCESS\",\"service\":\"" << service_name 
                 << "\",\"rpi_id\":\"RPi-2\",\"return_value\":" << res << "}";
    }
    else if (service_name == "Read_Temperature") {
        char scale = 'C';
        if (!arg_str.empty() && (arg_str[0] == 'F' || arg_str[0] == 'f')) {
            scale = 'F';
        }
        int res = Read_Temperature(scale);
        response << "{\"status\":\"SUCCESS\",\"service\":\"" << service_name 
                 << "\",\"rpi_id\":\"RPi-1\",\"scale\":\"" << scale << "\",\"return_value\":" << res << "}";
    }
    else if (service_name == "Set_Alarm_LED") {
        int state = 0;
        if (!arg_str.empty()) {
            state = std::atoi(arg_str.c_str());
        }
        Set_Alarm_LED(state);
        response << "{\"status\":\"SUCCESS\",\"service\":\"" << service_name 
                 << "\",\"rpi_id\":\"RPi-2\",\"state\":" << state << ",\"return_value\":\"void\"}";
    }
    else if (service_name == "Set_Cooling_Indicator") {
        int state = 0;
        if (!arg_str.empty()) {
            state = std::atoi(arg_str.c_str());
        }
        Set_Cooling_Indicator(state);
        response << "{\"status\":\"SUCCESS\",\"service\":\"" << service_name 
                 << "\",\"rpi_id\":\"RPi-1\",\"state\":" << state << ",\"return_value\":\"void\"}";
    }
    else {
        response << "{\"status\":\"ERROR\",\"message\":\"Unknown service: " << service_name << "\"}";
    }

    return response.str();
}

// Handle HTTP client request
void handle_client(SOCKET client_socket) {
    std::string request;
    char buffer[4096];
    size_t content_length = 0;
    bool headers_complete = false;

    // Loop until full HTTP request including Content-Length body is received
    while (true) {
        int bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) break;
        request.append(buffer, bytes);

        if (!headers_complete) {
            size_t header_end = request.find("\r\n\r\n");
            if (header_end != std::string::npos) {
                headers_complete = true;
                // Parse Content-Length header case-insensitively
                std::string lower_req = request;
                std::transform(lower_req.begin(), lower_req.end(), lower_req.begin(), ::tolower);
                size_t cl_pos = lower_req.find("content-length:");
                if (cl_pos != std::string::npos) {
                    size_t val_start = cl_pos + 15;
                    while (val_start < lower_req.size() && lower_req[val_start] == ' ') val_start++;
                    size_t val_end = lower_req.find("\r\n", val_start);
                    if (val_end != std::string::npos) {
                        content_length = std::stoul(lower_req.substr(val_start, val_end - val_start));
                    }
                }
            }
        }

        if (headers_complete) {
            size_t header_end = request.find("\r\n\r\n");
            size_t body_len = request.size() - (header_end + 4);
            if (body_len >= content_length) {
                break; // Fully received headers and body
            }
        }
    }

    if (request.empty()) {
        closesocket(client_socket);
        return;
    }

    // Extract HTTP method and raw path
    std::string method, raw_path;
    std::istringstream req_stream(request);
    req_stream >> method >> raw_path;

    // Separate clean path from query string
    std::string path = raw_path;
    std::string query_string;
    size_t q_pos = raw_path.find('?');
    if (q_pos != std::string::npos) {
        path = raw_path.substr(0, q_pos);
        query_string = raw_path.substr(q_pos + 1);
    }

    std::string response_body;
    std::string content_type = "application/json";
    int status_code = 200;

    // CORS preflight OPTIONS request
    if (method == "OPTIONS") {
        std::ostringstream oss;
        oss << "HTTP/1.1 204 No Content\r\n"
            << "Access-Control-Allow-Origin: *\r\n"
            << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
            << "Access-Control-Allow-Headers: Content-Type\r\n\r\n";
        std::string reply = oss.str();
        send(client_socket, reply.c_str(), static_cast<int>(reply.size()), 0);
        closesocket(client_socket);
        return;
    }

    if (method == "GET" && (path == "/services" || path == "/api/services")) {
        // Return available service vectors
        auto services = get_available_services(g_rpi_id);
        std::ostringstream json;
        json << "{\"rpi_id\":\"" << g_rpi_id << "\",\"vectors\":[";
        for (size_t i = 0; i < services.size(); ++i) {
            json << "\"" << services[i].to_vector_string() << "\"";
            if (i + 1 < services.size()) json << ",";
        }
        json << "],\"services\":[";
        for (size_t i = 0; i < services.size(); ++i) {
            json << services[i].to_json();
            if (i + 1 < services.size()) json << ",";
        }
        json << "]}";
        response_body = json.str();
    }
    else if (path == "/call" || path == "/api/call") {
        // Find body after double CRLF
        size_t body_pos = request.find("\r\n\r\n");
        std::string body = (body_pos != std::string::npos) ? request.substr(body_pos + 4) : "";

        // 1. Extract from JSON body
        std::string service_name = extract_json_field(body, "service_name");
        std::string param_val = extract_json_field(body, "param");
        if (param_val.empty()) {
            param_val = extract_json_field(body, "value");
        }

        // 2. Fallback to URL query parameters
        if (service_name.empty() && !query_string.empty()) {
            service_name = extract_query_param(query_string, "service_name");
            if (service_name.empty()) service_name = extract_query_param(query_string, "service");
            if (param_val.empty()) param_val = extract_query_param(query_string, "param");
            if (param_val.empty()) param_val = extract_query_param(query_string, "value");
        }

        std::cout << "[Server] Executing Service Call: '" << service_name 
                  << "' | Param: '" << param_val << "'" << std::endl;

        response_body = execute_service_call(service_name, param_val);
    }
    else if (method == "GET" && path == "/status") {
        response_body = "{\"status\":\"RUNNING\",\"rpi_id\":\"" + g_rpi_id + "\"}";
    }
    else {
        status_code = 404;
        response_body = "{\"status\":\"ERROR\",\"message\":\"Endpoint not found: " + path + "\"}";
    }

    std::ostringstream http_response;
    http_response << "HTTP/1.1 " << status_code << " OK\r\n"
                  << "Content-Type: " << content_type << "\r\n"
                  << "Access-Control-Allow-Origin: *\r\n"
                  << "Content-Length: " << response_body.size() << "\r\n"
                  << "Connection: close\r\n\r\n"
                  << response_body;

    std::string out = http_response.str();
    send(client_socket, out.c_str(), static_cast<int>(out.size()), 0);
    closesocket(client_socket);
}

int main(int argc, char* argv[]) {
    int port = DEFAULT_PORT;
    if (argc > 1) {
        g_rpi_id = argv[1];
    }
    if (argc > 2) {
        port = std::atoi(argv[2]);
    }

    std::cout << "====================================================\n";
    std::cout << "  Virtual Smart Space - RPi Microservice Server     \n";
    std::cout << "  B8019: Programming the Internet of Things (Lab 4) \n";
    std::cout << "====================================================\n";
    std::cout << "RPi ID: " << g_rpi_id << " | Port: " << port << std::endl;

    // Initialize hardware pins & ADC
    init_hardware();

    // Print advertised service vectors
    auto services = get_available_services(g_rpi_id);
    std::cout << "\nAdvertised Service Vectors to Edge:\n";
    for (const auto& s : services) {
        std::cout << "  * " << s.to_vector_string() << " -> " << s.description << "\n";
    }
    std::cout << "----------------------------------------------------\n";

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock.\n";
        return 1;
    }
#endif

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket.\n";
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
        std::cerr << "Bind failed on port " << port << ".\n";
        closesocket(server_socket);
        return 1;
    }

    if (listen(server_socket, 10) == SOCKET_ERROR) {
        std::cerr << "Listen failed.\n";
        closesocket(server_socket);
        return 1;
    }

    std::cout << "[Server] Listening for Edge service calls on port " << port << "...\n";

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        SOCKET client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == INVALID_SOCKET) {
            continue;
        }

        // Handle in worker thread to prevent blocking
        std::thread([client_socket]() {
            handle_client(client_socket);
        }).detach();
    }

    closesocket(server_socket);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
